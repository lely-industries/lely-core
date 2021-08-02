/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * virtual CAN bus
 *
 * @see lely/io2/vcan.h
 *
 * @copyright 2019-2021 Lely Industries N.V.
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

#include "can.h"

#if !LELY_NO_MALLOC

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/io2/ctx.h>
#include <lely/io2/vcan.h>
#include <lely/util/diag.h>
#include <lely/util/spscring.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef LELY_IO_VCAN_BITRATE
/// The default bitrate of a virtual CAN bus.
#define LELY_IO_VCAN_BITRATE 1000000
#endif

#ifndef LELY_IO_VCAN_RXLEN
/**
 * The default receive queue length (in number of CAN frames) of a vritual CAN
 * channel.
 */
#define LELY_IO_VCAN_RXLEN 1024
#endif

struct io_vcan_frame {
	int is_err;
	union {
		struct can_msg msg;
		struct can_err err;
	} u;
	struct timespec ts;
};

static int io_vcan_ctrl_stop(io_can_ctrl_t *ctrl);
static int io_vcan_ctrl_stopped(const io_can_ctrl_t *ctrl);
static int io_vcan_ctrl_restart(io_can_ctrl_t *ctrl);
static int io_vcan_ctrl_get_bitrate(
		const io_can_ctrl_t *ctrl, int *pnominal, int *pdata);
static int io_vcan_ctrl_set_bitrate(io_can_ctrl_t *ctrl, int nominal, int data);
static int io_vcan_ctrl_get_state(const io_can_ctrl_t *ctrl);

// clang-format off
static const struct io_can_ctrl_vtbl io_vcan_ctrl_vtbl = {
	&io_vcan_ctrl_stop,
	&io_vcan_ctrl_stopped,
	&io_vcan_ctrl_restart,
	&io_vcan_ctrl_get_bitrate,
	&io_vcan_ctrl_set_bitrate,
	&io_vcan_ctrl_get_state
};
// clang-format on

/// The implementation of a virtual CAN controller.
struct io_vcan_ctrl {
	/// A pointer to the virtual table for the CAN controller interface.
	const struct io_can_ctrl_vtbl *ctrl_vptr;
	/**
	 * A pointer to the clock used to obtain the timestamp when sending CAN
	 * frames.
	 */
	io_clock_t *clock;
	/// The flags specifying which CAN bus features are enabled.
	int flags;
#if !LELY_NO_THREADS
	/**
	 * The mutex protecting the controller and the list of virtual CAN
	 * channels.
	 */
	mtx_t mtx;
	/**
	 * The condition variable used to wake up blocked synchronous write
	 * operations.
	 */
	cnd_t cond;
#endif
	/// A flag indicating whether the controller is stopped.
	int stopped;
	/// The nominal bitrate.
	int nominal;
	/// The data bitrate.
	int data;
	/// The state of the virtual CAN bus.
	int state;
	/// The list of registered virtual CAN channels.
	struct sllist list;
};

static inline struct io_vcan_ctrl *io_vcan_ctrl_from_ctrl(
		const io_can_ctrl_t *ctrl);

static void io_vcan_ctrl_insert(io_can_ctrl_t *ctrl, io_can_chan_t *chan);
static void io_vcan_ctrl_remove(io_can_ctrl_t *ctrl, io_can_chan_t *chan);

static void io_vcan_ctrl_signal(struct spscring *ring, void *arg);

static int io_vcan_ctrl_write(io_can_ctrl_t *ctrl, io_can_chan_t *chan,
		const struct can_msg *msg, const struct can_err *err,
		int timeout);

static void io_vcan_ctrl_do_stop(struct io_vcan_ctrl *vcan_ctrl);

static io_ctx_t *io_vcan_chan_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_vcan_chan_dev_get_exec(const io_dev_t *dev);
static size_t io_vcan_chan_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_vcan_chan_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_vcan_chan_dev_vtbl = {
	&io_vcan_chan_dev_get_ctx,
	&io_vcan_chan_dev_get_exec,
	&io_vcan_chan_dev_cancel,
	&io_vcan_chan_dev_abort
};
// clang-format on

static io_dev_t *io_vcan_chan_get_dev(const io_can_chan_t *chan);
static int io_vcan_chan_get_flags(const io_can_chan_t *chan);
static int io_vcan_chan_read(io_can_chan_t *chan, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout);
static void io_vcan_chan_submit_read(
		io_can_chan_t *chan, struct io_can_chan_read *read);
static int io_vcan_chan_write(
		io_can_chan_t *chan, const struct can_msg *msg, int timeout);
static void io_vcan_chan_submit_write(
		io_can_chan_t *chan, struct io_can_chan_write *write);

// clang-format off
static const struct io_can_chan_vtbl io_vcan_chan_vtbl = {
	&io_vcan_chan_get_dev,
	&io_vcan_chan_get_flags,
	&io_vcan_chan_read,
	&io_vcan_chan_submit_read,
	&io_vcan_chan_write,
	&io_vcan_chan_submit_write
};
// clang-format on

static void io_vcan_chan_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_vcan_chan_svc_vtbl = {
	NULL,
	&io_vcan_chan_svc_shutdown
};
// clang-format on

