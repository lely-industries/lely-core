/**@file
 * This is the internal header file of the CANopen library.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_CO_INTERN_CO_H_
#define LELY_CO_INTERN_CO_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/co/co.h>

#if LELY_NO_MALLOC
// Disable EDS/DCF support.
#undef LELY_NO_CO_DCF
#define LELY_NO_CO_DCF 1
// Disable static device description support.
#undef LELY_NO_CO_SDEV
#define LELY_NO_CO_SDEV 1
// Disable Wireless Transmission Media (WTM) support.
#undef LELY_NO_CO_WTM
#define LELY_NO_CO_WTM 1
// Disable gateway support.
#undef LELY_NO_CO_GW
#define LELY_NO_CO_GW 1
// Disable ASCII gateway support.
#undef LELY_NO_CO_GW_TXT
#define LELY_NO_CO_GW_TXT 1
#endif // LELY_NO_MALLOC

#if LELY_NO_STDIO
// Disable EDS/DCF support.
#undef LELY_NO_CO_DCF
#define LELY_NO_CO_DCF 1
// Disable UploadFile/DownloadFile support.
#undef LELY_NO_CO_OBJ_FILE
#define LELY_NO_CO_OBJ_FILE 1
// Disable ASCII gateway support.
#undef LELY_NO_CO_GW_TXT
#define LELY_NO_CO_GW_TXT 1
#endif

#if defined(NDEBUG) || LELY_NO_STDIO || LELY_NO_DIAG
#define trace(...)
#else
#include <lely/util/diag.h>
#define trace(...) \
	diag_at(DIAG_DEBUG, 0, &(struct floc){ __FILE__, __LINE__, 0 }, \
			__VA_ARGS__)
#endif

#endif // !LELY_CO_INTERN_CO_H_
