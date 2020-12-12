/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the fiber functions.
 *
 * @see lely/util/fiber.h
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#if !_WIN32
// _FORTIFY_SOURCE=2 breaks longjmp() with a different stack frame.
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
#endif

#include "util.h"

#if !LELY_NO_MALLOC

#include <lely/libc/stddef.h>
#include <lely/util/errnum.h>
#include <lely/util/fiber.h>
#if !_WIN32
#include <lely/util/mkjmp.h>
#endif
#include <lely/util/util.h>

#include <assert.h>
#if !_WIN32 && !defined(__NEWLIB__)
#include <fenv.h>
#endif
#include <stdlib.h>

#if _WIN32
#include <windows.h>
#elif _POSIX_MAPPED_FILES
#include <sys/mman.h>
#include <unistd.h>
#endif

#if !_WIN32 && LELY_HAVE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#ifndef LELY_FIBER_MINSTKSZ
/// The minimum size (in bytes) of a fiber stack frame.
#ifdef MINSIGSTKSZ
#define LELY_FIBER_MINSTKSZ MINSIGSTKSZ
#elif __WORDSIZE == 64
#define LELY_FIBER_MINSTKSZ 8192
#else
#define LELY_FIBER_MINSTKSZ 4096
#endif
#endif

#ifndef LELY_FIBER_STKSZ
/// The default size (in bytes) of a fiber stack frame.
#define LELY_FIBER_STKSZ 131072
#endif

#if !_WIN32 && _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)

/*
 * The size (in bytes) of a memory page. The value is stored in a static
 * variable to prevent multiple calls to sysconf().
 */
static long page_size;

/**
 * Creates an anonymous private mapping of the specified size, surrounded by
 * guard pages on either side.
 *
 * @param addr a hint for the address at which to create the mapping.
 * @param len  the length of the mapping, excluding the guard pages.
 * @param prot the desired memory protection of the mapping, excluding the guard
 *             pages.
 *
 * @returns a pointer to the first usable byte in the mapping (i.e., after the
 * guard page), or NULL on error. In the latter case, the error number can be
 * obtained with get_errc().
 *
 * @see guard_munmap()
 */
static void *guard_mmap(void *addr, size_t len, int prot);

/**
 * Unmaps an anonymous private mapping created by guard_mmap().
 *
 * @param addr the address of a mapping returned by guard_mmap().
 * @param len  the size of the mapping.
 */
static void guard_munmap(void *addr, size_t len);

#endif // !_WIN32 && _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)

#if !_WIN32

/**
 * Saves the <b>from</b> calling environment with `setjmp(from)` and restores
 * the <b>to</b> calling environment with `longjmp(to, 1)`.
 */
static inline void jmpto(jmp_buf from, jmp_buf to);

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
/**
 * Saves the <b>from</b> calling environment with `sigsetjmp(from, savemask)`
 * and restores the <b>to</b> calling environment with `siglongjmp(to, 1)`.
 */
static inline void sigjmpto(sigjmp_buf from, sigjmp_buf to, int savemask);
#endif

#endif // !_WIN32

struct fiber_thrd;

/// A fiber.
struct fiber {
	/// A pointer to the function to be executed in the fiber.
	fiber_func_t *func;
	/// The second argument supplied to #func.
	void *arg;
	/// The flags provided to fiber_create().
	int flags;
	/// A pointer to the data region.
	void *data;
	/// A pointer to the thread that resumed this fiber.
	struct fiber_thrd *thr;
	/// A pointer to the now suspended fiber that resumed this fiber.
	fiber_t *from;
#if _WIN32
	/// A address of the native Windows fiber.
	LPVOID lpFiber;
#else
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	/// The size of the mapping at #stack_addr, excluding the guard pages.
	size_t stack_size;
	/**
	 * The address of the anonymous private mapping used for the stack,
	 * excluding the guard pages.
	 */
	void *stack_addr;
#endif
#if LELY_HAVE_VALGRIND
	/// The Valgrind stack id for the fiber stack.
	unsigned id;
#endif
#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
	/// The saved registers and signal mask.
	sigjmp_buf env;
#else
	/// The saved registers.
	jmp_buf env;
#endif
#endif
};

