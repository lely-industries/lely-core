/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * CAN network interface.
 *
 * @see lely/io2/can_net.h
 *
 * @copyright 2018-2029 Lely Industries N.V.
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

#include "io2.h"

#if !LELY_NO_MALLOC

#include <lely/can/net.h>
#include <lely/io2/can_net.h>
#include <lely/io2/ctx.h>
#include <lely/libc/stdlib.h>
#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/util/diag.h>
#include <lely/util/spscring.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>

#ifndef LELY_IO_CAN_NET_TXLEN
/**
 * The default length (in number of CAN frames) of the user-space transmit queue
 * of a CAN network interface.
 */
#define LELY_IO_CAN_NET_TXLEN 1000
#endif

#ifndef LELY_IO_CAN_NET_TXTIMEO
/**
 * The default timeout (in milliseconds) of a CAN network interface when waiting
 * for a CAN frame write confirmation.
 */
#define LELY_IO_CAN_NET_TXTIMEO 100
#endif

static void io_can_net_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_can_net_svc_vtbl = {
	NULL,
	&io_can_net_svc_shutdown
};
// clang-format on

/// The implementation of a CAN network interface.
struct io_can_net {
	/// The I/O service representing the channel.
	struct io_svc svc;
	/// A pointer to the I/O context with which the channel is registered.
	io_ctx_t *ctx;
	/// A pointer to the executor ...
	ev_exec_t *exec;
	/// A pointer to the timer used for CAN network events.
	io_timer_t *timer;
	/// A pointer to the timer queue used to schedule wait operations.
	io_tqueue_t *tq;
	/// The operation used to wait for the next CANopen event.
	struct io_tqueue_wait wait_next;
	/// The operation used to wait for a CAN frame write confirmation.
	struct io_tqueue_wait wait_confirm;
	/**
	 * The timeout (in milliseconds) when waiting for a CAN frame write
	 * confirmation.
	 */
	int txtimeo;
	/// A pointer to a CAN channel.
	io_can_chan_t *chan;
	/// The CAN frame being read.
	struct can_msg read_msg;
	/// The CAN error frame being read.
	struct can_err read_err;
	/// The operation used to read CAN frames.
	struct io_can_chan_read read;
	/// The error code of the last read operation.
	int read_errc;
	/// The number of errors since the last successful read operation.
	size_t read_errcnt;
	/// The current state of the CAN bus.
	int state;
	/// The CAN frame being written.
	struct can_msg write_msg;
	/// The operation used to write CAN frames.
	struct io_can_chan_write write;
	/// The error code of the last write operation.
	int write_errc;
	/// The number of errors since the last successful write operation.
	size_t write_errcnt;
	/// The ring buffer used to control the transmit queue.
	struct spscring tx_ring;
	/// The transmit queue.
	struct can_msg *tx_buf;
	/// The number of frames dropped due to the transmit queue being full.
	size_t tx_errcnt;
#if !LELY_NO_THREADS
	/**
	 * The mutex protecting the callbacks, flags and internal CAN network
	 * interface.
	 */
	mtx_t mtx;
#endif
	/**
	 * A pointer to the function invoked when a new CAN frame read error
	 * occurs, or when a read operation completes successfully after one or
	 * more errors.
	 */
	io_can_net_on_error_func_t *on_read_error_func;
	/// The user-specified argument for #on_read_error_func.
	void *on_read_error_arg;
	/**
	 * A pointer to the function invoked when a CAN frame is dropped because
	 * the transmit queue is full, or when a frame is successfully queued
	 * after one or more errors.
	 */
	io_can_net_on_error_func_t *on_queue_error_func;
	/// The user-specified argument for #on_queue_error_func.
	void *on_queue_error_arg;
	/**
	 * A pointer to the function invoked when a new CAN frame write error
	 * occurs, or when a write operation completes successfully after one or
	 * more errors.
	 */
	io_can_net_on_error_func_t *on_write_error_func;
	/// The user-specified argument for #on_write_error_func.
	void *on_write_error_arg;
	/**
	 * A pointer to the function to be invoked when a CAN bus state change
	 * is detected.
	 */
	io_can_net_on_can_state_func_t *on_can_state_func;
	/// The user-specified argument for #on_state_func.
	void *on_can_state_arg;
	/**
	 * A pointer to the function to be invoked when an error is detected on
	 * the CAN bus.
	 */
	io_can_net_on_can_error_func_t *on_can_error_func;
	/// The user-specified argument for #on_error_func.
	void *on_can_error_arg;
	/// A flag indicating wheter the CAN network interface has been started.
	unsigned started : 1;
	/// A flag indicating whether the I/O service has been shut down.
	unsigned shutdown : 1;
	/// A flag indicating whether #wait_next has been submitted to #tq.
	unsigned wait_next_submitted : 1;
	/// A flag indicating whether #wait_confirm has been submitted to #tq.
	unsigned wait_confirm_submitted : 1;
	/// A flag indicating whether #read has been submitted to #chan.
	unsigned read_submitted : 1;
	/// A flag indicating whether #write has been submitted to #chan.
	unsigned write_submitted : 1;
	/// A pointer to the internal CAN network interface.
	can_net_t *net;
	/// The time at which the next CAN timer will trigger.
	struct timespec next;
};