/// The implementation of a virtual CAN channel.
struct io_vcan_chan {
	/// A pointer to the virtual table for the I/O device interface.
	const struct io_dev_vtbl *dev_vptr;
	/// A pointer to the virtual table for the CAN channel interface.
	const struct io_can_chan_vtbl *chan_vptr;
	/// The I/O service representing the channel.
	struct io_svc svc;
	/// A pointer to the I/O context with which the channel is registered.
	io_ctx_t *ctx;
	/// A pointer to the executor used to execute all I/O tasks.
	ev_exec_t *exec;
	/// The task responsible for initiating read operations.
	struct ev_task read_task;
	/// The task responsible for initiating write operations.
	struct ev_task write_task;
	/// The ring buffer used to control the receive queue.
	struct spscring rxring;
	/// The receive queue.
	struct io_vcan_frame *rxbuf;
#if !LELY_NO_THREADS
	/**
	 * The mutex protecting the channel and the queues of pending
	 * operations.
	 */
	mtx_t mtx;
	/**
	 * The condition variable used to wake up blocked synchronous read
	 * operations.
	 */
	cnd_t cond;
#endif
	/**
	 * A pointer to the virtual CAN controller with which this channel is
	 * registered.
	 */
	io_can_ctrl_t *ctrl;
	/**
	 * The node of the channel in the list of the virtual CAN controller.
	 * #node is protected by the mutex in #io_vcan_ctrl.
	 */
	struct slnode node;
	/// A flag indicating whether the I/O service has been shut down.
	unsigned shutdown : 1;
	/// A flag indicating the virtual CAN controller is stopped.
	unsigned stopped : 1;
	/// A flag indicating whether #read_task has been posted to #exec.
	unsigned read_posted : 1;
	/// A flag indicating whether #write_task has been posted to #exec.
	unsigned write_posted : 1;
	/// The queue containing pending read operations.
	struct sllist read_queue;
	/// The queue containing pending write operations.
	struct sllist write_queue;
	/// The write operation currently being executed.
	struct ev_task *current_write;
};

static void io_vcan_chan_read_task_func(struct ev_task *task);
static void io_vcan_chan_write_task_func(struct ev_task *task);

static inline struct io_vcan_chan *io_vcan_chan_from_dev(const io_dev_t *dev);
static inline struct io_vcan_chan *io_vcan_chan_from_chan(
		const io_can_chan_t *chan);
static inline struct io_vcan_chan *io_vcan_chan_from_svc(
		const struct io_svc *svc);

static void io_vcan_chan_signal(struct spscring *ring, void *arg);

static void io_vcan_chan_do_pop(struct io_vcan_chan *vcan,
		struct sllist *read_queue, struct sllist *write_queue,
		struct ev_task *task);

static size_t io_vcan_chan_do_abort_tasks(struct io_vcan_chan *vcan);

void *
io_vcan_ctrl_alloc(void)
{
	struct io_vcan_ctrl *vcan = malloc(sizeof(*vcan));
	if (!vcan) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return NULL;
	}
	// Suppress a GCC maybe-uninitialized warning.
	vcan->ctrl_vptr = NULL;
	// cppcheck-suppress memleak symbolName=vcan
	return &vcan->ctrl_vptr;
}

void
io_vcan_ctrl_free(void *ptr)
{
	if (ptr)
		free(io_vcan_ctrl_from_ctrl(ptr));
}

io_can_ctrl_t *
io_vcan_ctrl_init(io_can_ctrl_t *ctrl, io_clock_t *clock, int flags,
		int nominal, int data, int state)
{
	struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);
	assert(clock);

	if (!nominal)
		nominal = LELY_IO_VCAN_BITRATE;
#if !LELY_NO_CANFD
	if (!data)
		data = nominal;
#else
	(void)data;
#endif

#if !LELY_NO_THREADS
	int errc = 0;
#endif

	vcan->ctrl_vptr = &io_vcan_ctrl_vtbl;

	vcan->clock = clock;

	vcan->flags = flags;

#if !LELY_NO_THREADS
	if (mtx_init(&vcan->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}

	if (cnd_init(&vcan->cond) != thrd_success) {
		errc = get_errc();
		goto error_init_cond;
	}
#endif

	vcan->stopped = state == CAN_STATE_STOPPED;
	vcan->nominal = nominal;
	vcan->data = 0;
#if !LELY_NO_CANFD
	if (vcan->flags & IO_CAN_BUS_FLAG_BRS)
		vcan->data = data;
#endif
	vcan->state = state;

	sllist_init(&vcan->list);

	return ctrl;

#if !LELY_NO_THREADS
	// cnd_destroy(&vcan->cond);
error_init_cond:
	mtx_destroy(&vcan->mtx);
error_init_mtx:
	set_errc(errc);
	return NULL;
#endif
}

void
io_vcan_ctrl_fini(io_can_ctrl_t *ctrl)
{
	struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);

	assert(sllist_empty(&vcan->list));

#if LELY_NO_THREADS
	(void)vcan;
#else
	cnd_destroy(&vcan->cond);
	mtx_destroy(&vcan->mtx);
#endif
}

