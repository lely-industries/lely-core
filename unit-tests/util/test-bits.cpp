/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
 *
 * Unit Test Suite was developed under a programme of,
 * and funded by, the European Space Agency.
 *
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

#include <CppUTest/TestHarness.h>
#include <lely/util/bits.h>

TEST_GROUP(UtilBitsGroup){};

TEST(UtilBitsGroup, BSwap16) {
  // clang-format off
  static uint_least16_t test16[] = {
    0x0000, 0x0000,
    0xffff, 0xffff,
    0xafaf, 0xafaf,
    0x00ff, 0xff00,
    0x0001, 0x0100,
    0x1000, 0x0010,
    0x1234, 0x3412,
    0xbbdd, 0xddbb,
    0x4a3d, 0x3d4a,
    0x8758, 0x5887,
    0xa486, 0x86a4,
    0x28ea, 0xea28,
    0xe00d, 0x0de0,
    0x6222, 0x2262,
    0xadd7, 0xd7ad,
    0xfe57, 0x57fe,
  };
  // clang-format on

  const int tsize = sizeof(test16) / sizeof(uint_least16_t);

  for (int i = 0; i < tsize; i += 2) {
    CHECK_EQUAL(test16[i], bswap16(test16[i + 1]));
    CHECK_EQUAL(test16[i + 1], bswap16(test16[i]));
  }
}

TEST(UtilBitsGroup, BSwap32) {
  // clang-format off
  static uint_least32_t test32[] = {
    0x00000000, 0x00000000,
    0xffffffff, 0xffffffff,
    0xabcdcdab, 0xabcdcdab,
    0x0000ffff, 0xffff0000,
    0x00000001, 0x01000000,
    0x10000000, 0x00000010,
    0x12345678, 0x78563412,
    0xabde1379, 0x7913deab,
    0xbcde5c2c, 0x2c5cdebc,
    0x11c61f9b, 0x9b1fc611,
    0x62978ffa, 0xfa8f9762,
    0xd0b2fb90, 0x90fbb2d0,
    0x80d2b6a8, 0xa8b6d280,
    0xec14ef9e, 0x9eef14ec,
    0x7c8c8529, 0x29858c7c,
    0x7f5b330f, 0x0f335b7f,
  };
  // clang-format on

  const int tsize = sizeof(test32) / sizeof(uint_least32_t);

  for (int i = 0; i < tsize; i += 2) {
    CHECK_EQUAL(test32[i], bswap32(test32[i + 1]));
    CHECK_EQUAL(test32[i + 1], bswap32(test32[i]));
  }
}

TEST(UtilBitsGroup, BSwap64) {
  // clang-format off
  static uint_least64_t test64[] = {
    0x0000000000000000, 0x0000000000000000,
    0xffffffffffffffff, 0xffffffffffffffff,
    0xabcdef1212efcdab, 0xabcdef1212efcdab,
    0x00000000ffffffff, 0xffffffff00000000,
    0x0000000000000001, 0x0100000000000000,
    0x1000000000000000, 0x0000000000000010,
    0x0123456789abcdef, 0xefcdab8967452301,
    0xa8a43d00f3e67b7c, 0x7c7be6f3003da4a8,
    0xd519e9912a041e87, 0x871e042a91e919d5,
    0x31b0cd63c0b2a4c5, 0xc5a4b2c063cdb031,
    0xbcb66618fa9aaf22, 0x22af9afa1866b6bc,
    0xa617a18293bbd3f9, 0xf9d3bb9382a117a6,
    0xf42fe1ce2ee3bdbb, 0xbbbde32ecee12ff4,
    0xae3fbd91d8d4e911, 0x11e9d4d891bd3fae,
    0xf2ff58e8800a7ac9, 0xc97a0a80e858fff2,
    0xfb027d92f19239c2, 0xc23992f1927d02fb,
  };
  // clang-format on

  const int tsize = sizeof(test64) / sizeof(uint_least64_t);

  for (int i = 0; i < tsize; i += 2) {
    CHECK_EQUAL(test64[i], bswap64(test64[i + 1]));
    CHECK_EQUAL(test64[i + 1], bswap64(test64[i]));
  }
}