static void io_can_net_wait_next_func(struct ev_task *task);
static void io_can_net_wait_confirm_func(struct ev_task *task);
static void io_can_net_read_func(struct ev_task *task);
static void io_can_net_write_func(struct ev_task *task);

static int io_can_net_next_func(const struct timespec *tp, void *data);
static int io_can_net_send_func(const struct can_msg *msg, void *data);

static void io_can_net_c_wait_func(struct spscring *ring, void *arg);

static inline io_can_net_t *io_can_net_from_svc(const struct io_svc *svc);

int io_can_net_do_wait(io_can_net_t *net);
void io_can_net_do_write(io_can_net_t *net);

size_t io_can_net_do_abort_tasks(io_can_net_t *net);

static void default_on_read_error_func(int errc, size_t errcnt, void *arg);
static void default_on_queue_error_func(int errc, size_t errcnt, void *arg);
static void default_on_write_error_func(int errc, size_t errcnt, void *arg);
static void default_on_can_state_func(int new_state, int old_state, void *arg);
static void default_on_can_error_func(int error, void *arg);

void *
io_can_net_alloc(void)
{
	void *ptr = aligned_alloc(_Alignof(io_can_net_t), sizeof(io_can_net_t));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
io_can_net_free(void *ptr)
{
	aligned_free(ptr);
}

io_can_net_t *
io_can_net_init(io_can_net_t *net, ev_exec_t *exec, io_timer_t *timer,
		io_can_chan_t *chan, size_t txlen, int txtimeo)
{
	assert(net);
	assert(timer);
	assert(chan);

	int errc = 0;

	if (!exec)
		exec = io_can_chan_get_exec(chan);

	if (!txlen)
		txlen = LELY_IO_CAN_NET_TXLEN;

	if (!txtimeo)
		txtimeo = LELY_IO_CAN_NET_TXTIMEO;

	net->svc = (struct io_svc)IO_SVC_INIT(&io_can_net_svc_vtbl);
	net->ctx = io_can_chan_get_ctx(chan);
	assert(net->ctx);

	net->exec = exec;

	net->timer = timer;
	if (!(net->tq = io_tqueue_create(net->timer, NULL))) {
		errc = get_errc();
		goto error_create_tq;
	}
	net->wait_next = (struct io_tqueue_wait)IO_TQUEUE_WAIT_INIT(
			0, 0, NULL, &io_can_net_wait_next_func);
	net->wait_confirm = (struct io_tqueue_wait)IO_TQUEUE_WAIT_INIT(
			0, 0, NULL, &io_can_net_wait_confirm_func);
	net->txtimeo = txtimeo;

	net->chan = chan;

	net->read_msg = (struct can_msg)CAN_MSG_INIT;
	net->read_err = (struct can_err)CAN_ERR_INIT;
	net->read = (struct io_can_chan_read)IO_CAN_CHAN_READ_INIT(
			&net->read_msg, &net->read_err, NULL, NULL,
			&io_can_net_read_func);
	net->read_errc = 0;
	net->read_errcnt = 0;

	net->state = CAN_STATE_ACTIVE;

	net->write_msg = (struct can_msg)CAN_MSG_INIT;
	net->write = (struct io_can_chan_write)IO_CAN_CHAN_WRITE_INIT(
			&net->write_msg, NULL, &io_can_net_write_func);
	net->write_errc = 0;
	net->write_errcnt = 0;

	spscring_init(&net->tx_ring, txlen);
	net->tx_buf = calloc(txlen, sizeof(struct can_msg));
	if (!net->tx_buf) {
		errc = get_errc();
		goto error_alloc_tx_buf;
	}
	net->tx_errcnt = 0;

#if !LELY_NO_THREADS
	if (mtx_init(&net->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}
#endif

	net->on_read_error_func = &default_on_read_error_func;
	net->on_read_error_arg = NULL;
	net->on_queue_error_func = &default_on_queue_error_func;
	net->on_queue_error_arg = NULL;
	net->on_write_error_func = &default_on_write_error_func;
	net->on_write_error_arg = NULL;
	net->on_can_state_func = &default_on_can_state_func;
	net->on_can_state_arg = NULL;
	net->on_can_error_func = &default_on_can_error_func;
	net->on_can_error_arg = NULL;

	net->started = 0;
	net->shutdown = 0;
	net->wait_next_submitted = 0;
	net->wait_confirm_submitted = 0;
	net->read_submitted = 0;
	net->write_submitted = 0;

	if (!(net->net = can_net_create())) {
		errc = get_errc();
		goto error_create_net;
	}
	net->next = (struct timespec){ 0, 0 };

	// Initialize the CAN network clock with the current time.
	if (io_can_net_set_time(net) == -1) {
		errc = get_errc();
		goto error_set_time;
	}

	// Register the function to be invoked when the time at which the next
	// timer triggers is updated.
	can_net_set_next_func(net->net, &io_can_net_next_func, net);
	// Register the function to be invoked when a CAN frame needs to be
	// sent.
	can_net_set_send_func(net->net, &io_can_net_send_func, net);

	io_ctx_insert(net->ctx, &net->svc);

	return net;

error_set_time:
	can_net_destroy(net->net);
error_create_net:
#if !LELY_NO_THREADS
	mtx_destroy(&net->mtx);
error_init_mtx:
#endif
	free(net->tx_buf);
error_alloc_tx_buf:
	io_tqueue_destroy(net->tq);
error_create_tq:
	set_errc(errc);
	return NULL;
}

void
io_can_net_fini(io_can_net_t *net)
{
	assert(net);

	io_ctx_remove(net->ctx, &net->svc);
	// Cancel all pending operations.
	io_can_net_svc_shutdown(&net->svc);

#if !LELY_NO_THREADS
	int warning = 0;
	mtx_lock(&net->mtx);
	// If necessary, busy-wait until all submitted operations complete.
	while (net->wait_next_submitted || net->wait_confirm_submitted
			|| net->read_submitted || net->write_submitted) {
		if (io_can_net_do_abort_tasks(net))
			continue;
		mtx_unlock(&net->mtx);
		if (!warning) {
			warning = 1;
			diag(DIAG_WARNING, 0,
					"io_can_net_fini() invoked with pending operations");
		}
		thrd_yield();
		mtx_lock(&net->mtx);
	}
	mtx_unlock(&net->mtx);
#endif

	can_net_destroy(net->net);
#if !LELY_NO_THREADS
	mtx_destroy(&net->mtx);
#endif
	free(net->tx_buf);
	io_tqueue_destroy(net->tq);
}

io_can_net_t *
io_can_net_create(ev_exec_t *exec, io_timer_t *timer, io_can_chan_t *chan,
		size_t txlen, int txtimeo)
{
	int errc = 0;

	io_can_net_t *net = io_can_net_alloc();
	if (!net) {
		errc = get_errc();
		goto error_alloc;
	}

	io_can_net_t *tmp =
			io_can_net_init(net, exec, timer, chan, txlen, txtimeo);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	net = tmp;

	return net;

error_init:
	io_can_net_free(net);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_can_net_destroy(io_can_net_t *net)
{
	if (net) {
		io_can_net_fini(net);
		io_can_net_free(net);
	}
}

void
io_can_net_start(io_can_net_t *net)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	if (!net->started && !net->shutdown) {
		net->started = 1;

		// Start waiting for CAN frames to be put into the transmit
		// queue.
		io_can_net_do_wait(net);

		assert(!net->read_submitted);
		net->read_submitted = 1;
		// Start receiving CAN frames.
		io_can_chan_submit_read(net->chan, &net->read);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif
}

io_ctx_t *
io_can_net_get_ctx(const io_can_net_t *net)
{
	assert(net);

	return net->ctx;
}

ev_exec_t *
io_can_net_get_exec(const io_can_net_t *net)
{
	assert(net);

	return net->exec;
}

io_clock_t *
io_can_net_get_clock(const io_can_net_t *net)
{
	assert(net);

	return io_timer_get_clock(io_tqueue_get_timer(net->tq));
}

io_tqueue_t *
io_can_net_get_tqueue(const io_can_net_t *net)
{
	assert(net);

	return net->tq;
}

void
io_can_net_get_on_read_error_func(const io_can_net_t *net,
		io_can_net_on_error_func_t **pfunc, void **parg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&net->mtx);
#endif
	if (pfunc)
		*pfunc = net->on_read_error_func;
	if (parg)
		*parg = net->on_read_error_arg;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&net->mtx);
#endif
}

void
io_can_net_set_on_read_error_func(
		io_can_net_t *net, io_can_net_on_error_func_t *func, void *arg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	net->on_read_error_func = func ? func : &default_on_read_error_func;
	net->on_read_error_arg = func ? arg : NULL;
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif
}

void
io_can_net_get_on_queue_error_func(const io_can_net_t *net,
		io_can_net_on_error_func_t **pfunc, void **parg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&net->mtx);
#endif
	if (pfunc)
		*pfunc = net->on_queue_error_func;
	if (parg)
		*parg = net->on_queue_error_arg;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&net->mtx);
