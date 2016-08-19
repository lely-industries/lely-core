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
#include <lely/util/print.h>
#include "unicode.h"

#include <ctype.h>

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

