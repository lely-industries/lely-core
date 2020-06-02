/**@file
 * This header file is part of the CANopen library; it contains the Wireless
 * Transmission Media (WTM) declarations.
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

#ifndef LELY_CO_WTM_H_
#define LELY_CO_WTM_H_

#include <lely/can/msg.h>
#include <lely/co/co.h>
#include <lely/libc/time.h>

/**
 * The maximum size of a CANopen WTM generic frame (4 (header) + 255 (payload) +
 * 2 (CRC checksum) = 261).
 */
#define CO_WTM_MAX_LEN 261

/// CANopen WTM abort code: General error.
#define CO_WTM_AC_ERROR UINT32_C(0x01000000)

/// CANopen WTM abort code: Diagnostic protocol timed out limit reached.
#define CO_WTM_AC_TIMEOUT UINT32_C(0x01000001)

/// CANopen WTM abort code: Out of memory.
#define CO_WTM_AC_NO_MEM UINT32_C(0x01000002)

/// CANopen WTM abort code: Access failed due to a hardware error.
#define CO_WTM_AC_HARDWARE UINT32_C(0x01000003)

/**
 * CANopen WTM abort code: Data cannot be transferred or stored to the
 * application.
 */
#define CO_WTM_AC_DATA UINT32_C(0x01000004)

/**
 * CANopen WTM abort code: Data cannot be transferred or stored to the
 * application because of local control.
 */
#define CO_WTM_AC_DATA_CTL UINT32_C(0x01000005)

/**
 * CANopen WTM abort code: Data cannot be transferred or stored to the
 * application because of the present device state.
 */
#define CO_WTM_AC_DATA_DEV UINT32_C(0x01000006)

/// CANopen WTM abort code: No data available.
#define CO_WTM_AC_NO_DATA UINT32_C(0x01000007)

/// CANopen WTM abort code: Requested interface not implemented.
#define CO_WTM_AC_NO_IF UINT32_C(0x01000008)

/// CANopen WTM abort code: Requested interface disabled.
#define CO_WTM_AC_IF_DOWN UINT32_C(0x01000009)

/// CANopen WTM abort code: Diagnostic data generation not supported.
#define CO_WTM_AC_DIAG UINT32_C(0x0100000a)

/**
 * CANopen WTM abort code: Diagnostic data generation for requested CAN
 * interface not supported.
 */
#define CO_WTM_AC_DIAG_CAN UINT32_C(0x0100000b)

/**
 * CANopen WTM abort code: Diagnostic data generation for requested WTM
 * interface not supported.
 */
#define CO_WTM_AC_DIAG_WTM UINT32_C(0x0100000c)

/// CANopen WTM abort code: General generic frame error.
#define CO_WTM_AC_FRAME UINT32_C(0x02000000)

/// CANopen WTM abort code: Invalid generic frame preamble.
#define CO_WTM_AC_PREAMBLE UINT32_C(0x02000001)

/// CANopen WTM abort code: Invalid sequence counter in generic frame.
#define CO_WTM_AC_SEQ UINT32_C(0x02000002)

/// CANopen WTM abort code: Message type not valid or unknown.
#define CO_WTM_AC_TYPE UINT32_C(0x02000003)

/// CANopen WTM abort code: Payload field in generic frame invalid.
#define CO_WTM_AC_PAYLOAD UINT32_C(0x02000004)

/// CANopen WTM abort code: CRC error (Generic frame).
#define CO_WTM_AC_CRC UINT32_C(0x02000005)

/// CANopen WTM abort code: CAN telegram essentials invalid.
#define CO_WTM_AC_CAN UINT32_C(0x02000006)