#endif
}

void
io_can_net_set_on_queue_error_func(
		io_can_net_t *net, io_can_net_on_error_func_t *func, void *arg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	net->on_queue_error_func = func ? func : &default_on_queue_error_func;
	net->on_queue_error_arg = func ? arg : NULL;
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif
}

void
io_can_net_get_on_write_error_func(const io_can_net_t *net,
		io_can_net_on_error_func_t **pfunc, void **parg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&net->mtx);
#endif
	if (pfunc)
		*pfunc = net->on_write_error_func;
	if (parg)
		*parg = net->on_write_error_arg;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&net->mtx);
#endif
}

void
io_can_net_set_on_write_error_func(
		io_can_net_t *net, io_can_net_on_error_func_t *func, void *arg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	net->on_write_error_func = func ? func : &default_on_write_error_func;
	net->on_write_error_arg = func ? arg : NULL;
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif
}

void
io_can_net_get_on_can_state_func(const io_can_net_t *net,
		io_can_net_on_can_state_func_t **pfunc, void **parg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&net->mtx);
#endif
	if (pfunc)
		*pfunc = net->on_can_state_func;
	if (parg)
		*parg = net->on_can_state_arg;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&net->mtx);
#endif
}

void
io_can_net_set_on_can_state_func(io_can_net_t *net,
		io_can_net_on_can_state_func_t *func, void *arg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	net->on_can_state_func = func ? func : &default_on_can_state_func;
	net->on_can_state_arg = func ? arg : NULL;
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif
}

