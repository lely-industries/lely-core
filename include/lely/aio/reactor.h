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

#ifndef LELY_AIO_REACTOR_H_
#define LELY_AIO_REACTOR_H_

#include <lely/aio/poll.h>

#if !LELY_AIO_WITH_IOCP
#include <lely/util/rbtree.h>
#endif

#ifndef LELY_AIO_REACTOR_INLINE
#define LELY_AIO_REACTOR_INLINE inline
#endif

enum aio_watch_event {
	AIO_WATCH_READ = 1u << 0,
	AIO_WATCH_WRITE = 1u << 1,
	AIO_WATCH_ERROR = 1u << 2
};

struct aio_watch {
#if LELY_AIO_WITH_IOCP
	void (*func)(struct aio_watch *watch, struct aio_task *task, int errc,
			size_t nbytes);
#else
	int (*func)(struct aio_watch *watch, int events);
	aio_handle_t _handle;
	struct rbnode _node;
	int _events;
#endif
};

#if LELY_AIO_WITH_IOCP
#define AIO_WATCH_INIT(func) \
	{ \
		(func) \
	}
#else
#define AIO_WATCH_INIT(func) \
	{ \
		(func), AIO_INVALID_HANDLE, { NULL, 0, NULL, NULL }, 0 \
	}
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct aio_reactor_vtbl {
	aio_context_t *(*get_context)(const aio_reactor_t *reactor);
	aio_poll_t *(*get_poll)(const aio_reactor_t *reactor);
#if LELY_AIO_WITH_IOCP
	int (*watch)(aio_reactor_t *reactor, struct aio_watch *watch,
			aio_handle_t handle);
#else
	int (*watch)(aio_reactor_t *reactor, struct aio_watch *watch,
			aio_handle_t handle, int events);
#endif
};

LELY_AIO_REACTOR_INLINE aio_context_t *aio_reactor_get_context(
		const aio_reactor_t *reactor);
LELY_AIO_REACTOR_INLINE aio_poll_t *aio_reactor_get_poll(
		const aio_reactor_t *reactor);
#if LELY_AIO_WITH_IOCP
LELY_AIO_REACTOR_INLINE int aio_reactor_watch(aio_reactor_t *reactor,
		struct aio_watch *watch, aio_handle_t handle);
#else
LELY_AIO_REACTOR_INLINE int aio_reactor_watch(aio_reactor_t *reactor,
		struct aio_watch *watch, aio_handle_t handle, int events);
#endif

void *aio_reactor_alloc(void);
void aio_reactor_free(void *ptr);
aio_reactor_t *aio_reactor_init(aio_reactor_t *reactor, aio_context_t *ctx);
void aio_reactor_fini(aio_reactor_t *reactor);

aio_reactor_t *aio_reactor_create(aio_context_t *ctx);
void aio_reactor_destroy(aio_reactor_t *reactor);

LELY_AIO_REACTOR_INLINE
aio_context_t *
aio_reactor_get_context(const aio_reactor_t *reactor)
{
	return (*reactor)->get_context(reactor);
}

LELY_AIO_REACTOR_INLINE aio_poll_t *
aio_reactor_get_poll(const aio_reactor_t *reactor)
{
	return (*reactor)->get_poll(reactor);
}

#if LELY_AIO_WITH_IOCP
LELY_AIO_REACTOR_INLINE int
aio_reactor_watch(aio_reactor_t *reactor, struct aio_watch *watch,
		aio_handle_t handle)
{
	return (*reactor)->watch(reactor, watch, handle);
}
#else
LELY_AIO_REACTOR_INLINE int
aio_reactor_watch(aio_reactor_t *reactor, struct aio_watch *watch,
		aio_handle_t handle, int events)
{
	return (*reactor)->watch(reactor, watch, handle, events);
}
#endif

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_REACTOR_H_
