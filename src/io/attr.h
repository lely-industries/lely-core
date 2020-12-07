/**@file
 * This is the internal header file of the serial I/O attributes declarations.
 *
 * @see lely/io/attr.h
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

#ifndef LELY_IO_INTERN_ATTR_H_
#define LELY_IO_INTERN_ATTR_H_

#include "io.h"
#include <lely/io/attr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32

static inline LPDCB io_attr_lpDCB(const io_attr_t *attr);
static inline LPCOMMTIMEOUTS io_attr_lpCommTimeouts(const io_attr_t *attr);

static inline LPDCB
io_attr_lpDCB(const io_attr_t *attr)
{
	struct io_attr {
		DCB DCB;
		COMMTIMEOUTS CommTimeouts;
	};

	return &((struct io_attr *)attr)->DCB;
}

static inline LPCOMMTIMEOUTS
io_attr_lpCommTimeouts(const io_attr_t *attr)
{
	struct io_attr {
		DCB DCB;
		COMMTIMEOUTS CommTimeouts;
	};

	return &((struct io_attr *)attr)->CommTimeouts;
}

#endif // _WIN32

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_INTERN_ATTR_H_