void
io_can_net_get_on_can_error_func(const io_can_net_t *net,
		io_can_net_on_can_error_func_t **pfunc, void **parg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&net->mtx);
#endif
	if (pfunc)
		*pfunc = net->on_can_error_func;
	if (parg)
		*parg = net->on_can_error_arg;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&net->mtx);
#endif
}

void
io_can_net_set_on_can_error_func(io_can_net_t *net,
		io_can_net_on_can_error_func_t *func, void *arg)
{
	assert(net);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	net->on_can_error_func = func ? func : &default_on_can_error_func;
	net->on_can_error_arg = func ? arg : NULL;
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif
}

int
io_can_net_lock(io_can_net_t *net)
{
#if LELY_NO_THREADS
	(void)net;

	return 0;
#else
	assert(net);

	return mtx_lock(&net->mtx);
#endif
}

int
io_can_net_unlock(io_can_net_t *net)
{
#if LELY_NO_THREADS
	(void)net;

	return 0;
#else
	assert(net);

	return mtx_unlock(&net->mtx);
#endif
	assert(net);
}

can_net_t *
io_can_net_get_net(const io_can_net_t *net)
{
	assert(net);

	return net->net;
}

int
io_can_net_set_time(io_can_net_t *net)
{
	struct timespec now = { 0, 0 };
	if (io_clock_gettime(io_can_net_get_clock(net), &now) == -1)
		return -1;
	return can_net_set_time(io_can_net_get_net(net), &now);
}

