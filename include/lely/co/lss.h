/**@file
 * This header file is part of the CANopen library; it contains the Layer
 * Setting Services (LSS) and protocols declarations.
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

#ifndef LELY_CO_LSS_H_
#define LELY_CO_LSS_H_

#include <lely/can/net.h>
#include <lely/co/dev.h>

#ifndef LELY_CO_LSS_INHIBIT
/// The default LSS inhibit time (in multiples of 100 microseconds).
#define LELY_CO_LSS_INHIBIT 10
#endif

#ifndef LELY_CO_LSS_TIMEOUT
/// The default LSS timeout (in milliseconds).
#define LELY_CO_LSS_TIMEOUT 100
#endif

/// The CAN identifier used for LSS by the master (1) or the slave (0).
#define CO_LSS_CANID(master) (0x7e4 + !!(master))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen LSS 'activate bit timing' indication function, invoked
 * when a baudrate switch is requested.
 *
 * @param lss   a pointer to an LSS slave service.
 * @param rate  the new baudrate (in kbit/s), or 0 for automatic bit rate
 *              detection.
 * @param delay the delay (in milliseconds) before the switch and the delay
 *              after the switch during which CAN frames MUST NOT be sent.
 * @param data  a pointer to user-specified data.
 */
typedef void co_lss_rate_ind_t(
		co_lss_t *lss, co_unsigned16_t rate, int delay, void *data);

/**
 * The type of a CANopen LSS 'store configuration' indication function, invoked
 * when the pending node-ID and baudrate should be copied to the persistent
 * node-ID and baudrate.
 *
 * @param lss  a pointer to an LSS slave service.
 * @param id   the node-ID.
 * @param rate the new baudrate (in kbit/s), or 0 for automatic bit rate
 *             detection.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or -1 on error.
 */
typedef int co_lss_store_ind_t(co_lss_t *lss, co_unsigned8_t id,
		co_unsigned16_t rate, void *data);

/**
 * The type of a CANopen LSS command received indication function, invoked when
 * a 'switch state selective','LSS identify remote slave' or 'LSS identify
 * non-configured remote slave' request completes.
 *
 * @param lss  a pointer to an LSS master service.
 * @param cs   the received command specifier (one of 0x44, 0x4f or 0x50), or 0
 *             on timeout.
 * @param data a pointer to user-specified data.
 */
typedef void co_lss_cs_ind_t(co_lss_t *lss, co_unsigned8_t cs, void *data);

/**
 * The type of a CANopen LSS error received indication function, invoked when a
 * 'configure node-ID', 'configure bit timing parameters' or 'store
 * configuration' request completes.
 *
 * @param lss  a pointer to an LSS master service.
 * @param cs   the received command specifier (one of 0x11, 0x13 or 0x17), or 0
 *             on timeout.
 * @param err  the error code (0 on success).
 * @param spec the implementation-specific error code (if <b>err</b> is 0xff).
 * @param data a pointer to user-specified data.
 */
typedef void co_lss_err_ind_t(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned8_t err, co_unsigned8_t spec, void *data);

/**
 * The type of a CANopen LSS inquire identity indication function, invoked when
 * an 'inquire identity vendor-ID', 'inquire identity product-code', 'inquire
 * identity revision-number' or 'inquire identity serial-number' request
 * completes.
 *
 * @param lss  a pointer to an LSS master service.
 * @param cs   the received command specifier (one of 0x5a, 0x5b, 0x5c or 0x5d),
 *             or 0 on timeout.
 * @param id   the received LSS number.
 * @param data a pointer to user-specified data.
 */
typedef void co_lss_lssid_ind_t(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned32_t id, void *data);

/**
 * The type of a CANopen LSS inquire node-ID indication function, invoked when
 * an 'inquire node-ID' request completes.
 *
 * @param lss  a pointer to an LSS master service.
 * @param cs   the received command specifier (0x5e), or 0 on timeout.
 * @param id   the received node-ID.
 * @param data a pointer to user-specified data.
 */
typedef void co_lss_nid_ind_t(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned8_t id, void *data);

/**
 * The type of a CANopen LSS identify remote slave indication function, invoked
 * when a 'Slowscan' or 'Fastscan' request completes.
 *
 * @param lss  a pointer to an LSS master service.
 * @param cs   the received command specifier (0x44 or 0x4f), or 0 if no slave
 *             was found.
 * @param id   a pointer to the received LSS address, or NULL if no slave was
 *             found.
 * @param data a pointer to user-specified data.
 */
typedef void co_lss_scan_ind_t(co_lss_t *lss, co_unsigned8_t cs,
		const struct co_id *id, void *data);

void *__co_lss_alloc(void);
void __co_lss_free(void *ptr);
struct __co_lss *__co_lss_init(struct __co_lss *lss, co_nmt_t *nmt);
void __co_lss_fini(struct __co_lss *lss);

