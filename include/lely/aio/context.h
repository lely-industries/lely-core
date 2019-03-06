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

#ifndef LELY_AIO_CONTEXT_H_
#define LELY_AIO_CONTEXT_H_

#include <lely/aio/aio.h>
#include <lely/util/dllist.h>

enum aio_fork_event { AIO_FORK_PREPARE, AIO_FORK_PARENT, AIO_FORK_CHILD };

struct aio_service_vtbl;

struct aio_service {
	const struct aio_service_vtbl *vptr;
	int _shutdown;
	struct dlnode _node;
};

#define AIO_SERVICE_INIT(vptr) \
	{ \
		(vptr), 0, { NULL, NULL } \
	}

#ifdef __cplusplus
extern "C" {
#endif

struct aio_service_vtbl {
	int (*notify_fork)(struct aio_service *srv, enum aio_fork_event e);
	void (*shutdown)(struct aio_service *srv);
};

void *aio_context_alloc(void);
void aio_context_free(void *ptr);
aio_context_t *aio_context_init(aio_context_t *ctx);
void aio_context_fini(aio_context_t *ctx);

aio_context_t *aio_context_create(void);
void aio_context_destroy(aio_context_t *ctx);

void aio_context_insert(aio_context_t *ctx, struct aio_service *srv);
void aio_context_remove(aio_context_t *ctx, struct aio_service *srv);

int aio_context_notify_fork(aio_context_t *ctx, enum aio_fork_event e);
void aio_context_shutdown(aio_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_CONTEXT_H_
