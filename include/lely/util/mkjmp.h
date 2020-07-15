/**@file
 * This header file is part of the utilities library; it contains the mkjmp()
 * and sigmkjmp() function declarations.
 *
 * setjmp() and longjmp() provide support for nonlocal jumps between different
 * calling environments. Since these functions are defined by the C standard,
 * they are the most portable primitives that can be used for implementing
 * user-level context switches. Using mkjmp() (and sigmkjmp() on POSIX
 * platforms) it is possible to create new calling environments with a
 * user-provided stack. This allows setjmp() and longjmp() to be used as a
 * basis for fibers and stackfull coroutines.
 *
 * Although context switches using these functions are slower than dedicated
 * assembly implementations (like fcontext_t from
 * <a href="https://www.boost.org/doc/libs/release/libs/context/doc/html/index.html">Boost.Context</a>),
 * they are the most portable solution (and, in case of sigsetjmp() and
 * siglongjmp(), significantly faster than those based on the deprecated
 * swapcontext() function).
 *
 * The implementation of mkjmp() and sigmkjmp() requires changing the stack
 * pointer. This cannot be done in standard C, but for most platforms it can be
 * implemented with a single assembly instruction.
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

#ifndef LELY_UTIL_MKJMP_H_
#define LELY_UTIL_MKJMP_H_

#include <lely/features.h>

#include <setjmp.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates and stores a calling environment with a user-provided stack suitable
 * for use by `longjmp()`. On success, the new environment is about to invoke
 * `func(arg)`. Note that environments created by mkjmp() can only be used with
 * `longjmp()` in the same thread in which they were created.
 *
 * @param env  the variable in which to store the new environment.
 * @param func the address of the function to be invoked in the new environment.
 *             If this function returns, the thread on which it ran is
 *             terminated as if by `thrd_exit(0)` (or `exit(EXIT_SUCCESS)` if
 *             multithreading support is disabled).
 * @param arg  the argument provided to <b>func</b>.
 * @param sp   a pointer to the beginning of a memory region to be used as the
 *             stack for the new environment. Depending on the platform, the
 *             stack pointer in the new environment may point to either the
 *             beginning or the end of the memory region. It is the
 *             responsibility of the caller to ensure proper alignment.
 * @param size the size (in bytes) of the memory region at <b>sp</b>. It is the
 *             responsibility of the caller to ensure the stack size is
 *             sufficient.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained from `errno`.
 */
int mkjmp(jmp_buf env, void (*func)(void *), void *arg, void *sp, size_t size);

#if _POSIX_C_SOURCE >= 200112L && (!defined(__NEWLIB__) || defined(__CYGWIN__))

/**
 * Creates and stores a calling environment with a user-provided stack suitable
 * for use by `siglongjmp()`. On success, the new environment is about to invoke
 * `func(arg)`. Note that environments created by sigmkjmp() can only be used
 *  with `siglongjmp()` in the same thread in which they were created.
 *
 * @param env      the variable in which to store the new environment.
 * @param savemask if not 0, the current signal mask of the calling thread shall
 *                 be saved as part of the new environment.
 * @param func     the address of the function to be invoked in the new
 *                 environment. If this function returns, the thread on which it
 *                 ran is terminated as if by `thrd_exit(0)` (or
 *                 `exit(EXIT_SUCCESS)` if multithreading support is disabled).
 * @param arg      the argument provided to <b>func</b>.
 * @param sp       a pointer to the beginning of a memory region to be used as
 *                 the stack for the new environment. Depending on the platform,
 *                 the stack pointer in the new environment may point to either
 *                 the beginning or the end of the memory region. It is the
 *                 responsibility of the caller to ensure proper alignment.
 * @param size     the size (in bytes) of the memory region at <b>sp</b>. It is
 *                 the responsibility of the caller to ensure the stack size is
 *                 sufficient.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained from `errno`.
 */
int sigmkjmp(sigjmp_buf env, int savemask, void (*func)(void *), void *arg,
		void *sp, size_t size);

#endif // _POSIX_C_SOURCE >= 200112L && (!__NEWLIB__ || __CYGWIN__)

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_MKJMP_H_
