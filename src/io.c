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

