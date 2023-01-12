/**@file
 * This header file is part of the I/O library; it contains the CAN bus
 * declarations for Linux.
 *
 * This implementation uses the SocketCAN interface.
 *
 * When the transmit queue of a SocketCAN network interface is full, `write()`
 * and `send()` operations return `ENOBUFS` instead of blocking or returning
 * `EAGAIN`. Those operations only block if the per-socket `SO_SNDBUF` limit is
 * reached. In order to achieve the expected blocking behavior, this
 * implementation sets the `SO_SNDBUF` limit to its minimal value. It is the
 * responsibility of the user to ensure the transmit queue is large enough to
 * prevent `ENOBUFS` errors (typically at least 15 times the number of open file
 * descriptors referring to the same network interface, see section 3.4 in
 * https://rtime.felk.cvut.cz/can/socketcan-qdisc-final.pdf).
 *
 * @copyright 2018-2022 Lely Industries N.V.
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

#ifndef LELY_IO2_LINUX_CAN_H_
#define LELY_IO2_LINUX_CAN_H_

#include <lely/io2/can.h>
#include <lely/io2/sys/io.h>

#ifdef __cplusplus
extern "C" {
#endif

void *io_can_ctrl_alloc(void);
void io_can_ctrl_free(void *ptr);
io_can_ctrl_t *io_can_ctrl_init(
		io_can_ctrl_t *ctrl, unsigned int index, size_t txlen);
void io_can_ctrl_fini(io_can_ctrl_t *ctrl);

/**
 * Creates a new CAN controller from an interface name.
 *
 * This function MAY need the CAP_NET_ADMIN capability to set the transmit queue
 * length of the specified SocketCAN interface to at least <b>txlen</b>.
 *
 * @param name  the name of a SocketCAN network interface.
 * @param txlen the transmission queue length (in number of frames) of the
 *              network interface. If <b>txlen</b> is 0, the default value
 *              #LELY_IO_CAN_TXLEN is used.
 *
 * @returns a pointer to a new CAN controller, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_can_ctrl_create_from_index()
 */
io_can_ctrl_t *io_can_ctrl_create_from_name(const char *name, size_t txlen);

/**
 * Creates a new CAN controller from an interface index.
 *
 * This function MAY need the CAP_NET_ADMIN capability to set the transmit queue
 * length of the specified SocketCAN interface to at least <b>txlen</b>.
 *
 * @param index the index of a SocketCAN network interface.
 * @param txlen the transmission queue length (in number of frames) of the
 *              network interface. If <b>txlen</b> is 0, the default value
 *              #LELY_IO_CAN_TXLEN is used.
 *
 * @returns a pointer to a new CAN controller, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_can_ctrl_create_from_name()
 */
io_can_ctrl_t *io_can_ctrl_create_from_index(unsigned int index, size_t txlen);

/**
 * Destroys a CAN controller.
 *
 * @see io_can_ctrl_create_from_name(), io_can_ctrl_create_from_index()
 */
void io_can_ctrl_destroy(io_can_ctrl_t *ctrl);

/// Returns the interface name of a CAN controller.
const char *io_can_ctrl_get_name(const io_can_ctrl_t *ctrl);

/// Returns the interface index of a CAN controller.
unsigned int io_can_ctrl_get_index(const io_can_ctrl_t *ctrl);

/**
 * Returns the flags specifying which CAN bus features are enabled.
 *
 * @returns any combination of IO_CAN_BUS_FLAG_ERR, #IO_CAN_BUS_FLAG_FDF and
 * #IO_CAN_BUS_FLAG_BRS.
 */
int io_can_ctrl_get_flags(const io_can_ctrl_t *ctrl);

void *io_can_chan_alloc(void);
void io_can_chan_free(void *ptr);
io_can_chan_t *io_can_chan_init(io_can_chan_t *chan, io_poll_t *poll,
		ev_exec_t *exec, size_t rxlen, int wait_confirm);
void io_can_chan_fini(io_can_chan_t *chan);

/**
 * Creates a new CAN channel.
 *
 * @param poll   a pointer to the I/O polling instance used to monitor CAN bus
 *               events. If NULL, I/O operations MAY cause the event loop to
 *               block.
 * @param exec   a pointer to the executor used to execute asynchronous tasks.
 * @param rxlen  the receive queue length (in number of frames) of the CAN
 *               channel. If <b>rxlen</b> is 0, the default value
 *               #LELY_IO_CAN_RXLEN is used.
 * @param twxait a flag indicating whether the channel should wait for a write
 *               confirmation before sending the next CAN frame.
 *
 * @returns a pointer to a new CAN channel, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_can_chan_t *io_can_chan_create(
		io_poll_t *poll, ev_exec_t *exec, size_t rxlen, int txwait);

/// Destroys a CAN channel. @see io_can_chan_create()
void io_can_chan_destroy(io_can_chan_t *chan);

/**
 * Returns the SocketCAN file descriptor associated with a CAN channel, or -1 if
 * the channel is closed.
 */
int io_can_chan_get_handle(const io_can_chan_t *chan);

/**
 * Opens a CAN channel. If the channel was already open, it is first closed as
 * if by io_can_chan_close().
 *
 * @param chan  a pointer to a CAN channel.
 * @param ctrl  a pointer to the a controller representing a SocketCAN network
 *              interface.
 * @param flags the flags specifying which CAN bus features MUST be enabled (any
 *              combination of #IO_CAN_BUS_FLAG_ERR, #IO_CAN_BUS_FLAG_FDF and
 *              #IO_CAN_BUS_FLAG_BRS).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post on success, io_can_chan_is_open() returns 1.
 */
int io_can_chan_open(io_can_chan_t *chan, const io_can_ctrl_t *ctrl, int flags);

/**
 * Assigns an existing SocketCAN file descriptor to a CAN channel. Before being
 * assigned, the file descriptor will be modified in the following way:
 * - if the channel was created with the use of write confirmations enabled,
 *   reception of CAN frames sent by the socket is enabled with the
 *   `CAN_RAW_LOOPBACK` and `CAN_RAW_RECV_OWN_MSGS` socket options, and
 * - the size of the kernel send buffer is set to its minimum value.
 *
 * If the channel was already open, it is first closed as if by
 * io_can_chan_close().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post on success, io_can_chan_is_open() returns 1.
 */
int io_can_chan_assign(io_can_chan_t *chan, int fd);

/**
 * Dissociates and returns the SocketCAN file descriptor from a CAN channel. Any
 * pending read or write operations are canceled as if by
 * io_can_chan_cancel_read() and io_can_chan_cancel_write().
 *
 * @returns a file descriptor, or -1 if the channel was closed.
 *
 * @post io_can_chan_is_open() returns 0.
 */
int io_can_chan_release(io_can_chan_t *chan);

/**
 * Returns 1 if the CAN channel is open and 0 if not. This function is
 * equivalent to `io_can_chan_get_handle(chan) != -1`.
 */
int io_can_chan_is_open(const io_can_chan_t *chan);

/**
 * Closes the SocketCAN file descriptor associated with a CAN channel. Any
 * pending read or write operations are canceled as if by
 * io_can_chan_cancel_read() and io_can_chan_cancel_write().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc(). Note that the file descriptor is closed even
 * when this function reports error.
 *
 * @post io_can_chan_is_open() returns 0.
 */
int io_can_chan_close(io_can_chan_t *chan);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_LINUX_CAN_H_