#define FIBER_ALIGNOF _Alignof(max_align_t)
#define FIBER_SIZEOF ALIGN(sizeof(fiber_t), FIBER_ALIGNOF)

/**
 * A thread-local struct containing the fiber associated with the thread and
 * a pointer to the fiber currently running in the thread.
 */
#if LELY_NO_THREADS
static struct fiber_thrd {
#else
static _Thread_local struct fiber_thrd {
#endif
	/**
	 * The reference counter tracking the number of calls to
	 * fiber_thrd_init() minus those to fiber_thrd_fini().
	 */
	size_t refcnt;
	/**
	 * The fiber representing this thread. Pointers to this fiber are
	 * converted to and from NULL to prevent the address from being exposed
	 * to the user. If it was, the user could try to resume this fiber from
	 * another thread, which is impossible.
	 */
	fiber_t main;
	/// A pointer to the fiber currently running on this thread.
	fiber_t *curr;
} fiber_thrd;

/**
 * The function running in a fiber. This function never returns. Instead, after
 * the user-speficied function finishes, it continuously resumes the fiber that
 * resumes it.
 */
#if _WIN32
static _Noreturn void CALLBACK fiber_start(void *arg);
#else
static _Noreturn void fiber_start(void *arg);
#endif

int
fiber_thrd_init(int flags)
{
	struct fiber_thrd *thr = &fiber_thrd;
	assert(!thr->curr || thr->curr == &thr->main);

	if (flags & ~FIBER_SAVE_ALL) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (thr->refcnt++)
		return 1;

#if _WIN32
	DWORD dwFlags = 0;
	if (flags & FIBER_SAVE_FENV)
		dwFlags |= FIBER_FLAG_FLOAT_SWITCH;
	assert(!thr->main.lpFiber);
	thr->main.lpFiber = ConvertThreadToFiberEx(NULL, dwFlags);
	if (!thr->main.lpFiber)
		return -1;
#endif
	thr->main.thr = thr;
	thr->main.flags = flags;

	assert(!thr->curr);
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
#if _WIN32
		assert(thr->main.lpFiber);
		ConvertFiberToThread();
		thr->main.lpFiber = NULL;
#endif
		thr->main.flags = 0;
	}
}

fiber_t *
fiber_create(fiber_func_t *func, void *arg, int flags, size_t data_size,
		size_t stack_size)
{
	struct fiber_thrd *thr = &fiber_thrd;
	assert(thr->curr);

#if !_WIN32 && _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	if (flags & ~(FIBER_SAVE_ALL | FIBER_GUARD_STACK)) {
#else
	if (flags & ~FIBER_SAVE_ALL) {
#endif
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}

	data_size = ALIGN(data_size, FIBER_ALIGNOF);

	if (!stack_size)
		stack_size = LELY_FIBER_STKSZ;
	else if (stack_size < LELY_FIBER_MINSTKSZ)
		stack_size = LELY_FIBER_MINSTKSZ;
	stack_size = ALIGN(stack_size, FIBER_ALIGNOF);

	size_t size = FIBER_SIZEOF + data_size;
#if !_WIN32
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	if (!(flags & FIBER_GUARD_STACK))
#endif
		size += stack_size;
#endif

	int errc = 0;

	fiber_t *fiber = malloc(size);
	if (!fiber) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_malloc_fiber;
	}

	// The function pointer is stored by the call to fiber_resume_with()
	// below.
	fiber->func = NULL;
	fiber->arg = NULL;

	fiber->flags = flags;

	// The data region immediately follows the fiber struct.
	fiber->data = (char *)fiber + FIBER_SIZEOF;

	fiber->thr = thr;
	fiber->from = NULL;

#if !_WIN32
	void *sp;
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
	fiber->stack_size = 0;
	fiber->stack_addr = NULL;
	if (flags & FIBER_GUARD_STACK) {
		// Create an anonymous private mapping for the stack with guard
		// pages at both ends. This is the most portable solution,
		// although it does waste a page. If we know in which direction
		// the stack grows, we could omit one of the pages.
		fiber->stack_size = stack_size;
		fiber->stack_addr = guard_mmap(NULL, fiber->stack_size,
				PROT_READ | PROT_WRITE);
		if (!fiber->stack_addr) {
			errc = get_errc();
			goto error_create_stack;
		}
		sp = fiber->stack_addr;
	} else
