/**@file
 * This header file is part of the I/O library; it contains the abstract CAN bus
 * interface.
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_IO2_CAN_H_
#define LELY_IO2_CAN_H_

#include <lely/ev/future.h>
#include <lely/ev/task.h>
#include <lely/io2/can/err.h>
#include <lely/io2/can/msg.h>
#include <lely/io2/dev.h>
#include <lely/libc/time.h>

#ifndef LELY_IO_CAN_INLINE
#define LELY_IO_CAN_INLINE static inline
#endif

/// The CAN bus flags.
enum io_can_bus_flag {
	/// Reception of error frames is enabled.
	IO_CAN_BUS_FLAG_ERR = 1u << 0,
#if !LELY_NO_CANFD
	/// FD Format (formerly Extended Data Length) support is enabled.
	IO_CAN_BUS_FLAG_FDF = 1u << 1,
	/// Bit Rate Switch support is enabled.
	IO_CAN_BUS_FLAG_BRS = 1u << 2,
#endif
	IO_CAN_BUS_FLAG_NONE = 0,
#if LELY_NO_CANFD
	IO_CAN_BUS_FLAG_MASK = IO_CAN_BUS_FLAG_ERR
#else
	IO_CAN_BUS_FLAG_MASK = IO_CAN_BUS_FLAG_ERR | IO_CAN_BUS_FLAG_FDF
			| IO_CAN_BUS_FLAG_BRS
#endif
};

/// An abstract CAN controller.
typedef const struct io_can_ctrl_vtbl *const io_can_ctrl_t;

/// An abstract CAN channel.
typedef const struct io_can_chan_vtbl *const io_can_chan_t;

/// The result of a CAN channel read operation.
struct io_can_chan_read_result {
	/**
	 * The result of the read operation: 1 if a CAN frame is received, 0 if
	 * an error frame is received, or -1 on error (or if the operation is
	 * canceled). In the latter case, the error number is stored in #errc.
	 */
	int result;
	/// The error number, obtained as if by get_errc(), if #result is -1.
	int errc;
};

/// A CAN channel read operation.
struct io_can_chan_read {
	/**
	 * The address at which to store the CAN frame. If not NULL, it is the
	 * responsibility of the user to ensure the buffer remains valid until
	 * the read operation completes.
	 */
	struct can_msg *msg;
	/**
	 * The address at which to store the CAN error frame. If not NULL, it
	 * is the responsibility of the user to ensure the buffer remains valid
	 * until the read operation completes.
	 */
	struct can_err *err;
	/**
	 * The address at which to store the system time at which the CAN frame
	 * or CAN error frame was received. If not NULL, it is the
	 * responsibility of the user to ensure the buffer remains valid until
	 * the read operation completes.
	 */
	struct timespec *tp;
	/**
	 * The task (to be) submitted upon completion (or cancellation) of the
	 * read operation.
	 */
	struct ev_task task;
	/// The result of the read operation.
	struct io_can_chan_read_result r;
};

/// The static initializer for #io_can_chan_read.
#define IO_CAN_CHAN_READ_INIT(msg, err, tp, exec, func) \
	{ \
		(msg), (err), (tp), EV_TASK_INIT(exec, func), { 0, 0 } \
	}

/// A CAN channel write operation.
struct io_can_chan_write {
	/**
	 * A pointer to the CAN frame to be written. It is the responsibility of
	 * the user to ensure the buffer remains valid until the write operation
	 * completes.
	 */
	const struct can_msg *msg;
	/**
	 * The task (to be) submitted upon completion (or cancellation) of the
	 * write operation.
	 */
	struct ev_task task;
	/**
	 * The error number, obtained as if by get_errc(), if an error occurred
	 * or the operation was canceled.
	 */
	int errc;
};

/// The static initializer for #io_can_chan_write.
#define IO_CAN_CHAN_WRITE_INIT(msg, exec, func) \
	{ \
		(msg), EV_TASK_INIT(exec, func), 0 \
	}