TEST(UtilBitsGroup, ClsClz8) {
  // clang-format off
  static uint_least8_t test8[] = {
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff,
    0x16, 0x9c, 0xd8, 0xed, 0xf5, 0xf9, 0xfd, 0xfe, 0xff,
    0x32, 0xa1, 0xc9, 0xe1, 0xf3, 0xfa, 0xfc, 0xfe, 0xff,
    0x79, 0xb3, 0xd5, 0xe8, 0xf7, 0xfb, 0xfd, 0xfe, 0xff,
  };
  // clang-format on

  const int tsize = sizeof(test8) / sizeof(uint_least8_t);
  const int max_s = cls8(0xff) + 1;
  const int max_z = clz8(0x00) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls8(test8[i]));
    CHECK_EQUAL(i % max_z, clz8(~test8[i]));
  }
}

TEST(UtilBitsGroup, ClsClz16) {
  // clang-format off
  static uint_least16_t test16[] = {
    0x0000,
    0x8000, 0xc000, 0xe000, 0xf000,
    0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0,
    0xfff8, 0xfffc, 0xfffe, 0xffff,

    0x741e,
    0x9d45, 0xcab3, 0xe0cc, 0xf4af,
    0xf9ad, 0xfd53, 0xfed2, 0xff68,
    0xffa0, 0xffd3, 0xffea, 0xfff5,
    0xfffb, 0xfffd, 0xfffe, 0xffff,
  };
  // clang-format on

  int tsize = sizeof(test16) / sizeof(uint_least16_t);
  const int max_s = cls16(0xffff) + 1;
  const int max_z = clz16(0x0) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls16(test16[i]));
    CHECK_EQUAL(i % max_z, clz16(~test16[i]));
  }
}

TEST(UtilBitsGroup, ClsClz32) {
  // clang-format off
  static uint_least32_t test32[] = {
    0x00000000,
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff,

    0x314ffe89,
    0xb5300dfc, 0xca78c934, 0xed9b3ecf, 0xf3b0843b,
    0xf8438ffd, 0xfdf6c7b9, 0xfefff835, 0xff3ef887,
    0xffaea43a, 0xffd8d510, 0xffeb2240, 0xfff40b94,
    0xfffb960e, 0xfffdeb71, 0xfffef349, 0xffff4bfd,
    0xffffad7a, 0xffffd203, 0xffffe2cc, 0xfffff6b6,
    0xfffff8f5, 0xfffffdea, 0xfffffe01, 0xffffff14,
    0xffffffa2, 0xffffffd0, 0xffffffe7, 0xfffffff5,
    0xfffffffa, 0xfffffffd, 0xfffffffe, 0xffffffff,
  };

  // clang-format on

  int tsize = sizeof(test32) / sizeof(uint_least32_t);
  const int max_s = cls32(0xffffffff) + 1;
  const int max_z = clz32(0x0) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls32(test32[i]));
    CHECK_EQUAL(i % max_z, clz32(~test32[i]));
  }
}

