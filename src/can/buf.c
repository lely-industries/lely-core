/**@file
 * This file is part of the CAN library; it contains the implementation of the
 * CAN frame buffer.
 *
 * @see lely/can/buf.h
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

#include "can.h"
#define LELY_CAN_BUF_INLINE extern inline
#include <lely/can/buf.h>
#include <lely/util/errnum.h>

#include <assert.h>
#if !LELY_NO_MALLOC
#include <stdlib.h>
#include <string.h>
#endif

#ifndef LELY_CAN_BUF_SIZE
/// The minimum size (in number of frames) of a CAN frame buffer.
#define LELY_CAN_BUF_SIZE 16
#endif

void
can_buf_fini(struct can_buf *buf)
{
	assert(buf);

#if LELY_NO_MALLOC
	(void)buf;
#else
	free(buf->ptr);
#endif
}

size_t
can_buf_reserve(struct can_buf *buf, size_t n)
{
	assert(buf);

	const size_t capacity = can_buf_capacity(buf);
	if (n <= capacity)
		return capacity;

#if LELY_NO_MALLOC
	set_errnum(ERRNUM_NOMEM);
	return 0;
#else
	size_t size = LELY_CAN_BUF_SIZE;
	while (size - 1 < can_buf_size(buf) + n)
		size *= 2;
	size--;

	// Reallocate the existing buffer.
	struct can_msg *ptr =
			realloc(buf->ptr, (size + 1) * sizeof(struct can_msg));
	if (!ptr) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return 0;
	}
	buf->ptr = ptr;

	// If the buffer consists of two regions (because of wrapping), move the
	// second region to the end of the buffer.
	size_t begin = buf->begin & buf->size;
	size_t end = buf->end & buf->size;
	if (begin > end) {
		struct can_msg *endptr = buf->ptr + buf->size + 1;
		struct can_msg *srcptr = buf->ptr + begin;
		begin += size - buf->size;
		struct can_msg *dstptr = buf->ptr + begin;
		memmove(dstptr, srcptr,
				(endptr - srcptr) * sizeof(struct can_msg));
		buf->begin = begin;
		buf->end = end;
	}

	buf->size = size;

	return can_buf_capacity(buf);
#endif
}