io_can_ctrl_t *
io_vcan_ctrl_create(
		io_clock_t *clock, int flags, int nominal, int data, int state)
{
	int errc = 0;

	io_can_ctrl_t *ctrl = io_vcan_ctrl_alloc();
	if (!ctrl) {
		errc = get_errc();
		goto error_alloc;
	}

	io_can_ctrl_t *tmp = io_vcan_ctrl_init(
			ctrl, clock, flags, nominal, data, state);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	ctrl = tmp;

	return ctrl;

error_init:
	io_vcan_ctrl_free((void *)ctrl);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_vcan_ctrl_destroy(io_can_ctrl_t *ctrl)
{
	if (ctrl) {
		io_vcan_ctrl_fini(ctrl);
		io_vcan_ctrl_free((void *)ctrl);
	}
}

void
io_vcan_ctrl_set_state(io_can_ctrl_t *ctrl, int state)
{
	struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	int changed = vcan->state != state;
	if (changed) {
		vcan->state = state;
		if (!vcan->stopped && vcan->state == CAN_STATE_STOPPED)
			io_vcan_ctrl_do_stop(vcan);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif

	// Send an error frame if the state of the CAN bus changed. We ignore
	// write errors, since the user does not expect this operation to block.
	if (changed && state != CAN_STATE_STOPPED
			&& (vcan->flags & IO_CAN_BUS_FLAG_ERR)) {
		struct can_err err = { .state = state };
		int errsv = get_errc();
		if (io_vcan_ctrl_write_err(ctrl, &err, 0) == -1)
			set_errc(errsv);
	}
}

int
io_vcan_ctrl_write_msg(
		io_can_ctrl_t *ctrl, const struct can_msg *msg, int timeout)
{
	return io_vcan_ctrl_write(ctrl, NULL, msg, NULL, timeout);
}

int
io_vcan_ctrl_write_err(
		io_can_ctrl_t *ctrl, const struct can_err *err, int timeout)
{
	return io_vcan_ctrl_write(ctrl, NULL, NULL, err, timeout);
}

void *
io_vcan_chan_alloc(void)
{
	struct io_vcan_chan *vcan = malloc(sizeof(*vcan));
	if (!vcan) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return NULL;
	}
	// Suppress a GCC maybe-uninitialized warning.
	vcan->chan_vptr = NULL;
	// cppcheck-suppress memleak symbolName=vcan
	return &vcan->chan_vptr;
}

void
io_vcan_chan_free(void *ptr)
{
	if (ptr)
		free(io_vcan_chan_from_chan(ptr));
}

io_can_chan_t *
io_vcan_chan_init(io_can_chan_t *chan, io_ctx_t *ctx, ev_exec_t *exec,
		size_t rxlen)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_chan(chan);
	assert(ctx);
	assert(exec);

	if (!rxlen)
		rxlen = LELY_IO_VCAN_RXLEN;

	int errc = 0;

	vcan->dev_vptr = &io_vcan_chan_dev_vtbl;
	vcan->chan_vptr = &io_vcan_chan_vtbl;

	vcan->svc = (struct io_svc)IO_SVC_INIT(&io_vcan_chan_svc_vtbl);
	vcan->ctx = ctx;

	vcan->exec = exec;

	vcan->read_task = (struct ev_task)EV_TASK_INIT(
			vcan->exec, &io_vcan_chan_read_task_func);
	vcan->write_task = (struct ev_task)EV_TASK_INIT(
			vcan->exec, &io_vcan_chan_write_task_func);

	spscring_init(&vcan->rxring, rxlen);
	vcan->rxbuf = calloc(rxlen, sizeof(struct io_vcan_frame));
	if (!vcan->rxbuf) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_alloc_rxbuf;
	}

#if !LELY_NO_THREADS
	if (mtx_init(&vcan->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}

	if (cnd_init(&vcan->cond) != thrd_success) {
		errc = get_errc();
		goto error_init_cond;
	}
#endif

	vcan->ctrl = NULL;
	slnode_init(&vcan->node);

	vcan->shutdown = 0;
	vcan->stopped = 0;
	vcan->read_posted = 0;
	vcan->write_posted = 0;

	sllist_init(&vcan->read_queue);
	sllist_init(&vcan->write_queue);
	vcan->current_write = NULL;

	io_ctx_insert(vcan->ctx, &vcan->svc);

	return chan;

#if !LELY_NO_THREADS
	// cnd_destroy(&vcan->cond);
error_init_cond:
	mtx_destroy(&vcan->mtx);
error_init_mtx:
#endif
	free(vcan->rxbuf);
error_alloc_rxbuf:
	set_errc(errc);
	return NULL;
}

void
io_vcan_chan_fini(io_can_chan_t *chan)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_chan(chan);

	// Close the CAN channel.
	io_vcan_chan_close(chan);

	io_ctx_remove(vcan->ctx, &vcan->svc);
	// Cancel all pending tasks.
	io_vcan_chan_svc_shutdown(&vcan->svc);

	// Abort any consumer wait operation. Any producer wait operation was
	// canceled when the channel was closed.
	spscring_c_abort_wait(&vcan->rxring);

#if !LELY_NO_THREADS
	int warning = 0;
	mtx_lock(&vcan->mtx);
	// If necessary, busy-wait until io_vcan_chan_read_task_func() and
	// io_vcan_chan_write_task_func() complete.
	while (vcan->read_posted || vcan->write_posted) {
		if (io_vcan_chan_do_abort_tasks(vcan))
			continue;
		mtx_unlock(&vcan->mtx);
		if (!warning) {
			warning = 1;
			diag(DIAG_WARNING, 0,
					"io_vcan_chan_fini() invoked with pending operations");
		}
		thrd_yield();
		mtx_lock(&vcan->mtx);
	}
	mtx_unlock(&vcan->mtx);

	cnd_destroy(&vcan->cond);
	mtx_destroy(&vcan->mtx);
#endif

	free(vcan->rxbuf);
}

io_can_chan_t *
io_vcan_chan_create(io_ctx_t *ctx, ev_exec_t *exec, size_t rxlen)
{
	int errc = 0;

	io_can_chan_t *chan = io_vcan_chan_alloc();
	if (!chan) {
		errc = get_errc();
		goto error_alloc;
	}

	io_can_chan_t *tmp = io_vcan_chan_init(chan, ctx, exec, rxlen);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	chan = tmp;

	return chan;

error_init:
	io_vcan_chan_free((void *)chan);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_vcan_chan_destroy(io_can_chan_t *chan)
{
	if (chan) {
		io_vcan_chan_fini(chan);
		io_vcan_chan_free((void *)chan);
	}
}

io_can_ctrl_t *
io_vcan_chan_get_ctrl(const io_can_chan_t *chan)
{
	const struct io_vcan_chan *vcan = io_vcan_chan_from_chan(chan);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&vcan->mtx);
#endif
	io_can_ctrl_t *ctrl = vcan->ctrl;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&vcan->mtx);
