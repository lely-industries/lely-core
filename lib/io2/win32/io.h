/**@file
 * This is the internal header file of the Windows-specific I/O declarations.
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_IO2_INTERN_WIN32_IO_H_
#define LELY_IO2_INTERN_WIN32_IO_H_

#include "../sys/io.h"

#if _WIN32

#include <winternl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG(NTAPI *LPFN_RTLNTSTATUSTODOSERROR)(NTSTATUS Status);
extern LPFN_RTLNTSTATUSTODOSERROR lpfnRtlNtStatusToDosError;

int io_win32_ntdll_init(void);
void io_win32_ntdll_fini(void);

int io_win32_sigset_init(void);
void io_win32_sigset_fini(void);

#ifdef __cplusplus
}
#endif

#endif // _WIN32

#endif // LELY_IO2_INTERN_WIN32_IO_H_