struct __co_wtm;
#if !defined(__cplusplus) || LELY_NO_CXX
/// An opaque CANopen Wireless Transmission Media (WTM) interface type.
typedef struct __co_wtm co_wtm_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen WTM diagnostic confirmation function, invoked when a
 * CAN communication quality response is received.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param nif  the remote CAN interface indicator (in the range [1..127]).
 * @param st   The current CAN controller status (one of `CAN_STATE_ACTIVE`,
 *             `CAN_STATE_PASSIVE` or `CAN_STATE_BUSOFF`, or 0xf if the
 *             information is not available).
 * @param err  the last detected error (0 if no error was detected, one of
 *             `CAN_ERROR_BIT`, `CAN_ERROR_STUFF`, `CAN_ERROR_CRC`,
 *             `CAN_ERROR_FORM` or `CAN_ERROR_ACK` in case of an error, or 0xf
 *             if the information is not available).
 * @param load the current busload percentage (in the range [0..100], or 0xff if
 *             the information is not available).
 * @param ec   the number of detected errors that led to the increase of one of
 *             the CAN controller internal error counters (in the range
 *             [0..0xfffe], or 0xffff if the information is not available).
 * @param foc  the FIFO overrun counter (in the range [0..0xfffe], or 0xffff if
 *             the information is not available).
 * @param coc  the CAN controller overrun counter (in the range [0..0xfffe], or
 *             0xffff if the information is not available).
 * @param data a pointer to user-specified data.
 */
typedef void co_wtm_diag_can_con_t(co_wtm_t *wtm, uint_least8_t nif,
		uint_least8_t st, uint_least8_t err, uint_least8_t load,
		uint_least16_t ec, uint_least16_t foc, uint_least16_t coc,
		void *data);

/**
 * The type of a CANopen WTM diagnostic confirmation function, invoked when a
 * WTM communication quality response is received.
 *
 * @param wtm     a pointer to a CANopen WTM interface.
 * @param nif     the remote WTM interface indicator (in the range [1..127]).
 * @param quality the link quality percentage (in the range [0..100], or 0xff if
 *                the information is not available).
 * @param data    a pointer to user-specified data.
 */
typedef void co_wtm_diag_wtm_con_t(co_wtm_t *wtm, uint_least8_t nif,
		uint_least8_t quality, void *data);

/**
 * The type of a CANopen WTM diagnostic indication function, invoked when a CAN
 * communication quality reset message is received.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param nif  the remote CAN interface indicator (in the range [1..127]).
 * @param data a pointer to user-specified data.
 */
typedef void co_wtm_diag_can_ind_t(
		co_wtm_t *wtm, uint_least8_t nif, void *data);

/**
 * The type of a CANopen WTM diagnostic indication function, invoked when a WTM
 * communication quality reset message is received.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param data a pointer to user-specified data.
 */
typedef void co_wtm_diag_wtm_ind_t(co_wtm_t *wtm, void *data);

/**
 * The type of a CANopen WTM diagnostic indication function, invoked when an
 * abort code is generated or received.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param ac   the abort code.
 * @param data a pointer to user-specified data.
 */
typedef void co_wtm_diag_ac_ind_t(co_wtm_t *wtm, uint_least32_t ac, void *data);

/**
 * The type of a CANopen WTM receive callback function, invoked when a CAN frame
 * is received.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param nif  the CAN interface indicator (in the range [1..127]).
 * @param tp   a pointer to the (relative) time at which the CAN frame was
 *             received (can be NULL).
 * @param msg  a pointer to the received CAN frame.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with `set_errnum()`.
 */
typedef int co_wtm_recv_func_t(co_wtm_t *wtm, uint_least8_t nif,
		const struct timespec *tp, const struct can_msg *msg,
		void *data);

/**
 * The type of a CANopen WTM send callback function, invoked when a byte stream
 * needs to be sent.
 *
 * @param wtm    a pointer to a CANopen WTM interface.
 * @param buf    a pointer to the bytes to be sent.
 * @param nbytes the number of bytes to send.
 * @param data   a pointer to user-specified data.
 *
 * @returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with `set_errnum()`.
 */
