/**@file
 * This header file is part of the CANopen library; it contains the network
 * management (NMT) declarations.
 *
 * @copyright 2021 Lely Industries N.V.
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

#ifndef LELY_CO_NMT_H_
#define LELY_CO_NMT_H_

#include <lely/can/net.h>
#include <lely/co/type.h>

#ifndef LELY_CO_NMT_TIMEOUT
/**
 * The default SDO timeout (in milliseconds) for the NMT 'boot slave' and
 * 'check configuration' processes.
 */
#define LELY_CO_NMT_TIMEOUT 100
#endif

/// The CAN identifier used for NMT commands.
#define CO_NMT_CS_CANID 0x000

/// The NMT command specifier 'start'.
#define CO_NMT_CS_START 0x01

/// The NMT command specifier 'stop'.
#define CO_NMT_CS_STOP 0x02

/// The NMT command specifier 'enter pre-operational'.
#define CO_NMT_CS_ENTER_PREOP 0x80

/// The NMT command specifier 'reset node'.
#define CO_NMT_CS_RESET_NODE 0x81

/// The NMT command specifier 'reset communication'.
#define CO_NMT_CS_RESET_COMM 0x82

/// The NMT state 'boot-up'.
#define CO_NMT_ST_BOOTUP 0x00

/// The NMT state 'stopped'.
#define CO_NMT_ST_STOP 0x04

/// The NMT state 'operational'.
#define CO_NMT_ST_START 0x05

/// The NMT sub-state 'reset application'.
#define CO_NMT_ST_RESET_NODE 0x06

/// The NMT sub-state 'reset communication'.
#define CO_NMT_ST_RESET_COMM 0x07

/// The NMT state 'pre-operational'.
#define CO_NMT_ST_PREOP 0x7f

/// The mask to get/set the toggle bit from an NMT state.
#define CO_NMT_ST_TOGGLE 0x80

/// The CAN identifier used for both node guarding and heartbeat monitoring.
#define CO_NMT_EC_CANID(id) (0x700 + ((id)&0x7f))

enum {
	/// An NMT error control event occurred.
	CO_NMT_EC_OCCURRED,
	/// An NMT error control event was resolved.
	CO_NMT_EC_RESOLVED
};

