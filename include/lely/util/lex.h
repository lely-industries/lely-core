/*!\file
 * This header file is part of the utilities library; it contains the lexer
 * function declarations.
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

#ifndef LELY_UTIL_LEX_H
#define LELY_UTIL_LEX_H

#include <lely/libc/stdint.h>
#include <lely/libc/uchar.h>
#include <lely/util/util.h>

#include <ctype.h>

// The file location struct from <lely/util/diag.h>.
struct floc;

#ifdef __cplusplus
extern "C" {
#endif

//! Returns 1 if \a c is a line break character, and 0 otherwise.
static inline int __cdecl isbreak(int c);

//! Returns 1 if \a c is an octal digit, and 0 otherwise.
static inline int __cdecl isodigit(int c);

//! Returns the octal digit corresponding to the character \a c. \see otoc()
static inline int ctoo(int c);

/*!
 * Returns the hexadecimal digit corresponding to the character \a c.
 *
 * \see xtoc()
 */
static inline int ctox(int c);

/*!
 * Lexes the specified character from a memory buffer.
 *
 * \param c     the character to match.
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 *
 * \returns the number of characters read (at most one).
 */
LELY_UTIL_EXTERN size_t lex_char(int c, const char *begin, const char *end,
		struct floc *at);

/*!
 * Greedily lexes a sequence of characters of the specified class from a memory
 * buffer.
 *
 * \param ctype a pointer to a function returning a non-zero value if its
 *              argument is part of the character class.
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 *
 * \returns the number of characters read.
 */
LELY_UTIL_EXTERN size_t lex_ctype(int (__cdecl *ctype)(int),
		const char *begin, const char *end, struct floc *at);

/*!
 * Lexes a single line break from a memory buffer.
 *
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 *
 * \returns the number of characters read (at most two).
 */
LELY_UTIL_EXTERN size_t lex_break(const char *begin, const char *end,
		struct floc *at);

/*!
 * Lexes a UTF-8 encoded Unicode character from a memory buffer. Illegal
 * Unicode code points are replaced with the replacement character (U+FFFD).
 *
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 * \param pc32  the address at which to store the Unicode character. On success,
 *              if \a pc32 is not NULL, *\a pc32 contains the UTF-32 encoded
 *              character. On error, *\a pc32 is left untouched.
 *
 * \returns the number of characters read (at least 1 if `begin < end`).
 *
 * \see lex_c99_esc()
 */
LELY_UTIL_EXTERN size_t lex_utf8(const char *begin, const char *end,
		struct floc *at, char32_t *pc32);

/*!
 * Lexes a C99 character escape sequence from a memory buffer if the buffer
 * begins with '\', and a UTF-8 encoded Unicode character if not. If the escape
 * sequence is invalid, the initial '\' is returned as is.
 *
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 * \param pc32  the address at which to store the escape sequence or Unicode
 *              character. On success, if \a pc32 is not NULL, *\a pc32 contains
 *              the UTF-32 encoded character. On error, *\a pc32 is left
 *              untouched.
 *
 * \returns the number of characters read (at least 1 if `begin < end`).
 *
 * \see lex_utf8()
 */
LELY_UTIL_EXTERN size_t lex_c99_esc(const char *begin, const char *end,
		struct floc *at, char32_t *pc32);

/*!
 * Lexes a UTF-8 encoded Unicode string delimited by double quotes from a
 * memory buffer. The string MAY contain C99 character escape sequences.
 *
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 *
 * \returns the number of characters read (including the quotes).
 *
 * \see lex_c99_esc()
 */
LELY_UTIL_EXTERN size_t lex_c99_str(const char *begin, const char *end,
		struct floc *at);

/*!
 * Lexes a C99 preprocessing number from a memory buffer. Note that this does
 * not necessarily correspond to a valid integer or floating-point constant.
 *
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 *
 * \returns the number of characters read.
 */
LELY_UTIL_EXTERN size_t lex_c99_pp_num(const char *begin, const char *end,
		struct floc *at);

#define LELY_UTIL_DEFINE_LEX_SIGNED(type, suffix, strtov, pname) \
	/*! Parses a C99 `type` from a memory buffer. The actual conversion is
	performed by `strtov()`.
	\param begin a pointer to the start of the buffer.
	\param end   a pointer to the end of the buffer (can be NULL if the
	             buffer is null-terminated).
	\param at    an optional pointer to the file location of \a begin (used
	             for diagnostic purposes). On success, if `at != NULL`,
	             *\a at points to one past the last character parsed. On
	             error, *\a at is left untouched.
	\param pname the address at which to store the value. On success, if
	             \a pname is not NULL, *\a pname contains the parsed value.
	             On error, *\a pname is left untouched. On
	             underflow/overflow, *\a pname contains the minimum/maximum
	             value and get_errnum() returns #ERRNUM_RANGE.
	\returns the number of characters read. */ \
	LELY_UTIL_EXTERN size_t lex_c99_##suffix(const char *begin, \
			const char *end, struct floc *at, type *pname);

