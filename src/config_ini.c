/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the INI parser for configuration structs.
 *
 * \see lely/util/config.h
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
#include <lely/util/config.h>
#include <lely/util/diag.h>
#include <lely/util/frbuf.h>
#include <lely/util/lex.h>
#include <lely/util/membuf.h>
#include <lely/util/print.h>

#include <assert.h>

static int __cdecl issection(int c);
static int __cdecl iskey(int c);
static int __cdecl isvalue(int c);

static size_t skip(const char *begin, const char *end, struct floc *at);

static void membuf_print_chars(struct membuf *buf, const char *s, size_t n);

LELY_UTIL_EXPORT size_t
config_parse_ini_file(config_t *config, const char *filename)
{
	frbuf_t *buf = frbuf_create(filename);
	if (__unlikely(!buf)) {
		diag(DIAG_ERROR, get_errc(), "%s", filename);
		return 0;
	}

	size_t size = 0;
	const void *map = frbuf_map(buf, 0, &size);
	if (__unlikely(!map)) {
		diag(DIAG_ERROR, get_errc(), "unable to map file");
		frbuf_destroy(buf);
		return 0;
	}

	const char *begin = map;
	const char *end = begin + size;
	struct floc at = { filename, 1, 1 };
	size_t chars = config_parse_ini_text(config, begin, end, &at);

	frbuf_destroy(buf);

	return chars;
}

LELY_UTIL_EXPORT size_t
config_parse_ini_text(config_t *config, const char *begin, const char *end,
		struct floc *at)
{
	assert(config);
	assert(begin);
	assert(!end || end >= begin);

	struct membuf section = MEMBUF_INIT;
	struct membuf key = MEMBUF_INIT;
	struct membuf value = MEMBUF_INIT;

	const char *cp = begin;
	size_t chars = 0;

	for (;;) {
		// Skip comments and empty lines.
		for (;;) {
			cp += skip(cp, end, at);
			if ((chars = lex_break(cp, end, at)) > 0)
				cp += chars;
			else
				break;
		}
		if ((end && cp >= end) || !*cp)
			break;

		if ((chars = lex_char('[', cp, end, at)) > 0) {
			cp += chars;
			cp += skip(cp, end, at);
			if ((chars = lex_ctype(&issection, cp, end, at)) > 0) {
				membuf_print_chars(&section, cp, chars);
				cp += chars;
				cp += skip(cp, end, at);
				if ((chars = lex_char(']', cp, end, at)) > 0)
					cp += chars;
				else if (at)
					diag_at(DIAG_ERROR, 0, at,
							"expected ']' after section name");
			} else if (at) {
				diag_at(DIAG_ERROR, 0, at,
						"expected section name after '['");
			}
			cp += lex_line_comment(NULL, cp, end, at);
		} else if ((chars = lex_ctype(&iskey, cp, end, at)) > 0) {
			membuf_print_chars(&key, cp, chars);
			cp += chars;
			cp += skip(cp, end, at);
			if ((chars = lex_char('=', cp, end, at)) > 0) {
				cp += chars;
				cp += skip(cp, end, at);
				if ((chars = lex_char('\"', cp, end, at)) > 0) {
					cp += chars;
					lex_c99_str(cp, end, at, NULL, &chars);
					chars++;
					membuf_clear(&value);
					membuf_reserve(&value, chars);
					char *s = membuf_alloc(&value, &chars);
					if (__likely(s && chars))
						s[--chars] = '\0';
					cp += lex_c99_str(cp, end, NULL, s,
							&chars);
					if ((chars = lex_char('\"', cp, end,
							at)) > 0)
						cp += chars;
					else if (at)
						diag_at(DIAG_ERROR, 0, at,
								"expected '\"' after string");
				} else {
					chars = lex_ctype(&isvalue, cp, end,
							at);
					membuf_print_chars(&value, cp, chars);
					cp += chars;
				}
				config_set(config, membuf_begin(&section),
						membuf_begin(&key),
						membuf_begin(&value));
				membuf_clear(&value);
			} else if (at) {
				diag_at(DIAG_ERROR, 0, at,
						"expected '=' after key");
			}
			cp += lex_line_comment(NULL, cp, end, at);
		} else {
			if (at) {
				if (isgraph((unsigned char)*cp))
					diag_at(DIAG_ERROR, 0, at,
							"unknown character '%c'",
							*cp);
				else
					diag_at(DIAG_ERROR, 0, at,
							"unknown character '\\%o'",
							*cp);
			}
			// Skip the offending character.
			cp += lex_char(*cp, cp, end, at);
		}
	}

	membuf_fini(&value);
	membuf_fini(&key);
	membuf_fini(&section);

	return cp - begin;
}

static int __cdecl
issection(int c)
{
	return isgraph(c) && c != '#' && c != ';' && c != '[' && c != ']';
}

static int __cdecl
iskey(int c)
{
	return isgraph(c) && c != '#' && c != ';' && c != '=';
}

static int __cdecl
isvalue(int c)
{
	return isprint(c) && c != '#' && c != ';';
}

static size_t
skip(const char *begin, const char *end, struct floc *at)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;

	cp += lex_ctype(&isblank, cp, end, at);
	cp += lex_line_comment("#", cp, end, at);
	cp += lex_line_comment(";", cp, end, at);

	return cp - begin;
}

static void
membuf_print_chars(struct membuf *buf, const char *s, size_t n)
{
	assert(buf);

	// Remove trailing whitespace.
	while (n && isspace((unsigned char)s[n - 1]))
		n--;

	membuf_clear(buf);
	if (__unlikely(!membuf_reserve(buf, n + 1)))
		return;
	membuf_write(buf, s, n);
	membuf_write(buf, "", 1);
}

