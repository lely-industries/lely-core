/**@file
 * This header file is part of the I/O library; it contains the IXXAT CAN bus
 * declarations for Windows.
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

#ifndef LELY_IO2_WIN32_IXXAT_H_
#define LELY_IO2_WIN32_IXXAT_H_

#include <lely/io2/can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Loads the "vcinpl.dll" or "vcinpl2.dll" library and makes the IXXAT functions
 * available for use. This function is not thread-safe, but can be invoked
 * multiple times, as long as it is matched by an equal number of calls to
 * io_ixxat_fini().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_ixxat_init(void);

/**
 * Frees the "vcinpl.dll" or "vcinpl2.dll" library and terminates the
 * availability of the IXXAT functions. Note that this function MUST be invoked
 * once for each call to io_ixxat_init(). Only the last invocation will free the
 * library.
 */
void io_ixxat_fini(void);

void *io_ixxat_ctrl_alloc(void);
void io_ixxat_ctrl_free(void *ptr);
io_can_ctrl_t *io_ixxat_ctrl_init(io_can_ctrl_t *ctrl, const LUID *lpLuid,
		UINT32 dwCanNo, int flags, int nominal, int data);
void io_ixxat_ctrl_fini(io_can_ctrl_t *ctrl);

/**
 * Creates a new IXXAT CAN controller from a device index.
 *
 * @param dwIndex the index of the device in the list of fieldbus adapters
 *                registered with the VCI server.
 * @param dwCanNo the number of the CAN connection of the control unit to be
 *                opened.
 * @param flags   the flags specifying which CAN bus features MUST be enabled
 *                (any combination of #IO_CAN_BUS_FLAG_ERR, #IO_CAN_BUS_FLAG_FDF
 *                and #IO_CAN_BUS_FLAG_BRS).
 * @param nominal the nominal bitrate. For the CAN FD protocol, this is the bit
 *                rate of the arbitration phase.
 * @param data    the data bitrate. This bit rate is only defined for the CAN FD
 *                protocol; the value is ignored otherwise.
 *
 * @returns a pointer to a new CAN controller, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_ixxat_ctrl_create_from_luid()
 */
io_can_ctrl_t *io_ixxat_ctrl_create_from_index(UINT32 dwIndex, UINT32 dwCanNo,
		int flags, int nominal, int data);

/**
 * Creates a new IXXAT CAN controller from a locally unique identifier (LUID).
 *
 * @param lpLuid  a pointer to the locally unique identifier of the VCI device.
 * @param dwCanNo the number of the CAN connection of the control unit to be
 *                opened.
 * @param flags   the flags specifying which CAN bus features MUST be enabled
 *                (any combination of #IO_CAN_BUS_FLAG_ERR, #IO_CAN_BUS_FLAG_FDF
 *                and #IO_CAN_BUS_FLAG_BRS).
 * @param nominal the nominal bitrate. For the CAN FD protocol, this is the bit
 *                rate of the arbitration phase.
 * @param data    the data bitrate. This bit rate is only defined for the CAN FD
 *                protocol; the value is ignored otherwise.
 *
 * @returns a pointer to a new CAN controller, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see io_ixxat_ctrl_create_from_index()
 */
io_can_ctrl_t *io_ixxat_ctrl_create_from_luid(const LUID *lpLuid,
		UINT32 dwCanNo, int flags, int nominal, int data);

/// Destroys an IXXAT CAN controller. @see io_ixxat_ctrl_create())
void io_ixxat_ctrl_destroy(io_can_ctrl_t *ctrl);

/// Returns the native handle of the CAN controller.
HANDLE io_ixxat_ctrl_get_handle(const io_can_ctrl_t *ctrl);

void *io_ixxat_chan_alloc(void);
void io_ixxat_chan_free(void *ptr);
io_can_chan_t *io_ixxat_chan_init(io_can_chan_t *chan, io_ctx_t *ctx,
		ev_exec_t *exec, int rxtimeo, int txtimeo);
void io_ixxat_chan_fini(io_can_chan_t *chan);

