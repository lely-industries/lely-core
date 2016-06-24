/*!\file
 * This header file is part of the I/O library; it contains network address
 * declarations.
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

#ifndef LELY_IO_ADDR_H
#define LELY_IO_ADDR_H

#include <lely/libc/stdint.h>
#include <lely/io/io.h>

//! An opaque network address type.
struct __io_addr {
	//! The size (in bytes) of #addr.
	int addrlen;
	//! The network address.
	union { char __size[128]; long __align; } addr;
};

//! The static initializer for #io_addr_t.
#define IO_ADDR_INIT	{ 0, { { 0 } } }

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Compares two network addresses.
 *
 * \returns an integer greater than, equal to, or less than 0 if the address at
 * \a p1 is greater than, equal to, or less than the address at \a p2.
 */
LELY_IO_EXTERN int __cdecl io_addr_cmp(const void *p1, const void *p2);

#ifdef __cplusplus
}
#endif

#endif