TEST(UtilBitsGroup, ClsClz64) {
  // clang-format off
  static uint_least64_t test64[] = {
    0x0000000000000000,
    0x8000000000000000, 0xc000000000000000, 0xe000000000000000, 0xf000000000000000,  // NOLINT
    0xf800000000000000, 0xfc00000000000000, 0xfe00000000000000, 0xff00000000000000,  // NOLINT
    0xff80000000000000, 0xffc0000000000000, 0xffe0000000000000, 0xfff0000000000000,  // NOLINT
    0xfff8000000000000, 0xfffc000000000000, 0xfffe000000000000, 0xffff000000000000,  // NOLINT
    0xffff800000000000, 0xffffc00000000000, 0xffffe00000000000, 0xfffff00000000000,  // NOLINT
    0xfffff80000000000, 0xfffffc0000000000, 0xfffffe0000000000, 0xffffff0000000000,  // NOLINT
    0xffffff8000000000, 0xffffffc000000000, 0xffffffe000000000, 0xfffffff000000000,  // NOLINT
    0xfffffff800000000, 0xfffffffc00000000, 0xfffffffe00000000, 0xffffffff00000000,  // NOLINT
    0xffffffff80000000, 0xffffffffc0000000, 0xffffffffe0000000, 0xfffffffff0000000,  // NOLINT
    0xfffffffff8000000, 0xfffffffffc000000, 0xfffffffffe000000, 0xffffffffff000000,  // NOLINT
    0xffffffffff800000, 0xffffffffffc00000, 0xffffffffffe00000, 0xfffffffffff00000,  // NOLINT
    0xfffffffffff80000, 0xfffffffffffc0000, 0xfffffffffffe0000, 0xffffffffffff0000,  // NOLINT
    0xffffffffffff8000, 0xffffffffffffc000, 0xffffffffffffe000, 0xfffffffffffff000,  // NOLINT
    0xfffffffffffff800, 0xfffffffffffffc00, 0xfffffffffffffe00, 0xffffffffffffff00,  // NOLINT
    0xffffffffffffff80, 0xffffffffffffffc0, 0xffffffffffffffe0, 0xfffffffffffffff0,  // NOLINT
    0xfffffffffffffff8, 0xfffffffffffffffc, 0xfffffffffffffffe, 0xffffffffffffffff,  // NOLINT

    0x42b139630a0b0596,
    0xacaa634ea2d357ab, 0xcab7e5abf85f3afc, 0xe4f7d1195acad6bf, 0xf019bcd064df1b62,  // NOLINT
    0xfbb88dd93589318b, 0xfccd0e9dbca29c56, 0xfe31bdeb1878a15d, 0xff71e71ae41c7cb6,  // NOLINT
    0xff819f2bfedaf46f, 0xffd53d744135992c, 0xffe18474b591b9c4, 0xfff3354a561b1bda,  // NOLINT
    0xfffbc68b13c69247, 0xfffd2f3db32cf9d0, 0xfffe94cd581a9bcb, 0xffff63a0e7c8cf46,  // NOLINT
    0xffff8d451a297fc2, 0xffffdf2affe13dde, 0xffffe0e9f450868b, 0xfffff33a9cb37a47,  // NOLINT
    0xfffffbef74ad5c91, 0xfffffd652a8b5c36, 0xfffffe5dc90b792c, 0xffffff0cdbfac333,  // NOLINT
    0xffffff9b23749aab, 0xffffffc8f3973014, 0xffffffe73617d2c9, 0xfffffff03d3ef0fb,  // NOLINT
    0xfffffff9a6888689, 0xfffffffc1c0e9717, 0xfffffffef1d8acbc, 0xffffffff7e391679,  // NOLINT
    0xffffffff8788908b, 0xffffffffd73e1ec9, 0xffffffffe35953e8, 0xfffffffff411752b,  // NOLINT
    0xfffffffff846f930, 0xfffffffffd89d7cc, 0xfffffffffe45ebcf, 0xffffffffff2e00ea,  // NOLINT
    0xffffffffffa5943b, 0xffffffffffc0774c, 0xffffffffffe7f965, 0xfffffffffff3474a,  // NOLINT
    0xfffffffffff8fe93, 0xfffffffffffc50cf, 0xfffffffffffea621, 0xffffffffffff08f0,  // NOLINT
    0xffffffffffff9d14, 0xffffffffffffd2cd, 0xffffffffffffed4b, 0xfffffffffffff555,  // NOLINT
    0xfffffffffffffa18, 0xfffffffffffffd22, 0xfffffffffffffe07, 0xffffffffffffff61,  // NOLINT
    0xffffffffffffffba, 0xffffffffffffffd9, 0xffffffffffffffeb, 0xfffffffffffffff5,  // NOLINT
    0xfffffffffffffffb, 0xfffffffffffffffd, 0xfffffffffffffffe, 0xffffffffffffffff,  // NOLINT
  };
  // clang-format on

  int tsize = sizeof(test64) / sizeof(uint_least64_t);
  const int max_s = cls64(0xffffffffffffffff) + 1;
  const int max_z = clz64(0x0) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls64(test64[i]));
    CHECK_EQUAL(i % max_z, clz64(~test64[i]));
  }
}

