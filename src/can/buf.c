/**@file
 * This file is part of the CAN library; it contains the implementation of the
 * CAN frame buffer.
 *
 * @see lely/can/buf.h
 *
 * @copyright 2016-2018 Lely Industries N.V.
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
#include <stdlib.h>
#include <string.h>

#ifndef LELY_CAN_BUF_SIZE
/// The minimum size (in number of frames) of a CAN frame buffer.
#define LELY_CAN_BUF_SIZE 16
#endif

int
can_buf_init(struct can_buf *buf, size_t size)
{
	assert(buf);

	// Computer the nearest power of two minus one.
	buf->size = LELY_CAN_BUF_SIZE;
	while (buf->size - 1 < size)
		buf->size *= 2;
	buf->size--;

	buf->ptr = malloc((buf->size + 1) * sizeof(struct can_msg));
	if (!buf->ptr) {
		set_errc(errno2c(errno));
		return -1;
	}

#ifdef LELY_NO_ATOMICS
	buf->begin = 0;
	buf->end = 0;
#else
	atomic_init(&buf->begin, 0);
	atomic_init(&buf->end, 0);
#endif

	return 0;
}

void
can_buf_fini(struct can_buf *buf)
{
	assert(buf);

	free(buf->ptr);
}

struct can_buf *
can_buf_create(size_t size)
{
	int errc = 0;

	struct can_buf *buf = malloc(sizeof(*buf));
	if (!buf) {
		errc = errno2c(errno);
		goto error_alloc_buf;
	}

	if (can_buf_init(buf, size) == -1) {
		errc = get_errc();
		goto error_init_buf;
	}

	return buf;

error_init_buf:
	free(buf);
error_alloc_buf:
	set_errc(errc);
	return NULL;
}

void
can_buf_destroy(struct can_buf *buf)
{
	if (!buf)
		return;

	can_buf_fini(buf);

	free(buf);
}

size_t
can_buf_reserve(struct can_buf *buf, size_t n)
{
	assert(buf);

#ifdef LELY_NO_ATOMICS
	size_t begin = buf->begin & buf->size;
	size_t end = buf->end & buf->size;
#else
	size_t begin = atomic_load(&buf->begin) & buf->size;
	size_t end = atomic_load(&buf->end) & buf->size;
#endif

	// Do not resize the buffer if it is not necessary.
	size_t capacity = (begin - end - 1) & buf->size;
	if (capacity >= n)
		return capacity;

	size_t size = LELY_CAN_BUF_SIZE;
	while (size - 1 < buf->size - capacity + n)
		size *= 2;
	size--;

	// Reallocate the existing buffer.
	struct can_msg *ptr =
			realloc(buf->ptr, (size + 1) * sizeof(struct can_msg));
	if (!ptr) {
		set_errc(errno2c(errno));
		return 0;
	}
	buf->ptr = ptr;

	// If the buffer consists of two regions (because of wrapping), move the
	// second region to the end of the buffer.
	if (begin > end) {
		struct can_msg *end = buf->ptr + buf->size + 1;
		struct can_msg *src = buf->ptr + begin;
		begin += size - buf->size;
		struct can_msg *dst = buf->ptr + begin;
		memmove(dst, src, (end - src) * sizeof(struct can_msg));
	}

	buf->size = size;
#ifdef LELY_NO_ATOMICS
	buf->begin = begin;
	buf->end = end;
#else
	atomic_store(&buf->begin, begin);
	atomic_store(&buf->end, end);
#endif

	return (begin - end - 1) & buf->size;
}
