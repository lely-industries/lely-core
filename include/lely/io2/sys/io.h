/**@file
 * This header file is part of the I/O library; it contains system-dependent I/O
 * initialization/finalization function declarations.
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

#ifndef LELY_IO2_SYS_IO_H_
#define LELY_IO2_SYS_IO_H_

#include <lely/io2/io2.h>

/// The system-dependent I/O polling interface.
typedef struct io_poll io_poll_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the I/O library and makes the I/O functions available for use.
 * This function is not thread-safe, but can be invoked multiple times, as long
 * as it is matched by an equal number of calls to io_fini().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_init(void);

/**
 * Finalizes the I/O library and terminates the availability of the I/O
 * functions. Note that this function MUST be invoked once for each call to
 * io_init(). Only the last invocation will finalize the library.
 */
void io_fini(void);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_SYS_IO_H_
