/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * serial I/O functions.
 *
 * @see lely/io/serial.h
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

#include "io.h"

#if !LELY_NO_STDIO

#include <lely/io/serial.h>

#include <assert.h>
#include <string.h>

#include "attr.h"
#include "default.h"

#if _WIN32 || _POSIX_C_SOURCE >= 200112L

static int serial_flush(struct io_handle *handle);
static int serial_purge(struct io_handle *handle, int flags);

static const struct io_handle_vtab serial_vtab = { .type = IO_TYPE_SERIAL,
	.size = sizeof(struct io_handle),
	.fini = &default_fini,
	.flags = &default_flags,
	.read = &default_read,
	.write = &default_write,
	.flush = &serial_flush,
	.purge = &serial_purge };

io_handle_t
io_open_serial(const char *path, io_attr_t *attr)
{
	assert(path);

	int errc = 0;

#if _WIN32
	HANDLE fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (fd == INVALID_HANDLE_VALUE) {
		errc = get_errc();
		goto error_CreateFile;
	}

	if (!SetCommMask(fd, EV_RXCHAR)) {
		errc = get_errc();
		goto error_SetCommMask;
	}

	DCB DCB;
	memset(&DCB, 0, sizeof(DCB));
	DCB.DCBlength = sizeof(DCB);
	if (!GetCommState(fd, &DCB)) {
		errc = get_errc();
		goto error_GetCommState;
	}

	COMMTIMEOUTS CommTimeouts;
	if (!GetCommTimeouts(fd, &CommTimeouts)) {
		errc = get_errc();
		goto error_GetCommTimeouts;
	}

	if (attr) {
		*io_attr_lpDCB(attr) = DCB;
		*io_attr_lpCommTimeouts(attr) = CommTimeouts;
	}

	DCB.fBinary = TRUE;
	DCB.fParity = FALSE;
	DCB.fOutxCtsFlow = FALSE;
	DCB.fOutxDsrFlow = FALSE;
	DCB.fDtrControl = DTR_CONTROL_ENABLE;
	DCB.fDsrSensitivity = FALSE;
	DCB.fTXContinueOnXoff = TRUE;
	DCB.fOutX = FALSE;
	DCB.fInX = FALSE;
	DCB.fErrorChar = FALSE;
	DCB.fNull = FALSE;
	DCB.fRtsControl = RTS_CONTROL_ENABLE;
	DCB.fAbortOnError = TRUE;
	DCB.ByteSize = 8;
	DCB.Parity = NOPARITY;

	if (!SetCommState(fd, &DCB)) {
		errc = get_errc();
		goto error_SetCommState;
	}

	// Block on reading by waiting as long as possible (MAXDWORD - 1
	// milliseconds) until at least one bytes arrives. serial_read()
	// contains a loop to make this time infinite.
	CommTimeouts.ReadIntervalTimeout = MAXDWORD;
	CommTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
	CommTimeouts.ReadTotalTimeoutConstant = MAXDWORD - 1;
	// Do not use timeouts for write operations.
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(fd, &CommTimeouts)) {
		errc = get_errc();
		goto error_SetCommTimeouts;
	}
#else
	int fd;
	int errsv = errno;
	do {
		errno = errsv;
		fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
	} while (fd == -1 && errno == EINTR);
	if (fd == -1) {
		errc = get_errc();
		goto error_open;
	}

	struct termios ios;
	if (tcgetattr(fd, &ios) == -1) {
		errc = get_errc();
		goto error_tcgetattr;
	}

	if (attr)
		*(struct termios *)attr = ios;

	// These options are taken from cfmakeraw() on BSD.
	ios.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | ISTRIP | IXON
			| PARMRK);
	ios.c_oflag &= ~OPOST;
	ios.c_cflag &= ~(CSIZE | PARENB);
	ios.c_cflag |= CS8;
	ios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

	ios.c_iflag |= IGNPAR;
	ios.c_cflag |= CREAD | CLOCAL;

	ios.c_cc[VMIN] = 1;
	ios.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSANOW, &ios) == -1) {
		errc = get_errc();
		goto error_tcsetattr;
	}
#endif

	struct io_handle *handle = io_handle_alloc(&serial_vtab);
	if (!handle) {
		errc = get_errc();
		goto error_alloc_handle;
	}

	handle->fd = fd;

	return io_handle_acquire(handle);

error_alloc_handle:
#if _WIN32
error_SetCommTimeouts:
error_SetCommState:
error_GetCommTimeouts:
error_GetCommState:
error_SetCommMask:
	CloseHandle(fd);
error_CreateFile:
#else
error_tcsetattr:
error_tcgetattr:
	close(fd);
error_open:
#endif
	set_errc(errc);
	return IO_HANDLE_ERROR;
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

int
io_purge(io_handle_t handle, int flags)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (!handle->vtab->purge) {
		set_errnum(ERRNUM_NOTTY);
		return -1;
	}

	return handle->vtab->purge(handle, flags);
}

#if _WIN32 || _POSIX_C_SOURCE >= 200112L

int
io_serial_get_attr(io_handle_t handle, io_attr_t *attr)
{
	assert(attr);

	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#if _WIN32
	LPDCB lpDCB = io_attr_lpDCB(attr);
	memset(lpDCB, 0, sizeof(*lpDCB));
	lpDCB->DCBlength = sizeof(*lpDCB);
	if (!GetCommState(handle->fd, lpDCB))
		return -1;

	// clang-format off
	return GetCommTimeouts(handle->fd, io_attr_lpCommTimeouts(attr))
			? 0 : -1;
	// clang-format on
#else
	return tcgetattr(handle->fd, (struct termios *)attr);
#endif
}

int
io_serial_set_attr(io_handle_t handle, const io_attr_t *attr)
{
	assert(attr);

	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#if _WIN32
	if (!SetCommState(handle->fd, io_attr_lpDCB(attr)))
		return -1;

	if (!SetCommTimeouts(handle->fd, io_attr_lpCommTimeouts(attr)))
		return -1;

	return 0;
#else
	int result;
	int errsv = errno;
	do {
		errno = errsv;
		result = tcsetattr(handle->fd, TCSANOW,
				(const struct termios *)attr);
	} while (result == -1 && errno == EINTR);
	return result;
#endif
}

static int
serial_flush(struct io_handle *handle)
{
	assert(handle);

#if _WIN32
	return FlushFileBuffers(handle->fd) ? 0 : -1;
#else
	int result;
	int errsv = errno;
	do {
		errno = errsv;
		result = tcdrain(handle->fd);
	} while (result == -1 && errno == EINTR);
	return result;
#endif
}

static int
serial_purge(struct io_handle *handle, int flags)
{
	assert(handle);

#if _WIN32
	DWORD dwFlags = 0;
	if (flags & IO_PURGE_RX)
		dwFlags |= PURGE_RXABORT | PURGE_RXCLEAR;
	if (flags & IO_PURGE_TX)
		dwFlags |= PURGE_TXABORT | PURGE_TXCLEAR;

	return PurgeComm(handle->fd, dwFlags) ? 0 : -1;
#else
	// clang-format off
	return tcflush(handle->fd, (flags & IO_PURGE_RX)
			? (flags & IO_PURGE_TX) ? TCIOFLUSH : TCIFLUSH
			: (flags & IO_PURGE_TX) ? TCOFLUSH : 0);
	// clang-format on
#endif
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

#endif // !LELY_NO_STDIO