static void
io_can_net_svc_shutdown(struct io_svc *svc)
{
	io_can_net_t *net = io_can_net_from_svc(svc);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	int shutdown = !net->shutdown;
	net->shutdown = 1;
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif

	if (shutdown)
		spscring_c_abort_wait(&net->tx_ring);
}

static void
io_can_net_wait_next_func(struct ev_task *task)
{
	assert(task);
	struct io_tqueue_wait *wait_next = io_tqueue_wait_from_task(task);
	io_can_net_t *net = structof(wait_next, io_can_net_t, wait_next);
	assert(net->wait_next_submitted);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif

	// Update the time of the CAN network interface.
	io_can_net_set_time(net);

	// Check if the next timeout is in the future and another wait operation
	// needs to be submitted.
	int submit_wait_next = 0;
	if (!net->shutdown) {
		struct timespec now = { 0, 0 };
		can_net_get_time(net->net, &now);
		if (timespec_cmp(&now, &net->next) < 0) {
			net->wait_next.value = net->next;
			submit_wait_next = 1;
		}
	}
	net->wait_next_submitted = submit_wait_next;

#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif

	if (submit_wait_next)
		io_tqueue_submit_wait(net->tq, &net->wait_next);
}

static void
io_can_net_wait_confirm_func(struct ev_task *task)
{
	assert(task);
	struct io_tqueue_wait *wait_confirm = io_tqueue_wait_from_task(task);
	io_can_net_t *net = structof(wait_confirm, io_can_net_t, wait_confirm);
	assert(net->txtimeo >= 0);
	assert(net->wait_confirm_submitted);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif
	net->wait_confirm_submitted = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif

	// No confirmation message was received; cancel the ongoing write
	// operation.
	io_can_chan_cancel_write(net->chan, &net->write);
}

static void
io_can_net_read_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_read *read = io_can_chan_read_from_task(task);
	io_can_net_t *net = structof(read, io_can_net_t, read);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif

	if (read->r.errc && errc2num(read->r.errc) != ERRNUM_CANCELED) {
		net->read_errcnt += net->read_errcnt < SIZE_MAX;
		if (read->r.errc != net->read_errc) {
			net->read_errc = read->r.errc;
			// Only invoke the callback for unique read errors.
			assert(net->on_read_error_func);
			net->on_read_error_func(net->read_errc, net->read_errc,
					net->on_read_error_arg);
		}
	} else if (!read->r.errc && net->read_errc) {
		assert(net->on_read_error_func);
		net->on_read_error_func(
				0, net->read_errc, net->on_read_error_arg);
		net->read_errc = 0;
		net->read_errcnt = 0;
	}

	if (read->r.result == 1) {
		// Update the internal clock before processing the incoming CAN
		// frame.
		io_can_net_set_time(net);
		can_net_recv(net->net, &net->read_msg);
	} else if (read->r.result == 0) {
		if (net->read_err.state != net->state) {
			int new_state = net->read_err.state;
			int old_state = net->state;
			net->state = net->read_err.state;

			if (old_state == CAN_STATE_BUSOFF)
				// Cancel the ongoing write operation if we just
				// recovered from bus off.
				io_can_chan_cancel_write(
						net->chan, &net->write);

			assert(net->on_can_state_func);
			net->on_can_state_func(new_state, old_state,
					net->on_can_state_arg);
		}

		if (net->read_err.error) {
			assert(net->on_can_error_func);
			net->on_can_error_func(net->read_err.error,
					net->on_can_error_arg);
		}
	}

	int submit_read = net->read_submitted = !net->shutdown;

#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif

	if (submit_read)
		io_can_chan_submit_read(net->chan, &net->read);
}

static void
io_can_net_write_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_write *write = io_can_chan_write_from_task(task);
	io_can_net_t *net = structof(write, io_can_net_t, write);

#if !LELY_NO_THREADS
	mtx_lock(&net->mtx);
