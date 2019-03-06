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

#ifndef LELY_AIO_EXEC_H_
#define LELY_AIO_EXEC_H_

#include <lely/aio/aio.h>

#ifndef LELY_AIO_EXEC_INLINE
#define LELY_AIO_EXEC_INLINE inline
#endif

struct aio_exec_vtbl;
typedef const struct aio_exec_vtbl *const aio_exec_t;

struct aio_task;

#ifdef __cplusplus
extern "C" {
#endif

struct aio_exec_vtbl {
	void (*run)(aio_exec_t *exec, struct aio_task *task);
	void (*dispatch)(aio_exec_t *exec, struct aio_task *task);
	void (*post)(aio_exec_t *exec, struct aio_task *task);
	void (*defer)(aio_exec_t *exec, struct aio_task *task);
	void (*on_task_started)(aio_exec_t *exec);
	void (*on_task_finished)(aio_exec_t *exec);
};

typedef void aio_task_func_t(struct aio_task *task);

struct aio_task {
	aio_exec_t *exec;
	aio_task_func_t *func;
	int errc;
	struct aio_task *_next;
#if LELY_AIO_WITH_IOCP
	struct aio_iocp _iocp;
#endif
};

#if LELY_AIO_WITH_IOCP
#define AIO_TASK_INIT(exec, func) \
	{ \
		(exec), (func), 0, NULL, AIO_IOCP_INIT \
	}
#else
#define AIO_TASK_INIT(exec, func) \
	{ \
		(exec), (func), 0, NULL \
	}
#endif

LELY_AIO_EXEC_INLINE void aio_exec_run(aio_exec_t *exec, struct aio_task *task);
LELY_AIO_EXEC_INLINE void aio_exec_dispatch(
		aio_exec_t *exec, struct aio_task *task);
LELY_AIO_EXEC_INLINE void aio_exec_post(
		aio_exec_t *exec, struct aio_task *task);
LELY_AIO_EXEC_INLINE void aio_exec_defer(
		aio_exec_t *exec, struct aio_task *task);
LELY_AIO_EXEC_INLINE void aio_exec_on_task_started(aio_exec_t *exec);
LELY_AIO_EXEC_INLINE void aio_exec_on_task_finished(aio_exec_t *exec);

void *aio_exec_alloc(void);
void aio_exec_free(void *ptr);
aio_exec_t *aio_exec_init(aio_exec_t *exec, aio_loop_t *loop);
void aio_exec_fini(aio_exec_t *exec);

aio_exec_t *aio_exec_create(aio_loop_t *loop);
void aio_exec_destroy(aio_exec_t *exec);

LELY_AIO_EXEC_INLINE void
aio_exec_run(aio_exec_t *exec, struct aio_task *task)
{
	(*exec)->run(exec, task);
}

LELY_AIO_EXEC_INLINE void
aio_exec_dispatch(aio_exec_t *exec, struct aio_task *task)
{
	(*exec)->dispatch(exec, task);
}

LELY_AIO_EXEC_INLINE void
aio_exec_post(aio_exec_t *exec, struct aio_task *task)
{
	(*exec)->post(exec, task);
}

LELY_AIO_EXEC_INLINE void
aio_exec_defer(aio_exec_t *exec, struct aio_task *task)
{
	(*exec)->defer(exec, task);
}

LELY_AIO_EXEC_INLINE void
aio_exec_on_task_started(aio_exec_t *exec)
{
	(*exec)->on_task_started(exec);
}

LELY_AIO_EXEC_INLINE void
aio_exec_on_task_finished(aio_exec_t *exec)
{
	(*exec)->on_task_finished(exec);
}

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_EXEC_H_