#endif

	return ctrl;
}

void
io_vcan_chan_open(io_can_chan_t *chan, io_can_ctrl_t *ctrl)
{
	io_vcan_chan_close(chan);
	if (ctrl)
		io_vcan_ctrl_insert(ctrl, chan);
}

int
io_vcan_chan_is_open(const io_can_chan_t *chan)
{
	return io_vcan_chan_get_ctrl(chan) != NULL;
}

void
io_vcan_chan_close(io_can_chan_t *chan)
{
	io_can_ctrl_t *ctrl = io_vcan_chan_get_ctrl(chan);
	if (ctrl)
		io_vcan_ctrl_remove(ctrl, chan);
}

static int
io_vcan_ctrl_stop(io_can_ctrl_t *ctrl)
{
	struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	io_vcan_ctrl_do_stop(vcan);
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif

	return 0;
}

static int
io_vcan_ctrl_stopped(const io_can_ctrl_t *ctrl)
{
	const struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&vcan->mtx);
#endif
	int stopped = vcan->stopped;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&vcan->mtx);
#endif

	return stopped;
}

static int
io_vcan_ctrl_restart(io_can_ctrl_t *ctrl)
{
	struct io_vcan_ctrl *vcan_ctrl = io_vcan_ctrl_from_ctrl(ctrl);

#if !LELY_NO_THREADS
	mtx_lock(&vcan_ctrl->mtx);
#endif
	int stopped = vcan_ctrl->stopped;
	if (stopped) {
		vcan_ctrl->stopped = 0;
		vcan_ctrl->state = CAN_STATE_ACTIVE;
		// Update the state of each channel.
		sllist_foreach (&vcan_ctrl->list, node) {
			struct io_vcan_chan *vcan_chan = structof(
					node, struct io_vcan_chan, node);
#if !LELY_NO_THREADS
			mtx_lock(&vcan_chan->mtx);
#endif
			vcan_chan->stopped = 0;
#if !LELY_NO_THREADS
			mtx_unlock(&vcan_chan->mtx);
#endif
		}
	}
#if !LELY_NO_THREADS
	mtx_unlock(&vcan_ctrl->mtx);
#endif

	// Send an error frame if the CAN bus was restarted. We ignore write
	// errors, since the user does not expect this operation to block.
	if (stopped && (vcan_ctrl->flags & IO_CAN_BUS_FLAG_ERR)) {
		struct can_err err = { .state = CAN_STATE_ACTIVE };
		int errsv = get_errc();
		if (io_vcan_ctrl_write_err(ctrl, &err, 0) == -1)
			set_errc(errsv);
	}

	return 0;
}

static int
io_vcan_ctrl_get_bitrate(const io_can_ctrl_t *ctrl, int *pnominal, int *pdata)
{
	const struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&vcan->mtx);
#endif
	if (pnominal)
		*pnominal = vcan->nominal;
	if (pdata)
		*pdata = vcan->data;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&vcan->mtx);
#endif

	return 0;
}

static int
io_vcan_ctrl_set_bitrate(io_can_ctrl_t *ctrl, int nominal, int data)
{
	struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	io_vcan_ctrl_do_stop(vcan);
	vcan->nominal = nominal;
#if !LELY_NO_CANFD
	if (vcan->flags & IO_CAN_BUS_FLAG_BRS)
		vcan->data = data;
#else
	(void)data;
#endif
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif

	return 0;
}

static int
io_vcan_ctrl_get_state(const io_can_ctrl_t *ctrl)
{
	const struct io_vcan_ctrl *vcan = io_vcan_ctrl_from_ctrl(ctrl);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&vcan->mtx);
#endif
	int state = vcan->state;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&vcan->mtx);
#endif

	return state;
}

static inline struct io_vcan_ctrl *
io_vcan_ctrl_from_ctrl(const io_can_ctrl_t *ctrl)
{
	assert(ctrl);

	return structof(ctrl, struct io_vcan_ctrl, ctrl_vptr);
}

static void
io_vcan_ctrl_insert(io_can_ctrl_t *ctrl, io_can_chan_t *chan)
{
	struct io_vcan_ctrl *vcan_ctrl = io_vcan_ctrl_from_ctrl(ctrl);
	struct io_vcan_chan *vcan_chan = io_vcan_chan_from_chan(chan);

#if !LELY_NO_THREADS
	mtx_lock(&vcan_ctrl->mtx);
#endif
	sllist_push_back(&vcan_ctrl->list, &vcan_chan->node);
#if !LELY_NO_THREADS
	mtx_lock(&vcan_chan->mtx);
#endif
	vcan_chan->ctrl = ctrl;
	vcan_chan->stopped = vcan_ctrl->stopped;
#if !LELY_NO_THREADS
	mtx_unlock(&vcan_chan->mtx);
#endif
#if !LELY_NO_THREADS
	mtx_unlock(&vcan_ctrl->mtx);
#endif
}

static void
io_vcan_ctrl_remove(io_can_ctrl_t *ctrl, io_can_chan_t *chan)
{
	struct io_vcan_ctrl *vcan_ctrl = io_vcan_ctrl_from_ctrl(ctrl);
	struct io_vcan_chan *vcan_chan = io_vcan_chan_from_chan(chan);

	struct sllist read_queue, write_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);

