/*!\file
 * This is the public header file of the asynchronous I/O library.
 *
 * \copyright 2018 Lely Industries N.V.
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

#ifndef LELY_AIO_AIO_H_
#define LELY_AIO_AIO_H_

#include <lely/libc/features.h>

#ifndef LELY_AIO_EXTERN
#ifdef LELY_AIO_INTERN
#define LELY_AIO_EXTERN	LELY_DLL_EXPORT
#else
#define LELY_AIO_EXTERN	LELY_DLL_IMPORT
#endif
#endif

struct aio_context;
typedef struct aio_context aio_context_t;

struct aio_loop;
typedef struct aio_loop aio_loop_t;

struct aio_future;
typedef struct aio_future aio_future_t;

#if _WIN32
typedef HANDLE aio_handle_t;
#else
typedef int aio_handle_t;
#endif

#if _WIN32
#define AIO_INVALID_HANDLE	INVALID_HANDLE_VALUE
#else
#define AIO_INVALID_HANDLE	(-1)
#endif

struct aio_reactor_vtbl;
typedef const struct aio_reactor_vtbl *const aio_reactor_t;

struct aio_watch;

#ifndef LELY_AIO_WITH_IOCP
#if _WIN32

#define LELY_AIO_WITH_IOCP	1

struct aio_iocp {
	OVERLAPPED Overlapped;
	struct aio_watch *watch;
};

#define AIO_IOCP_INIT	{ { NULL, NULL, { { 0, 0 } }, NULL }, NULL }

#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

LELY_AIO_EXTERN int aio_init(void);
LELY_AIO_EXTERN void aio_fini(void);

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_AIO_H_
