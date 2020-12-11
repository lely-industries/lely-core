/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * the initialization and finalization functions.
 *
 * @see lely/io/io.h
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

#include "handle.h"

static int lely_io_ref;

int
lely_io_init(void)
{
	if (lely_io_ref++)
		return 0;

#if _WIN32
	int errc = 0;

	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if ((errc = WSAStartup(wVersionRequested, &wsaData)))
		goto error_WSAStartup;
#endif

	return 0;

#if _WIN32
	// WSACleanup();
error_WSAStartup:
	lely_io_ref--;
	set_errc(errc);
	return -1;
#endif
}

void
lely_io_fini(void)
{
	if (lely_io_ref <= 0) {
		lely_io_ref = 0;
		return;
	}
	if (--lely_io_ref)
		return;

#if _WIN32
	WSACleanup();
#endif
}

int
io_close(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	io_handle_release(handle);

	return 0;
}

int
io_get_type(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	return handle->vtab->type;
}

HANDLE
io_get_fd(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return INVALID_HANDLE_VALUE;
	}

	return handle->fd;
}

int
io_get_flags(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	io_handle_lock(handle);
	int flags = handle->flags;
	io_handle_unlock(handle);
	return flags;
}

int
io_set_flags(io_handle_t handle, int flags)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	flags &= (IO_FLAG_NO_CLOSE | IO_FLAG_NONBLOCK | IO_FLAG_LOOPBACK);

	int result = 0;
	io_handle_lock(handle);
	if (flags != handle->flags) {
		if (handle->vtab->flags) {
			result = handle->vtab->flags(handle, flags);
			if (!result)
				handle->flags = flags;
		} else {
			set_errnum(ERRNUM_NXIO);
			result = -1;
		}
	}
	io_handle_unlock(handle);
	return result;
}

ssize_t
io_read(io_handle_t handle, void *buf, size_t nbytes)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (!handle->vtab->read) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

	return handle->vtab->read(handle, buf, nbytes);
}

ssize_t
io_write(io_handle_t handle, const void *buf, size_t nbytes)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (!handle->vtab->write) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

	return handle->vtab->write(handle, buf, nbytes);
}

int
io_flush(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (!handle->vtab->flush) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

	return handle->vtab->flush(handle);
}

#endif // !LELY_NO_STDIO
