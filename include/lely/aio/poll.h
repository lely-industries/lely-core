/**@file
 * This header file is part of the asynchronous I/O library; it contains ...
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

#ifndef LELY_AIO_POLL_H_
#define LELY_AIO_POLL_H_

#include <lely/aio/aio.h>

#include <stddef.h>

#ifndef LELY_AIO_POLL_INLINE
#define LELY_AIO_POLL_INLINE inline
#endif

struct aio_poll_vtbl;
typedef const struct aio_poll_vtbl *const aio_poll_t;

#ifdef __cplusplus
extern "C" {
#endif

struct aio_poll_vtbl {
	size_t (*wait)(aio_poll_t *poll, int timeout);
	void (*stop)(aio_poll_t *poll);
};

LELY_AIO_POLL_INLINE size_t aio_poll_wait(aio_poll_t *poll, int timeout);
LELY_AIO_POLL_INLINE void aio_poll_stop(aio_poll_t *poll);

LELY_AIO_POLL_INLINE size_t
aio_poll_wait(aio_poll_t *poll, int timeout)
{
	return (*poll)->wait(poll, timeout);
}

LELY_AIO_POLL_INLINE void
aio_poll_stop(aio_poll_t *poll)
{
	(*poll)->stop(poll);
}

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_POLL_H_
