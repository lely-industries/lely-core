/**@file
 * This file is part of the I/O library; it contains the CAN channel
 * implementation for Linux.
 *
 * @see lely/io2/linux/can.h
 *
 * @copyright 2015-2019 Lely Industries N.V.
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

#ifdef __linux__

#include "../can.h"
#include <lely/ev/strand.h>
#include <lely/io2/ctx.h>
#include <lely/io2/linux/can.h>
#include <lely/io2/posix/poll.h>
#include <lely/util/util.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#if !LELY_NO_THREADS
#include <pthread.h>
#endif
#include <unistd.h>

#include <linux/can/raw.h>
#include <sys/ioctl.h>

#include "../cbuf.h"
#include "../posix/fd.h"
#include "can_attr.h"
#include "can_err.h"
#include "can_msg.h"

#ifndef LELY_IO_CAN_RXLEN
/// The default SocketCAN receive queue length (in number of CAN frames).
#define LELY_IO_CAN_RXLEN 1024
#endif

struct io_can_frame {
#if LELY_NO_CANFD
	struct can_frame frame;
#else
	struct canfd_frame frame;
#endif
	size_t nbytes;
	struct timespec ts;
};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
LELY_IO_DEFINE_CBUF(io_can_buf, struct io_can_frame)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static int io_can_fd_set_default(int fd);
#if LELY_NO_CANFD
static int io_can_fd_read(int fd, struct can_frame *frame, size_t *pnbytes,
		int *pflags, struct timespec *tp, int timeout);
#else
static int io_can_fd_read(int fd, struct canfd_frame *frame, size_t *pnbytes,
		int *pflags, struct timespec *tp, int timeout);
#endif
#if LELY_NO_CANFD
static int io_can_fd_write(int fd, const struct can_frame *frame, size_t nbytes,
		int dontwait);
#else
static int io_can_fd_write(int fd, const struct canfd_frame *frame,
		size_t nbytes, int timeout);
#endif
static int io_can_fd_write_msg(int fd, const struct can_msg *msg, int timeout);

static io_ctx_t *io_can_chan_impl_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_can_chan_impl_dev_get_exec(const io_dev_t *dev);
static size_t io_can_chan_impl_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_can_chan_impl_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_can_chan_impl_dev_vtbl = {
	&io_can_chan_impl_dev_get_ctx,
	&io_can_chan_impl_dev_get_exec,
	&io_can_chan_impl_dev_cancel,
	&io_can_chan_impl_dev_abort
};
// clang-format on

static io_dev_t *io_can_chan_impl_get_dev(const io_can_chan_t *chan);
static int io_can_chan_impl_get_flags(const io_can_chan_t *chan);
static int io_can_chan_impl_read(io_can_chan_t *chan, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout);
static void io_can_chan_impl_submit_read(
		io_can_chan_t *chan, struct io_can_chan_read *read);
static int io_can_chan_impl_write(
		io_can_chan_t *chan, const struct can_msg *msg, int timeout);
static void io_can_chan_impl_submit_write(
		io_can_chan_t *chan, struct io_can_chan_write *write);

// clang-format off
static const struct io_can_chan_vtbl io_can_chan_impl_vtbl = {
	&io_can_chan_impl_get_dev,
	&io_can_chan_impl_get_flags,
	&io_can_chan_impl_read,
	&io_can_chan_impl_submit_read,
	&io_can_chan_impl_write,
	&io_can_chan_impl_submit_write
};
// clang-format on

static void io_can_chan_impl_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_can_chan_impl_svc_vtbl = {
	NULL,
	&io_can_chan_impl_svc_shutdown
};
// clang-format on

struct io_can_chan_impl {
	const struct io_dev_vtbl *dev_vptr;
	const struct io_can_chan_vtbl *chan_vptr;
	io_poll_t *poll;
	struct io_svc svc;
	io_ctx_t *ctx;
	ev_exec_t *exec;
	struct io_poll_watch watch;
	ev_exec_t *strand;
	struct ev_task rxbuf_task;
	struct ev_task read_task;
	struct ev_task write_task;
#if !LELY_NO_THREADS
	pthread_mutex_t task_mtx;
#endif
	unsigned shutdown : 1;
	unsigned rxbuf_posted : 1;
	unsigned read_posted : 1;
	unsigned write_posted : 1;
	struct sllist read_queue;
	struct sllist write_queue;
	struct ev_task *current_write;
	struct sllist confirm_queue;
#if !LELY_NO_THREADS
	pthread_mutex_t io_mtx;
#endif
	struct io_can_buf rxbuf;
	int fd;
	int flags;
};

static void io_can_chan_impl_watch_func(
		struct io_poll_watch *watch, int events);
static void io_can_chan_impl_rxbuf_task_func(struct ev_task *task);
static void io_can_chan_impl_read_task_func(struct ev_task *task);
static void io_can_chan_impl_write_task_func(struct ev_task *task);

static inline struct io_can_chan_impl *io_can_chan_impl_from_dev(
		const io_dev_t *dev);
static inline struct io_can_chan_impl *io_can_chan_impl_from_chan(
		const io_can_chan_t *chan);
static inline struct io_can_chan_impl *io_can_chan_impl_from_svc(
		const struct io_svc *svc);

static int io_can_chan_impl_read_impl(struct io_can_chan_impl *impl,
		struct can_msg *msg, struct can_err *err, struct timespec *tp,
		int timeout);

static void io_can_chan_impl_do_pop(struct io_can_chan_impl *impl,
		struct sllist *read_queue, struct sllist *write_queue,
		struct sllist *confirm_queue, struct ev_task *task);

static void io_can_chan_impl_do_read(
		struct io_can_chan_impl *impl, struct sllist *queue);
static void io_can_chan_impl_do_confirm(struct io_can_chan_impl *impl,
		struct sllist *queue, const struct can_msg *msg);

static size_t io_can_chan_impl_do_abort_tasks(struct io_can_chan_impl *impl);

static int io_can_chan_impl_set_fd(
		struct io_can_chan_impl *impl, int fd, int flags);

void *
io_can_chan_alloc(void)
{
	struct io_can_chan_impl *impl = malloc(sizeof(*impl));
	// cppcheck-suppress memleak symbolName=impl
	return impl ? &impl->chan_vptr : NULL;
}

void
io_can_chan_free(void *ptr)
{
	if (ptr)
		free(io_can_chan_impl_from_chan(ptr));
}

io_can_chan_t *
io_can_chan_init(io_can_chan_t *chan, io_poll_t *poll, ev_exec_t *exec,
		size_t rxlen)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);
	assert(exec);
	io_ctx_t *ctx = poll ? io_poll_get_ctx(poll) : NULL;

	if (!rxlen)
		rxlen = LELY_IO_CAN_RXLEN;

	int errsv = 0;

	impl->dev_vptr = &io_can_chan_impl_dev_vtbl;
	impl->chan_vptr = &io_can_chan_impl_vtbl;

	impl->poll = poll;

	impl->svc = (struct io_svc)IO_SVC_INIT(&io_can_chan_impl_svc_vtbl);
	impl->ctx = ctx;

	impl->exec = exec;

	impl->watch = (struct io_poll_watch)IO_POLL_WATCH_INIT(
			&io_can_chan_impl_watch_func);

	impl->strand = ev_strand_create(impl->exec);
	if (!impl->strand) {
		errsv = errno;
		goto error_create_strand;
	}

	impl->rxbuf_task = (struct ev_task)EV_TASK_INIT(
			impl->strand, &io_can_chan_impl_rxbuf_task_func);
	impl->read_task = (struct ev_task)EV_TASK_INIT(
			impl->strand, &io_can_chan_impl_read_task_func);
	impl->write_task = (struct ev_task)EV_TASK_INIT(
			impl->strand, &io_can_chan_impl_write_task_func);

#if !LELY_NO_THREADS
	if ((errsv = pthread_mutex_init(&impl->task_mtx, NULL)))
		goto error_init_task_mtx;
#endif

	impl->shutdown = 0;
	impl->rxbuf_posted = 0;
	impl->read_posted = 0;
	impl->write_posted = 0;

	sllist_init(&impl->read_queue);
	sllist_init(&impl->write_queue);
	impl->current_write = NULL;
	sllist_init(&impl->confirm_queue);

#if !LELY_NO_THREADS
	if ((errsv = pthread_mutex_init(&impl->io_mtx, NULL)))
		goto error_init_io_mtx;
#endif

	if (io_can_buf_init(&impl->rxbuf, rxlen) == -1) {
		errsv = errno;
		goto error_init_rxbuf;
	}

	impl->fd = -1;
	impl->flags = 0;

	if (impl->ctx)
		io_ctx_insert(impl->ctx, &impl->svc);

	return chan;

	// io_can_buf_fini(&impl->rxbuf);
error_init_rxbuf:
#if !LELY_NO_THREADS
	pthread_mutex_destroy(&impl->io_mtx);
error_init_io_mtx:
	pthread_mutex_destroy(&impl->task_mtx);
error_init_task_mtx:
#endif
	ev_strand_destroy(impl->strand);
error_create_strand:
	errno = errsv;
	return NULL;
}

void
io_can_chan_fini(io_can_chan_t *chan)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

	if (impl->ctx)
		io_ctx_remove(impl->ctx, &impl->svc);
	// Cancel all pending operations.
	io_can_chan_impl_svc_shutdown(&impl->svc);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
	// If necessary, busy-wait until io_can_chan_impl_rxbuf_task_func(),
	// io_can_chan_impl_read_task_func() and
	// io_can_chan_impl_write_task_func() complete.
	while (impl->rxbuf_posted || impl->read_posted || impl->write_posted) {
		if (io_can_chan_impl_do_abort_tasks(impl))
			continue;
		pthread_mutex_unlock(&impl->task_mtx);
		do
			sched_yield();
		while (pthread_mutex_lock(&impl->task_mtx) == EINTR);
	}
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	// Close the socket.
	if (impl->fd != -1) {
		if (!impl->shutdown && impl->poll)
			io_poll_watch(impl->poll, impl->fd, 0, &impl->watch);
		close(impl->fd);
	}

	io_can_buf_fini(&impl->rxbuf);

#if !LELY_NO_THREADS
	pthread_mutex_destroy(&impl->io_mtx);
	pthread_mutex_destroy(&impl->task_mtx);
#endif

	ev_strand_destroy(impl->strand);
}

io_can_chan_t *
io_can_chan_create(io_poll_t *poll, ev_exec_t *exec, size_t rxlen)
{
	int errsv = 0;

	io_can_chan_t *chan = io_can_chan_alloc();
	if (!chan) {
		errsv = errno;
		goto error_alloc;
	}

	io_can_chan_t *tmp = io_can_chan_init(chan, poll, exec, rxlen);
	if (!tmp) {
		errsv = errno;
		goto error_init;
	}
	chan = tmp;

	return chan;

error_init:
	io_can_chan_free((void *)chan);
error_alloc:
	errno = errsv;
	return NULL;
}

void
io_can_chan_destroy(io_can_chan_t *chan)
{
	if (chan) {
		io_can_chan_fini(chan);
		io_can_chan_free((void *)chan);
	}
}

int
io_can_chan_get_handle(const io_can_chan_t *chan)
{
	const struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock((pthread_mutex_t *)&impl->io_mtx) == EINTR)
		;
#endif
	int fd = impl->fd;
#if !LELY_NO_THREADS
	pthread_mutex_unlock((pthread_mutex_t *)&impl->io_mtx);
#endif
	return fd;
}

int
io_can_chan_open(io_can_chan_t *chan, const io_can_ctrl_t *ctrl, int flags)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

	if (flags & ~(io_can_ctrl_get_flags(ctrl) | IO_CAN_BUS_FLAG_ERR)) {
		errno = EINVAL;
		return -1;
	}

	int errsv = 0;

	int fd = socket(AF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
	if (fd == -1) {
		errsv = errno;
		goto error_socket;
	}

	struct sockaddr_can addr = { .can_family = AF_CAN,
		.can_ifindex = io_can_ctrl_get_index(ctrl) };

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		errsv = errno;
		goto error_bind;
	}

	if (flags & IO_CAN_BUS_FLAG_ERR) {
		can_err_mask_t optval = CAN_ERR_MASK;
		// clang-format off
		if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &optval,
				sizeof(optval)) == -1) {
			// clang-format on
			errsv = errno;
			goto error_setsockopt;
		}
	}

#if !LELY_NO_CANFD
	if (flags & IO_CAN_BUS_FLAG_FDF) {
		int optval = 1;
		// clang-format off
		if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &optval,
				sizeof(optval)) == -1) {
			// clang-format on
			errsv = errno;
			goto error_setsockopt;
		}
	}
#endif

	if (io_can_fd_set_default(fd) == -1) {
		errsv = errno;
		goto error_set_default;
	}

	fd = io_can_chan_impl_set_fd(impl, fd, flags);
	if (fd != -1)
		close(fd);

	return 0;

error_set_default:
error_setsockopt:
error_bind:
	close(fd);
error_socket:
	errno = errsv;
	return -1;
}

int
io_can_chan_assign(io_can_chan_t *chan, int fd)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

	struct sockaddr_can addr = { .can_family = AF_UNSPEC };
	socklen_t addrlen = sizeof(addr);
	if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) == -1)
		return -1;
	if (addrlen < sizeof(addr) || addr.can_family != AF_CAN) {
		errno = ENODEV;
		return -1;
	}
	unsigned int ifindex = addr.can_ifindex;

	struct io_can_attr attr = IO_CAN_ATTR_INIT;
	if (io_can_attr_get(&attr, ifindex) == -1)
		return -1;
	int flags = attr.flags;

	{
		can_err_mask_t optval = 0;
		socklen_t optlen = sizeof(optval);
		// clang-format off
		if (getsockopt(fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &optval,
				&optlen) == -1)
			// clang-format on
			return -1;
		if (optval & CAN_ERR_MASK)
			flags |= IO_CAN_BUS_FLAG_ERR;
	}

#if !LELY_NO_CANFD
	// Check if CAN FD frames are allowed. If the check fails, we assume
	// they are not.
	{
		int errsv = errno;
		int optval = 0;
		socklen_t optlen = sizeof(optval);
		// clang-format off
		if (!getsockopt(fd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &optval,
				&optlen) && !optval)
			// clang-format on
			flags &= ~(IO_CAN_BUS_FLAG_FDF | IO_CAN_BUS_FLAG_BRS);
		errno = errsv;
	}
#endif

	if (io_can_fd_set_default(fd) == -1)
		return -1;

	fd = io_can_chan_impl_set_fd(impl, fd, flags);
	if (fd != -1)
		close(fd);

	return 0;
}

int
io_can_chan_release(io_can_chan_t *chan)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

	return io_can_chan_impl_set_fd(impl, -1, 0);
}

int
io_can_chan_is_open(const io_can_chan_t *chan)
{
	return io_can_chan_get_handle(chan) != -1;
}

int
io_can_chan_close(io_can_chan_t *chan)
{
	int fd = io_can_chan_release(chan);
	return fd != -1 ? close(fd) : 0;
}

static int
io_can_fd_set_default(int fd)
{
	int optval;

	// Enable local loopback.
	optval = 1;
	// clang-format off
	if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &optval,
			sizeof(optval)) == -1)
		// clang-format on
		return -1;

	// Enable the reception of CAN frames sent by this socket so we can
	// check for successful transmission.
	optval = 1;
	// clang-format off
	if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &optval,
			sizeof(optval)) == -1)
		// clang-format on
		return -1;

	// Set the size of the send buffer to its minimum value. This causes
	// write operations to block (or return EAGAIN) instead of returning
	// ENOBUFS.
	optval = 0;
	// clang-format off
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval))
			== -1)
		// clang-format on
		return -1;

	return 0;
}

static int
#if LELY_NO_CANFD
io_can_fd_read(int fd, struct can_frame *frame, size_t *pnbytes, int *pflags,
		struct timespec *tp, int timeout)
#else
io_can_fd_read(int fd, struct canfd_frame *frame, size_t *pnbytes, int *pflags,
		struct timespec *tp, int timeout)
#endif
{
	struct iovec iov = { .iov_base = (void *)frame,
		.iov_len = sizeof(*frame) };
	struct msghdr msg = { .msg_iov = &iov, .msg_iovlen = 1 };

	ssize_t result;
	for (;;) {
		result = io_fd_recvmsg(fd, &msg, 0, timeout);
		if (result < 0)
			return result;
#if LELY_NO_CANFD
		if (result == CAN_MTU)
#else
		if (result == CAN_MTU || result == CANFD_MTU)
#endif
			break;
		if (timeout > 0)
			timeout = 0;
	}

	if (pnbytes)
		*pnbytes = result;

	if (pflags)
		*pflags = msg.msg_flags;

	if (tp) {
		struct timeval tv = { 0, 0 };
		if (ioctl(fd, SIOCGSTAMP, &tv) == -1)
			return -1;
		tp->tv_sec = tv.tv_sec;
		tp->tv_nsec = tv.tv_usec * 1000;
	}

	return 0;
}

static int
#if LELY_NO_CANFD
io_can_fd_write(int fd, const struct can_frame *frame, size_t nbytes,
		int timeout)
#else
io_can_fd_write(int fd, const struct canfd_frame *frame, size_t nbytes,
		int timeout)
#endif
{
	struct iovec iov = { .iov_base = (void *)frame, .iov_len = nbytes };
	struct msghdr msg = { .msg_iov = &iov, .msg_iovlen = 1 };

	return io_fd_sendmsg(fd, &msg, 0, timeout) > 0 ? 0 : -1;
}

static int
io_can_fd_write_msg(int fd, const struct can_msg *msg, int timeout)
{
	assert(msg);

	// Convert the frame to the SocketCAN format.
	struct io_can_frame frame;
#if !LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_FDF) {
		if (can_msg2canfd_frame(msg, &frame.frame) == -1) {
			errno = EINVAL;
			return -1;
		}
		frame.nbytes = CANFD_MTU;
	} else {
#endif
		if (can_msg2can_frame(msg, (struct can_frame *)&frame.frame)
				== -1) {
			errno = EINVAL;
			return -1;
		}
		frame.nbytes = CAN_MTU;
#if !LELY_NO_CANFD
	}
#endif

	return io_can_fd_write(fd, &frame.frame, frame.nbytes, timeout);
}

static io_ctx_t *
io_can_chan_impl_dev_get_ctx(const io_dev_t *dev)
{
	const struct io_can_chan_impl *impl = io_can_chan_impl_from_dev(dev);

	return impl->ctx;
}

static ev_exec_t *
io_can_chan_impl_dev_get_exec(const io_dev_t *dev)
{
	const struct io_can_chan_impl *impl = io_can_chan_impl_from_dev(dev);

	return impl->exec;
}

static size_t
io_can_chan_impl_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_dev(dev);

	size_t n = 0;

	struct sllist read_queue, write_queue, confirm_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);
	sllist_init(&confirm_queue);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	io_can_chan_impl_do_pop(
			impl, &read_queue, &write_queue, &confirm_queue, task);
	// Mark the ongoing write operation as canceled, if necessary.
	if (impl->current_write && (!task || task == impl->current_write)) {
		impl->current_write = NULL;
		n++;
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	size_t nread = io_can_chan_read_queue_post(&read_queue, -1, ECANCELED);
	n = n < SIZE_MAX - nread ? n + nread : SIZE_MAX;
	size_t nwrite = io_can_chan_write_queue_post(&write_queue, ECANCELED);
	n = n < SIZE_MAX - nwrite ? n + nwrite : SIZE_MAX;
	size_t nconfirm =
			io_can_chan_write_queue_post(&confirm_queue, ECANCELED);
	n = n < SIZE_MAX - nconfirm ? n + nconfirm : SIZE_MAX;

	return n;
}

static size_t
io_can_chan_impl_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	io_can_chan_impl_do_pop(impl, &queue, &queue, &queue, task);
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	return ev_task_queue_abort(&queue);
}

static io_dev_t *
io_can_chan_impl_get_dev(const io_can_chan_t *chan)
{
	const struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

	return &impl->dev_vptr;
}

static int
io_can_chan_impl_get_flags(const io_can_chan_t *chan)
{
	const struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock((pthread_mutex_t *)&impl->io_mtx) == EINTR)
		;
#endif
	int flags = impl->flags;
#if !LELY_NO_THREADS
	pthread_mutex_unlock((pthread_mutex_t *)&impl->io_mtx);
#endif
	return flags;
}

static int
io_can_chan_impl_read(io_can_chan_t *chan, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

	return io_can_chan_impl_read_impl(impl, msg, err, tp, timeout);
}

static void
io_can_chan_impl_submit_read(io_can_chan_t *chan, struct io_can_chan_read *read)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);
	assert(read);
	struct ev_task *task = &read->task;

	if (!task->exec)
		task->exec = impl->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	if (impl->shutdown) {
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->task_mtx);
#endif
		io_can_chan_read_post(read, -1, ECANCELED);
	} else {
		int post_read = !impl->read_posted
				&& sllist_empty(&impl->read_queue);
		sllist_push_back(&impl->read_queue, &task->_node);
		if (post_read)
			impl->read_posted = 1;
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->task_mtx);
#endif
		if (post_read)
			ev_exec_post(impl->read_task.exec, &impl->read_task);
	}
}

static int
io_can_chan_impl_write(
		io_can_chan_t *chan, const struct can_msg *msg, int timeout)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);

#if !LELY_NO_CANFD
	int flags = 0;
	if (msg->flags & CAN_FLAG_FDF)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (msg->flags & CAN_FLAG_BRS)
		flags |= IO_CAN_BUS_FLAG_BRS;
#endif

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
		;
#endif
#if !LELY_NO_CANFD
	if ((flags & impl->flags) != flags) {
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->io_mtx);
#endif
		errno = EINVAL;
		return -1;
	}
#endif
	int fd = impl->fd;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->io_mtx);
#endif

	return io_can_fd_write_msg(fd, msg, timeout);
}

static void
io_can_chan_impl_submit_write(
		io_can_chan_t *chan, struct io_can_chan_write *write)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_chan(chan);
	assert(write);
	assert(write->msg);
	struct ev_task *task = &write->task;

#if !LELY_NO_CANFD
	int flags = 0;
	if (write->msg->flags & CAN_FLAG_FDF)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (write->msg->flags & CAN_FLAG_BRS)
		flags |= IO_CAN_BUS_FLAG_BRS;
#endif

	if (!task->exec)
		task->exec = impl->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	if (impl->shutdown) {
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->task_mtx);
#endif
		io_can_chan_write_post(write, ECANCELED);
#if !LELY_NO_CANFD
	} else if ((flags & impl->flags) != flags) {
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->task_mtx);
#endif
		io_can_chan_write_post(write, EINVAL);
#endif
	} else {
		int post_write = !impl->write_posted
				&& sllist_empty(&impl->write_queue);
		sllist_push_back(&impl->write_queue, &task->_node);
		if (post_write)
			impl->write_posted = 1;
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->task_mtx);
#endif
		if (post_write)
			ev_exec_post(impl->write_task.exec, &impl->write_task);
	}
}

static void
io_can_chan_impl_svc_shutdown(struct io_svc *svc)
{
	struct io_can_chan_impl *impl = io_can_chan_impl_from_svc(svc);
	io_dev_t *dev = &impl->dev_vptr;

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	int shutdown = !impl->shutdown;
	impl->shutdown = 1;
	if (shutdown) {
#if !LELY_NO_THREADS
		while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
			;
#endif
		if (impl->poll && impl->fd != -1)
			// Stop monitoring I/O events.
			io_poll_watch(impl->poll, impl->fd, 0, &impl->watch);
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->io_mtx);
#endif
		// Try to abort io_can_chan_impl_rxbuf_task_func(),
		// io_can_chan_impl_read_task_func() and
		// io_can_chan_impl_write_task_func().
		io_can_chan_impl_do_abort_tasks(impl);
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	if (shutdown)
		// Cancel all pending operations.
		io_can_chan_impl_dev_cancel(dev, NULL);
}

static void
io_can_chan_impl_watch_func(struct io_poll_watch *watch, int events)
{
	assert(watch);
	struct io_can_chan_impl *impl =
			structof(watch, struct io_can_chan_impl, watch);

	struct ev_task *write_task = NULL;

	int errc = 0;
	if (events & IO_EVENT_ERR) {
		int errsv = errno;
#if !LELY_NO_THREADS
		while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
			;
#endif
		// clang-format off
		if (getsockopt(impl->fd, SOL_SOCKET, SO_ERROR, &errc,
				&(socklen_t){ sizeof(int) }) == -1)
			// clang-format on
			errc = errno;
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->io_mtx);
#endif
		errno = errsv;
	}

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	// Report a socket error to the first pending write operation.
	if (errc)
		write_task = ev_task_from_node(
				sllist_pop_front(&impl->write_queue));

	// Process incoming CAN frames.
	int post_rxbuf = 0;
	if ((events & (IO_EVENT_IN | IO_EVENT_ERR)) && !impl->shutdown) {
		post_rxbuf = !impl->rxbuf_posted;
		impl->rxbuf_posted = 1;
	}

	// Retry any pending write operations.
	int post_write = 0;
	if ((events & (IO_EVENT_OUT | IO_EVENT_ERR))
			&& !sllist_empty(&impl->write_queue)
			&& !impl->shutdown) {
		post_write = !impl->write_posted;
		impl->write_posted = 1;
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	if (write_task) {
		struct io_can_chan_write *write =
				io_can_chan_write_from_task(write_task);
		io_can_chan_write_post(write, errc);
	}

	if (post_rxbuf)
		ev_exec_post(impl->rxbuf_task.exec, &impl->rxbuf_task);
	if (post_write)
		ev_exec_post(impl->write_task.exec, &impl->write_task);
}

static void
io_can_chan_impl_rxbuf_task_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_impl *impl =
			structof(task, struct io_can_chan_impl, rxbuf_task);

	int errsv = errno;

	struct sllist queue;
	sllist_init(&queue);
	struct io_can_chan_read *read = NULL;
	int result = 0;
	int wouldblock = 0;

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	while (!result && !wouldblock) {
#if !LELY_NO_THREADS
		while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
			;
#endif
		// Process any pending read operations to clear the read buffer
		// as much as possible.
		io_can_chan_impl_do_read(impl, &queue);
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->task_mtx);
#endif
		struct io_can_frame frame;
		int flags = 0;
		// Try to read a CAN or CAN FD format frame from the CAN bus and
		// push it to the read buffer unless it is a write confirmation.
		result = io_can_fd_read(impl->fd, &frame.frame, &frame.nbytes,
				&flags, &frame.ts,
				impl->poll ? 0 : LELY_IO_RX_TIMEOUT);
		int errc = !result ? 0 : errno;
		if (!result && !(flags & MSG_CONFIRM)
				&& io_can_buf_capacity(&impl->rxbuf))
			*io_can_buf_push(&impl->rxbuf) = frame;
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->io_mtx);
#endif
		wouldblock = errc == EAGAIN || errc == EWOULDBLOCK;
		// Convert the frame from the SocktetCAN format if it is a write
		// confirmation.
		struct can_msg msg;
		if (!result && (flags & MSG_CONFIRM)) {
			void *src = &frame.frame;
#if !LELY_NO_CANFD
			if (frame.nbytes == CANFD_MTU)
				canfd_frame2can_msg(src, &msg);
			else
#endif
				can_frame2can_msg(src, &msg);
		}
#if !LELY_NO_THREADS
		while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
			;
#endif
		if (!result && (flags & MSG_CONFIRM)) {
			// Process the write confirmation, if any.
			io_can_chan_impl_do_confirm(impl, &queue, &msg);
		} else if (result < 0 && !wouldblock) {
			// Cancel all outstanding read operations on error.
			while ((task = ev_task_from_node(sllist_pop_front(
						&impl->read_queue)))) {
				read = io_can_chan_read_from_task(task);
				read->r.result = result;
				read->r.errc = errc;
				sllist_push_back(&queue, &task->_node);
			}
		}
		// Only read a single frame at a time in blocking mode.
		if (!impl->poll)
			break;
	}
#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
		;
#endif
	// Process any pending read operations.
	io_can_chan_impl_do_read(impl, &queue);
	// clang-format off
	if (impl->poll && wouldblock && !(sllist_empty(&impl->read_queue)
			&& sllist_empty(&impl->confirm_queue)) && impl->fd != -1
			&& !impl->shutdown) {
		// clang-format on
		int events = IO_EVENT_IN;
		if (!impl->write_posted && !sllist_empty(&impl->write_queue))
			events |= IO_EVENT_OUT;
		io_poll_watch(impl->poll, impl->fd, events, &impl->watch);
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->io_mtx);
#endif
	// clang-format off
	int post_rxbuf = impl->rxbuf_posted =
			!(sllist_empty(&impl->read_queue)
			&& sllist_empty(&impl->confirm_queue))
			&& !(impl->poll && wouldblock)
			&& !impl->shutdown;
	// clang-format on
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	ev_task_queue_post(&queue);

	if (post_rxbuf)
		ev_exec_post(impl->rxbuf_task.exec, &impl->rxbuf_task);

	errno = errsv;
}

static void
io_can_chan_impl_read_task_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_impl *impl =
			structof(task, struct io_can_chan_impl, read_task);

	int errsv = errno;

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
	while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
		;
#endif
	io_can_chan_impl_do_read(impl, &queue);
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->io_mtx);
#endif
	int post_rxbuf = impl->read_posted = !impl->rxbuf_posted
			&& !sllist_empty(&impl->read_queue) && !impl->shutdown;
	if (post_rxbuf)
		impl->rxbuf_posted = 1;
	impl->read_posted = 0;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	ev_task_queue_post(&queue);

	if (post_rxbuf)
		ev_exec_post(impl->rxbuf_task.exec, &impl->rxbuf_task);

	errno = errsv;
}

static void
io_can_chan_impl_write_task_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_impl *impl =
			structof(task, struct io_can_chan_impl, write_task);

	int errsv = errno;

	struct io_can_chan_write *write = NULL;
	int wouldblock = 0;

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
#endif
	// Try to process all pending write operations at once, unless we're in
	// blocking mode.
	while ((task = impl->current_write = ev_task_from_node(
				sllist_pop_front(&impl->write_queue)))) {
		write = io_can_chan_write_from_task(task);
#if !LELY_NO_THREADS
		while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
			;
		pthread_mutex_unlock(&impl->task_mtx);
#endif
		int result = io_can_fd_write_msg(impl->fd, write->msg,
				impl->poll ? 0 : LELY_IO_TX_TIMEOUT);
		int errc = !result ? 0 : errno;
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->io_mtx);
#endif
		wouldblock = errc == EAGAIN || errc == EWOULDBLOCK;
		if (!wouldblock && errc)
			// The operation failed immediately.
			io_can_chan_write_post(write, errc);
#if !LELY_NO_THREADS
		while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
			;
#endif
		if (!errc)
			// Wait for the write confirmation.
			sllist_push_back(&impl->confirm_queue, &task->_node);
		if (task == impl->current_write) {
			// Put the write operation back on the queue if it would
			// block, unless it was canceled.
			if (wouldblock) {
				sllist_push_front(&impl->write_queue,
						&task->_node);
				task = NULL;
			}
			impl->current_write = NULL;
		}
		assert(!impl->current_write);
		// Stop if the operation did or would block.
		if (!impl->poll || wouldblock)
			break;
	}
#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
		;
#endif
	// If the operation would block, start watching the file descriptor.
	// (unless it has been closed in the mean time).
	if (impl->poll && wouldblock && !sllist_empty(&impl->write_queue)
			&& impl->fd != -1 && !impl->shutdown) {
		int events = IO_EVENT_OUT;
		// clang-format off
		if (!impl->rxbuf_posted && (!sllist_empty(&impl->read_queue)
				|| !sllist_empty(&impl->confirm_queue)))
			// clang-format on
			events |= IO_EVENT_IN;
		io_poll_watch(impl->poll, impl->fd, events, &impl->watch);
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->io_mtx);
#endif
	// Start reading CAN frames if we're waiting for a write confirmation.
	int post_rxbuf = !impl->rxbuf_posted
			&& !sllist_empty(&impl->confirm_queue)
			&& !impl->shutdown;
	if (post_rxbuf)
		impl->rxbuf_posted = 1;
	// Repost this task if any write operations remain in the queue, unless
	// we're waiting the file descriptor to become ready.
	int post_write = impl->write_posted = !sllist_empty(&impl->write_queue)
			&& !(impl->poll && wouldblock) && !impl->shutdown;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	if (task && wouldblock)
		// The operation would block but was canceled before it could be
		// requeued.
		io_can_chan_write_post(
				io_can_chan_write_from_task(task), ECANCELED);

	if (post_rxbuf)
		ev_exec_post(impl->rxbuf_task.exec, &impl->rxbuf_task);

	if (post_write)
		ev_exec_post(impl->write_task.exec, &impl->write_task);

	errno = errsv;
}

static inline struct io_can_chan_impl *
io_can_chan_impl_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, struct io_can_chan_impl, dev_vptr);
}

static inline struct io_can_chan_impl *
io_can_chan_impl_from_chan(const io_can_chan_t *chan)
{
	assert(chan);

	return structof(chan, struct io_can_chan_impl, chan_vptr);
}

static inline struct io_can_chan_impl *
io_can_chan_impl_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, struct io_can_chan_impl, svc);
}

static int
io_can_chan_impl_read_impl(struct io_can_chan_impl *impl, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout)
{
	assert(impl);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock((pthread_mutex_t *)&impl->io_mtx) == EINTR)
		;
#endif
	struct io_can_frame *frame = io_can_buf_pop(&impl->rxbuf);
	int fd = impl->fd;
#if !LELY_NO_THREADS
	pthread_mutex_unlock((pthread_mutex_t *)&impl->io_mtx);
#endif

	struct io_can_frame frame_;
	if (!frame) {
		frame = &frame_;
		int result;
		int flags = 0;
		do
			result = io_can_fd_read(fd, &frame->frame,
					&frame->nbytes, &flags, &frame->ts,
					timeout);
		while (result > 0 && (flags & MSG_CONFIRM));
		if (result < 0)
			return result;
	}

	int is_err = can_frame2can_err((struct can_frame *)&frame->frame, err);
	if (is_err == -1)
		return -1;

	if (!is_err && msg) {
#if !LELY_NO_CANFD
		if (frame->nbytes == CANFD_MTU)
			canfd_frame2can_msg(&frame->frame, msg);
		else
#endif
			can_frame2can_msg(
					(struct can_frame *)&frame->frame, msg);
	}

	if (tp)
		*tp = frame->ts;

	return !is_err;
}

static void
io_can_chan_impl_do_pop(struct io_can_chan_impl *impl,
		struct sllist *read_queue, struct sllist *write_queue,
		struct sllist *confirm_queue, struct ev_task *task)
{
	assert(impl);
	assert(read_queue);
	assert(write_queue);
	assert(confirm_queue);

	if (!task) {
		sllist_append(read_queue, &impl->read_queue);
		sllist_append(write_queue, &impl->write_queue);
		sllist_append(confirm_queue, &impl->confirm_queue);
	} else if (sllist_remove(&impl->read_queue, &task->_node)) {
		sllist_push_back(read_queue, &task->_node);
	} else if (sllist_remove(&impl->write_queue, &task->_node)) {
		sllist_push_back(write_queue, &task->_node);
	} else if (sllist_remove(&impl->confirm_queue, &task->_node)) {
		sllist_push_back(confirm_queue, &task->_node);
	}
}

static void
io_can_chan_impl_do_read(struct io_can_chan_impl *impl, struct sllist *queue)
{
	assert(impl);
	assert(queue);

	struct slnode *node;
	while ((node = sllist_first(&impl->read_queue))) {
		struct io_can_frame *frame = io_can_buf_pop(&impl->rxbuf);
		if (!frame)
			break;
		void *data = &frame->frame;

		struct ev_task *task = ev_task_from_node(node);
		struct io_can_chan_read *read =
				io_can_chan_read_from_task(task);

		int is_err = can_frame2can_err(data, read->err);
		if (is_err == -1) {
			continue;
		} else if (is_err) {
			read->r.result = 0;
		} else {
			if (read->msg) {
#if !LELY_NO_CANFD
				if (frame->nbytes == CANFD_MTU)
					canfd_frame2can_msg(data, read->msg);
				else
#endif
					can_frame2can_msg(data, read->msg);
			}
			read->r.result = 1;
		}
		read->r.errc = 0;
		if (read->tp)
			*read->tp = frame->ts;

		sllist_pop_front(&impl->read_queue);
		sllist_push_back(queue, node);
	}
}

static void
io_can_chan_impl_do_confirm(struct io_can_chan_impl *impl, struct sllist *queue,
		const struct can_msg *msg)
{
	assert(impl);
	assert(queue);
	assert(msg);

	// Find the matching write operation.
	struct slnode *node = sllist_first(&impl->confirm_queue);
	while (node) {
		struct io_can_chan_write *write = io_can_chan_write_from_task(
				ev_task_from_node(node));
		if (!can_msg_cmp(msg, write->msg))
			break;
		node = node->next;
	}
	if (!node)
		return;

	// Complete the matching write operation. Any preceding write operations
	// waiting for confirmation are considered to have failed.
	struct ev_task *task;
	while ((task = ev_task_from_node(
				sllist_pop_front(&impl->confirm_queue)))) {
		sllist_push_front(queue, &task->_node);
		struct io_can_chan_write *write =
				io_can_chan_write_from_task(task);
		if (&task->_node == node) {
			write->errc = 0;
			break;
		} else {
			write->errc = EIO;
		}
	}
}

static size_t
io_can_chan_impl_do_abort_tasks(struct io_can_chan_impl *impl)
{
	assert(impl);

	size_t n = 0;

	// Try to abort io_can_chan_impl_rxbuf_task_func().
	// clang-format off
	if (impl->rxbuf_posted && ev_exec_abort(impl->rxbuf_task.exec,
			&impl->rxbuf_task)) {
		// clang-format on
		impl->rxbuf_posted = 0;
		n++;
	}

	// Try to abort io_can_chan_impl_read_task_func().
	// clang-format off
	if (impl->read_posted && ev_exec_abort(impl->read_task.exec,
			&impl->read_task)) {
		// clang-format on
		impl->read_posted = 0;
		n++;
	}

	// Try to abort io_can_chan_impl_write_task_func().
	// clang-format off
	if (impl->write_posted && ev_exec_abort(impl->write_task.exec,
			&impl->write_task)) {
		// clang-format on
		impl->write_posted = 0;
		n++;
	}

	return n;
}

static int
io_can_chan_impl_set_fd(struct io_can_chan_impl *impl, int fd, int flags)
{
	assert(impl);
	assert(!(flags & ~IO_CAN_BUS_FLAG_MASK));

	struct sllist read_queue, write_queue, confirm_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);
	sllist_init(&confirm_queue);

#if !LELY_NO_THREADS
	while (pthread_mutex_lock(&impl->task_mtx) == EINTR)
		;
	while (pthread_mutex_lock(&impl->io_mtx) == EINTR)
		;
#endif

	if (impl->fd != -1 && !impl->shutdown && impl->poll)
		// Stop monitoring I/O events.
		io_poll_watch(impl->poll, impl->fd, 0, &impl->watch);

	io_can_buf_clear(&impl->rxbuf);

	int tmp = impl->fd;
	impl->fd = fd;
	fd = tmp;

	impl->flags = flags;

#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->io_mtx);
#endif

	// Cancel pending operations.
	sllist_append(&read_queue, &impl->read_queue);
	sllist_append(&write_queue, &impl->write_queue);
	sllist_append(&confirm_queue, &impl->confirm_queue);

	// Mark the ongoing write operation as canceled, if necessary.
	impl->current_write = NULL;

#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->task_mtx);
#endif

	io_can_chan_read_queue_post(&read_queue, -1, ECANCELED);
	io_can_chan_write_queue_post(&write_queue, ECANCELED);
	io_can_chan_write_queue_post(&confirm_queue, ECANCELED);

	return fd;
}

#endif // __linux__
