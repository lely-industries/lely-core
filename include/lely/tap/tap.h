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
#ifdef DLL_EXPORT
#ifdef LELY_TAP_INTERN
#define LELY_TAP_EXTERN	extern __dllexport
#else
#define LELY_TAP_EXTERN	extern __dllimport
#endif
#else
#define LELY_TAP_EXTERN	extern
#endif
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif

#define tap_plan(...) \
	_tap_plan(__VA_ARGS__, "")
#define _tap_plan(n, ...) \
	__tap_plan(n, "" __VA_ARGS__)

#define tap_test(...) \
	_tap_test(__VA_ARGS__, "")
#define _tap_test(expr, ...) \
	__tap_test(!!(expr), #expr, __FILE__, __LINE__, "" __VA_ARGS__)

#define tap_pass(...) \
	__tap_test(1, "", __FILE__, __LINE__, "" __VA_ARGS__)

#define tap_fail(...) \
	__tap_test(0, "", __FILE__, __LINE__, "" __VA_ARGS__)

#define tap_todo(...) \
	_tap_todo(__VA_ARGS__, "")
#define _tap_todo(expr, ...) \
	__tap_test(!!(expr), #expr, __FILE__, __LINE__, " # TODO " __VA_ARGS__)

#define tap_skip(...) \
	_tap_skip(__VA_ARGS__, "")
#define _tap_skip(expr, ...) \
	tap_pass(" # SKIP " __VA_ARGS__)

#define tap_abort(...) \
	__tap_abort("" __VA_ARGS__)

#define tap_assert(expr) \
	((expr) ? (void)0 \
		: tap_abort("%s:%d: Assertion `%s' failed.", __FILE__, __LINE__, \
				#expr))

#ifdef __cplusplus
extern "C" {
#endif

LELY_TAP_EXTERN void __tap_plan(int n, const char *format, ...)
		__format_printf(2, 3);
LELY_TAP_EXTERN int __tap_test(int test, const char *expr, const char *file,
		int line, const char *format, ...) __format_printf(5, 6);
LELY_TAP_EXTERN _Noreturn void __tap_abort(const char *format, ...)
		__format_printf(1, 2);

#ifdef __cplusplus
}
#endif

#endif

