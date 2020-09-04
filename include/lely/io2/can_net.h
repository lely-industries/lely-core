/**@file
 * This header file is part of the I/O library; it contains the CAN network
 * interface declarations.
 *
 * This interface couples a timer and CAN channel to the internal CAN network
 * interface of the CAN library. It provides notifications of CAN bus state
 * changes and I/O errors through user-defined callbacks. CAN frames sent
 * through this interface are first put in a user-space transmit queue before
 * being sent to the underlying CAN channel.
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_IO2_CAN_NET_H_
#define LELY_IO2_CAN_NET_H_

#include <lely/io2/can.h>
#include <lely/io2/tqueue.h>

/// A CAN network interface.
typedef struct io_can_net io_can_net_t;

// Avoid including <lely/can/net.h>.
struct __can_net;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of function invoked when an error occurs during a CAN network
 * interface operations, or when the operation completes successfully after one
 * or more errors. The default implementation prints a warning or informational
 * message with diag().
 *
 * @param errc   the error code (0 on success).
 * @param errcnt the number of errors since the last successful operation.
 * @param arg    the user-specifed argument.
 */
typedef void io_can_net_on_error_func_t(int errc, size_t errcnt, void *arg);

/**
 * The type of function invoked when a CAN bus state change is detected by a CAN
 * network interface. The state is represented by one the #CAN_STATE_ACTIVE,
 * #CAN_STATE_PASSIVE, #CAN_STATE_BUSOFF, #CAN_STATE_SLEEPING or
 * #CAN_STATE_STOPPED values. The default implementation prints a warning or
 * informational message with diag().
 *
 * The mutex protecting the CAN network interface will be locked when this
 * function is called.
 *
 * @param new_state the current state of the CAN bus (#CAN_STATE_ACTIVE,
 *                  #CAN_STATE_PASSIVE or #CAN_STATE_BUSOFF).
 * @param old_state the previous state of the CAN bus (#CAN_STATE_ACTIVE,
 *                  #CAN_STATE_PASSIVE or #CAN_STATE_BUSOFF).
 * @param arg       the user-specifed argument.
 */
typedef void io_can_net_on_can_state_func_t(
		int new_state, int old_state, void *arg);

/**
 * The type of function invoked when a CAN bus error is detected by an CAN
 * network interface. The default implementation prints a warning with diag().
 *
 * The mutex protecting the CAN network interface will be locked when this
 * function is called.
 *
 * @param error the detected errors (any combination of #CAN_ERROR_BIT,
 *              #CAN_ERROR_STUFF, #CAN_ERROR_CRC, #CAN_ERROR_FORM,
 *              #CAN_ERROR_ACK and #CAN_ERROR_OTHER).
 * @param arg   the user-specifed argument.
 */
typedef void io_can_net_on_can_error_func_t(int error, void *arg);

void *io_can_net_alloc(void);
void io_can_net_free(void *ptr);
io_can_net_t *io_can_net_init(io_can_net_t *net, ev_exec_t *exec,
		io_timer_t *timer, io_can_chan_t *chan, size_t txlen,
		int txtimeo);
void io_can_net_fini(io_can_net_t *net);

/**
 * Creates a new CAN network interface.
 *
 * @param exec    a pointer to the executor used to execute asynchronous tasks.
 *                If <b>exec</b> is NULL, the CAN channel executor is used.
 * @param timer   a pointer to a timer. This timer MUST NOT be used for any
 *                other purpose.
 * @param chan    a pointer to a CAN channel. This channel MUST NOT be used for
 *                any other purpose.
 * @param txlen   the length (in number of frames) of the user-space transmit
 *                queue length. If <b>txlen</b> is 0, the default value
 *                #LELY_IO_CAN_NET_TXLEN is used.
 * @param txtimeo the timeout (in milliseconds) when waiting for a CAN frame
 *                write confirmation. If <b>txtimeo</b> is 0, the default value
 *                #LELY_IO_CAN_CTX_TXTIMEO is used. If <b>txtimeo</b> is
 *                negative. the timeout is disabled.
 *
 * @returns a pointer to a new CAN network interface, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
io_can_net_t *io_can_net_create(ev_exec_t *exec, io_timer_t *timer,
		io_can_chan_t *chan, size_t txlen, int txtimeo);

/// Destroys a CAN network interface.
void io_can_net_destroy(io_can_net_t *net);

/**
 * Starts a CAN network interface and begins processing CAN frames.
 *
 * This function locks the mutex protecting the CAN network interface.
 */
