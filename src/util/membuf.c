/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the memory buffer.
 *
 * @see lely/util/membuf.h
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#include "util.h"
#include <lely/util/errnum.h>
#define LELY_UTIL_MEMBUF_INLINE extern inline
#include <lely/util/membuf.h>

#include <assert.h>
#if !LELY_NO_MALLOC
#include <stdlib.h>
#endif

#ifndef LELY_MEMBUF_SIZE
/// The initial size (in bytes) of a memory buffer.
#define LELY_MEMBUF_SIZE 16
#endif

void
membuf_fini(struct membuf *buf)
{
	assert(buf);

#if LELY_NO_MALLOC
	(void)buf;
#else
	free(buf->begin);
#endif
}

size_t
membuf_reserve(struct membuf *buf, size_t size)
{
	assert(buf);

	size_t capacity = membuf_capacity(buf);
	if (size <= capacity)
		return capacity;

#if LELY_NO_MALLOC
	set_errnum(ERRNUM_NOMEM);
	return 0;
#else
	// The required size equals the size of the data already in the buffer,
	// plus the data to be added. To limit the number of allocations, we
	// keep doubling the size of the buffer until it is large enough.
	size_t buf_size = LELY_MEMBUF_SIZE;
	while (buf_size < membuf_size(buf) + size)
		buf_size *= 2;

	char *begin = realloc(buf->begin, buf_size);
	if (!begin) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return 0;
	}

	buf->cur = begin + membuf_size(buf);
	buf->begin = begin;
	buf->end = buf->begin + buf_size;

	return membuf_capacity(buf);
#endif // LELY_NO_MALLOC
}

void
membuf_flush(struct membuf *buf, size_t size)
{
	assert(buf);

	size = MIN(size, membuf_size(buf));
	if (size)
		memmove(buf->begin, buf->begin + size, membuf_size(buf) - size);
	buf->cur -= size;
}