/**
 * Creates a new IXXAT CAN channel.
 *
 * @param ctx     a pointer to the I/O context with which the channel should be
 *                registered.
 * @param exec    a pointer to the executor used to execute asynchronous tasks.
 *                These tasks including blocking read and write functions, the
 *                timeout of which can be specified with  <b>rxtimeo</b> and
 *                <b>txtimeo</b>, respectively.
 * @param rxtimeo the timeout (in milliseconds) when waiting asynchronously for
 *                a CAN message to be read. If <b>rxtimeo</b> is 0, the default
 *                value #LELY_IO_RX_TIMEOUT is used. If <b>rxtimeo</b> is
 *                negative, the read function will wait indefinitely.
 * @param txtimeo the timeout (in milliseconds) when waiting asynchronously for
 *                a CAN message to be written. If <b>txtimeo</b> is 0, the
 *                default value #LELY_IO_TX_TIMEOUT is used. If <b>txtimeo</b>
 *                is negative, the write function will wait indefinitely.
 *
 * @returns a pointer to a new CAN channel, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_can_chan_t *io_ixxat_chan_create(
		io_ctx_t *ctx, ev_exec_t *exec, int rxtimeo, int txtimeo);

/// Destroys an IXXAT CAN channel. @see io_ixxat_chan_create()
void io_ixxat_chan_destroy(io_can_chan_t *chan);

/**
 * Returns the native handle of the CAN channel, or NULL if the channel is
 * closed.
 */
HANDLE io_ixxat_chan_get_handle(const io_can_chan_t *chan);

/**
 * Opens a CAN channel. If the channel was already open, it is first closed as
 * if by io_ixxat_chan_close().
 *
 * @param chan        a pointer to an IXXAT CAN channel.
 * @param ctrl        a pointer to an IXXAT CAN controller.
 * @param wRxFifoSize the size of the receive buffer (in number of frames). If
 *                    <b>wRxFifoSize</b> is 0, the default value
 *                    #LELY_IO_IXXAT_RX_FIFO_SIZE is used.
 * @param wTxFifoSize the size of the transmit buffer (in number of frames). If
 *                    <b>wTxFifoSize</b> is 0, the default value
 *                    #LELY_IO_IXXAT_TX_FIFO_SIZE is used.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post on success, io_ixxat_chan_is_open() returns 1.
 */
int io_ixxat_chan_open(io_can_chan_t *chan, const io_can_ctrl_t *ctrl,
		UINT16 wRxFifoSize, UINT16 wTxFifoSize);

/**
 * Assigns an existing handle to a CAN channel, and activates the channel if
 * necessary. If the channel was already open, it is first closed as if by
 * io_ixxat_chan_close().
 *
 * @param chan         a pointer to an IXXAT CAN channel.
 * @param hCanChn      a handle to a native CAN channel.
 * @param dwTscClkFreq the clock frequency of the time stamp counter (in Hz).
 * @param dwTscDivisor the divisor for the message time stamp counter.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post on success, io_ixxat_chan_is_open() returns 1.
 */
int io_ixxat_chan_assign(io_can_chan_t *chan, HANDLE hCanChn,
		UINT32 dwTscClkFreq, UINT32 dwTscDivisor);

/**
 * Dissociates and returns the native handle from a CAN channel. Any pending
 * read or write operations are canceled as if by io_can_chan_cancel_read() and
 * io_can_chan_cancel_write().
 *
 * @returns a file descriptor, or -1 if the channel was closed.
 *
 * @post io_ixxat_chan_is_open() returns 0.
 */
HANDLE io_ixxat_chan_release(io_can_chan_t *chan);

/**
 * Returns 1 is the CAN channel is open and 0 if not. This function is
 * equivalent to `io_ixxat_chan_get_handle(chan) != NULL`.
 */
int io_ixxat_chan_is_open(const io_can_chan_t *chan);

/**
 * Closes a CAN channel. Any pending read or write operations are canceled as if
 * by io_can_chan_cancel_read() and io_can_chan_cancel_write().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @post io_ixxat_chan_is_open() returns 0.
 */
int io_ixxat_chan_close(io_can_chan_t *chan);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_WIN32_IXXAT_H_
