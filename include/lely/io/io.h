/*!\file
 * This is the public header file of the I/O library.
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

#ifndef LELY_IO_IO_H
#define LELY_IO_IO_H

#include <lely/libc/libc.h>
#include <lely/util/util.h>

#ifndef LELY_IO_EXTERN
#ifdef DLL_EXPORT
#ifdef LELY_IO_INTERN
#define LELY_IO_EXTERN	extern __dllexport
#else
#define LELY_IO_EXTERN	extern __dllimport
#endif
#else
#define LELY_IO_EXTERN	extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initializes the I/O library and makes the I/O functions available for use.
 * On Windows this function invokes `WSAStartup()`. This function is not thread
 * safe, but can be invoked multiple times, as long as it is matched by an equal
 * number of calls to lely_io_fini().
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN int lely_io_init(void);

/*!
 * Finalizes the I/O library and terminates the availability of the I/O
 * functions. On Windows this function invokes `WSACleanup()`. Note that this
 * function MUST be invoked once for each corresponding call to lely_io_init().
 * Only the last invocation will finalize the library.
 */
LELY_IO_EXTERN void lely_io_fini(void);

#ifdef __cplusplus
}
#endif

#endif

