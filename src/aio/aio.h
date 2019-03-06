/**@file
 * This is the internal header file of the asynchronous I/O library.
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_AIO_INTERN_AIO_H_
#define LELY_AIO_INTERN_AIO_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define LELY_AIO_INTERN 1
#include <lely/aio/aio.h>

#ifndef LELY_AIO_EXPORT
#define LELY_DLL_EXPORT
#endif

#ifndef LELY_AIO_WITH_CAN_BUS
#ifdef __linux__
#define LELY_AIO_WITH_CAN_BUS 1
#endif
#endif

#ifndef LELY_AIO_WITH_CLOCK
#if _POSIX_TIMERS > 0 || _WIN32
#define LELY_AIO_WITH_CLOCK 1
#endif
#endif

#ifndef LELY_AIO_WITH_REACTOR
#ifdef __linux__
#define LELY_AIO_WITH_REACTOR 1
#endif
#endif

#ifndef LELY_AIO_WITH_TIMER
#ifdef __linux__
#define LELY_AIO_WITH_TIMER 1
#endif
#endif

#endif // LELY_AIO_INTERN_AIO_H_