/**
 * Creates a new CANopen LSS master/slave service. The service is started as if
 * by co_lss_start().
 *
 * @param nmt a pointer to an NMT master/slave service.
 *
 * @returns a pointer to a new LSS service, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see co_lss_destroy()
 */
co_lss_t *co_lss_create(co_nmt_t *nmt);

/// Destroys a CANopen LSS master/slave service. @see co_lss_create()
void co_lss_destroy(co_lss_t *lss);

/**
 * Starts an LSS service.
 *
 * @post on success, co_lss_is_stopped() returns 0.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_lss_stop()
 */
int co_lss_start(co_lss_t *lss);

/**
 * Stops an LSS service.
 *
 * @post co_lss_is_stopped() returns 1.
 *
 * @see co_lss_start()
 */
void co_lss_stop(co_lss_t *lss);

/**
 * Retuns 1 if the specified LSS service is stopped, and 0 if not.
 *
 * @see co_lss_start, co_lss_stop()
 */
int co_lss_is_stopped(const co_lss_t *lss);

/// Returns a pointer to the NMT service of an LSS master/slave service.
co_nmt_t *co_lss_get_nmt(const co_lss_t *lss);

/**
 * Retrieves the indication function invoked when an LSS 'activate bit timing'
 * request is received.
 *
 * @param lss   a pointer to an LSS slave service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_lss_set_rate_ind()
 */