typedef int co_wtm_send_func_t(
		co_wtm_t *wtm, const void *buf, size_t nbytes, void *data);

/// Returns a string describing a CANopen WTM abort code.
const char *co_wtm_ac_str(uint_least32_t ac);

void *__co_wtm_alloc(void);
void __co_wtm_free(void *ptr);
struct __co_wtm *__co_wtm_init(struct __co_wtm *wtm);
void __co_wtm_fini(struct __co_wtm *wtm);

/**
 * Creates a new CANopen Wireless Transmission Media (WTM) interface.
 *
 * @returns a pointer to a new WTM interface, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see co_wtm_destroy()
 */
co_wtm_t *co_wtm_create(void);

/**
 * Destroys a CANopen Wireless Transmission Media (WTM) interface.
 *
 * @see co_wtm_create()
 */
void co_wtm_destroy(co_wtm_t *wtm);

/**
 * Returns the interface indicator of a CANopen WTM interface.
 *
 * @see co_wtm_set_nif()
 */
uint_least8_t co_wtm_get_nif(const co_wtm_t *wtm);

/**
 * Sets the interface indicator of a CANopen WTM interface.
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param nif the WTM interface indicator (in the range [1..127]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_wtm_get_nif()
 */
int co_wtm_set_nif(co_wtm_t *wtm, uint_least8_t nif);

/**
 * Sets the diagnostic parameters of a CAN interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param nif  the local CAN interface indicator (in the range [1..127]).
 * @param st   The current CAN controller status (one of `CAN_STATE_ACTIVE`,
 *             `CAN_STATE_PASSIVE` or `CAN_STATE_BUSOFF`, or 0xf if the
 *             information is not available).
 * @param err  the last detected error (0 if no error was detected, one of
 *             `CAN_ERROR_BIT`, `CAN_ERROR_STUFF`, `CAN_ERROR_CRC`,
 *             `CAN_ERROR_FORM` or `CAN_ERROR_ACK` in case of an error, or 0xf
 *             if the information is not available).
 * @param load the current busload percentage (in the range [0..100], or 0xff if
 *             the information is not available).
 * @param ec   the number of detected errors that led to the increase of one of
 *             the CAN controller internal error counters (in the range
 *             [0..0xfffe], or 0xffff if the information is not available).
 * @param foc  the FIFO overrun counter (in the range [0..0xfffe], or 0xffff if
 *             the information is not available).
 * @param coc  the CAN controller overrun counter (in the range [0..0xfffe], or
 *             0xffff if the information is not available).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_set_diag_can(co_wtm_t *wtm, uint_least8_t nif, uint_least8_t st,
		uint_least8_t err, uint_least8_t load, uint_least16_t ec,
		uint_least16_t foc, uint_least16_t coc);

/**
 * Sets the diagnostic parameters of a WTM interface.
 *
 * @param wtm     a pointer to a CANopen WTM interface.
 * @param quality the link quality percentage (in the range [0..100], or 0xff if
 *                the information is not available).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_set_diag_wtm(co_wtm_t *wtm, uint_least8_t quality);

/**
 * Retrieves the confirmation function invoked when a CAN communication quality
 * response is received by a CANopen WTM interface.
 *
 * @param wtm   a pointer to a CANopen WTM interface.
 * @param pcon  the address at which to store a pointer to the confirmation
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_wtm_set_diag_can_con()
 */
void co_wtm_get_diag_can_con(const co_wtm_t *wtm, co_wtm_diag_can_con_t **pcon,
		void **pdata);

/**
 * Sets the confirmation function invoked when a CAN communication quality
 * response is received by a CANopen WTM interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param con  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>con</b>.
 *
 * @see co_wtm_get_diag_can_con()
 */
void co_wtm_set_diag_can_con(
		co_wtm_t *wtm, co_wtm_diag_can_con_t *con, void *data);

