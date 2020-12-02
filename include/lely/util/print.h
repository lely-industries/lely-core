/**@file
 * This header file is part of the utilities library; it contains the printing
 * function declarations.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_PRINT_H_
#define LELY_UTIL_PRINT_H_

#include <lely/libc/uchar.h>
#include <lely/util/util.h>

#include <stdarg.h>
#include <stddef.h>

#ifndef LELY_UTIL_PRINT_INLINE
#define LELY_UTIL_PRINT_INLINE static inline
#endif

#ifndef LENll
/// A cross-platform "ll" length modifier macro for format specifiers.
#if _WIN32
#define LENll "I64"
#ifdef __MINGW32__
// Ignore complaints that "I64" is not a valid ISO C length modifier.
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif
#else
#define LENll "ll"
#endif
#endif

#ifndef LENj
/// A cross-platform "j" length modifier macro for format specifiers.
#if _WIN32
#define LENj LENll
#else
#define LENj "j"
#endif
#endif

#ifndef LENz
/// A cross-platform "z" length modifier macro for format specifiers.
#if _WIN32
#if __WORDSIZE == 64
#define LENz LENll
#else
#define LENz LENl
#endif
#else
#define LENz "z"
#endif
#endif

#ifndef LENt
/// A cross-platform "t" length modifier macro for format specifiers.
#if _WIN32
#if __WORDSIZE == 64
#define LENz LENll
#else
#define LENz LENl
#endif
#else
#define LENt "t"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Returns the character corresponding to the octal digit <b>i</b>. @see ctoo()
LELY_UTIL_PRINT_INLINE int otoc(int i);

/**
 * Returns the character corresponding to the hexadecimal digit <b>i</b>.
 *
 * @see ctox()
 */
LELY_UTIL_PRINT_INLINE int xtoc(int i);

/**
 * Prints a formatted string to a memory buffer. Note that the output is _not_
 * null-terminated.
 *
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin`
 *               characters are written.
 * @param format a printf-style format string.
 * @param ...    an optional list of arguments to be printed according to
 *               <b>format</b>.
 *
 * @returns the number of characters that would have been written had the buffer
 * been sufficiently large, or 0 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
size_t print_fmt(char **pbegin, char *end, const char *format, ...)
		format_printf__(3, 4);

/**
 * Prints a formatted string to a memory buffer. This function is equivalent to
 * #print_fmt(), except that it accepts a `va_list` instead of a variable number
 * of arguments.
 */
size_t vprint_fmt(char **pbegin, char *end, const char *format, va_list ap)
		format_printf__(3, 0);

/**
 * Prints a single character to a memory buffer. Note that the output is _not_
 * null-terminated.
 *
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin`
 *               characters are written.
 * @param c      the character to be written.
 *
 * @returns 1.
 */
LELY_UTIL_PRINT_INLINE size_t print_char(char **pbegin, char *end, int c);

/**
 * Prints a UTF-8 encoded Unicode character to a memory buffer. Illegal Unicode
 * code points are silently replaced by the Unicode replacement character
 * (U+FFFD). Note that the output is _not_ null-terminated.
 *
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin` characters are
 *               written, and the output may be truncated.
 * @param c32    the Unicode character to be written.
 *
 * @returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 *
 * @see print_c99_esc()
 */
size_t print_utf8(char **pbegin, char *end, char32_t c32);

/**
 * Prints a UTF-8 encoded Unicode character to a memory buffer. Non-printable
 * ASCII characters are printed using C99 escape sequences, illegal Unicode code
 * points using hexadecimal escape sequences. Note that the output is _not_
 * null-terminated.
 *
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin` characters are
 *               written, and the output may be truncated.
 * @param c32    the Unicode character to be written.
 *
 * @returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 *
 * @see print_utf8()
 */
size_t print_c99_esc(char **pbegin, char *end, char32_t c32);