void co_lss_get_rate_ind(
		const co_lss_t *lss, co_lss_rate_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when an LSS 'activate bit timing'
 * request is received.
 *
 * @param lss  a pointer to an LSS slave service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_lss_get_rate_ind()
 */
void co_lss_set_rate_ind(co_lss_t *lss, co_lss_rate_ind_t *ind, void *data);

/**
 * Retrieves the indication function invoked when an LSS 'store configuration'
 * request is received.
 *
 * @param lss   a pointer to an LSS slave service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_lss_set_store_ind()
 */
void co_lss_get_store_ind(
		const co_lss_t *lss, co_lss_store_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when an LSS 'store configuration'
 * request is received.
 *
 * @param lss  a pointer to an LSS slave service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_lss_get_store_ind()
 */
void co_lss_set_store_ind(co_lss_t *lss, co_lss_store_ind_t *ind, void *data);

/**
 * Returns the inhibit time (in multiples of 100 microseconds) of an LSS master
 * service. A return value of 0 means the inhibit time is disabled.
 *
 * @see co_lss_set_inhibit()
 */
co_unsigned16_t co_lss_get_inhibit(const co_lss_t *lss);

/**
 * Sets the inhibit time between successive LSS messages of an LSS master
 * service.
 *
 * @param lss     a pointer to an LSS master service.
 * @param inhibit the inhibit time (in multiples of 100 microseconds. A value of
 *                0 disables the inhibit time.
 *
 * @see co_lss_get_inhibit()
 */
void co_lss_set_inhibit(co_lss_t *lss, co_unsigned16_t inhibit);

/**
 * Returns the timeout (in milliseconds) of an LSS master service. A return
 * value of 0 means no timeout is being used.
 *
 * @see co_lss_set_timeout()
 */
int co_lss_get_timeout(const co_lss_t *lss);

/**
 * Sets the timeout of an LSS master service.
 *
 * @param lss     a pointer to an LSS master service.
 * @param timeout the timeout (in milliseconds). A value of 0 disables the
 *                timeout.
 *
 * @see co_lss_get_timeout()
 */
void co_lss_set_timeout(co_lss_t *lss, int timeout);

/// Returns 1 if the specified CANopen LSS service is a master, and 0 if not.
int co_lss_is_master(const co_lss_t *lss);

/**
 * Returns 1 if the specified LSS master is idle, and 0 if a request is ongoing.
 */
int co_lss_is_idle(const co_lss_t *lss);

/**
 * Aborts the current LSS master request. This function has no effect if the
 * LSS service is idle (see co_lss_is_idle()).
 */
void co_lss_abort_req(co_lss_t *lss);

/**
 * Requests the 'switch state global' service.
 *
 * See section 6.3.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param mode 0 to switch to all slaves the LSS waiting state, 1 to switch all
 *             slaves to the LSS configuration state.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_switch_req(co_lss_t *lss, co_unsigned8_t mode);

/**
 * Requests the 'switch state selective' service.
 *
 * See section 6.3.3 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param id   a pointer to the LSS address of the slave to be configured.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_switch_sel_req(co_lss_t *lss, const struct co_id *id,
		co_lss_cs_ind_t *ind, void *data);

/**
 * Requests the 'configure node-ID' service. It is the responsibility of the LSS
 * master to ensure that a single LSS slave is in the LSS configuration state.
 *
 * See section 6.4.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param id   the pending node-ID to be configured.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_set_id_req(co_lss_t *lss, co_unsigned8_t id, co_lss_err_ind_t *ind,
		void *data);

/**
 * Requests the 'configure bit timing parameters' service. It is the
 * responsibility of the LSS master to ensure that a single LSS slave is in the
 * LSS configuration state.
 *
 * See section 6.4.3 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param rate the pending baudrate (in kbit/s), or 0 for automatic bit rate
 *             detection.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_set_rate_req(co_lss_t *lss, co_unsigned16_t rate,
		co_lss_err_ind_t *ind, void *data);

/**
 * Requests the 'activate bit timing parameters' service.
 *
 * See section 6.4.4 in CiA 305 version 3.0.0.
 *
 * @param lss   a pointer to an LSS master service.
 * @param delay the delay (in milliseconds) before the switch and the delay
 *              after the switch during which CAN frames MUST NOT be sent.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_switch_rate_req(co_lss_t *lss, int delay);

/**
 * Requests the 'store configuration' service. It is the responsibility of the
 * LSS master to ensure that a single LSS slave is in the LSS configuration
 * state.
 *
 * See section 6.4.5 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_store_req(co_lss_t *lss, co_lss_err_ind_t *ind, void *data);

/**
 * Requests the 'inquire identity vendor-ID' service. It is the responsibility
 * of the LSS master to ensure that a single LSS slave is in the LSS
 * configuration state.
 *
 * See section 6.5.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_get_vendor_id_req(
		co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data);

/**
 * Requests the 'inquire identity product-code' service. It is the
 * responsibility of the LSS master to ensure that a single LSS slave is in the
 * LSS configuration state.
 *
 * See section 6.5.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_get_product_code_req(
		co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data);

/**
 * Requests the 'inquire identity revision-number' service. It is the
 * responsibility of the LSS master to ensure that a single LSS slave is in the
 * LSS configuration state.
 *
 * See section 6.5.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_get_revision_req(co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data);

/**
 * Requests the 'inquire identity serial-number' service. It is the
 * responsibility of the LSS master to ensure that a single LSS slave is in the
 * LSS configuration state.
 *
 * See section 6.5.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_get_serial_nr_req(
		co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data);

/**
 * Requests the 'inquire node-ID' service. It is the responsibility of the LSS
 * master to ensure that a single LSS slave is in the LSS configuration state.
 *
 * See section 6.5.3 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_get_id_req(co_lss_t *lss, co_lss_nid_ind_t *ind, void *data);

/**
 * Requests the 'LSS identify remote slave' service. The specified indication
 * function is invoked as soon as the first slave responds.
 *
 * See section 6.6.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param lo   a pointer to the lower bound of the LSS address.
 * @param hi   a pointer to the upper bound of the LSS address. The vendor-ID
 *             and product-code MUST be the same as in *<b>lo</b>.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_id_slave_req(co_lss_t *lss, const struct co_id *lo,
		const struct co_id *hi, co_lss_cs_ind_t *ind, void *data);

/**
 * Requests the 'LSS identify non-configured remote slave' service. The
 * specified indication function is invoked as soon as the first slave responds.
 *
 * See section 6.6.4 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_id_non_cfg_slave_req(
		co_lss_t *lss, co_lss_cs_ind_t *ind, void *data);

/**
 * Requests the 'LSS Slowscan' service. This service performs a binary search
 * using the 'LSS identify remote slave' service to obtain a single LSS address,
 * followed by the 'switch state selective' service. If the request completes
 * with success, the identified slave is in the LSS configuration state.
 *
 * See section 8.4.2 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param lo   a pointer to the lower bound of the LSS address.
 * @param hi   a pointer to the upper bound of the LSS address. The vendor-ID
 *             and product-code MUST be the same as in *<b>lo</b>.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_slowscan_req(co_lss_t *lss, const struct co_id *lo,
		const struct co_id *hi, co_lss_scan_ind_t *ind, void *data);

/**
 * Requests the 'LSS Fastscan' service. If the request completes with success,
 * the identified slave is in the LSS configuration state.
 *
 * See section 6.7 in CiA 305 version 3.0.0.
 *
 * @param lss  a pointer to an LSS master service.
 * @param id   a pointer a struct containing the bits of the LSS address that
 *             are already known and can be skipped during scanning (can be
 *             NULL).
 * @param mask a pointer to a struct containing the mask specifying which bits
 *             in *<b>id</b> are already known (can be NULL). If a bit in
 *             *<b>mask</b> is 1, the corresponding bit in *<b>id</b> is _not_
 *             checked.
 * @param ind  a pointer to the indication function (can be NULL).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_lss_fastscan_req(co_lss_t *lss, const struct co_id *id,
		const struct co_id *mask, co_lss_scan_ind_t *ind, void *data);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_LSS_H_
