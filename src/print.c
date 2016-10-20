/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the printing functions.
 *
 * \see lely/util/print.h
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

#include "util.h"
#define LELY_UTIL_PRINT_INLINE	extern inline LELY_DLL_EXPORT
#include <lely/libc/stdint.h>
#include <lely/libc/stdio.h>
#include <lely/util/lex.h>
#include <lely/util/print.h>
#include "unicode.h"

#include <assert.h>
#include <float.h>
#if __STDC_NO_VLA__
#include <stdlib.h>
#endif
#include <string.h>

#ifdef __MINGW32__
#pragma GCC diagnostic ignored "-Wformat"
#endif

LELY_UTIL_EXPORT size_t
print_utf8(char32_t c32, char **pbegin, char *end)
{
	static const unsigned char mark[] = { 0x00, 0xc0, 0xe0, 0xf0 };

	// Fast path for ASCII characters.
	if (__likely(c32 <= 0x7f))
		return print_char(c32, pbegin, end);

	// Replace invalid characters by the replacement character (U+FFFD).
	if (__unlikely(!utf32_valid(c32)))
		c32 = 0xfffd;

	int n = c32 <= 0x07ff ? 1 : (c32 <= 0xffff ? 2 : 3);
	size_t chars = print_char(((c32 >> (n * 6)) & 0x3f) | mark[n], pbegin,
			end);
	while (n--)
		chars += print_char(((c32 >> (n * 6)) & 0x3f) | 0x80, pbegin,
				end);
	return chars;
}

LELY_UTIL_EXPORT size_t
print_c99_esc(char32_t c32, char **pbegin, char *end)
{
	size_t chars = 0;

	if (c32 < 0x80) {
		switch (c32) {
		case '\'':
			chars += print_char('\\', pbegin, end);
			chars += print_char('\'', pbegin, end);
			break;
		case '\"':
			chars += print_char('\\', pbegin, end);
			chars += print_char('\"', pbegin, end);
			break;
		case '\\':
			chars += print_char('\\', pbegin, end);
			chars += print_char('\\', pbegin, end);
			break;
		case '\a':
			chars += print_char('\\', pbegin, end);
			chars += print_char('a', pbegin, end);
			break;
		case '\b':
			chars += print_char('\\', pbegin, end);
			chars += print_char('b', pbegin, end);
			break;
		case '\f':
			chars += print_char('\\', pbegin, end);
			chars += print_char('f', pbegin, end);
			break;
		case '\n':
			chars += print_char('\\', pbegin, end);
			chars += print_char('n', pbegin, end);
			break;
		case '\r':
			chars += print_char('\\', pbegin, end);
			chars += print_char('r', pbegin, end);
			break;
		case '\t':
			chars += print_char('\\', pbegin, end);
			chars += print_char('t', pbegin, end);
			break;
		case '\v':
			chars += print_char('\\', pbegin, end);
			chars += print_char('v', pbegin, end);
			break;
		default:
			if (__likely(isprint(c32))) {
				chars += print_char(c32, pbegin, end);
			} else {
				// For non-printable characters, we use an octal
				// escape sequence.
				chars += print_char('\\', pbegin, end);
				if ((c32 >> 6) & 7)
					chars += print_char(otoc(c32 >> 6),
							pbegin, end);
				if ((c32 >> 3) & 7)
					chars += print_char(otoc(c32 >> 3),
							pbegin, end);
				chars += print_char(otoc(c32), pbegin, end);
			}
			break;
		}
	} else if (__likely(utf32_valid(c32))) {
		chars += print_utf8(c32, pbegin, end);
	} else {
		// For invalid Unicode code points, we use a hexadecimal escape
		// sequence.
		chars += print_char('\\', pbegin, end);
		chars += print_char('x', pbegin, end);
		// Compute the number of hex digits.
		int n = 1;
		while (c32 >> (4 * n))
			n++;
		// Print the hex digits.
		for (int i = 0; i < n; i++)
			chars += print_char(xtoc(c32 >> (4 * (n - i - 1))),
					pbegin, end);
	}

	return chars;
}

LELY_UTIL_EXPORT size_t
print_c99_str(const char *s, char **pbegin, char *end)
{
	assert(s);

	size_t chars = 0;
	while (*s) {
		char32_t c32 = 0;
		size_t n = lex_utf8(s, NULL, NULL, &c32);
		if (n) {
			s += n;
			chars += print_c99_esc(c32, pbegin, end);
		}
	}
	return chars;
}

#if __STDC_NO_VLA__
#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, format) \
	LELY_UTIL_EXPORT size_t \
	print_c99_##suffix(type name, char **pbegin, char *end) \
	{ \
		if (pbegin && *pbegin) { \
			char *buf = NULL; \
			int chars = asprintf(&buf, format, name); \
			if (__unlikely(chars < 0)) \
				return 0; \
			memcpy(*pbegin, buf, end ? MIN(end - *pbegin, chars) \
					: chars); \
			(*pbegin) += chars; \
			free(buf); \
			return chars; \
		} \
		return snprintf(NULL, 0, format, name); \
	}
#else
#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, format) \
	LELY_UTIL_EXPORT size_t \
	print_c99_##suffix(type name, char **pbegin, char *end) \
	{ \
		int chars = snprintf(NULL, 0, format, name); \
		assert(chars > 0); \
		if (pbegin && *pbegin) { \
			char buf[chars + 1]; \
			sprintf(buf, format, name); \
			memcpy(*pbegin, buf, end ? MIN(end - *pbegin, chars) \
					: chars); \
			(*pbegin) += chars; \
		} \
		return chars; \
	}