/**
 * Retrieves the confirmation function invoked when a WTM communication quality
 * response is received by a CANopen WTM interface.
 *
 * @param wtm   a pointer to a CANopen WTM interface.
 * @param pcon  the address at which to store a pointer to the confirmation
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_wtm_set_diag_wtm_con()
 */
void co_wtm_get_diag_wtm_con(const co_wtm_t *wtm, co_wtm_diag_wtm_con_t **pcon,
		void **pdata);

/**
 * Sets the confirmation function invoked when a WTM communication quality
 * response is received by a CANopen WTM interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param con  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>con</b>.
 *
 * @see co_wtm_get_diag_wtm_con()
 */
void co_wtm_set_diag_wtm_con(
		co_wtm_t *wtm, co_wtm_diag_wtm_con_t *con, void *data);

/**
 * Retrieves the indication function invoked when a CAN communication quality
 * reset message is received by a CANopen WTM interface.
 *
 * @param wtm   a pointer to a CANopen WTM interface.
 * @param pcon  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_wtm_set_diag_can_ind()
 */
void co_wtm_get_diag_can_ind(const co_wtm_t *wtm, co_wtm_diag_can_ind_t **pcon,
		void **pdata);

/**
 * Sets the indication function invoked when a CAN communication quality reset
 * message is received by a CANopen WTM interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param con  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>con</b>.
 *
 * @see co_wtm_get_diag_can_ind()
 */
void co_wtm_set_diag_can_ind(
		co_wtm_t *wtm, co_wtm_diag_can_ind_t *con, void *data);

/**
 * Retrieves the indication function invoked when a WTM communication quality
 * reset message is received by a CANopen WTM interface.
 *
 * @param wtm   a pointer to a CANopen WTM interface.
 * @param pcon  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_wtm_set_diag_wtm_ind()
 */
void co_wtm_get_diag_wtm_ind(const co_wtm_t *wtm, co_wtm_diag_wtm_ind_t **pcon,
		void **pdata);

/**
 * Sets the indication function invoked when a WTM communication quality reset
 * message is received by a CANopen WTM interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param con  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>con</b>.
 *
 * @see co_wtm_get_diag_wtm_ind()
 */
void co_wtm_set_diag_wtm_ind(
		co_wtm_t *wtm, co_wtm_diag_wtm_ind_t *con, void *data);

/**
 * Retrieves the indication function invoked when an abort code is generated or
 * received by a CANopen WTM interface.
 *
 * @param wtm   a pointer to a CANopen WTM interface.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_wtm_set_diag_ac_ind()
 */
