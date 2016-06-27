/*!\file
 * This file is part of the I/O library; it contains the implementation of the
 * I/O handle functions.
 *
 * \see src/handle.h
 *
 * \copyright 2016 Lely Industries N.V.
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

#include "io.h"
#include <lely/util/errnum.h>
#include "handle.h"

#include <assert.h>
#include <stdlib.h>

struct io_handle *
io_handle_alloc(const struct io_handle_vtab *vtab)
{
	assert(vtab);
	assert(vtab->size >= sizeof(struct io_handle));

	struct io_handle *handle = malloc(vtab->size);
	if (__unlikely(!handle)) {
		set_errno(errno);
		return NULL;
	}

	handle->vtab = vtab;
#ifndef LELY_NO_ATOMICS
	atomic_init(&handle->ref, 0);
#else
	handle->ref = 0;
#endif

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
	free(handle);
}

void
io_handle_destroy(struct io_handle *handle)
{
	if (handle) {
		io_handle_fini(handle);
		io_handle_free(handle);
	}
}

