/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdint.h>` and defines any missing functionality.
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_LIBC_STDINT_H_
#define LELY_LIBC_STDINT_H_

#include <lely/features.h>

#include <stdint.h>

#ifndef SSIZE_MAX
#if __WORDSIZE == 64
#define SSIZE_MAX INT_LEAST64_MAX
#else
#define SSIZE_MAX INT_LEAST32_MAX
#endif
#endif

#endif // !LELY_LIBC_STDINT_H_
