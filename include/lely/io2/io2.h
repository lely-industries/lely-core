/**@file
 * This is the public header file of the I/O library.
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

#ifndef LELY_IO2_IO2_H_
#define LELY_IO2_IO2_H_

#include <lely/features.h>

/// An I/O context.
typedef struct io_ctx io_ctx_t;

#ifndef LELY_IO_RX_TIMEOUT
/// The default timeout (in milliseconds) for I/O read operations.
#define LELY_IO_RX_TIMEOUT 100
#endif

#ifndef LELY_IO_TX_TIMEOUT
/// The default timeout (in milliseconds) for I/O write operations.
#define LELY_IO_TX_TIMEOUT 10
#endif

#endif // !LELY_IO2_IO2_H_
