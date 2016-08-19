/*!\file
 * This is the internal header file of the C11 and POSIX compatibility library.
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

#ifndef LELY_LIBC_INTERN_LIBC_H
#define LELY_LIBC_INTERN_LIBC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define LELY_LIBC_INTERN	1
#include <lely/libc/libc.h>

#ifndef LELY_LIBC_EXPORT
#define LELY_LIBC_EXPORT	LELY_DLL_EXPORT
#endif

#ifdef _WIN32
#include <windows.h>
#elif defined(_POSIX_C_SOURCE)
#include <unistd.h>
#endif

#endif

