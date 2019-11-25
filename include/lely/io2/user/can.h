/**@file
 * This header file is part of the I/O library; it contains the user-defined CAN
 * channel declarations.
 *
 * The user-defined CAN channel is a passive channel; it does not actively read
 * CAN frames, but requires the user to notify it of incoming (error) frames
 * with io_user_can_chan_on_msg() and io_user_can_chan_on_err(). A user-defined
 * callback function is invoked when a CAN frame needs to be written.
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

#ifndef LELY_IO2_USER_CAN_H_
#define LELY_IO2_USER_CAN_H_

#include <lely/io2/can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of function invoked by a user-defined CAN channel when a CAN frame
 * needs to be written.
 *
 * @param msg     a pointer to the CAN frame to be written.
 * @param arg     the user-specific value submitted to
 *                io_user_can_chan_create().
 * @param timeout the maximum number of milliseconds this function SHOULD block.
 *                If <b>timeout</b> is negative, this function SHOULD block
 *                indefinitely.
 *
 * @returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with set_errc() or set_errnum().
 */
typedef int io_user_can_chan_write_t(
		const struct can_msg *msg, int timeout, void *arg);

void *io_user_can_chan_alloc(void);
void io_user_can_chan_free(void *ptr);
io_can_chan_t *io_user_can_chan_init(io_can_chan_t *chan, io_ctx_t *ctx,
		ev_exec_t *exec, int flags, size_t rxlen, int txtimeo,
		io_user_can_chan_write_t *func, void *arg);
void io_user_can_chan_fini(io_can_chan_t *chan);

/**
 * Creates a new user-defined CAN channel.
 *
 * @param ctx     a pointer to the I/O context with which the channel should be
 *                registered.
 * @param exec    a pointer to the executor used to execute asynchronous tasks.
 * @param flags   the flags specifying which CAN bus features MUST be enabled
 *                (any combination of #IO_CAN_BUS_FLAG_ERR, #IO_CAN_BUS_FLAG_FDF
 *                and #IO_CAN_BUS_FLAG_BRS).
 * @param rxlen   the receive queue length (in number of frames) of the channel.
 *                If <b>rxlen</b> is 0, the default value
 *                #LELY_IO_USER_CAN_RXLEN is used.
 * @param txtimeo the timeout (in milliseconds) passed to <b>func</b> when
 *                writing a CAN frame asynchronously. If <b>txtimeo</b> is 0,
 *                the default value #LELY_IO_TX_TIMEOUT is used. If
 *                <b>txtimeo</b> is negative, the write function will wait
 *                indefinitely.
 * @param func    a pointer to the function to be invoked when a CAN frame needs
 *                to be written (can be NULL).
 * @param arg     the user-specific value to be passed as the second argument to
 *                <b>func</b>.
 *
 * @returns a pointer to a new channel, or NULL on error. In the latter case,
 * the error number can be obtained with get_errc().
 */
io_can_chan_t *io_user_can_chan_create(io_ctx_t *ctx, ev_exec_t *exec,
		int flags, size_t rxlen, int txtimeo,
		io_user_can_chan_write_t *func, void *arg);

/// Destroys a user-defined CAN channel. @see io_user_can_chan_create()
void io_user_can_chan_destroy(io_can_chan_t *chan);

/**
 * Processes an incoming CAN frame.
 *
 * @param chan    a pointer to a user-defined CAN channel.
 * @param msg     a pointer to the incoming CAN frame.
 * @param tp      a pointer to the system time at which the CAN frame was
 *                received (can be NULL).
 * @param timeout the maximum number of milliseconds this function will block
 *                when the receive queue is full. If <b>timeout</b> is negative,
 *                this function will block indefinitely.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_user_can_chan_on_msg(io_can_chan_t *chan, const struct can_msg *msg,
		const struct timespec *tp, int timeout);

/**
 * Processes an incoming CAN error frame.
 *
 * @param chan    a pointer to a user-defined CAN channel.
 * @param err     a pointer to the incoming CAN error frame.
 * @param tp      a pointer to the system time at which the CAN error frame was
 *                received (can be NULL).
 * @param timeout the maximum number of milliseconds this function will block
 *                when the receive queue is full. If <b>timeout</b> is negative,
 *                this function will block indefinitely.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_user_can_chan_on_err(io_can_chan_t *chan, const struct can_err *err,
		const struct timespec *tp, int timeout);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_USER_CAN_H_
