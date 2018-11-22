/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdlib.h>` and defines any missing functionality.
 *
 * @copyright 2014-2018 Lely Industries N.V.
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

#ifndef LELY_LIBC_STDLIB_H_
#define LELY_LIBC_STDLIB_H_

#include <lely/features.h>

#include <stdlib.h>

#if __STDC_VERSION__ >= 201112L || defined(__USE_ISOC11)

#define aligned_free free

#else // !(__STDC_VERSION__ >= 201112L || __USE_ISOC11)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocates space for an object whose alignment is specified by
 * <b>alignment</b>, whose size is specified by <b>size</b>, and whose value is
 * indeterminate. The value of <b>alignment</b> SHALL be a valid alignment
 * supported by the implementation and the value of <b>size</b> SHALL be an
 * integral multiple of <b>alignment</b>.
 *
 * To ensure maximum portability, objects allocated with this function SHOULD be
 * freed with aligned_free().
 *
 * @returns either a NULL pointer or a pointer to the allocated space.
 */
void *aligned_alloc(size_t alignment, size_t size);

/**
 * Causes the space at <b>ptr</b> to be deallocated, that is, made available for
 * further allocation. If <b>ptr</b> is a NULL pointer, no action occurs.
 * Otherwise, if the argument does not match a pointer earlier returned by
 * aligned_alloc(), or if the space has been deallocated by a call to
 * aligned_free(), the behavior is undefined.
 */
void aligned_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // !(__STDC_VERSION__ >= 201112L || __USE_ISOC11)

#if !(_POSIX_C_SOURCE > 200112L)

/**
 * Updates or adds a variable in the environment of the calling process.
 * <b>envname</b> points to a string containing the name of the variable to be
 * added or altered. If the variable does not exist, or overwrite is non-zero,
 * it SHALL be set to the value two which <b>envval</b> points. Otherwise the
 * environment SHALL remain unchanged.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the environment is
 * unchanged.
 */
int setenv(const char *envname, const char *envval, int overwrite);

#endif // !(_POSIX_C_SOURCE > 200112L)

#endif // !LELY_LIBC_STDLIB_H_
