/**@file
 * This header file is part of the CANopen library; it contains the time stamp
 * (TIME) object declarations.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_CO_TIME_H_
#define LELY_CO_TIME_H_

#include <lely/can/net.h>
#include <lely/co/type.h>

/// The bit in the TIME COB-ID specifying whether the device is a consumer.
#define CO_TIME_COBID_CONSUMER UINT32_C(0x80000000)

/// The bit in the TIME COB-ID specifying whether the device is a producer.
#define CO_TIME_COBID_PRODUCER UINT32_C(0x40000000)

/**
 * The bit in the TIME COB-ID specifying whether to use an 11-bit (0) or 29-bit
 * (1) CAN-ID.
 */
#define CO_TIME_COBID_FRAME UINT32_C(0x20000000)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen TIME indication function, invoked when a time stamp is
 * received.
 *
 * @param time a pointer to a TIME consumer service.
 * @param tp   a pointer to the absolute time.
 * @param data a pointer to user-specified data.
 */
typedef void co_time_ind_t(
		co_time_t *time, const struct timespec *tp, void *data);

/**
 * Loads the absolute time from a CANopen TIME_OF_DAY value.
 *
 * @see co_time_of_day_set()
 */
void co_time_of_day_get(const co_time_of_day_t *tod, struct timespec *tp);

/**
 * Stores the absolute time into a CANopen TIME_OF_DAY value.
 *
 * @see co_time_of_day_get()
 */
void co_time_of_day_set(co_time_of_day_t *tod, const struct timespec *tp);

/**
 * Loads a time difference from a CANopen TIME_DIFFERENCE value.
 *
 * @see co_time_diff_set()
 */
void co_time_diff_get(const co_time_diff_t *td, struct timespec *tp);

/**
 * Stores a time difference into a CANopen TIME_DIFFERENCE value.
 *
 * @see co_time_diff_get()
 */
void co_time_diff_set(co_time_diff_t *td, const struct timespec *tp);

void *__co_time_alloc(void);
void __co_time_free(void *ptr);
struct __co_time *__co_time_init(
		struct __co_time *time, can_net_t *net, co_dev_t *dev);
void __co_time_fini(struct __co_time *time);

/**
 * Creates a new CANopen TIME producer/consumer service. The service is started
 * as if by co_time_start().
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device.
 *
 * @returns a pointer to a new TIME service, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see co_time_destroy()
 */
co_time_t *co_time_create(can_net_t *net, co_dev_t *dev);

/// Destroys a CANopen TIME producer/consumer service. @see co_time_create()
void co_time_destroy(co_time_t *time);

/**
 * Starts a TIME service.
 *
 * @post on success, co_time_is_stopped() returns 0.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_time_stop()
 */
int co_time_start(co_time_t *time);

/**
 * Stops a TIME service. This function invokes co_time_stop_prod() to stop the
 * TIME producer, if necessary.
 *
 * @post co_time_is_stopped() returns 1.
 *
 * @see co_time_start()
 */
void co_time_stop(co_time_t *time);

/**
 * Retuns 1 if the specified TIME service is stopped, and 0 if not.
 *
 * @see co_time_start, co_time_stop()
 */
int co_time_is_stopped(const co_time_t *time);

/// Returns a pointer to the CAN network of a TIME producer/consumer service.
can_net_t *co_time_get_net(const co_time_t *time);

/// Returns a pointer to the CANopen device of a TIME producer/consumer service.
co_dev_t *co_time_get_dev(const co_time_t *time);

/**
 * Retrieves the indication function invoked when a CANopen time stamp is
 * received.
 *
 * @param time  a pointer to a TIME consumer service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_time_set_ind()
 */
void co_time_get_ind(const co_time_t *time, co_time_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a CANopen time stamp is received.
 *
 * @param time a pointer to a TIME consumer service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_time_get_ind()
 */
void co_time_set_ind(co_time_t *time, co_time_ind_t *ind, void *data);

/**
 * Starts a CANopen TIME producer. This function has no effect if the TIME
 * service is not a producer.
 *
 * @param time     a pointer to a TIME producer service.
 * @param start    a pointer to the _absolute_ time when the next time stamp is
 *                 to be sent. If <b>start</b> is NULL, this time is given by
 *                 <b>interval</b> with respect to the current time as obtained
 *                 with can_net_get_time(). If <b>interval</b> is also NULL, the
 *                 producer is stopped.
 * @param interval a pointer to the interval between successive time stamps. If
 *                 <b>interval</b> is NULL, only a single time stamp is sent, at
 *                 the time given by <b>start</b>.
 *
 * @see co_time_stop_prod()
 */
void co_time_start_prod(co_time_t *time, const struct timespec *start,
		const struct timespec *interval);

/// Stops a CANopen TIME producer. @see co_time_start_prod()
void co_time_stop_prod(co_time_t *time);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_TIME_H_
