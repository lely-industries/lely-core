/**@file
 * This file is part of the Test Anything Protocol (TAP) library; it contains
 * the implementation of the TAP functions.
 *
 * @see lely/tap/tap.h
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

#include "tap.h"
#if _WIN32
#include <lely/libc/stdio.h>
#endif
#include <lely/tap/tap.h>

#include <assert.h>
#include <stdarg.h>
#if !_WIN32
#include <stdio.h>
#endif
#include <stdlib.h>

#ifdef __MINGW32__
#include <fcntl.h>
#include <io.h>
#endif

static int tap_num;

static void tap_vprintf(const char *format, va_list ap);

void
tap_plan_impl(int n, const char *format, ...)
{
	if (tap_num)
		return;

#ifdef __MINGW32__
	// Set the stdout translation mode to binary to prevent printf() from
	// changing '\n' to '\r\n', as this trips up tap-driver.sh.
	fflush(stdout);
	_setmode(1, _O_BINARY);
#endif

	printf("1..%d", n);
	if (!n) {
		printf(" # SKIP");
		if (format && *format) {
			fputc(' ', stdout);
			va_list ap;
			va_start(ap, format);
			tap_vprintf(format, ap);
			va_end(ap);
		}
	}
	fputc('\n', stdout);

#ifdef __MINGW32__
	// Revert back to text mode, since the problem only occurs when parsing
	// the test plan.
	fflush(stdout);
	_setmode(1, _O_TEXT);
#endif
}

int
tap_test_impl(int test, const char *expr, const char *file, int line,
		const char *format, ...)
{
	assert(expr);
	assert(file);
	assert(format);

	printf(test ? "ok %d" : "not ok %d", ++tap_num);
	if (*format) {
		fputc(' ', stdout);
		va_list ap;
		va_start(ap, format);
		tap_vprintf(format, ap);
		va_end(ap);
	}
	fputc('\n', stdout);

	if (!test && *expr)
		printf("# %s:%d: Test `%s' failed.\n", file, line, expr);

	return test;
}

void
tap_diag_impl(const char *format, ...)
{
	assert(format);

	va_list ap;
	va_start(ap, format);
	tap_vprintf(format, ap);
	va_end(ap);
	fputc('\n', stdout);
}

_Noreturn void
tap_abort_impl(const char *format, ...)
{
	assert(format);

	printf("Bail out!");
	if (*format) {
		fputc(' ', stdout);
		va_list ap;
		va_start(ap, format);
		tap_vprintf(format, ap);
		va_end(ap);
	}
	fputc('\n', stdout);

	exit(EXIT_FAILURE);
}

static void
tap_vprintf(const char *format, va_list ap)
{
	assert(format);

	char *s = NULL;
	int n = vasprintf(&s, format, ap);
	if (n < 0)
		return;

	for (char *cp = s; cp < s + n; cp++) {
		switch (*cp) {
		case '\r':
			if (cp + 1 < s + n && cp[1] == '\n')
				cp++;
		// ... falls through ...
		case '\n':
			fputc('\n', stdout);
			fputc('#', stdout);
			fputc(' ', stdout);
			break;
		default: fputc(*cp, stdout);
		}
	}

	free(s);
}
