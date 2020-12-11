/**@file
 * This file is part of the I/O library; it contains the I/O polling
 * implementation for Linux.
 *
 * @see lely/io2/posix/poll.h
 *
 * @copyright 2015-2020 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "io.h"

#if !LELY_NO_STDIO && defined(__linux__)

#include <lely/io2/posix/poll.h>
#include <lely/util/util.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>

#if !LELY_NO_THREADS
#include <pthread.h>
#endif
#include <unistd.h>

#include <sys/epoll.h>

// clang-format off
#define EPOLL_EVENT_INIT(events, fd) \
	{ \
		(((events) & IO_EVENT_IN) ? (EPOLLIN | EPOLLRDHUP) : 0) \
				| (((events) & IO_EVENT_PRI) ? EPOLLPRI : 0) \
				| (((events) & IO_EVENT_OUT) ? EPOLLOUT : 0) \
				| EPOLLONESHOT, \
		{ .fd = (fd) } \
	}
// clang-format on

#ifndef LELY_IO_EPOLL_MAXEVENTS
#define LELY_IO_EPOLL_MAXEVENTS \
	MAX((LELY_VLA_SIZE_MAX / sizeof(struct epoll_event)), 1)
#endif

struct io_poll_thrd {
	int stopped;
#if !LELY_NO_THREADS
	pthread_t *thread;
#endif
};

static int io_poll_svc_notify_fork(struct io_svc *svc, enum io_fork_event e);

// clang-format off
static const struct io_svc_vtbl io_poll_svc_vtbl = {
	&io_poll_svc_notify_fork,
	NULL
};
// clang-format on

static void *io_poll_poll_self(const ev_poll_t *poll);
static int io_poll_poll_wait(ev_poll_t *poll, int timeout);
static int io_poll_poll_kill(ev_poll_t *poll, void *thr);

// clang-format off
static const struct ev_poll_vtbl io_poll_poll_vtbl = {
	&io_poll_poll_self,
	&io_poll_poll_wait,
	&io_poll_poll_kill
};
// clang-format on

struct io_poll {
	struct io_svc svc;
	const struct ev_poll_vtbl *poll_vptr;
	io_ctx_t *ctx;
	int signo;
	struct sigaction oact;
	sigset_t oset;
	int epfd;
#if !LELY_NO_THREADS
	pthread_mutex_t mtx;
#endif
	struct rbtree tree;
	size_t nwatch;
};

static inline io_poll_t *io_poll_from_svc(const struct io_svc *svc);
static inline io_poll_t *io_poll_from_poll(const ev_poll_t *poll);

static int io_poll_open(io_poll_t *poll);
static int io_poll_close(io_poll_t *poll);

static void io_poll_process(
		io_poll_t *poll, int revents, struct io_poll_watch *watch);

static int io_fd_cmp(const void *p1, const void *p2);

static inline struct io_poll_watch *io_poll_watch_from_node(
		struct rbnode *node);

static void sig_ign(int signo);

void *
io_poll_alloc(void)
{
	return malloc(sizeof(io_poll_t));
}

void
io_poll_free(void *ptr)
{
	free(ptr);
}

io_poll_t *
io_poll_init(io_poll_t *poll, io_ctx_t *ctx, int signo)
{
	assert(poll);
	assert(ctx);

	if (!signo)
		signo = SIGUSR1;

	int errsv = 0;

	poll->svc = (struct io_svc)IO_SVC_INIT(&io_poll_svc_vtbl);
	poll->ctx = ctx;

	poll->poll_vptr = &io_poll_poll_vtbl;

	poll->signo = signo;

	struct sigaction act;
	act.sa_handler = &sig_ign;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(poll->signo, &act, &poll->oact) == -1) {
		errsv = errno;
		goto error_sigaction;
	}

	// Block the wake up signal so it is only delivered during polling.
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, poll->signo);
#if LELY_NO_THREADS
	if (sigprocmask(SIG_BLOCK, &set, &poll->oset) == -1) {
		errsv = errno;
#else
	if ((errsv = pthread_sigmask(SIG_BLOCK, &set, &poll->oset))) {
#endif
		goto error_sigmask;
	}

	poll->epfd = -1;
#if !LELY_NO_THREADS
	if ((errsv = pthread_mutex_init(&poll->mtx, NULL)))
		goto error_init_mtx;
#endif

	rbtree_init(&poll->tree, &io_fd_cmp);
	poll->nwatch = 0;

	if (io_poll_open(poll) == -1) {
		errsv = errno;
		goto error_open;
	}

	io_ctx_insert(poll->ctx, &poll->svc);

	return poll;

	// io_poll_close(poll);
error_open:
#if !LELY_NO_THREADS
	pthread_mutex_destroy(&poll->mtx);
error_init_mtx:
#endif
#if LELY_NO_THREADS
	sigprocmask(SIG_SETMASK, &poll->oset, NULL);
#else
	pthread_sigmask(SIG_SETMASK, &poll->oset, NULL);
#endif
error_sigmask:
	sigaction(poll->signo, &poll->oact, NULL);
error_sigaction:
	errno = errsv;
	return NULL;
}

void
io_poll_fini(io_poll_t *poll)
{
	assert(poll);

	io_ctx_remove(poll->ctx, &poll->svc);

	io_poll_close(poll);

#if !LELY_NO_THREADS
	pthread_mutex_destroy(&poll->mtx);
#endif

	// Clear any pending (and currently blocked) wake up signal.
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, poll->signo);
	int errsv = errno;
	while (sigtimedwait(&set, &(siginfo_t){ 0 }, &(struct timespec){ 0, 0 })
			== poll->signo)
		;
	errno = errsv;
	// Unblock the wake up signal and restore the original signal handler.
#if LELY_NO_THREADS
	sigprocmask(SIG_SETMASK, &poll->oset, NULL);
#else
	pthread_sigmask(SIG_SETMASK, &poll->oset, NULL);
#endif
	sigaction(poll->signo, &poll->oact, NULL);
}

io_poll_t *
io_poll_create(io_ctx_t *ctx, int signo)
{
	int errsv = 0;

	io_poll_t *poll = io_poll_alloc();
	if (!poll) {
		errsv = errno;
		goto error_alloc;
	}

	io_poll_t *tmp = io_poll_init(poll, ctx, signo);
	if (!tmp) {
		errsv = errno;
		goto error_init;
	}
	poll = tmp;

	return poll;

error_init:
	io_poll_free(poll);
error_alloc:
	errno = errsv;
	return NULL;
}

void
io_poll_destroy(io_poll_t *poll)
{
	if (poll) {
		io_poll_fini(poll);
		io_poll_free(poll);
	}
}

io_ctx_t *
io_poll_get_ctx(const io_poll_t *poll)
{
	return poll->ctx;
}

ev_poll_t *
io_poll_get_poll(const io_poll_t *poll)
{
	return &poll->poll_vptr;
}

int
io_poll_watch(io_poll_t *poll, int fd, int events, struct io_poll_watch *watch)
{
	assert(poll);
	assert(watch);
	int epfd = poll->epfd;

	if (fd == -1 || fd == epfd) {
		errno = EBADF;
		return -1;
	}

	if (events < 0) {
		errno = EINVAL;
		return -1;
	}
	events &= IO_EVENT_MASK;

	int result = -1;
	int errsv = errno;
#if !LELY_NO_THREADS
	pthread_mutex_lock(&poll->mtx);
#endif

	struct rbnode *node = rbtree_find(&poll->tree, &fd);
	if (node && node != &watch->_node) {
		errsv = EEXIST;
		goto error;
	}

	if (events) {
		struct epoll_event event = EPOLL_EVENT_INIT(events, fd);
		if (node && events != watch->_events) {
			if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event) == -1) {
				errsv = errno;
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				rbtree_remove(&poll->tree, node);
				if (watch->_events)
					poll->nwatch--;
				watch->_events = 0;
				goto error;
			}
		} else if (!node) {
			if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
				errsv = errno;
				goto error;
			}
			watch->_fd = fd;
			rbnode_init(&watch->_node, &watch->_fd);
			watch->_events = 0;
			rbtree_insert(&poll->tree, &watch->_node);
		}
		if (!watch->_events)
			poll->nwatch++;
		watch->_events = events;
	} else if (node) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		rbtree_remove(&poll->tree, node);
		if (watch->_events)
			poll->nwatch--;
		watch->_events = 0;
	}

	result = 0;

error:
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&poll->mtx);
#endif
	errno = errsv;
	return result;
}

static int
io_poll_svc_notify_fork(struct io_svc *svc, enum io_fork_event e)
{
	io_poll_t *poll = io_poll_from_svc(svc);

	if (e != IO_FORK_CHILD)
		return 0;

	int result = 0;
	int errsv = errno;

	if (io_poll_close(poll) == -1) {
		errsv = errno;
		result = -1;
	}

	if (io_poll_open(poll) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	int epfd = poll->epfd;
	rbtree_foreach (&poll->tree, node) {
		struct io_poll_watch *watch = io_poll_watch_from_node(node);
		int fd = watch->_fd;
		int events = watch->_events;
		if (events) {
			struct epoll_event event = EPOLL_EVENT_INIT(events, fd);

			if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
				if (!result) {
					errsv = errno;
					result = -1;
				}
				rbtree_remove(&poll->tree, node);
			}
		} else {
			rbtree_remove(&poll->tree, node);
		}
	}

	errno = errsv;
	return result;
}

static void *
io_poll_poll_self(const ev_poll_t *poll)
{
	(void)poll;

#if LELY_NO_THREADS
	static struct io_poll_thrd thr = { 0 };
#else
	static _Thread_local struct io_poll_thrd thr = { 0, NULL };
	if (!thr.thread) {
		static _Thread_local pthread_t thread;
		thread = pthread_self();
		thr.thread = &thread;
	}
#endif
	return &thr;
}

static int
io_poll_poll_wait(ev_poll_t *poll_, int timeout)
{
	io_poll_t *poll = io_poll_from_poll(poll_);
	void *thr_ = io_poll_poll_self(poll_);
	struct io_poll_thrd *thr = (struct io_poll_thrd *)thr_;

	int n = 0;
	int errsv = errno;

	sigset_t set;
	sigemptyset(&set);

	struct epoll_event events[LELY_IO_EPOLL_MAXEVENTS];

#if !LELY_NO_THREADS
	pthread_mutex_lock(&poll->mtx);
#endif
	if (!timeout)
		thr->stopped = 1;

	int stopped = 0;
	do {
		if (thr->stopped) {
			if (!poll->nwatch)
				break;
			timeout = 0;
		}

#if !LELY_NO_THREADS
		pthread_mutex_unlock(&poll->mtx);
#endif
		errno = 0;
		int nevents = epoll_pwait(poll->epfd, events,
				LELY_IO_EPOLL_MAXEVENTS, timeout, &set);
		if (nevents == -1 && errno == EINTR) {
			// If the wait is interrupted by a signal, we don't
			// receive any events. This can result in starvation if
			// too many signals are generated (e.g., when too many
			// tasks are queued). To prevent starvation, we poll for
			// events again, but this time with the interrupt signal
			// blocked (and timeout 0).
			sigaddset(&set, poll->signo);
#if !LELY_NO_THREADS
			pthread_mutex_lock(&poll->mtx);
#endif
			thr->stopped = 1;
			continue;
		}
		if (nevents == -1) {
			if (!n) {
				errsv = errno;
				n = -1;
			}
#if !LELY_NO_THREADS
			pthread_mutex_lock(&poll->mtx);
#endif
			break;
		}

		for (int i = 0; i < nevents; i++) {
			int revents = 0;
			if (events[i].events & (EPOLLIN | EPOLLRDHUP))
				revents |= IO_EVENT_IN;
			if (events[i].events & EPOLLPRI)
				revents |= IO_EVENT_PRI;
			if (events[i].events & EPOLLOUT)
				revents |= IO_EVENT_OUT;
			if (events[i].events & EPOLLERR)
				revents |= IO_EVENT_ERR;
			if (events[i].events & EPOLLHUP)
				revents |= IO_EVENT_HUP;
			int fd = events[i].data.fd;
#if !LELY_NO_THREADS
			pthread_mutex_lock(&poll->mtx);
#endif
			struct rbnode *node = rbtree_find(&poll->tree, &fd);
			if (node) {
				struct io_poll_watch *watch =
						io_poll_watch_from_node(node);
				io_poll_process(poll, revents, watch);
				n += n < INT_MAX;
			}
#if !LELY_NO_THREADS
			pthread_mutex_unlock(&poll->mtx);
#endif
		}

#if !LELY_NO_THREADS
		pthread_mutex_lock(&poll->mtx);
#endif
		thr->stopped = 1;
		stopped = nevents != LELY_IO_EPOLL_MAXEVENTS;
	} while (!stopped);
	thr->stopped = 0;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&poll->mtx);
#endif

	errno = errsv;
	return n;
}

static int
io_poll_poll_kill(ev_poll_t *poll_, void *thr_)
{
#if !LELY_NO_THREADS
	io_poll_t *poll = io_poll_from_poll(poll_);
#endif
	struct io_poll_thrd *thr = thr_;
	assert(thr);

	if (thr_ == io_poll_poll_self(poll_))
		return 0;

#if !LELY_NO_THREADS
	pthread_mutex_lock(&poll->mtx);
#endif
	int stopped = thr->stopped;
	if (!stopped)
		thr->stopped = 1;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&poll->mtx);
	if (!stopped) {
		int errsv = pthread_kill(*thr->thread, poll->signo);
		if (errsv) {
			errno = errsv;
			return -1;
		}
	}
#endif
	return 0;
}

static inline io_poll_t *
io_poll_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, io_poll_t, svc);
}

static inline io_poll_t *
io_poll_from_poll(const ev_poll_t *poll)
{
	assert(poll);

	return structof(poll, io_poll_t, poll_vptr);
}

static int
io_poll_open(io_poll_t *poll)
{
	if (io_poll_close(poll) == -1)
		return -1;

	return (poll->epfd = epoll_create1(EPOLL_CLOEXEC)) == -1 ? -1 : 0;
}

static int
io_poll_close(io_poll_t *poll)
{
	assert(poll);

	int epfd = poll->epfd;
	if (epfd == -1)
		return 0;
	poll->epfd = -1;

	return close(epfd);
}

static void
io_poll_process(io_poll_t *poll, int revents, struct io_poll_watch *watch)
{
	assert(poll);
	assert(poll->nwatch);
	assert(watch);
	assert(watch->_events);

	watch->_events = 0;
	poll->nwatch--;

	if (watch->func) {
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&poll->mtx);
#endif
		watch->func(watch, revents);
#if !LELY_NO_THREADS
		pthread_mutex_lock(&poll->mtx);
#endif
	}
}

static int
io_fd_cmp(const void *p1, const void *p2)
{
	assert(p1);
	int fd1 = *(const int *)p1;
	assert(p2);
	int fd2 = *(const int *)p2;

	return (fd2 < fd1) - (fd1 < fd2);
}

static inline struct io_poll_watch *
io_poll_watch_from_node(struct rbnode *node)
{
	assert(node);

	return structof(node, struct io_poll_watch, _node);
}

static void
sig_ign(int signo)
{
	(void)signo;
}

#endif // !LELY_NO_STDIO && __linux__