#endif

	if (write->errc) {
		net->write_errcnt += net->write_errcnt < SIZE_MAX;
		if (write->errc != net->write_errc) {
			net->write_errc = write->errc;
			// Only invoke the callback for unique write errors.
			net->on_write_error_func(net->write_errc,
					net->write_errc,
					net->on_write_error_arg);
		}
	} else if (net->write_errc) {
		assert(net->on_write_error_func);
		net->on_write_error_func(
				0, net->write_errc, net->on_write_error_arg);
		net->write_errc = 0;
		net->write_errcnt = 0;
	}

	// Remove the frame from the transmit queue, unless the write operation
	// was canceled, in which we discard the entire queue.
	assert(spscring_c_capacity(&net->tx_ring) >= 1);
	size_t n = 1;
	if (errc2num(write->errc) == ERRNUM_CANCELED) {
		n = spscring_c_capacity(&net->tx_ring);
		// Track the number of dropped frames. The first frame has
		// already been accounted for.
		net->write_errcnt += n - 1;
	}
	spscring_c_commit(&net->tx_ring, n);

	// Stop the timeout after receiving a write confirmation (or write
	// error).
	if (net->wait_confirm_submitted
			&& io_tqueue_abort_wait(net->tq, &net->wait_confirm))
		net->wait_confirm_submitted = 0;

	// Write the next frame, if available.
	net->write_submitted = 0;
	if (!net->shutdown && !io_can_net_do_wait(net))
		io_can_net_do_write(net);

#if !LELY_NO_THREADS
	mtx_unlock(&net->mtx);
#endif
}

static int
io_can_net_next_func(const struct timespec *tp, void *data)
{
	assert(tp);
	io_can_net_t *net = data;
	assert(net);

	// Ignore calls that do not change the next timeout.
	if (!timespec_cmp(&net->next, tp))
		return 0;

	// In case io_can_net_wait_next_func() is currently running, store the
	// time for the next earliest timeout so io_can_net_wait_next_func() can
	// re-submit the wait operation.
	net->next = *tp;

	if (net->shutdown)
		return 0;

	// Re-submit the wait operation with the new timeout, but only if we can
	// be sure io_can_net_wait_next_func() is not currently running.
	if (!net->wait_next_submitted
			|| io_tqueue_abort_wait(net->tq, &net->wait_next)) {
		net->wait_next_submitted = 1;
		net->wait_next.value = *tp;
		io_tqueue_submit_wait(net->tq, &net->wait_next);
	}

	return 0;
}

static int
io_can_net_send_func(const struct can_msg *msg, void *data)
{
	assert(msg);
	io_can_net_t *net = data;
	assert(net);

	size_t n = 1;
	size_t i = spscring_p_alloc(&net->tx_ring, &n);
	if (n) {
		net->tx_buf[i] = *msg;
		spscring_p_commit(&net->tx_ring, n);
		if (net->tx_errcnt) {
			assert(net->on_queue_error_func);
			net->on_queue_error_func(0, net->tx_errcnt,
					net->on_queue_error_arg);
			net->tx_errcnt = 0;
		}
		return 0;
	} else {
		set_errnum(ERRNUM_AGAIN);
		net->tx_errcnt += net->tx_errcnt < SIZE_MAX;
		if (net->tx_errcnt == 1) {
			// Only invoke the callback for the first transmission
			// error.
			assert(net->on_queue_error_func);
			net->on_queue_error_func(get_errc(), net->tx_errcnt,
					net->on_queue_error_arg);
		}
		return -1;
	}
}

static void
io_can_net_c_wait_func(struct spscring *ring, void *arg)
{
	(void)ring;
	io_can_net_t *net = arg;
	assert(net);

	// A frame was just added to the transmit queue; try to send it.
	if (!io_can_net_do_wait(net))
		io_can_net_do_write(net);
}

static inline io_can_net_t *
io_can_net_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, io_can_net_t, svc);
}

int
io_can_net_do_wait(io_can_net_t *net)
{
	assert(net);

	// Wait for next frame to become available.
	// clang-format off
	if (spscring_c_submit_wait(
			&net->tx_ring, 1, &io_can_net_c_wait_func, net))
		// clang-format on
		return 1;

	// Extract the frame from the transmit queue.
	size_t n = 1;
	size_t i = spscring_c_alloc(&net->tx_ring, &n);
	assert(n == 1);
	net->write_msg = net->tx_buf[i];

	return 0;
}

