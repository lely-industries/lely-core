/*!\file
 * This header file is part of the utilities library; it contains the file
 * buffer declarations.
 *
 * A file buffer is used to load an entire file into memory for reading. This
 * makes it possible to use pointer manipulation and functions like `memcpy()`,
 * instead of `fseek()` and `fread()`. This implementation uses
 * `CreateFileMapping()` or `mmap()`, if available, to avoid explicitly loading
 * the file into memory.
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

#ifndef LELY_UTIL_FBUF_H
#define LELY_UTIL_FBUF_H

#include <lely/util/util.h>

#include <stddef.h>

struct __fbuf;
#ifndef __cplusplus
//! An opaque file buffer type.
typedef struct __fbuf fbuf_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

LELY_UTIL_EXTERN void *__fbuf_alloc(void);
LELY_UTIL_EXTERN void __fbuf_free(void *ptr);
LELY_UTIL_EXTERN struct __fbuf *__fbuf_init(struct __fbuf *buf,
		const char *filename);
LELY_UTIL_EXTERN void __fbuf_fini(struct __fbuf *buf);

/*!
 * Creates a new file buffer.
 *
 * \param filename the name of the file to be read into a buffer.
 *
 * \returns a pointer to a new file buffer, or NULL on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 *
 * \see fbuf_destroy()
 */
LELY_UTIL_EXTERN fbuf_t *fbuf_create(const char *filename);

//! Destroys a file buffer. \see fbuf_create()
LELY_UTIL_EXTERN void fbuf_destroy(fbuf_t *buf);

//! Returns a pointer to the first byte in a file buffer.
LELY_UTIL_EXTERN void *fbuf_begin(const fbuf_t *buf);

//! Returns the size (in bytes) of a file buffer.
LELY_UTIL_EXTERN size_t fbuf_size(const fbuf_t *buf);

#ifdef __cplusplus
}
#endif

#endif