#ifdef __cplusplus
extern "C" {
#endif

struct io_can_ctrl_vtbl {
	int (*stop)(io_can_ctrl_t *ctrl);
	int (*stopped)(const io_can_ctrl_t *ctrl);
	int (*restart)(io_can_ctrl_t *ctrl);
	int (*get_bitrate)(
			const io_can_ctrl_t *ctrl, int *pnominal, int *pdata);
	int (*set_bitrate)(io_can_ctrl_t *ctrl, int nominal, int data);
	int (*get_state)(const io_can_ctrl_t *ctrl);
};

struct io_can_chan_vtbl {
	io_dev_t *(*get_dev)(const io_can_chan_t *chan);
	int (*get_flags)(const io_can_chan_t *chan);
	int (*read)(io_can_chan_t *chan, struct can_msg *msg,
			struct can_err *err, struct timespec *tp, int timeout);
	void (*submit_read)(io_can_chan_t *chan, struct io_can_chan_read *read);
	int (*write)(io_can_chan_t *chan, const struct can_msg *msg,
			int timeout);
	void (*submit_write)(
			io_can_chan_t *chan, struct io_can_chan_write *write);
};

/**
 * Stops a CAN controller. This terminates the transmission and reception of
 * CAN frames. If the controller is already stopped, this function has no
 * effect.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post on success, io_can_ctrl_stopped() returns 1.
 */
LELY_IO_CAN_INLINE int io_can_ctrl_stop(io_can_ctrl_t *ctrl);

/**
 * Returns 1 in the CAN controller is stopped, 0 if not, and -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
LELY_IO_CAN_INLINE int io_can_ctrl_stopped(const io_can_ctrl_t *ctrl);

/**
 * (Re)starts a CAN contoller.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post on success, io_can_ctrl_stopped() returns 0.
 */
LELY_IO_CAN_INLINE int io_can_ctrl_restart(io_can_ctrl_t *ctrl);

/**
 * Obtains the bitrate(s) of a CAN controller.
 *
 * @param ctrl     a pointer to a CAN controller.
 * @param pnominal the address at which to store the nominal bitrate (can be
 *                 NULL). For the CAN FD protocol, this is the bit rate of the
 *                 arbitration phase.
 * @param pdata    the address at which to store the data bit rate (can be
 *                 NULL). This bit rate is only defined for the CAN FD protocol;
 *                 the value will be 0 otherwise.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
LELY_IO_CAN_INLINE int io_can_ctrl_get_bitrate(
		const io_can_ctrl_t *ctrl, int *pnominal, int *pdata);

/**
 * Configures the bitrate(s) of a CAN controller.
 *
 * @param ctrl    a pointer to a CAN controller.
 * @param nominal the nominal bitrate. For the CAN FD protocol, this is the bit
 *                rate of the arbitration phase.
 * @param data    the data bitrate. This bit rate is only defined for the CAN FD
 *                protocol; the value is ignored otherwise.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post io_can_ctrl_stopped() returns 1.
 */
LELY_IO_CAN_INLINE int io_can_ctrl_set_bitrate(
		io_can_ctrl_t *ctrl, int nominal, int data);

/**
 * Returns the state of the CAN controller: one of #CAN_STATE_ACTIVE,
 * #CAN_STATE_PASSIVE, #CAN_STATE_BUSOFF, #CAN_STATE_SLEEPING or
 * #CAN_STATE_STOPPED, or -1 on error. In the latter case, the error number can
 * be obtained with get_errc().
 */
LELY_IO_CAN_INLINE int io_can_ctrl_get_state(const io_can_ctrl_t *ctrl);

/// @see io_dev_get_ctx()
static inline io_ctx_t *io_can_chan_get_ctx(const io_can_chan_t *chan);

/// @see io_dev_get_exec()
static inline ev_exec_t *io_can_chan_get_exec(const io_can_chan_t *chan);

/// @see io_dev_cancel()
static inline size_t io_can_chan_cancel(
		io_can_chan_t *chan, struct ev_task *task);

/// @see io_dev_abort()
static inline size_t io_can_chan_abort(
		io_can_chan_t *chan, struct ev_task *task);

/// Returns a pointer to the abstract I/O device representing the CAN channel.
LELY_IO_CAN_INLINE io_dev_t *io_can_chan_get_dev(const io_can_chan_t *chan);

/**
 * Returns the flafs of the CAN bus: any combination of #IO_CAN_BUS_FLAG_ERR,
 * #IO_CAN_BUS_FLAG_FDF and #IO_CAN_BUS_FLAG_BRS, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
LELY_IO_CAN_INLINE int io_can_chan_get_flags(const io_can_chan_t *chan);

/**
 * Reads a CAN frame or CAN error frame from a CAN channel. This function blocks
 * until a frame is read, the timeout expires or an error occurs.
 *
 * @param chan    a pointer to a CAN channel.
 * @param msg     the address at which to store the CAN frame (can be NULL).
 * @param err     the address at which to store the CAN error frame (can be
 *                NULL).
 * @param tp      the address at which to store the system time at which the CAN
 *                frame or CAN error frame was received (can be NULL).
 * @param timeout the maximum number of milliseconds this function will block.
 *                If <b>timeout</b> is negative, this function will block
 *                indefinitely.
 *
 * @returns 1 if a CAN frame is received, 0 if an error frame is received, or -1
 * on error. In the latter case, the error number can be obtained with
 * get_errc().
 */
LELY_IO_CAN_INLINE int io_can_chan_read(io_can_chan_t *chan,
		struct can_msg *msg, struct can_err *err, struct timespec *tp,
		int timeout);

/**
 * Submits a read operation to a CAN channel. The completion task is submitted
 * for execution once a CAN frame or error frame is received or a read error
 * occurs.
 */
LELY_IO_CAN_INLINE void io_can_chan_submit_read(
		io_can_chan_t *chan, struct io_can_chan_read *read);

/**
 * Cancels the specified CAN channel read operation if it is pending. The
 * completion task is submitted for execution with <b>result</b> = -1 and
 * <b>errc</b> = #errnum2c(#ERRNUM_CANCELED).
 *
 * @returns 1 if the operation was canceled, and 0 if it was not pending.
 *
 * @see io_dev_cancel()
 */
static inline size_t io_can_chan_cancel_read(
		io_can_chan_t *chan, struct io_can_chan_read *read);

/**
 * Aborts the specified CAN channel read operation if it is pending. If aborted,
 * the completion task is _not_ submitted for execution.
 *
 * @returns 1 if the operation was aborted, and 0 if it was not pending.
 *
 * @see io_dev_abort()
 */
static inline size_t io_can_chan_abort_read(
		io_can_chan_t *chan, struct io_can_chan_read *read);

/**
 * Submits an asynchronous read operation to a CAN channel and creates a future
 * which becomes ready once the read operation completes (or is canceled). The
 * result of the future has type #io_can_chan_read_result.
 *
 * @param chan  a pointer to a CAN channel.
 * @param exec  a pointer to the executor used to execute the completion
 *              function of the read operation. If NULL, the default executor of
 *              the CAN channel is used.
 * @param msg   the address at which to store the CAN frame (can be NULL).
 * @param err   the address at which to store the CAN error frame (can be NULL).
 * @param tp    the address at which to store the system time at which the CAN
 *              frame or CAN error frame was received (can be NULL).
 * @param pread the address at which to store a pointer to the read operation
 *              (can be NULL).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_can_chan_async_read(io_can_chan_t *chan, ev_exec_t *exec,
		struct can_msg *msg, struct can_err *err, struct timespec *tp,
		struct io_can_chan_read **pread);

/**
 * Writes a CAN frame to a CAN channel. This function blocks until the frame is
 * written, the timeout expires or an error occurs.
 *
 * @param chan    a pointer to a CAN channel.
 * @param msg     a pointer to the CAN frame to be written.
 * @param timeout the maximum number of milliseconds this function will block.
 *                If <b>timeout</b> is negative, this function will block
 *                indefinitely.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
LELY_IO_CAN_INLINE int io_can_chan_write(
		io_can_chan_t *chan, const struct can_msg *msg, int timeout);

/**
 * Submits a write operation to a CAN channel. The completion task is submitted
 * for execution once the CAN frame is written or a write error occurs.
 */
LELY_IO_CAN_INLINE void io_can_chan_submit_write(
		io_can_chan_t *chan, struct io_can_chan_write *write);

/**
 * Cancels the specified CAN channel write operation if it is pending. The
 * completion task is submitted for execution with <b>result</b> = -1 and
 * <b>errc</b> = #errnum2c(#ERRNUM_CANCELED).
 *
 * @returns 1 if the operation was canceled, and 0 if it was not pending.
 *
 * @see io_dev_cancel()
 */
static inline size_t io_can_chan_cancel_write(
		io_can_chan_t *chan, struct io_can_chan_write *write);

/**
 * Aborts the specified CAN channel write operation if it is pending. If
 * aborted, the completion task is _not_ submitted for execution.
 *
 * @returns 1 if the operation was aborted, and 0 if it was not pending.
 *
 * @see io_dev_abort()
 */
static inline size_t io_can_chan_abort_write(
		io_can_chan_t *chan, struct io_can_chan_write *write);

/**
 * Submits an asynchronous write operation to a CAN channel and creates a future
 * which becomes ready once the write operation completes (or is canceled). The
 * result of the future is an `int` containing the error number.
 *
 * @param chan   a pointer to a CAN channel.
 * @param exec   a pointer to the executor used to execute the completion
 *               function of the write operation. If NULL, the default executor
 *               of the CAN channel is used.
 * @param msg    a pointer to the CAN frame to be writen.
 * @param pwrite the address at which to store a pointer to the write operation
 *               (can be NULL).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_can_chan_async_write(io_can_chan_t *chan, ev_exec_t *exec,
		const struct can_msg *msg, struct io_can_chan_write **pwrite);

/**
 * Obtains a pointer to a CAN channel read operation from a pointer to its
 * completion task.
 */
struct io_can_chan_read *io_can_chan_read_from_task(struct ev_task *task);

/**
 * Obtains a pointer to a CAN channel write operation from a pointer to its
 * completion task.
 */
struct io_can_chan_write *io_can_chan_write_from_task(struct ev_task *task);

inline int
io_can_ctrl_stop(io_can_ctrl_t *ctrl)
{
	return (*ctrl)->stop(ctrl);
}

inline int
io_can_ctrl_stopped(const io_can_ctrl_t *ctrl)
{
	return (*ctrl)->stopped(ctrl);
}

inline int
io_can_ctrl_restart(io_can_ctrl_t *ctrl)
{
	return (*ctrl)->restart(ctrl);
}

inline int
io_can_ctrl_get_bitrate(const io_can_ctrl_t *ctrl, int *pnominal, int *pdata)
{
	return (*ctrl)->get_bitrate(ctrl, pnominal, pdata);
}

inline int
io_can_ctrl_set_bitrate(io_can_ctrl_t *ctrl, int nominal, int data)
{
	return (*ctrl)->set_bitrate(ctrl, nominal, data);
}

inline int
io_can_ctrl_get_state(const io_can_ctrl_t *ctrl)
{
	return (*ctrl)->get_state(ctrl);
}

static inline io_ctx_t *
io_can_chan_get_ctx(const io_can_chan_t *chan)
{
	return io_dev_get_ctx(io_can_chan_get_dev(chan));
}

static inline ev_exec_t *
io_can_chan_get_exec(const io_can_chan_t *chan)
{
	return io_dev_get_exec(io_can_chan_get_dev(chan));
}

static inline size_t
io_can_chan_cancel(io_can_chan_t *chan, struct ev_task *task)
{
	return io_dev_cancel(io_can_chan_get_dev(chan), task);
}

static inline size_t
io_can_chan_abort(io_can_chan_t *chan, struct ev_task *task)
{
	return io_dev_abort(io_can_chan_get_dev(chan), task);
}

inline io_dev_t *
io_can_chan_get_dev(const io_can_chan_t *chan)
{
	return (*chan)->get_dev(chan);
}

inline int
io_can_chan_get_flags(const io_can_chan_t *chan)
{
	return (*chan)->get_flags(chan);
}

inline int
io_can_chan_read(io_can_chan_t *chan, struct can_msg *msg, struct can_err *err,
		struct timespec *tp, int timeout)
{
	return (*chan)->read(chan, msg, err, tp, timeout);
}

inline void
io_can_chan_submit_read(io_can_chan_t *chan, struct io_can_chan_read *read)
{
	(*chan)->submit_read(chan, read);
}

static inline size_t
io_can_chan_cancel_read(io_can_chan_t *chan, struct io_can_chan_read *read)
{
	return io_can_chan_cancel(chan, &read->task);
}

static inline size_t
io_can_chan_abort_read(io_can_chan_t *chan, struct io_can_chan_read *read)
{
	return io_can_chan_abort(chan, &read->task);
}

inline int
io_can_chan_write(io_can_chan_t *chan, const struct can_msg *msg, int timeout)
{
	return (*chan)->write(chan, msg, timeout);
}

inline void
io_can_chan_submit_write(io_can_chan_t *chan, struct io_can_chan_write *write)
{
	(*chan)->submit_write(chan, write);
}

static inline size_t
io_can_chan_cancel_write(io_can_chan_t *chan, struct io_can_chan_write *write)
{
	return io_can_chan_cancel(chan, &write->task);
}

static inline size_t
io_can_chan_abort_write(io_can_chan_t *chan, struct io_can_chan_write *write)
{
	return io_can_chan_abort(chan, &write->task);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_CAN_H_
