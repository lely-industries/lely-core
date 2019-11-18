/**@file
 * This file is part of the utilities library; it contains the
 * setjmp()/longjmp() implementation of the fiber functions.
 *
 * @see fiber/fiber.h
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

// _FORTIFY_SOURCE=2 breaks longjmp() with a different stack frame.
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include "fiber.h"

#if !_WIN32

#include <lely/libc/stddef.h>
#include <lely/util/errnum.h>
#include <lely/util/fiber.h>
#include <lely/util/mkjmp.h>

#include <assert.h>
#include <errno.h>
#ifndef __NEWLIB__
#include <fenv.h>
#endif
#include <stdlib.h>

#if _POSIX_MAPPED_FILES
#include <sys/mman.h>
#include <unistd.h>
#endif

#if LELY_HAVE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
#ifndef LELY_FIBER_MMAPSZ
/**
 * The threshold (in bytes) to use mmap() instead of malloc() to allocate the
 * stack frame of a fiber.
 */
#define LELY_FIBER_MMAPSZ 131072
#endif
#endif

struct fiber_thrd;

struct fiber {
	fiber_func_t *func;
	void *arg;
	int flags;
	void *data;
	struct fiber_thrd *thr;
	fiber_t *from;
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	size_t size;
	void *sp;
#endif
#if LELY_HAVE_VALGRIND
	unsigned id;
#endif
#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
	sigjmp_buf env;
#else
	jmp_buf env;
#endif
#ifndef __NEWLIB__
	fenv_t fenv;
#endif
	int errsv;
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

static _Noreturn void fiber_start(void *arg);

static fiber_t *fiber_switch(fiber_t *fiber);

int
fiber_thrd_init(int flags)
{
	struct fiber_thrd *thr = &fiber_thrd;

	if (flags & ~FIBER_SAVE_ALL) {
#ifdef EINVAL
		errno = EINVAL;
#endif
		return -1;
	}

	if (thr->refcnt++)
		return 1;

	assert(!thr->main.thr);
	assert(!thr->main.from);
	assert(!thr->curr);

	thr->main.flags = flags;
	thr->main.thr = thr;

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
		thr->main.thr = NULL;
		thr->main.flags = 0;
	}
}

