/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the printing functions.
 *
 * @see lely/util/print.h
 *
 * @copyright 2017-2019 Lely Industries N.V.
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

#include "util.h"

#if !LELY_NO_STDIO

#define LELY_UTIL_PRINT_INLINE extern inline
#include <lely/libc/stdio.h>
#include <lely/libc/uchar.h>
#include <lely/util/lex.h>
#include <lely/util/print.h>

#include <assert.h>
#include <float.h>
#include <stdint.h>
#if __STDC_NO_VLA__
#include <stdlib.h>
#endif
#include <string.h>

size_t
print_fmt(char **pbegin, char *end, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	size_t chars = vprint_fmt(pbegin, end, format, ap);
	va_end(ap);
	return chars;
}

size_t
vprint_fmt(char **pbegin, char *end, const char *format, va_list ap)
{
#if __STDC_NO_VLA__
	if (pbegin && *pbegin && (!end || *pbegin < end)) {
		char *buf = NULL;
		va_list aq;
		va_copy(aq, ap);
		int chars = vasprintf(&buf, format, aq);
		va_end(aq);
		if (chars < 0)
			return 0;
		memcpy(*pbegin, buf, end ? MIN(end - *pbegin, chars) : chars);
		(*pbegin) += chars;
		free(buf);
		return chars;
	}
	return vsnprintf(NULL, 0, format, ap);
#else
	va_list aq;
	va_copy(aq, ap);
	int chars = vsnprintf(NULL, 0, format, aq);
	va_end(aq);
	assert(chars > 0);
	if (pbegin && *pbegin && (!end || *pbegin < end)) {
		char buf[chars + 1];
		vsprintf(buf, format, ap);
		memcpy(*pbegin, buf, end ? MIN(end - *pbegin, chars) : chars);
		(*pbegin) += chars;
	}
	return chars;
#endif
}

size_t
print_utf8(char **pbegin, char *end, char32_t c32)
{
	static const unsigned char mark[] = { 0x00, 0xc0, 0xe0, 0xf0 };

	// Fast path for ASCII characters.
	if (c32 <= 0x7f)
		return print_char(pbegin, end, c32);

	// Replace invalid characters by the replacement character (U+FFFD).
	if ((c32 >= 0xd800 && c32 <= 0xdfff) || c32 > 0x10ffff)
		c32 = 0xfffd;

	int n = c32 <= 0x07ff ? 1 : (c32 <= 0xffff ? 2 : 3);
	size_t chars = print_char(
			pbegin, end, ((c32 >> (n * 6)) & 0x3f) | mark[n]);
	while (n--)
		chars += print_char(
				pbegin, end, ((c32 >> (n * 6)) & 0x3f) | 0x80);
	return chars;
}

size_t
print_c99_esc(char **pbegin, char *end, char32_t c32)
{
	size_t chars = 0;

	if (c32 < 0x80) {
		switch (c32) {
		case '\'':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, '\'');
			break;
		case '\"':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, '\"');
			break;
		case '\\':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, '\\');
			break;
		case '\a':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, 'a');
			break;
		case '\b':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, 'b');
			break;
		case '\f':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, 'f');
			break;
		case '\n':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, 'n');
			break;
		case '\r':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, 'r');
			break;
		case '\t':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, 't');
			break;
		case '\v':
			chars += print_char(pbegin, end, '\\');
			chars += print_char(pbegin, end, 'v');
			break;
		default:
			if (isprint(c32)) {
				chars += print_char(pbegin, end, c32);
			} else {
				// For non-printable characters, we use an octal
				// escape sequence.
				chars += print_char(pbegin, end, '\\');
				if ((c32 >> 6) & 7)
					chars += print_char(pbegin, end,
							otoc(c32 >> 6));
				if ((c32 >> 3) & 7)
					chars += print_char(pbegin, end,
							otoc(c32 >> 3));
				chars += print_char(pbegin, end, otoc(c32));
			}
			break;
		}
	} else if ((c32 < 0xd800 || c32 > 0xdfff) && c32 <= 0x10ffff) {
		chars += print_utf8(pbegin, end, c32);
	} else {
		// For invalid Unicode code points, we use a hexadecimal escape
		// sequence.
		chars += print_char(pbegin, end, '\\');
		chars += print_char(pbegin, end, 'x');
		// Compute the number of hex digits.
		int n = 1;
		while (c32 >> (4 * n))
			n++;
		// Print the hex digits.
		for (int i = 0; i < n; i++)
			chars += print_char(pbegin, end,
					xtoc(c32 >> (4 * (n - i - 1))));
	}

	return chars;
}