void
io_can_net_do_write(io_can_net_t *net)
{
	assert(net);
	assert(spscring_c_capacity(&net->tx_ring) >= 1);
	assert(!net->write_submitted);

	// Send the frame.
	net->write_submitted = 1;
	io_can_chan_submit_write(net->chan, &net->write);

	// Register a timeout for the write confirmation, if necessary.
	if (net->txtimeo >= 0
			&& (!net->wait_confirm_submitted
					|| io_tqueue_abort_wait(net->tq,
							&net->wait_confirm))) {
		net->wait_confirm_submitted = 1;
		io_clock_gettime(io_can_net_get_clock(net),
				&net->wait_confirm.value);
		timespec_add_msec(&net->wait_confirm.value, net->txtimeo);
		io_tqueue_submit_wait(net->tq, &net->wait_confirm);
	}
}

size_t
io_can_net_do_abort_tasks(io_can_net_t *net)
{
	assert(net);

	size_t n = 0;

	if (net->wait_next_submitted
			&& io_tqueue_abort_wait(net->tq, &net->wait_next)) {
		net->wait_next_submitted = 0;
		n++;
	}

	if (net->wait_confirm_submitted
			&& io_tqueue_abort_wait(net->tq, &net->wait_confirm)) {
		net->wait_confirm_submitted = 0;
		n++;
	}

	if (net->read_submitted
			&& io_can_chan_abort_read(net->chan, &net->read)) {
		net->read_submitted = 0;
		n++;
	}

	if (net->write_submitted
			&& io_can_chan_abort_write(net->chan, &net->write)) {
		net->write_submitted = 0;
		n++;
	}

	return 0;
}

static void
default_on_read_error_func(int errc, size_t errcnt, void *arg)
{
	(void)arg;

	if (errc)
		diag(DIAG_WARNING, errc, "error reading CAN frame");
	else
		diag(DIAG_INFO, 0,
				"CAN frame successfully read after %zu read error%s",
				errcnt, errcnt > 1 ? "s" : "");
}

static void
default_on_queue_error_func(int errc, size_t errcnt, void *arg)
{
	(void)arg;

	if (errc)
		diag(DIAG_WARNING, errc,
				"CAN transmit queue full; dropping frame");
	else
		diag(DIAG_INFO, 0,
				"CAN frame successfully queued after dropping %zd frame%s",
				errcnt, errcnt > 1 ? "s" : "");
}

static void
default_on_write_error_func(int errc, size_t errcnt, void *arg)
{
	(void)arg;

	if (errc)
		diag(DIAG_WARNING, errc, "error writing CAN frame");
	else
		diag(DIAG_INFO, 0,
				"CAN frame successfully written after %zu write error%s",
				errcnt, errcnt > 1 ? "s" : "");
}

static void
default_on_can_state_func(int new_state, int old_state, void *arg)
{
	(void)old_state;
	(void)arg;

	switch (new_state) {
	case CAN_STATE_ACTIVE:
		diag(DIAG_INFO, 0, "CAN bus is in the error active state");
		break;
	case CAN_STATE_PASSIVE:
		diag(DIAG_INFO, 0, "CAN bus is in the error passive state");
		break;
	case CAN_STATE_BUSOFF:
		diag(DIAG_WARNING, 0, "CAN bus is in the bus off state");
		break;
	case CAN_STATE_SLEEPING:
		diag(DIAG_INFO, 0, "CAN interface is in sleep mode");
		break;
	case CAN_STATE_STOPPED:
		diag(DIAG_WARNING, 0, "CAN interface is stopped");
		break;
	}
}

static void
default_on_can_error_func(int error, void *arg)
{
	(void)arg;

	if (error & CAN_ERROR_BIT)
		diag(DIAG_WARNING, 0, "single bit error detected on CAN bus");
	if (error & CAN_ERROR_STUFF)
		diag(DIAG_WARNING, 0, "bit stuffing error detected on CAN bus");
	if (error & CAN_ERROR_CRC)
		diag(DIAG_WARNING, 0, "CRC sequence error detected on CAN bus");
	if (error & CAN_ERROR_FORM)
		diag(DIAG_WARNING, 0, "form error detected on CAN bus");
	if (error & CAN_ERROR_ACK)
		diag(DIAG_WARNING, 0,
				"acknowledgment error detected on CAN bus");
	if (error & CAN_ERROR_OTHER)
		diag(DIAG_WARNING, 0,
				"one or more unknown errors detected on CAN bus");
}

#endif // !LELY_NO_MALLOC
