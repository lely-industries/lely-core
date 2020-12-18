/**@file
 * This file is part of the I/O library; it contains the standard system
 * implementation of the I/O initialization/finalization functions.
 *
 * @see lely/io2/sys/io.h
 *
 * @copyright 2020 Lely Industries N.V.
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

#if LELY_NO_STDIO || (!_WIN32 && !defined(_POSIX_C_SOURCE))

#include <lely/io2/sys/io.h>

int
io_init(void)
{
	return 0;
}

void
io_fini(void)
{
}

#endif // LELY_NO_STDIO || (!_WIN32 && !_POSIX_C_SOURCE)
