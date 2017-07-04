/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdalign.h>`, if it exists, and defines any missing functionality.
 *
 * \copyright 2017 Lely Industries N.V.
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

#ifndef LELY_LIBC_STDALIGN_H
#define LELY_LIBC_STDALIGN_H

#include <lely/libc/libc.h>

#ifndef LELY_HAVE_STDALIGN_H
#if (__STDC_VERSION__ >= 201112L || __cplusplus >= 201103L) \
		&& __has_include(<stdalign.h>)
#define LELY_HAVE_STDALIGN_H	1
#endif
#endif

#if LELY_HAVE_STDALIGN_H
#include <stdalign.h>
#else

#ifndef __cplusplus

#ifndef alignas
#define alignas	_Alignas
#endif

#ifndef __alignas_is_defined
#define __alignas_is_defined	1
#endif

#ifndef alignof
#define alignof	_Alignof
#endif

#ifndef __alignof_is_defined
#define __alignof_is_defined	1
#endif

#endif

#endif // LELY_HAVE_STDALIGN_H

#endif