#if !LELY_NO_THREADS
	mtx_lock(&vcan_ctrl->mtx);
#endif
	if (sllist_remove(&vcan_ctrl->list, &vcan_chan->node)) {
		// Abort any wait operation started by io_vcan_ctrl_write().
		spscring_p_abort_wait(&vcan_chan->rxring);
#if !LELY_NO_THREADS
		mtx_lock(&vcan_chan->mtx);
#endif
		vcan_chan->ctrl = NULL;
		// Cancel all pending operations.
		sllist_append(&read_queue, &vcan_chan->read_queue);
		sllist_append(&write_queue, &vcan_chan->write_queue);
		vcan_chan->current_write = NULL;
#if !LELY_NO_THREADS
		mtx_unlock(&vcan_chan->mtx);
#endif
	}
#if !LELY_NO_THREADS
	mtx_unlock(&vcan_ctrl->mtx);
#endif

	io_can_chan_read_queue_post(&read_queue, -1, errnum2c(ERRNUM_CANCELED));
	io_can_chan_write_queue_post(&write_queue, errnum2c(ERRNUM_CANCELED));
}

static void
io_vcan_ctrl_signal(struct spscring *ring, void *arg)
{
	(void)ring;
	struct io_vcan_ctrl *vcan = arg;
	assert(vcan);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	// Post the write tasks of all channels, if necessary, to process any
	// pending write operations.
	sllist_foreach (&vcan->list, node) {
		struct io_vcan_chan *chan =
				structof(node, struct io_vcan_chan, node);
#if !LELY_NO_THREADS
		mtx_lock(&chan->mtx);
#endif
		if (!chan->write_posted && !sllist_empty(&chan->write_queue)) {
			chan->write_posted = 1;
			ev_exec_post(chan->write_task.exec, &chan->write_task);
		}
#if !LELY_NO_THREADS
		mtx_unlock(&chan->mtx);
#endif
	}
#if !LELY_NO_THREADS
	// Wake up all threads waiting for this signal.
	cnd_broadcast(&vcan->cond);
	mtx_unlock(&vcan->mtx);
#endif
}

static int
io_vcan_ctrl_write(io_can_ctrl_t *ctrl, io_can_chan_t *chan,
		const struct can_msg *msg, const struct can_err *err,
		int timeout)
{
	struct io_vcan_ctrl *vcan_ctrl = io_vcan_ctrl_from_ctrl(ctrl);
	assert(!msg || !err);

	if (!msg && !err)
		return 0;

	struct timespec ts = { 0, 0 };
	// Compute the absolute timeout for cnd_timedwait().
#if !LELY_NO_THREADS
	if (timeout > 0) {
#if LELY_NO_TIMEOUT
		timeout = 0;
#else
		if (!timespec_get(&ts, TIME_UTC))
			return -1;
		timespec_add_msec(&ts, timeout);
#endif
	}
#endif

#if !LELY_NO_CANFD
	// Check if CAN FD frames are supported.
	if (msg) {
		int flags = 0;
		if (msg->flags & CAN_FLAG_FDF)
			flags |= IO_CAN_BUS_FLAG_FDF;
		if (msg->flags & CAN_FLAG_BRS)
			flags |= IO_CAN_BUS_FLAG_BRS;
		if ((flags & vcan_ctrl->flags) != flags) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
	}
#endif
	// Check if error frames are supported.
	if (err && !(vcan_ctrl->flags & IO_CAN_BUS_FLAG_ERR)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

#if !LELY_NO_THREADS
	mtx_lock(&vcan_ctrl->mtx);
#endif

	// Return the same error as SocketCAN when the controller is stopped.
	if (vcan_ctrl->stopped) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan_ctrl->mtx);
#endif
		set_errnum(ERRNUM_NETDOWN);
		return -1;
	}

	// Check if all registered channels have a slot available in their
	// receive queue.
	for (;;) {
		struct io_vcan_chan *vcan_chan = NULL;
		int wouldblock = 0;
		sllist_foreach (&vcan_ctrl->list, node) {
			vcan_chan = structof(node, struct io_vcan_chan, node);
			// Skip the sending channel.
			if (chan && chan == &vcan_chan->chan_vptr)
				continue;
			// Try to allocate a single slot.
			size_t n = 1;
			spscring_p_alloc(&vcan_chan->rxring, &n);
			if (!n) {
				wouldblock = 1;
				break;
			}
		}
		// If all channels are ready, continue below with writing the
		// frame.
		if (!wouldblock)
			break;
		// Always submit a wait operation, even when timeout is 0, so
		// pending write operations will be signaled once the blocked
		// channel becomes ready.
		// clang-format off
		if (!spscring_p_submit_wait(&vcan_chan->rxring, 1,
				&io_vcan_ctrl_signal, vcan_ctrl))
			// clang-format on
			// If the wait condition was already sastisfied, try
			// again.
			continue;
		// Wait for the blocked channel to signal it is ready, or time
		// out if that takes too long.
		if (!timeout) {
#if !LELY_NO_THREADS
			mtx_unlock(&vcan_ctrl->mtx);
#endif
			set_errnum(ERRNUM_AGAIN);
			return -1;
		}
#if !LELY_NO_THREADS
		int result;
#if !LELY_NO_TIMEOUT
		if (timeout > 0)
			result = cnd_timedwait(
					&vcan_ctrl->cond, &vcan_ctrl->mtx, &ts);
		else
#endif
			result = cnd_wait(&vcan_ctrl->cond, &vcan_ctrl->mtx);
		if (result != thrd_success) {
			mtx_unlock(&vcan_ctrl->mtx);
#if !LELY_NO_TIMEOUT
			if (result == thrd_timedout)
				set_errnum(ERRNUM_AGAIN);
#endif
			return -1;
		}
#endif
	}

	// Obtain a timestamp for the CAN frame. We obtain it here, after
	// waiting above, so the timestamp reflects the actual time the frame
	// was put on the bus. This guarantees that all frames are ordered in
	// time.
	if (io_clock_gettime(vcan_ctrl->clock, &ts) == -1) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan_ctrl->mtx);
