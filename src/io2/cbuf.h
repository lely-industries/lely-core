/**@file
 * This is the internal header file of the generic circular buffer
 * implementation.
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_IO2_INTERN_CBUF_H_
#define LELY_IO2_INTERN_CBUF_H_

#include <assert.h>
#include <stdlib.h>

#define IO_CBUF_INIT \
	{ \
		NULL, 0, 0, 0 \
	}

#define LELY_IO_DEFINE_CBUF(name, type) \
	struct name { \
		type *ptr; \
		size_t size; \
		size_t begin; \
		size_t end; \
	}; \
\
	static inline int name##_init(struct name *buf, size_t size); \
	static inline void name##_fini(struct name *buf); \
\
	static inline type *name##_begin(const struct name *buf); \
	static inline type *name##_end(const struct name *buf); \
\
	static inline int name##_empty(const struct name *buf); \
	static inline size_t name##_size(const struct name *buf); \
	static inline size_t name##_capacity(const struct name *buf); \
\
	static inline void name##_clear(struct name *buf); \
	static inline type *name##_push(struct name *buf); \
	static inline type *name##_pop(struct name *buf); \
\
	inline int name##_init(struct name *buf, size_t size) \
	{ \
		assert(buf); \
\
		buf->ptr = calloc(size, sizeof(type)); \
		if (!buf->ptr && size) \
			return -1; \
\
		buf->size = size; \
\
		name##_clear(buf); \
\
		return 0; \
	} \
\
	inline void name##_fini(struct name *buf) \
	{ \
		assert(buf); \
\
		free(buf->ptr); \
	} \
\
	inline type *name##_begin(const struct name *buf) \
	{ \
		assert(buf); \
		assert(buf->begin < buf->size); \
\
		return buf->ptr + buf->begin; \
	} \
\
	inline type *name##_end(const struct name *buf) \
	{ \
		assert(buf); \
		assert(buf->end < buf->size); \
\
		return buf->ptr + buf->end; \
	} \
\
	inline int name##_empty(const struct name *buf) \
	{ \
		assert(buf); \
\
		return buf->begin == buf->end; \
	} \
\
	inline size_t name##_size(const struct name *buf) \
	{ \
		assert(buf); \
\
		return (buf->end - buf->begin) % buf->size; \
	} \
\
	inline size_t name##_capacity(const struct name *buf) \
	{ \
		assert(buf); \
\
		return (buf->begin - buf->end - 1) % buf->size; \
	} \
\
	inline void name##_clear(struct name *buf) \
	{ \
		assert(buf); \
\
		buf->begin = buf->end = 0; \
	} \
\
	inline type *name##_push(struct name *buf) \
	{ \
		type *ptr = name##_end(buf); \
		buf->end = (buf->end + 1) % buf->size; \
		if (buf->begin == buf->end) \
			buf->begin = (buf->begin + 1) % buf->size; \
		return ptr; \
	} \
\
	inline type *name##_pop(struct name *buf) \
	{ \
		if (name##_empty(buf)) \
			return NULL; \
\
		type *ptr = name##_begin(buf); \
		buf->begin = (buf->begin + 1) % buf->size; \
		return ptr; \
	}

#endif // !LELY_IO2_INTERN_CBUF_H_
