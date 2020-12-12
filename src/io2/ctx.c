/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * I/O context functions.
 *
 * @see lely/io2/ctx.h
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

#include "io2.h"

#if !LELY_NO_MALLOC

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/io2/ctx.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

struct io_ctx {
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	struct dllist list;
};

void *
io_ctx_alloc(void)
{
	void *ptr = malloc(sizeof(io_ctx_t));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
io_ctx_free(void *ptr)
{
	free(ptr);
}

io_ctx_t *
io_ctx_init(io_ctx_t *ctx)
{
	assert(ctx);

#if !LELY_NO_THREADS
	if (mtx_init(&ctx->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	dllist_init(&ctx->list);

	return ctx;
}

void
io_ctx_fini(io_ctx_t *ctx)
{
	assert(ctx);

#if LELY_NO_THREADS
	(void)ctx;
#else
	mtx_destroy(&ctx->mtx);
#endif
}

io_ctx_t *
io_ctx_create(void)
{
	int errc = 0;

	io_ctx_t *ctx = io_ctx_alloc();
	if (!ctx) {
		errc = get_errc();
		goto error_alloc;
	}

	io_ctx_t *tmp = io_ctx_init(ctx);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	ctx = tmp;

	return ctx;

error_init:
	io_ctx_free(ctx);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_ctx_destroy(io_ctx_t *ctx)
{
	if (ctx) {
		io_ctx_fini(ctx);
		io_ctx_free(ctx);
	}
}

void
io_ctx_insert(io_ctx_t *ctx, struct io_svc *svc)
{
	assert(ctx);
	assert(svc);

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	dllist_push_back(&ctx->list, &svc->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&ctx->mtx);
#endif
}

void
io_ctx_remove(io_ctx_t *ctx, struct io_svc *svc)
{
	assert(ctx);
	assert(svc);

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	dllist_remove(&ctx->list, &svc->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&ctx->mtx);
#endif
}

int
io_ctx_notify_fork(io_ctx_t *ctx, enum io_fork_event e)
{
	assert(ctx);

	int result = 0;
	int errc = get_errc();

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	// clang-format off
	struct dlnode *node = e == IO_FORK_PREPARE
			? dllist_last(&ctx->list)
			: dllist_first(&ctx->list);
	// clang-format on
	while (node) {
		struct io_svc *svc = structof(node, struct io_svc, _node);
		node = e == IO_FORK_PREPARE ? node->prev : node->next;

		const struct io_svc_vtbl *vptr = svc->vptr;
#if !LELY_NO_THREADS
		mtx_unlock(&ctx->mtx);
#endif
		assert(vptr);
		if (vptr->notify_fork) {
			set_errc(0);
			if (vptr->notify_fork(svc, e) == -1 && !result) {
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
io_ctx_shutdown(io_ctx_t *ctx)
{
	assert(ctx);

#if !LELY_NO_THREADS
	mtx_lock(&ctx->mtx);
#endif
	struct dlnode *node = dllist_last(&ctx->list);
	while (node) {
		struct io_svc *svc = structof(node, struct io_svc, _node);
		node = node->prev;

		if (svc->_shutdown)
			continue;
		svc->_shutdown = 1;

		const struct io_svc_vtbl *vptr = svc->vptr;
#if !LELY_NO_THREADS
		mtx_unlock(&ctx->mtx);
#endif
		assert(vptr);
		if (vptr->shutdown)
			vptr->shutdown(svc);
#if !LELY_NO_THREADS
		mtx_lock(&ctx->mtx);
#endif
	}
#if !LELY_NO_THREADS
	mtx_unlock(&ctx->mtx);
#endif
}

#endif // !LELY_NO_MALLOC
