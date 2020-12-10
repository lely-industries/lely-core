/**@file
 * This header file is part of the CANopen library; it contains the
 * synchronization (SYNC) object declarations.
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#ifndef LELY_CO_SYNC_H_
#define LELY_CO_SYNC_H_

#include <lely/can/net.h>
#include <lely/co/type.h>

/// The bit in the SYNC COB-ID specifying whether the device is a producer.
#define CO_SYNC_COBID_PRODUCER UINT32_C(0x40000000)

/**
 * The bit in the SYNC COB-ID specifying whether to use an 11-bit (0) or 29-bit
 * (1) CAN-ID.
 */
#define CO_SYNC_COBID_FRAME UINT32_C(0x20000000)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen SYNC indication function, invoked after a SYNC message
 * is received or transmitted.
 *
 * @param sync a pointer to a SYNC consumer service.
 * @param cnt  the counter.
 * @param data a pointer to user-specified data.
 */
typedef void co_sync_ind_t(co_sync_t *sync, co_unsigned8_t cnt, void *data);

/**
 * The type of a CANopen SYNC error handling function, invoked when the SYNC
 * data length does not match.
 *
 * @param sync a pointer to a SYNC consumer service.
 * @param eec  the emergency error code (0x8240).
 * @param er   the error register (0x10).
 * @param data a pointer to user-specified data.
 */
typedef void co_sync_err_t(co_sync_t *sync, co_unsigned16_t eec,
		co_unsigned8_t er, void *data);

void *__co_sync_alloc(void);
void __co_sync_free(void *ptr);
struct __co_sync *__co_sync_init(
		struct __co_sync *sync, can_net_t *net, co_dev_t *dev);
void __co_sync_fini(struct __co_sync *sync);

/**
 * Creates a new CANopen SYNC producer/consumer service. The service is started
 * as if by co_sync_start().
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device.
 *
 * @returns a pointer to a new SYNC service, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see co_sync_destroy()
 */
co_sync_t *co_sync_create(can_net_t *net, co_dev_t *dev);

/// Destroys a CANopen SYNC producer/consumer service. @see co_sync_create()
void co_sync_destroy(co_sync_t *sync);

/**
 * Starts a SYNC service.
 *
 * @post on success, co_sync_is_stopped() returns 0.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_sync_stop()
 */
int co_sync_start(co_sync_t *sync);

/**
 * Stops a SYNC service.
 *
 * @post co_sync_is_stopped() returns 1.
 *
 * @see co_sync_start()
 */
void co_sync_stop(co_sync_t *sync);

/**
 * Retuns 1 if the specified SYNC service is stopped, and 0 if not.
 *
 * @see co_sync_start, co_sync_stop()
 */
int co_sync_is_stopped(const co_sync_t *sync);

/// Returns a pointer to the CAN network of a SYNC producer/consumer service.
can_net_t *co_sync_get_net(const co_sync_t *sync);

/// Returns a pointer to the CANopen device of a SYNC producer/consumer service.
co_dev_t *co_sync_get_dev(const co_sync_t *sync);

/**
 * Retrieves the indication function invoked after a CANopen SYNC message is
 * received or transmitted.
 *
 * @param sync  a pointer to a SYNC consumer service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_sync_set_ind()
 */
void co_sync_get_ind(const co_sync_t *sync, co_sync_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked after a CANopen SYNC message is received
 * or transmitted.
 *
 * @param sync a pointer to a SYNC consumer service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_sync_get_ind()
 */
void co_sync_set_ind(co_sync_t *sync, co_sync_ind_t *ind, void *data);

/**
 * Retrieves the error handling function of a SYNC consumer service.
 *
 * @param sync  a pointer to a SYNC consumer service.
 * @param perr  the address at which to store a pointer to the error handling
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_sync_set_err()
 */
void co_sync_get_err(const co_sync_t *sync, co_sync_err_t **perr, void **pdata);

/**
 * Sets the error handling function of a SYNC consumer service.
 *
 * @param sync a pointer to a SYNC consumer service.
 * @param err  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>err</b>.
 *
 * @see co_sync_get_err()
 */
void co_sync_set_err(co_sync_t *sync, co_sync_err_t *err, void *data);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_SYNC_H_
