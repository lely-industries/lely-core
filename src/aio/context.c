/**@file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * @see lely/aio/exec.h
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

#include "aio.h"
#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/aio/context.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

struct aio_context {
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	struct dllist list;
};

static size_t aio_context_init_cnt;

void *
aio_context_alloc(void)
{
	void *ptr = malloc(sizeof(aio_context_t));
	if (!ptr)
		set_errc(errno2c(errno));
	return ptr;
}

void
aio_context_free(void *ptr)
{
	free(ptr);
}

aio_context_t *
aio_context_init(aio_context_t *ctx)
{
	assert(ctx);

	int errc = 0;

	if (!aio_context_init_cnt++ && aio_init() == -1) {
		errc = get_errc();
		goto error_init_aio;
	}

#if !LELY_NO_THREADS
	if (mtx_init(&ctx->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}
#endif

	dllist_init(&ctx->list);

	return ctx;

#if !LELY_NO_THREADS
error_init_mtx:
#endif
	if (!--aio_context_init_cnt)
		aio_fini();
error_init_aio:
	set_errc(errc);
	return NULL;
}

void
aio_context_fini(aio_context_t *ctx)
{
	assert(ctx);
	assert(aio_context_init_cnt);

#if LELY_NO_THREADS
	(void)ctx;
#else
	mtx_destroy(&ctx->mtx);
#endif

	if (!--aio_context_init_cnt)
		aio_fini();
}

aio_context_t *
aio_context_create(void)
{
	int errc = 0;

	aio_context_t *ctx = aio_context_alloc();
	if (!ctx) {
		errc = get_errc();
		goto error_alloc;
	}

	aio_context_t *tmp = aio_context_init(ctx);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	ctx = tmp;

	return ctx;

error_init:
	aio_context_free(ctx);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
aio_context_destroy(aio_context_t *ctx)
{
	if (ctx) {
		aio_context_fini(ctx);
		aio_context_free(ctx);
	}
}

void
aio_context_insert(aio_context_t *ctx, struct aio_service *srv)
{
	assert(ctx);
	assert(srv);

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	dllist_push_back(&ctx->list, &srv->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&ctx->mtx);
#endif
}

void
aio_context_remove(aio_context_t *ctx, struct aio_service *srv)
{
	assert(ctx);
	assert(srv);

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	dllist_remove(&ctx->list, &srv->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&ctx->mtx);
#endif
}

int
aio_context_notify_fork(aio_context_t *ctx, enum aio_fork_event e)
{
	assert(ctx);

	int result = 0;
	int errc = get_errc();

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	// clang-format off
	struct dlnode *node = e == AIO_FORK_PREPARE
			? dllist_last(&ctx->list) : dllist_first(&ctx->list);
	// clang-format on
	while (node) {
		struct aio_service *srv =
				structof(node, struct aio_service, _node);
		node = e == AIO_FORK_PREPARE ? node->prev : node->next;

		const struct aio_service_vtbl *vptr = srv->vptr;
#if !LELY_NO_THREADS
		mtx_unlock(&ctx->mtx);
#endif
		assert(vptr);
		if (vptr->notify_fork) {
			set_errc(0);
			if (vptr->notify_fork(srv, e) == -1 && !result) {
				errc = get_errc();
				result = -1;
			}
		}
#if !LELY_NO_THREADS
		mtx_lock(&ctx->mtx);
#endif
	}
#if !LELY_NO_THREADS
	mtx_unlock(&ctx->mtx);
#endif

	set_errc(errc);
	return result;
}

void
aio_context_shutdown(aio_context_t *ctx)
{
	assert(ctx);

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	struct dlnode *node = dllist_last(&ctx->list);
	while (node) {
		struct aio_service *srv =
				structof(node, struct aio_service, _node);
		node = node->prev;

		if (srv->_shutdown)
			continue;
		srv->_shutdown = 1;

		const struct aio_service_vtbl *vptr = srv->vptr;
#if !LELY_NO_THREADS
		mtx_unlock(&ctx->mtx);
#endif
		assert(vptr);
		if (vptr->shutdown)
			vptr->shutdown(srv);
#if !LELY_NO_THREADS
		mtx_lock(&ctx->mtx);
#endif
	}
#if !LELY_NO_THREADS
	mtx_unlock(&ctx->mtx);
#endif
}
