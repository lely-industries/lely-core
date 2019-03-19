/**@file
 * This is the internal header file of the common file descriptor functions.
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

#ifndef LELY_IO2_INTERN_POSIX_FD_H_
#define LELY_IO2_INTERN_POSIX_FD_H_

#include "io.h"

#if _POSIX_C_SOURCE >= 200112L

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets the `FD_CLOEXEC` flag of the file descriptor <b>fd</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained from `errno`.
 */
int io_fd_set_cloexec(int fd);

/**
 * Sets the `O_NONBLOCK` flag of the file descriptor <b>fd</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained from `errno`.
 */
int io_fd_set_nonblock(int fd);

#ifdef __cplusplus
}
#endif

#endif // _POSIX_C_SOURCE >= 200112L

#endif // !LELY_IO2_INTERN_POSIX_FD_H_