#endif
		return -1;
	}

	// Put the frame in the receive queue of every channel.
	sllist_foreach (&vcan_ctrl->list, node) {
		struct io_vcan_chan *vcan_chan =
				structof(node, struct io_vcan_chan, node);
		// Skip the sending channel.
		if (chan && chan == &vcan_chan->chan_vptr)
			continue;

		size_t n = 1;
		size_t i = spscring_p_alloc(&vcan_chan->rxring, &n);
		// We did not release the mutex after checking that all channels
		// are ready, so this cannot fail.
		assert(n == 1);
		if (msg)
			vcan_chan->rxbuf[i] = (struct io_vcan_frame){
				.is_err = 0, .u.msg = *msg, .ts = ts
			};
		else
			vcan_chan->rxbuf[i] = (struct io_vcan_frame){
				.is_err = 1, .u.err = *err, .ts = ts
			};
		spscring_p_commit(&vcan_chan->rxring, n);
	}

#if !LELY_NO_THREADS
	mtx_unlock(&vcan_ctrl->mtx);
#endif

	return 0;
}

static void
io_vcan_ctrl_do_stop(struct io_vcan_ctrl *vcan_ctrl)
{
	assert(vcan_ctrl);

	vcan_ctrl->stopped = 1;
	vcan_ctrl->state = CAN_STATE_STOPPED;

	struct sllist read_queue, write_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);

	sllist_foreach (&vcan_ctrl->list, node) {
		struct io_vcan_chan *vcan_chan =
				structof(node, struct io_vcan_chan, node);
#if !LELY_NO_THREADS
		mtx_lock(&vcan_chan->mtx);
#endif
		vcan_chan->stopped = 1;
		// Cancel all pending operations.
		sllist_append(&read_queue, &vcan_chan->read_queue);
		sllist_append(&write_queue, &vcan_chan->write_queue);
		vcan_chan->current_write = NULL;
#if !LELY_NO_THREADS
		mtx_unlock(&vcan_chan->mtx);
#endif
	}

	io_can_chan_read_queue_post(&read_queue, -1, errnum2c(ERRNUM_NETDOWN));
	io_can_chan_write_queue_post(&write_queue, errnum2c(ERRNUM_NETDOWN));
}

static io_ctx_t *
io_vcan_chan_dev_get_ctx(const io_dev_t *dev)
{
	const struct io_vcan_chan *vcan = io_vcan_chan_from_dev(dev);

	return vcan->ctx;
}

static ev_exec_t *
io_vcan_chan_dev_get_exec(const io_dev_t *dev)
{
	const struct io_vcan_chan *vcan = io_vcan_chan_from_dev(dev);

	return vcan->exec;
}

static size_t
io_vcan_chan_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_dev(dev);

	size_t n = 0;

	struct sllist read_queue, write_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	io_vcan_chan_do_pop(vcan, &read_queue, &write_queue, task);
	// Mark the ongoing write operation as canceled, if necessary.
	if (vcan->current_write && (!task || task == vcan->current_write)) {
		vcan->current_write = NULL;
		n++;
	}
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif

	size_t nread = io_can_chan_read_queue_post(
			&read_queue, -1, errnum2c(ERRNUM_CANCELED));
	n = n < SIZE_MAX - nread ? n + nread : SIZE_MAX;
	size_t nwrite = io_can_chan_write_queue_post(
			&write_queue, errnum2c(ERRNUM_CANCELED));
	n = n < SIZE_MAX - nwrite ? n + nwrite : SIZE_MAX;

	return n;
}

static size_t
io_vcan_chan_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	io_vcan_chan_do_pop(vcan, &queue, &queue, task);
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif

	return ev_task_queue_abort(&queue);
}

static io_dev_t *
io_vcan_chan_get_dev(const io_can_chan_t *chan)
{
	const struct io_vcan_chan *vcan = io_vcan_chan_from_chan(chan);

	return &vcan->dev_vptr;
}

static int
io_vcan_chan_get_flags(const io_can_chan_t *chan)
{
	const struct io_vcan_chan *vcan_chan = io_vcan_chan_from_chan(chan);
	const struct io_vcan_ctrl *vcan_ctrl =
			io_vcan_ctrl_from_ctrl(vcan_chan->ctrl);

	return vcan_ctrl->flags;
}

static int
io_vcan_chan_read(io_can_chan_t *chan, struct can_msg *msg, struct can_err *err,
		struct timespec *tp, int timeout)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_chan(chan);

#if !LELY_NO_THREADS
	// Compute the absolute timeout for cnd_timedwait().
	struct timespec ts = { 0, 0 };
	if (timeout > 0) {
#if LELY_NO_TIMEOUT
		timeout = 0;
		(void)ts;
#else
		if (!timespec_get(&ts, TIME_UTC))
			return -1;
		timespec_add_msec(&ts, timeout);
#endif
	}