#define LELY_UTIL_DEFINE_LEX_UNSIGNED(type, suffix, strtov, pname) \
	/*! Parses a C99 `type` from a memory buffer. The actual conversion is
	performed by `strtov()`.
	\param begin a pointer to the start of the buffer.
	\param end   a pointer to the end of the buffer (can be NULL if the
	             buffer is null-terminated).
	\param at    an optional pointer to the file location of \a begin (used
	             for diagnostic purposes). On success, if `at != NULL`,
	             *\a at points to one past the last character parsed. On
	             error, *\a at is left untouched.
	\param pname the address at which to store the value. On success, if
	             \a pname is not NULL, *\a pname contains the parsed value.
	             On error, *\a pname is left untouched. On overflow,
	             *\a pname contains the maximum value and get_errnum()
	             returns #ERRNUM_RANGE.
	\returns the number of characters read. */ \
	LELY_UTIL_EXTERN size_t lex_c99_##suffix(const char *begin, \
			const char *end, struct floc *at, type *pname);

LELY_UTIL_DEFINE_LEX_SIGNED(long, long, strtol, pl)
LELY_UTIL_DEFINE_LEX_UNSIGNED(unsigned long, ulong, strtoul, pul)
#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L
LELY_UTIL_DEFINE_LEX_SIGNED(long long, llong, strtoll, pll)
LELY_UTIL_DEFINE_LEX_UNSIGNED(unsigned long long, ullong, strtoull, pul)
#endif
LELY_UTIL_DEFINE_LEX_SIGNED(float, flt, strtof, pf)
LELY_UTIL_DEFINE_LEX_SIGNED(double, dbl, strtod, pd)
LELY_UTIL_DEFINE_LEX_SIGNED(long double, ldbl, strtold, pld)

#undef LELY_UTIL_DEFINE_LEX_UNSIGNED
#undef LELY_UTIL_DEFINE_LEX_SIGNED

#define LELY_UTIL_DEFINE_LEX_SIGNED(type, suffix, pname) \
	/*! Parses a C99 `type` from a memory buffer.
	\param begin a pointer to the start of the buffer.
	\param end   a pointer to the end of the buffer (can be NULL if the
	             buffer is null-terminated).
	\param at    an optional pointer to the file location of \a begin (used
	             for diagnostic purposes). On success, if `at != NULL`,
	             *\a at points to one past the last character parsed. On
	             error, *\a at is left untouched.
	\param pname the address at which to store the value. On success, if
	             \a pname is not NULL, *\a pname contains the parsed value.
	             On error, *\a pname is left untouched. On
	             underflow/overflow, *\a pname contains the minimum/maximum
	             value and get_errnum() returns #ERRNUM_RANGE.
	\returns the number of characters read. */ \
	LELY_UTIL_EXTERN size_t lex_c99_##suffix(const char *begin, \
			const char *end, struct floc *at, type *pname);

#define LELY_UTIL_DEFINE_LEX_UNSIGNED(type, suffix, pname) \
	/*! Parses a C99 `type` from a memory buffer.
	\param begin a pointer to the start of the buffer.
	\param end   a pointer to the end of the buffer (can be NULL if the
	             buffer is null-terminated).
	\param at    an optional pointer to the file location of \a begin (used
	             for diagnostic purposes). On success, if `at != NULL`,
	             *\a at points to one past the last character parsed. On
	             error, *\a at is left untouched.
	\param pname the address at which to store the value. On success, if
	             \a pname is not NULL, *\a pname contains the parsed value.
	             On error, *\a pname is left untouched. On overflow,
	             *\a pname contains the maximum value and get_errnum()
	             returns #ERRNUM_RANGE.
	\returns the number of characters read. */ \
	LELY_UTIL_EXTERN size_t lex_c99_##suffix(const char *begin, \
			const char *end, struct floc *at, type *pname);

LELY_UTIL_DEFINE_LEX_SIGNED(int8_t, i8, pi8)
LELY_UTIL_DEFINE_LEX_SIGNED(int16_t, i16, pi16)
LELY_UTIL_DEFINE_LEX_SIGNED(int32_t, i32, pi32)
LELY_UTIL_DEFINE_LEX_SIGNED(int64_t, i64, pi64)
LELY_UTIL_DEFINE_LEX_UNSIGNED(uint8_t, u8, pu8)
LELY_UTIL_DEFINE_LEX_UNSIGNED(uint16_t, u16, pu16)
LELY_UTIL_DEFINE_LEX_UNSIGNED(uint32_t, u32, pu32)
LELY_UTIL_DEFINE_LEX_UNSIGNED(uint64_t, u64, pu64)

#undef LELY_UTIL_DEFINE_LEX_UNSIGNED
#undef LELY_UTIL_DEFINE_LEX_SIGNED

/*!
 * Lexes a single line-comment (excluding the line break) starting with the
 * specified delimiter from a memory buffer.
 *
 * \param delim a pointer to the delimiter indicating the start of a comment
 *              (can be NULL or pointing to the empty string to parse a line
 *              unconditionally).
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character parsed. On error, *\a at is left
 *              untouched.
 *
 * \returns the number of characters read.
 */
LELY_UTIL_EXTERN size_t lex_line_comment(const char *delim, const char *begin,
		const char *end, struct floc *at);

static inline int __cdecl
isbreak(int c)
{
	return c == '\n' || c == '\r';
}

static inline int __cdecl
isodigit(int c)
{
	return c >= '0' && c <= '7';
}

static inline int
ctoo(int c)
{
	return c - '0';
}

static inline int
ctox(int c)
{
	return isdigit(c) ? c - '0' : 10 + (isupper(c) ? c - 'A' : c - 'a');
}

#ifdef __cplusplus
}
#endif

#endif

