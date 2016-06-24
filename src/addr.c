/*!\file
 * This file is part of the I/O library; it contains the implementation of the
 * network address functions.
 *
 * \see lely/io/addr.h
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

#include "io.h"
#include <lely/util/cmp.h>
#include <lely/io/addr.h>

#include <string.h>

LELY_IO_EXPORT int __cdecl
io_addr_cmp(const void *p1, const void *p2)
{
	if (p1 == p2)
		return 0;

	if (__unlikely(!p1))
		return -1;
	if (__unlikely(!p2))
		return 1;

	const io_addr_t *a1 = p1;
	const io_addr_t *a2 = p2;

	int cmp = memcmp(&a1->addr, &a2->addr, MIN(a1->addrlen, a2->addrlen));
	if (!cmp)
		cmp = (a2->addrlen < a1->addrlen) - (a1->addrlen < a2->addrlen);
	return cmp;
}

