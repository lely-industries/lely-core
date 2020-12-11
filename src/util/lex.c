/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the lexer functions.
 *
 * @see lely/util/lex.h
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#define LELY_UTIL_LEX_INLINE extern inline
#include <lely/libc/string.h>
#include <lely/libc/uchar.h>
#include <lely/util/diag.h>
#include <lely/util/lex.h>
#include <lely/util/print.h>

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>

size_t
lex_char(int c, const char *begin, const char *end, struct floc *at)
{
	assert(begin);

	const char *cp = begin;

	if ((end && cp >= end) || *cp++ != c)
		return 0;

	return floc_lex(at, begin, begin + 1);
}

size_t
lex_ctype(int (*ctype)(int), const char *begin, const char *end,
		struct floc *at)
{
	assert(ctype);
	assert(begin);

	const char *cp = begin;

	while ((!end || cp < end) && ctype((unsigned char)*cp))
		cp++;

	return floc_lex(at, begin, cp);
}

size_t
lex_break(const char *begin, const char *end, struct floc *at)
{
	assert(begin);

	const char *cp = begin;

	if ((end && cp >= end) || !isbreak((unsigned char)*cp))
		return 0;

	// Treat "\r\n" as a single line break.
	if (*cp++ == '\r' && (!end || cp < end) && *cp == '\n')
		cp++;

	return floc_lex(at, begin, cp);
}

size_t
lex_utf8(const char *begin, const char *end, struct floc *at, char32_t *pc32)
{
	assert(begin);

	const char *cp = begin;

	if (end && cp >= end)
		return 0;

	char32_t c32;

	int n;
	unsigned char c = *cp++;
	if (!(c & 0x80)) {
		// 0xxxxxxx is an ASCII character.
		c32 = c & 0x7f;
		n = 0;
	} else if ((c & 0xc0) == 0x80) {
		// 10xxxxxx is a continuation byte. It cannot appear at the
		// beginning of a valid UTF-8 sequence. We skip all subsequent
		// continuation bytes to prevent multiple errors for the same
		// sequence.
		while ((!end || cp < end)
				&& ((unsigned char)*cp & 0xc0) == 0x80)
			cp++;
		diag_if(DIAG_WARNING, 0, at,
				"a UTF-8 sequence cannot begin with a continuation byte");
		goto error;
	} else if ((c & 0xe0) == 0xc0) {
		// 110xxxxx is the first byte in a two-byte sequence.
		c32 = c & 0x1f;
		n = 1;
	} else if ((c & 0xf0) == 0xe0) {
		// 1110xxxx is the first byte in a three-byte sequence.
		c32 = c & 0x0f;
		n = 2;
	} else if ((c & 0xf8) == 0xf0) {
		// 11110xxx is the first byte in a four-byte sequence.
		c32 = c & 0x07;
		n = 3;
	} else {
		// Five- and six-byte sequences have been deprecated since 2003.
		diag_if(DIAG_WARNING, 0, at, "invalid UTF-8 byte");
		goto error;
	}

	// Lex the continuation bytes.
	while (n--) {
		if ((end && cp > end) || ((unsigned char)*cp & 0xc0) != 0x80)
			goto error;
		c32 = (c32 << 6) | ((unsigned char)*cp & 0x3f);
	}

	// Valid Unicode code points fall in the range between U+0000 and
	// U+10FFFF, with the exception of U+D800 to U+DFFF, which are reserved
	// for UTF-16 encoding of high and low surrogates, respectively.
	if ((c32 >= 0xd800 && c32 <= 0xdfff) || c32 > 0x10ffff) {
		diag_if(DIAG_WARNING, 0, at,
				"illegal Unicode code point U+%" PRIX32, c32);
		goto error;
	}

done:
	if (pc32)
		*pc32 = c32;

	return floc_lex(at, begin, cp);

error:
	// Replace an invalid code point by the Unicode replacement character
	// (U+FFFD).
	c32 = 0xfffd;
	goto done;
}