#endif

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	size_t i = 0;
	for (;;) {
		// Check if a frame is available in the receive queue.
		size_t n = 1;
		i = spscring_c_alloc(&vcan->rxring, &n);
		if (n)
			break;
		// Return the same error as SocketCAN when the channel is
		// closed.
		if (!vcan->ctrl) {
#if !LELY_NO_THREADS
			mtx_unlock(&vcan->mtx);
#endif
			set_errnum(ERRNUM_BADF);
			return -1;
		}
		// Return the same error as SocketCAN when the controller is
		// stopped.
		if (vcan->stopped) {
#if !LELY_NO_THREADS
			mtx_unlock(&vcan->mtx);
#endif
			set_errnum(ERRNUM_NETDOWN);
			return -1;
		}
		// Always submit a wait operation, even when timeout is 0, so
		// pending read operations will be signaled once a frame is
		// available.
		// clang-format off
		if (!spscring_c_submit_wait(&vcan->rxring, 1,
				&io_vcan_chan_signal, vcan))
			// clang-format on
			// If the wait condition was already satisfied, try
			// again.
			continue;
		// Wait for the buffer to signal that a frame is available, or
		// time out if that takes too long.
		if (!timeout) {
#if !LELY_NO_THREADS
			mtx_unlock(&vcan->mtx);
#endif
			set_errnum(ERRNUM_AGAIN);
			return -1;
		}
#if !LELY_NO_THREADS
		int result;
#if !LELY_NO_TIMEOUT
		if (timeout > 0)
			result = cnd_timedwait(&vcan->cond, &vcan->mtx, &ts);
		else
#endif
			result = cnd_wait(&vcan->cond, &vcan->mtx);
		if (result != thrd_success) {
			mtx_unlock(&vcan->mtx);
#if !LELY_NO_TIMEOUT
			if (result == thrd_timedout)
				set_errnum(ERRNUM_AGAIN);
#endif
			return -1;
		}
#endif
	}
	// Copy the frame from the buffer.
	struct io_vcan_frame *frame = &vcan->rxbuf[i];
	int is_err = frame->is_err;
	if (!is_err && msg)
		*msg = frame->u.msg;
	else if (is_err && err)
		*err = frame->u.err;
	if (tp)
		*tp = frame->ts;
	spscring_c_commit(&vcan->rxring, 1);
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif

	return !is_err;
}

static void
io_vcan_chan_submit_read(io_can_chan_t *chan, struct io_can_chan_read *read)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_chan(chan);
	assert(read);
	struct ev_task *task = &read->task;

	if (!task->exec)
		task->exec = vcan->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	if (vcan->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		io_can_chan_read_post(read, -1, errnum2c(ERRNUM_CANCELED));
	} else if (!vcan->ctrl) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		io_can_chan_read_post(read, -1, errnum2c(ERRNUM_BADF));
	} else if (vcan->stopped) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		io_can_chan_read_post(read, -1, errnum2c(ERRNUM_NETDOWN));
	} else {
		int post_read = !vcan->read_posted
				&& sllist_empty(&vcan->read_queue);
		sllist_push_back(&vcan->read_queue, &task->_node);
		if (post_read)
			vcan->read_posted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		// cppcheck-suppress duplicateCondition
		if (post_read)
			ev_exec_post(vcan->read_task.exec, &vcan->read_task);
	}
}

static int
io_vcan_chan_write(io_can_chan_t *chan, const struct can_msg *msg, int timeout)
{
	io_can_ctrl_t *ctrl = io_vcan_chan_get_ctrl(chan);
	if (!ctrl) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}
	return io_vcan_ctrl_write(ctrl, chan, msg, NULL, timeout);
}

static void
io_vcan_chan_submit_write(io_can_chan_t *chan, struct io_can_chan_write *write)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_chan(chan);
	assert(write);
	struct ev_task *task = &write->task;

	if (!task->exec)
		task->exec = vcan->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	if (vcan->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		io_can_chan_write_post(write, errnum2c(ERRNUM_CANCELED));
	} else if (!vcan->ctrl) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		io_can_chan_write_post(write, errnum2c(ERRNUM_BADF));
	} else if (vcan->stopped) {
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		io_can_chan_write_post(write, errnum2c(ERRNUM_NETDOWN));
	} else {
		int post_write = !vcan->write_posted
				&& sllist_empty(&vcan->write_queue);
		sllist_push_back(&vcan->write_queue, &task->_node);
		if (post_write)
			vcan->write_posted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		// cppcheck-suppress duplicateCondition
		if (post_write)
			ev_exec_post(vcan->write_task.exec, &vcan->write_task);
	}
}

static void
io_vcan_chan_svc_shutdown(struct io_svc *svc)
{
	struct io_vcan_chan *vcan = io_vcan_chan_from_svc(svc);
	io_dev_t *dev = &vcan->dev_vptr;

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	int shutdown = !vcan->shutdown;
	vcan->shutdown = 1;
	if (shutdown)
		// Try to abort io_vcan_chan_read_task_func() and
		// io_vcan_chan_write_task_func().
		io_vcan_chan_do_abort_tasks(vcan);
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif
	// cppcheck-suppress duplicateCondition
	if (shutdown)
		// Cancel all pending operations.
		io_vcan_chan_dev_cancel(dev, NULL);
}

