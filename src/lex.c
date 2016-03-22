/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the lexer functions.
 *
 * \see lely/util/lex.h
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
#include <lely/util/diag.h>
#include <lely/util/lex.h>
#include "unicode.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

LELY_UTIL_EXPORT size_t
lex_char(int c, const char *begin, const char *end, struct floc *at)
{
	assert(begin);
	assert(!end || end >= begin);

	if (__unlikely(end && begin >= end))
		return 0;

	if (*begin != c)
		return 0;

	if (at)
		floc_strninc(at, begin, 1);

	return 1;
}

LELY_UTIL_EXPORT size_t
lex_ctype(int (__cdecl *ctype)(int), const char *begin, const char *end,
		struct floc *at)
{
	assert(ctype);
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;

	while ((!end || cp < end) && ctype((unsigned char)*cp))
		cp++;

	if (at)
		floc_strninc(at, begin, cp - begin);

	return cp - begin;
}

LELY_UTIL_EXPORT size_t
lex_break(const char *begin, const char *end, struct floc *at)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;

	if (!isbreak(*cp))
		return 0;

	// Treat "\r\n" as a single line break.
	if (*cp++ == '\r' && (!end || cp < end) && *cp == '\n')
		cp++;

	if (at)
		floc_strninc(at, begin, cp - begin);

	return cp - begin;
}

LELY_UTIL_EXPORT size_t
lex_utf8(const char *begin, const char *end, struct floc *at, char32_t *pc32)
{
	assert(begin);
	assert(!end || end >= begin);

	static const unsigned char mask[] = {
		0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
		0x00, 0x00, 0x00, 0x00,
		0x1f, 0x1f,
		0x0f,
		0x07
	};

	const char *cp = begin;

	char32_t c32 = *cp & mask[(*cp >> 4) & 0xf];
	switch (utf8_bytes(cp++)) {
	case 4:
		if (__unlikely((end && cp >= end) || (*cp & 0xc0) != 0x80))
			goto error;
		c32 = (c32 << 6) | (*cp++ & 0x3f);
	case 3:
		if (__unlikely((end && cp >= end) || (*cp & 0xc0) != 0x80))
			goto error;
		c32 = (c32 << 6) | (*cp++ & 0x3f);
	case 2:
		if (__unlikely((end && cp >= end) || (*cp & 0xc0) != 0x80))
			goto error;
		c32 = (c32 << 6) | (*cp++ & 0x3f);
	case 1:
		break;
	default:
		// Skip all continuation bytes.
		while ((!end || cp < end) && (*cp & 0xc0) == 0x80)
			cp++;
		goto error;
	}

	if (__unlikely(!utf32_valid(c32))) {
		if (at)
			diag(DIAG_WARNING, 0, "illegal Unicode code point U+%X",
					c32);
		c32 = 0xfffd;
	}

done:
	if (pc32)
		*pc32 = c32;

	if (at)
		floc_strninc(at, begin, cp - begin);

	return cp - begin;

error:
	if (at)
		diag_at(DIAG_WARNING, 0, at, "invalid UTF-8 sequence");
	c32 = 0xfffd;
	goto done;
}

