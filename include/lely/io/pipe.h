/**@file
 * This header file is part of the I/O library; it contains the pipe
 * declarations.
 *
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_IO_PIPE_H_
#define LELY_IO_PIPE_H_

#include <lely/io/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a pipe.
 *
 * @param handle_vector a 2-value array which, on success, contains the device
 *                      handles of the pipe. `handle_vector[0]` corresponds to
 *                      the read end and `handle_vector[1]` to the write end.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_open_pipe(io_handle_t handle_vector[2]);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_PIPE_H_
