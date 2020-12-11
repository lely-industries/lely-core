/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * polling interface.
 *
 * @see lely/io/poll.h
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#if !LELY_NO_STDIO

#include <lely/io/poll.h>
#include <lely/util/cmp.h>
#include <lely/util/errnum.h>
#include <lely/util/rbtree.h>
#if _WIN32
#include <lely/io/sock.h>
#else
#include <lely/io/pipe.h>
#endif
#include "handle.h"

#include <assert.h>
#include <stdlib.h>

#if _POSIX_C_SOURCE >= 200112L
#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
#include <sys/epoll.h>
#else
#include <poll.h>
#endif
#endif

/// An I/O polling interface.
struct __io_poll {
#if !LELY_NO_THREADS
	/// The mutex protecting #tree.
	mtx_t mtx;
#endif
	/// The tree containing the I/O device handles being watched.
	struct rbtree tree;
#if _WIN32 || _POSIX_C_SOURCE >= 200112L
	/// A self-pipe used to generate signal events.
	io_handle_t pipe[2];
#endif
#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
	/// The epoll file descriptor.
	int epfd;
#endif
};

/// The attributes of an I/O device handle being watched.
struct io_watch {
	/// The node in the tree of file descriptors.
	struct rbnode node;
	/// A pointer to the I/O device handle.
	struct io_handle *handle;
	/// The events being watched.
	struct io_event event;
	/**
	 * A flag indicating whether to keep watching the file descriptor after
	 * an event occurs.
	 */
	int keep;
};

#if LELY_NO_THREADS
#define io_poll_lock(poll)
#define io_poll_unlock(poll)
#else
static void io_poll_lock(io_poll_t *poll);
static void io_poll_unlock(io_poll_t *poll);
#endif

static struct io_watch *io_poll_insert(
		io_poll_t *poll, struct io_handle *handle);
static void io_poll_remove(io_poll_t *poll, struct io_watch *watch);

#if _POSIX_C_SOURCE >= 200112L \
		&& !(defined(__linux__) && defined(HAVE_SYS_EPOLL_H))
static int _poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif

void *
__io_poll_alloc(void)
{
	void *ptr = malloc(sizeof(struct __io_poll));
	if (!ptr)
		set_errc(errno2c(errno));
	return ptr;
}

void
__io_poll_free(void *ptr)
{
	free(ptr);
}

