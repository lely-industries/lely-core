/**@file
 * This header file is part of the utilities library; it contains the memory
 * buffer declarations.
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

#ifndef LELY_UTIL_MEMBUF_H_
#define LELY_UTIL_MEMBUF_H_

#include <lely/util/util.h>

#include <assert.h>
#include <stddef.h>
#include <string.h>

#ifndef LELY_UTIL_MEMBUF_INLINE
#define LELY_UTIL_MEMBUF_INLINE static inline
#endif

/// A memory buffer.
struct membuf {
	/// A pointer to one past the last byte written to the buffer.
	char *cur;
	/// A pointer to the first byte in the buffer.
	char *begin;
	/// A pointer to one past the last byte in the buffer.
	char *end;
};

/// The static initializer for struct #membuf.
#define MEMBUF_INIT \
	{ \
		NULL, NULL, NULL \
	}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a memory buffer.
 *
 * @param buf  a pointer to a memory buffer.
 * @param ptr  a pointer to the memory region to be used by the buffer. If not
 *             NULL, the buffer takes ownership of the region at <b>ptr</b> and
 *             MAY reallocate or free the region during subsequent calls to
 *             membuf_reserve() and membuf_fini().
 * @param size the number of bytes available at <b>ptr</b>.
 *
 * @see membuf_fini()
 */
LELY_UTIL_MEMBUF_INLINE void membuf_init(
		struct membuf *buf, void *ptr, size_t size);

/// Finalizes a memory buffer. @see membuf_init()
void membuf_fini(struct membuf *buf);

/// Returns a pointer to the first byte in a memory buffer.
LELY_UTIL_MEMBUF_INLINE void *membuf_begin(const struct membuf *buf);

/// Clears a memory buffer. @see membuf_flush()
LELY_UTIL_MEMBUF_INLINE void membuf_clear(struct membuf *buf);

/// Returns the total number of bytes written to a memory buffer.
LELY_UTIL_MEMBUF_INLINE size_t membuf_size(const struct membuf *buf);

/// Returns the number of unused bytes remaining in a memory buffer.
LELY_UTIL_MEMBUF_INLINE size_t membuf_capacity(const struct membuf *buf);

/**
 * Resizes a memory buffer, if necessary, to make room for at least an
 * additional <b>size</b> bytes. This function may also shrink the buffer if it
 * is larger than required. Although this functions preserves the data already
 * written to the buffer, it invalidates the pointers obtained from previous
 * calls to membuf_alloc().
 *
 * @param buf  a pointer to a memory buffer.
 * @param size the required capacity of the buffer.
 *
 * @returns the new capacity of the buffer, or 0 on error. The new capacity can
 * be larger than the requested capacity.
 */
size_t membuf_reserve(struct membuf *buf, size_t size);

/**
 * Adjusts the position indicator of a memory buffer by <b>offset</b> bytes. The
 * offset will be truncated to a valid range: if negative, -<b>offset</b> must
 * not exceed membuf_size(); if positive, <b>offset</b> must not exceed
 * membuf_capacity().
 *
 * @param buf    a pointer to a memory buffer.
 * @param offset the offset (in bytes) with respect to the current position
 *               indicator.
 *
 * @returns the actual applied offset.
 */
LELY_UTIL_MEMBUF_INLINE ptrdiff_t membuf_seek(
		struct membuf *buf, ptrdiff_t offset);

/**
 * Creates region of *<b>size</b> bytes in a memory buffer, starting at the
 * current position indicator given by membuf_size(), and sets the indicator to
 * the end of the requested region. If the requested size is larger than the
 * current capacity, *<b>size</b> will be updated with the actual size of the
 * region. If the requested size turns out to be too large or too small, the
 * region can be adjusted with membuf_seek().
 *
 * @param buf  a pointer to a memory buffer.
 * @param size the address of the value containing the requested size. On exit,
 *             *<b>size</b> is updated with the actual size.
 *
 * @returns a pointer to a memory region of at least *<b>size</b> consecutive
 * bytes (which may be 0). Note that this pointer is only valid until the next
 * call to membuf_reserve().
 */
LELY_UTIL_MEMBUF_INLINE void *membuf_alloc(struct membuf *buf, size_t *size);

/**
 * Writes data to a memory buffer. Writing starts at the current position
 * indicator given by membuf_size().
 *
 * @param buf  a pointer to a memory buffer.
 * @param ptr  a pointer to the data to be copied.
 * @param size the number of bytes to be written.
 *
 * @returns the number of bytes written, which may be smaller than <b>size</b>
 * in case of insufficient capacity.
 */
LELY_UTIL_MEMBUF_INLINE size_t membuf_write(
		struct membuf *buf, const void *ptr, size_t size);

/// Flushes <b>size</b> bytes from the beginning of a memory buffer.
void membuf_flush(struct membuf *buf, size_t size);

LELY_UTIL_MEMBUF_INLINE void
membuf_init(struct membuf *buf, void *ptr, size_t size)
{
	assert(buf);
	assert(ptr || !size);

	buf->begin = buf->cur = (char *)ptr;
	buf->end = buf->begin + size;
}

LELY_UTIL_MEMBUF_INLINE void *
membuf_begin(const struct membuf *buf)
{
	assert(buf);

	return buf->begin;
}

LELY_UTIL_MEMBUF_INLINE void
membuf_clear(struct membuf *buf)
{
	assert(buf);

	buf->cur = buf->begin;
}

LELY_UTIL_MEMBUF_INLINE size_t
membuf_size(const struct membuf *buf)
{
	assert(buf);

	return buf->cur - buf->begin;
}

LELY_UTIL_MEMBUF_INLINE size_t
membuf_capacity(const struct membuf *buf)
{
	assert(buf);

	return buf->end - buf->cur;
}

LELY_UTIL_MEMBUF_INLINE ptrdiff_t
membuf_seek(struct membuf *buf, ptrdiff_t offset)
{
	assert(buf);

	char *cur = buf->cur + offset;
	if (cur - buf->begin < 0) {
		cur = buf->begin;
		offset = cur - buf->cur;
	} else if (buf->end - cur < 0) {
		cur = buf->end;
		offset = cur - buf->cur;
	}
	buf->cur = cur;

	return offset;
}

LELY_UTIL_MEMBUF_INLINE void *
membuf_alloc(struct membuf *buf, size_t *size)
{
	assert(buf);
	assert(size);

	void *cur = buf->cur;
	*size = membuf_seek(buf, *size);
	return cur;
}

LELY_UTIL_MEMBUF_INLINE size_t
membuf_write(struct membuf *buf, const void *ptr, size_t size)
{
	assert(buf);
	assert(ptr || !size);

	void *cur = membuf_alloc(buf, &size);
	if (size)
		memcpy(cur, ptr, size);
	return size;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_MEMBUF_H_
