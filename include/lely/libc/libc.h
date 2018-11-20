/**@file
 * This is the public header file of the C11 and POSIX compatibility library.
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

#ifndef LELY_LIBC_LIBC_H_
#define LELY_LIBC_LIBC_H_

#include <lely/lely.h>

#ifndef LELY_LIBC_EXTERN
#ifdef LELY_LIBC_INTERN
#define LELY_LIBC_EXTERN	LELY_DLL_EXPORT
#else
#define LELY_LIBC_EXTERN	LELY_DLL_IMPORT
#endif
#endif

#endif // !LELY_LIBC_LIBC_H_