fiber_t *
fiber_create(fiber_func_t *func, void *arg, int flags, size_t data_size,
		size_t stack_size)
{
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	if (flags & ~(FIBER_SAVE_ALL | FIBER_GUARD_STACK)) {
#else
	if (flags & ~FIBER_SAVE_ALL) {
#endif
#ifdef EINVAL
		errno = EINVAL;
#endif
		return NULL;
	}

	if (!stack_size)
		stack_size = LELY_FIBER_STKSZ;
	else if (stack_size < LELY_FIBER_MINSTKSZ)
		stack_size = LELY_FIBER_MINSTKSZ;

	size_t size = FIBER_SIZE + data_size;
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	int use_mmap = (flags & FIBER_GUARD_STACK)
			|| ALIGN(size, _Alignof(max_align_t)) + stack_size
					>= LELY_FIBER_MMAPSZ;
	if (!use_mmap)
#endif
		size = ALIGN(size, _Alignof(max_align_t));

	int errsv = errno;

	fiber_t *fiber;
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	if (use_mmap)
		fiber = malloc(size);
	else
#endif
		fiber = malloc(size + stack_size);

	if (!fiber) {
		errsv = errno;
		goto error_malloc_fiber;
	}

	fiber->func = func;
	fiber->arg = arg;
	fiber->flags = flags;

	fiber->data = (char *)fiber + FIBER_SIZE;

	fiber->thr = &fiber_thrd;
	fiber->from = NULL;

	void *sp;
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	fiber->size = 0;
	fiber->sp = NULL;
	if (use_mmap) {
		long page_size = sysconf(_SC_PAGE_SIZE);
		assert(page_size > 0);
		assert(powerof2(page_size));
		stack_size = ALIGN(stack_size, page_size);

		fiber->size = stack_size;
		if (fiber->flags & FIBER_GUARD_STACK)
			// Allocate two guard pages on both ends of the stack.
			// This is the most portable solution, although it does
			// waste a page. If we know in which direction the stack
			// grows, we can omit one of the pages.
			fiber->size += 2 * page_size;

		fiber->sp = mmap(NULL, fiber->size,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (fiber->sp == MAP_FAILED)
			fiber->sp = mmap(NULL, fiber->size,
					PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (fiber->sp == MAP_FAILED) {
			errsv = errno;
			goto error_mmap;
		}
		errno = errsv;

		sp = fiber->sp;
		if (fiber->flags & FIBER_GUARD_STACK) {
			sp = (char *)sp + page_size;
			// clang-format off
			if (mprotect((char *)sp - page_size, page_size,
					PROT_NONE) == -1) {
				// clang-format on
				errsv = errno;
				goto error_mprotect;
			}
			// clang-format off
			if (mprotect((char *)sp + stack_size, page_size,
					PROT_NONE) == -1) {
				// clang-format on
				errsv = errno;
				goto error_mprotect;
			}
		}
	} else
#endif // _POSIX_MAPPED_FILES && MAP_ANONYMOUS
		sp = (char *)fiber + size;

#if LELY_HAVE_VALGRIND
	fiber->id = VALGRIND_STACK_REGISTER(sp, (char *)sp + stack_size);
#endif

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
	// clang-format off
	if (sigmkjmp(fiber->env, fiber->flags & FIBER_SAVE_MASK, &fiber_start,
			fiber, sp, stack_size) == -1) {
		// clang-format on
#else
	if (mkjmp(fiber->env, &fiber_start, fiber, sp, stack_size) == -1) {
#endif
		errsv = errno;
		goto error_mkjmp;
	}

	// Invoke fiber_start(). After setting up the environment it will return
	// here.
	fiber_resume(fiber);

	// Cppcheck gets confused by the lonjmp() performed in fiber_resume()
	// and assumes we never return and therefore leak memory.
	// cppcheck-suppress memleak
	return fiber;

error_mkjmp:
#if LELY_HAVE_VALGRIND
	VALGRIND_STACK_DEREGISTER(fiber->id);
#endif
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
error_mprotect:
	if (use_mmap)
		munmap(fiber->sp, fiber->size);
error_mmap:
#endif
	free(fiber);
error_malloc_fiber:
	errno = errsv;
	return NULL;
}

void
fiber_destroy(fiber_t *fiber)
{
	if (fiber && fiber->data) {
#if LELY_HAVE_VALGRIND
		VALGRIND_STACK_DEREGISTER(fiber->id);
#endif
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
		if (fiber->sp)
			munmap(fiber->sp, fiber->size);
#endif
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

	if (fiber == fiber->thr->curr)
		return fiber->from = fiber;

	return fiber_switch(fiber);
}

fiber_t *
fiber_resume_with(fiber_t *fiber, fiber_func_t *func, void *arg)
{
	if (!fiber)
		fiber = &fiber_thrd.main;

	if (fiber == fiber->thr->curr)
		return fiber->from = func ? func(fiber, arg) : fiber;

	fiber->func = func;
	fiber->arg = arg;

	return fiber_switch(fiber);
}

static _Noreturn void
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
	struct fiber_thrd *thr = fiber->thr;
	assert(thr == &fiber_thrd);
	fiber_t *curr = thr->curr;
	assert(curr);
	assert(fiber != curr);

	if (curr->flags & FIBER_SAVE_ERROR)
		curr->errsv = errno;

#ifndef __NEWLIB__
	if (curr->flags & FIBER_SAVE_FENV)
		fegetenv(&curr->fenv);
#endif

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
	if (!sigsetjmp(curr->env, curr->flags & FIBER_SAVE_MASK)) {
#else
	if (!setjmp(curr->env)) {
#endif
		fiber->from = thr->curr;
		thr->curr = fiber;
#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
		siglongjmp(fiber->env, 1);
#else
		longjmp(fiber->env, 1);
#endif
	}
	curr = thr->curr;

#ifndef __NEWLIB__
	if (curr->flags & FIBER_SAVE_FENV)
		fesetenv(&curr->fenv);
#endif

	if (curr->flags & FIBER_SAVE_ERROR)
		errno = curr->errsv;

	if (curr->func) {
		fiber_func_t *func = curr->func;
		curr->func = NULL;
		void *arg = curr->arg;
		curr->arg = NULL;
		curr->from = func(curr->from, arg);
	}

	return curr->from;
}

#endif // !_WIN32
