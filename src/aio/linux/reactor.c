/*!\file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * \see lely/aio/reactor.h
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include "../aio.h"

#ifdef __linux__

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/aio/context.h>
#include <lely/aio/reactor.h>
#include <lely/aio/self_pipe.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/epoll.h>

#ifndef LELY_AIO_EPOLL_MAXEVENTS
#define LELY_AIO_EPOLL_MAXEVENTS	64
#endif

static int aio_handle_cmp(const void *p1, const void *p2);

static aio_context_t *aio_reactor_impl_get_context(
		const aio_reactor_t *reactor);
static aio_poll_t *aio_reactor_impl_get_poll(const aio_reactor_t *reactor);
static int aio_reactor_impl_watch(aio_reactor_t *reactor,
		struct aio_watch *watch, aio_handle_t handle, int events);

static const struct aio_reactor_vtbl aio_reactor_impl_vtbl = {
	&aio_reactor_impl_get_context,
	&aio_reactor_impl_get_poll,
	&aio_reactor_impl_watch
};

static int aio_reactor_impl_service_notify_fork(struct aio_service *srv,
		enum aio_fork_event e);
static void aio_reactor_impl_service_shutdown(struct aio_service *srv);

static const struct aio_service_vtbl aio_reactor_impl_service_vtbl = {
	&aio_reactor_impl_service_notify_fork,
	&aio_reactor_impl_service_shutdown
};

static size_t aio_reactor_impl_poll_wait(aio_poll_t *poll, int timeout);
static void aio_reactor_impl_poll_stop(aio_poll_t *poll);

static const struct aio_poll_vtbl aio_reactor_impl_poll_vtbl = {
	&aio_reactor_impl_poll_wait,
	&aio_reactor_impl_poll_stop
};

struct aio_reactor_impl {
	const struct aio_reactor_vtbl *reactor_vptr;
	struct aio_service srv;
	const struct aio_poll_vtbl *poll_vptr;
	aio_context_t *ctx;
	int epfd;
	struct aio_self_pipe pipe;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	int waiting;
	struct rbtree tree;
};

static inline struct aio_reactor_impl *aio_impl_from_reactor(
		const aio_reactor_t *reactor);
static inline struct aio_reactor_impl *aio_impl_from_service(
		const struct aio_service *srv);
static inline struct aio_reactor_impl *aio_impl_from_poll(
		const aio_poll_t *poll);

static int aio_reactor_impl_open(struct aio_reactor_impl *impl);
static int aio_reactor_impl_close(struct aio_reactor_impl *impl);

static void aio_reactor_impl_do_events(struct aio_reactor_impl *impl,
		struct aio_watch *watch, int revents);

static inline struct aio_watch *aio_watch_from_node(struct rbnode *node);

LELY_AIO_EXPORT void *
aio_reactor_alloc(void)
{
	struct aio_reactor_impl *impl = malloc(sizeof(*impl));
	return impl ? &impl->reactor_vptr : NULL;
}

LELY_AIO_EXPORT void
aio_reactor_free(void *ptr)
{
	if (ptr)
		free(aio_impl_from_reactor(ptr));
}

LELY_AIO_EXPORT aio_reactor_t *
aio_reactor_init(aio_reactor_t *reactor, aio_context_t *ctx)
{
	struct aio_reactor_impl *impl = aio_impl_from_reactor(reactor);
	assert(ctx);

	int errc = 0;

	impl->reactor_vptr = &aio_reactor_impl_vtbl;

	impl->srv = (struct aio_service)AIO_SERVICE_INIT(
			&aio_reactor_impl_service_vtbl);
	impl->ctx = ctx;

	impl->poll_vptr = &aio_reactor_impl_poll_vtbl;

	impl->epfd = -1;
	impl->pipe = (struct aio_self_pipe)AIO_SELF_PIPE_INIT;
	if (aio_reactor_impl_open(impl) == -1) {
		errc = errno;
		goto error_open;
	}

#if !LELY_NO_THREADS
	if (mtx_init(&impl->mtx, mtx_plain) != thrd_success) {
		errc = errno;
		goto error_mtx_init;
	}
#endif

	impl->waiting = 0;

	rbtree_init(&impl->tree, &aio_handle_cmp);

	aio_context_insert(impl->ctx, &impl->srv);

	return reactor;

#if !LELY_NO_THREADS
	mtx_destroy(&impl->mtx);
error_mtx_init:
#endif
	aio_reactor_impl_close(impl);
error_open:
	errno = errc;
	return NULL;
}

LELY_AIO_EXPORT void
aio_reactor_fini(aio_reactor_t *reactor)
{
	struct aio_reactor_impl *impl = aio_impl_from_reactor(reactor);

	aio_context_remove(impl->ctx, &impl->srv);

#if !LELY_NO_THREADS
	mtx_destroy(&impl->mtx);
#endif

	aio_reactor_impl_close(impl);
}

LELY_AIO_EXPORT aio_reactor_t *
aio_reactor_create(aio_context_t *ctx)
{
	int errc = 0;

	aio_reactor_t *reactor = aio_reactor_alloc();
	if (!reactor) {
		errc = errno;
		goto error_alloc;
	}

	aio_reactor_t *tmp = aio_reactor_init(reactor, ctx);
	if (!tmp) {
		errc = errno;
		goto error_init;
	}
	reactor = tmp;

	return reactor;

error_init:
	aio_reactor_free((void *)reactor);
error_alloc:
	errno = errc;
	return NULL;
}

LELY_AIO_EXPORT void
aio_reactor_destroy(aio_reactor_t *reactor)
{
	if (reactor) {
		aio_reactor_fini(reactor);
		aio_reactor_free((void *)reactor);
	}
}

static int
aio_handle_cmp(const void *p1, const void *p2)
{
	assert(p1);
	int fd1 = *(const int *)p1;
	assert(p2);
	int fd2 = *(const int *)p2;

	return (fd2 < fd1) - (fd1 < fd2);
}

static aio_context_t *
aio_reactor_impl_get_context(const aio_reactor_t *reactor)
{
	const struct aio_reactor_impl *impl = aio_impl_from_reactor(reactor);

	return impl->ctx;
}

static aio_poll_t *
aio_reactor_impl_get_poll(const aio_reactor_t *reactor)
{
	const struct aio_reactor_impl *impl = aio_impl_from_reactor(reactor);

	return &impl->poll_vptr;
}

static int
aio_reactor_impl_watch(aio_reactor_t *reactor, struct aio_watch *watch,
		aio_handle_t handle, int events)
{
	struct aio_reactor_impl *impl = aio_impl_from_reactor(reactor);
	assert(watch);

	int epfd = impl->epfd;
	int efd = impl->pipe.handles[0];

	if (handle == -1 || handle == epfd || handle == efd) {
		errno = EBADF;
		return -1;
	}

	if (events < 0) {
		errno = EINVAL;
		return -1;
	}

	int result = -1;
	int errc = errno;

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	struct epoll_event event = { 0, { .fd = handle } };
	if (events & AIO_WATCH_READ)
		event.events |= EPOLLIN | EPOLLPRI;
	if (events & AIO_WATCH_WRITE)
		event.events |= EPOLLOUT;

	struct rbnode *node = rbtree_find(&impl->tree, &handle);
	if (node && node != &watch->_node) {
		errc = EALREADY; // TODO: Check error code.
		goto error;
	}

	if (events) {
		if (node && events != watch->_events) {
			if (epoll_ctl(epfd, EPOLL_CTL_MOD, handle, &event)
					== -1) {
				errc = errno;
				epoll_ctl(epfd, EPOLL_CTL_DEL, handle, NULL);
				rbtree_remove(&impl->tree, node);
				watch->_events = 0;
				goto error;
			}
		} else if (!node) {
			if (epoll_ctl(epfd, EPOLL_CTL_ADD, handle, &event)
					== -1) {
				errc = errno;
				goto error;
			}
			watch->_handle = handle;
			rbnode_init(&watch->_node, &watch->_handle);
			rbtree_insert(&impl->tree, &watch->_node);
		}
	} else if (node) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, handle, NULL);
		rbtree_remove(&impl->tree, node);
	}
	watch->_events = events;

	result = 0;

error:
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	errno = errc;
	return result;
}

static int
aio_reactor_impl_service_notify_fork(struct aio_service *srv,
		enum aio_fork_event e)
{
	struct aio_reactor_impl *impl = aio_impl_from_service(srv);

	if (e != AIO_FORK_CHILD)
		return 0;

	int result = -1;
	int errc = errno;

	aio_self_pipe_write(&impl->pipe);

	if (aio_reactor_impl_close(impl) == -1 && !result) {
		errc = errno;
		result = -1;
	}

	if (aio_reactor_impl_open(impl) == -1 && !result) {
		errc = errno;
		result = -1;
	}

	int epfd = impl->epfd;
	rbtree_foreach(&impl->tree, node) {
		struct aio_watch *watch = aio_watch_from_node(node);
		aio_handle_t handle = watch->_handle;
		int events = watch->_events;

		struct epoll_event event = { 0, { .fd = handle } };
		if (events & AIO_WATCH_READ)
			event.events |= EPOLLIN | EPOLLPRI;
		if (events & AIO_WATCH_WRITE)
			event.events |= EPOLLOUT;

		if (epoll_ctl(epfd, EPOLL_CTL_ADD, handle, &event) == -1
				&& !result) {
			errc = errno;
			result = -1;;
		}
	}

	errno = errc;
	return result;
}

static void
aio_reactor_impl_service_shutdown(struct aio_service *srv)
{
	__unused_var(srv);
}

static size_t
aio_reactor_impl_poll_wait(aio_poll_t *poll, int timeout)
{
	struct aio_reactor_impl *impl = aio_impl_from_poll(poll);

	int epfd = impl->epfd;
	int efd = impl->pipe.handles[0];

	size_t n = 0;
	int errc = errno;

	struct epoll_event events[LELY_AIO_EPOLL_MAXEVENTS];
	int nevents;

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	do {
		if (impl->waiting) {
			errc = EALREADY; // TODO: Check error code.
			break;
		}
		impl->waiting = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		nevents = epoll_wait(epfd, events, LELY_AIO_EPOLL_MAXEVENTS,
				timeout);
#if !LELY_NO_THREADS
		int errsv = errno;
		mtx_lock(&impl->mtx);
		errno = errsv;
#endif
		impl->waiting = 0;

		if (nevents <= 0) {
			errc = errno;
			break;
		}

		for (int i = 0; i < nevents; i++) {
			int revents = 0;
			if (events[i].events & (EPOLLIN | EPOLLPRI))
				revents |= AIO_WATCH_READ;
			if (events[i].events & EPOLLOUT)
				revents |= AIO_WATCH_WRITE;
			if (events[i].events & (EPOLLERR | EPOLLHUP))
				revents |= AIO_WATCH_ERROR;
			int fd = events[i].data.fd;

			if (fd == efd) {
				int errsv = errno;
				aio_self_pipe_read(&impl->pipe);
				errno = errsv;
				continue;
			}

			struct rbnode *node = rbtree_find(&impl->tree, &fd);
			if (node) {
				struct aio_watch *watch =
						aio_watch_from_node(node);
				aio_reactor_impl_do_events(impl, watch,
						revents);
				n++;
			}
		}

		timeout = 0;
	} while (nevents == LELY_AIO_EPOLL_MAXEVENTS);
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	errno = errc;
	return n;
}

static void
aio_reactor_impl_poll_stop(aio_poll_t *poll)
{
	struct aio_reactor_impl *impl = aio_impl_from_poll(poll);

	int errsv = errno;
	aio_self_pipe_write(&impl->pipe);
	errno = errsv;
}

static inline struct aio_reactor_impl *
aio_impl_from_reactor(const aio_reactor_t *reactor)
{
	assert(reactor);

	return structof(reactor, struct aio_reactor_impl, reactor_vptr);
}

static inline struct aio_reactor_impl *
aio_impl_from_service(const struct aio_service *srv)
{
	assert(srv);

	return structof(srv, struct aio_reactor_impl, srv);
}

static inline struct aio_reactor_impl *
aio_impl_from_poll(const aio_poll_t *poll)
{
	assert(poll);

	return structof(poll, struct aio_reactor_impl, poll_vptr);
}

static int
aio_reactor_impl_open(struct aio_reactor_impl *impl)
{
	assert(impl);

	int errc = 0;

	if (aio_reactor_impl_close(impl) == -1) {
		errc = errno;
		goto error_close;
	}

	impl->epfd = epoll_create1(EPOLL_CLOEXEC);
	if (impl->epfd == -1) {
		errc = errno;
		goto error_epoll_create1;
	}

	impl->pipe = (struct aio_self_pipe)AIO_SELF_PIPE_INIT;
	if (aio_self_pipe_open(&impl->pipe) == -1) {
		errc = errno;
		goto error_self_pipe_open;
	}
	int efd = impl->pipe.handles[0];

	struct epoll_event event = { EPOLLIN, { .fd = efd } };
	if (epoll_ctl(impl->epfd, EPOLL_CTL_ADD, efd, &event) == -1) {
		errc = errno;
		goto error_epoll_ctl;
	}

	return 0;

error_epoll_ctl:
	aio_self_pipe_close(&impl->pipe);
error_self_pipe_open:
	close(impl->epfd);
error_epoll_create1:
error_close:
	errno = errc;
	return -1;
}

static int
aio_reactor_impl_close(struct aio_reactor_impl *impl)
{
	assert(impl);

	int epfd = impl->epfd;
	if (epfd == -1)
		return 0;
	impl->epfd  =-1;

	int result = 0;
	int errc = errno;

	struct aio_self_pipe pipe = impl->pipe;
	impl->pipe = (struct aio_self_pipe)AIO_SELF_PIPE_INIT;

	if (aio_self_pipe_is_open(&pipe)) {
		if (epfd != -1) {
			int errsv = errno;
			epoll_ctl(epfd, EPOLL_CTL_DEL, pipe.handles[0], NULL);
			errno = errsv;
		}
		if (aio_self_pipe_close(&pipe) == -1 && !result) {
			errc = errno;
			result = -1;
		}
	}

	if (close(epfd) && !result) {
		errc = errno;
		result = -1;
	}

	errno = errc;
	return result;
}

static void
aio_reactor_impl_do_events(struct aio_reactor_impl *impl,
		struct aio_watch *watch, int revents)
{
	assert(impl);
	assert(watch);

	int epfd = impl->epfd;

	aio_handle_t handle = watch->_handle;
	int events = watch->_events;
	if (watch->func) {
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		events = watch->func(watch, revents);
#if !LELY_NO_THREADS
		mtx_lock(&impl->mtx);
#endif
	}

	struct rbnode *node = rbtree_find(&impl->tree, &handle);
	if (node != &watch->_node)
		return;

	if (events > 0 && events != watch->_events) {
		struct epoll_event event = { 0, { .fd = handle } };
		if (events & AIO_WATCH_READ)
			event.events |= EPOLLIN | EPOLLPRI;
		if (events & AIO_WATCH_WRITE)
			event.events |= EPOLLOUT;

		if (epoll_ctl(epfd, EPOLL_CTL_MOD, handle, &event) == -1)
			events = -1;
	}

	if (events <= 0) {
		events = 0;

		int errsv = errno;
		epoll_ctl(epfd, EPOLL_CTL_DEL, handle, NULL);
		errno = errsv;
		rbtree_remove(&impl->tree, node);
	}

	watch->_events = events;
}

static inline struct aio_watch *
aio_watch_from_node(struct rbnode *node)
{
	return structof(node, struct aio_watch, _node);
}

#endif // __linux__
