/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * I/O handle functions.
 *
 * @see src/handle.h
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#include "io.h"

#if !LELY_NO_STDIO

#include <lely/util/errnum.h>

#include <assert.h>
#include <stdlib.h>

#include "handle.h"

io_handle_t
io_handle_acquire(io_handle_t handle)
{
	if (handle != IO_HANDLE_ERROR)
#if !LELY_NO_ATOMICS
		atomic_fetch_add_explicit(
				&handle->ref, 1, memory_order_relaxed);
#elif !LELY_NO_THREADS && _WIN32
		InterlockedIncrementNoFence(&handle->ref);
#else
		handle->ref++;
#endif
	return handle;
}

void
io_handle_release(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR)
		return;

#if !LELY_NO_ATOMICS
	if (atomic_fetch_sub_explicit(&handle->ref, 1, memory_order_release)
			== 1) {
		atomic_thread_fence(memory_order_acquire);
#elif !LELY_NO_THREADS && _WIN32
	if (!InterlockedDecrementRelease(&handle->ref)) {
		MemoryBarrier();
#else
	if (handle->ref-- == 1) {
#endif
		io_handle_destroy(handle);
	}
}

int
io_handle_unique(io_handle_t handle)
{
#if !LELY_NO_ATOMICS
	return handle != IO_HANDLE_ERROR && atomic_load(&handle->ref) == 1;
#else
	return handle != IO_HANDLE_ERROR && handle->ref == 1;
#endif
}

struct io_handle *
io_handle_alloc(const struct io_handle_vtab *vtab)
{
	assert(vtab);
	assert(vtab->size >= sizeof(struct io_handle));

	struct io_handle *handle = malloc(vtab->size);
	if (!handle) {
		set_errc(errno2c(errno));
		return NULL;
	}

	handle->vtab = vtab;
#if !LELY_NO_ATOMICS
	atomic_init(&handle->ref, 0);
#else
	handle->ref = 0;
#endif
	handle->fd = INVALID_HANDLE_VALUE;

#if !LELY_NO_THREADS
	mtx_init(&handle->mtx, mtx_plain);
#endif

	handle->flags = 0;

	return handle;
}

void
io_handle_fini(struct io_handle *handle)
{
	assert(handle);
	assert(handle->vtab);

	if (handle->vtab->fini)
		handle->vtab->fini(handle);
}

void
io_handle_free(struct io_handle *handle)
{
	if (handle) {
#if !LELY_NO_THREADS
		mtx_destroy(&handle->mtx);
#endif

		free(handle);
	}
}

void
io_handle_destroy(struct io_handle *handle)
{
	if (handle) {
		io_handle_fini(handle);
		io_handle_free(handle);
	}
}

#if !LELY_NO_THREADS

void
io_handle_lock(struct io_handle *handle)
{
	assert(handle);

	mtx_lock(&handle->mtx);
}

void
io_handle_unlock(struct io_handle *handle)
{
	assert(handle);

	mtx_unlock(&handle->mtx);
}

#endif // !LELY_NO_THREADS

#endif // !LELY_NO_STDIO