TEST(UtilBitsGroup, CtsCtz8) {
  // clang-format off
  static uint_least8_t test8[] = {
    0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
    0xaa, 0x11, 0x8b, 0x47, 0x6f, 0x5f, 0xbf, 0x7f, 0xff,
    0x40, 0xbd, 0xdb, 0x97, 0xef, 0xdf, 0x3f, 0x7f, 0xff,
    0x10, 0x61, 0xe3, 0xc7, 0xcf, 0x9f, 0xbf, 0x7f, 0xff,
  };
  // clang-format on

  const int tsize = sizeof(test8) / sizeof(uint_least8_t);
  const int max_s = cts8(0xff) + 1;
  const int max_z = ctz8(0x0) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts8(test8[i]));
    CHECK_EQUAL(i % max_z, ctz8(~test8[i]));
  }
}

TEST(UtilBitsGroup, CtsCtz16) {
  // clang-format off
  static uint_least16_t test16[] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f,
    0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff,
    0x1fff, 0x3fff, 0x7fff, 0xffff,

    0x5406,
    0x01e9, 0xa093, 0x11e7, 0x97cf,
    0xd01f, 0x21bf, 0x367f, 0x72ff,
    0xadff, 0xcbff, 0x57ff, 0x6fff,
    0xdfff, 0x3fff, 0x7fff, 0xffff,
  };
  // clang-format on

  const int tsize = sizeof(test16) / sizeof(uint_least16_t);
  const int max_s = cts16(0xffff) + 1;
  const int max_z = ctz16(0x0) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts16(test16[i]));
    CHECK_EQUAL(i % max_z, ctz16(~test16[i]));
  }
}

TEST(UtilBitsGroup, CtsCtz32) {
  // clang-format off
  static uint_least32_t test32[] = {
    0x00000000,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,

    0xf4d17e3c,
    0xdd5882dd, 0x198de57b, 0xc72748c7, 0x2285326f,
    0x7e00c31f, 0x17d64b3f, 0xd5a7e97f, 0x9f3f30ff,
    0xc116e5ff, 0x8f497bff, 0x136bd7ff, 0xbe510fff,
    0x14831fff, 0xbbbd3fff, 0xa6297fff, 0xd1eeffff,
    0x4e5dffff, 0x7e5bffff, 0xfb27ffff, 0xd52fffff,
    0xaa1fffff, 0xd7bfffff, 0xbb7fffff, 0x44ffffff,
    0xc5ffffff, 0x8bffffff, 0x67ffffff, 0x4fffffff,
    0x9fffffff, 0xbfffffff, 0x7fffffff, 0xffffffff,
  };
  // clang-format on

  const int tsize = sizeof(test32) / sizeof(uint_least32_t);
  const int max_s = cts32(0xffffffff) + 1;
  const int max_z = ctz32(0x0) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts32(test32[i]));
    CHECK_EQUAL(i % max_z, ctz32(~test32[i]));
  }
}

