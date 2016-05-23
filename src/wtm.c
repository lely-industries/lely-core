/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the Wireless Transmission Media (WTM) functions.
 *
 * \see lely/co/wtm.h
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

#include "co.h"

#ifndef LELY_NO_CO_WTM

#include <lely/util/diag.h>
#include <lely/util/endian.h>
#include <lely/util/time.h>
#include <lely/co/crc.h>
#include <lely/co/wtm.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

//! The maximum value of a CAN/WTM interface indicator.
#define CO_WTM_MAX_NIF	127

//! A CANopen WTM CAN interface.
struct co_wtm_can {
	//! The current time of the CAN frame receiver.
	struct timespec recv_time;
	//! The current time of the CAN frame sender.
	struct timespec send_time;
	//! The time at which the next frame is sent.
	struct timespec send_next;
};

//! A CANopen Wireless Transmission Media (WTM) interface.
struct __co_wtm {
	//! The WTM interface indicator.
	uint8_t nif;
	//! The CAN interfaces.
	struct co_wtm_can can[CO_WTM_MAX_NIF];
	/*!
	 * A pointer to the callback function invoked when an abort code is
	 * generated or received.
	 */
	co_wtm_diag_func_t *diag_func;
	//! A pointer to the user-specified data for #diag_func.
	void *diag_data;
	//! A pointer to the callback function invoked by co_wtm_recv().
	co_wtm_recv_func_t *recv_func;
	//! A pointer to the user-specified data for #recv_func.
	void *recv_data;
	//! A pointer to the callback function invoked by co_wtm_send().
	co_wtm_send_func_t *send_func;
	//! A pointer to the user-specified data for #send_func.
	void *send_data;
	//! The buffer used to receive byte streams.
	uint8_t recv_buf[CO_WTM_MAX_LEN];
	//! The number of bytes in #recv_buf.
	size_t recv_nbytes;
	//! The sequence number for received generic frames.
	uint8_t recv_nseq;
	//! The buffer used to send byte streams.
	uint8_t send_buf[CO_WTM_MAX_LEN];
	//! The number of bytes in #send_buf.
	size_t send_nbytes;
	//! The sequence number for sent generic frames.
	uint8_t send_nseq;
};

static void co_wtm_diag(co_wtm_t *wtm, uint32_t ac);

//! The default diagnostic callback function. \see co_wtm_diag_func_t
static void default_wtm_diag_func(co_wtm_t *wtm, uint32_t ac, void *data);

/*!
 * Processes a generic frame containing CAN messages.
 *
 * \param wtm    a pointer to a CANopen WTM interface.
 * \param buf    a pointer to the payload of the generic frame.
 * \param nbytes the number of bytes at \a buf.
 *
 * \returns 0 on success, or a WTM abort code on error.
 */
static uint32_t co_wtm_recv_can(co_wtm_t *wtm, const void *buf, size_t nbytes);

LELY_CO_EXPORT const char *
co_wtm_ac_str(uint32_t ac)
{
	switch (ac) {
	case CO_WTM_AC_ERROR:
		return "General error";
	case CO_WTM_AC_TIMEOUT:
		return "Diagnostic protocol timed out limit reached";
	case CO_WTM_AC_NO_MEM:
		return "Out of memory";
	case CO_WTM_AC_HARDWARE:
		return "Access failed due to a hardware error";
	case CO_WTM_AC_DATA:
		return "Data cannot be transferred or stored to the application";
	case CO_WTM_AC_DATA_CTL:
		return "Data cannot be transferred or stored to the application because of local control";
	case CO_WTM_AC_DATA_DEV:
		return "Data cannot be transferred or stored to the application because of the present device state";
	case CO_WTM_AC_NO_DATA:
		return "No data available";
	case CO_WTM_AC_NO_IF:
		return "Requested interface not implemented";
	case CO_WTM_AC_IF_DOWN:
		return "Requested interface disabled";
	case CO_WTM_AC_DIAG:
		return "Diagnostic data generation not supported";
	case CO_WTM_AC_DIAG_CAN:
		return "Diagnostic data generation for requested CAN interface not supported";
	case CO_WTM_AC_DIAG_WTM:
		return "Diagnostic data generation for requested WTM interface not supported";
	case CO_WTM_AC_FRAME:
		return "General generic frame error";
	case CO_WTM_AC_PREAMBLE:
		return "Invalid generic frame preamble";
	case CO_WTM_AC_SEQ:
		return "Invalid sequence counter in generic frame";
	case CO_WTM_AC_TYPE:
		return "Message type not valid or unknown";
	case CO_WTM_AC_PAYLOAD:
		return "Payload field in generic frame invalid";
	case CO_WTM_AC_CRC:
		return "CRC error (Generic frame)";
	case CO_WTM_AC_CAN:
		return "CAN telegram essentials invalid";
	default:
		return "Unknown abort code";
	}
}

