/**@file
 * This file is part of the utilities library; it exposes the byte order
 * functions.
 *
 * @see lely/util/endian.h
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

#include "util.h"
#define LELY_UTIL_ENDIAN_INLINE extern inline LELY_DLL_EXPORT
#include <lely/util/endian.h>

#include <assert.h>

static inline void bitcpy(
		unsigned char *dst, unsigned char src, unsigned char mask);

LELY_UTIL_EXPORT void
bcpybe(void *dst, int dstbit, const void *src, int srcbit, size_t n)
{
	if (!n)
		return;

	assert(dst);
	assert(src);

	unsigned char *dp = dst;
	dp += dstbit / CHAR_BIT;
	dstbit %= CHAR_BIT;
	if (dstbit < 0) {
		dp--;
		dstbit += CHAR_BIT;
	}

	const unsigned char *sp = src;
	sp += srcbit / CHAR_BIT;
	srcbit %= CHAR_BIT;
	if (srcbit < 0) {
		sp--;
		srcbit += CHAR_BIT;
	}

	unsigned char first = UCHAR_MAX >> dstbit;
	unsigned char last = ~(UCHAR_MAX >> ((dstbit + n) % CHAR_BIT));

	int shift = dstbit - srcbit;
	if (shift) {
		int right = shift & (CHAR_BIT - 1);
		int left = -shift & (CHAR_BIT - 1);

		if (dstbit + n <= CHAR_BIT) {
			if (last)
				first &= last;
			if (shift > 0) {
				bitcpy(dp, *sp >> right, first);
			} else if (srcbit + n <= CHAR_BIT) {
				bitcpy(dp, *sp << left, first);
			} else {
				bitcpy(dp, *sp << left | sp[1] >> right, first);
			}
		} else {
			unsigned char b = *sp++;
			if (shift > 0) {
				bitcpy(dp, b >> right, first);
			} else {
				bitcpy(dp, b << left | *sp >> right, first);
				b = *sp++;
			}
			dp++;
			n -= CHAR_BIT - dstbit;

			int m = n % CHAR_BIT;
			n /= CHAR_BIT;
			while (n--) {
				*dp++ = b << left | *sp >> right;
				b = *sp++;
			}

			if (last) {
				if (m <= right)
					bitcpy(dp, b << left, last);
				else
					bitcpy(dp, b << left | *sp >> right,
							last);
			}
		}
	} else {
		if (dstbit + n <= CHAR_BIT) {
			if (last)
				first &= last;
			bitcpy(dp, *sp, first);
		} else {
			if (first) {
				bitcpy(dp++, *sp++, first);
				n -= CHAR_BIT - dstbit;
			}

			n /= CHAR_BIT;
			while (n--)
				*dp++ = *sp++;

			if (last)
				bitcpy(dp, *sp, last);
		}
	}
}

LELY_UTIL_EXPORT void
bcpyle(void *dst, int dstbit, const void *src, int srcbit, size_t n)
{
	if (!n)
		return;

	assert(dst);
	assert(src);

	unsigned char *dp = dst;
	dp += dstbit / CHAR_BIT;
	dstbit %= CHAR_BIT;
	if (dstbit < 0) {
		dp--;
		dstbit += CHAR_BIT;
	}

	const unsigned char *sp = src;
	sp += srcbit / CHAR_BIT;
	srcbit %= CHAR_BIT;
	if (srcbit < 0) {
		sp--;
		srcbit += CHAR_BIT;
	}

	unsigned char first = UCHAR_MAX << dstbit;
	unsigned char last = ~(UCHAR_MAX << ((dstbit + n) % CHAR_BIT));

	int shift = dstbit - srcbit;
	if (shift) {
		int right = -shift & (CHAR_BIT - 1);
		int left = shift & (CHAR_BIT - 1);

		if (dstbit + n <= CHAR_BIT) {
			if (last)
				first &= last;
			if (shift > 0) {
				bitcpy(dp, *sp << left, first);
			} else if (srcbit + n <= CHAR_BIT) {
				bitcpy(dp, *sp >> right, first);
			} else {
				bitcpy(dp, *sp >> right | sp[1] << left, first);
			}
		} else {
			unsigned char b = *sp++;
			if (shift > 0) {
				bitcpy(dp, b << left, first);
			} else {
				bitcpy(dp, b >> right | *sp << left, first);
				b = *sp++;
			}
			dp++;
			n -= CHAR_BIT - dstbit;

			int m = n % CHAR_BIT;
			n /= CHAR_BIT;
			while (n--) {
				*dp++ = b >> right | *sp << left;
				b = *sp++;
			}

			if (last) {
				if (m <= right)
					bitcpy(dp, b >> right, last);
				else
					bitcpy(dp, b >> right | *sp << left,
							last);
			}
		}
	} else {
		if (dstbit + n <= CHAR_BIT) {
			if (last)
				first &= last;
			bitcpy(dp, *sp, first);
		} else {
			if (first) {
				bitcpy(dp++, *sp++, first);
				n -= CHAR_BIT - dstbit;
			}

			n /= CHAR_BIT;
			while (n--)
				*dp++ = *sp++;

			if (last)
				bitcpy(dp, *sp, last);
		}
	}
}

static inline void
bitcpy(unsigned char *dst, unsigned char src, unsigned char mask)
{
	*dst = ((src ^ *dst) & mask) ^ *dst;
}