static void
io_vcan_chan_read_task_func(struct ev_task *task)
{
	assert(task);
	struct io_vcan_chan *vcan =
			structof(task, struct io_vcan_chan, read_task);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	vcan->read_posted = 0;
	// Try to process all pending read operations at once.
	while ((task = ev_task_from_node(sllist_first(&vcan->read_queue)))) {
		// Check if a frame is available in the receive queue.
		size_t n = 1;
		size_t i = spscring_c_alloc(&vcan->rxring, &n);
		if (!n) {
			// If the queue is empty, register a wait operation and
			// return.
			// clang-format off
			if (!spscring_c_submit_wait(&vcan->rxring, 1,
					io_vcan_chan_signal, vcan))
				// clang-format on
				continue;
			break;
		}
		sllist_pop_front(&vcan->read_queue);
		struct io_vcan_frame *frame = &vcan->rxbuf[i];

		struct io_can_chan_read *read =
				io_can_chan_read_from_task(task);
		if (!frame->is_err && read->msg)
			*read->msg = frame->u.msg;
		if (frame->is_err && read->err)
			*read->err = frame->u.err;
		if (read->tp)
			*read->tp = frame->ts;
		io_can_chan_read_post(read, !frame->is_err, 0);

		spscring_c_commit(&vcan->rxring, 1);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif
}

static void
io_vcan_chan_write_task_func(struct ev_task *task)
{
	assert(task);
	struct io_vcan_chan *vcan =
			structof(task, struct io_vcan_chan, write_task);

	int errsv = get_errc();

	struct io_can_chan_write *write = NULL;
	int wouldblock = 0;

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	vcan->write_posted = 0;
	// Try to process all pending write operations at once.
	while ((task = vcan->current_write = ev_task_from_node(
				sllist_pop_front(&vcan->write_queue)))) {
		write = io_can_chan_write_from_task(task);
#if !LELY_NO_THREADS
		mtx_unlock(&vcan->mtx);
#endif
		// Perform a non-blocking write.
		int result = io_vcan_chan_write(
				&vcan->chan_vptr, write->msg, 0);
		int errc = !result ? 0 : get_errc();
		wouldblock = errc == errnum2c(ERRNUM_AGAIN)
				|| errc == errnum2c(ERRNUM_WOULDBLOCK);
		if (!wouldblock)
			// The operation succeeded or failed immediately.
			io_can_chan_write_post(write, errc);
#if !LELY_NO_THREADS
		mtx_lock(&vcan->mtx);
#endif
		if (task == vcan->current_write) {
			// Put the write operation back on the queue if it would
			// block, unless it was canceled.
			if (wouldblock) {
				sllist_push_front(&vcan->write_queue,
						&task->_node);
				task = NULL;
			}
			vcan->current_write = NULL;
		}
		assert(!vcan->current_write);
		// Stop if the operation would block, or this task has been
		// reposted.
		if (wouldblock || vcan->write_posted)
			break;
	}
	int post_write = !vcan->write_posted
			&& !sllist_empty(&vcan->write_queue) && !vcan->shutdown;
	if (post_write)
		vcan->write_posted = 1;
#if !LELY_NO_THREADS
	mtx_unlock(&vcan->mtx);
#endif

	if (task && wouldblock)
		// The operation would block but was canceled before it could be
		// requeued.
		io_can_chan_write_post(io_can_chan_write_from_task(task),
				errnum2c(ERRNUM_CANCELED));

	if (post_write)
		ev_exec_post(vcan->write_task.exec, &vcan->write_task);

	set_errc(errsv);
}

static inline struct io_vcan_chan *
io_vcan_chan_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, struct io_vcan_chan, dev_vptr);
}

static inline struct io_vcan_chan *
io_vcan_chan_from_chan(const io_can_chan_t *chan)
{
	assert(chan);

	return structof(chan, struct io_vcan_chan, chan_vptr);
}

static inline struct io_vcan_chan *
io_vcan_chan_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, struct io_vcan_chan, svc);
}

static void
io_vcan_chan_signal(struct spscring *ring, void *arg)
{
	(void)ring;
	struct io_vcan_chan *vcan = arg;
	assert(vcan);

#if !LELY_NO_THREADS
	mtx_lock(&vcan->mtx);
#endif
	int post_read = !vcan->read_posted && !sllist_empty(&vcan->read_queue);
	if (post_read)
		vcan->read_posted = 1;
#if !LELY_NO_THREADS
	cnd_broadcast(&vcan->cond);
	mtx_unlock(&vcan->mtx);
#endif
	// cppcheck-suppress duplicateCondition
	if (post_read)
		ev_exec_post(vcan->read_task.exec, &vcan->read_task);
}

static void
io_vcan_chan_do_pop(struct io_vcan_chan *vcan, struct sllist *read_queue,
		struct sllist *write_queue, struct ev_task *task)
{
	assert(vcan);
	assert(read_queue);
	assert(write_queue);

	if (!task) {
		sllist_append(read_queue, &vcan->read_queue);
		sllist_append(write_queue, &vcan->write_queue);
	} else if (sllist_remove(&vcan->read_queue, &task->_node)) {
		sllist_push_back(read_queue, &task->_node);
	} else if (sllist_remove(&vcan->write_queue, &task->_node)) {
		sllist_push_back(write_queue, &task->_node);
	}
}

static size_t
io_vcan_chan_do_abort_tasks(struct io_vcan_chan *vcan)
{
	assert(vcan);

	size_t n = 0;

	// Try to abort io_vcan_chan_read_task_func().
	// clang-format off
	if (vcan->read_posted && ev_exec_abort(vcan->read_task.exec,
			&vcan->read_task)) {
		// clang-format on
		vcan->read_posted = 0;
		n++;
	}

	// Try to abort io_vcan_chan_write_task_func().
	// clang-format off
	if (vcan->write_posted && ev_exec_abort(vcan->write_task.exec,
			&vcan->write_task)) {
		// clang-format on
		vcan->write_posted = 0;
		n++;
	}

	return n;
}

#endif // !LELY_NO_MALLOC
