/**@file
 * This header file is part of the I/O library; it contains the CAN frame router
 * declarations.
 *
 * The CAN frame router enables applications to be notified when a CAN frame
 * with a specific identifier and combination of flags is received. Multiple
 * readers can receive the same frame. To avoid copying CAN (error) frames, all
 * operations are executed on a strand executor created by the CAN frame router.
 * The completion tasks of all matching readers are guaranteed to have finished
 * executing before the next frame is read.
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

#ifndef LELY_IO2_CAN_RT_H_
#define LELY_IO2_CAN_RT_H_

#include <lely/io2/can.h>
#include <lely/util/rbtree.h>

/// A CAN frame router.
typedef struct io_can_rt io_can_rt_t;

/// The result of a CAN frame read operation.
struct io_can_rt_read_msg_result {
	/**
	 * A pointer to the received CAN frame, or NULL on error (or if the
	 * operation is canceled). In the latter case, the error number is
	 * stored in #errc. The CAN frame is only guaranteed to be valid until
	 * the completion task of the read operation finishes executing.
	 */
	const struct can_msg *msg;
	/// The error number, obtained as if by get_errc(), if #msg is NULL.
	int errc;
};

/// A CAN frame read operation suitable for use with a CAN frame router.
struct io_can_rt_read_msg {
	/**
	 * The identifier of the CAN frame to be received. Upon successful
	 * completion of the read operation, `r.msg->id == id`.
	 */
	uint_least32_t id;
	/**
	 * The flags of the CAN frame to be received (any combination of
	 * #CAN_FLAG_IDE, #CAN_FLAG_RTR, #CAN_FLAG_FDF, #CAN_FLAG_BRS and
	 * #CAN_FLAG_ESI). Upon successful completion of the read operation,
	 * `r.msg->flags == flags`.
	 */
	uint_least8_t flags;
	/**
	 * The task (to be) submitted upon completion (or cancellation) of the
	 * read operation.
	 */
	struct ev_task task;
	/// The result of the read operation.
	struct io_can_rt_read_msg_result r;
	struct rbnode _node;
	struct sllist _queue;
};

/// The static initializer for #io_can_rt_read_msg.
#define IO_CAN_RT_READ_MSG_INIT(id, flags, func) \
	{ \
		(id), (flags), EV_TASK_INIT(NULL, func), { NULL, 0 }, \
				RBNODE_INIT, \
		{ \
			NULL, NULL \
		} \
	}

/// The result of a CAN error frame read operation.
struct io_can_rt_read_err_result {
	/**
	 * A pointer to the received CAN error frame, or NULL on error (or if
	 * the operation is canceled). In the latter case, the error number is
	 * stored in #errc. The CAN error frame is only guaranteed to be valid
	 * until the completion task of the read operation finishes executing.
	 */
	const struct can_err *err;
	/// The error number, obtained as if by get_errc(), if #err is NULL.
	int errc;
};

/// A CAN error frame read operation suitable for use with a CAN frame router.
struct io_can_rt_read_err {
	/**
	 * The task (to be) submitted upon completion (or cancellation) of the
	 * read operation.
	 */
	struct ev_task task;
	/// The result of the read operation.
	struct io_can_rt_read_err_result r;
};

/// The static initializer for #io_can_rt_read_err.
#define IO_CAN_RT_READ_ERR_INIT(func) \
	{ \
		EV_TASK_INIT(NULL, func), { NULL, 0 } \
	}