size_t
print_c99_str(char **pbegin, char *end, const char *s, size_t n)
{
	assert(s);

	// cppcheck-suppress nullPointerArithmeticRedundantCheck
	const char *ends = s + (s ? n : 0);

	size_t chars = 0;
	while (s < ends) {
		char32_t c32 = 0;
		s += lex_utf8(s, ends, NULL, &c32);
		chars += print_c99_esc(pbegin, end, c32);
	}
	return chars;
}

#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, format) \
	size_t print_c99_##suffix(char **pbegin, char *end, type name) \
	{ \
		return print_fmt(pbegin, end, format, name); \
	}

LELY_UTIL_DEFINE_PRINT(long, long, l, "%li")
LELY_UTIL_DEFINE_PRINT(unsigned long, ulong, ul, "%lu")
LELY_UTIL_DEFINE_PRINT(long long, llong, ll, "%" LENll "i")
LELY_UTIL_DEFINE_PRINT(unsigned long long, ullong, ull, "%" LENll "u")

#undef LELY_UTIL_DEFINE_PRINT

#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, format, dig) \
	size_t print_c99_##suffix(char **pbegin, char *end, type name) \
	{ \
		return print_fmt(pbegin, end, format, dig, name); \
	}

LELY_UTIL_DEFINE_PRINT(float, flt, f, "%.*g", FLT_DIG)
LELY_UTIL_DEFINE_PRINT(double, dbl, d, "%.*g", DBL_DIG)
#ifndef _WIN32
LELY_UTIL_DEFINE_PRINT(long double, ldbl, ld, "%.*Lg", LDBL_DIG)
#endif

#undef LELY_UTIL_DEFINE_PRINT

#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, alias) \
	size_t print_c99_##suffix(char **pbegin, char *end, type name) \
	{ \
		return print_c99_##alias(pbegin, end, name); \
	}

LELY_UTIL_DEFINE_PRINT(int_least8_t, i8, i8, long)
LELY_UTIL_DEFINE_PRINT(int_least16_t, i16, i16, long)
LELY_UTIL_DEFINE_PRINT(int_least32_t, i32, i32, long)
#if LONG_BIT == 32
LELY_UTIL_DEFINE_PRINT(int_least64_t, i64, i64, llong)
#else
LELY_UTIL_DEFINE_PRINT(int_least64_t, i64, i64, long)
#endif
LELY_UTIL_DEFINE_PRINT(uint_least8_t, u8, u8, ulong)
LELY_UTIL_DEFINE_PRINT(uint_least16_t, u16, u16, ulong)
LELY_UTIL_DEFINE_PRINT(uint_least32_t, u32, u32, ulong)
#if LONG_BIT == 32
LELY_UTIL_DEFINE_PRINT(uint_least64_t, u64, u64, ullong)
#else
LELY_UTIL_DEFINE_PRINT(uint_least64_t, u64, u64, ulong)
#endif

#undef LELY_UTIL_DEFINE_PRINT

size_t
print_base64(char **pbegin, char *end, const void *ptr, size_t n)
{
	// clang-format off
	static const char tab[64] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
			"ghijklmnopqrstuvwxyz0123456789+/";
	// clang-format on

	size_t chars = 0;

	const unsigned char *bp = ptr;
	while (n) {
		char c = tab[(bp[0] >> 2) & 0x3f];
		chars += print_char(pbegin, end, c);
		if (!((chars + 2) % 78)) {
			chars += print_char(pbegin, end, '\r');
			chars += print_char(pbegin, end, '\n');
		}

		c = tab[((bp[0] << 4) + (--n ? (bp[1] >> 4) : 0)) & 0x3f];
		chars += print_char(pbegin, end, c);
		if (!((chars + 2) % 78)) {
			chars += print_char(pbegin, end, '\r');
			chars += print_char(pbegin, end, '\n');
		}

		// clang-format off
		c = n ? tab[((bp[1] << 2) + (--n ? (bp[2] >> 6) : 0)) & 0x3f]
				: '=';
		// clang-format on
		chars += print_char(pbegin, end, c);
		if (!((chars + 2) % 78)) {
			chars += print_char(pbegin, end, '\r');
			chars += print_char(pbegin, end, '\n');
		}

		c = n ? (--n, tab[bp[2] & 0x3f]) : '=';
		chars += print_char(pbegin, end, c);
		if (n && !((chars + 2) % 78)) {
			chars += print_char(pbegin, end, '\r');
			chars += print_char(pbegin, end, '\n');
		}

		bp += 3;
	}

	return chars;
}

#endif // !LELY_NO_STDIO