void io_can_net_start(io_can_net_t *net);

/**
 * Returns a pointer to the I/O context with which the CAN network interface is
 * registered.
 */
io_ctx_t *io_can_net_get_ctx(const io_can_net_t *net);

/**
 * Returns a pointer to the executor used by the CAN network interface to
 * execute asynchronous tasks.
 */
ev_exec_t *io_can_net_get_exec(const io_can_net_t *net);

/// Returns a pointer to the clock used by the CAN network interface.
io_clock_t *io_can_net_get_clock(const io_can_net_t *net);

/// Returns a pointer to the internal timer queue of a CAN network interface.
io_tqueue_t *io_can_net_get_tqueue(const io_can_net_t *net);

/**
 * Retrieves the function invoked when a new CAN frame read error occurs, or
 * when a read operation completes successfully after one or more errors.
 *
 * @param net   a pointer to a CAN network interface.
 * @param pfunc the address at which to store a pointer to the function (can be
 *              NULL).
 * @param parg  the address at which to store the user-specified argument (can
 *              be NULL).
 *
 * @see io_can_net_set_on_read_error_func()
 */
void io_can_net_get_on_read_error_func(const io_can_net_t *net,
		io_can_net_on_error_func_t **pfunc, void **parg);

/**
 * Sets the function invoked when a new CAN frame read error occurs, or when a
 * read operation completes successfully after one or more errors.
 *
 * @param net  a pointer to a CAN network interface.
 * @param func a pointer to the function to be invoked. If <b>func</b> is NULL,
 *             the default implementation will be used.
 * @param arg  the user-specified argument (can be NULL). <b>arg</b> is passed
 *             as the last argument to <b>func</b>.
 *
 * @see io_can_net_get_on_read_error_func()
 */
void io_can_net_set_on_read_error_func(
		io_can_net_t *net, io_can_net_on_error_func_t *func, void *arg);

/**
 * Retrieves the function invoked when a CAN frame is dropped because the
 * transmit queue is full, or when a frame is successfully queued after one or
 * more errors.
 *
 * @param net   a pointer to a CAN network interface.
 * @param pfunc the address at which to store a pointer to the function (can be
 *              NULL).
 * @param parg  the address at which to store the user-specified argument (can
 *              be NULL).
 *
 * @see io_can_net_set_on_queue_error_func()
 */
void io_can_net_get_on_queue_error_func(const io_can_net_t *net,
		io_can_net_on_error_func_t **pfunc, void **parg);

/**
 * Sets the function invoked when a CAN frame is dropped because the transmit
 * queue is full, or when a frame is successfully queued after one or more
 * errors.
 *
 * @param net  a pointer to a CAN network interface.
 * @param func a pointer to the function to be invoked. If <b>func</b> is NULL,
 *             the default implementation will be used.
 * @param arg  the user-specified argument (can be NULL). <b>arg</b> is passed
 *             as the last argument to <b>func</b>.
 *
 * @see io_can_net_get_on_queue_error_func()
 */
void io_can_net_set_on_queue_error_func(
		io_can_net_t *net, io_can_net_on_error_func_t *func, void *arg);

/**
 * Retrieves the function invoked when a new CAN frame write error occurs, or
 * when a write operation completes successfully after one or more errors.
 *
 * @param net   a pointer to a CAN network interface.
 * @param pfunc the address at which to store a pointer to the function (can be
 *              NULL).
 * @param parg  the address at which to store the user-specified argument (can
 *              be NULL).
 *
 * @see io_can_net_set_on_write_error_func()
 */