/**
 * Prints a UTF-8 encoded Unicode string to a memory buffer. Non-printable ASCII
 * characters are printed using C99 escape sequences, illegal Unicode code
 * points using hexadecimal escape sequences. Note that the output is _not_
 * delimited by double quotes and _not_ null-terminated.
 *
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin` characters are
 *               written, and the output may be truncated.
 * @param s      a pointer to the string to be written.
 * @param n      the number of characters at <b>s</b>.
 *
 * @returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 *
 * @see print_c99_esc()
 */
size_t print_c99_str(char **pbegin, char *end, const char *s, size_t n);

// clang-format off
#define LELY_UTIL_DEFINE_PRINT(type, suffix, name) \
	/** Prints a C99 `type` to a memory buffer. Note that the output is
	_not_ null-terminated.
	@param pbegin the address of a pointer to the start of the buffer. If
	              <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is
	              written; Otherwise, on exit, *<b>pbegin</b> points to one
	              past the last character written.
	@param end    a pointer to one past the last character in the buffer. If
	              <b>end</b> is not NULL, at most `end - *pbegin` characters
	              are written, and the output may be truncated.
	@param name   the value to be written.
	@returns the number of characters that would have been written had the
	buffer been sufficiently large, or 0 on error. In the latter case, the
	error number can be obtained with get_errc().*/ \
	size_t print_c99_##suffix( \
			char **pbegin, char *end, type name);
// clang-format on

LELY_UTIL_DEFINE_PRINT(long, long, l)
LELY_UTIL_DEFINE_PRINT(unsigned long, ulong, ul)
LELY_UTIL_DEFINE_PRINT(long long, llong, ll)
LELY_UTIL_DEFINE_PRINT(unsigned long long, ullong, ul)
LELY_UTIL_DEFINE_PRINT(float, flt, f)
LELY_UTIL_DEFINE_PRINT(double, dbl, d)
LELY_UTIL_DEFINE_PRINT(long double, ldbl, ld)

LELY_UTIL_DEFINE_PRINT(int_least8_t, i8, i8)
LELY_UTIL_DEFINE_PRINT(int_least16_t, i16, i16)
LELY_UTIL_DEFINE_PRINT(int_least32_t, i32, i32)
LELY_UTIL_DEFINE_PRINT(int_least64_t, i64, i64)
LELY_UTIL_DEFINE_PRINT(uint_least8_t, u8, u8)
LELY_UTIL_DEFINE_PRINT(uint_least16_t, u16, u16)
LELY_UTIL_DEFINE_PRINT(uint_least32_t, u32, u32)
LELY_UTIL_DEFINE_PRINT(uint_least64_t, u64, u64)

#undef LELY_UTIL_DEFINE_PRINT

/**
 * Prints the Base64 representation of binary data to a memory buffer. This
 * function implements the MIME variant of Base64 as specified in
 * <a href="https://tools.ietf.org/html/rfc2045">RFC 2045</a>. Note that the
 * output is _not_ null-terminated.
 *
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin` characters are
 *               written, and the output may be truncated.
 * @param ptr    a pointer to the binary data to be encoded and written.
 * @param n      the number of bytes at <b>ptr</b>.
 *
 * @returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 */
size_t print_base64(char **pbegin, char *end, const void *ptr, size_t n);

LELY_UTIL_PRINT_INLINE int
otoc(int i)
{
	return '0' + (i & 7);
}

LELY_UTIL_PRINT_INLINE int
xtoc(int i)
{
	i &= 0xf;
	if (i < 10)
		return '0' + i;
#if __STDC_ISO_10646__ && !__STDC_MB_MIGHT_NEQ_WC__
	return 'a' + i - 10;
#else
	switch (i) {
	case 0xa: return 'a';
	case 0xb: return 'b';
	case 0xc: return 'c';
	case 0xd: return 'd';
	case 0xe: return 'e';
	case 0xf: return 'f';
	default: return -1;
	}
#endif
}

LELY_UTIL_PRINT_INLINE size_t
print_char(char **pbegin, char *end, int c)
{
	if (pbegin && *pbegin && (!end || *pbegin < end))
		*(*pbegin)++ = (unsigned char)c;
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_PRINT_H_