struct __io_poll *
__io_poll_init(struct __io_poll *poll)
{
	assert(poll);

	int errc = 0;

#if !LELY_NO_THREADS
	mtx_init(&poll->mtx, mtx_plain);
#endif

#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
	// Track attributes with the I/O device handle.
	rbtree_init(&poll->tree, ptr_cmp);
#else
	// Track attributes with native file descriptor.
#if _WIN32
	rbtree_init(&poll->tree, ptr_cmp);
#else
	rbtree_init(&poll->tree, int_cmp);
#endif
#endif

#if _WIN32 || _POSIX_C_SOURCE >= 200112L
	// Create a self-pipe for signal events.
#if _WIN32
	if (io_open_socketpair(IO_SOCK_IPV4, IO_SOCK_STREAM, poll->pipe)
			== -1) {
#else
	if (io_open_pipe(poll->pipe) == -1) {
#endif
		errc = get_errc();
		goto error_open_pipe;
	}

	// Make the both ends of the self-pipe non-blocking.
	if (io_set_flags(poll->pipe[0], IO_FLAG_NONBLOCK) == -1) {
		errc = get_errc();
		goto error_set_flags;
	}
	if (io_set_flags(poll->pipe[1], IO_FLAG_NONBLOCK) == -1) {
		errc = get_errc();
		goto error_set_flags;
	}
#endif

#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
	poll->epfd = epoll_create1(EPOLL_CLOEXEC);
	if (poll->epfd == -1) {
		errc = get_errc();
		goto error_epoll_create1;
	}

	// Register the read end of the self-pipe with epoll.
	struct epoll_event ev = { .events = EPOLLIN,
		.data.ptr = poll->pipe[0] };
	if (epoll_ctl(poll->epfd, EPOLL_CTL_ADD, poll->pipe[0]->fd, &ev)
			== -1) {
		errc = get_errc();
		goto error_epoll_ctl;
	}
#endif

	return poll;

#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
error_epoll_ctl:
	close(poll->epfd);
error_epoll_create1:
#endif
#if _WIN32 || _POSIX_C_SOURCE >= 200112L
error_set_flags:
	io_close(poll->pipe[1]);
	io_close(poll->pipe[0]);
error_open_pipe:
#endif
	set_errc(errc);
	return NULL;
}

void
__io_poll_fini(struct __io_poll *poll)
{
	assert(poll);

	rbtree_foreach (&poll->tree, node)
		io_poll_remove(poll, structof(node, struct io_watch, node));

#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
	close(poll->epfd);
#endif

#if _WIN32 || _POSIX_C_SOURCE >= 200112L
	io_close(poll->pipe[1]);
	io_close(poll->pipe[0]);
#endif

#if !LELY_NO_THREADS
	mtx_destroy(&poll->mtx);
#endif
}

io_poll_t *
io_poll_create(void)
{
	int errc = 0;

	io_poll_t *poll = __io_poll_alloc();
	if (!poll) {
		errc = get_errc();
		goto error_alloc_poll;
	}

	if (!__io_poll_init(poll)) {
		errc = get_errc();
		goto error_init_poll;
	}

	return poll;

error_init_poll:
	__io_poll_free(poll);
error_alloc_poll:
	set_errc(errc);
	return NULL;
}

void
io_poll_destroy(io_poll_t *poll)
{
	if (poll) {
		__io_poll_fini(poll);
		__io_poll_free(poll);
	}
}

int
io_poll_watch(io_poll_t *poll, io_handle_t handle, struct io_event *event,
		int keep)
{
	assert(poll);

	if (!handle) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	switch (handle->vtab->type) {
#if defined(__linux__) && defined(HAVE_LINUX_CAN_H)
	case IO_TYPE_CAN:
#endif
#if _POSIX_C_SOURCE >= 200112L
	case IO_TYPE_FILE:
	case IO_TYPE_PIPE:
	case IO_TYPE_SERIAL:
#endif
#if _WIN32 || _POSIX_C_SOURCE >= 200112L
	case IO_TYPE_SOCK:
#endif
		break;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}

	int errc = 0;

	io_poll_lock(poll);

	// Check if the I/O device has already been registered.
#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
	struct rbnode *node = rbtree_find(&poll->tree, handle);
#else
	struct rbnode *node = rbtree_find(&poll->tree, &handle->fd);
#endif
	struct io_watch *watch =
			node ? structof(node, struct io_watch, node) : NULL;
	// If event is not NULL, register the device or update the events being
	// watched. If event is NULL, remove the device.
	if (event) {
		if (!watch) {
			watch = io_poll_insert(poll, handle);
			if (!watch) {
				errc = get_errc();
				goto error_watch;
			}
		}

		// Update the events being watched.
		watch->event = *event;
		watch->keep = keep;

#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
		// Modify or add the event to the epoll instance depending on
		// whether the file descriptor is already registered.
		int op = node ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

		struct epoll_event ev = { 0, { NULL } };
		if (event->events & IO_EVENT_READ)
			ev.events |= EPOLLIN | EPOLLRDHUP | EPOLLPRI;
		if (event->events & IO_EVENT_WRITE)
			ev.events |= EPOLLOUT;
		ev.data.ptr = watch->handle;

		if (epoll_ctl(poll->epfd, op, watch->handle->fd, &ev) == -1) {
			errc = get_errc();
			goto error_epoll_ctl;
		}
#endif
	} else {
		if (!watch) {
			errc = errnum2c(ERRNUM_INVAL);
			goto error_watch;
		}

#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
		// Delete the event from the epoll instance.
		epoll_ctl(poll->epfd, EPOLL_CTL_DEL, watch->handle->fd, NULL);
#endif
		io_poll_remove(poll, watch);
	}

	io_poll_unlock(poll);

	return 0;

#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
error_epoll_ctl:
	epoll_ctl(poll->epfd, EPOLL_CTL_DEL, watch->handle->fd, NULL);
#endif
	io_poll_remove(poll, watch);
error_watch:
	io_poll_unlock(poll);
	set_errc(errc);
	return -1;
}

int
io_poll_wait(io_poll_t *poll, int maxevents, struct io_event *events,
		int timeout)
{
	assert(poll);

	if (maxevents < 0) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (!maxevents || !events)
		return 0;

	int nevents = 0;
#if _WIN32 || _POSIX_C_SOURCE >= 200112L
	unsigned char sig = 0;
#endif

#if _WIN32
	fd_set readfds;
	FD_ZERO(&readfds);

	int nwritefds = 0;
	fd_set writefds;
	FD_ZERO(&writefds);

	fd_set errorfds;
	FD_ZERO(&errorfds);

	FD_SET((SOCKET)poll->pipe[0]->fd, &readfds);

	io_poll_lock(poll);
	struct rbnode *node = rbtree_first(&poll->tree);
	while (node) {
		struct io_watch *watch = structof(node, struct io_watch, node);
		node = rbnode_next(node);
		// Skip abandoned device handles.
		if (io_handle_unique(watch->handle)) {
			io_poll_remove(poll, watch);
			continue;
		}

		SOCKET fd = (SOCKET)watch->handle->fd;
		if (watch->event.events & IO_EVENT_READ)
			FD_SET(fd, &readfds);
		if (watch->event.events & IO_EVENT_WRITE) {
			nwritefds++;
			FD_SET(fd, &writefds);
		}
		FD_SET(fd, &errorfds);
	}
	io_poll_unlock(poll);

	struct timeval tv = { .tv_sec = timeout / 1000,
		.tv_usec = (timeout % 1000) * 1000 };
	int result = select(0, &readfds, nwritefds ? &writefds : NULL,
			&errorfds, timeout >= 0 ? &tv : NULL);
	if (result == -1)
		return -1;

	// Check the read end of the self-pipe.
	if (FD_ISSET((SOCKET)poll->pipe[0]->fd, &readfds))
		sig = 1;

	io_poll_lock(poll);
	node = rbtree_first(&poll->tree);
	while (node && nevents < maxevents) {
		struct io_watch *watch = structof(node, struct io_watch, node);
		node = rbnode_next(node);
		// Skip abandoned device handles.
		if (io_handle_unique(watch->handle)) {
			io_poll_remove(poll, watch);
			continue;
		}

		events[nevents].events = 0;
		if (FD_ISSET((SOCKET)watch->handle->fd, &readfds)
				&& (watch->event.events & IO_EVENT_READ))
			events[nevents].events |= IO_EVENT_READ;
		if (FD_ISSET((SOCKET)watch->handle->fd, &writefds)
				&& (watch->event.events & IO_EVENT_WRITE))
			events[nevents].events |= IO_EVENT_WRITE;
		if (FD_ISSET((SOCKET)watch->handle->fd, &errorfds))
			events[nevents].events |= IO_EVENT_ERROR;
		// Ignore non-events.
		if (!events[nevents].events)
			continue;

		events[nevents].u = watch->event.u;
		nevents++;

		if (!watch->keep)
			io_poll_remove(poll, watch);
	}
	io_poll_unlock(poll);
#elif _POSIX_C_SOURCE >= 200112L
#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
	struct epoll_event ev[maxevents];
	int nfds;
	int errsv = errno;
	do {
		errno = errsv;
		nfds = epoll_wait(poll->epfd, ev, maxevents,
				timeout >= 0 ? timeout : -1);
	} while (nfds == -1 && errno == EINTR);
	if (nfds == -1)
		return -1;

	io_poll_lock(poll);
	for (int i = 0; i < nfds; i++) {
		// Ignore signal events; they are handled below.
		if (ev[i].data.ptr == poll->pipe[0]) {
			sig = 1;
			continue;
		}

		struct rbnode *node = rbtree_find(&poll->tree, ev[i].data.ptr);
		if (!node)
			continue;
		struct io_watch *watch = structof(node, struct io_watch, node);

		if (!io_handle_unique(watch->handle)) {
			events[nevents].events = 0;
			// We consider hang up and high-priority (OOB) data an
			// error.
			// clang-format off
			if (ev[i].events & (EPOLLRDHUP | EPOLLPRI | EPOLLERR
					| EPOLLHUP))
				// clang-format on
				events[nevents].events |= IO_EVENT_ERROR;
			if (ev[i].events & EPOLLIN)
				events[nevents].events |= IO_EVENT_READ;
			if (ev[i].events & EPOLLOUT)
				events[nevents].events |= IO_EVENT_WRITE;
			events[nevents].u = watch->event.u;
			nevents++;
		}

		if (io_handle_unique(watch->handle) || !watch->keep) {
			epoll_ctl(poll->epfd, EPOLL_CTL_DEL, watch->handle->fd,
					NULL);
			io_poll_remove(poll, watch);
		}
	}
	io_poll_unlock(poll);
#else
	io_poll_lock(poll);
	struct pollfd fds[rbtree_size(&poll->tree) + 1];
	nfds_t nfds = 0;
	// Watch the read end of the self-pipe.
	fds[nfds].fd = poll->pipe[0]->fd;
	fds[nfds].events = POLLIN;
	nfds++;
	struct rbnode *node = rbtree_first(&poll->tree);
	while (node) {
		struct io_watch *watch = structof(node, struct io_watch, node);
		node = rbnode_next(node);
		// Skip abandoned device handles.
		if (io_handle_unique(watch->handle)) {
			io_poll_remove(poll, watch);
			continue;
		}

		fds[nfds].fd = watch->handle->fd;
		fds[nfds].events = 0;
		if (watch->event.events & IO_EVENT_READ)
			fds[nfds].events |= POLLIN | POLLPRI;
		if (watch->event.events & IO_EVENT_WRITE)
			fds[nfds].events |= POLLOUT;
		nfds++;
	}
	io_poll_unlock(poll);

	int n;
	int errsv = errno;
	do {
		errno = errsv;
		n = _poll(fds, nfds, timeout >= 0 ? timeout : -1);
	} while (n == -1 && errno == EINTR);
	if (n == -1)
		return -1;
	maxevents = MIN(n, maxevents);

	io_poll_lock(poll);
	for (nfds_t nfd = 0; nfd < nfds && nevents < maxevents; nfd++) {
		// Ignore signal events; they are handled below.
		if (fds[nfd].fd == poll->pipe[0]->fd) {
			sig = 1;
			continue;
		}

		events[nevents].events = 0;
		// We consider hang up and high-priority (OOB) data an error.
		if (fds[nfd].revents & (POLLPRI | POLLERR | POLLHUP | POLLNVAL))
			events[nevents].events |= IO_EVENT_ERROR;
		// We don't distinguish between normal and high-priority data.
		if (fds[nfd].revents & POLLIN)
			events[nevents].events |= IO_EVENT_READ;
		if (fds[nfd].revents & POLLOUT)
			events[nevents].events |= IO_EVENT_WRITE;
		// Ignore non-events.
		if (!events[nevents].events)
			continue;

		struct rbnode *node = rbtree_find(&poll->tree, &fds[nfd].fd);
		if (!node)
			continue;
		struct io_watch *watch = structof(node, struct io_watch, node);

		if (!io_handle_unique(watch->handle)) {
			events[nevents].u = watch->event.u;
			nevents++;
		}

		if (io_handle_unique(watch->handle) || !watch->keep)
			io_poll_remove(poll, watch);
	}
	io_poll_unlock(poll);
#endif // __linux__ && HAVE_SYS_EPOLL_H
#else
	(void)timeout;
#endif // _WIN32

#if _WIN32 || _POSIX_C_SOURCE >= 200112L
	if (sig) {
		// If one or more signals were received, generate the
		// corresponding events.
		while (nevents < maxevents
				&& io_read(poll->pipe[0], &sig, 1) == 1) {
			events[nevents].events = IO_EVENT_SIGNAL;
			events[nevents].u.sig = sig;
			nevents++;
		}
	}
#endif

	return nevents;
}

#if _WIN32 || _POSIX_C_SOURCE >= 200112L
int
io_poll_signal(io_poll_t *poll, unsigned char sig)
{
	assert(poll);

	return io_write(poll->pipe[1], &sig, 1) == 1 ? 0 : -1;
}
#endif

#if !LELY_NO_THREADS

static void
io_poll_lock(io_poll_t *poll)
{
	assert(poll);

	mtx_lock(&poll->mtx);
}

static void
io_poll_unlock(io_poll_t *poll)
{
	assert(poll);

	mtx_unlock(&poll->mtx);
}

#endif // !LELY_NO_THREADS

static struct io_watch *
io_poll_insert(io_poll_t *poll, struct io_handle *handle)
{
	assert(poll);
	assert(handle);

	struct io_watch *watch = malloc(sizeof(*watch));
	if (!watch)
		return NULL;

	watch->handle = io_handle_acquire(handle);
#if defined(__linux__) && defined(HAVE_SYS_EPOLL_H)
	watch->node.key = watch->handle;
#else
	watch->node.key = &watch->handle->fd;
#endif
	rbtree_insert(&poll->tree, &watch->node);

	return watch;
}

static void
io_poll_remove(io_poll_t *poll, struct io_watch *watch)
{
	assert(poll);
	assert(watch);

	struct io_handle *handle = watch->handle;
	rbtree_remove(&poll->tree, &watch->node);
	free(watch);
	io_handle_release(handle);
}

#if _POSIX_C_SOURCE >= 200112L \
		&& !(defined(__linux__) && defined(HAVE_SYS_EPOLL_H))
static int
_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	return poll(fds, nfds, timeout);
}
#endif

#endif // !LELY_NO_STDIO
