/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdlib.h>` and defines any missing functionality.
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

#ifndef LELY_LIBC_STDLIB_H
#define LELY_LIBC_STDLIB_H

#include <lely/libc/libc.h>

#include <stdlib.h>

#if __STDC_VERSION__ >= 201112L || defined(__USE_ISOC11)

#define aligned_free	free

#else

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Allocates space for an object whose alignment is specified by \a alignment,
 * whose size is specified by \a size, and whose value is indeterminate. The
 * value of \a alignment SHALL be a valid alignment supported by the
 * implementation and the value of \a size SHALL be an integral multiple of
 * \a alignment.
 *
 * To ensure maximum portability, objects allocated with this function SHOULD be
 * freed with aligned_free().
 *
 * \returns either a NULL pointer or a pointer to the allocated space.
 */
LELY_LIBC_EXTERN void * __cdecl aligned_alloc(size_t alignment, size_t size);

/*!
 * Causes the space at \a ptr to be deallocated, that is, made available for
 * further allocation. If \a ptr is a NULL pointer, no action occurs. Otherwise,
 * if the argument does not match a pointer earlier returned by aligned_alloc(),
 * or if the space has been deallocated by a call to aligned_free(), the
 * behavior is undefined.
 */
LELY_LIBC_EXTERN void __cdecl aligned_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // __STDC_VERSION__ >= 201112L || __USE_ISOC11

#endif

