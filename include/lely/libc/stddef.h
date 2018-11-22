/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stddef.h>` and defines any missing functionality.
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_LIBC_STDDEF_H_
#define LELY_LIBC_STDDEF_H_

#include <lely/features.h>

#include <stddef.h>

#if !(__STDC_VERSION__ >= 201112L) && !(__cplusplus >= 201103L)
/**
 * An object type whose alignment is as great as is supported by the
 * implementation in all contexts.
 */
typedef union {
	long long _ll;
	long double _ld;
	void *_ptr;
	int (*_fptr)(void);
} max_align_t;
#endif

#endif // !LELY_LIBC_STDDEF_H_
