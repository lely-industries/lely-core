/*!\file
 * This is the public header file of the Test Anything Protocol (TAP) library.
 *
 * \copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_TAP_TAP_H_
#define LELY_TAP_TAP_H_

#include <lely/lely.h>

#ifndef LELY_TAP_EXTERN
#ifdef LELY_TAP_INTERN
#define LELY_TAP_EXTERN	LELY_DLL_EXPORT
#else
#define LELY_TAP_EXTERN	LELY_DLL_IMPORT
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
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_plan(...) \
	tap_plan_(__VA_ARGS__, "")
#define tap_plan_(n, ...) \
	__tap_plan_impl(n, "" __VA_ARGS__)
#else
#define tap_plan(n) \
	__tap_plan_impl(n, "")
#endif

/*!
 * Evaluates an expression. If the result is non-zero, the test passed,
 * otherwise it failed. The first argument MUST be the expression to be
 * evaluated. The second argument is optional. If it is specified, it is
 * interpreted as a printf-style format string describing the test. Any further
 * arguments are printed under the control of the format string.
 */
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_test(...) \
	tap_test_(__VA_ARGS__, "")
#define tap_test_(expr, ...) \
	__tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, "" __VA_ARGS__)
#else
#define tap_test(expr) \
	__tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, "")
#endif

/*!
 * Indicates that a test has passed. No arguments are required, but if they are
 * specified, the first argument is interpreted as a printf-style format string
 * describing the test. Any further arguments are printed under the control of
 * the format string.
 *
 * \see tap_fail()
 */
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_pass(...) \
	__tap_test_impl(1, "", __FILE__, __LINE__, "" __VA_ARGS__)
#else
#define tap_pass() \
	__tap_test_impl(1, "", __FILE__, __LINE__, "")
#endif

/*!
 * Indicates that a test has failed. No arguments are required, but if they are
 * specified, the first argument is interpreted as a printf-style format string
 * describing the test. Any further arguments are printed under the control of
 * the format string.
 *
 * \see tap_pass()
 */
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_fail(...) \
	__tap_test_impl(0, "", __FILE__, __LINE__, "" __VA_ARGS__)
#else
#define tap_fail() \
	__tap_test_impl(0, "", __FILE__, __LINE__, "")
#endif

/*!
 * Indicates that a test is expected to fail. If the expression evaluates to
 * zero, the test is not considered to have failed. The arguments are the same
 * as for tap_test().
 */
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_todo(...) \
	tap_todo_(__VA_ARGS__, "")
#define tap_todo_(expr, ...) \
	__tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, \
			" # TODO " __VA_ARGS__)
#else
#define tap_todo(expr) \
	__tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, " # TODO")
#endif

/*!
 * Skips a test. The provided expression is _not_ evaluated. The arguments are
 * the same as for tap_test().
 */
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_skip(...) \
	tap_skip_(__VA_ARGS__, "")
#define tap_skip_(expr, ...) \
	tap_pass(" # SKIP " __VA_ARGS__)
#else
#define tap_skip(expr) \
	tap_pass(" # SKIP")
#endif

/*!
 * Emits a diagnostic message. The first argument, if provided, is interpreted
 * as a printf-style format string. Any further arguments are printed under the
 * control of the format string.
 */
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_diag(...) \
	__tap_diag_impl("# " __VA_ARGS__)
#else
#define tap_diag(format) \
	__tap_diag_impl("# " format)
#endif

/*!
 * Aborts all tests. The first argument, if provided, is interpreted as a
 * printf-style format string describing the reason for aborting. Any further
 * arguments are printed under the control of the format string. Note that this
 * function aborts the running process and does not return.
 */
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_abort(...) \
	__tap_abort_impl("" __VA_ARGS__)
#else
#define tap_abort() \
	__tap_abort_impl("")
#endif

//! Similar to `assert()`, but invokes tap_abort() if \a expr evaluates to zero.
#if !defined(__cplusplus) || __cplusplus >= 201103L
#define tap_assert(expr) \
	((expr) ? (void)0 \
		: tap_abort("%s:%d: Assertion `%s' failed.", __FILE__, __LINE__, \
				#expr))
#else
#define tap_assert(expr) \
	tap_assert_(expr, __FILE__, __LINE__)
#define tap_assert_(expr, file, line) \
	tap_assert__(expr, file, line)
#define tap_assert__(expr, file, line) \
	((expr) ? (void)0 \
		: __tap_abort_impl(file ":" #line ": Assertion `" #expr "' failed."))
#endif

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

#endif // LELY_TAP_TAP_H_
