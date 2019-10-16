/**@file
 * This file is part of the utilities library; it contains the implementation of
 * mkjmp() and sigmkjmp().
 *
 * @see lely/util/mkjmp.h
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

#include "util.h"
#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/util/mkjmp.h>

#include <stdlib.h>

#if defined(__clang__) || defined(__GNUC__)
#if defined(__arm__) || defined(__aarch64__)
#define MKJMP_SET_SP(sp, size) \
	__asm__("mov     sp, %0" ::"r"((char *)(sp) + (size)))
#elif defined(__i386__)
#define MKJMP_SET_SP(sp, size) \
	__asm__("movl    %0, %%esp" ::"r"((char *)(sp) + (size)))
#elif defined(__x86_64__)
#define MKJMP_SET_SP(sp, size) \
	__asm__("movq    %0, %%rsp" ::"r"((char *)(sp) + (size)))
#endif
#elif defined(_MSC_VER)
#if defined(_M_AMD64)
#define MKJMP_SET_SP(sp, size) \
	void *_sp = (char *)(sp) + (size); \
	__asm mov rsp, _sp
#elif defined(_M_IX86)
#define MKJMP_SET_SP(sp, size) \
	void *_sp = (char *)(sp) + (size); \
	__asm mov esp, _sp
#endif
#endif

#ifndef MKJMP_SET_SP
#error Unsupported compiler or architecture for mkjmp()/sigmkjmp().
#endif

#if LELY_NO_THREADS
static struct {
#else
static thread_local struct {
#endif
	void *penv;
	void (*func)(void *);
	void *arg;
	jmp_buf env;
} ctx;

static _Noreturn void ctx_init(void *sp, size_t size);
static _Noreturn void ctx_func(void);

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))

#if LELY_NO_THREADS
static struct sigctx {
#else
static thread_local struct {
#endif
	void *penv;
	int savemask;
	void (*func)(void *);
	void *arg;
	sigjmp_buf env;
} sigctx;

static _Noreturn void sigctx_init(void *sp, size_t size);
static _Noreturn void sigctx_func(void);

#endif // _POSIX_C_SOURCE >= 200112L && (!__NEWLIB__ || __CYGWIN__)

int
mkjmp(jmp_buf env, void (*func)(void *), void *arg, void *sp, size_t size)
{
	ctx.penv = env;
	ctx.func = func;
	ctx.arg = arg;

	if (!setjmp(ctx.env))
		ctx_init(sp, size);

	return 0;
}

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))

int
sigmkjmp(sigjmp_buf env, int savemask, void (*func)(void *), void *arg,
		void *sp, size_t size)
{
	sigctx.penv = env;
	sigctx.savemask = savemask;
	sigctx.func = func;
	sigctx.arg = arg;

	if (!sigsetjmp(sigctx.env, 0))
		sigctx_init(sp, size);

	return 0;
}

#endif // _POSIX_C_SOURCE >= 200112L && (!__NEWLIB__ || __CYGWIN__)

static _Noreturn void
ctx_init(void *sp, size_t size)
{
	(void)size; // This argument may be unused on some platforms.

	// Set the stack pointer.
	MKJMP_SET_SP(sp, size);

	// Setup a proper stack by jumping into a function.
	ctx_func();
}

static _Noreturn void
ctx_func(void)
{
	// Store the function to be invoked on the (new) stack. The static
	// variables may be changed before this function is resumed.
	void (*func)(void *) = ctx.func;
	void *arg = ctx.arg;

	// Save the current environment, on succes, and jump back to mkjmp().
	if (!setjmp(ctx.penv))
		longjmp(ctx.env, 1);

	// The environment has been restored by longjmp(). Invoke the
	// user-provided function on the current (new) stack.
	func(arg);

	// The user-provided function returned. Since we do not have an
	// environment to restore, terminate the current thread.
#if !LELY_NO_THREADS
	thrd_exit(0);
#else
	exit(EXIT_SUCCESS);
#endif
	for (;;)
		;
}

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))

static _Noreturn void
sigctx_init(void *sp, size_t size)
{
	(void)size; // This argument may be unused on some platforms.

	// Set the stack pointer.
	MKJMP_SET_SP(sp, size);

	// Setup a proper stack by jumping into a function.
	sigctx_func();
}

static _Noreturn void
sigctx_func(void)
{
	// Store the function to be invoked on the (new) stack. The static
	// variables may be changed before this function is resumed.
	void (*func)(void *) = sigctx.func;
	void *arg = sigctx.arg;

	// Save the current environment, on succes, and jump back to sigmkjmp().
	if (!sigsetjmp(sigctx.penv, sigctx.savemask))
		siglongjmp(sigctx.env, 1);

	// The environment has been restored by siglongjmp(). Invoke the
	// user-provided function on the current (new) stack.
	func(arg);

	// The user-provided function returned. Since we do not have an
	// environment to restore, terminate the current thread.
#if !LELY_NO_THREADS
	thrd_exit(0);
#else
	exit(EXIT_SUCCESS);
#endif
	for (;;)
		;
}

#endif // _POSIX_C_SOURCE >= 200112L && (!__NEWLIB__ || __CYGWIN__)