enum {
	/// An NMT error control timeout event.
	CO_NMT_EC_TIMEOUT,
	/// An NMT error control state change event.
	CO_NMT_EC_STATE
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen NMT command indication function, invoked when an NMT
 * command is received (and _after_ the state switch has occurred).
 *
 * Since the indication function is invoked during an NMT state transition, it
 * is NOT safe to invoke co_nmt_cs_ind() (or co_nmt_cs_req() with the node-ID of
 * the master) from this function.
 *
 * @param nmt  a pointer to an NMT master/slave service.
 * @param cs   the NMT command specifier (one of #CO_NMT_CS_START,
 *             #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *             #CO_NMT_CS_RESET_COMM).
 * @param data a pointer to user-specified data.
 */
typedef void co_nmt_cs_ind_t(co_nmt_t *nmt, co_unsigned8_t cs, void *data);

/**
 * The type of a CANopen NMT node guarding indication function, invoked when a
 * node guarding event occurs (see sections 7.2.8.2.2.1 and 7.2.8.3.2.1 in CiA
 * 301 version 4.2.0). The default handler invokes co_nmt_on_ng().
 *
 * @param nmt    a pointer to an NMT master service.
 * @param id     the node-ID (in the range [1..127]).
 * @param state  indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *               was resolved (#CO_NMT_EC_RESOLVED).
 * @param reason indicates whether the event occurred because of a timeout
 *               (#CO_NMT_EC_TIMEOUT) or an unexpected state change
 *               (#CO_NMT_EC_STATE).
 * @param data   a pointer to user-specified data.
 */
typedef void co_nmt_ng_ind_t(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);

/**
 * The type of a CANopen NMT life guarding indication function, invoked when a
 * life guarding event occurs (see section 7.2.8.2.2.2 in CiA 301 version
 * 4.2.0). The default handler invokes co_nmt_on_lg().
 *
 * @param nmt   a pointer to an NMT slave service.
 * @param state indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *              was resolved (#CO_NMT_EC_RESOLVED).
 * @param data  a pointer to user-specified data.
 */
typedef void co_nmt_lg_ind_t(co_nmt_t *nmt, int state, void *data);

/**
 * The type of a CANopen NMT heartbeat indication function, invoked when a
 * heartbeat event occurs (see sections 7.2.8.2.2.3 and 7.2.8.3.2.2 in CiA 301
 * version 4.2.0). The default handler invokes co_nmt_on_hb().
 *
 * @param nmt    a pointer to an NMT master/slave service.
 * @param id     the node-ID (in the range [1..127]).
 * @param state  indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *               was resolved (#CO_NMT_EC_RESOLVED). Note that heartbeat state
 *               change events only occur and are never resolved.
 * @param reason indicates whether the event occurred because of a timeout
 *               (#CO_NMT_EC_TIMEOUT) or a state change (#CO_NMT_EC_STATE).
 * @param data   a pointer to user-specified data.
 */
typedef void co_nmt_hb_ind_t(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);

/**
 * The type of a CANopen NMT state change indication function, invoked when a
 * state change is detected by the node guarding or heartbeat protocol. This
 * indication is an extension of the boot-up event indication (sections
 * 7.2.8.2.3.1 and 7.2.8.3.3 in CiA 301 version 4.2.0) since it reports all
 * state changes, not just the boot-up event. The default handler invokes
 * co_nmt_on_st().
 *
 * This indication function is also invoked by an NMT master/slave service to
 * notify the user of its own state changes. It that case it is NOT safe to
 * invoke co_nmt_cs_ind() (or co_nmt_cs_req() with the node-ID of the master)
 * from this function.
 *
 * @param nmt  a pointer to an NMT master/slave service.
 * @param id   the node-ID (in the range [1..127]).
 * @param st   the state of the node (excluding the toggle bit).
 * @param data a pointer to user-specified data.
 */
typedef void co_nmt_st_ind_t(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, void *data);

/**
 * The type of a CANopen LSS request function, invoked by an NMT master before
 * booting the slaves (see Fig. 1 in CiA 302-2 version 4.1.0). This function
 * MUST cause co_nmt_lss_con() to be invoked once the LSS process completes.
 *
 * @param nmt  a pointer to an NMT master service.
 * @param lss  a pointer to an LSS master service.
 * @param data a pointer to user-specified data.
 */
typedef void co_nmt_lss_req_t(co_nmt_t *nmt, co_lss_t *lss, void *data);

/**
 * The type of a CANopen NMT 'boot slave' indication function, invoked when the
 * 'boot slave' process completes.
 *
 * @param nmt  a pointer to an NMT master service.
 * @param id   the node-ID of the slave (in the range [1..127]).
 * @param st   the state of the node (including the toggle bit).
 * @param es   the error status (in the range ['A'..'O'], or 0 on success).
 * @param data a pointer to user-specified data.
 */
typedef void co_nmt_boot_ind_t(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, char es, void *data);

/**
 * The type of a CANopen NMT 'update configuration' indication function, invoked
 * when a configuration request is received. This function MUST cause
 * co_nmt_cfg_res() to be invoked once the 'update configuration' step completes
 * (with success or failure).
 *
 * @param nmt  a pointer to an NMT master service.
 * @param id   the node-ID of the slave (in the range [1..127]).
 * @param sdo  a pointer to a Client-SDO connected to the slave.
 * @param data a pointer to user-specified data.
 */
typedef void co_nmt_cfg_ind_t(
		co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo, void *data);

/**
 * The type of a CANopen NMT 'configuration request' confirmation callback
 * function, invoked when a configuration request completes (with success or
 * failure).
 *
 * @param nmt  a pointer to an NMT master service.
 * @param id   the node-ID of the slave (in the range [1..127]).
 * @param ac   the SDO abort code (0 on success).
 * @param data a pointer to user-specified data.
 */
typedef void co_nmt_cfg_con_t(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned32_t ac, void *data);

/**
 * The type of an SDO request progress indication function, invoked by a CANopen
 * NMT master to notify the user of the progress of the an SDO upload/download
 * request during the 'boot slave' process.
 *
 * @param nmt    a pointer to an NMT master service.
 * @param id     the node-ID of the slave (in the range [1..127]).
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param size   The total size (in bytes) of the value being
 *               uploaded/downloaded.
 * @param nbyte  The number of bytes already uploaded/downloaded.
 * @param data   a pointer to user-specified data.
 *
 * @see co_csdo_ind_t
 */
typedef void co_nmt_sdo_ind_t(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned16_t idx, co_unsigned8_t subidx, size_t size,
		size_t nbyte, void *data);

/**
 * The type of a SYNC indication function, invoked by co_nmt_on_sync() _after_
 * PDOs are transmitted/processed upon reception/transmission of a SYNC message.
 *
 * @param nmt  a pointer to an NMT master/slave service.
 * @param cnt  the counter.
 * @param data a pointer to user-specified data.
 *
 * @see co_sync_ind_t
 */
typedef void co_nmt_sync_ind_t(co_nmt_t *nmt, co_unsigned8_t cnt, void *data);

/**
 * Configures heartbeat consumption for the specified node by updating CANopen
 * object 1016 (Consumer heartbeat time).
 *
 * @param dev a pointer to a CANopen device.
 * @param id  the node-ID (in the range [1..127]).
 * @param ms  the heartbeat time (in milliseconds). If <b>ms</b> is 0, the
 *            heartbeat consumer is disabled.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_dev_cfg_hb(
		co_dev_t *dev, co_unsigned8_t id, co_unsigned16_t ms);

/// Returns a pointer to a string describing an NMT boot error status.
const char *co_nmt_es2str(char es);

void *__co_nmt_alloc(void);
void __co_nmt_free(void *ptr);
struct __co_nmt *__co_nmt_init(
		struct __co_nmt *nmt, can_net_t *net, co_dev_t *dev);
void __co_nmt_fini(struct __co_nmt *nmt);

/**
 * Creates a new CANopen NMT master/slave service.
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device.
 *
 * @returns a pointer to a new NMT service, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see co_nmt_destroy()
 */
co_nmt_t *co_nmt_create(can_net_t *net, co_dev_t *dev);

/// Destroys a CANopen NMT master/slave service. @see co_nmt_create()
void co_nmt_destroy(co_nmt_t *nmt);

/// Returns a pointer to the CAN network of an NMT master/slave service.
can_net_t *co_nmt_get_net(const co_nmt_t *nmt);

/// Returns a pointer to the CANopen device of an NMT master/slave service.
co_dev_t *co_nmt_get_dev(const co_nmt_t *nmt);

/**
 * Retrieves the indication function invoked when an NMT command is received.
 *
 * @param nmt   a pointer to an NMT master/slave service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_cs_ind()
 */
void co_nmt_get_cs_ind(
		const co_nmt_t *nmt, co_nmt_cs_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when an NMT command is received.
 *
 * @param nmt  a pointer to an NMT master/slave service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_cs_ind()
 */
void co_nmt_set_cs_ind(co_nmt_t *nmt, co_nmt_cs_ind_t *ind, void *data);

/**
 * Retrieves the indication function invoked when a node guarding event occurs.
 *
 * @param nmt   a pointer to an NMT master service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_ng_ind()
 */
void co_nmt_get_ng_ind(
		const co_nmt_t *nmt, co_nmt_ng_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a node guarding event occurs.
 *
 * @param nmt  a pointer to an NMT master service.
 * @param ind  a pointer to the function to be invoked. If <b>ind</b> is NULL,
 *             the default indication function will be used (which invokes
 *             co_nmt_on_ng()).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_ng_ind()
 */
void co_nmt_set_ng_ind(co_nmt_t *nmt, co_nmt_ng_ind_t *ind, void *data);

/**
 * Implements the default behavior when a node guarding event occurs (see
 * sections 7.2.8.2.2.1 and 7.2.8.3.2.1 in CiA 301 version 4.2.0). This function
 * invokes co_nmt_comm_err_ind() when an event occurs.
 *
 * @see co_nmt_ng_ind_t
 */
void co_nmt_on_ng(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason);

/**
 * Retrieves the indication function invoked when a life guarding event occurs.
 *
 * @param nmt   a pointer to an NMT slave service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_lg_ind()
 */
void co_nmt_get_lg_ind(
		const co_nmt_t *nmt, co_nmt_lg_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a life guarding event occurs.
 *
 * @param nmt  a pointer to an NMT slave service.
 * @param ind  a pointer to the function to be invoked. If <b>ind</b> is NULL,
 *             the default indication function will be used (which invokes
 *             co_nmt_on_lg()).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_lg_ind()
 */
void co_nmt_set_lg_ind(co_nmt_t *nmt, co_nmt_lg_ind_t *ind, void *data);

/**
 * Implements the default behavior when a life guarding event occurs (see
 * section 7.2.8.2.2.2 in CiA 301 version 4.2.0). This function invokes
 * co_nmt_comm_err_ind() when an event occurs.
 *
 * @see co_nmt_lg_ind_t
 */
void co_nmt_on_lg(co_nmt_t *nmt, int state);

/**
 * Retrieves the indication function invoked when a heartbeat event occurs.
 *
 * @param nmt   a pointer to an NMT master/slave service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_hb_ind()
 */
void co_nmt_get_hb_ind(
		const co_nmt_t *nmt, co_nmt_hb_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a heartbeat event occurs.
 *
 * @param nmt  a pointer to an NMT master/slave service.
 * @param ind  a pointer to the function to be invoked. If <b>ind</b> is NULL,
 *             the default indication function will be used (which invokes
 *             co_nmt_on_hb()).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_hb_ind()
 */
void co_nmt_set_hb_ind(co_nmt_t *nmt, co_nmt_hb_ind_t *ind, void *data);

/**
 * Implements the default behavior when a heartbeat event occurs (see sections
 * 7.2.8.2.2.3 and 7.2.8.3.2.2 in CiA 301 version 4.2.0). This function invokes
 * co_nmt_node_err_ind() or co_nmt_comm_err_ind() when a timeout event occurs,
 * depending on whether the NMT service is a master or not (see
 * co_nmt_is_master()).
 *
 * @see co_nmt_hb_ind_t
 */
void co_nmt_on_hb(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason);

/**
 * Retrieves the indication function invoked when a state change is detected.
 *
 * @param nmt   a pointer to an NMT master/slave service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_st_ind()
 */
void co_nmt_get_st_ind(
		const co_nmt_t *nmt, co_nmt_st_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a state change is detected.
 *
 * @param nmt  a pointer to an NMT master/slave service.
 * @param ind  a pointer to the function to be invoked. If <b>ind</b> is NULL,
 *             the default indication function will be used (which invokes
 *             co_nmt_on_st()).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_st_ind()
 */
void co_nmt_set_st_ind(co_nmt_t *nmt, co_nmt_st_ind_t *ind, void *data);

/**
 * Implements the default behavior when a state change is detected by the node
 * guarding or heartbeat protocol. In case of a boot-up event (and the NMT
 * service is a master), this function invokes co_nmt_boot_req() to boot the
 * slave (see Fig. 13 in CiA 302-2).
 *
 * @see co_nmt_st_ind_t
 */
void co_nmt_on_st(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st);

/**
 * Retrieves the request function invoked to perform LSS when booting an NMT
 * master.
 *
 * @param nmt   a pointer to an NMT master service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_lss_req()
 */
void co_nmt_get_lss_req(
		const co_nmt_t *nmt, co_nmt_lss_req_t **pind, void **pdata);

/**
 * Sets the request function invoked to perform LSS when booting an NMT master.
 * Setting this function means LSS is required (see Fig. 1 in CiA 302-2 version
 * 4.1.0).
 *
 * @param nmt  a pointer to an NMT master service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_lss_req()
 */
void co_nmt_set_lss_req(co_nmt_t *nmt, co_nmt_lss_req_t *ind, void *data);

/**
 * Retrieves the indication function invoked when a CANopen NMT 'boot slave'
 * process completes.
 *
 * @param nmt   a pointer to an NMT master service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_boot_ind()
 */
void co_nmt_get_boot_ind(
		const co_nmt_t *nmt, co_nmt_boot_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a CANopen NMT 'boot slave' process
 * completes.
 *
 * @param nmt  a pointer to an NMT master service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_boot_ind()
 */
void co_nmt_set_boot_ind(co_nmt_t *nmt, co_nmt_boot_ind_t *ind, void *data);

/**
 * Retrieves the indication function invoked when a CANopen NMT 'configuration
 * request' is received.
 *
 * @param nmt   a pointer to an NMT master service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_cfg_ind()
 */
void co_nmt_get_cfg_ind(
		const co_nmt_t *nmt, co_nmt_cfg_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a CANopen NMT 'configuration
 * request' process is received.
 *
 * @param nmt  a pointer to an NMT master service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_cfg_ind()
 */
void co_nmt_set_cfg_ind(co_nmt_t *nmt, co_nmt_cfg_ind_t *ind, void *data);

/**
 * Retrieves the indication function used to notify the user of the progress of
 * the current SDO download request during the CANopen NMT 'boot slave' process.
 *
 * @param nmt   a pointer to an NMT master service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_dn_ind()
 */
void co_nmt_get_dn_ind(
		const co_nmt_t *nmt, co_nmt_sdo_ind_t **pind, void **pdata);

/**
 * Sets the indication function used to notify the user of the progress of the
 * current SDO download request during the CANopen NMT 'boot slave' process.
 *
 * @param nmt  a pointer to an NMT master service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_dn_ind()
 */
void co_nmt_set_dn_ind(co_nmt_t *nmt, co_nmt_sdo_ind_t *ind, void *data);

/**
 * Retrieves the indication function used to notify the user of the progress of
 * the current SDO upload request during the CANopen NMT 'boot slave' process.
 *
 * @param nmt   a pointer to an NMT master service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_up_ind()
 */
void co_nmt_get_up_ind(
		const co_nmt_t *nmt, co_nmt_sdo_ind_t **pind, void **pdata);

/**
 * Sets the indication function used to notify the user of the progress of the
 * current SDO upload request during the CANopen NMT 'boot slave' process.
 *
 * @param nmt  a pointer to an NMT master service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_up_ind()
 */
void co_nmt_set_up_ind(co_nmt_t *nmt, co_nmt_sdo_ind_t *ind, void *data);

/**
 * Retrieves the indication function invoked by co_nmt_on_sync() after all PDOs
 * have been transmitted/processed.
 *
 * @param nmt   a pointer to an NMT master/slave service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_nmt_set_sync_ind()
 */
void co_nmt_get_sync_ind(
		const co_nmt_t *nmt, co_nmt_sync_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked by co_nmt_on_sync() after all PDOs have
 * been transmitted/processed.
 *
 * @param nmt  a pointer to an NMT master/slave service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_nmt_get_sync_ind()
 */
void co_nmt_set_sync_ind(co_nmt_t *nmt, co_nmt_sync_ind_t *ind, void *data);

/**
 * Implements the default behavior after a SYNC object is received or
 * transmitted. First all synchronous Transmit-PDOs are sent with
 * co_tpdo_sync(). Then all synchronous Receive-PDOs are actuated with
 * co_rpdo_sync(). Finally the user-defined callback function set with
 * co_nmt_set_sync_ind() is invoked.
 *
 * @see co_sync_ind_t, co_nmt_sync_ind_t
 */
void co_nmt_on_sync(co_nmt_t *nmt, co_unsigned8_t cnt);

/**
 * Implements the default error handling behavior by generating an EMCY message
 * with co_emcy_push() and invoking co_nmt_comm_err_ind() in case of a
 * communication error (emergency error code 0x81xx).
 *
 * @see co_rpdo_err_t, co_sync_err_t
 */
void co_nmt_on_err(co_nmt_t *nmt, co_unsigned16_t eec, co_unsigned8_t er,
		const co_unsigned8_t msef[5]);

/**
 * Implements the default behavior when an event is indicated for an
 * event-driven (asynchronous) Transmit-PDO by triggering the transmission of
 * the PDO with co_tpdo_event().
 *
 * The transmission of PDOs can be postponed with co_nmt_on_tpdo_event_lock().
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param n   the PDO number (in the range [1..512]). If <b>n</b> is 0, the
 *            transmission of all PDOs is triggered.
 *
 * @see co_dev_tpdo_event_ind_t
 */
void co_nmt_on_tpdo_event(co_nmt_t *nmt, co_unsigned16_t n);

/**
 * Postpones the transmission of PDOs triggered by co_nmt_on_tpdo_event() until
 * a matching call to co_nmt_on_tpdo_event_unlock(). This function can be
 * invoked multiple times. PDO transmission will resume after an equal number of
 * calls to co_nmt_on_tpdo_event_unlock().
 */
void co_nmt_on_tpdo_event_lock(co_nmt_t *nmt);

/**
 * Undoes the effect of a single call to co_nmt_on_tpdo_event_lock() and
 * possibly triggers the transmission of postponed PDOs.
 */
void co_nmt_on_tpdo_event_unlock(co_nmt_t *nmt);

/**
 * Implements the default behavior when an event is indicated for a source
 * address mode multiplex PDO by triggering the transmission of the PDO with
 * co_sam_mpdo_event().
 *
 * @param nmt    a pointer to an NMT master/slave service.
 * @param n      the PDO number (in the range [1..512]).
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @see co_dev_sam_mpdo_event_ind_t
 */
void co_nmt_on_sam_mpdo_event(co_nmt_t *nmt, co_unsigned16_t n,
		co_unsigned16_t idx, co_unsigned8_t subidx);

/// Returns the pending node-ID. @see co_nmt_set_id()
co_unsigned8_t co_nmt_get_id(const co_nmt_t *nmt);

/**
 * Sets the pending node-ID. The node-ID of the device will be updated once the
 * NMT 'reset communication command' is received. This is used for the LSS
 * configure node-ID protocol.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param id  the node-ID of the device (in the range [1..127, 255]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_nmt_get_id(), co_dev_set_id()
 */
int co_nmt_set_id(co_nmt_t *nmt, co_unsigned8_t id);

/**
 * Returns the current state of a CANopen NMT service (one of #CO_NMT_ST_BOOTUP,
 * #CO_NMT_ST_STOP, #CO_NMT_ST_START, #CO_NMT_ST_RESET_NODE,
 * #CO_NMT_ST_RESET_COMM or #CO_NMT_ST_PREOP).
 */
co_unsigned8_t co_nmt_get_st(const co_nmt_t *nmt);

/// Returns 1 if the specified CANopen NMT service is a master, and 0 if not.
int co_nmt_is_master(const co_nmt_t *nmt);

/**
 * Returns the default SDO timeout used during the NMT 'boot slave' and
 * 'check configuration' processes.
 *
 * @see co_nmt_set_timeout()
 */
int co_nmt_get_timeout(const co_nmt_t *nmt);

/**
 * Sets the default SDO timeout used during the NMT 'boot slave' and
 * 'check configuration' processes.
 *
 * @param nmt     a pointer to an NMT master service.
 * @param timeout the SDO timeout (in milliseconds). See co_csdo_set_timeout().
 *
 * @see co_nmt_get_timeout()
 */
void co_nmt_set_timeout(co_nmt_t *nmt, int timeout);

/**
 * Submits an NMT request to a slave. If <b>id</b> equals the node-ID of the
 * master, this is equivalent to `co_nmt_cs_ind(nmt, cs)`.
 *
 * @param nmt a pointer to an NMT master service.
 * @param cs  the NMT command specifier (one of #CO_NMT_CS_START,
 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *            #CO_NMT_CS_RESET_COMM).
 * @param id  the node-ID (0 for all nodes, [1..127] for a specific slave).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_cs_req(co_nmt_t *nmt, co_unsigned8_t cs, co_unsigned8_t id);

/**
 * Confirms the completion of the process when booting an NMT master. The
 * function specified to co_nmt_set_lss_req() MUST cause this function to be
 * invoked.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_lss_con(co_nmt_t *nmt);

/**
 * Requests the NMT 'boot slave' process for the specified node. The function
 * specified to co_nmt_set_boot_ind() is invoked on completion.
 *
 * @param nmt     a pointer to an NMT master service.
 * @param id      the node-ID (in the range [1..127]).
 * @param timeout the SDO timeout (in milliseconds). See co_csdo_set_timeout().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_boot_req(co_nmt_t *nmt, co_unsigned8_t id, int timeout);

/**
 * Returns 1 if the NMT 'boot slave' process is currently running for the
 * specified node, and 0 if not.
 */
int co_nmt_is_booting(const co_nmt_t *nmt, co_unsigned8_t id);

/**
 * Checks if a boot-up message has been received from the specified node(s).
 *
 * @param nmt a pointer to an NMT master service.
 * @param id  the node-ID (0 for all mandatory nodes, [1..127] for a specific
 *            slave).
 *
 * @returns 1 if a boot-up message was received, 0 if not, or -1 on error. In
 * the latter case, the error number can be obtained with get_errc().
 */
int co_nmt_chk_bootup(const co_nmt_t *nmt, co_unsigned8_t id);

/**
 * Issues the NMT 'configuration request' for the specified node. The function
 * specified to co_nmt_set_cfg_ind() is invoked to complete the request.
 *
 * @param nmt     a pointer to an NMT master service.
 * @param id      the node-ID (0 for all nodes, [1..127] for a specific slave).
 * @param timeout the SDO timeout (in milliseconds). See co_csdo_set_timeout().
 * @param con     a pointer to the confirmation function (can be NULL).
 * @param data    a pointer to user-specified data (can be NULL). <b>data</b> is
 *                passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_cfg_req(co_nmt_t *nmt, co_unsigned8_t id, int timeout,
		co_nmt_cfg_con_t *con, void *data);

/**
 * Indicates the result of the 'update configuration' step of an NMT 'request
 * configuration' request for the specified node (see Fig. 8 in CiA 302-2
 * version 4.1.0). This function MUST be called upon completion.
 *
 * @param nmt a pointer to an NMT master service.
 * @param id  the node-ID (in the range [1..127]).
 * @param ac  the SDO abort code (0 on success).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_cfg_res(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned32_t ac);

/**
 * Request the node guarding service for the specified node, even if it is not
 * in the network list. If the guard time or lifetime factor is 0, node guarding
 * is disabled.
 *
 * @param nmt a pointer to an NMT master service.
 * @param id  the node-ID (in the range [1..127]).
 * @param gt  the guard time (in milliseconds).
 * @param ltf the lifetime factor.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_ng_req(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned16_t gt,
		co_unsigned8_t ltf);

/**
 * Processes an NMT command from the master or the application. Note that this
 * function MAY trigger a reset of one or more CANopen services and invalidate
 * previously obtained results of co_nmt_get_rpdo(), co_nmt_get_tpdo(),
 * co_nmt_get_ssdo(), co_nmt_get_csdo(), co_nmt_get_sync(), co_nmt_get_time()
 * and/or co_nmt_get_emcy().
 *
 * This function MUST NOT be invoked during an NMT state transition. It is
 * therefore unsafe to call co_nmt_cs_ind() (or co_nmt_cs_req() with the node-ID
 * of the master) from the #co_nmt_cs_ind_t and #co_nmt_st_ind_t indication
 * functions.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param cs  the NMT command specifier (one of #CO_NMT_CS_START,
 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *            #CO_NMT_CS_RESET_COMM).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_cs_ind(co_nmt_t *nmt, co_unsigned8_t cs);

/**
 * Indicates the occurrence of a communication error and invokes the specified
 * error behavior (object 1029:01). Note that this function MAY invoke
 * co_nmt_cs_ind().
 */
void co_nmt_comm_err_ind(co_nmt_t *nmt);

/**
 * Indicates the occurrence of an error event and triggers the error handling
 * process (see Fig. 12 in CiA 302-2 version 4.1.0). Note that this function MAY
 * invoke co_nmt_cs_ind().
 *
 * @param nmt a pointer to an NMT master service.
 * @param id  the node-ID (in the range [1..127]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_node_err_ind(co_nmt_t *nmt, co_unsigned8_t id);

/**
 * Returns a pointer to a Receive-PDO service.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param n   the PDO number (in the range [1..512]).
 */
co_rpdo_t *co_nmt_get_rpdo(const co_nmt_t *nmt, co_unsigned16_t n);

/**
 * Returns a pointer to a Transmit-PDO service.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param n   the PDO number (in the range [1..512]).
 */
co_tpdo_t *co_nmt_get_tpdo(const co_nmt_t *nmt, co_unsigned16_t n);

/**
 * Returns a pointer to a Server-SDO service.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param n   the SDO number (in the range [1..128]).
 */
co_ssdo_t *co_nmt_get_ssdo(const co_nmt_t *nmt, co_unsigned8_t n);

/**
 * Returns a pointer to a Client-SDO service.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param n   the SDO number (in the range [1..128]).
 */
co_csdo_t *co_nmt_get_csdo(const co_nmt_t *nmt, co_unsigned8_t n);

/// Returns a pointer to the SYNC producer/consumer service.
co_sync_t *co_nmt_get_sync(const co_nmt_t *nmt);

/// Returns a pointer to the TIME producer/consumer service.
co_time_t *co_nmt_get_time(const co_nmt_t *nmt);

/// Returns a pointer to the EMCY producer/consumer service.
co_emcy_t *co_nmt_get_emcy(const co_nmt_t *nmt);

/// Returns a pointer to the LSS master/slave service.
co_lss_t *co_nmt_get_lss(const co_nmt_t *nmt);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_NMT_H_
