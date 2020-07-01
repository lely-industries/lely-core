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

#include <lely/co/crc.h>

TEST_GROUP(CO_Crc){};

TEST(CO_Crc, CheckZero) {
  const uint_least8_t buf = 0x00;

  const auto crc = co_crc(0x0000, &buf, 1);

  CHECK_EQUAL(0x0000, crc);
}

/* "VerifyCRCComplianceECSS" test cases are based on section B.1.5 in Annex B
 * of ECSS-E-ST-70-41C (15 April 2016) document.
 */
TEST(CO_Crc, VerifyCRCComplianceECSS_1) {
  const size_t BUF_SIZE = 2;
  const uint_least8_t buf[BUF_SIZE] = {0x00, 0x00};

  const auto crc = co_crc(0xffff, buf, BUF_SIZE);

  CHECK_EQUAL(0x1d0f, crc);
}

TEST(CO_Crc, VerifyCRCComplianceECSS_2) {
  const size_t BUF_SIZE = 3;
  const uint_least8_t buf[BUF_SIZE] = {0x00, 0x00, 0x00};

  const auto crc = co_crc(0xffff, buf, BUF_SIZE);

  CHECK_EQUAL(0xcc9c, crc);
}

TEST(CO_Crc, VerifyCRCComplianceECSS_3) {
  const size_t BUF_SIZE = 4;
  const uint_least8_t buf[BUF_SIZE] = {0xab, 0xcd, 0xef, 0x01};

  const auto crc = co_crc(0xffff, buf, BUF_SIZE);

  CHECK_EQUAL(0x04a2, crc);
}

TEST(CO_Crc, VerifyCRCComplianceECSS_4) {
  const size_t BUF_SIZE = 6;
  const uint_least8_t buf[BUF_SIZE] = {0x14, 0x56, 0xf8, 0x9a, 0x00, 0x01};

  const auto crc = co_crc(0xffff, buf, BUF_SIZE);

  CHECK_EQUAL(0x7fd5, crc);
}

/* "VerifyCRC_CiA301" test case is based on section 7.2.4.3.16 in CiA 301
 * (version 4.2.0) document.
 */
TEST(CO_Crc, VerifyCRC_CiA301) {
  const size_t BUF_SIZE = 9;
  const uint_least8_t buf[BUF_SIZE] = {'1', '2', '3', '4', '5',
                                       '6', '7', '8', '9'};

  const auto crc = co_crc(0x0000, buf, BUF_SIZE);

  CHECK_EQUAL(0x31c3, crc);
}

TEST(CO_Crc, NullBuffer) {
  const auto crc = co_crc(0x0000, nullptr, 5);

  CHECK_EQUAL(0x0000, crc);
}

TEST(CO_Crc, ZeroSize) {
  const size_t BUF_SIZE = 4;
  const uint_least8_t buf[BUF_SIZE] = {0x4d, 0x88, 0x12, 0xac};

  const auto crc = co_crc(0x0000, buf, 0);

  CHECK_EQUAL(0x0000, crc);
}
