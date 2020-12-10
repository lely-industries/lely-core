/**@file
 * This header file is part of the CANopen library; it contains the Server-SDO
 * declarations.
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

#ifndef LELY_CO_SSDO_H_
#define LELY_CO_SSDO_H_

#include <lely/can/net.h>
#include <lely/co/sdo.h>

#ifdef __cplusplus
extern "C" {
#endif

void *__co_ssdo_alloc(void);
void __co_ssdo_free(void *ptr);
struct __co_ssdo *__co_ssdo_init(struct __co_ssdo *sdo, can_net_t *net,
		co_dev_t *dev, co_unsigned8_t num);
void __co_ssdo_fini(struct __co_ssdo *sdo);

/**
 * Creates a new CANopen Server-SDO service. The service is started as if by
 * co_ssdo_start().
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device describing the server.
 * @param num the SDO number (in the range [1..128]). Except when <b>num</b> is
 *            1, the SDO parameter record MUST exist in the object dictionary of
 *            <b>dev</b>.
 *
 * @returns a pointer to a new Server-SDO service, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_ssdo_destroy()
 */
co_ssdo_t *co_ssdo_create(can_net_t *net, co_dev_t *dev, co_unsigned8_t num);

/// Destroys a CANopen Server-SDO service. @see co_ssdo_create()
void co_ssdo_destroy(co_ssdo_t *sdo);

/**
 * Starts a Server-SDO service.
 *
 * @post on success, co_ssdo_is_stopped() returns 0.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_ssdo_stop()
 */
int co_ssdo_start(co_ssdo_t *sdo);

/**
 * Stops a Server-SDO service. Any ongoing request is aborted.
 *
 * @post co_ssdo_is_stopped() returns 1.
 *
 * @see co_ssdo_start()
 */
void co_ssdo_stop(co_ssdo_t *sdo);

/**
 * Retuns 1 if the specified Server-SDO service is stopped, and 0 if not.
 *
 * @see co_ssdo_start, co_ssdo_stop()
 */
int co_ssdo_is_stopped(const co_ssdo_t *sdo);

/// Returns a pointer to the CAN network of a Server-SDO.
can_net_t *co_ssdo_get_net(const co_ssdo_t *sdo);

/// Returns a pointer to the CANopen device of a Server-SDO.
co_dev_t *co_ssdo_get_dev(const co_ssdo_t *sdo);

/// Returns the SDO number of a Server-SDO.
co_unsigned8_t co_ssdo_get_num(const co_ssdo_t *sdo);

/// Returns a pointer to the SDO parameter record of a Server-SDO.
const struct co_sdo_par *co_ssdo_get_par(const co_ssdo_t *sdo);

/**
 * Returns the timeout (in milliseconds) of a Server-SDO. A return value of 0
 * (the default) means no timeout is being used.
 *
 * @see co_ssdo_set_timeout()
 */
int co_ssdo_get_timeout(const co_ssdo_t *sdo);

/**
 * Sets the timeout of a Server-SDO. If the timeout expires before receiving a
 * request from a client, the server aborts the transfer.
 *
 * @param sdo     a pointer to a Server-SDO service.
 * @param timeout the timeout (in milliseconds). A value of 0 (the default)
 *                disables the timeout.
 *
 * @see co_ssdo_get_timeout()
 */
void co_ssdo_set_timeout(co_ssdo_t *sdo, int timeout);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_SSDO_H_