LELY_CO_EXPORT void *
__co_wtm_alloc(void)
{
	return malloc(sizeof(struct __co_wtm));
}

LELY_CO_EXPORT void
__co_wtm_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_wtm *
__co_wtm_init(struct __co_wtm *wtm)
{
	assert(wtm);

	wtm->nif = 1;

	for (uint8_t nif = 1; nif <= CO_WTM_MAX_NIF; nif++) {
		wtm->can[nif - 1].recv_time = (struct timespec){ 0, 0 };
		wtm->can[nif - 1].send_time = (struct timespec){ 0, 0 };
		wtm->can[nif - 1].send_next = (struct timespec){ 0, 0 };
	}

	wtm->diag_func = &default_wtm_diag_func;
	wtm->diag_data = NULL;

	wtm->recv_func = NULL;
	wtm->recv_data = NULL;

	wtm->send_func = NULL;
	wtm->send_data = NULL;

	wtm->recv_nbytes = 0;
	wtm->recv_nseq = 0;

	wtm->send_nbytes = 0;
	wtm->send_nseq = 0;

	return wtm;
}

LELY_CO_EXPORT void
__co_wtm_fini(struct __co_wtm *wtm)
{
	__unused_var(wtm);
}

LELY_CO_EXPORT co_wtm_t *
co_wtm_create(void)
{
	errc_t errc = 0;

	co_wtm_t *wtm = __co_wtm_alloc();
	if (__unlikely(!wtm)) {
		errc = get_errc();
		goto error_alloc_wtm;
	}

	if (__unlikely(!__co_wtm_init(wtm))) {
		errc = get_errc();
		goto error_init_wtm;
	}

	return wtm;

error_init_wtm:
	__co_wtm_free(wtm);
error_alloc_wtm:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
co_wtm_destroy(co_wtm_t *wtm)
{
	if (wtm) {
		__co_wtm_fini(wtm);
		__co_wtm_free(wtm);
	}
}

LELY_CO_EXPORT uint8_t
co_wtm_get_nif(const co_wtm_t *wtm)
{
	assert(wtm);

	return wtm->nif;
}

LELY_CO_EXPORT int
co_wtm_set_nif(co_wtm_t *wtm, uint8_t nif)
{
	assert(wtm);

	if (__unlikely(!nif || nif > CO_WTM_MAX_NIF)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	wtm->nif = nif;

	return 0;
}

LELY_CO_EXPORT void
co_wtm_get_diag_func(const co_wtm_t *wtm, co_wtm_diag_func_t **pfunc,
		void **pdata)
{
	assert(wtm);

	if (pfunc)
		*pfunc = wtm->diag_func;
	if (pdata)
		*pdata = wtm->diag_data;
}

LELY_CO_EXPORT void
co_wtm_set_diag_func(co_wtm_t *wtm, co_wtm_diag_func_t *func, void *data)
{
	assert(wtm);

	wtm->diag_func = func ? func : &default_wtm_diag_func;
	wtm->diag_data = func ? data : NULL;
}

LELY_CO_EXPORT void
co_wtm_recv(co_wtm_t *wtm, const void *buf, size_t nbytes)
{
	assert(wtm);
	assert(wtm->diag_func);
	assert(buf);

	for (const char *bp = buf; nbytes;) {
		uint32_t ac = 0;
		// Search for the preamble (see section 5.2 in CiA 315 version
		// 1.0.0).
		size_t size = 1;
		if (wtm->recv_nbytes < size) {
			wtm->recv_buf[wtm->recv_nbytes] = *bp;
			wtm->recv_nbytes++; bp++; nbytes--;
		}
		if (__unlikely(wtm->recv_buf[0] != 0x55)) {
			ac = CO_WTM_AC_PREAMBLE;
			goto error;
		}
		// Copy the rest of the header (plus the CRC checksum if there
		// is no payload).
		size += 5;
		if (wtm->recv_nbytes < size) {
			size_t n = MIN(nbytes, size - wtm->recv_nbytes);
			memcpy(wtm->recv_buf + wtm->recv_nbytes, bp, n);
			wtm->recv_nbytes += n; bp += n; nbytes -= n;
			if (wtm->recv_nbytes < size)
				continue;
		}
		// Copy the payload (plus the CRC checksum).
		uint8_t len = wtm->recv_buf[1];
		size += len;
		if (wtm->recv_nbytes < size) {
			size_t n = MIN(nbytes, size - wtm->recv_nbytes);
			memcpy(wtm->recv_buf + wtm->recv_nbytes, bp, n);
			wtm->recv_nbytes += n; bp += n; nbytes -= n;
			if (wtm->recv_nbytes < size)
				continue;
		}
		// Check the CRC checksum (see section 5.7 in CiA 315 version
		// 1.0.0).
		uint16_t crc = co_crc(0xffff, wtm->recv_buf, 4 + len);
		if (__unlikely(crc != ldle_u16(wtm->recv_buf + 4 + len))) {
			ac = CO_WTM_AC_CRC;
			goto error;
		}
		// Check the sequence counter (see section 5.4 in CiA 315
		// version 1.0.0).
		uint8_t seq = wtm->recv_buf[2];
		if (__unlikely(seq != wtm->recv_nseq))
			// Generate an error, but do not abort processing the
			// message.
			co_wtm_diag(wtm, CO_WTM_AC_CRC);
		wtm->recv_nseq = seq + 1;
		// Process message payload based on its type (see Table 2 in CiA
		// 315 version 1.0.0).
		uint8_t type = wtm->recv_buf[3];
		uint8_t nif;
		switch (type) {
		// CAN messages forwarding (see section 6 in CiA 315 version
		// 1.0.0).
		case 0x00:
			// Process the CAN frames.
			if (__unlikely(ac = co_wtm_recv_can(wtm,
					wtm->recv_buf + 4, len)))
				goto error;
			break;
		// Keep-alive (see section 7.3 in CiA 315 version 1.0.0).
		case 0x10:
			if (__unlikely(len < 1)) {
				ac = CO_WTM_AC_PAYLOAD;
				goto error;
			}
			// Obtain the WTM interface indicator.
			nif = wtm->recv_buf[4];
			if (__unlikely(nif <= 0x80)) {
				ac = CO_WTM_AC_PAYLOAD;
				goto error;
			}
			// Ignore keep-alive messages for other WTM interfaces.
			if (__unlikely(nif != 0x80 + wtm->nif))
				break;
			// TODO: handle keep-alive message.
			break;
		// Timer-overrun (see section 7.4 in CiA 315 version 1.0.0).
		case 0x11:
			if (__unlikely(len < 1)) {
				ac = CO_WTM_AC_PAYLOAD;
				goto error;
			}
			// Obtain the CAN interface indicator.
			nif = wtm->recv_buf[4];
			if (__unlikely(!nif || nif > CO_WTM_MAX_NIF)) {
				ac = CO_WTM_AC_PAYLOAD;
				goto error;
			}
			// Add 6553.5 ms to the CAN interface timer.
			timespec_add_usec(&wtm->can[nif - 1].recv_time,
					6553500);
			break;
		// Communication quality request.
		case 0x12:
		// Communication quality response.
		case 0x13:
		// Communication quality reset.
		case 0x14:
			// TODO: implement diagnostic protocol.
			co_wtm_send_abort(wtm, CO_WTM_AC_DIAG);
			break;
		// Diagnostic abort message.
		case 0x15:
			if (__unlikely(len < 5)) {
				ac = CO_WTM_AC_PAYLOAD;
				goto error;
			}
			// Obtain the WTM interface indicator.
			nif = wtm->recv_buf[4];
			if (__unlikely(nif <= 0x80)) {
				ac = CO_WTM_AC_PAYLOAD;
				goto error;
			}
			// Ignore diagnostic abort messages for other WTM
			// interfaces.
			if (__unlikely(nif != 0x80 + wtm->nif))
				break;
			ac = ldle_u32(wtm->recv_buf + 5);
			break;
		default:
			ac = CO_WTM_AC_TYPE;
			goto error;
		}
	error:
		if (__unlikely(ac)) {
			co_wtm_diag(wtm, ac);
			ac = 0;
		}
		// Empty the buffer for the next message.
		wtm->recv_nbytes = 0;
	}
}

LELY_CO_EXPORT void
co_wtm_get_recv_func(const co_wtm_t *wtm, co_wtm_recv_func_t **pfunc,
		void **pdata)
{
	assert(wtm);

	if (pfunc)
		*pfunc = wtm->recv_func;
	if (pdata)
		*pdata = wtm->recv_data;
}

LELY_CO_EXPORT void
co_wtm_set_recv_func(co_wtm_t *wtm, co_wtm_recv_func_t *func, void *data)
{
	assert(wtm);

	wtm->recv_func = func;
	wtm->recv_data = data;
}

LELY_CO_EXPORT int
co_wtm_get_time(const co_wtm_t *wtm, uint8_t nif, struct timespec *tp)
{
	assert(wtm);

	if (__unlikely(!nif || nif > CO_WTM_MAX_NIF)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (tp)
		*tp = wtm->can[nif - 1].send_next;

	return 0;
}

LELY_CO_EXPORT int
co_wtm_set_time(co_wtm_t *wtm, uint8_t nif, const struct timespec *tp)
{
	assert(wtm);
	assert(tp);

	if (__unlikely(!nif || nif > CO_WTM_MAX_NIF)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	struct co_wtm_can *can = &wtm->can[nif - 1];

	// Since the time stamp is relative, store the absolute time on the
	// first invocation.
	if (!can->send_time.tv_sec && !can->send_time.tv_nsec)
		can->send_time = *tp;
	// Update the time stamp for the next CAN frame.
	can->send_next = *tp;

	// If the difference between the current (reference) and next time is
	// larger than 6553.5 ms, send a timer-overrun message.
	while (timespec_diff_usec(&can->send_next, &can->send_time) > 6553500) {
		if (__unlikely(co_wtm_flush(wtm) == -1))
			return -1;
		wtm->send_buf[3] = 0x11;
		wtm->send_buf[4] = nif;
		wtm->send_nbytes = 5;
		// Update the current (reference) time.
		timespec_add_usec(&can->send_time, 6553500);
	}

	return 0;
}

LELY_CO_EXPORT int
co_wtm_send(co_wtm_t *wtm, uint8_t nif, const struct can_msg *msg)
{
	assert(wtm);
	assert(msg);

	if (__unlikely(!nif || nif > CO_WTM_MAX_NIF)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	struct co_wtm_can *can = &wtm->can[nif - 1];

#ifndef LELY_NO_CANFD
	// CAN FD frames are not supported.
	if (__unlikely(msg->flags & CAN_FLAG_EDL)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
#endif

	if (__unlikely(msg->len > CAN_MAX_LEN)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	// Compute the length of the CAN frame.
	size_t len = 1 + (nif != 1) + ((msg->flags & CAN_FLAG_IDE) ? 4 : 2)
			+ msg->len + 2;
	// Flush the buffer if necessary.
	if ((wtm->send_nbytes > 3 && wtm->send_buf[3] != 0x00)
			|| wtm->send_nbytes + len + 2 > CO_WTM_MAX_LEN) {
		if (__unlikely(co_wtm_flush(wtm) == -1))
			return -1;
	}
	wtm->send_buf[3] = 0x00;
	wtm->send_nbytes = MAX(wtm->send_nbytes, 4);
	uint8_t *bp = wtm->send_buf + wtm->send_nbytes;
	size_t nbytes = wtm->send_nbytes;

	// Write the data length code.
	uint8_t dlc = msg->len | 0x40;
	if (msg->flags & CAN_FLAG_RTR)
		dlc |= 0x10;
	if (msg->flags & CAN_FLAG_IDE)
		dlc |= 0x20;
	if (nif != 1)
		dlc |= 0x80;
	*bp = dlc;
	bp++; nbytes++;
	// Write the interface indicator.
	if (nif != 1) {
		*bp = nif;
		bp++; nbytes++;
	}
	// Write the CAN identifier.
	if (msg->flags & CAN_FLAG_IDE) {
		stle_u32(bp, msg->id & CAN_MASK_EID);
		bp += 4; nbytes += 4;
	} else {
		stle_u16(bp, msg->id & CAN_MASK_BID);
		bp += 2; nbytes += 2;
	}
	// Copy the frame payload.
	memcpy(bp, msg->data, msg->len);
	bp += msg->len; nbytes += msg->len;
	// Write the time stamp.
	int64_t usec = timespec_diff_usec(&can->send_next, &can->send_time);
	stle_u16(bp, usec / 100);
	bp += 2; nbytes += 2;

	assert(nbytes + 2 <= CO_WTM_MAX_LEN);
	wtm->send_nbytes = nbytes;

	return 0;
}

LELY_CO_EXPORT int
co_wtm_send_alive(co_wtm_t *wtm)
{
	assert(wtm);
	assert(wtm->nif && wtm->nif <= CO_WTM_MAX_NIF);

	if (__unlikely(co_wtm_flush(wtm) == -1))
		return -1;
	wtm->send_buf[3] = 0x10;
	wtm->send_buf[4] = 0x80 + wtm->nif;
	wtm->send_nbytes = 5;
	return co_wtm_flush(wtm);
}

LELY_CO_EXPORT int
co_wtm_send_abort(co_wtm_t *wtm, uint32_t ac)
{
	assert(wtm);
	assert(wtm->nif && wtm->nif <= CO_WTM_MAX_NIF);

	if (__unlikely(co_wtm_flush(wtm) == -1))
		return -1;
	wtm->send_buf[3] = 0x15;
	wtm->send_buf[4] = 0x80 + wtm->nif;
	stle_u32(wtm->send_buf + 5, ac);
	wtm->send_nbytes = 9;
	return co_wtm_flush(wtm);
}

LELY_CO_EXPORT int
co_wtm_flush(co_wtm_t *wtm)
{
	assert(wtm);

	// Do not flush if there is no header.
	if (wtm->send_nbytes < 4)
		return 0;
	uint8_t len = wtm->send_nbytes - 4;
	wtm->send_nbytes = 0;

	// Fill in the header fields.
	wtm->send_buf[0] = 0x55;
	wtm->send_buf[1] = len;
	wtm->send_buf[2] = wtm->send_nseq++;
	// Compute the CRC checksum.
	uint16_t crc = co_crc(0xffff, wtm->send_buf, 4 + len);
	stle_u16(wtm->send_buf + 4 + len, crc);

	// Invoke the user-specified callback function to send the generic
	// frame.
	if (__unlikely(!wtm->send_func)) {
		set_errnum(ERRNUM_NOSYS);
		return -1;
	}
	return wtm->send_func(wtm, wtm->send_buf, 4 + len + 2, wtm->send_data)
			? -1 : 0;
}

LELY_CO_EXPORT void
co_wtm_get_send_func(const co_wtm_t *wtm, co_wtm_send_func_t **pfunc,
		void **pdata)
{
	assert(wtm);

	if (pfunc)
		*pfunc = wtm->send_func;
	if (pdata)
		*pdata = wtm->send_data;
}

LELY_CO_EXPORT void
co_wtm_set_send_func(co_wtm_t *wtm, co_wtm_send_func_t *func, void *data)
{
	assert(wtm);

	wtm->send_func = func;
	wtm->send_data = data;
}

static void
co_wtm_diag(co_wtm_t *wtm, uint32_t ac)
{
	assert(wtm);
	assert(wtm->diag_func);

	wtm->diag_func(wtm, ac, wtm->diag_data);
}

static void
default_wtm_diag_func(co_wtm_t *wtm, uint32_t ac, void *data)
{
	__unused_var(wtm);
	__unused_var(data);

	diag(DIAG_WARNING, 0, "received WTM abort code %08X: %s", ac,
			co_wtm_ac_str(ac));
}

static uint32_t
co_wtm_recv_can(co_wtm_t *wtm, const void *buf, size_t nbytes)
{
	assert(wtm);
	assert(buf);
	assert(nbytes);

	uint32_t ac = 0;
	for (const char *bp = buf; nbytes;) {
		struct can_msg msg = CAN_MSG_INIT;
		// Obtain the data length code.
		uint8_t dlc = *bp;
		bp++; nbytes--;
		msg.len = dlc & 0x0f;
		if (__unlikely(msg.len > CAN_MAX_LEN)) {
			ac = CO_WTM_AC_CAN;
			goto error;
		}
		if (dlc & 0x10)
			msg.flags |= CAN_FLAG_RTR;
		// Obtain the CAN interface indicator.
		uint8_t nif = 1;
		if (dlc & 0x80) {
			if (__unlikely(nbytes < 1)) {
				ac = CO_WTM_AC_CAN;
				goto error;
			}
			nif = *bp;
			bp++; nbytes--;
		}
		// Obtain the CAN identifier.
		if (dlc & 0x20) {
			if (__unlikely(nbytes < 4)) {
				ac = CO_WTM_AC_CAN;
				goto error;
			}
			msg.id = ldle_u32(bp) & CAN_MASK_EID;
			bp += 4; nbytes -= 4;
			msg.flags |= CAN_FLAG_IDE;
		} else {
			if (__unlikely(nbytes < 2)) {
				ac = CO_WTM_AC_CAN;
				goto error;
			}
			msg.id = ldle_u16(bp) & CAN_MASK_BID;
			bp += 2; nbytes -= 2;
		}
		// Obtain the frame payload.
		if (__unlikely(nbytes < msg.len)) {
			ac = CO_WTM_AC_CAN;
			goto error;
		}
		memcpy(msg.data, bp, msg.len);
		bp += msg.len; nbytes -= msg.len;
		// Obtain the time stamp.
		uint16_t ts = 0;
		if (dlc & 0x40) {
			if (__unlikely(nbytes < 2)) {
				ac = CO_WTM_AC_CAN;
				goto error;
			}
			ts = ldle_u16(bp);
			bp += 2; nbytes -= 2;
		}
		// Ignore CAN frames with an invalid interface indicator.
		if (__unlikely(!nif || nif > CO_WTM_MAX_NIF))
			continue;
		// Update the CAN interface timer.
		struct timespec *tp = NULL;
		if (dlc & 0x40) {
			tp = &wtm->can[nif - 1].recv_time;
			timespec_add_usec(tp, ts * 100);
		}
		// Invoke the user-specified callback function.
		if (wtm->recv_func) {
			errc_t errc = get_errc();
			if (__unlikely(wtm->recv_func(wtm, nif, tp, &msg,
					wtm->recv_data))) {
				// Convert the error number to a WTM abort code.
				if (!ac) {
					ac = get_errnum() == ERRNUM_NOMEM
							? CO_WTM_AC_NO_MEM
							: CO_WTM_AC_ERROR;
				}
				set_errc(errc);
			}
		}
	}

error:
	return ac;
}

#endif // !LELY_NO_CO_WTM

