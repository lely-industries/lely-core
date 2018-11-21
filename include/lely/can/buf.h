/**@file
 * This header file is part of the CAN library; it contains the CAN frame buffer
 * declarations.
 *
 * The CAN frame buffer is implemented as a circular buffer. Apart from
 * can_buf_clear() and can_buf_reserve() (an obviously can_buf_init() and
 * can_buf_fini()), all buffer functions are thread-safe and lock-free, provided
 * there are at most two threads accessing the buffer at the same time
 * (single-reader single-writer).
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

#ifndef LELY_CAN_BUF_H_
#define LELY_CAN_BUF_H_

#ifndef LELY_NO_ATOMICS
#define LELY_NO_ATOMICS 1
#ifndef LELY_NO_THREADS
#include <lely/libc/stdatomic.h>
#ifndef __STDC_NO_ATOMICS__
#undef LELY_NO_ATOMICS
#endif
#endif
#endif
#include <lely/can/msg.h>

#ifndef LELY_CAN_BUF_INLINE
#define LELY_CAN_BUF_INLINE static inline
#endif

/// A CAN frame buffer.
struct can_buf {
	/// A pointer to the allocated memory for the buffer.
	struct can_msg *ptr;
	/**
	 * The total size (in number of frames) of the buffer, excluding the
	 * unused frame used to distinguish between a full and an empty buffer.
	 * This needs to be a power of two minus one, so we can use '`& size`'
	 * instead of '`% (size + 1)`' in wrapping calculations.
	 */
	size_t size;
	/**
	 * The offset (with respect to #ptr) of the first value available for
	 * reading (and two past the last available for writing, modulo
	 * #size + 1).
	 */
#ifdef LELY_NO_ATOMICS
	size_t begin;
#else
	atomic_size_t begin;
#endif
	/**
	 * The offset (with respect to #ptr) of one past the last value
	 * available for reading (and the first available for writing, modulo
	 * #size + 1).
	 */
#ifdef LELY_NO_ATOMICS
	size_t end;
#else
	atomic_size_t end;
#endif
};

/// The static initializer for struct #can_buf.
#ifdef LELY_NO_ATOMICS
#define CAN_BUF_INIT \
	{ \
		NULL, 0, 0, 0 \
	}
#else
#define CAN_BUF_INIT \
	{ \
		NULL, 0, ATOMIC_VAR_INIT(0), ATOMIC_VAR_INIT(0) \
	}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a CAN frame buffer.
 *
 * @param buf  a pointer to a CAN frame buffer.
 * @param size the initial size (in number of frames). The size will be rounded
 *             up to the nearest power of two minus one.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see can_buf_fini()
 */
int can_buf_init(struct can_buf *buf, size_t size);

/// Finalizes a CAN frame buffer. @see can_buf_init()
void can_buf_fini(struct can_buf *buf);

/**
 * Allocates and initializes a CAN frame buffer.
 *
 * @param size the initial size (in number of frames). The size will be rounded
 *             up to the nearest power of two minus one.
 *
 * @returns a pointer to a new frame buffer, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see can_buf_destroy(), can_buf_init()
 */
struct can_buf *can_buf_create(size_t size);

// Finalizes and frees a CAN frame buffer. @see can_buf_create(), can_buf_fini()
void can_buf_destroy(struct can_buf *buf);

/// Clears a CAN frame buffer.
LELY_CAN_BUF_INLINE void can_buf_clear(struct can_buf *buf);

/**
 * Returns the number of frames available for reading in a CAN buffer.
 *
 * @see can_buf_capacity()
 */
LELY_CAN_BUF_INLINE size_t can_buf_size(const struct can_buf *buf);

/**
 * Returns the number of frames available for writing in a CAN buffer.
 *
 * @see can_buf_size()
 */
LELY_CAN_BUF_INLINE size_t can_buf_capacity(const struct can_buf *buf);

/**
 * Resizes a CAN frame buffer, if necessary, to make room for at least <b>n</b>
 * additional frames. Note that the new capacity can be larger than the
 * requested capacity.
 *
 * @returns the new capacity of the buffer (in number of frames), or 0 on error.
 * In the latter case, the error number can be obtained with get_errc().
 */
size_t can_buf_reserve(struct can_buf *buf, size_t n);

/**
 * Reads, but does not remove, frames from a CAN frame buffer.
 *
 * @param buf a pointer to a CAN frame buffer.
 * @param ptr the address at which to store the frames (can be NULL).
 * @param n   the number of frames to read.
 *
 * @returns the number of frames read.
 *
 * @see can_buf_read(), can_buf_write()
 */
LELY_CAN_BUF_INLINE size_t can_buf_peek(
		struct can_buf *buf, struct can_msg *ptr, size_t n);