LELY_UTIL_EXPORT size_t
lex_c99_esc(const char *begin, const char *end, struct floc *at, char32_t *pc32)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;

	if (*cp++ != '\\')
		return lex_utf8(begin, end, at, pc32);

	// Treat a backslash at the end as '\\'.
	if (__unlikely(end && cp >= end))
		cp--;

	char32_t c32 = 0;
	if (isodigit(*cp)) {
		c32 = ctoo(*cp);
		cp++;
		while ((!end || cp < end) && isodigit(*cp)) {
			c32 = c32 * 8 + ctoo(*cp);
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
			while ((!end || cp < end) && isxdigit(
					(unsigned char)*cp)) {
				c32 = c32 * 16 + ctox(*cp);
				cp++;
			}
			break;
		default:
			cp--;
			if (at)
				diag_at(DIAG_ERROR, 0, at,
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

	if (at)
		floc_strninc(at, begin, cp - begin);

	return cp - begin;
}

LELY_UTIL_EXPORT size_t
lex_c99_str(const char *begin, const char *end, struct floc *at)
{
	assert(begin);
	assert(!end || end >= begin);

	struct floc *floc = NULL;
	struct floc floc_;
	if (at) {
		floc = &floc_;
		*floc = *at;
	}

	const char *cp = begin;

	size_t chars = lex_char('"', cp, end, floc);
	cp += chars;
	if (__unlikely(!chars))
		return 0;

	char32_t c32 = 0;
	while (c32 != '\"') {
		if (__unlikely(end && cp >= end))
			return 0;
		if (*cp == '\\')
			chars = lex_c99_esc(cp, end, floc, NULL);
		else
			chars = lex_utf8(cp, end, floc, &c32);
		cp += chars;
		if (__unlikely(!chars))
			return 0;
	}

	if (at)
		*at = *floc;

	return cp - begin;
}

LELY_UTIL_EXPORT size_t
lex_c99_pp_num(const char *begin, const char *end, struct floc *at)
{
	assert(begin);
	assert(!end || end >= begin);

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
			if (cp < end && (*cp == '+' || *cp == '-'))
				cp++;
		} else if (*cp == '.' || *cp == '_'
				|| isalnum((unsigned char)*cp)) {
			cp++;
		} else {
			break;
		}
	}

	if (at)
		floc_strninc(at, begin, cp - begin);

	return cp - begin;
}

#define LELY_UTIL_DEFINE_LEX(type, suffix, strtov, min, max, pname) \
	LELY_UTIL_EXPORT size_t \
	lex_c99_##suffix(const char *begin, const char *end, struct floc *at, \
			type *pname) \
	{ \
		size_t chars = lex_c99_pp_num(begin, end, NULL); \
		if (!chars) \
			return 0; \
	\
		char buf[chars + 1]; \
		memcpy(buf, begin, chars); \
		buf[chars] = '\0'; \
	\
		int errsv = errno; \
		errno = 0; \
	\
		char *endptr; \
		type result = strtov(buf, &endptr); \
		chars = endptr - buf; \
	\
		if (__unlikely(min && errno == ERANGE && result == min)) { \
			set_errnum(ERRNUM_RANGE); \
			if (at) \
				diag_at(DIAG_WARNING, get_errc(), at, #type " underflow"); \
		} else if (__unlikely(errno == ERANGE && result == max)) { \
			set_errnum(ERRNUM_RANGE); \
			if (at) \
				diag_at(DIAG_WARNING, get_errc(), at, #type " overflow"); \
		} else if (!errno) { \
			errno = errsv; \
		} \
	\
		if (pname) \
			*pname = result; \
	\
		if (at) \
			floc_strninc(at, begin, chars); \
	\
		return chars; \
	}
#define strtov(nptr, endptr)	strtol(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX(long, long, strtov, LONG_MIN, LONG_MAX, pl)
#undef strtov
#define strtov(nptr, endptr)	strtol(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX(unsigned long, ulong, strtov, 0, ULONG_MAX, pul)
#undef strtov
#define strtov(nptr, endptr)	strtoll(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX(long long, llong, strtov, LLONG_MIN, LLONG_MAX, pll)
#undef strtov
#define strtov(nptr, endptr)	strtoll(nptr, endptr, 0)
LELY_UTIL_DEFINE_LEX(unsigned long long, ullong, strtov, 0, ULLONG_MAX, pull)
#undef strtov
LELY_UTIL_DEFINE_LEX(float, flt, strtof, -HUGE_VALF, HUGE_VALF, pf)
LELY_UTIL_DEFINE_LEX(double, dbl, strtod, -HUGE_VAL, HUGE_VAL, pd)
LELY_UTIL_DEFINE_LEX(long double, ldbl, strtold, -HUGE_VALL, HUGE_VALL, pld)
#undef LELY_UTIL_DEFINE_LEX

LELY_UTIL_EXPORT size_t
lex_line_comment(const char *delim, const char *begin, const char *end,
		struct floc *at)
{
	assert(begin);
	assert(!end || end >= begin);

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

	if (at)
		floc_strninc(at, begin, cp - begin);

	return cp - begin;
}