size_t
lex_c99_id(const char *begin, const char *end, struct floc *at, char *s,
		size_t *pn)
{
	assert(begin);

	const char *cp = begin;

	if ((end && cp >= end) || !(*cp == '_' || isalpha((unsigned char)*cp)))
		return 0;
	cp++;

	while ((!end || cp < end)
			&& (*cp == '_' || isalnum((unsigned char)*cp)))
		cp++;

	if (pn) {
		if (s)
			memcpy(s, begin, MIN((size_t)(cp - begin), *pn));
		*pn = cp - begin;
	}

	return floc_lex(at, begin, cp);
}

size_t
lex_c99_esc(const char *begin, const char *end, struct floc *at, char32_t *pc32)
{
	assert(begin);

	const char *cp = begin;

	if (end && cp >= end)
		return 0;

	if (*cp++ != '\\')
		return lex_utf8(begin, end, at, pc32);

	// Treat a backslash at the end as '\\'.
	if (end && cp >= end)
		cp--;

	char32_t c32 = 0;
	if (isodigit(*cp)) {
		c32 = ctoo(*cp);
		cp++;
		while ((!end || cp < end) && isodigit((unsigned char)*cp)) {
			c32 = (c32 << 3) | ctoo(*cp);
			cp++;
		}
	} else {
		switch (*cp++) {
		case '\'': c32 = '\''; break;
		case '\"': c32 = '\"'; break;
		case '\\': c32 = '\\'; break;
		case 'a': c32 = '\a'; break;
		case 'b': c32 = '\b'; break;
		case 'f': c32 = '\f'; break;
		case 'n': c32 = '\n'; break;
		case 'r': c32 = '\r'; break;
		case 't': c32 = '\t'; break;
		case 'v': c32 = '\v'; break;
		case 'x':
			while ((!end || cp < end)
					&& isxdigit((unsigned char)*cp)) {
				c32 = (c32 << 4) | ctox(*cp);
				cp++;
			}
			break;
		default:
			cp--;
			diag_if(DIAG_WARNING, 0, at,
					isgraph((unsigned char)*cp)
							? "illegal escape sequence '\\%c'"
							: "illegal escape sequence '\\\\%o'",
					*cp);
			// Treat an invalid escape sequence as '\\'.
			c32 = '\\';
			break;
		}
	}

	if (pc32)
		*pc32 = c32;

	return floc_lex(at, begin, cp);
}

size_t
lex_c99_str(const char *begin, const char *end, struct floc *at, char *s,
		size_t *pn)
{
	assert(begin);

	struct floc *floc = NULL;
	struct floc floc_;
	if (at) {
		floc = &floc_;
		*floc = *at;
	}

	const char *cp = begin;

	size_t n = 0;
	char *ends = s + (s && pn ? *pn : 0);

	while ((!end || cp < end) && *cp && *cp != '\"'
			&& !isbreak((unsigned char)*cp)) {
		char32_t c32 = 0;
		if (*cp == '\\')
			cp += lex_c99_esc(cp, end, floc, &c32);
		else
			cp += lex_utf8(cp, end, floc, &c32);
		if (s || pn)
			n += print_utf8(&s, ends, c32);
	}

	if (pn)
		*pn = n;

	if (at)
		*at = *floc;

	return cp - begin;
}

