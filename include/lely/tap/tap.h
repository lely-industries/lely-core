/**@file
 * This is the public header file of the Test Anything Protocol (TAP) library.
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_TAP_TAP_H_
#define LELY_TAP_TAP_H_

#include <lely/features.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif

/**
 * Specifies the test plan. The first argument MUST be the number of test to
 * run. If no tests are run, a second argument MAY be provided. If it is, it is
 * interpreted as a printf-style format string describing the reason for
 * skipping the tests. Any further arguments are printed under the control of
 * the format string.
 */
#define tap_plan(...) tap_plan_(__VA_ARGS__, "")
#define tap_plan_(n, ...) tap_plan_impl(n, "" __VA_ARGS__)

/**
 * Evaluates an expression. If the result is non-zero, the test passed,
 * otherwise it failed. The first argument MUST be the expression to be
 * evaluated. The second argument is optional. If it is specified, it is
 * interpreted as a printf-style format string describing the test. Any further
 * arguments are printed under the control of the format string.
 */
#define tap_test(...) tap_test_(__VA_ARGS__, "")
#define tap_test_(expr, ...) \
	tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, "" __VA_ARGS__)

/**
 * Indicates that a test has passed. No arguments are required, but if they are
 * specified, the first argument is interpreted as a printf-style format string
 * describing the test. Any further arguments are printed under the control of
 * the format string.
 *
 * @see tap_fail()
 */
#define tap_pass(...) tap_test_impl(1, "", __FILE__, __LINE__, "" __VA_ARGS__)

/**
 * Indicates that a test has failed. No arguments are required, but if they are
 * specified, the first argument is interpreted as a printf-style format string
 * describing the test. Any further arguments are printed under the control of
 * the format string.
 *
 * @see tap_pass()
 */
#define tap_fail(...) tap_test_impl(0, "", __FILE__, __LINE__, "" __VA_ARGS__)

/**
 * Indicates that a test is expected to fail. If the expression evaluates to
 * zero, the test is not considered to have failed. The arguments are the same
 * as for tap_test().
 */
#define tap_todo(...) tap_todo_(__VA_ARGS__, "")
#define tap_todo_(expr, ...) \
	tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, \
			" # TODO " __VA_ARGS__)

/**
 * Skips a test. The provided expression is _not_ evaluated. The arguments are
 * the same as for tap_test().
 */
#define tap_skip(...) tap_skip_(__VA_ARGS__, "")
#define tap_skip_(expr, ...) tap_pass(" # SKIP " __VA_ARGS__)

/**
 * Emits a diagnostic message. The first argument, if provided, is interpreted
 * as a printf-style format string. Any further arguments are printed under the
 * control of the format string.
 */
#define tap_diag(...) tap_diag_impl("# " __VA_ARGS__)

/**
 * Aborts all tests. The first argument, if provided, is interpreted as a
 * printf-style format string describing the reason for aborting. Any further
 * arguments are printed under the control of the format string. Note that this
 * function aborts the running process and does not return.
 */
#define tap_abort(...) tap_abort_impl("" __VA_ARGS__)

/**
 * Similar to `assert()`, but invokes tap_abort() if <b>expr</b> evaluates to
 * zero.
 */
// clang-format off
#define tap_assert(expr) \
	((expr) ? (void)0 \
		: tap_abort("%s:%d: Assertion `%s' failed.", __FILE__, \
				__LINE__, #expr))
// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

void tap_plan_impl(int n, const char *format, ...) format_printf__(2, 3);
int tap_test_impl(int test, const char *expr, const char *file, int line,
		const char *format, ...) format_printf__(5, 6);
void tap_diag_impl(const char *format, ...) format_printf__(1, 2);
_Noreturn void tap_abort_impl(const char *format, ...) format_printf__(1, 2);

#ifdef __cplusplus
}
#endif

#endif // !LELY_TAP_TAP_H_
