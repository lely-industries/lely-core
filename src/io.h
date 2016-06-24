/*!\file
 * This is the internal header file of the I/O library.
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

#ifndef LELY_IO_INTERN_IO_H
#define LELY_IO_INTERN_IO_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define LELY_IO_INTERN	1
#include <lely/io/io.h>

#ifndef LELY_IO_EXPORT
#ifdef DLL_EXPORT
#define LELY_IO_EXPORT	__dllexport
#else
#define LELY_IO_EXPORT
#endif
#endif

#endif

