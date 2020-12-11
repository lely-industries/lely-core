/**@file
 * This file is part of the I/O library; it contains the Windows implementation
 * of the I/O initialization/finalization functions.
 *
 * @see lely/io2/sys/io.h
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#if !LELY_NO_STDIO && _WIN32

#include <lely/io2/sys/io.h>

#include <assert.h>

#include <windows.h>

LPFN_RTLNTSTATUSTODOSERROR lpfnRtlNtStatusToDosError;

static size_t io_init_refcnt = 0;

int
io_init(void)
{
	if (io_init_refcnt++)
		return 0;

	DWORD dwErrCode = 0;

	if (io_win32_ntdll_init() == -1) {
		dwErrCode = GetLastError();
		goto error_ntdll_init;
	}

	if (io_win32_sigset_init() == -1) {
		dwErrCode = GetLastError();
		goto error_sigset_init;
	}

	return 0;

	// io_win32_sigset_fini();
error_sigset_init:
	io_win32_ntdll_fini();
error_ntdll_init:
	SetLastError(dwErrCode);
	io_init_refcnt--;
	return -1;
}

void
io_fini(void)
{
	assert(io_init_refcnt);

	if (--io_init_refcnt)
		return;

	io_win32_sigset_fini();
	io_win32_ntdll_fini();
}

int
io_win32_ntdll_init(void)
{
	HMODULE hLibModule = GetModuleHandleA("ntdll.dll");
	if (!hLibModule)
		goto error_ntdll;

#if __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
	lpfnRtlNtStatusToDosError = (LPFN_RTLNTSTATUSTODOSERROR)GetProcAddress(
			hLibModule, "RtlNtStatusToDosError");
	if (!lpfnRtlNtStatusToDosError)
		goto error_RtlNtStatusToDosError;
#if __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif

	return 0;

	// lpfnRtlNtStatusToDosError = NULL;
error_RtlNtStatusToDosError:
error_ntdll:
	return -1;
}

void
io_win32_ntdll_fini(void)
{
	lpfnRtlNtStatusToDosError = NULL;
}

#endif // !LELY_NO_STDIO && _WIN32
