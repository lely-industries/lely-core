/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020-2021 N7 Space Sp. z o.o.
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

#include <lely/co/crc.h>

TEST_GROUP(CO_Crc){};
/// @name co_crc()
///@{

/// \Given any initial CRC value
///
/// \When co_crc() is called with no buffer
///
/// \Then the same CRC value is returned
TEST(CO_Crc, NullBuffer) {
  const auto crc = co_crc(0x1234u, nullptr, 0);

  CHECK_EQUAL(0x1234u, crc);
}

/// \Given any initial CRC value
///
/// \When co_crc() is called with a buffer of zero size
///
/// \Then the same CRC value is returned
TEST(CO_Crc, ZeroSize) {
  const uint_least8_t buf[] = {0xffu};

  const auto crc = co_crc(0x0000u, buf, 0);

  CHECK_EQUAL(0x0000u, crc);
}

/// \Given the initial CRC `0x0000`
///
/// \When co_crc() is called with a buffer containing `0x00`
///
/// \Then `0x0000` is returned
TEST(CO_Crc, CheckZero) {
  const size_t BUF_SIZE = 1u;
  const uint_least8_t buf[BUF_SIZE] = {0x00u};

  const auto crc = co_crc(0x0000u, buf, BUF_SIZE);

  CHECK_EQUAL(0x0000u, crc);
}

/// \Given the initial CRC `0xffff`
///
/// \When co_crc() is called with a buffer containing `0x00 0x00`
///
/// \Then `0x1d0f` is returned
///
/// \note This case is based on ECSS-E-ST-70-41C Annex B Table B-2.
TEST(CO_Crc, VerifyCRCComplianceECSS_1) {
  const size_t BUF_SIZE = 2u;
  const uint_least8_t buf[BUF_SIZE] = {0x00u, 0x00u};

  const auto crc = co_crc(0xffffu, buf, BUF_SIZE);

  CHECK_EQUAL(0x1d0fu, crc);
}

/// \Given the initial CRC `0xffff`
///
/// \When co_crc() is called with a buffer containing `0x00 0x00 0x00`
///
/// \Then `0xcc9c` is returned
///
/// \note This case is based on ECSS-E-ST-70-41C Annex B Table B-2.
TEST(CO_Crc, VerifyCRCComplianceECSS_2) {
  const size_t BUF_SIZE = 3u;
  const uint_least8_t buf[BUF_SIZE] = {0x00u, 0x00u, 0x00u};

  const auto crc = co_crc(0xffffu, buf, BUF_SIZE);

  CHECK_EQUAL(0xcc9cu, crc);
}

/// \Given the initial CRC `0xffff`
///
/// \When co_crc() is called with a buffer containing `0xab 0xcd 0xef 0x01`
///
/// \Then `0x04a2` is returned
///
/// \note This case is based on ECSS-E-ST-70-41C Annex B Table B-2.
TEST(CO_Crc, VerifyCRCComplianceECSS_3) {
  const size_t BUF_SIZE = 4u;
  const uint_least8_t buf[BUF_SIZE] = {0xabu, 0xcdu, 0xefu, 0x01u};

  const auto crc = co_crc(0xffffu, buf, BUF_SIZE);

  CHECK_EQUAL(0x04a2u, crc);
}

/// \Given the initial CRC `0xffff`
///
/// \When co_crc() is called with a buffer containing
///       `0x14 0x56 0xf8 0x9a 0x00 0x01`
///
/// \Then `0x7fd5` is returned
///
/// \note This case is based on ECSS-E-ST-70-41C Annex B Table B-2.
TEST(CO_Crc, VerifyCRCComplianceECSS_4) {
  const size_t BUF_SIZE = 6u;
  const uint_least8_t buf[BUF_SIZE] = {0x14u, 0x56u, 0xf8u,
                                       0x9au, 0x00u, 0x01u};

  const auto crc = co_crc(0xffffu, buf, BUF_SIZE);

  CHECK_EQUAL(0x7fd5u, crc);
}

/// \Given the initial CRC `0x0000`
///
/// \When co_crc() is called with a buffer of characters `"123456789"`
///
/// \Then `0x31c3` is returned
///
/// \note This case is based on section 7.2.4.3.16 in CiA 301 (version 4.2.0).
TEST(CO_Crc, VerifyCRC_CiA301) {
  const size_t BUF_SIZE = 9u;
  const uint_least8_t buf[BUF_SIZE] = {'1', '2', '3', '4', '5',
                                       '6', '7', '8', '9'};

  const auto crc = co_crc(0x0000u, buf, BUF_SIZE);

  CHECK_EQUAL(0x31c3u, crc);
}

///@}
