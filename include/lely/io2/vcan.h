/**@file
 * This header file is part of the I/O library; it contains the virtual CAN bus
 * declarations.
 *
 * A virtual CAN bus makes it possible to write platform-independent tests for
 * CAN applications and to create controlled error conditions. It consists of a
 * virtual CAN controller with which one or more virtual CAN channels are
 * registered. When a virtual channel sends a message, it is placed into the
 * receive queue of all other channels. Each channel only as a receive queue.
 * Send operations succeed if every (other) channel has a slot available in
 * their receive queue; otherwise they block or time out.
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_IO2_VCAN_H_
#define LELY_IO2_VCAN_H_

#include <lely/io2/can.h>
#include <lely/io2/clock.h>

#ifdef __cplusplus
extern "C" {
#endif

void *io_vcan_ctrl_alloc(void);
void io_vcan_ctrl_free(void *ptr);
io_can_ctrl_t *io_vcan_ctrl_init(io_can_ctrl_t *ctrl, io_clock_t *clock,
		int flags, int nominal, int data, int state);
void io_vcan_ctrl_fini(io_can_ctrl_t *ctrl);

/**
 * Creates a new virtual CAN controller.
 *
 * @param clock   a pointer to the clock used to obtain the timestamp when
 *                sending CAN frames.
 * @param flags   the flags specifying which CAN bus features MUST be enabled
 *                (any combination of #IO_CAN_BUS_FLAG_ERR, #IO_CAN_BUS_FLAG_FDF
 *                and #IO_CAN_BUS_FLAG_BRS).
 * @param nominal the nominal bitrate. For the CAN FD protocol, this is the bit
 *                rate of the arbitration phase. If <b>nominal</b> is 0, the
 *                default value #LELY_IO_VCAN_BITRATE is used.
 * @param data    the data bitrate. This bit rate is only defined for the CAN FD
 *                protocol; the value is ignored otherwise. If <b>data</b> is 0,
 *                the data bitrate equals the nominal bitrate.
 * @param state   the initial state of the virtual CAN bus (one of
 *                #CAN_STATE_ACTIVE, #CAN_STATE_PASSIVE, #CAN_STATE_BUSOFF,
 *                #CAN_STATE_SLEEPING or #CAN_STATE_STOPPED). If <b>state</b>
 *                is #CAN_STATE_STOPPED, the controller is stopped.
 *
 * @returns a pointer to a new CAN controller, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_can_ctrl_t *io_vcan_ctrl_create(
		io_clock_t *clock, int flags, int nominal, int data, int state);

/// Destroys a virtual CAN controller. @see io_vcan_ctrl_create()
void io_vcan_ctrl_destroy(io_can_ctrl_t *ctrl);

/**
 * Sets the state of a virtual CAN bus: one of #CAN_STATE_ACTIVE,
 * #CAN_STATE_PASSIVE, #CAN_STATE_BUSOFF, #CAN_STATE_SLEEPING or
 * #CAN_STATE_STOPPED.
 *
 * If the requested state is #CAN_STATE_STOPPED, the controller will be stopped
 * as if by io_can_ctrl_stop() and all pending and future I/O operations will
 * fail with error number #ERRNUM_NETDOWN. Subsequent calls to this function
 * have no effect until the controller is restarted with io_can_ctrl_restart().
 *
 * If the requested state differs from the current state and is _not_
 * #CAN_STATE_STOPPED, and the controller supports error frames
 * (#IO_CAN_BUS_FLAG_ERR), an error frame with the new state is sent to all
 * registered virtual CAN channels.
 *
 * @see io_can_ctrl_get_state()
 */
void io_vcan_ctrl_set_state(io_can_ctrl_t *ctrl, int state);

/**
 * Writes a CAN frame to a all virtual CAN channels registered with a virtual
 * CAN controller. This function blocks until the frame is written or an error
 * occurs.
 *
 * @param ctrl    a pointer to a virtual CAN controller.
 * @param msg     a pointer to the CAN frame to be written.
 * @param timeout the maximum number of milliseconds this function will block.
 *                If <b>timeout</b> is negative, this function will block
 *                indefinitely.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_vcan_ctrl_write_err()
 */
int io_vcan_ctrl_write_msg(
		io_can_ctrl_t *ctrl, const struct can_msg *msg, int timeout);

/**
 * Writes a CAN error frame to a all virtual CAN channels registered with a
 * virtual CAN controller. This function blocks until the error frame is written
 * or an error occurs. If the #IO_CAN_BUS_FLAG_ERR flag was not specified when
 * the controller was created with io_vcan_ctrl_create(), the controller does
 * not support error frames and this function will fail.
 *
 * @param ctrl    a pointer to a virtual CAN controller.
 * @param err     a pointer to the CAN error frame to be written.
 * @param timeout the maximum number of milliseconds this function will block.
 *                If <b>timeout</b> is negative, this function will block
 *                indefinitely.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_vcan_ctrl_write_msg()
 */
int io_vcan_ctrl_write_err(
		io_can_ctrl_t *ctrl, const struct can_err *err, int timeout);

void *io_vcan_chan_alloc(void);
void io_vcan_chan_free(void *ptr);
io_can_chan_t *io_vcan_chan_init(io_can_chan_t *chan, io_ctx_t *ctx,
		ev_exec_t *exec, size_t rxlen);
void io_vcan_chan_fini(io_can_chan_t *chan);

/**
 * Creates a new virtual CAN channel.
 *
 * @param ctx   a pointer to the I/O context with which the channel should be
 *              registered.
 * @param exec  a pointer to the executor used to execute asynchronous tasks.
 * @param rxlen the receive queue length (in number of frames) of the channel.
 *              If <b>rxlen</b> is 0, the default value #LELY_IO_VCAN_RXLEN is
 *              used.
 *
 * @returns a pointer to a new CAN channel, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_can_chan_t *io_vcan_chan_create(
		io_ctx_t *ctx, ev_exec_t *exec, size_t rxlen);

/// Destroys a virtual CAN channel. @see io_vcan_chan_create()
void io_vcan_chan_destroy(io_can_chan_t *chan);

/**
 * Returns a pointer to the virtual CAN controller with which a virtual CAN
 * channel is registered, or NULL if the channel is closed.
 */
io_can_ctrl_t *io_vcan_chan_get_ctrl(const io_can_chan_t *chan);

/**
 * Opens a virtual CAN channel by registering it with the specified virtual CAN
 * controller. If the channel was already open, it is first closed as if by
 * io_vcan_chan_close().
 *
 * @post io_vcan_chan_is_open() returns 1.
 */
void io_vcan_chan_open(io_can_chan_t *chan, io_can_ctrl_t *ctrl);

/**
 * Returns 1 if the CAN channel is open and 0 if not. This function is
 * equivalent to `io_vcan_chan_get_ctrl(chan) != NULL`.
 */
int io_vcan_chan_is_open(const io_can_chan_t *chan);

/**
 * Closes a virtual CAN channel. Any pending read or write operations are
 * canceled as if by io_can_chan_cancel_read() and io_can_chan_cancel_write().
 *
 * @post io_vcan_chan_is_open() returns 0.
 */
void io_vcan_chan_close(io_can_chan_t *chan);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_VCAN_H_
