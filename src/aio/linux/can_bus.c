/*!\file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * \see lely/aio/can_bus.h
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
#include <lely/can/socket.h>
#include <lely/aio/can_bus.h>
#include <lely/aio/context.h>
#include <lely/aio/queue.h>
#include <lely/aio/reactor.h>

#include <net/if.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/can/error.h>
#include <linux/can/raw.h>
#include <linux/can.h>
#include <sys/ioctl.h>

static aio_exec_t *aio_can_bus_impl_get_exec(const aio_can_bus_t *bus);
static int aio_can_bus_impl_read(aio_can_bus_t *bus, struct can_msg *msg,
		struct can_msg_info *info);
static int aio_can_bus_impl_submit_read(aio_can_bus_t *bus,
		struct aio_can_bus_read_op *op);
static size_t aio_can_bus_impl_cancel_read(aio_can_bus_t *bus,
		struct aio_can_bus_read_op *op);
static int aio_can_bus_impl_write(aio_can_bus_t *bus,
		const struct can_msg *msg);
static int aio_can_bus_impl_submit_write(aio_can_bus_t *bus,
		struct aio_can_bus_write_op *op);
static size_t aio_can_bus_impl_cancel_write(aio_can_bus_t *bus,
		struct aio_can_bus_write_op *op);
static size_t aio_can_bus_impl_cancel(aio_can_bus_t *bus);

static const struct aio_can_bus_vtbl aio_can_bus_impl_vtbl = {
	&aio_can_bus_impl_get_exec,
	&aio_can_bus_impl_read,
	&aio_can_bus_impl_submit_read,
	&aio_can_bus_impl_cancel_read,
	&aio_can_bus_impl_write,
	&aio_can_bus_impl_submit_write,
	&aio_can_bus_impl_cancel_write,
	&aio_can_bus_impl_cancel
};

static void aio_can_bus_impl_service_shutdown(struct aio_service *srv);

static const struct aio_service_vtbl aio_can_bus_impl_service_vtbl = {
	NULL,
	&aio_can_bus_impl_service_shutdown
};

struct aio_can_bus_impl {
	const struct aio_can_bus_vtbl *can_bus_vptr;
	aio_exec_t *exec;
	aio_reactor_t *reactor;
	struct aio_service srv;
	aio_context_t *ctx;
	struct aio_watch watch;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	int shutdown;
	aio_handle_t handle;
#if !LELY_NO_CANFD
	unsigned fd_frames:1;
#endif
	unsigned error_frames:1;
	struct aio_queue read_queue;
	struct aio_queue write_queue;
};

static int aio_can_bus_impl_func(struct aio_watch *watch, int events);

static inline struct aio_can_bus_impl *aio_impl_from_can_bus(
		const aio_can_bus_t *bus);
static inline struct aio_can_bus_impl *aio_impl_from_service(
		const struct aio_service *srv);

static int aio_can_bus_impl_do_assign(struct aio_can_bus_impl *impl,
		aio_handle_t handle);

static int aio_can_bus_impl_do_read(struct aio_can_bus_impl *impl,
		struct can_msg *msg, struct can_msg_info *info);
static void aio_can_bus_impl_do_submit_read(struct aio_can_bus_impl *impl,
		struct aio_queue *queue);

static int aio_can_bus_impl_do_write(struct aio_can_bus_impl *impl,
		const struct can_msg *msg);
static void aio_can_bus_impl_do_submit_write(struct aio_can_bus_impl *impl,
		struct aio_queue *queue);

static int aio_can_bus_impl_do_watch(struct aio_can_bus_impl *impl);

#if !LELY_NO_CANFD
static int aio_can_bus_get_fd_frames(aio_handle_t handle, int *pvalue);
static int aio_can_bus_set_fd_frames(aio_handle_t handle, int value);
#endif

static int aio_can_bus_get_error_frames(aio_handle_t handle, int *pvalue);
static int aio_can_bus_set_error_frames(aio_handle_t handle, int value);

LELY_AIO_EXPORT void *
aio_can_bus_alloc(void)
{
	struct aio_can_bus_impl *impl = malloc(sizeof(*impl));
	return impl ? &impl->can_bus_vptr : NULL;
}

LELY_AIO_EXPORT void
aio_can_bus_free(void *ptr)
{
	if (ptr)
		free(aio_impl_from_can_bus(ptr));
}

LELY_AIO_EXPORT aio_can_bus_t *
aio_can_bus_init(aio_can_bus_t *bus, aio_exec_t *exec, aio_reactor_t *reactor)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);
	assert(exec);
	assert(reactor);
	aio_context_t *ctx = aio_reactor_get_context(reactor);
	assert(ctx);

	impl->can_bus_vptr = &aio_can_bus_impl_vtbl;

	impl->exec = exec;

	impl->reactor = reactor;

	impl->srv = (struct aio_service)AIO_SERVICE_INIT(
			&aio_can_bus_impl_service_vtbl);
	impl->ctx = ctx;

	impl->watch = (struct aio_watch)AIO_WATCH_INIT(&aio_can_bus_impl_func);

#if !LELY_NO_THREADS
	if (mtx_init(&impl->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	impl->shutdown = 0;

	impl->handle = -1;

#if !LELY_NO_CANFD
	impl->fd_frames = 0;
#endif
	impl->error_frames = 0;

	aio_queue_init(&impl->read_queue);
	aio_queue_init(&impl->write_queue);

	aio_context_insert(impl->ctx, &impl->srv);

	return bus;
}

LELY_AIO_EXPORT void
aio_can_bus_fini(aio_can_bus_t *bus)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	aio_context_remove(impl->ctx, &impl->srv);

	if (impl->handle != -1) {
		if (!impl->shutdown)
			aio_reactor_watch(impl->reactor, &impl->watch,
					impl->handle, 0);
		close(impl->handle);
	}

#if !LELY_NO_THREADS
	mtx_destroy(&impl->mtx);
#endif
}

LELY_AIO_EXPORT aio_can_bus_t *
aio_can_bus_create(aio_exec_t *exec, aio_reactor_t *reactor)
{
	int errc = 0;

	aio_can_bus_t *bus = aio_can_bus_alloc();
	if (!bus) {
		errc = errno;
		goto error_alloc;
	}

	aio_can_bus_t *tmp = aio_can_bus_init(bus, exec, reactor);
	if (!tmp) {
		errc = errno;
		goto error_init;
	}
	bus = tmp;

	return bus;

error_init:
	aio_can_bus_free((void *)bus);
error_alloc:
	errno = errc;
	return NULL;
}

LELY_AIO_EXPORT void
aio_can_bus_destroy(aio_can_bus_t *bus)
{
	if (bus) {
		aio_can_bus_fini(bus);
		aio_can_bus_free((void *)bus);
	}
}

LELY_AIO_EXPORT aio_handle_t
aio_can_bus_get_handle(const aio_can_bus_t *bus)
{
	const struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&impl->mtx);
#endif
	int s = impl->handle;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&impl->mtx);
#endif

	return s;
}

LELY_AIO_EXPORT int
aio_can_bus_open(aio_can_bus_t *bus, const char *ifname)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);
	assert(ifname);

	int errc = 0;
#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (impl->handle != -1) {
		errc = EALREADY; // TODO: Check error code.
		goto error_open;
	}

	unsigned int ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		errc = errno;
		goto error_if_nametoindex;
	}

	aio_handle_t handle = socket(AF_CAN,
			SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC, CAN_RAW);
	if (handle == -1) {
		errc = errno;
		goto error_socket;
	}

	struct sockaddr_can addr = {
		.can_family = AF_CAN,
		.can_ifindex = ifindex
	};

	if (bind(handle, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		errc = errno;
		goto error_bind;
	}

	if (aio_can_bus_impl_do_assign(impl, handle) == -1) {
		errc = errno;
		goto error_assign;
	}

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif
	return 0;

error_assign:
error_bind:
	close(handle);
error_socket:
error_if_nametoindex:
error_open:
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif
	errno = errc;
	return -1;
}

LELY_AIO_EXPORT int
aio_can_bus_assign(aio_can_bus_t *bus, aio_handle_t handle)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	int errc = 0;
#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (impl->handle != -1) {
		errc = EALREADY; // TODO: Check error code.
		goto error_open;
	}

	struct sockaddr_can addr = { .can_family = AF_CAN, .can_ifindex = 0 };
	socklen_t len = sizeof(addr);
	if (getsockname(handle, (struct sockaddr *)&addr, &len) == -1) {
		errc = errno;
		goto error_getsockname;
	}
	if (!addr.can_ifindex) {
		errc = ENOTSOCK;
		goto error_getsockname;
	}

	int arg = fcntl(handle, F_GETFL);
	if (arg == -1) {
		errc = errno;
		goto error_fcntl;
	}
	if ((arg | O_NONBLOCK) != arg
			&& fcntl(handle, F_SETFL, arg | O_NONBLOCK) == -1) {
		errc = errno;
		goto error_fcntl;
	}

	if (aio_can_bus_impl_do_assign(impl, handle) == -1) {
		errc = errno;
		goto error_assign;
	}

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	return 0;

error_assign:
error_fcntl:
error_getsockname:
error_open:
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif
	errno = errc;
	return -1;
}

LELY_AIO_EXPORT aio_handle_t
aio_can_bus_release(aio_can_bus_t *bus)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (impl->handle == -1) {
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		errno = EBADF;
		return -1;
	}

	aio_queue_move(&queue, &impl->write_queue, NULL);
	aio_queue_move(&queue, &impl->read_queue, NULL);

	if (!impl->shutdown)
		aio_reactor_watch(impl->reactor, &impl->watch, impl->handle, 0);
	aio_handle_t handle = impl->handle;
	impl->handle = -1;
#if !LELY_NO_CANFD
	impl->fd_frames = 0;
#endif
	impl->error_frames = 0;

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	aio_queue_cancel(&queue, ECANCELED);

	return handle;
}

LELY_AIO_EXPORT int
aio_can_bus_is_open(const aio_can_bus_t *bus)
{
	return aio_can_bus_get_handle(bus) != -1;
}

LELY_AIO_EXPORT int
aio_can_bus_close(aio_can_bus_t *bus)
{
	return close(aio_can_bus_release(bus));
}

LELY_AIO_EXPORT int
aio_can_bus_get_option(const aio_can_bus_t *bus, int name, void *pvalue,
		size_t *plen)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);
	assert(pvalue);
	assert(plen);

	int result = -1;
	int errc = errno;
#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (impl->handle == -1) {
		errc = EBADF;
		goto error;
	}

	switch (name) {
#if !LELY_NO_CANFD
	case AIO_CAN_BUS_FD_FRAMES:
		if (*plen < sizeof(int)) {
			errc = EINVAL;
			goto error;
		}
		if (aio_can_bus_get_fd_frames(impl->handle, pvalue) == -1) {
			errc = errno;
			goto error;
		}
		*plen = sizeof(int);
		break;
#endif
	case AIO_CAN_BUS_ERROR_FRAMES:
		if (*plen < sizeof(int)) {
			errc = EINVAL;
			goto error;
		}
		if (aio_can_bus_get_error_frames(impl->handle, pvalue) == -1) {
			errc = errno;
			goto error;
		}
		*plen = sizeof(int);
		break;
	default:
		errc = EINVAL;
		goto error;
	}

	result = 0;

error:
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif
	errno = errc;
	return result;
}

LELY_AIO_EXPORT int
aio_can_bus_set_option(aio_can_bus_t *bus, int name, const void *pvalue,
		size_t len)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);
	assert(pvalue);
	assert(len);

	int result = -1;
	int errc = errno;
#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (impl->handle == -1) {
		errc = EBADF;
		goto error;
	}

	switch (name) {
#if !LELY_NO_CANFD
	case AIO_CAN_BUS_FD_FRAMES: {
		if (len != sizeof(int)) {
			errc = EINVAL;
			goto error;
		}
		int value = *(const int *)pvalue;
		if (aio_can_bus_set_fd_frames(impl->handle, value) == -1) {
			errc = errno;
			goto error;
		}
		impl->fd_frames = !!value;
		break;
	}
#endif
	case AIO_CAN_BUS_ERROR_FRAMES: {
		if (len != sizeof(int)) {
			errc = EINVAL;
			goto error;
		}
		int value = *(const int *)pvalue;
		if (aio_can_bus_set_error_frames(impl->handle, value) == -1) {
			errc = errno;
			goto error;
		}
		impl->error_frames = !!value;
		break;
	}
	default:
		errc = EINVAL;
		goto error;
	}

	result = 0;

error:
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif
	errno = errc;
	return result;
}

static aio_exec_t *
aio_can_bus_impl_get_exec(const aio_can_bus_t *bus)
{
	const struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	return impl->exec;
}

static int
aio_can_bus_impl_read(aio_can_bus_t *bus, struct can_msg *msg,
		struct can_msg_info *info)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	int errc = errno;
#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	errno = 0;
	int result = aio_can_bus_impl_do_read(impl, msg, info);
	if (result == -1)
		errc = errno;
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif
	errno = errc;

	return result;
}

static int
aio_can_bus_impl_submit_read(aio_can_bus_t *bus, struct aio_can_bus_read_op *op)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);
	assert(op);
	struct aio_task *task = &op->task;

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (impl->handle == -1) {
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		errno = EBADF;
		return -1;
	}

	if (!task->exec)
		task->exec = aio_can_bus_get_exec(bus);
	aio_exec_on_task_started(task->exec);
	task->errc = EINPROGRESS;

	op->result = -1;

	if (impl->shutdown) {
		task->errc = ECANCELED;
		aio_queue_push(&queue, task);
	} else {
		int first = aio_queue_empty(&impl->read_queue);
		aio_queue_push(&impl->read_queue, task);
		if (first) {
			aio_can_bus_impl_do_submit_read(impl, &queue);
			aio_can_bus_impl_do_watch(impl); // TODO: Handle error?
		}
	}

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	aio_queue_post(&queue);

	return 0;
}

static size_t
aio_can_bus_impl_cancel_read(aio_can_bus_t *bus, struct aio_can_bus_read_op *op)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	aio_queue_move(&queue, &impl->read_queue, op ? &op->task : NULL);
	if (aio_queue_empty(&queue)) {
#if !LELY_NO_THREADS
			mtx_unlock(&impl->mtx);
#endif
			return 0;
	}

	if (aio_queue_empty(&impl->read_queue))
		aio_can_bus_impl_do_watch(impl); // TODO: Handle error?

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	return aio_queue_cancel(&queue, ECANCELED);
}

static int
aio_can_bus_impl_write(aio_can_bus_t *bus, const struct can_msg *msg)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	int errc = errno;
#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	errno = 0;
	int result = aio_can_bus_impl_do_write(impl, msg);
	if (result == -1)
		errc = errno;
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif
	errno = errc;

	return result;
}

static int
aio_can_bus_impl_submit_write(aio_can_bus_t *bus,
		struct aio_can_bus_write_op *op)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);
	assert(op);
	assert(op->msg);
	struct aio_task *task = &op->task;

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (impl->handle == -1) {
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		errno = EBADF;
		return -1;
	}

	if (!task->exec)
		task->exec = aio_can_bus_get_exec(bus);
	aio_exec_on_task_started(task->exec);
	task->errc = EINPROGRESS;

	op->result = -1;

	if (impl->shutdown) {
		task->errc = ECANCELED;
		aio_queue_push(&queue, task);
	} else {
		int first = aio_queue_empty(&impl->write_queue);
		aio_queue_push(&impl->write_queue, task);
		if (first) {
			aio_can_bus_impl_do_submit_write(impl, &queue);
			aio_can_bus_impl_do_watch(impl); // TODO: Handle error?
		}
	}

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	aio_queue_post(&queue);

	return 0;
}

static size_t
aio_can_bus_impl_cancel_write(aio_can_bus_t *bus,
		struct aio_can_bus_write_op *op)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	aio_queue_move(&queue, &impl->write_queue, op ? &op->task : NULL);
	if (aio_queue_empty(&queue)) {
#if !LELY_NO_THREADS
			mtx_unlock(&impl->mtx);
#endif
			return 0;
	}

	if (aio_queue_empty(&impl->write_queue))
		aio_can_bus_impl_do_watch(impl); // TODO: Handle error?

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	return aio_queue_cancel(&queue, ECANCELED);
}

static size_t
aio_can_bus_impl_cancel(aio_can_bus_t *bus)
{
	struct aio_can_bus_impl *impl = aio_impl_from_can_bus(bus);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (aio_queue_empty(&impl->read_queue)
			&& aio_queue_empty(&impl->write_queue)) {
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		return 0;
	}

	aio_queue_move(&queue, &impl->write_queue, NULL);
	aio_queue_move(&queue, &impl->read_queue, NULL);

	assert(impl->handle != -1);
	if (!impl->shutdown)
		aio_reactor_watch(impl->reactor, &impl->watch, impl->handle, 0);

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	return aio_queue_cancel(&queue, ECANCELED);
}

static void
aio_can_bus_impl_service_shutdown(struct aio_service *srv)
{
	struct aio_can_bus_impl *impl = aio_impl_from_service(srv);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	assert(!impl->shutdown);
	impl->shutdown = 1;

	aio_reactor_watch(impl->reactor, &impl->watch, impl->handle, 0);

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	aio_can_bus_impl_cancel(&impl->can_bus_vptr);
}

static int
aio_can_bus_impl_func(struct aio_watch *watch, int events)
{
	assert(watch);
	struct aio_can_bus_impl *impl =
			structof(watch, struct aio_can_bus_impl, watch);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	if (events & AIO_WATCH_READ)
		aio_can_bus_impl_do_submit_read(impl, &queue);
	if (events & AIO_WATCH_WRITE)
		aio_can_bus_impl_do_submit_write(impl, &queue);

	events = 0;
	if (!aio_queue_empty(&impl->read_queue))
		events |= AIO_WATCH_READ;
	if (!aio_queue_empty(&impl->write_queue))
		events |= AIO_WATCH_WRITE;

#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	aio_queue_post(&queue);

	return events;
}

static inline struct aio_can_bus_impl *
aio_impl_from_can_bus(const aio_can_bus_t *bus)
{
	assert(bus);

	return structof(bus, struct aio_can_bus_impl, can_bus_vptr);
}

static inline struct aio_can_bus_impl *
aio_impl_from_service(const struct aio_service *srv)
{
	assert(srv);

	return structof(srv, struct aio_can_bus_impl, srv);
}

static int
aio_can_bus_impl_do_assign(struct aio_can_bus_impl *impl, aio_handle_t handle)
{
	assert(impl);
	assert(handle != -1);

#if !LELY_NO_CANFD
	int fd_frames = 0;
	if (aio_can_bus_get_fd_frames(handle, &fd_frames) == -1)
		return -1;
#endif

	int error_frames = 0;
	if (aio_can_bus_get_error_frames(handle, &error_frames) == -1)
		return -1;

	impl->handle = handle;
#if !LELY_NO_CANFD
	impl->fd_frames = !!fd_frames;
#endif
	impl->error_frames = !!error_frames;

	return 0;
}

static int
aio_can_bus_impl_do_read(struct aio_can_bus_impl *impl, struct can_msg *msg,
		struct can_msg_info *info)
{
	assert(impl);

	if (impl->handle == -1) {
		errno = EBADF;
		return -1;
	}

	int errsv = errno;

	for (;;) {
#if LELY_NO_CANFD
		struct can_frame frame;
#else
		struct canfd_frame frame;
#endif
		int result;
		do {
			errno = 0;
			result = read(impl->handle, &frame, sizeof(frame));
		} while (result == -1 && errno == EINTR);
		if (result == -1)
			return -1;
		errno = errsv;

#if LELY_NO_CANFD
		if (result != CAN_MTU)
#else
		if (result != CAN_MTU
				&& (!impl->fd_frames || result != CANFD_MTU))
#endif
			continue;

		struct timeval tv = { 0, 0 };
		if (info && ioctl(impl->handle, SIOCGSTAMP, &tv) == -1)
			return -1;

		enum can_state state = CAN_STATE_ACTIVE;
		enum can_error error = 0;
		int is_error = can_frame_is_error((struct can_frame *)&frame,
				&state, &error);
		if (is_error == -1)
			return -1;
		if (!impl->error_frames && is_error)
			continue;

#if !LELY_NO_CANFD
		if (!is_error && msg && result == CANFD_MTU) {
			if (canfd_frame2can_msg(&frame, msg) == -1)
				return -1;
		} else
#endif
		if (!is_error && msg) {
			if (can_frame2can_msg((struct can_frame *)&frame, msg)
					== -1)
				return -1;
		}

		if (info) {
			info->ts.tv_sec = tv.tv_sec;
			info->ts.tv_nsec = tv.tv_usec * 1000;
			if (is_error) {
				info->state = state;
				info->error = error;
			}
		}

		return !is_error;
	}
}

static void
aio_can_bus_impl_do_submit_read(struct aio_can_bus_impl *impl,
		struct aio_queue *queue)
{
	assert(impl);
	assert(queue);

	int errsv = errno;

	struct aio_task *task;
	while ((task = aio_queue_front(&impl->read_queue))) {
		struct aio_can_bus_read_op *op = structof(task,
				struct aio_can_bus_read_op, task);

		errno = 0;
		op->result = aio_can_bus_impl_do_read(impl, op->msg, op->info);
		if (op->result == -1
				&& (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		aio_queue_pop(&impl->read_queue);
		task->errc = op->result == -1 ? errno : 0;
		aio_queue_push(queue, task);
	}

	errno = errsv;
}

static int
aio_can_bus_impl_do_write(struct aio_can_bus_impl *impl,
		const struct can_msg *msg)
{
	assert(impl);
	assert(msg);

	if (impl->handle == -1) {
		errno = EBADF;
		return -1;
	}

#if LELY_NO_CANFD
	struct can_frame frame;
#else
	struct canfd_frame frame;
#endif
	size_t nbytes = 0;
#if !LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_EDL) {
		if (!impl->fd_frames) {
			errno = ENOTSUP; // TODO: Check error code.
			return -1;
		}
		if (can_msg2canfd_frame(msg, &frame) == -1)
			return -1;
		nbytes = CANFD_MTU;
	} else {
#endif
		if (can_msg2can_frame(msg, (struct can_frame *)&frame) == -1)
			return -1;
		nbytes = CAN_MTU;
#if !LELY_NO_CANFD
	}
#endif

	int errsv = errno;
	int result;
	do {
		errno = 0;
		result = write(impl->handle, &frame, nbytes);
	} while (result == -1 && errno == EINTR);
	if (result == -1)
		return -1;
	errno = errsv;

	return 1;
}

static void
aio_can_bus_impl_do_submit_write(struct aio_can_bus_impl *impl,
		struct aio_queue *queue)
{
	assert(impl);
	assert(queue);
	int errsv = errno;

	struct aio_task *task;
	while ((task = aio_queue_front(&impl->write_queue))) {
		struct aio_can_bus_write_op *op = structof(task,
				struct aio_can_bus_write_op, task);

		errno = 0;
		op->result = aio_can_bus_impl_do_write(impl, op->msg);
		if (op->result == -1
				&& (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		aio_queue_pop(&impl->write_queue);
		task->errc = op->result == -1 ? errno : 0;
		aio_queue_push(queue, task);
	}

	errno = errsv;
}

static int
aio_can_bus_impl_do_watch(struct aio_can_bus_impl *impl)
{
	assert(impl);
	assert(impl->handle != -1);

	if (impl->shutdown)
		return 0;

	int events = 0;
	if (!aio_queue_empty(&impl->read_queue))
		events |= AIO_WATCH_READ;
	if (!aio_queue_empty(&impl->write_queue))
		events |= AIO_WATCH_WRITE;

	return aio_reactor_watch(impl->reactor, &impl->watch, impl->handle,
			events);
}

#if !LELY_NO_CANFD

static int
aio_can_bus_get_fd_frames(aio_handle_t handle, int *pvalue)
{
	assert(handle != -1);
	assert(pvalue);

	socklen_t optlen = sizeof(*pvalue);
	return getsockopt(handle, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, pvalue,
			&optlen);
}

static int
aio_can_bus_set_fd_frames(aio_handle_t handle, int value)
{
	assert(handle != -1);
	value = !!value;

	return setsockopt(handle, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &value,
			sizeof(value));
}

#endif // !LELY_NO_CANFD

static int
aio_can_bus_get_error_frames(aio_handle_t handle, int *pvalue)
{
	assert(handle != -1);
	assert(pvalue);

	can_err_mask_t optval = 0;
	socklen_t optlen = sizeof(optval);
	if (getsockopt(handle, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &optval,
			&optlen) == -1)
		return -1;
	*pvalue = !!(optval & CAN_ERR_MASK);

	return 0;
}

static int
aio_can_bus_set_error_frames(aio_handle_t handle, int value)
{
	assert(handle != -1);

	can_err_mask_t optval = value ? CAN_ERR_MASK : 0;
	return setsockopt(handle, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &optval,
			sizeof(optval));
}

#endif // __linux__
