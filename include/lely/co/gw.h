/*!\file
 * This header file is part of the CANopen library; it contains the gateway
 * declarations (see CiA 309-1 version 2.0).
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

#ifndef LELY_CO_GW_H
#define LELY_CO_GW_H

#include <lely/co/type.h>

//! The maximum number of networks in a CANopen gateway.
#define CO_GW_NUM_NET	127

//! CANopen gateway internal error: Request not supported.
#define CO_GW_IEC_BAD_SRV	100

//! CANopen gateway internal error: Syntax error.
#define CO_GW_IEC_SYNTAX	101

//! CANopen gateway internal error: Request not processed due to internal state.
#define CO_GW_IEC_INTERN	102

//! CANopen gateway internal error: Time-out.
#define CO_GW_IEC_TIMEOUT	103

//! CANopen gateway internal error: No default net set.
#define CO_GW_IEC_NO_DEF_NET	104

//! CANopen gateway internal error: No default node set.
#define CO_GW_IEC_NO_DEF_NODE	105

//! CANopen gateway internal error: Unsupported net.
#define CO_GW_IEC_BAD_NET	106

//! CANopen gateway internal error: Unsupported node.
#define CO_GW_IEC_BAD_NODE	107

//! CANopen gateway internal error: Lost guarding message.
#define CO_GW_IEC_NG_OCCURRED	200

//! CANopen gateway internal error: Lost connection.
#define CO_GW_IEC_LG_OCCURRED	201

//! CANopen gateway internal error: Heartbeat started.
#define CO_GW_IEC_HB_RESOLVED	202

//! CANopen gateway internal error: Heartbeat lost.
#define CO_GW_IEC_HB_OCCURRED	203

//! CANopen gateway internal error: Wrong NMT state.
#define CO_GW_IEC_ST_OCCURRED	204

//! CANopen gateway internal error: Boot-up.
#define CO_GW_IEC_BOOTUP	205

//! CANopen gateway internal error: Error passive.
#define CO_GW_IEC_CAN_PASSIVE	300

//! CANopen gateway internal error: Bus off.
#define CO_GW_IEC_CAN_BUSOFF	301

//! CANopen gateway internal error: CAN buffer overflow.
#define CO_GW_IEC_CAN_OVERFLOW	303

//! CANopen gateway internal error: CAN init.
#define CO_GW_IEC_CAN_INIT	304

//! CANopen gateway internal error: CAN active.
#define CO_GW_IEC_CAN_ACTIVE	305

//! CANopen gateway internal error: PDO already used.
#define CO_GW_IEC_PDO_INUSE	400

//! CANopen gateway internal error: PDO length exceeded.
#define CO_GW_IEC_PDO_LEN	401

//! CANopen gateway internal error: LSS error.
#define CO_GW_IEC_LSS	501

//! CANopen gateway internal error: LSS node-ID not supported.
#define CO_GW_IEC_LSS_ID	502

//! CANopen gateway internal error: LSS bit-rate not supported.
#define CO_GW_IEC_LSS_RATE	503

//! CANopen gateway internal error: LSS parameter storing failed.
#define CO_GW_IEC_LSS_PARAM	504

//! CANopen gateway internal error: LSS command failed because of media error.
#define CO_GW_IEC_LSS_MEDIA	505

//! CANopen gateway internal error: Running out of memory.
#define CO_GW_IEC_NO_MEM	600

struct __co_gw;
#ifndef __cplusplus
//! An opaque CANopen gateway type.
typedef struct __co_gw co_gw_t;
#endif

//! The common parameters of a CANopen gateway service.
struct co_gw_srv {
	//! The size of this struct (in bytes).
	size_t size;
	//! The service number.
	int srv;
};

//! The common parameters of a CANopen gateway request.
struct co_gw_req {
	//! The size of this struct (in bytes).
	size_t size;
	//! The service number.
	int srv;
	//! A pointer to user-specified data.
	void *data;
};

//! The common parameters of a CANopen gateway confirmation.
struct co_gw_con {
	//! The size of this struct (in bytes).
	size_t size;
	//! The service number.
	int srv;
	//! A pointer to user-specified data.
	void *data;
	//! The internal error code (0 on success).
	int iec;
	//! The SDO abort code (0 on success).
	co_unsigned32_t ac;
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a CANopen gateway send callback function, invoked by a gateway
 * when an indication or confirmation needs to be sent.
 *
 * \param srv  a pointer to the service parameters to be sent.
 * \param data a pointer to user-specified data.
 *
 * \returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with `set_errnum()`.
 */
typedef int co_gw_send_func_t(const struct co_gw_srv *srv, void *data);

//! Returns a string describing an internal error code.
LELY_CO_EXTERN const char *co_gw_iec2str(int iec);

LELY_CO_EXTERN void *__co_gw_alloc(void);
LELY_CO_EXTERN void __co_gw_free(void *ptr);
LELY_CO_EXTERN struct __co_gw *__co_gw_init(struct __co_gw *gw);
LELY_CO_EXTERN void __co_gw_fini(struct __co_gw *gw);

//! Creates a new CANopen gateway. \see co_gw_destroy()
LELY_CO_EXTERN co_gw_t *co_gw_create(void);

//! Destroys a CANopen gateway. \see co_gw_create()
LELY_CO_EXTERN void co_gw_destroy(co_gw_t *gw);

/*!
 * Registers a CANopen network with a gateway.
 *
 * \param gw  a pointer to a CANopen gateway.
 * \param id  the network-ID (in the range [1..127]).
 * \param nmt a pointer to the NMT service of the gateway node in the network.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_gw_fini_net()
 */
LELY_CO_EXTERN int co_gw_init_net(co_gw_t *gw, co_unsigned16_t id,
		co_nmt_t *nmt);

/*!
 * Unregisters a CANopen network with a gateway.
 *
 * \param gw a pointer to a CANopen gateway.
 * \param id the network-ID (in the range [1..127]).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_gw_init_net()
 */
LELY_CO_EXTERN int co_gw_fini_net(co_gw_t *gw, co_unsigned16_t id);

/*!
 * Receives and processes a request with a CANopen gateway.
 *
 * \param gw  a pointer to a CANopen gateway.
 * \param req a pointer to the parameters of the request.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_gw_recv(co_gw_t *gw, const struct co_gw_req *req);

/*!
 * Retrieves the callback function used to send indications and confirmations
 * from a CANopen gateway.
 *
 * \param gw   a pointer to a CANopen gateway.
 * \param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_gw_set_send_func()
 */
LELY_CO_EXTERN void co_gw_get_send_func(const co_gw_t *gw,
		co_gw_send_func_t **pfunc, void **pdata);

/*!
 * Sets the callback function used to send indications and confirmations from a
 * CANopen gateway.
 *
 * \param gw   a pointer to a CANopen gateway.
 * \param func a pointer to the function to be invoked when an indication or
 *             confirmation needs to be sent.
 * \param data a pointer to user-specified data (can be NULL). \a data is passed
 *             as the last parameter to \a func.
 *
 * \see co_gw_get_send_func()
 */
LELY_CO_EXTERN void co_gw_set_send_func(co_gw_t *gw, co_gw_send_func_t *func,
		void *data);

#ifdef __cplusplus
}
#endif

#endif

