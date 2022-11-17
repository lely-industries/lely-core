/**@file
 * This header file is part of the CAN library; it contains the CAN network
 * interface declarations.
 *
 * @copyright 2015-2022 Lely Industries N.V.
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

#ifndef LELY_CAN_NET_H_
#define LELY_CAN_NET_H_

#include <lely/can/msg.h>
#include <lely/libc/time.h>

struct __can_net;
#if !defined(__cplusplus) || LELY_NO_CXX
/// An opaque CAN network interface type.
typedef struct __can_net can_net_t;
#endif

struct __can_timer;
#if !defined(__cplusplus) || LELY_NO_CXX
/// An opaque CAN timer type.
typedef struct __can_timer can_timer_t;
#endif

struct __can_recv;
#if !defined(__cplusplus) || LELY_NO_CXX
/// An opaque CAN frame receiver type.
typedef struct __can_recv can_recv_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CAN timer callback function, invoked by a CAN timer when the
 * time is updated, or by a CAN network interface when the time at which the
 * next timer triggers is updated.
 *
 * @param tp   a pointer to the current time.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with `set_errnum()`.
 */
typedef int can_timer_func_t(const struct timespec *tp, void *data);

/**
 * The type of a CAN receive callback function, invoked by a CAN frame receiver
 * when a frame is received.
 *
 * @param msg  a pointer to the received CAN frame.
 * @param data a pointer to user-specified data.
 *
 * @returns a non-negative value on success, or -1 on error. In the latter case,
 * implementations SHOULD set the error number with `set_errnum()`.
 * Implementations MUST return a positive value if the callback MAY have
 * modified the CAN frame receiver list, for example by unregistering itself
 * with can_recv_stop().
 */
typedef int can_recv_func_t(const struct can_msg *msg, void *data);

/**
 * The type of a CAN send callback function, invoked by a CAN network interface
 * when a frame needs to be sent.
 *
 * @param msg  a pointer to the CAN frame to be sent.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with `set_errnum()`.
 */
typedef int can_send_func_t(const struct can_msg *msg, void *data);

void *__can_net_alloc(void);
void __can_net_free(void *ptr);
struct __can_net *__can_net_init(struct __can_net *net);
void __can_net_fini(struct __can_net *net);

/// Creates a new CAN network interface. @see can_net_destroy()
can_net_t *can_net_create(void);

/// Destroys a CAN network interface. @see can_net_create()
void can_net_destroy(can_net_t *net);

/**
 * Retrieves the current time of a CAN network interface.
 *
 * @param net a pointer to a CAN network interface.
 * @param tp  the address at which to store the current time (can be NULL).
 *
 * @see can_net_set_time()
 */
void can_net_get_time(const can_net_t *net, struct timespec *tp);

/**
 * Sets the current time of a CAN network interface. This MAY invoke one or more
 * CAN timer callback functions.
 *
 * @param net a pointer to a CAN network interface.
 * @param tp  a pointer to the current time.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * set by the first failed CAN timer callback function can be obtained with
 * get_errc().
 *
 * @see can_net_get_time()
 */
int can_net_set_time(can_net_t *net, const struct timespec *tp);

/**
 * Retrieves the callback function invoked when the time at which the next CAN
 * timer triggers is updated.
 *
 * @param net   a pointer to a CAN network interface.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see can_net_set_next_func()
 */
void can_net_get_next_func(
		const can_net_t *net, can_timer_func_t **pfunc, void **pdata);

/**
 * Sets the callback function invoked when the time at which the next CAN timer
 * triggers is updated.
 *
 * @param net  a pointer to a CAN network interface.
 * @param func a pointer to the function to be invoked by can_net_send().
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see can_net_get_next_func()
 */
void can_net_set_next_func(can_net_t *net, can_timer_func_t *func, void *data);

/**
 * Receives a CAN frame with a network interface and processes it with the
 * corresponding receiver(s). This function MAY invoke on or more CAN frame
 * receiver callback functions.
 *
 * @param net a pointer to a CAN network interface.
 * @param msg a pointer to the CAN frame to be processed.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * set by the first failed CAN frame receiver callback function can be obtained
 * with get_errc().
 */
int can_net_recv(can_net_t *net, const struct can_msg *msg);

/**
 * Sends a CAN frame from a network interface. This function invokes the
 * callback function set by can_net_set_send_func().
 *
 * @param net a pointer to a CAN network interface.
 * @param msg a pointer to the CAN frame to be sent.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * set by the CAN send callback function can be obtained with get_errc().
 */
int can_net_send(can_net_t *net, const struct can_msg *msg);

/**
 * Retrieves the callback function used to send CAN frames from a network
 * interface.
 *
 * @param net   a pointer to a CAN network interface.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see can_net_set_send_func()
 */
