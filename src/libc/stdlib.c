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

#if !(__STDC_VERSION__ >= 201112L) && !LELY_NO_MALLOC

#ifndef LELY_HAVE_POSIX_MEMALIGN
#if _POSIX_C_SOURCE >= 200112L
#define LELY_HAVE_POSIX_MEMALIGN 1
#if defined(__NEWLIB__) && !defined(__rtems__)
#undef LELY_HAVE_POSIX_MEMALIGN
#endif
#endif
#endif

#if _WIN32
#include <malloc.h>
#elif LELY_HAVE_POSIX_MEMALIGN
#if !LELY_NO_ERRNO
#include <errno.h>
#endif
#else
#include <stdint.h>
#endif

void *
aligned_alloc(size_t alignment, size_t size)
{
#if _WIN32
	if (!size)
		return NULL;
	return _aligned_malloc(size, alignment);
#elif LELY_HAVE_POSIX_MEMALIGN
	void *ptr = NULL;
	int errc = posix_memalign(&ptr, alignment, size);
	if (errc) {
#if !LELY_NO_ERRNO
		errno = errc;
#endif
		return NULL;
	}
	return ptr;
	// clang-format off
#else // !_WIN32 && !LELY_HAVE_POSIX_MEMALIGN
	// Check if the alignment is a multiple if sizeof(void *) that is also a
	// power of two
	if ((alignment & (alignment - 1)) || alignment < sizeof(void *))
		return NULL;
	// clang-format on
	if (!size)
		return NULL;

	// malloc() is guaranteed to return a pointer aligned to at at least
	// sizeof(void *), the minimum value of 'alignment'. We therefore need
	// at most 'alignment' extra bytes.
	void *ptr = malloc(size + alignment);
	if (!ptr)
		return NULL;
	// Align the pointer.
	void *aligned_ptr = (void *)(((uintptr_t)(char *)ptr + alignment - 1)
			& ~(uintptr_t)(alignment - 1));
	// Store the pointer obtained from malloc() right before the memory
	// region we return to the user.
	((void **)aligned_ptr)[-1] = ptr;

	return aligned_ptr;
#endif // !_WIN32 && !LELY_HAVE_POSIX_MEMALIGN
}

#ifndef __USE_ISOC11

void
aligned_free(void *ptr)
{
#if _WIN32
	_aligned_free(ptr);
#elif LELY_HAVE_POSIX_MEMALIGN
	free(ptr);
#else
	if (ptr)
		free(((void **)ptr)[-1]);
#endif
}

#endif // !__USE_ISOC11

#endif // !(__STDC_VERSION__ >= 201112L) && !LELY_NO_MALLOC

#if _WIN32 && !LELY_NO_STDIO

#include <lely/libc/stdio.h>

#include <errno.h>

int
setenv(const char *envname, const char *envval, int overwrite)
{
	if (!envname) {
		errno = EINVAL;
		return -1;
	}

	if (!overwrite && getenv(envname))
		return 0;

	for (const char *cp = envname; *cp; cp++) {
		if (*cp == '=') {
			errno = EINVAL;
			return -1;
		}
	}

	char *string = NULL;
	if (asprintf(&string, "%s=%s", envname, envval) < 0)
		return -1;

	if (_putenv(string)) {
		int errsv = errno;
		free(string);
		errno = errsv;
		return -1;
	}

	free(string);
	return 0;
}

#endif // _WIN32
