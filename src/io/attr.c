/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * serial I/O attributes functions.
 *
 * @see lely/io/attr.h
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

#include "io.h"

#if !LELY_NO_STDIO

#include <lely/util/errnum.h>

#include <assert.h>

#include "attr.h"

#if _WIN32 || _POSIX_C_SOURCE >= 200112L

int
io_attr_get_speed(const io_attr_t *attr)
{
	assert(attr);

#if _WIN32
	return io_attr_lpDCB(attr)->BaudRate;
#else
	switch (cfgetospeed((const struct termios *)attr)) {
	case B0: return 0;
	case B50: return 50;
	case B75: return 75;
	case B110: return 110;
	case B134: return 134;
	case B150: return 150;
	case B200: return 200;
	case B300: return 300;
	case B600: return 600;
	case B1200: return 1200;
	case B1800: return 1800;
	case B2400: return 2400;
	case B4800: return 4800;
	case B9600: return 9600;
	case B19200: return 19200;
	case B38400: return 38400;
#ifdef B7200
	case B7200: return 7200;
#endif
#ifdef B14400
	case B14400: return 14400;
#endif
#ifdef B57600
	case B57600: return 57600;
#endif
#ifdef B115200
	case B115200: return 115200;
#endif
#ifdef B230400
	case B230400: return 230400;
#endif
#ifdef B460800
	case B460800: return 460800;
#endif
#ifdef B500000
	case B500000: return 500000;
#endif
#ifdef B576000
	case B576000: return 576000;
#endif
#ifdef B921600
	case B921600: return 921600;
#endif
#ifdef B1000000
	case B1000000: return 1000000;
#endif
#ifdef B1152000
	case B1152000: return 1152000;
#endif
#ifdef B2000000
	case B2000000: return 2000000;
#endif
#ifdef B3000000
	case B3000000: return 3000000;
#endif
#ifdef B3500000
	case B3500000: return 3500000;
#endif
#ifdef B4000000
	case B4000000: return 4000000;
#endif
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
#endif
}

int
io_attr_set_speed(io_attr_t *attr, int speed)
{
	assert(attr);

#if _WIN32
	io_attr_lpDCB(attr)->BaudRate = speed;

	return 0;
#else
	speed_t baud;
	switch (speed) {
	case 0: baud = B0; break;
	case 50: baud = B50; break;
	case 75: baud = B75; break;
	case 110: baud = B110; break;
	case 134: baud = B134; break;
	case 150: baud = B150; break;
	case 200: baud = B200; break;
	case 300: baud = B300; break;
	case 600: baud = B600; break;
	case 1200: baud = B1200; break;
	case 1800: baud = B1800; break;
	case 2400: baud = B2400; break;
	case 4800: baud = B4800; break;
	case 9600: baud = B9600; break;
	case 19200: baud = B19200; break;
	case 38400: baud = B38400; break;
#ifdef B7200
	case 7200: baud = B7200; break;
#endif
#ifdef B14400
	case 14400: baud = B14400; break;
#endif
#ifdef B57600
	case 57600: baud = B57600; break;
#endif
#ifdef B115200
	case 115200: baud = B115200; break;
#endif
#ifdef B230400
	case 230400: baud = B230400; break;
#endif
#ifdef B460800
	case 460800: baud = B460800; break;
#endif
#ifdef B500000
	case 500000: baud = B500000; break;
#endif
#ifdef B576000
	case 576000: baud = B576000; break;
#endif
#ifdef B921600
	case 921600: baud = B921600; break;
#endif
#ifdef B1000000
	case 1000000: baud = B1000000; break;
#endif
#ifdef B1152000
	case 1152000: baud = B1152000; break;
#endif
#ifdef B2000000
	case 2000000: baud = B2000000; break;
#endif
#ifdef B3000000
	case 3000000: baud = B3000000; break;
#endif
#ifdef B3500000
	case 3500000: baud = B3500000; break;
#endif
#ifdef B4000000
	case 4000000: baud = B4000000; break;
#endif
	default: set_errnum(ERRNUM_INVAL); return -1;
	}

	if (cfsetispeed((struct termios *)attr, baud) == -1)
		return -1;
	if (cfsetospeed((struct termios *)attr, baud) == -1)
		return -1;

	return 0;
#endif
}

int
io_attr_get_flow_control(const io_attr_t *attr)
{
	assert(attr);

#if _WIN32
	LPDCB lpDCB = io_attr_lpDCB(attr);
	return lpDCB->fOutX || lpDCB->fInX;
#else
	return !!(((const struct termios *)attr)->c_iflag & (IXOFF | IXON));
#endif
}

int
io_attr_set_flow_control(io_attr_t *attr, int flow_control)
{
	assert(attr);

#if _WIN32
	LPDCB lpDCB = io_attr_lpDCB(attr);
	lpDCB->fOutxCtsFlow = FALSE;
	lpDCB->fOutxDsrFlow = FALSE;
	lpDCB->fDtrControl = DTR_CONTROL_ENABLE;
	lpDCB->fRtsControl = RTS_CONTROL_ENABLE;
	if (flow_control) {
		lpDCB->fOutX = TRUE;
		lpDCB->fInX = TRUE;
	} else {
		lpDCB->fOutX = FALSE;
		lpDCB->fInX = FALSE;
	}

	return 0;
#else
	struct termios *ios = (struct termios *)attr;
	if (flow_control)
		ios->c_iflag |= IXOFF | IXON;
	else
		ios->c_iflag &= ~(IXOFF | IXON);

	return 0;
#endif
}

int
io_attr_get_parity(const io_attr_t *attr)
{
	assert(attr);

#if _WIN32
	switch (io_attr_lpDCB(attr)->Parity) {
	case EVENPARITY: return IO_PARITY_EVEN;
	case ODDPARITY: return IO_PARITY_ODD;
	default: return IO_PARITY_NONE;
	}
#else
	const struct termios *ios = (const struct termios *)attr;
	if (ios->c_cflag & PARENB)
		return (ios->c_cflag & PARODD) ? IO_PARITY_ODD : IO_PARITY_EVEN;
	return IO_PARITY_NONE;
#endif
}

int
io_attr_set_parity(io_attr_t *attr, int parity)
{
	assert(attr);

#if _WIN32
	LPDCB lpDCB = io_attr_lpDCB(attr);
	switch (parity) {
	case IO_PARITY_NONE:
		lpDCB->fParity = FALSE;
		lpDCB->Parity = NOPARITY;
		return 0;
	case IO_PARITY_ODD:
		lpDCB->fParity = TRUE;
		lpDCB->Parity = ODDPARITY;
		return 0;
	case IO_PARITY_EVEN:
		lpDCB->fParity = TRUE;
		lpDCB->Parity = EVENPARITY;
		return 0;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
#else
	struct termios *ios = (struct termios *)attr;
	switch (parity) {
	case IO_PARITY_NONE:
		ios->c_iflag |= IGNPAR;
		ios->c_cflag &= ~(PARENB | PARODD);
		return 0;
	case IO_PARITY_ODD:
		ios->c_iflag &= ~(IGNPAR | PARMRK);
		ios->c_iflag |= INPCK;
		ios->c_cflag |= PARENB;
		ios->c_cflag &= ~PARODD;
		return 0;
	case IO_PARITY_EVEN:
		ios->c_iflag &= ~(IGNPAR | PARMRK);
		ios->c_iflag |= INPCK;
		ios->c_cflag |= (PARENB | PARODD);
		return 0;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
#endif
}

int
io_attr_get_stop_bits(const io_attr_t *attr)
{
	assert(attr);

#if _WIN32
	return io_attr_lpDCB(attr)->StopBits == TWOSTOPBITS;
#else
	return !!(((const struct termios *)attr)->c_cflag & CSTOPB);
#endif
}

int
io_attr_set_stop_bits(io_attr_t *attr, int stop_bits)
{
	assert(attr);

#if _WIN32
	LPDCB lpDCB = io_attr_lpDCB(attr);
	if (stop_bits)
		lpDCB->StopBits = TWOSTOPBITS;
	else
		lpDCB->StopBits = ONESTOPBIT;

	return 0;
#else
	struct termios *ios = (struct termios *)attr;
	if (stop_bits)
		ios->c_cflag |= CSTOPB;
	else
		ios->c_cflag &= ~CSTOPB;

	return 0;
#endif
}

int
io_attr_get_char_size(const io_attr_t *attr)
{
	assert(attr);

#if _WIN32
	return io_attr_lpDCB(attr)->ByteSize;
#else
	switch (((const struct termios *)attr)->c_cflag & CSIZE) {
	case CS5: return 5;
	case CS6: return 6;
	case CS7: return 7;
	case CS8: return 8;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
#endif
}

int
io_attr_set_char_size(io_attr_t *attr, int char_size)
{
	assert(attr);

#if _WIN32
	io_attr_lpDCB(attr)->ByteSize = char_size;

	return 0;
#else
	struct termios *ios = (struct termios *)attr;
	ios->c_cflag &= ~CSIZE;
	switch (char_size) {
	case 5: ios->c_cflag |= CS5; return 0;
	case 6: ios->c_cflag |= CS6; return 0;
	case 7: ios->c_cflag |= CS7; return 0;
	case 8: ios->c_cflag |= CS8; return 0;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
#endif
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

#endif // !LELY_NO_STDIO
