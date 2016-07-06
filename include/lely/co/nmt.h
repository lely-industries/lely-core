/*!\file
 * This header file is part of the CANopen library; it contains the network
 * management (NMT) declarations.
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

#ifndef LELY_CO_NMT_H
#define LELY_CO_NMT_H

#include <lely/can/net.h>
#include <lely/co/type.h>

//! The NMT command specifier 'start'.
#define CO_NMT_CS_START	0x01

//! The NMT command specifier 'stop'.
#define CO_NMT_CS_STOP	0x02

//! The NMT command specifier 'enter pre-operational'.
#define CO_NMT_CS_ENTER_PREOP	0x80

//! The NMT command specifier 'reset node'.
#define CO_NMT_CS_RESET_NODE	0x81

//! The NMT command specifier 'reset communication'.
#define CO_NMT_CS_RESET_COMM	0x82

//! The NMT state 'boot-up'.
#define CO_NMT_ST_BOOTUP	0x00

//! The NMT state 'stopped'.
#define CO_NMT_ST_STOP	0x04

//! The NMT state 'operational'.
#define CO_NMT_ST_START	0x05

//! The NMT sub-state 'reset application'.
#define CO_NMT_ST_RESET_NODE	0x06

//! The NMT sub-state 'reset communication'.
#define CO_NMT_ST_RESET_COMM	0x07

//! The NMT state 'pre-operational'.
#define CO_NMT_ST_PREOP	0x7f

//! The mask to get/set the toggle bit from an NMT state.
#define CO_NMT_ST_TOGGLE	0x80

enum {
	//! An NMT error control event occurred.
	CO_NMT_EC_OCCURRED,
	//! An NMT error control event was resolved.
	CO_NMT_EC_RESOLVED
};

enum {
	//! An NMT error control timeout event.
	CO_NMT_EC_TIMEOUT,
	//! An NMT error control state change event.
	CO_NMT_EC_STATE
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a CANopen NMT command indication function, invoked when an NMT
 * command is received (and _after_ the state switch has occurred).
 *
 * \param nmt  a pointer to an NMT master/slave service.
 * \param cs   the NMT command specifier (one of #CO_NMT_CS_START,
 *             #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *             #CO_NMT_CS_RESET_COMM).
 * \param data a pointer to user-specified data.
 */
typedef void co_nmt_cs_ind_t(co_nmt_t *nmt, co_unsigned8_t cs, void *data);

/*!
 * The type of a CANopen NMT error control indication function, invoked when a
 * node guarding event occurs. The default handler invokes
 * co_nmt_comm_err_ind().
 *
 * \param nmt    a pointer to an NMT master service.
 * \param id     the node-ID (in the range [1..127]).
 * \param state  indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *               was resolved (#CO_NMT_EC_RESOLVED).
 * \param reason indicates whether the event occurred because of a timeout
 *               (#CO_NMT_EC_TIMEOUT) or an unexpected state change
 *               (#CO_NMT_EC_STATE).
 * \param data   a pointer to user-specified data.
 */
typedef void co_nmt_ng_ind_t(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);

/*!
 * The type of a CANopen NMT error control indication function, invoked when a
 * life guarding event occurs. The default handler invokes
 * co_nmt_comm_err_ind().
 *
 * \param nmt   a pointer to an NMT master/slave service.
 * \param state indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *              was resolved (#CO_NMT_EC_RESOLVED).
 * \param data  a pointer to user-specified data.
 */
typedef void co_nmt_lg_ind_t(co_nmt_t *nmt, int state, void *data);

