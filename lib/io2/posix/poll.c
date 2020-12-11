/**@file
 * This file is part of the I/O library; it contains the I/O polling
 * implementation for POSIX platforms.
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

#if !LELY_NO_STDIO && _POSIX_C_SOURCE >= 200112L

#include <lely/io2/posix/poll.h>
#include <lely/util/dllist.h>
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

#ifndef LELY_HAVE_PPOLL
// TODO: ppoll() is also supported by DragonFly 4.6 and OpenBSD 5.4. On
// NetBSD 3.0 this function is called pollts().
#if defined(__linux__) || (__FreeBSD__ >= 11)
#define LELY_HAVE_PPOLL 1
#endif
#endif

#if LELY_HAVE_PPOLL
#include <poll.h>
#else
#include <sys/select.h>
#endif

#if LELY_HAVE_PPOLL
#ifndef LELY_IO_PPOLL_NFDS
#define LELY_IO_PPOLL_NFDS MAX((LELY_VLA_SIZE_MAX / sizeof(struct pollfd)), 1)
#endif
#endif

struct io_poll_thrd {
	int stopped;
#if !LELY_NO_THREADS
	pthread_t *thread;
	struct dlnode node;
#endif
};

// clang-format off
static const struct io_svc_vtbl io_poll_svc_vtbl = {
	NULL,
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
#if !LELY_NO_THREADS
	pthread_mutex_t mtx;
#endif
	struct rbtree tree;
#if !LELY_NO_THREADS
	struct dllist threads;
#endif
};

static inline io_poll_t *io_poll_from_svc(const struct io_svc *svc);
static inline io_poll_t *io_poll_from_poll(const ev_poll_t *poll);

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

#if !LELY_NO_THREADS
	if ((errsv = pthread_mutex_init(&poll->mtx, NULL)))
		goto error_init_mtx;
#endif

	rbtree_init(&poll->tree, &io_fd_cmp);

#if !LELY_NO_THREADS
	dllist_init(&poll->threads);
#endif

	io_ctx_insert(poll->ctx, &poll->svc);

	return poll;

#if !LELY_NO_THREADS
	// pthread_mutex_destroy(&poll->mtx);
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

#if !LELY_NO_THREADS
	assert(dllist_empty(&poll->threads));
#endif

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

	if (fd == -1) {
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
	if (node) {
		if (node != &watch->_node) {
			errsv = EEXIST;
			goto error;
		}
		if (!events)
			rbtree_remove(&poll->tree, node);
	} else if (events) {
#if LELY_HAVE_PPOLL
		if (rbtree_size(&poll->tree) >= (nfds_t)~0) {
#else
		if (rbtree_size(&poll->tree) >= FD_SETSIZE) {
#endif
			errsv = EINVAL;
			goto error;
		}
		watch->_fd = fd;
		rbnode_init(&watch->_node, &watch->_fd);
		rbtree_insert(&poll->tree, &watch->_node);
	}

	if (events != watch->_events) {
		watch->_events = events;
#if !LELY_NO_THREADS
		// Wake up all polling threads.
		dllist_foreach (&poll->threads, node) {
			struct io_poll_thrd *thr = structof(
					node, struct io_poll_thrd, node);
			if (!thr->stopped) {
				thr->stopped = 1;
				pthread_kill(*thr->thread, poll->signo);
			}
		}
#endif
	}

	result = 0;

error:
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&poll->mtx);
#endif
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
	static _Thread_local struct io_poll_thrd thr = { 0, NULL, DLNODE_INIT };
	if (!thr.thread) {
		static _Thread_local pthread_t thread;
		thread = pthread_self();
		thr.thread = &thread;
	}
#endif
	return &thr;
}

static int
io_poll_poll_wait(ev_poll_t *poll_, int timeout_)
{
	io_poll_t *poll = io_poll_from_poll(poll_);
	void *thr_ = io_poll_poll_self(poll_);
	struct io_poll_thrd *thr = (struct io_poll_thrd *)thr_;

	struct timespec tv = { 0, 0 };
	if (timeout_ > 0) {
		tv.tv_sec = timeout_ / 1000;
		tv.tv_nsec = (timeout_ % 1000) * 1000000l;
	}
	struct timespec *timeout = timeout_ >= 0 ? &tv : NULL;

	int n = 0;
	int errsv = errno;

	sigset_t set;
	sigemptyset(&set);

#if LELY_HAVE_PPOLL
	struct pollfd fds_[LELY_IO_PPOLL_NFDS];
	struct pollfd *fds = fds_;
#else
	fd_set rfds, wfds, efds;
#endif

#if !LELY_NO_THREADS
	pthread_mutex_lock(&poll->mtx);
	dllist_push_back(&poll->threads, &thr->node);
#endif
	if (!timeout_)
		thr->stopped = 1;

	int stopped = 0;
	do {
		if (thr->stopped) {
			if (rbtree_empty(&poll->tree))
				break;
			timeout = &tv;
			tv = (struct timespec){ 0, 0 };
		}

#if LELY_HAVE_PPOLL
		assert(rbtree_size(&poll->tree) <= (nfds_t)~0);
		if (fds != fds_)
			free(fds);
		fds = fds_;
		nfds_t nfds = rbtree_size(&poll->tree);
		if (nfds > LELY_IO_PPOLL_NFDS) {
			fds = malloc(nfds * sizeof(*fds));
			if (!fds) {
				errsv = errno;
				n = -1;
				break;
			}
		}

		nfds_t i = 0;
		rbtree_foreach (&poll->tree, node) {
			struct io_poll_watch *watch =
					io_poll_watch_from_node(node);
			int events = 0;
			if (watch->_events & IO_EVENT_IN)
				events |= POLLIN;
			if (watch->_events & IO_EVENT_PRI)
				events |= POLLRDBAND | POLLPRI;
			if (watch->_events & IO_EVENT_OUT)
				events |= POLLOUT;
			fds[i++] = (struct pollfd){ .fd = watch->_fd,
				.events = events };
		}
		assert(i == nfds);
#else
		assert(rbtree_size(&poll->tree) <= FD_SETSIZE);
		FD_ZERO(&rfds);
		int nrfds = 0;
		FD_ZERO(&wfds);
		int nwfds = 0;
		FD_ZERO(&efds);
		int nefds = 0;

		rbtree_foreach (&poll->tree, node) {
			struct io_poll_watch *watch =
					io_poll_watch_from_node(node);
			if (watch->_events & IO_EVENT_IN) {
				FD_SET(watch->_fd, &rfds);
				nrfds = MAX(nrfds, watch->_fd + 1);
			}
			if (watch->_events & IO_EVENT_OUT) {
				FD_SET(watch->_fd, &wfds);
				nwfds = MAX(nwfds, watch->_fd + 1);
			}
			FD_SET(watch->_fd, &efds);
			nefds = MAX(nefds, watch->_fd + 1);
		}
#endif

#if !LELY_NO_THREADS
		pthread_mutex_unlock(&poll->mtx);
#endif
		errno = 0;
#if LELY_HAVE_PPOLL
		int result = ppoll(fds, nfds, timeout, &set);
#else
		int result = pselect(nefds, nrfds ? &rfds : NULL,
				nwfds ? &wfds : NULL, nefds ? &efds : NULL,
				timeout, &set);
#endif
#if !LELY_NO_THREADS
		pthread_mutex_lock(&poll->mtx);
#endif
		if (result == -1) {
			if (errno == EINTR) {
				// If the wait is interrupted by a signal, we
				// don't receive any events. This can result in
				// starvation if too many signals are generated
				// (e.g., when too many tasks are queued). To
				// prevent starvation, we poll for events again,
				// but this time with the interrupt signal
				// blocked (and timeout 0).
				sigaddset(&set, poll->signo);
				thr->stopped = 1;
				continue;
			} else {
				errsv = errno;
				n = -1;
				break;
			}
		}

#if LELY_HAVE_PPOLL
		for (i = 0; i < nfds; i++) {
			if (!fds[i].revents || (fds[i].revents & POLLNVAL))
				continue;
			struct rbnode *node =
					rbtree_find(&poll->tree, &fds[i].fd);
			if (!node)
				continue;
			struct io_poll_watch *watch =
					io_poll_watch_from_node(node);
			int revents = 0;
			if (fds[i].revents & POLLIN)
				revents |= IO_EVENT_IN;
			if (fds[i].revents & (POLLRDBAND | POLLPRI))
				revents |= IO_EVENT_PRI;
			if (fds[i].revents & POLLOUT)
				revents |= IO_EVENT_OUT;
			if (fds[i].revents & POLLERR)
				revents |= IO_EVENT_ERR;
			if (fds[i].revents & POLLHUP)
				revents |= IO_EVENT_HUP;
			io_poll_process(poll, revents, watch);
			n += n < INT_MAX;
		}
#else
		rbtree_foreach (&poll->tree, node) {
			struct io_poll_watch *watch =
					io_poll_watch_from_node(node);
			if (!result)
				break;
			int revents = 0;
			if (FD_ISSET(watch->_fd, &rfds))
				revents |= IO_EVENT_IN;
			if (FD_ISSET(watch->_fd, &wfds))
				revents |= IO_EVENT_OUT;
			if (FD_ISSET(watch->_fd, &efds)) {
				if (watch->_events & IO_EVENT_PRI)
					revents |= IO_EVENT_PRI;
				revents |= IO_EVENT_ERR;
			}
			if (revents) {
				result--;
				io_poll_process(poll, revents, watch);
				n += n < INT_MAX;
			}
		}
#endif

		stopped = thr->stopped = 1;
	} while (!stopped);
#if !LELY_NO_THREADS
	dllist_remove(&poll->threads, &thr->node);
#endif
	thr->stopped = 0;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&poll->mtx);
#endif

#if LELY_HAVE_PPOLL
	if (fds != fds_)
		free(fds);
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

static void
io_poll_process(io_poll_t *poll, int revents, struct io_poll_watch *watch)
{
	assert(poll);
	assert(watch);

	rbtree_remove(&poll->tree, &watch->_node);
	watch->_events = 0;
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

#endif // !LELY_NO_STDIO && _POSIX_C_SOURCE >= 200112L