void co_wtm_get_diag_ac_ind(
		const co_wtm_t *wtm, co_wtm_diag_ac_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when an abort code is generated or
 * received by a CANopen WTM interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param ind  a pointer to the function to be invoked. If <b>ind</b> is NULL,
 *             the default indication function will be used (which invokes
 *             `diag()`).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_wtm_get_diag_ac_ind()
 */
void co_wtm_set_diag_ac_ind(
		co_wtm_t *wtm, co_wtm_diag_ac_ind_t *ind, void *data);

/**
 * Receives and processes a byte stream with a CANopen WTM interface. This
 * function MAY invoke the callback function specified to co_wtm_set_recv_func()
 * 0 or more times.
 *
 * @param wtm    a pointer to a CANopen WTM interface.
 * @param buf    a pointer to the bytes to be processed.
 * @param nbytes the number of bytes received.
 */
void co_wtm_recv(co_wtm_t *wtm, const void *buf, size_t nbytes);

/**
 * Retrieves the callback function invoked when a CAN frame is received by a
 * CANopen WTM interface.
 *
 * @param wtm   a pointer to a CANopen WTM interface.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_wtm_set_recv_func()
 */
void co_wtm_get_recv_func(
		const co_wtm_t *wtm, co_wtm_recv_func_t **pfunc, void **pdata);

/**
 * Sets the callback function invoked when a CAN frame is received by a CANopen
 * WTM interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param func a pointer to the function to be invoked by co_wtm_recv().
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see co_wtm_get_recv_func()
 */
void co_wtm_set_recv_func(co_wtm_t *wtm, co_wtm_recv_func_t *func, void *data);

/**
 * Retrieves the current time of a CANopen WTM interface.
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param nif the CAN interface indicator (in the range [1..127]).
 * @param tp  the address at which to store the current time (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_wtm_set_time()
 */
int co_wtm_get_time(
		const co_wtm_t *wtm, uint_least8_t nif, struct timespec *tp);

/**
 * Sets the current time of a CANopen WTM interface. This function MAY invoke
 * the callback function set by co_wtm_set_send_func().
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param nif the CAN interface indicator (in the range [1..127]).
 * @param tp  a pointer to the current time.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_wtm_get_time()
 */
int co_wtm_set_time(
		co_wtm_t *wtm, uint_least8_t nif, const struct timespec *tp);

/**
 * Sends a CAN frame from a CANopen WTM interface. This function MAY invoke the
 * callback function set by co_wtm_set_send_func(). Note that no generic frames
 * are sent until the send buffer is full. Sending a generic frame can be forced
 * with co_wtm_flush().
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param nif the CAN interface indicator (in the range [1..127]).
 * @param msg a pointer to the CAN frame to be sent.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_send(co_wtm_t *wtm, uint_least8_t nif, const struct can_msg *msg);

/**
 * Sends a keep-alive message from a CANopen WTM interface. This function MAY
 * invoke the callback function set by co_wtm_set_send_func().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_send_alive(co_wtm_t *wtm);

/**
 * Sends a CAN communication quality request. The function specified to
 * co_wtm_set_diag_can_con() is invoked if a response is received.
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param nif the remote CAN interface indicator (in the range [1..127]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_send_diag_can_req(co_wtm_t *wtm, uint_least8_t nif);

/**
 * Sends a WTM communication quality request. The function specified to
 * co_wtm_set_diag_wtm_con() is invoked if a response is received.
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param nif the remote WTM interface indicator (in the range [1..127]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_send_diag_wtm_req(co_wtm_t *wtm, uint_least8_t nif);

/**
 * Sends a CAN communication quality reset message.
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param nif the remote CAN interface indicator (in the range [1..127]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_send_diag_can_rst(co_wtm_t *wtm, uint_least8_t nif);

/**
 * Sends a WTM communication quality reset message.
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param nif the remote WTM interface indicator (in the range [1..127]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_send_diag_wtm_rst(co_wtm_t *wtm, uint_least8_t nif);

/**
 * Sends a diagnostic abort message from a CANopen WTM interface. This function
 * MAY invoke the callback function set by co_wtm_set_send_func().
 *
 * @param wtm a pointer to a CANopen WTM interface.
 * @param ac  the abort code.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_send_diag_ac(co_wtm_t *wtm, uint_least32_t ac);

/**
 * Flushes the current send buffer of a CANopen WTM interface. This function MAY
 * invoke the callback function set by co_wtm_set_send_func().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_wtm_flush(co_wtm_t *wtm);

/**
 * Retrieves the callback function used to send byte streams from a CANopen WTM
 * interface.
 *
 * @param wtm   a pointer to a CANopen WTM interface.
 * @param pfunc the address at which to store a pointer to the callback function
 *              (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_wtm_set_send_func()
 */
void co_wtm_get_send_func(
		const co_wtm_t *wtm, co_wtm_send_func_t **pfunc, void **pdata);

/**
 * Sets the callback function used to send byte streams from a CANopen WTM
 * interface.
 *
 * @param wtm  a pointer to a CANopen WTM interface.
 * @param func a pointer to the function to be invoked by co_wtm_send().
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see co_wtm_get_send_func()
 */
void co_wtm_set_send_func(co_wtm_t *wtm, co_wtm_send_func_t *func, void *data);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_WTM_H_