#endif

#ifdef _WIN32
LELY_UTIL_DEFINE_PRINT(long, long, l, "%I32i")
LELY_UTIL_DEFINE_PRINT(unsigned long, ulong, ul, "%I32u")
LELY_UTIL_DEFINE_PRINT(long long, llong, ll, "%I64i")
LELY_UTIL_DEFINE_PRINT(unsigned long long, ullong, ull, "%I64u")
#else
LELY_UTIL_DEFINE_PRINT(long, long, l, "%li")
LELY_UTIL_DEFINE_PRINT(unsigned long, ulong, ul, "%lu")
LELY_UTIL_DEFINE_PRINT(long long, llong, ll, "%lli")
LELY_UTIL_DEFINE_PRINT(unsigned long long, ullong, ull, "%llu")
#endif

#undef LELY_UTIL_DEFINE_PRINT

#if __STDC_NO_VLA__
#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, format, dig) \
	LELY_UTIL_EXPORT size_t \
	print_c99_##suffix(type name, char **pbegin, char *end) \
	{ \
		if (pbegin && *pbegin) { \
			char *buf = NULL; \
			int chars = asprintf(&buf, format, dig, name); \
			if (__unlikely(chars < 0)) \
				return 0; \
			memcpy(*pbegin, buf, end ? MIN(end - *pbegin, chars) \
					: chars); \
			(*pbegin) += chars; \
			free(buf); \
			return chars; \
		} \
		return snprintf(NULL, 0, format, dig, name); \
	}
#else
#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, format, dig) \
	LELY_UTIL_EXPORT size_t \
	print_c99_##suffix(type name, char **pbegin, char *end) \
	{ \
		int chars = snprintf(NULL, 0, format, dig, name); \
		assert(chars > 0); \
		if (pbegin && *pbegin) { \
			char buf[chars + 1]; \
			sprintf(buf, format, dig, name); \
			memcpy(*pbegin, buf, end ? MIN(end - *pbegin, chars) \
					: chars); \
			(*pbegin) += chars; \
		} \
		return chars; \
	}
#endif

LELY_UTIL_DEFINE_PRINT(float, flt, f, "%.*g", FLT_DIG)
LELY_UTIL_DEFINE_PRINT(double, dbl, d, "%.*g", DBL_DIG)
#ifndef _WIN32
LELY_UTIL_DEFINE_PRINT(long double, ldbl, ld, "%.*Lg", LDBL_DIG)
#endif

#undef LELY_UTIL_DEFINE_PRINT

#define LELY_UTIL_DEFINE_PRINT(type, suffix, name, alias) \
	LELY_UTIL_EXPORT size_t \
	print_c99_##suffix(type name, char **pbegin, char *end) \
	{ \
		return print_c99_##alias(name, pbegin, end); \
	}

LELY_UTIL_DEFINE_PRINT(int8_t, i8, i8, long)
LELY_UTIL_DEFINE_PRINT(int16_t, i16, i16, long)
LELY_UTIL_DEFINE_PRINT(int32_t, i32, i32, long)
#if LONG_BIT == 32
LELY_UTIL_DEFINE_PRINT(int64_t, i64, i64, llong)
#else
LELY_UTIL_DEFINE_PRINT(int64_t, i64, i64, long)
#endif
LELY_UTIL_DEFINE_PRINT(uint8_t, u8, u8, ulong)
LELY_UTIL_DEFINE_PRINT(uint16_t, u16, u16, ulong)
LELY_UTIL_DEFINE_PRINT(uint32_t, u32, u32, ulong)
#if LONG_BIT == 32
LELY_UTIL_DEFINE_PRINT(uint64_t, u64, u64, ullong)
#else
LELY_UTIL_DEFINE_PRINT(uint64_t, u64, u64, ulong)
#endif

#undef LELY_UTIL_DEFINE_PRINT

LELY_UTIL_EXPORT size_t
print_base64(const void *ptr, size_t n, char **pbegin, char *end)
{
	static const char tab[64] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
			"ghijklmnopqrstuvwxyz0123456789+/";

	size_t chars = 0;

	const uint8_t *bp = ptr;
	while (n) {
		char c = tab[(bp[0] >> 2) & 0x3f];
		chars += print_char(c, pbegin, end);
		if (!((chars + 2) % 78)) {
			chars += print_char('\r', pbegin, end);
			chars += print_char('\n', pbegin, end);
		}

		c = tab[((bp[0] << 4) + (--n ? (bp[1] >> 4) : 0)) & 0x3f];
		chars += print_char(c, pbegin, end);
		if (!((chars + 2) % 78)) {
			chars += print_char('\r', pbegin, end);
			chars += print_char('\n', pbegin, end);
		}

		c = n ? tab[((bp[1] << 2) + (--n ? (bp[2] >> 6) : 0)) & 0x3f] : '=';
		chars += print_char(c, pbegin, end);
		if (!((chars + 2) % 78)) {
			chars += print_char('\r', pbegin, end);
			chars += print_char('\n', pbegin, end);
		}

		c = n ? (--n, tab[bp[2] & 0x3f]) : '=';
		chars += print_char(c, pbegin, end);
		if (n && !((chars + 2) % 78)) {
			chars += print_char('\r', pbegin, end);
			chars += print_char('\n', pbegin, end);
		}

		bp += 3;
	}

	return chars;
}