/*!
 * The type of a CANopen NMT error control indication function, invoked when a
 * heartbeat event occurs. The default handler invokes co_nmt_node_err_ind() or
 * co_nmt_comm_err_ind(), depending on whether the NMT service is a master or
 * not (see co_nmt_is_master()).
 *
 * \param nmt    a pointer to an NMT master/slave service.
 * \param id     the node-ID (in the range [1..127]).
 * \param state  indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *               was resolved (#CO_NMT_EC_RESOLVED). Note that heartbeat state
 *               change events only occur and are never resolved.
 * \param reason indicates whether the event occurred because of a timeout
 *               (#CO_NMT_EC_TIMEOUT) or a state change (#CO_NMT_EC_STATE).
 * \param data   a pointer to user-specified data.
 */
typedef void co_nmt_hb_ind_t(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);

/*!
 * The type of a CANopen NMT error control indication function, invoked when a
 * state change is detected. This indication is an extension of the boot-up
 * event indication (sections 7.2.8.2.3.1 and 7.2.8.3.3 in CiA 301 version
 * 4.2.0) since it reports all state changes, not just the boot-up event.
 *
 * \param nmt  a pointer to an NMT master/slave service.
 * \param id   the node-ID (in the range [1..127]).
 * \param st   the state of the node (excluding the toggle bit).
 * \param data a pointer to user-specified data.
 */
typedef void co_nmt_st_ind_t(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, void *data);

/*!
 * The type of a CANopen NMT error control indication function, invoked when a
 * node guarding or heartbeat timeout event occurs. The default handler invokes
 * co_nmt_node_err_ind() or co_nmt_comm_err_ind(), depending on whether the NMT
 * service is a master or not (see co_nmt_is_master()).
 *
 * \param nmt   a pointer to an NMT master/slave service.
 * \param id    the node-ID (in the range [1..127]).
 * \param state indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *              was resolved (#CO_NMT_EC_RESOLVED).
 * \param data  a pointer to user-specified data.
 */
typedef void co_nmt_ec_ind_t(co_nmt_t *nmt, co_unsigned8_t id, int state,
		void *data);

/*!
 * The type of a CANopen NMT 'boot slave' indication function, invoked when the
 * 'boot slave' process completes.
 *
 * \param nmt  a pointer to an NMT master service.
 * \param id   the node-ID of the slave (in the range [1..127]).
 * \param st   the state of the node (including the toggle bit).
 * \param es   the error status (in the range ['A'..'O'], or 0 on success).
 * \param data a pointer to user-specified data.
 */
typedef void co_nmt_boot_ind_t(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, char es, void *data);

/*!
 * The type of a CANopen NMT 'update configuration' indication function, invoked
 * when a configuration request is received. This function MUST cause
 * co_nmt_cfg_res() to be invoked once the 'update configuration' step completes
 * (with success or failure).
 *
 * \param nmt  a pointer to an NMT master service.
 * \param id   the node-ID of the slave (in the range [1..127]).
 * \param sdo  a pointer to a Client-SDO connected to the slave.
 * \param data a pointer to user-specified data.
 */
typedef void co_nmt_cfg_ind_t(co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo,
		void *data);

/*!
 * The type of a CANopen NMT 'configuration request' confirmation callback
 * function, invoked when a configuration request completes (with success or
 * failure).
 *
 * \param nmt  a pointer to an NMT master service.
 * \param id   the node-ID of the slave (in the range [1..127]).
 * \param ac   the SDO abort code (0 on success).
 * \param data a pointer to user-specified data.
 */
typedef void co_nmt_cfg_con_t(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned32_t ac, void *data);

//! Returns a pointer to a string describing an NMT boot error status.
LELY_CO_EXTERN const char *co_nmt_es2str(char es);

LELY_CO_EXTERN void *__co_nmt_alloc(void);
LELY_CO_EXTERN void __co_nmt_free(void *ptr);
LELY_CO_EXTERN struct __co_nmt *__co_nmt_init(struct __co_nmt *nmt,
		can_net_t *net, co_dev_t *dev);
LELY_CO_EXTERN void __co_nmt_fini(struct __co_nmt *nmt);