size_t
lex_c99_pp_num(const char *begin, const char *end, struct floc *at)
{
	assert(begin);

	const char *cp = begin;

	// Parse the optional sign.
	if ((!end || cp < end) && (*cp == '+' || *cp == '-'))
		cp++;

	// Any number has to begin with either a digit, or a period followed by
	// a digit.
	if ((!end || cp < end) && isdigit((unsigned char)*cp)) {
		cp++;
	} else if ((!end || end - cp >= 2) && cp[0] == '.'
			&& isdigit((unsigned char)cp[1])) {
		cp += 2;
	} else {
		return 0;
	}

	while ((!end || cp < end) && *cp) {
		if (*cp == 'e' || *cp == 'E' || *cp == 'p' || *cp == 'P') {
			cp++;
			// Exponents may contain a sign.
			if ((!end || cp < end) && (*cp == '+' || *cp == '-'))
				cp++;
		} else if (*cp == '.' || *cp == '_'
				|| isalnum((unsigned char)*cp)) {
			cp++;
		} else {
			break;
		}
	}

	return floc_lex(at, begin, cp);
}

#define LELY_UTIL_DEFINE_LEX_SIGNED(type, suffix, strtov, min, max, pname) \
	size_t lex_c99_##suffix(const char *begin, const char *end, \
			struct floc *at, type *pname) \
	{ \
		size_t chars = lex_c99_pp_num(begin, end, NULL); \
		if (!chars) \
			return 0; \
\
		char *buf = strndup(begin, chars); \
		if (!buf) { \
			diag_if(DIAG_ERROR, errno2c(errno), at, \
					"unable to duplicate string"); \
			return 0; \
		} \
\
		int errsv = errno; \
		errno = 0; \
\
		char *endptr; \
		type result = strtov(buf, &endptr); \
		chars = endptr - buf; \
\
		free(buf); \
\
		if (errno == ERANGE && result == min) { \
			set_errnum(ERRNUM_RANGE); \
			diag_if(DIAG_WARNING, get_errc(), at, \
					#type " underflow"); \
		} else if (errno == ERANGE && result == max) { \
			set_errnum(ERRNUM_RANGE); \
			diag_if(DIAG_WARNING, get_errc(), at, \
					#type " overflow"); \
		} else if (!errno) { \
			errno = errsv; \
		} \
\
		if (pname) \
			*pname = result; \
\
		return floc_lex(at, begin, begin + chars); \
	}

#define LELY_UTIL_DEFINE_LEX_UNSIGNED(type, suffix, strtov, max, pname) \
	size_t lex_c99_##suffix(const char *begin, const char *end, \
			struct floc *at, type *pname) \
	{ \
		size_t chars = lex_c99_pp_num(begin, end, NULL); \
		if (!chars) \
			return 0; \
\
		char *buf = strndup(begin, chars); \
		if (!buf) { \
			diag_if(DIAG_ERROR, errno2c(errno), at, \
					"unable to duplicate string"); \
			return 0; \
		} \
\
		int errsv = errno; \
		errno = 0; \
\
		char *endptr; \
		type result = strtov(buf, &endptr); \
		chars = endptr - buf; \
\
		free(buf); \
\
		if (errno == ERANGE && result == max) { \
			set_errnum(ERRNUM_RANGE); \
			diag_if(DIAG_WARNING, get_errc(), at, \
					#type " overflow"); \
		} else if (!errno) { \
			errno = errsv; \
		} \
\
		if (pname) \
			*pname = result; \
\
		return floc_lex(at, begin, begin + chars); \
	}

#define strtov(nptr, endptr) strtol(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX_SIGNED(long, long, strtov, LONG_MIN, LONG_MAX, pl)
#undef strtov
#define strtov(nptr, endptr) strtoul(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX_UNSIGNED(unsigned long, ulong, strtov, ULONG_MAX, pul)
#undef strtov
#define strtov(nptr, endptr) strtoll(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX_SIGNED(long long, llong, strtov, LLONG_MIN, LLONG_MAX, pll)
#undef strtov
#define strtov(nptr, endptr) strtoull(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX_UNSIGNED(
		unsigned long long, ullong, strtov, ULLONG_MAX, pull)
#undef strtov
LELY_UTIL_DEFINE_LEX_SIGNED(float, flt, strtof, -HUGE_VALF, HUGE_VALF, pf)
LELY_UTIL_DEFINE_LEX_SIGNED(double, dbl, strtod, -HUGE_VAL, HUGE_VAL, pd)
LELY_UTIL_DEFINE_LEX_SIGNED(
		long double, ldbl, strtold, -HUGE_VALL, HUGE_VALL, pld)

#undef LELY_UTIL_DEFINE_LEX_UNSIGNED
#undef LELY_UTIL_DEFINE_LEX_SIGNED

size_t
lex_c99_i8(const char *begin, const char *end, struct floc *at,
		int_least8_t *pi8)
{
	long i8;
	size_t chars = lex_c99_long(begin, end, at, &i8);
	if (chars) {
		if (i8 < INT8_MIN) {
			i8 = INT8_MIN;
			set_errnum(ERRNUM_RANGE);
			diag_if(DIAG_WARNING, get_errc(), at,
					"int8_t underflow");
		} else if (i8 > INT8_MAX) {
			i8 = INT8_MAX;
			set_errnum(ERRNUM_RANGE);
			diag_if(DIAG_WARNING, get_errc(), at,
					"int8_t overflow");
		}
		if (pi8)
			*pi8 = (int_least8_t)i8;
	}
	return chars;
}

size_t
lex_c99_i16(const char *begin, const char *end, struct floc *at,
		int_least16_t *pi16)
{
	long i16;
	size_t chars = lex_c99_long(begin, end, at, &i16);
	if (chars) {
		if (i16 < INT16_MIN) {
			i16 = INT16_MIN;
			set_errnum(ERRNUM_RANGE);
			diag_if(DIAG_WARNING, get_errc(), at,
					"int16_t underflow");
		} else if (i16 > INT16_MAX) {
			i16 = INT16_MAX;
			set_errnum(ERRNUM_RANGE);
			diag_if(DIAG_WARNING, get_errc(), at,
					"int16_t overflow");
		}
		if (pi16)
			*pi16 = (int_least16_t)i16;
	}
	return chars;
}

size_t
lex_c99_i32(const char *begin, const char *end, struct floc *at,
		int_least32_t *pi32)
{
	long i32;
	size_t chars = lex_c99_long(begin, end, at, &i32);
	if (chars) {
#if LONG_BIT == 32
		if (get_errnum() == ERRNUM_RANGE && i32 == LONG_MIN) {
#else
		if (i32 < INT32_MIN) {
			i32 = INT32_MIN;
			set_errnum(ERRNUM_RANGE);
#endif
			diag_if(DIAG_WARNING, get_errc(), at,
					"int32_t underflow");
#if LONG_BIT == 32
		} else if (get_errnum() == ERRNUM_RANGE && i32 == LONG_MAX) {
#else
		} else if (i32 > INT32_MAX) {
			i32 = INT32_MAX;
			set_errnum(ERRNUM_RANGE);
#endif
			diag_if(DIAG_WARNING, get_errc(), at,
					"int32_t overflow");
		}
		if (pi32)
			*pi32 = (int_least32_t)i32;
	}
	return chars;
}

size_t
lex_c99_i64(const char *begin, const char *end, struct floc *at,
		int_least64_t *pi64)
{
#if LONG_BIT == 64
	long i64;
	size_t chars = lex_c99_long(begin, end, at, &i64);
#else
	long long i64;
	size_t chars = lex_c99_llong(begin, end, at, &i64);
#endif
	if (chars) {
#if LONG_BIT == 64
		if (get_errnum() == ERRNUM_RANGE && i64 == LONG_MIN) {
#else
		if ((get_errnum() == ERRNUM_RANGE && i64 == LLONG_MIN)
				|| i64 < INT64_MIN) {
			i64 = INT64_MIN;
			set_errnum(ERRNUM_RANGE);
#endif
			diag_if(DIAG_WARNING, get_errc(), at,
					"int64_t underflow");
#if LONG_BIT == 64
		} else if (get_errnum() == ERRNUM_RANGE && i64 == LONG_MAX) {
#else
		} else if ((get_errnum() == ERRNUM_RANGE && i64 == LLONG_MAX)
				|| i64 > INT64_MAX) {
			i64 = INT64_MAX;
			set_errnum(ERRNUM_RANGE);
#endif
			diag_if(DIAG_WARNING, get_errc(), at,
					"int64_t overflow");
		}
		if (pi64)
			*pi64 = (int_least64_t)i64;
	}
	return chars;
}

size_t
lex_c99_u8(const char *begin, const char *end, struct floc *at,
		uint_least8_t *pu8)
{
	unsigned long u8;
	size_t chars = lex_c99_ulong(begin, end, at, &u8);
	if (chars) {
		if (u8 > UINT8_MAX) {
			u8 = UINT8_MAX;
			set_errnum(ERRNUM_RANGE);
			diag_if(DIAG_WARNING, get_errc(), at,
					"uint8_t overflow");
		}
		if (pu8)
			*pu8 = (uint_least8_t)u8;
	}
	return chars;
}

size_t
lex_c99_u16(const char *begin, const char *end, struct floc *at,
		uint_least16_t *pu16)
{
	unsigned long u16;
	size_t chars = lex_c99_ulong(begin, end, at, &u16);
	if (chars) {
		if (u16 > UINT16_MAX) {
			u16 = UINT16_MAX;
			set_errnum(ERRNUM_RANGE);
			diag_if(DIAG_WARNING, get_errc(), at,
					"uint16_t overflow");
		}
		if (pu16)
			*pu16 = (uint_least16_t)u16;
	}
	return chars;
}

size_t
lex_c99_u32(const char *begin, const char *end, struct floc *at,
		uint_least32_t *pu32)
{
	unsigned long u32;
	size_t chars = lex_c99_ulong(begin, end, at, &u32);
	if (chars) {
#if LONG_BIT == 32
		if (get_errnum() == ERRNUM_RANGE && u32 == ULONG_MAX) {
#else
		if (u32 > UINT32_MAX) {
			u32 = UINT32_MAX;
			set_errnum(ERRNUM_RANGE);
#endif
			diag_if(DIAG_WARNING, get_errc(), at,
					"uint32_t overflow");
		}
		if (pu32)
			*pu32 = (uint_least32_t)u32;
	}
	return chars;
}

size_t
lex_c99_u64(const char *begin, const char *end, struct floc *at,
		uint_least64_t *pu64)
{
#if LONG_BIT == 64
	unsigned long u64;
	size_t chars = lex_c99_ulong(begin, end, at, &u64);
#else
	unsigned long long u64;
	size_t chars = lex_c99_ullong(begin, end, at, &u64);
#endif
	if (chars) {
#if LONG_BIT == 64
		if (get_errnum() == ERRNUM_RANGE && u64 == ULONG_MAX) {
#else
		if ((get_errnum() == ERRNUM_RANGE && u64 == ULONG_MAX)
				|| u64 > UINT64_MAX) {
			u64 = UINT64_MAX;
			set_errnum(ERRNUM_RANGE);
#endif
			diag_if(DIAG_WARNING, get_errc(), at,
					"uint64_t overflow");
		}
		if (pu64)
			*pu64 = (uint_least64_t)u64;
	}
	return chars;
}

size_t
lex_line_comment(const char *delim, const char *begin, const char *end,
		struct floc *at)
{
	assert(begin);

	const char *cp = begin;

	if (delim && *delim) {
		size_t n = strlen(delim);
		if ((end && cp + n > end) || strncmp(delim, cp, n))
			return 0;
		cp += n;
	}

	// Skip until end-of-line.
	while ((!end || cp < end) && *cp && !isbreak((unsigned char)*cp))
		cp++;

	return floc_lex(at, begin, cp);
}

size_t
lex_hex(const char *begin, const char *end, struct floc *at, void *ptr,
		size_t *pn)
{
	assert(begin);

	const char *cp = begin;

	unsigned char *bp = ptr;
	unsigned char *endb = bp + (ptr && pn ? *pn : 0);

	size_t n = 0, i = 0;
	for (; (!end || cp < end) && isxdigit((unsigned char)*cp); cp++, i++) {
		if (bp && bp < endb) {
			if (i % 2) {
				*bp <<= 4;
				*bp++ |= ctox(*cp) & 0xf;
			} else {
				*bp = ctox(*cp) & 0xf;
				n++;
			}
		}
	}

	if (pn)
		*pn = n;

	return floc_lex(at, begin, cp);
}

size_t
lex_base64(const char *begin, const char *end, struct floc *at, void *ptr,
		size_t *pn)
{
	assert(begin);

	const char *cp = begin;

	unsigned char *bp = ptr;
	unsigned char *endb = bp + (ptr && pn ? *pn : 0);

	size_t n = 0, i = 0;
	unsigned char s = 0;
	while ((!end || cp < end) && *cp) {
		unsigned char b;
		switch (*cp++) {
		case 'A': b = 0; break;
		case 'B': b = 1; break;
		case 'C': b = 2; break;
		case 'D': b = 3; break;
		case 'E': b = 4; break;
		case 'F': b = 5; break;
		case 'G': b = 6; break;
		case 'H': b = 7; break;
		case 'I': b = 8; break;
		case 'J': b = 9; break;
		case 'K': b = 10; break;
		case 'L': b = 11; break;
		case 'M': b = 12; break;
		case 'N': b = 13; break;
		case 'O': b = 14; break;
		case 'P': b = 15; break;
		case 'Q': b = 16; break;
		case 'R': b = 17; break;
		case 'S': b = 18; break;
		case 'T': b = 19; break;
		case 'U': b = 20; break;
		case 'V': b = 21; break;
		case 'W': b = 22; break;
		case 'X': b = 23; break;
		case 'Y': b = 24; break;
		case 'Z': b = 25; break;
		case 'a': b = 26; break;
		case 'b': b = 27; break;
		case 'c': b = 28; break;
		case 'd': b = 29; break;
		case 'e': b = 30; break;
		case 'f': b = 31; break;
		case 'g': b = 32; break;
		case 'h': b = 33; break;
		case 'i': b = 34; break;
		case 'j': b = 35; break;
		case 'k': b = 36; break;
		case 'l': b = 37; break;
		case 'm': b = 38; break;
		case 'n': b = 39; break;
		case 'o': b = 40; break;
		case 'p': b = 41; break;
		case 'q': b = 42; break;
		case 'r': b = 43; break;
		case 's': b = 44; break;
		case 't': b = 45; break;
		case 'u': b = 46; break;
		case 'v': b = 47; break;
		case 'w': b = 48; break;
		case 'x': b = 49; break;
		case 'y': b = 50; break;
		case 'z': b = 51; break;
		case '0': b = 52; break;
		case '1': b = 53; break;
		case '2': b = 54; break;
		case '3': b = 55; break;
		case '4': b = 56; break;
		case '5': b = 57; break;
		case '6': b = 58; break;
		case '7': b = 59; break;
		case '8': b = 60; break;
		case '9': b = 61; break;
		case '+': b = 62; break;
		case '/': b = 63; break;
		default: continue;
		}
		if (bp && bp < endb) {
			switch (i % 4) {
			case 0: s = b << 2; break;
			case 1:
				s |= b >> 4;
				*bp++ = s;
				s = b << 4;
				break;
			case 2:
				s |= b >> 2;
				*bp++ = s;
				s = b << 6;
				break;
			case 3:
				s |= b;
				*bp++ = s;
				s = 0;
				break;
			}
		}
		if (i++ % 4)
			n++;
	}

	if (pn)
		*pn = n;

	return floc_lex(at, begin, cp);
}

#endif // !LELY_NO_STDIO
