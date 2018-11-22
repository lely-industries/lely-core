/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<sys/types.h>`, if it exists, and defines any missing
 * functionality.
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

#ifndef LELY_LIBC_SYS_TYPES_H_
#define LELY_LIBC_SYS_TYPES_H_

#include <lely/features.h>

#ifndef LELY_HAVE_SYS_TYPES_H
#if defined(_POSIX_C_SOURCE) || defined(__MINGW32__) || defined(__NEWLIB__)
#define LELY_HAVE_SYS_TYPES_H 1
#endif
#endif

#if LELY_HAVE_SYS_TYPES_H
#include <sys/types.h>
#else // !LELY_HAVE_SYS_TYPES_H
#include <stddef.h>

/// Used for clock ID type in the clock and timer functions.
typedef int clockid_t;

/// Used for a count of bytes or an error indication.
typedef ptrdiff_t ssize_t;

#endif // !LELY_HAVE_SYS_TYPES_H

#endif // !LELY_LIBC_SYS_TYPES_H_
