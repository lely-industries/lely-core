/*!\file
 * This file is part of the I/O library; it contains the implementation of the
 * the initialization and finalization functions.
 *
 * \see lely/io/io.h
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
#include "handle.h"

#include <assert.h>

static int lely_io_ref;

LELY_IO_EXPORT int
lely_io_init(void)
{
	if (lely_io_ref++)
		return 0;

	errc_t errc = 0;

#ifdef _WIN32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (__unlikely(errc = WSAStartup(wVersionRequested, &wsaData)))
		goto error_WSAStartup;
#endif

	return 0;

#ifdef _WIN32
	WSACleanup();
error_WSAStartup:
#endif
	lely_io_ref--;
	set_errc(errc);
	return -1;
}

LELY_IO_EXPORT void
lely_io_fini(void)
{
	if (__unlikely(lely_io_ref <= 0)) {
		lely_io_ref = 0;
		return;
	}
	if (--lely_io_ref)
		return;

#ifdef _WIN32
	WSACleanup();
#endif
}

LELY_IO_EXPORT int
io_close(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	io_handle_release(handle);

	return 0;
}

LELY_IO_EXPORT ssize_t
io_read(io_handle_t handle, void *buf, size_t nbytes)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->read)) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

	return handle->vtab->read(handle, buf, nbytes);
}

LELY_IO_EXPORT ssize_t
io_write(io_handle_t handle, const void *buf, size_t nbytes)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->write)) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

	return handle->vtab->write(handle, buf, nbytes);
}

