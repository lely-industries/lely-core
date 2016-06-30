/*!\file
 * This file is part of the I/O library; it contains the implementation of the
 * serial I/O functions.
 *
 * \see lely/io/serial.h
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

#include "io.h"
#include <lely/util/errnum.h>
#include <lely/io/serial.h>
#include "attr.h"
#include "handle.h"

#include <assert.h>
#include <string.h>

#if defined(_WIN32) || _POSIX_C_SOURCE >= 200112L

static void serial_fini(struct io_handle *handle);
static int serial_flags(struct io_handle *handle, int flags);
static ssize_t serial_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t serial_write(struct io_handle *handle, const void *buf,
		size_t nbytes);
static int serial_flush(struct io_handle *handle);
static int serial_purge(struct io_handle *handle, int flags);

static const struct io_handle_vtab serial_vtab = {
	.type = IO_TYPE_SERIAL,
	.size = sizeof(struct io_handle),
	.fini = &serial_fini,
	.flags = &serial_flags,
	.read = &serial_read,
	.write = &serial_write,
	.flush = &serial_flush,
	.purge = &serial_purge
};

LELY_IO_EXPORT io_handle_t
io_open_serial(const char *path, io_attr_t *attr)
{
	assert(path);

	errc_t errc = 0;

#ifdef _WIN32
	HANDLE fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (__unlikely(fd == INVALID_HANDLE_VALUE)) {
		errc = get_errc();
		goto error_CreateFile;
	}

	if (__unlikely(!SetCommMask(fd, EV_RXCHAR))) {
		errc = get_errc();
		goto error_SetCommMask;
	}

	DCB DCB;
	memset(&DCB, 0, sizeof(DCB));
	DCB.DCBlength = sizeof(DCB);
	if (__unlikely(!GetCommState(fd, &DCB))) {
		errc = get_errc();
		goto error_GetCommState;
	}

	COMMTIMEOUTS CommTimeouts;
	if (__unlikely(!GetCommTimeouts(fd, &CommTimeouts))) {
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

	if (__unlikely(!SetCommState(fd, &DCB))) {
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

	if (__unlikely(!SetCommTimeouts(fd, &CommTimeouts))) {
		errc = get_errc();
		goto error_SetCommTimeouts;
	}
#else
	int fd;
	do fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
	while (__unlikely(fd == -1 && errno == EINTR));
	if (__unlikely(fd == -1)) {
		errc = get_errc();
		goto error_open;
	}

	struct termios ios;
	if (__unlikely(tcgetattr(fd, &ios) == -1)) {
		errc = get_errc();
		goto error_tcgetattr;
	}

	if (attr)
		*(struct termios *)attr = ios;

	// These options are taken from cfmakeraw() on BSD.
	ios.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | ISTRIP
			| IXON | PARMRK);
	ios.c_oflag &= ~OPOST;
	ios.c_cflag &= ~(CSIZE | PARENB);
	ios.c_cflag |= CS8;
	ios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

	ios.c_iflag |= IGNPAR;
	ios.c_cflag |= CREAD | CLOCAL;

	ios.c_cc[VMIN] = 1;
	ios.c_cc[VTIME] = 0;

	if (__unlikely(tcsetattr(fd, TCSANOW, &ios) == -1)) {
		errc = get_errc();
		goto error_tcsetattr;
	}
#endif

	struct io_handle *handle = io_handle_alloc(&serial_vtab);
	if (__unlikely(!handle)) {
		errc = get_errc();
		goto error_alloc_handle;
	}

	handle->fd = fd;

	return io_handle_acquire(handle);

error_alloc_handle:
#ifdef _WIN32
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

LELY_IO_EXPORT int
io_purge(io_handle_t handle, int flags)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->purge)) {
		set_errnum(ERRNUM_NOTTY);
		return -1;
	}

	return handle->vtab->purge(handle, flags);
}

#if defined(_WIN32) || _POSIX_C_SOURCE >= 200112L

LELY_IO_EXPORT int
io_serial_get_attr(io_handle_t handle, io_attr_t *attr)
{
	assert(attr);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	LPDCB lpDCB = io_attr_lpDCB(attr);
	memset(lpDCB, 0, sizeof(*lpDCB));
	lpDCB->DCBlength = sizeof(*lpDCB);
	if (__unlikely(!GetCommState(handle->fd, lpDCB)))
		return -1;

	return GetCommTimeouts(handle->fd, io_attr_lpCommTimeouts(attr))
			? 0 : -1;
#else
	return tcgetattr(handle->fd, (struct termios *)attr);
#endif
}

LELY_IO_EXPORT int
io_serial_set_attr(io_handle_t handle, const io_attr_t *attr)
{
	assert(attr);

	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

#ifdef _WIN32
	if (__unlikely(!SetCommState(handle->fd, io_attr_lpDCB(attr))))
		return -1;

	if (__unlikely(!SetCommTimeouts(handle->fd,
			io_attr_lpCommTimeouts(attr))))
		return -1;

	return 0;
#else
	int result;
	do result = tcsetattr(handle->fd, TCSANOW,
			(const struct termios *)attr);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

static void
serial_fini(struct io_handle *handle)
{
	assert(handle);

	if (!(handle->flags & IO_FLAG_NO_CLOSE))
#ifdef _WIN32
		CloseHandle(handle->fd);
#else
		close(handle->fd);
#endif
}

static int
serial_flags(struct io_handle *handle, int flags)
{
	assert(handle);

#ifdef _WIN32
	__unused_var(handle);
	__unused_var(flags);

	return 0;
#else
	int arg = fcntl(handle->fd, F_GETFL, 0);
	if (__unlikely(arg == -1))
		return -1;

	if (flags & IO_FLAG_NONBLOCK)
		arg |= O_NONBLOCK;
	else
		arg &= ~O_NONBLOCK;

	return fcntl(handle->fd, F_SETFL, arg);
#endif
}

static ssize_t
serial_read(struct io_handle *handle, void *buf, size_t nbytes)
{
	assert(handle);

#ifdef _WIN32
	DWORD dwErrCode = GetLastError();

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (__unlikely(!overlapped.hEvent)) {
		dwErrCode = GetLastError();
		goto error_CreateEvent;
	}

	DWORD dwNumberOfBytesRead = 0;

retry:
	io_handle_lock(handle);
	int flags = handle->flags;
	io_handle_unlock(handle);

	if (ReadFile(handle->fd, buf, nbytes, &dwNumberOfBytesRead,
			&overlapped))
		goto done;

	switch (GetLastError()) {
	case ERROR_IO_PENDING:
		break;
	case ERROR_OPERATION_ABORTED:
		if (__likely(ClearCommError(handle->fd, NULL, NULL)))
			goto retry;
	default:
		dwErrCode = GetLastError();
		goto error_ReadFile;
	}

	if ((flags & IO_FLAG_NONBLOCK) && __unlikely(!CancelIoEx(handle->fd,
			&overlapped) && GetLastError() == ERROR_NOT_FOUND)) {
		dwErrCode = GetLastError();
		goto error_CancelIoEx;
	}

	if (__unlikely(!GetOverlappedResult(handle->fd, &overlapped,
			&dwNumberOfBytesRead, TRUE))) {
		dwErrCode = GetLastError();
		goto error_GetOverlappedResult;
	}

	if (__unlikely(nbytes && !dwNumberOfBytesRead)) {
		if (!(flags & IO_FLAG_NONBLOCK))
			goto retry;
		dwErrCode = errnum2c(ERRNUM_AGAIN);
		goto error_dwNumberOfBytesRead;
	}

done:
	CloseHandle(overlapped.hEvent);
	SetLastError(dwErrCode);
	return dwNumberOfBytesRead;

error_dwNumberOfBytesRead:
error_GetOverlappedResult:
error_CancelIoEx:
error_ReadFile:
	CloseHandle(overlapped.hEvent);
error_CreateEvent:
	SetLastError(dwErrCode);
	return -1;
#else
	ssize_t result;
	do result = read(handle->fd, buf, nbytes);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

static ssize_t
serial_write(struct io_handle *handle, const void *buf, size_t nbytes)
{
	assert(handle);

#ifdef _WIN32
	DWORD dwErrCode = GetLastError();

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (__unlikely(!overlapped.hEvent)) {
		dwErrCode = GetLastError();
		goto error_CreateEvent;
	}

	DWORD dwNumberOfBytesWritten = 0;

retry:
	io_handle_lock(handle);
	int flags = handle->flags;
	io_handle_unlock(handle);

	if (WriteFile(handle->fd, buf, nbytes, &dwNumberOfBytesWritten,
			&overlapped))
		goto done;

	switch (GetLastError()) {
	case ERROR_IO_PENDING:
		break;
	case ERROR_OPERATION_ABORTED:
		if (__likely(ClearCommError(handle->fd, NULL, NULL)))
			goto retry;
	default:
		dwErrCode = GetLastError();
		goto error_WriteFile;
	}

	if ((flags & IO_FLAG_NONBLOCK) && __unlikely(!CancelIoEx(handle->fd,
			&overlapped) && GetLastError() == ERROR_NOT_FOUND)) {
		dwErrCode = GetLastError();
		goto error_CancelIoEx;
	}

	if (__unlikely(!GetOverlappedResult(handle->fd, &overlapped,
			&dwNumberOfBytesWritten, TRUE))) {
		dwErrCode = GetLastError();
		goto error_GetOverlappedResult;
	}

	if (__unlikely(nbytes && !dwNumberOfBytesWritten)) {
		if (!(flags & IO_FLAG_NONBLOCK))
			goto retry;
		dwErrCode = errnum2c(ERRNUM_AGAIN);
		goto error_dwNumberOfBytesWritten;
	}

done:
	CloseHandle(overlapped.hEvent);
	SetLastError(dwErrCode);
	return dwNumberOfBytesWritten;

error_dwNumberOfBytesWritten:
error_GetOverlappedResult:
error_CancelIoEx:
error_WriteFile:
	CloseHandle(overlapped.hEvent);
error_CreateEvent:
	SetLastError(dwErrCode);
	return -1;
#else
	ssize_t result;
	do result = write(handle->fd, buf, nbytes);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

static int
serial_flush(struct io_handle *handle)
{
	assert(handle);

#ifdef _WIN32
	return FlushFileBuffers(handle->fd) ? 0 : -1;
#else
	int result;
	do result = tcdrain(handle->fd);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

static int
serial_purge(struct io_handle *handle, int flags)
{
	assert(handle);

#ifdef _WIN32
	DWORD dwFlags = 0;
	if (flags & IO_PURGE_RX)
		dwFlags |= PURGE_RXABORT | PURGE_RXCLEAR;
	if (flags & IO_PURGE_TX)
		dwFlags |= PURGE_TXABORT | PURGE_TXCLEAR;

	return PurgeComm(handle->fd, dwFlags) ? 0 : -1;
#else
	return tcflush(handle->fd, (flags & IO_PURGE_RX)
			? (flags & IO_PURGE_TX) ? TCIOFLUSH : TCIFLUSH
			: (flags & IO_PURGE_TX) ? TCOFLUSH : 0);
#endif
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

