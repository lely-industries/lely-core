/*!\file
 * This is the public header file of the Test Anything Protocol (TAP) library.
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

#ifndef LELY_TAP_TAP_H
#define LELY_TAP_TAP_H

#include <lely/libc/libc.h>

#ifndef LELY_TAP_EXTERN
#ifdef LELY_TAP_INTERN
#define LELY_TAP_EXTERN	extern LELY_DLL_EXPORT
#else
#define LELY_TAP_EXTERN	extern LELY_DLL_IMPORT
#endif
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif

/*!
 * Specifies the test plan. The first argument MUST be the number of test to
 * run. If no tests are run, a second argument MAY be provided. If it is, it is
 * interpreted as a printf-style format string describing the reason for
 * skipping the tests. Any further arguments are printed under the control of
 * the format string.
 */
#define tap_plan(...) \
	_tap_plan(__VA_ARGS__, "")
#define _tap_plan(n, ...) \
	__tap_plan_impl(n, "" __VA_ARGS__)

/*!
 * Evaluates an expression. If the result is non-zero, the test passed,
 * otherwise it failed. The first argument MUST be the expression to be
 * evaluated. The second argument is optional. If it is specified, it is
 * interpreted as a printf-style format string describing the test. Any further
 * arguments are printed under the control of the format string.
 */
#define tap_test(...) \
	_tap_test(__VA_ARGS__, "")
#define _tap_test(expr, ...) \
	__tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, "" __VA_ARGS__)

/*!
 * Indicates that a test has passed. No arguments are required, but if they are
 * specified, the first argument is interpreted as a printf-style format string
 * describing the test. Any further arguments are printed under the control of
 * the format string.
 *
 * \see tap_fail()
 */
#define tap_pass(...) \
	__tap_test_impl(1, "", __FILE__, __LINE__, "" __VA_ARGS__)

/*!
 * Indicates that a test has failed. No arguments are required, but if they are
 * specified, the first argument is interpreted as a printf-style format string
 * describing the test. Any further arguments are printed under the control of
 * the format string.
 *
 * \see tap_pass()
 */
#define tap_fail(...) \
	__tap_test_impl(0, "", __FILE__, __LINE__, "" __VA_ARGS__)

/*!
 * Indicates that a test is expected to fail. If the expression evaluates to
 * zero, the test is not considered to have failed. The arguments are the same
 * as for tap_test().
 */
#define tap_todo(...) \
	_tap_todo(__VA_ARGS__, "")
#define _tap_todo(expr, ...) \
	__tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, \
			" # TODO " __VA_ARGS__)

/*!
 * Skips a test. The provided expression is _not_ evaluated. The arguments are
 * the same as for tap_test().
 */
#define tap_skip(...) \
	_tap_skip(__VA_ARGS__, "")
#define _tap_skip(expr, ...) \
	tap_pass(" # SKIP " __VA_ARGS__)

/*!
 * Emits a diagnostic message. The first argument, if provided, is interpreted
 * as a printf-style format string. Any further arguments are printed under the
 * control of the format string.
 */
#define tap_diag(...) \
	__tap_diag_impl("# " __VA_ARGS__)

/*!
 * Aborts all tests. The first argument, if provided, is interpreted as a
 * printf-style format string describing the reason for aborting. Any further
 * arguments are printed under the control of the format string. Note that this
 * function aborts the running process and does not return.
 */
#define tap_abort(...) \
	__tap_abort_impl("" __VA_ARGS__)

//! Similar to `assert()`, but invokes tap_abort() if \a expr evaluates to zero.
#define tap_assert(expr) \
	((expr) ? (void)0 \
		: tap_abort("%s:%d: Assertion `%s' failed.", __FILE__, __LINE__, \
				#expr))

#ifdef __cplusplus
extern "C" {
#endif

LELY_TAP_EXTERN void __tap_plan_impl(int n, const char *format, ...)
		__format_printf(2, 3);
LELY_TAP_EXTERN int __tap_test_impl(int test, const char *expr,
		const char *file, int line, const char *format, ...)
		__format_printf(5, 6);
LELY_TAP_EXTERN void __tap_diag_impl(const char *format, ...)
		__format_printf(1, 2);
LELY_TAP_EXTERN _Noreturn void __tap_abort_impl(const char *format, ...)
		__format_printf(1, 2);

#ifdef __cplusplus
}
#endif

#endif

