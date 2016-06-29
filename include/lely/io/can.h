/*!\file
 * This header file is part of the I/O library; it contains Controller Area
 * Network (CAN) declarations.
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

#ifndef LELY_IO_CAN_H
#define LELY_IO_CAN_H

#include <lely/io/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Opens a CAN device.
 *
 * \param path the (platform dependent) path (or interface) name of the device.
 *
 * \returns a new I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN io_handle_t io_open_can(const char *path);

#ifdef __cplusplus
}
#endif

#endif