void can_net_get_send_func(
		const can_net_t *net, can_send_func_t **pfunc, void **pdata);

/**
 * Sets the callback function used to send CAN frames from a network interface.
 *
 * @param net  a pointer to a CAN network interface.
 * @param func a pointer to the function to be invoked by can_net_send().
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see can_net_get_send_func()
 */
void can_net_set_send_func(can_net_t *net, can_send_func_t *func, void *data);

void *__can_timer_alloc(void);
void __can_timer_free(void *ptr);
struct __can_timer *__can_timer_init(struct __can_timer *timer);
void __can_timer_fini(struct __can_timer *timer);

/// Creates a new CAN timer. @see can_timer_destroy()
can_timer_t *can_timer_create(void);

/// Destroys a CAN timer. @see can_timer_create()
void can_timer_destroy(can_timer_t *timer);

/**
 * Retrieves the callback function invoked when a CAN timer is triggered.
 *
 * @param timer a pointer to a CAN timer.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see can_timer_set_func()
 */
void can_timer_get_func(const can_timer_t *timer, can_timer_func_t **pfunc,
		void **pdata);

/**
 * Sets the callback function invoked when a CAN timer is triggered.
 *
 * @param timer a pointer to a CAN timer.
 * @param func  a pointer to the function to be invoked by can_net_set_time().
 * @param data  a pointer to user-specified data (can be NULL). <b>data</b> is
 *              passed as the last parameter to <b>func</b>.
 *
 * @see can_timer_get_func()
 */
void can_timer_set_func(can_timer_t *timer, can_timer_func_t *func, void *data);

/**
 * Starts a CAN timer and registers it with a network interface.
 *
 * @param timer    a pointer to a CAN timer.
 * @param net      a pointer to a CAN network interface.
 * @param start    a pointer to the _absolute_ time of the next trigger of the
 *                 timer. If <b>start</b> is NULL, the next trigger time is
 *                 given by <b>interval</b> with respect to the current time as
 *                 obtained with can_net_get_time(). If <b>interval</b> is also
 *                 NULL, the timer is stopped.
 * @param interval a pointer to the interval between successive triggers of the
 *                 timer. If <b>interval</b> is NULL, the timer only triggers
 *                 once, at the time given by <b>start</b>.
 *
 * @see can_timer_stop()
 */
void can_timer_start(can_timer_t *timer, can_net_t *net,
		const struct timespec *start, const struct timespec *interval);

/**
 * Stops a CAN timer and unregisters it with a network interface.
 *
 * @see can_timer_start()
 */
void can_timer_stop(can_timer_t *timer);

/**
 * Starts a CAN timer and registers it with a network interface. The timer will
 * trigger only once.
 *
 * @param timer   a pointer to a CAN timer.
 * @param net     a pointer to a CAN network interface.
 * @param timeout the timeout (in milliseconds) with respect to the current
 *                time as obtained with can_net_get_time(). If <b>timeout</b> is
 *                negative, the timer is stopped.
 *
 * @see can_timer_start(), can_timer_stop()
 */
void can_timer_timeout(can_timer_t *timer, can_net_t *net, int timeout);

void *__can_recv_alloc(void);
void __can_recv_free(void *ptr);
struct __can_recv *__can_recv_init(struct __can_recv *recv);
void __can_recv_fini(struct __can_recv *recv);

/// Creates a new CAN frame receiver. @see can_recv_destroy()
can_recv_t *can_recv_create(void);

/// Destroys a CAN frame receiver. @see can_recv_create()
void can_recv_destroy(can_recv_t *recv);

/**
 * Retrieves the callback function used to process CAN frames with a receiver.
 *
 * @param recv  a pointer to a CAN frame receiver.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see can_recv_set_func()
 */
void can_recv_get_func(
		const can_recv_t *recv, can_recv_func_t **pfunc, void **pdata);

/**
 * Sets the callback function used to process CAN frames with a receiver.
 *
 * @param recv a pointer to a CAN frame receiver.
 * @param func a pointer to the function to be invoked by can_net_recv().
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see can_recv_get_func()
 */
void can_recv_set_func(can_recv_t *recv, can_recv_func_t *func, void *data);

/**
 * Registers a CAN frame receiver with a network interface and starts processing
 * frames.
 *
 * @param recv  a pointer to a CAN frame receiver.
 * @param net   a pointer to a CAN network interface.
 * @param id    the CAN identifier for which the receiver should be invoked.
 * @param flags the flags that should be set in every accepted frame.
 *
 * @see can_recv_stop()
 */
void can_recv_start(can_recv_t *recv, can_net_t *net, uint_least32_t id,
		uint_least8_t flags);

/**
 * Stops a CAN frame receiver from processing frames and unregisters it with the
 * network interface.
 *
 * @see can_recv_start()
 */
void can_recv_stop(can_recv_t *recv);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CAN_NET_H_