/**
 * Reads, and removes, frames from a CAN frame buffer.
 *
 * @param buf a pointer to a CAN frame buffer.
 * @param ptr the address at which to store the frames (can be NULL).
 * @param n   the number of frames to read.
 *
 * @returns the number of frames read.
 *
 * @see can_buf_peek(), can_buf_write()
 */
LELY_CAN_BUF_INLINE size_t can_buf_read(
		struct can_buf *buf, struct can_msg *ptr, size_t n);

/**
 * Writes frames to a CAN frame buffer.
 *
 * @param buf a pointer to a CAN frame buffer.
 * @param ptr the address from which to load the frames.
 * @param n   the number of frames to write.
 *
 * @returns the number of frames written.
 *
 * @see can_buf_peek(), can_buf_read()
 */
LELY_CAN_BUF_INLINE size_t can_buf_write(
		struct can_buf *buf, const struct can_msg *ptr, size_t n);

inline void
can_buf_clear(struct can_buf *buf)
{
#ifdef LELY_NO_ATOMICS
	buf->end = buf->begin;
#else
	size_t begin = atomic_load_explicit(&buf->begin, memory_order_acquire);
	atomic_store_explicit(&buf->end, begin, memory_order_release);
#endif
}

inline size_t
can_buf_size(const struct can_buf *buf)
{
#ifdef LELY_NO_ATOMICS
	size_t begin = buf->begin;
	size_t end = buf->end;
#else
	size_t begin = atomic_load_explicit(
			&((struct can_buf *)buf)->begin, memory_order_acquire);
	size_t end = atomic_load_explicit(
			&((struct can_buf *)buf)->end, memory_order_acquire);
#endif

	return (end - begin) & buf->size;
}

inline size_t
can_buf_capacity(const struct can_buf *buf)
{
#ifdef LELY_NO_ATOMICS
	size_t begin = buf->begin;
	size_t end = buf->end;
#else
	size_t begin = atomic_load_explicit(
			&((struct can_buf *)buf)->begin, memory_order_acquire);
	size_t end = atomic_load_explicit(
			&((struct can_buf *)buf)->end, memory_order_acquire);
#endif

	return (begin - end - 1) & buf->size;
}

inline size_t
can_buf_peek(struct can_buf *buf, struct can_msg *ptr, size_t n)
{
#ifdef LELY_NO_ATOMICS
	size_t begin = buf->begin;
#else
	size_t begin = atomic_load_explicit(&buf->begin, memory_order_acquire);
#endif
	for (size_t i = 0; i < n; i++) {
#ifdef LELY_NO_ATOMICS
		size_t end = buf->end;
#else
		size_t end = atomic_load_explicit(
				&buf->end, memory_order_acquire);
#endif
		if (!((end - begin) & buf->size))
			return i;

		if (ptr)
			ptr[i] = buf->ptr[begin & buf->size];
		begin++;
	}

	return n;
}

inline size_t
can_buf_read(struct can_buf *buf, struct can_msg *ptr, size_t n)
{
#ifdef LELY_NO_ATOMICS
	size_t begin = buf->begin;
#else
	size_t begin = atomic_load_explicit(&buf->begin, memory_order_acquire);
#endif
	for (size_t i = 0; i < n; i++) {
#ifdef LELY_NO_ATOMICS
		size_t end = buf->end;
#else
		size_t end = atomic_load_explicit(
				&buf->end, memory_order_acquire);
#endif
		if (!((end - begin) & buf->size))
			return i;

		if (ptr)
			ptr[i] = buf->ptr[begin & buf->size];
		begin++;

#ifdef LELY_NO_ATOMICS
		buf->begin = begin;
#else
		atomic_store_explicit(&buf->begin, begin, memory_order_release);
#endif
	}

	return n;
}

inline size_t
can_buf_write(struct can_buf *buf, const struct can_msg *ptr, size_t n)
{
#ifdef LELY_NO_ATOMICS
	size_t end = buf->end;
#else
	size_t end = atomic_load_explicit(&buf->end, memory_order_acquire);
#endif
	for (size_t i = 0; i < n; i++) {
#ifdef LELY_NO_ATOMICS
		size_t begin = buf->begin;
#else
		size_t begin = atomic_load_explicit(
				&buf->begin, memory_order_acquire);
#endif
		if (!((begin - end - 1) & buf->size))
			return i;

		buf->ptr[end++ & buf->size] = ptr[i];

#ifdef LELY_NO_ATOMICS
		buf->end = end;
#else
		atomic_store_explicit(&buf->end, end, memory_order_release);
#endif
	}

	return n;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_CAN_BUF_H_