/*!
 * Creates a new CANopen NMT master/slave service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device.
 *
 * \returns a pointer to a new NMT service, or NULL on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_nmt_destroy()
 */
LELY_CO_EXTERN co_nmt_t *co_nmt_create(can_net_t *net, co_dev_t *dev);

//! Destroys a CANopen NMT master/slave service. \see co_nmt_create()
LELY_CO_EXTERN void co_nmt_destroy(co_nmt_t *nmt);

/*!
 * Retrieves the indication function invoked when an NMT command is received.
 *
 * \param nmt   a pointer to an NMT master/slave service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_set_cs_ind()
 */
LELY_CO_EXTERN void co_nmt_get_cs_ind(const co_nmt_t *nmt,
		co_nmt_cs_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when an NMT command is received.
 *
 * \param nmt  a pointer to an NMT master/slave service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_get_cs_ind()
 */
LELY_CO_EXTERN void co_nmt_set_cs_ind(co_nmt_t *nmt, co_nmt_cs_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when a node guarding event occurs.
 *
 * \param nmt   a pointer to an NMT master service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_set_ng_ind()
 */
LELY_CO_EXTERN void co_nmt_get_ng_ind(const co_nmt_t *nmt,
		co_nmt_ng_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when a node guarding event occurs.
 *
 * \param nmt  a pointer to an NMT master service.
 * \param ind  a pointer to the function to be invoked. If \a ind is NULL, the
 *             default indication function will be used (which invokes
 *             co_nmt_node_err_ind()).
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_get_ng_ind()
 */
LELY_CO_EXTERN void co_nmt_set_ng_ind(co_nmt_t *nmt, co_nmt_ng_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when a life guarding event occurs.
 *
 * \param nmt   a pointer to an NMT slave service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_set_lg_ind()
 */
LELY_CO_EXTERN void co_nmt_get_lg_ind(const co_nmt_t *nmt,
		co_nmt_lg_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when a life guarding event occurs.
 *
 * \param nmt  a pointer to an NMT slave service.
 * \param ind  a pointer to the function to be invoked. If \a ind is NULL, the
 *             default indication function will be used (which invokes
 *             co_nmt_comm_err_ind()).
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_get_lg_ind()
 */
LELY_CO_EXTERN void co_nmt_set_lg_ind(co_nmt_t *nmt, co_nmt_lg_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when a heartbeat event occurs.
 *
 * \param nmt   a pointer to an NMT master/slave service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_set_hb_ind()
 */
LELY_CO_EXTERN void co_nmt_get_hb_ind(const co_nmt_t *nmt,
		co_nmt_hb_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when a heartbeat event occurs.
 *
 * \param nmt  a pointer to an NMT master/slave service.
 * \param ind  a pointer to the function to be invoked. If \a ind is NULL, the
 *             default indication function will be used (which invokes
 *             co_nmt_node_err_ind() or co_nmt_comm_err_ind()).
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_get_hb_ind()
 */
LELY_CO_EXTERN void co_nmt_set_hb_ind(co_nmt_t *nmt, co_nmt_hb_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when a state change is detected.
 *
 * \param nmt   a pointer to an NMT master/slave service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_set_st_ind()
 */
LELY_CO_EXTERN void co_nmt_get_st_ind(const co_nmt_t *nmt,
		co_nmt_st_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when a state change is detected.
 *
 * \param nmt  a pointer to an NMT master/slave service.
 * \param ind  a pointer to the function to be invoked. If \a ind is NULL, the
 *             default indication function will be used (which invokes
 *             co_nmt_boot_req() if a boot-up event indication is received by
 *             an NMT master).
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_get_st_ind()
 */
LELY_CO_EXTERN void co_nmt_set_st_ind(co_nmt_t *nmt, co_nmt_st_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when a CANopen NMT 'boot slave'
 * process completes.
 *
 * \param nmt   a pointer to an NMT master service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_set_boot_ind()
 */
LELY_CO_EXTERN void co_nmt_get_boot_ind(const co_nmt_t *nmt,
		co_nmt_boot_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when a CANopen NMT 'boot slave' process
 * completes.
 *
 * \param nmt  a pointer to an NMT master service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_get_boot_ind()
 */
LELY_CO_EXTERN void co_nmt_set_boot_ind(co_nmt_t *nmt, co_nmt_boot_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when a CANopen NMT 'configuration
 * request' is received.
 *
 * \param nmt   a pointer to an NMT master service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_set_cfg_ind()
 */
LELY_CO_EXTERN void co_nmt_get_cfg_ind(const co_nmt_t *nmt,
		co_nmt_cfg_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when a CANopen NMT 'configuration
 * request' process is received.
 *
 * \param nmt  a pointer to an NMT master service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_get_cfg_ind()
 */
LELY_CO_EXTERN void co_nmt_set_cfg_ind(co_nmt_t *nmt, co_nmt_cfg_ind_t *ind,
		void *data);

//! Returns the pending node-ID. \see co_nmt_set_id()
LELY_CO_EXTERN co_unsigned8_t co_nmt_get_id(const co_nmt_t *nmt);

/*!
 * Sets the pending node-ID. The node-ID of the device will be updated once the
 * NMT 'reset communication command' is received. This is used for the LSS
 * configure node-ID protocol.
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param id  the node-ID of the device (in the range [1..127, 255]).
 *
 * \see co_nmt_get_id(), co_dev_set_id()
 */
LELY_CO_EXTERN int co_nmt_set_id(co_nmt_t *nmt, co_unsigned8_t id);

/*!
 * Returns the current state of a CANopen NMT service (one of #CO_NMT_ST_BOOTUP,
 * #CO_NMT_ST_STOP, #CO_NMT_ST_START, #CO_NMT_ST_RESET_NODE,
 * #CO_NMT_ST_RESET_COMM or #CO_NMT_ST_PREOP).
 */
LELY_CO_EXTERN co_unsigned8_t co_nmt_get_st(const co_nmt_t *nmt);

//! Returns 1 if the specified CANopen NMT service is a master, and 0 if not.
LELY_CO_EXTERN int co_nmt_is_master(const co_nmt_t *nmt);

/*!
 * Submits an NMT request to a slave.
 *
 * \param nmt a pointer to an NMT master service.
 * \param cs  the NMT command specifier (one of #CO_NMT_CS_START,
 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *            #CO_NMT_CS_RESET_COMM).
 * \param id  the node-ID (0 for all nodes, [1..127] for a specific slave).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_nmt_cs_req(co_nmt_t *nmt, co_unsigned8_t cs,
		co_unsigned8_t id);

/*!
 * Requests the NMT 'boot slave' process for the specified node. The function
 * specified to co_nmt_set_boot_ind() is invoked on completion.
 *
 * \param nmt     a pointer to an NMT master service.
 * \param id      the node-ID (in the range [1..127]).
 * \param timeout the SDO timeout (in milliseconds). See co_csdo_set_timeout().
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_nmt_boot_req(co_nmt_t *nmt, co_unsigned8_t id,
		int timeout);

/*!
 * Issues the NMT 'configuration request' for the specified node. The function
 * specified to co_nmt_set_cfg_ind() is invoked to complete the request.
 *
 * \param nmt     a pointer to an NMT master service.
 * \param id      the node-ID (0 for all nodes, [1..127] for a specific slave).
 * \param timeout the SDO timeout (in milliseconds). See co_csdo_set_timeout().
 * \param con     a pointer to the confirmation function (can be NULL).
 * \param data    a pointer to user-specified data (can be NULL). \a data is
 *                passed as the last parameter to \a con.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_nmt_cfg_req(co_nmt_t *nmt, co_unsigned8_t id, int timeout,
		co_nmt_cfg_con_t *con, void *data);

/*!
 * Indicates the result of the 'update configuration' step of an NMT 'request
 * configuration' request for the specified node (see Fig. 8 in CiA 302-2
 * version 4.1.0). This function MUST be called upon completion.
 *
 * \param nmt a pointer to an NMT master service.
 * \param id  the node-ID (in the range [1..127]).
 * \param ac  the SDO abort code (0 on success).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_nmt_cfg_res(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned32_t ac);

/*!
 * Request the node guarding service for the specified node, even if it is not
 * in the network list.
 *
 * \param nmt a pointer to an NMT master service.
 * \param id  the node-ID (in the range [1..127]).
 * \param gt  the guard time (in milliseconds).
 * \param ltf the lifetime factor.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_nmt_ng_req(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned16_t gt, co_unsigned8_t ltf);

/*!
 * Processes an NMT command from the master or the application. Note that this
 * function MAY trigger a reset of one or more CANopen services and invalidate
 * previously obtained results of co_nmt_get_rpdo(), co_nmt_get_tpdo(),
 * co_nmt_get_ssdo(), co_nmt_get_csdo(), co_nmt_get_sync(), co_nmt_get_time()
 * and/or co_nmt_get_emcy().
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param cs  the NMT command specifier (one of #CO_NMT_CS_START,
 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *            #CO_NMT_CS_RESET_COMM).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_nmt_cs_ind(co_nmt_t *nmt, co_unsigned8_t cs);

/*!
 * Indicates the occurrence of a communication error and invokes the specified
 * error behavior (object 1029). Note that this function MAY invoke
 * co_nmt_cs_ind().
 */
LELY_CO_EXTERN void co_nmt_comm_err_ind(co_nmt_t *nmt);

/*!
 * Indicates the occurrence of an error event and triggers the error handling
 * process (see Fig. 12 in CiA 302-2 version 4.1.0). Note that this function MAY
 * invoke co_nmt_cs_ind().
 *
 * \param nmt a pointer to an NMT master service.
 * \param id  the node-ID (in the range [1..127]).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_nmt_node_err_ind(co_nmt_t *nmt, co_unsigned8_t id);

/*!
 * Returns a pointer to a Receive-PDO service.
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param n   the PDO number (in the range [1..512]).
 */
LELY_CO_EXTERN co_rpdo_t *co_nmt_get_rpdo(const co_nmt_t *nmt,
		co_unsigned16_t n);

/*!
 * Returns a pointer to a Transmit-PDO service.
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param n   the PDO number (in the range [1..512]).
 */
LELY_CO_EXTERN co_tpdo_t *co_nmt_get_tpdo(const co_nmt_t *nmt,
		co_unsigned16_t n);

/*!
 * Returns a pointer to a Server-SDO service.
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param n   the SDO number (in the range [1..128]).
 */
LELY_CO_EXTERN co_ssdo_t *co_nmt_get_ssdo(const co_nmt_t *nmt,
		co_unsigned8_t n);

/*!
 * Returns a pointer to a Client-SDO service.
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param n   the SDO number (in the range [1..128]).
 */
LELY_CO_EXTERN co_csdo_t *co_nmt_get_csdo(const co_nmt_t *nmt,
		co_unsigned8_t n);

//! Returns a pointer to the SYNC producer/consumer service.
LELY_CO_EXTERN co_sync_t *co_nmt_get_sync(const co_nmt_t *nmt);

//! Returns a pointer to the TIME producer/consumer service.
LELY_CO_EXTERN co_time_t *co_nmt_get_time(const co_nmt_t *nmt);

//! Returns a pointer to the EMCY producer/consumer service.
LELY_CO_EXTERN co_emcy_t *co_nmt_get_emcy(const co_nmt_t *nmt);

//! Returns a pointer to the LSS master/slave service.
LELY_CO_EXTERN co_lss_t *co_nmt_get_lss(const co_nmt_t *nmt);

#ifdef __cplusplus
}
#endif

#endif

