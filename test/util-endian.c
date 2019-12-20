#include "test.h"
#include <lely/util/endian.h>

#define VALUE UINT64_C(0x0123456789abcdef)
#define BVAL_BE \
	{ \
		0x00, 0x3c, 0xda, 0xb8, 0x96, 0x74, 0x52, 0x00 \
	}
#define BVAL_LE \
	{ \
		0x00, 0xdc, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x02 \
	}

int
main(void)
{
	tap_plan(4);

	// clang-format off
	static const uint_least8_t rev8[] = {
		0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
		0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
		0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
		0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
		0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
		0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
		0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
		0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
		0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
		0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
		0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
		0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
		0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
		0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
		0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
		0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
		0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
		0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
		0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
		0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
		0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
		0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
		0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
		0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
		0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
		0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
		0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
		0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
		0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
		0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
		0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
		0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
	};
	// clang-format on

	int ok;
	uint_least8_t chk[1 + 8 + 1];
	uint_least8_t src[1 + 8 + 1];
	uint_least8_t dst[8];

	bcpybe(0, 0, 0, 0, 0);

	ok = 1;
	for (int dstbit = 0; dstbit < 8 * 8 && ok; dstbit++) {
		memset(chk, 0, sizeof(chk));
		chk[dstbit / 8] = rev8[1 << (dstbit % 8)];
		for (int srcbit = 0; srcbit < 8 * 8 && ok; srcbit++) {
			memset(src, 0, sizeof(src));
			src[srcbit / 8] = rev8[1 << (srcbit % 8)];

			memset(dst, 0, sizeof(dst));
			bcpybe(dst, dstbit, src, srcbit, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));

			memset(dst, 0, sizeof(dst));
			bcpybe(dst + 1, dstbit - 8, src, srcbit, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));

			memset(dst, 0, sizeof(dst));
			bcpybe(dst, dstbit, src + 1, srcbit - 8, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));

			memset(dst, 0, sizeof(dst));
			bcpybe(dst + 1, dstbit - 8, src + 1, srcbit - 8, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));
		}
	}
	tap_test(ok);

	ok = 1;
	for (int n = 1; n <= 16 && ok; n++) {
		uint_least32_t mask = (1ul << n) - 1;
		for (int dstbit = 0; dstbit < (8 * 8 - n) && ok; dstbit++) {
			memset(chk, 0, sizeof(chk));
			uint_least32_t val = mask << (dstbit % 8);
			chk[(dstbit / 8) + 0] = rev8[val & 0xff];
			chk[(dstbit / 8) + 1] = rev8[(val >> 8) & 0xff];
			chk[(dstbit / 8) + 2] = rev8[(val >> 16) & 0xff];
			for (int srcbit = 0; srcbit < (8 * 8 - n) && ok;
					srcbit++) {
				memset(src, 0, sizeof(src));
				val = mask << (srcbit % 8);
				src[(srcbit / 8) + 0] = rev8[val & 0xff];
				src[(srcbit / 8) + 1] = rev8[(val >> 8) & 0xff];
				src[(srcbit / 8) + 2] =
						rev8[(val >> 16) & 0xff];

				memset(dst, 0, sizeof(dst));
				bcpybe(dst, dstbit, src, srcbit, n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));

				memset(dst, 0, sizeof(dst));
				bcpybe(dst + 1, dstbit - 8, src, srcbit, n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));

				memset(dst, 0, sizeof(dst));
				bcpybe(dst, dstbit, src + 1, srcbit - 8, n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));

				memset(dst, 0, sizeof(dst));
				bcpybe(dst + 1, dstbit - 8, src + 1, srcbit - 8,
						n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));
			}
		}
	}
	tap_test(ok);

	bcpyle(0, 0, 0, 0, 0);

	ok = 1;
	for (int dstbit = 0; dstbit < 8 * 8 && ok; dstbit++) {
		memset(chk, 0, sizeof(chk));
		chk[dstbit / 8] = 1 << (dstbit % 8);
		for (int srcbit = 0; srcbit < 8 * 8 && ok; srcbit++) {
			memset(src, 0, sizeof(src));
			src[srcbit / 8] = 1 << (srcbit % 8);

			memset(dst, 0, sizeof(dst));
			bcpyle(dst, dstbit, src, srcbit, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));

			memset(dst, 0, sizeof(dst));
			bcpyle(dst + 1, dstbit - 8, src, srcbit, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));

			memset(dst, 0, sizeof(dst));
			bcpyle(dst, dstbit, src + 1, srcbit - 8, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));

			memset(dst, 0, sizeof(dst));
			bcpyle(dst + 1, dstbit - 8, src + 1, srcbit - 8, 1);
			ok = ok && !memcmp(dst, chk, sizeof(dst));
		}
	}
	tap_test(ok);

	ok = 1;
	for (int n = 1; n <= 16 && ok; ++n) {
		uint_least32_t mask = (1u << n) - 1;
		for (int dstbit = 0; dstbit < (8 * 8 - n) && ok; ++dstbit) {
			memset(chk, 0, sizeof(chk));
			uint_least32_t val = mask << (dstbit % 8);
			chk[(dstbit / 8) + 0] = val & 0xff;
			chk[(dstbit / 8) + 1] = (val >> 8) & 0xff;
			chk[(dstbit / 8) + 2] = (val >> 16) & 0xff;
			for (int srcbit = 0; srcbit < (8 * 8 - n) && ok;
					++srcbit) {
				memset(src, 0, sizeof(src));
				val = mask << (srcbit % 8);
				src[(srcbit / 8) + 0] = val & 0xff;
				src[(srcbit / 8) + 1] = (val >> 8) & 0xff;
				src[(srcbit / 8) + 2] = (val >> 16) & 0xff;

				memset(dst, 0, sizeof(dst));
				bcpyle(dst, dstbit, src, srcbit, n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));

				memset(dst, 0, sizeof(dst));
				bcpyle(dst + 1, dstbit - 8, src, srcbit, n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));

				memset(dst, 0, sizeof(dst));
				bcpyle(dst, dstbit, src + 1, srcbit - 8, n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));

				memset(dst, 0, sizeof(dst));
				bcpyle(dst + 1, dstbit - 8, src + 1, srcbit - 8,
						n + 1);
				ok = ok && !memcmp(dst, chk, sizeof(dst));
			}
		}
	}
	tap_test(ok);

	return 0;
}