#ifdef __cplusplus
extern "C" {
#endif

void *io_can_rt_alloc(void);
void io_can_rt_free(void *ptr);
io_can_rt_t *io_can_rt_init(
		io_can_rt_t *rt, io_can_chan_t *chan, ev_exec_t *exec);
void io_can_rt_fini(io_can_rt_t *rt);

/**
 * Creates a new CAN frame router.
 *
 * @param chan a pointer to the CAN channel to be used to read and write CAN
 *             frames. During the lifetime of the CAN frame router, no other
 *             read operations SHOULD be submitted to the CAN channel.
 * @param exec a pointer to the executor used to execute asynchronous tasks.
 *
 * @returns a pointer to a new CAN frame router, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_can_rt_t *io_can_rt_create(io_can_chan_t *chan, ev_exec_t *exec);

/**
 * Destroys a CAN frame router. This function SHOULD NOT be called if there are
 * pending read operations, since their completion tasks will never be executed.
 * Use io_can_rt_async_shutdown() to cancel all operations and wait for the
 * completion tasks to finish executing.
 *
 * @see io_can_rt_create()
 */
void io_can_rt_destroy(io_can_rt_t *rt);

/**
 * Returns a pointer to the abstract I/O device representing the CAN frame
 * router.
 */
io_dev_t *io_can_rt_get_dev(const io_can_rt_t *rt);

/// Returns a pointer to the CAN channel used by the CAN frame router.
io_can_chan_t *io_can_rt_get_chan(const io_can_rt_t *rt);

/**
 * Submits a CAN frame read operation to a CAN frame router. Once a matching CAN
 * frame is received (or a read error occurs), the completion task is submitted
 * for execution to the strand executor of the CAN frame router.
 */
void io_can_rt_submit_read_msg(
		io_can_rt_t *rt, struct io_can_rt_read_msg *read_msg);

/**
 * Cancels the specified CAN frame read operation if it is pending. The
 * completion task is submitted for execution with
 * <b>errc</b> = #errnum2c(#ERRNUM_CANCELED).
 *
 * @returns 1 if the operation was canceled, and 0 if it was not pending.
 *
 * @see io_dev_cancel()
 */
size_t io_can_rt_cancel_read_msg(
		io_can_rt_t *rt, struct io_can_rt_read_msg *read_msg);

/**
 * Aborts the specified CAN frame read operation if it is pending. If aborted,
 * the completion task is _not_ submitted for execution.
 *
 * @returns 1 if the operation was aborted, and 0 if it was not pending.
 *
 * @see io_dev_abort()
 */
size_t io_can_rt_abort_read_msg(
		io_can_rt_t *rt, struct io_can_rt_read_msg *read_msg);

/**
 * Submits an asynchronous CAN frame read operation to a CAN frame router and
 * creates a future which becomes ready once the read operation completes (or is
 * canceled). The result of the future has type #io_can_rt_read_msg_result.
 *
 * @param rt        a pointer to a CAN frame router.
 * @param id        the identifier of the CAN frame to be received.
 * @param flags     the flags of the CAN frame to be received (any combination
 *                  of #CAN_FLAG_IDE, #CAN_FLAG_RTR, #CAN_FLAG_FDF,
 *                  #CAN_FLAG_BRS and #CAN_FLAG_ESI).
 * @param pread_msg the address at which to store a pointer to the read
 *                  operation (can be NULL).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_can_rt_async_read_msg(io_can_rt_t *rt, uint_least32_t id,
		uint_least8_t flags, struct io_can_rt_read_msg **pread_msg);

/**
 * Submits a CAN error frame read operation to a CAN frame router. Once a CAN
 * error frame is received (or a read error occurs), the completion task is
 * submitted for execution to the strand executor of the CAN frame router.
 */
void io_can_rt_submit_read_err(
		io_can_rt_t *rt, struct io_can_rt_read_err *read_err);

/**
 * Cancels the specified CAN error frame read operation if it is pending. The
 * completion task is submitted for execution with
 * <b>errc</b> = #errnum2c(#ERRNUM_CANCELED).
 *
 * @returns 1 if the operation was canceled, and 0 if it was not pending.
 *
 * @see io_dev_cancel()
 */
size_t io_can_rt_cancel_read_err(
		io_can_rt_t *rt, struct io_can_rt_read_err *read_err);

/**
 * Aborts the specified CAN error frame read operation if it is pending. If
 * aborted, the completion task is _not_ submitted for execution.
 *
 * @returns 1 if the operation was aborted, and 0 if it was not pending.
 *
 * @see io_dev_abort()
 */
size_t io_can_rt_abort_read_err(
		io_can_rt_t *rt, struct io_can_rt_read_err *read_err);

/**
 * Submits an asynchronous CAN error frame read operation to a CAN frame router
 * and creates a future which becomes ready once the read operation completes
 * (or is canceled). The result of the future has type
 * #io_can_rt_read_err_result.
 *
 * @param rt        a pointer to a CAN frame router.
 * @param pread_err the address at which to store a pointer to the read
 *                  operation (can be NULL).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_can_rt_async_read_err(
		io_can_rt_t *rt, struct io_can_rt_read_err **pread_err);

/**
 * Shuts down a CAN frame router, cancels all pending operations and creates a
 * (void) future which becomes ready once it is safe to invoke
 * io_can_rt_destroy() (i.e., once the completion task of the last canceled
 * reader has finished executing).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_can_rt_async_shutdown(io_can_rt_t *rt);

/**
 * Obtains a pointer to a CAN frame read operation from a pointer to its
 * completion task.
 */
struct io_can_rt_read_msg *io_can_rt_read_msg_from_task(struct ev_task *task);

/**
 * Obtains a pointer to a CAN error frame read operation from a pointer to its
 * completion task.
 */
struct io_can_rt_read_err *io_can_rt_read_err_from_task(struct ev_task *task);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_CAN_RT_H_