TEST(UtilBitsGroup, CtsCtz64) {
  // clang-format off
  static uint_least64_t test64[] = {
    0x0000000000000000,
    0x0000000000000001, 0x0000000000000003, 0x0000000000000007, 0x000000000000000f,  // NOLINT
    0x000000000000001f, 0x000000000000003f, 0x000000000000007f, 0x00000000000000ff,  // NOLINT
    0x00000000000001ff, 0x00000000000003ff, 0x00000000000007ff, 0x0000000000000fff,  // NOLINT
    0x0000000000001fff, 0x0000000000003fff, 0x0000000000007fff, 0x000000000000ffff,  // NOLINT
    0x000000000001ffff, 0x000000000003ffff, 0x000000000007ffff, 0x00000000000fffff,  // NOLINT
    0x00000000001fffff, 0x00000000003fffff, 0x00000000007fffff, 0x0000000000ffffff,  // NOLINT
    0x0000000001ffffff, 0x0000000003ffffff, 0x0000000007ffffff, 0x000000000fffffff,  // NOLINT
    0x000000001fffffff, 0x000000003fffffff, 0x000000007fffffff, 0x00000000ffffffff,  // NOLINT
    0x00000001ffffffff, 0x00000003ffffffff, 0x00000007ffffffff, 0x0000000fffffffff,  // NOLINT
    0x0000001fffffffff, 0x0000003fffffffff, 0x0000007fffffffff, 0x000000ffffffffff,  // NOLINT
    0x000001ffffffffff, 0x000003ffffffffff, 0x000007ffffffffff, 0x00000fffffffffff,  // NOLINT
    0x00001fffffffffff, 0x00003fffffffffff, 0x00007fffffffffff, 0x0000ffffffffffff,  // NOLINT
    0x0001ffffffffffff, 0x0003ffffffffffff, 0x0007ffffffffffff, 0x000fffffffffffff,  // NOLINT
    0x001fffffffffffff, 0x003fffffffffffff, 0x007fffffffffffff, 0x00ffffffffffffff,  // NOLINT
    0x01ffffffffffffff, 0x03ffffffffffffff, 0x07ffffffffffffff, 0x0fffffffffffffff,  // NOLINT
    0x1fffffffffffffff, 0x3fffffffffffffff, 0x7fffffffffffffff, 0xffffffffffffffff,  // NOLINT

    0xd082b0c953361364,
    0xdeab2ecd7f0fc25d, 0x699974bc5f7a7edb, 0x03208b5b39ff70f7, 0x9872283a5cc6394f,  // NOLINT
    0xba15af046d8a40df, 0x307b97335e8b113f, 0xacdc6b17044e917f, 0xd809c2f04835feff,  // NOLINT
    0xe02d29f55c904dff, 0x9dc69947208e03ff, 0xf537f67b3ca327ff, 0xbca9ff91f1f00fff,  // NOLINT
    0x9be3307ef517dfff, 0xde78d15b2f6ebfff, 0x808d2641990f7fff, 0xf93652c29832ffff,  // NOLINT
    0x80a6c80a142dffff, 0xab7a9d938f5bffff, 0xb15167d0d567ffff, 0xbd55891a80efffff,  // NOLINT
    0xce54e9cd35dfffff, 0x585249a6053fffff, 0x98d0a95d4f7fffff, 0x78095b96aeffffff,  // NOLINT
    0x54290dc925ffffff, 0x00dc5f72dbffffff, 0x535ee84597ffffff, 0xbaa4490eefffffff,  // NOLINT
    0x5e52603c5fffffff, 0x13d7e585bfffffff, 0x999b13257fffffff, 0x70335fc0ffffffff,  // NOLINT
    0x9cde0951ffffffff, 0x2fa4d78bffffffff, 0x1d19db47ffffffff, 0xb672a4efffffffff,  // NOLINT
    0x1ff5d8dfffffffff, 0x9bdff23fffffffff, 0xb90eec7fffffffff, 0xc0de8cffffffffff,  // NOLINT
    0x735265ffffffffff, 0x23018bffffffffff, 0xa0e1e7ffffffffff, 0x096a8fffffffffff,  // NOLINT
    0x82e61fffffffffff, 0x60a0bfffffffffff, 0x67e97fffffffffff, 0x0bd8ffffffffffff,  // NOLINT
    0xbf11ffffffffffff, 0x13e3ffffffffffff, 0x7377ffffffffffff, 0x0eafffffffffffff,  // NOLINT
    0x631fffffffffffff, 0xa73fffffffffffff, 0xfb7fffffffffffff, 0x92ffffffffffffff,  // NOLINT
    0x65ffffffffffffff, 0xabffffffffffffff, 0x47ffffffffffffff, 0x8fffffffffffffff,  // NOLINT
    0xdfffffffffffffff, 0xbfffffffffffffff, 0x7fffffffffffffff, 0xffffffffffffffff,  // NOLINT
  };
  // clang-format on

  const int tsize = sizeof(test64) / sizeof(uint_least64_t);
  const int max_s = cts64(0xffffffffffffffff) + 1;
  const int max_z = ctz64(0x0) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts64(test64[i]));
    CHECK_EQUAL(i % max_z, ctz64(~test64[i]));
  }
}
