/**@file
 * This file is part of the utilities library; it contains the Windows
 * implemenation of the fiber functions.
 *
 * @see lely/util/fiber.h
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

#include "fiber.h"

#if _WIN32

#include <lely/libc/stddef.h>
#include <lely/util/fiber.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <windows.h>

struct fiber {
	fiber_func_t *func;
	void *arg;
	int flags;
	void *data;
	fiber_t *from;
	LPVOID lpFiber;
	int errsv;
	DWORD dwErrCode;
};

#define FIBER_SIZE ALIGN(sizeof(fiber_t), _Alignof(max_align_t))

#if LELY_NO_THREADS
static struct fiber_thrd {
#else
static _Thread_local struct fiber_thrd {
#endif
	size_t refcnt;
	fiber_t main;
	fiber_t *curr;
} fiber_thrd;

static _Noreturn void CALLBACK fiber_start(void *arg);

static fiber_t *fiber_switch(fiber_t *fiber);

int
fiber_thrd_init(int flags)
{
	struct fiber_thrd *thr = &fiber_thrd;

	if (flags & ~(int)FIBER_SAVE_ALL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	if (thr->refcnt++)
		return 1;

	assert(!thr->main.lpFiber);
	assert(!thr->main.from);
	assert(!thr->curr);

	thr->main.flags = flags;
	DWORD dwFlags = 0;
	if (thr->main.flags & FIBER_SAVE_FENV)
		dwFlags |= FIBER_FLAG_FLOAT_SWITCH;
	thr->main.lpFiber = ConvertThreadToFiberEx(NULL, dwFlags);
	if (!thr->main.lpFiber) {
		thr->main.flags = 0;
		thr->refcnt--;
		return -1;
	}

	thr->curr = &thr->main;

	return 0;
}

void
fiber_thrd_fini(void)
{
	struct fiber_thrd *thr = &fiber_thrd;
	assert(thr->refcnt);

	if (!--thr->refcnt) {
		assert(thr->curr == &thr->main);

		thr->curr = NULL;

		thr->main.from = NULL;
		assert(thr->main.lpFiber);
		ConvertFiberToThread();
		thr->main.lpFiber = NULL;
		thr->main.flags = 0;
	}
}

fiber_t *
fiber_create(fiber_func_t *func, void *arg, int flags, size_t data_size,
		size_t stack_size)
{
	if (flags & ~(int)FIBER_SAVE_ALL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	if (!stack_size)
		stack_size = LELY_FIBER_STKSZ;
	else if (stack_size < LELY_FIBER_MINSTKSZ)
		stack_size = LELY_FIBER_MINSTKSZ;

	DWORD dwErrCode = 0;

	fiber_t *fiber = malloc(FIBER_SIZE + data_size);
	if (!fiber) {
		dwErrCode = GetLastError();
		goto error_malloc_fiber;
	}

	fiber->func = func;
	fiber->arg = arg;
	fiber->flags = flags;

	fiber->data = (char *)fiber + FIBER_SIZE;

	fiber->from = NULL;

	DWORD dwFlags = 0;
	if (fiber->flags & FIBER_SAVE_FENV)
		dwFlags |= FIBER_FLAG_FLOAT_SWITCH;
	fiber->lpFiber = CreateFiberEx(
			stack_size, 0, dwFlags, &fiber_start, fiber);
	if (!fiber->lpFiber) {
		dwErrCode = GetLastError();
		goto error_CreateFiberEx;
	}

	// Invoke fiber_start(). After setting up the environment it will return
	// here.
	fiber_resume(fiber);

	// Cppcheck gets confused by fiber_resume() and thinks we leak memory.
	// cppcheck-suppress memleak
	return fiber;

	DeleteFiber(fiber->lpFiber);
error_CreateFiberEx:
	free(fiber);
error_malloc_fiber:
	SetLastError(dwErrCode);
	return NULL;
}

void
fiber_destroy(fiber_t *fiber)
{
	if (fiber && fiber->data) {
		DeleteFiber(fiber->lpFiber);
		free(fiber);
	}
}

void *
fiber_data(const fiber_t *fiber)
{
	assert(fiber_thrd.curr);

	return fiber ? fiber->data : fiber_thrd.curr->data;
}

fiber_t *
fiber_resume(fiber_t *fiber)
{
	if (!fiber)
		fiber = &fiber_thrd.main;

	if (fiber == fiber_thrd.curr)
		return fiber->from = fiber;

	return fiber_switch(fiber);
}

fiber_t *
fiber_resume_with(fiber_t *fiber, fiber_func_t *func, void *arg)
{
	if (!fiber)
		fiber = &fiber_thrd.main;

	if (fiber == fiber_thrd.curr)
		return fiber->from = func ? func(fiber, arg) : fiber;

	fiber->func = func;
	fiber->arg = arg;

	return fiber_switch(fiber);
}

static _Noreturn void CALLBACK
fiber_start(void *arg)
{
	fiber_t *fiber = arg;
	assert(fiber);

	// Copy the function to be executed to the stack for later reference.
	fiber_func_t *func = fiber->func;
	fiber->func = NULL;
	arg = fiber->arg;
	fiber->arg = NULL;

	// Resume fiber_create().
	fiber = fiber_resume(fiber->from);

	// If the fiber is started with fiber_resume_with(), execute that
	// function before starting the original function.
	if (fiber->func) {
		fiber_func_t *func = fiber->func;
		fiber->func = NULL;
		void *arg = fiber->arg;
		fiber->arg = NULL;
		fiber = func(fiber, arg);
	}

	if (func)
		fiber = func(fiber, arg);

	// If no valid fiber is returned, return to the main context.
	if (!fiber)
		fiber = &fiber_thrd.main;

	// The function has terminated, so return to the caller immediately.
	for (;;)
		fiber = fiber_resume(fiber);
}

static fiber_t *
fiber_switch(fiber_t *fiber)
{
	assert(fiber);
	struct fiber_thrd *thr = &fiber_thrd;
	fiber_t *curr = thr->curr;
	assert(curr);
	assert(fiber != curr);

	if (curr->flags & FIBER_SAVE_ERROR) {
		curr->dwErrCode = GetLastError();
		curr->errsv = errno;
	}

	fiber->from = thr->curr;
	thr->curr = fiber;
	assert(fiber->lpFiber);
	SwitchToFiber(fiber->lpFiber);
	assert(curr == thr->curr);

	if (curr->flags & FIBER_SAVE_ERROR) {
		errno = curr->errsv;
		SetLastError(curr->dwErrCode);
	}

	if (curr->func) {
		fiber_func_t *func = curr->func;
		curr->func = NULL;
		void *arg = curr->arg;
		curr->arg = NULL;
		curr->from = func(curr->from, arg);
	}

	return curr->from;
}

#endif // _WIN32