void io_can_net_get_on_write_error_func(const io_can_net_t *net,
		io_can_net_on_error_func_t **pfunc, void **parg);

/**
 * Sets the function invoked when a new CAN frame write error occurs, or when a
 * write operation completes successfully after one or more errors.
 *
 * @param net  a pointer to a CAN network interface.
 * @param func a pointer to the function to be invoked. If <b>func</b> is NULL,
 *             the default implementation will be used.
 * @param arg  the user-specified argument (can be NULL). <b>arg</b> is passed
 *             as the last argument to <b>func</b>.
 *
 * @see io_can_net_get_on_write_error_func()
 */
void io_can_net_set_on_write_error_func(
		io_can_net_t *net, io_can_net_on_error_func_t *func, void *arg);

/**
 * Retrieves the function invoked when a CAN bus state change is detected.
 *
 * @param net   a pointer to a CAN network interface.
 * @param pfunc the address at which to store a pointer to the function (can be
 *              NULL).
 * @param parg  the address at which to store the user-specified argument (can
 *              be NULL).
 *
 * @see io_can_net_set_on_can_state_func()
 */
void io_can_net_get_on_can_state_func(const io_can_net_t *net,
		io_can_net_on_can_state_func_t **pfunc, void **parg);

/**
 * Sets the function to be invoked when a CAN bus state change is detected.
 *
 * @param net  a pointer to a CAN network interface.
 * @param func a pointer to the function to be invoked. If <b>func</b> is NULL,
 *             the default implementation will be used.
 * @param arg  the user-specified argument (can be NULL). <b>arg</b> is passed
 *             as the last argument to <b>func</b>.
 *
 * @see io_can_net_get_on_can_state_func()
 */
void io_can_net_set_on_can_state_func(io_can_net_t *net,
		io_can_net_on_can_state_func_t *func, void *arg);

/**
 * Retrieves the function invoked when a CAN bus error is detected.
 *
 * @param net   a pointer to a CAN network interface.
 * @param pfunc the address at which to store a pointer to the function (can be
 *              NULL).
 * @param parg  the address at which to store the user-specified argument (can
 *              be NULL).
 *
 * @see io_can_net_set_on_can_error_func()
 */
void io_can_net_get_on_can_error_func(const io_can_net_t *net,
		io_can_net_on_can_error_func_t **pfunc, void **parg);

/**
 * Sets the function to be invoked when a CAN bus error is detected.
 *
 * @param net  a pointer to a CAN network interface.
 * @param func a pointer to the function to be invoked. If <b>func</b> is NULL,
 *             the default implementation will be used.
 * @param arg  the user-specified argument (can be NULL). <b>arg</b> is passed
 *             as the last argument to <b>func</b>.
 *
 * @see io_can_net_get_on_can_error_func()
 */
void io_can_net_set_on_can_error_func(io_can_net_t *net,
		io_can_net_on_can_error_func_t *func, void *arg);

/**
 * Locks the mutex protecting the CAN network interface.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc();
 *
 * @see io_can_net_unlock()
 */
int io_can_net_lock(io_can_net_t *net);

/**
 * Unlocks the mutex protecting the CAN network interface.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc();
 *
 * @see io_can_net_lock()
 */
int io_can_net_unlock(io_can_net_t *net);

/**
 * Returns a pointer to the internal interface of a CAN network interface.
 *
 * The mutex protecting the CAN network interface MUST be locked when using the
 * internal interface.
 *
 * @see io_can_net_lock(), io_can_net_unlock()
 */
struct __can_net *io_can_net_get_net(const io_can_net_t *net);

/**
 * Updates the CAN network time.
 *
 * The mutex protecting the CAN network interface MUST be locked for the
 * duration of this call.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_can_net_lock(), io_can_net_unlock()
 */
int io_can_net_set_time(io_can_net_t *net);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_CAN_NET_H_
