/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/stdlib.h
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

#include "libc.h"
#include <lely/libc/stdlib.h>

#if !(__STDC_VERSION__ >= 201112L)

#ifndef LELY_HAVE_POSIX_MEMALIGN
#if _POSIX_C_SOURCE >= 200112L
#define LELY_HAVE_POSIX_MEMALIGN	1
#if defined(__NEWLIB__) && !defined(__rtems__)
#undef LELY_HAVE_POSIX_MEMALIGN
#endif
#endif
#endif

#ifdef _WIN32
#include <malloc.h>
#elif defined(LELY_HAVE_POSIX_MEMALIGN)
#include <errno.h>
#else
#include <lely/libc/stdint.h>
#endif

LELY_LIBC_EXPORT void * __cdecl
aligned_alloc(size_t alignment, size_t size)
{
#ifdef _WIN32
	if (__unlikely(!size))
		return NULL;
	return _aligned_malloc(size, alignment);
#elif LELY_HAVE_POSIX_MEMALIGN
	void *ptr = NULL;
	int errnum = posix_memalign(&ptr, alignment, size);
	if (__unlikely(errnum)) {
		errno = errnum;
		return NULL;
	}
	return ptr;
#else
	// Check if the alignment is a multiple if sizeof(void *) that is also a
	// power of two
	if (__unlikely((alignment & (alignment - 1))
			|| alignment < sizeof(void *)))
		return NULL;
	if (__unlikely(!size))
		return NULL;

	// malloc() is guaranteed to return a pointer aligned to at at least
	// sizeof(void *), the minimum value of 'alignment'. We therefore need
	// at most 'alignment' extra bytes.
	void *ptr = malloc(size + alignment);
	if (__unlikely(!ptr))
		return NULL;
	// Align the pointer.
	void *aligned_ptr = (void *)(((uintptr_t)(char *)ptr + alignment - 1)
			& ~(uintptr_t)(alignment - 1));
	// Store the pointer obtained from malloc() right before the memory
	// region we return to the user.
	((void **)aligned_ptr)[-1] = ptr;

	return aligned_ptr;
#endif
}

#ifndef __USE_ISOC11

LELY_LIBC_EXPORT void __cdecl
aligned_free(void *ptr)
{
#ifdef _WIN32
	_aligned_free(ptr);
#elif LELY_HAVE_POSIX_MEMALIGN
	free(ptr);
#else
	if (__likely(ptr))
		free(((void **)ptr)[-1]);
#endif
}

#endif

#endif // !(__STDC_VERSION__ >= 201112L)

#ifdef _WIN32

#include <lely/libc/stdio.h>

#include <errno.h>

LELY_LIBC_EXPORT int
setenv(const char *envname, const char *envval, int overwrite)
{
	if (__unlikely(!envname)) {
		errno = EINVAL;
		return -1;
	}

	if (!overwrite && getenv(envname))
		return 0;

	for (const char *cp = envname; *cp; cp++) {
		if (__unlikely(*cp == '=')) {
			errno = EINVAL;
			return -1;
		}
	}

	char *string = NULL;
	if (__unlikely(asprintf(&string, "%s=%s", envname, envval) < 0))
		return -1;

	if (__unlikely(_putenv(string))) {
		int errsv = errno;
		free(string);
		errno = errsv;
		return -1;
	}

	free(string);
	return 0;
}

#endif // _WIN32