#endif
		sp = (char *)fiber->data + data_size;
#if LELY_HAVE_VALGRIND
	// Register the stack with Valgrind to avoid false positives.
	fiber->id = VALGRIND_STACK_REGISTER(sp, (char *)sp + stack_size);
#endif
#endif // !_WIN32

	// Construct the fiber context.
#if _WIN32
	DWORD dwFlags = 0;
	if (fiber->flags & FIBER_SAVE_FENV)
		dwFlags |= FIBER_FLAG_FLOAT_SWITCH;
	fiber->lpFiber = CreateFiberEx(
			stack_size, 0, dwFlags, &fiber_start, fiber);
	if (!fiber->lpFiber) {
		errc = get_errc();
		goto error_CreateFiberEx;
	}
#else
#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
	// clang-format off
	if (sigmkjmp(fiber->env, fiber->flags & FIBER_SAVE_MASK, &fiber_start,
			fiber, sp, stack_size) == -1) {
		// clang-format on
#else
	if (mkjmp(fiber->env, &fiber_start, fiber, sp, stack_size) == -1) {
#endif
		errc = get_errc();
		goto error_mkjmp;
	}
#endif

	// Invoke fiber_start() and copy the user-specified function to the
	// fiber stack. After setting up the stack it will return here.
	fiber_resume_with(fiber, func, arg);

	// Cppcheck gets confused by fiber_resume() and thinks we leak memory.
	// cppcheck-suppress memleak
	return fiber;

#if _WIN32
	// DeleteFiber(fiber->lpFiber);
error_CreateFiberEx:
#else
error_mkjmp:
#if LELY_HAVE_VALGRIND
	VALGRIND_STACK_DEREGISTER(fiber->id);
#endif
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
error_create_stack:
	if (fiber->stack_addr)
		guard_munmap(fiber->stack_addr, fiber->stack_size);
#endif
#endif
	free(fiber);
error_malloc_fiber:
	set_errc(errc);
	return NULL;
}

