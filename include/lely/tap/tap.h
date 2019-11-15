/**@file
 * This is the public header file of the Test Anything Protocol (TAP) library.
 *
 * @copyright 2013-2019 Lely Industries N.V.
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
#endif

/// Evaluates to 1 if the argument list contains a comma, and 0 if not.
#ifdef __cplusplus
#define TAP_ARGS_HAS_COMMA(...) \
	TAP_ARGS_AT_127(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 0)
#define TAP_ARGS_AT_127(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
		_13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, \
		_25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, \
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, \
		_49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
		_61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, \
		_73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, \
		_85, _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, \
		_97, _98, _99, _100, _101, _102, _103, _104, _105, _106, _107, \
		_108, _109, _110, _111, _112, _113, _114, _115, _116, _117, \
		_118, _119, _120, _121, _122, _123, _124, _125, _126, ...) \
	TAP_ARGS_AT_127_(__VA_ARGS__, )
#define TAP_ARGS_AT_127_(_127, ...) _127
#else
#define TAP_ARGS_HAS_COMMA(...) \
	TAP_ARGS_AT_63(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define TAP_ARGS_AT_63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
		_13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, \
		_25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, \
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, \
		_49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
		_61, _62, ...) \
	TAP_ARGS_AT_63_(__VA_ARGS__, )
#define TAP_ARGS_AT_63_(_63, ...) _63
#endif // __cplusplus

/// Appends <b>arg</b> if the variadic argument list contains a single argument.
#define TAP_ARGS_DEFAULT(arg, ...) \
	TAP_ARGS_DEFAULT_(TAP_ARGS_HAS_COMMA(__VA_ARGS__), arg, __VA_ARGS__)
#define TAP_ARGS_DEFAULT_(n, ...) TAP_ARGS_DEFAULT__(n, __VA_ARGS__)
#define TAP_ARGS_DEFAULT__(n, ...) TAP_ARGS_DEFAULT_##n(__VA_ARGS__)
#define TAP_ARGS_DEFAULT_0(arg, ...) __VA_ARGS__, arg
#define TAP_ARGS_DEFAULT_1(arg, ...) __VA_ARGS__

/**
 * Specifies the test plan. The first argument MUST be the number of test to
 * run. If no tests are run, a second argument MAY be provided. If it is, it is
 * interpreted as a printf-style format string describing the reason for
 * skipping the tests. Any further arguments are printed under the control of
 * the format string.
 */
#define tap_plan(...) tap_plan_(TAP_ARGS_DEFAULT("", __VA_ARGS__))
#define tap_plan_(...) tap_plan__(__VA_ARGS__)
#define tap_plan__(n, ...) tap_plan_impl(n, "" __VA_ARGS__)

/**
 * Evaluates an expression. If the result is non-zero, the test passed,
 * otherwise it failed. The first argument MUST be the expression to be
 * evaluated. The second argument is optional. If it is specified, it is
 * interpreted as a printf-style format string describing the test. Any further
 * arguments are printed under the control of the format string.
 */
#define tap_test(...) tap_test_(TAP_ARGS_DEFAULT("", __VA_ARGS__))
#define tap_test_(...) tap_test__(__VA_ARGS__)
#define tap_test__(expr, ...) \
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
#define tap_todo(...) tap_todo_(TAP_ARGS_DEFAULT("", __VA_ARGS__))
#define tap_todo_(...) tap_todo__(__VA_ARGS__)
#define tap_todo__(expr, ...) \
	tap_test_impl(!!(expr), #expr, __FILE__, __LINE__, \
			" # TODO " __VA_ARGS__)

/**
 * Skips a test. The provided expression is _not_ evaluated. The arguments are
 * the same as for tap_test().
 */
#define tap_skip(...) tap_skip_(TAP_ARGS_DEFAULT("", __VA_ARGS__))
#define tap_skip_(...) tap_skip__(__VA_ARGS__)
#define tap_skip__(expr, ...) tap_pass(" # SKIP " __VA_ARGS__)

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
