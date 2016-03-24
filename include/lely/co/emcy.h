/*!\file
 * This header file is part of the CANopen library; it contains the emergency
 * (EMCY) object declarations.
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_CO_EMCY_H
#define LELY_CO_EMCY_H

#include <lely/can/net.h>
#include <lely/co/type.h>

//! The bit in the EMCY COB-ID specifying whether the EMCY exists and is valid.
#define CO_EMCY_COBID_VALID	UINT32_C(0x80000000)

/*!
 * The bit in the EMCY COB-ID specifying whether to use an 11-bit (0) or 29-bit
 * (1) CAN-ID.
 */
#define CO_EMCY_COBID_FRAME	UINT32_C(0x20000000)

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a CANopen EMCY indication function, invoked when an EMCY message
 * is received.
 *
 * \param emcy a pointer to an EMCY consumer service.
 * \param id   the Node-ID of the producer.
 * \param eec  the emergency error code.
 * \param er   the error register.
 * \param msef the manufacturer-specific error code.
 * \param data a pointer to user-specified data.
 */
typedef void co_emcy_ind_t(co_emcy_t *emcy, co_unsigned8_t id,
		co_unsigned16_t ec, co_unsigned8_t er, uint8_t msef[5],
		void *data);

LELY_CO_EXTERN void *__co_emcy_alloc(void);
LELY_CO_EXTERN void __co_emcy_free(void *ptr);
LELY_CO_EXTERN struct __co_emcy *__co_emcy_init(struct __co_emcy *emcy,
		can_net_t *net, co_dev_t *dev);
LELY_CO_EXTERN void __co_emcy_fini(struct __co_emcy *emcy);

/*!
 * Creates a new CANopen EMCY producer/consumer service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device.
 *
 * \returns a pointer to a new EMCY service, or NULL on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_emcy_destroy()
 */
LELY_CO_EXTERN co_emcy_t *co_emcy_create(can_net_t *net, co_dev_t *dev);

//! Destroys a CANopen EMCY producer/consumer service. \see co_emcy_create()
LELY_CO_EXTERN void co_emcy_destroy(co_emcy_t *emcy);

/*!
 * Pushes a CANopen EMCY message to the stack and broadcasts it if the EMCY
 * producer service is active.
 *
 * \param emcy a pointer to an EMCY producer service.
 * \param eec  the emergency error code.
 * \param er   the error register.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_emcy_pop(), co_emcy_peek(), co_emcy_clear()
 */
LELY_CO_EXTERN int co_emcy_push(co_emcy_t *emcy, co_unsigned16_t eec,
		co_unsigned8_t er);

/*!
 * Pops the most recent CANopen EMCY message from the stack and broadcasts an
 * 'error reset' message if the EMCY producer service is active.
 *
 * \param emcy a pointer to an EMCY producer service.
 * \param peec the address at which to store the emergency error code (can be
 *             NULL).
 * \param per  the address at which to store the error register (can be NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_emcy_push(), co_emcy_peek(), co_emcy_clear()
 */
LELY_CO_EXTERN int co_emcy_pop(co_emcy_t *emcy, co_unsigned16_t *peec,
		co_unsigned8_t *per);

/*!
 * Retrieves, but does not pop, the most recent CANopen EMCY message from the
 * stack.
 *
 * \param emcy a pointer to an EMCY producer service.
 * \param peec the address at which to store the emergency error code (can be
 *             NULL).
 * \param per  the address at which to store the error register (can be NULL).
 *
 * \see co_emcy_push(), co_emcy_pop(), co_emcy_clear()
 */
LELY_CO_EXTERN void co_emcy_peek(const co_emcy_t *emcy, co_unsigned16_t *peec,
		co_unsigned8_t *per);

/*!
 * Clears the CANopen EMCY message stack and broadcasts the 'error reset/no
 * error' message if the EMCY producer service is active.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_emcy_push(), co_emcy_pop(), co_emcy_peek()
 */
LELY_CO_EXTERN int co_emcy_clear(co_emcy_t *emcy);

/*!
 * Retrieves the indication function invoked when a CANopen EMCY message is
 * received.
 *
 * \param emcy  a pointer to an EMCY consumer service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_emcy_set_ind()
 */
LELY_CO_EXTERN void co_emcy_get_ind(const co_emcy_t *emcy, co_emcy_ind_t **pind,
		void **pdata);

/*!
 * Sets the indication function invoked when a CANopen EMCY message is received.
 *
 * \param emcy a pointer to an EMCY consumer service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a func.
 *
 * \see co_emcy_get_ind()
 */
LELY_CO_EXTERN void co_emcy_set_ind(co_emcy_t *emcy, co_emcy_ind_t *ind,
		void *data);

#ifdef __cplusplus
}
#endif

#endif