void
fiber_destroy(fiber_t *fiber)
{
	if (fiber) {
		assert(fiber != &fiber_thrd.main);
		assert(fiber != fiber_thrd.curr);
#if _WIN32
		DeleteFiber(fiber->lpFiber);
#else
#if LELY_HAVE_VALGRIND
		VALGRIND_STACK_DEREGISTER(fiber->id);
#endif
#if _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)
		if (fiber->stack_addr)
			guard_munmap(fiber->stack_addr, fiber->stack_size);
#endif
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
	return fiber_resume_with(fiber, NULL, NULL);
}

fiber_t *
fiber_resume_with(fiber_t *fiber, fiber_func_t *func, void *arg)
{
	struct fiber_thrd *thr = &fiber_thrd;
	fiber_t *curr = thr->curr;
	assert(curr);
	fiber_t *to = fiber ? fiber : &thr->main;
	assert(to);
	assert(!to->func);
	assert(!to->arg);

	// If a fiber is resuming itself, execute the function and return.
	if (to == curr) {
		if (func)
			fiber = func(fiber, arg);
		return fiber;
	}

	// Save the function to be executed.
	if (func) {
		to->func = func;
		to->arg = arg;
	}

	// Save the error code(s). On Windows, the thread's last-error code
	// reported by GetLastError() is distinct from errno, but it is equal to
	// the error status reported by WSAGetLastError().
#if _WIN32
	DWORD dwErrCode = 0;
	int errsv = 0;
	if (curr->flags & FIBER_SAVE_ERROR) {
		dwErrCode = GetLastError();
		errsv = errno;
	}
#else
	int errc = 0;
	if (curr->flags & FIBER_SAVE_ERROR)
		errc = get_errc();
#endif

	// Save the floating-point environment. Windows fibers can do this
	// automatically, depending on the flags provided to CreateFiberEx() or
	// ConvertThreadToFiberEx().
#if !_WIN32 && !defined(__NEWLIB__)
	fenv_t fenv;
	if (curr->flags & FIBER_SAVE_FENV)
		fegetenv(&fenv);
#endif

	to->thr = thr;
	to->from = curr;
	thr->curr = to;
	// Perform the actual context switch.
#if _WIN32
	assert(to->lpFiber);
	SwitchToFiber(to->lpFiber);
#elif _POSIX_C_SOURCE >= 200112L \
		&& (!defined(__NEWLIB__) || defined(__CYGWIN__))
	sigjmpto(curr->env, to->env, curr->flags & FIBER_SAVE_MASK);
#else
	jmpto(curr->env, to->env);
#endif
	thr = curr->thr;
	assert(thr);
	assert(curr->from);
	fiber = curr->from != &thr->main ? curr->from : NULL;

	// Restore the floating-point environment.
#if !_WIN32 && !defined(__NEWLIB__)
	if (curr->flags & FIBER_SAVE_FENV)
		fesetenv(&fenv);
#endif

	// Restore the error code(s).
	if (curr->flags & FIBER_SAVE_ERROR) {
#if _WIN32
		errno = errsv;
		SetLastError(dwErrCode);
#else
		set_errc(errc);
#endif
	}

	// Execute the saved function, if any, in this fiber before resuming the
	// suspended function.
	if (curr->func) {
		func = curr->func;
		curr->func = NULL;
		arg = curr->arg;
		curr->arg = NULL;
		fiber = func(fiber, arg);
	}

	return fiber;
}

#if !_WIN32 && _POSIX_MAPPED_FILES && defined(MAP_ANONYMOUS)

static void *
guard_mmap(void *addr, size_t len, int prot)
{
	if (!page_size)
		page_size = sysconf(_SC_PAGE_SIZE);
	assert(page_size > 0);
	assert(powerof2(page_size));

	// Adjust the hint, if any, to accomodate the guard page.
	if (addr)
		addr = (char *)addr - page_size;

	// Round the length up to the nearest multiple of the page size.
	len = ALIGN(len, page_size);

	int errc = 0;

	// Create a single anonymous private mapping that includes the guard
	// pages.
	addr = mmap(addr, len + 2 * page_size, prot,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (addr == MAP_FAILED) {
		errc = get_errc();
		goto error_mmap;
	}
	addr = (char *)addr + page_size;

	// Guard the page at the beginning.
	if (mprotect((char *)addr - page_size, page_size, PROT_NONE) == -1) {
		errc = get_errc();
		goto error_mprotect;
	}
	// Guard the page at the end.
	if (mprotect((char *)addr + len, page_size, PROT_NONE) == -1) {
		errc = get_errc();
		goto error_mprotect;
	}

	return addr;

error_mprotect:
	munmap((char *)addr - page_size, len + 2 * page_size);
error_mmap:
	set_errc(errc);
	return NULL;
}

static void
guard_munmap(void *addr, size_t len)
{
	if (addr) {
		assert(page_size > 0);
		assert(powerof2(page_size));

		len = ALIGN(len, page_size);

		munmap((char *)addr - page_size, len + 2 * page_size);
	}
}

#endif // _POSIX_MAPPED_FILES && MAP_ANONYMOUS

#if !_WIN32

static inline void
jmpto(jmp_buf from, jmp_buf to)
{
	if (!setjmp(from))
		longjmp(to, 1);
}

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))
static inline void
sigjmpto(sigjmp_buf from, sigjmp_buf to, int savemask)
{
	if (!sigsetjmp(from, savemask))
		siglongjmp(to, 1);
}
#endif

#endif // !_WIN32

#if _WIN32
static _Noreturn void CALLBACK
#else
static _Noreturn void
#endif
fiber_start(void *arg)
{
	fiber_t *curr = arg;
	assert(curr);
	fiber_t *fiber = curr->from;
	assert(fiber);

	// Copy the function to be executed to the stack for later reference.
	fiber_func_t *func = curr->func;
	curr->func = NULL;
	arg = curr->arg;
	curr->arg = NULL;

	// Resume fiber_create().
	fiber = fiber_resume(fiber);

	// Execute the original function, if specified.
	if (func)
		fiber = func(fiber, arg);

	// The function has terminated, so resume the caller fiber immediately.
	for (;;)
		fiber = fiber_resume(fiber);
}

#endif // !LELY_NO_MALLOC
