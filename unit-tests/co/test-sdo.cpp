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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory>
#include <vector>

#include <CppUTest/TestHarness.h>

#include <lely/can/msg.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/pdo.h>
#include <lely/co/sdo.h>
#include <lely/co/type.h>
#include <lely/util/diag.h>
#include <lely/util/ustring.h>

#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"
#include "holder/array-init.hpp"

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

TEST_GROUP(CO_Sdo){};

TEST(CO_Sdo, CoSdoAc2Str) {
  STRCMP_EQUAL("Success", co_sdo_ac2str(0));
  STRCMP_EQUAL("Toggle bit not altered", co_sdo_ac2str(CO_SDO_AC_TOGGLE));
  STRCMP_EQUAL("SDO protocol timed out", co_sdo_ac2str(CO_SDO_AC_TIMEOUT));
  STRCMP_EQUAL("Client/server command specifier not valid or unknown",
               co_sdo_ac2str(CO_SDO_AC_NO_CS));
  STRCMP_EQUAL("Invalid block size", co_sdo_ac2str(CO_SDO_AC_BLK_SIZE));
  STRCMP_EQUAL("Invalid sequence number", co_sdo_ac2str(CO_SDO_AC_BLK_SEQ));
  STRCMP_EQUAL("CRC error", co_sdo_ac2str(CO_SDO_AC_BLK_CRC));
  STRCMP_EQUAL("Out of memory", co_sdo_ac2str(CO_SDO_AC_NO_MEM));
  STRCMP_EQUAL("Unsupported access to an object",
               co_sdo_ac2str(CO_SDO_AC_NO_ACCESS));
  STRCMP_EQUAL("Attempt to read a write only object",
               co_sdo_ac2str(CO_SDO_AC_NO_READ));
  STRCMP_EQUAL("Attempt to write a read only object",
               co_sdo_ac2str(CO_SDO_AC_NO_WRITE));
  STRCMP_EQUAL("Object does not exist in the object dictionary",
               co_sdo_ac2str(CO_SDO_AC_NO_OBJ));
  STRCMP_EQUAL("Object cannot be mapped to the PDO",
               co_sdo_ac2str(CO_SDO_AC_NO_PDO));
  STRCMP_EQUAL(
      "The number and length of the objects to be mapped would exceed the PDO "
      "length",
      co_sdo_ac2str(CO_SDO_AC_PDO_LEN));
  STRCMP_EQUAL("General parameter incompatibility reason",
               co_sdo_ac2str(CO_SDO_AC_PARAM));
  STRCMP_EQUAL("General internal incompatibility in the device",
               co_sdo_ac2str(CO_SDO_AC_COMPAT));
  STRCMP_EQUAL("Access failed due to a hardware error",
               co_sdo_ac2str(CO_SDO_AC_HARDWARE));
  STRCMP_EQUAL(
      "Data type does not match, length of service parameter does not match",
      co_sdo_ac2str(CO_SDO_AC_TYPE_LEN));
  STRCMP_EQUAL("Data type does not match, length of service parameter too high",
               co_sdo_ac2str(CO_SDO_AC_TYPE_LEN_HI));
  STRCMP_EQUAL("Data type does not match, length of service parameter too low",
               co_sdo_ac2str(CO_SDO_AC_TYPE_LEN_LO));
  STRCMP_EQUAL("Sub-index does not exist", co_sdo_ac2str(CO_SDO_AC_NO_SUB));
  STRCMP_EQUAL("Invalid value for parameter",
               co_sdo_ac2str(CO_SDO_AC_PARAM_VAL));
  STRCMP_EQUAL("Value of parameter written too high",
               co_sdo_ac2str(CO_SDO_AC_PARAM_HI));
  STRCMP_EQUAL("Value of parameter written too low",
               co_sdo_ac2str(CO_SDO_AC_PARAM_LO));
  STRCMP_EQUAL("Maximum value is less than minimum value",
               co_sdo_ac2str(CO_SDO_AC_PARAM_RANGE));
  STRCMP_EQUAL("Resource not available: SDO connection",
               co_sdo_ac2str(CO_SDO_AC_NO_SDO));
  STRCMP_EQUAL("General error", co_sdo_ac2str(CO_SDO_AC_ERROR));
  STRCMP_EQUAL("Data cannot be transferred or stored to the application",
               co_sdo_ac2str(CO_SDO_AC_DATA));
  STRCMP_EQUAL(
      "Data cannot be transferred or stored to the application because of "
      "local control",
      co_sdo_ac2str(CO_SDO_AC_DATA_CTL));
  STRCMP_EQUAL(
      "Data cannot be transferred or stored to the application because of the "
      "present device state",
      co_sdo_ac2str(CO_SDO_AC_DATA_DEV));
  STRCMP_EQUAL(
      "Object dictionary dynamic generation fails or no object dictionary is "
      "present",
      co_sdo_ac2str(CO_SDO_AC_NO_OD));
  STRCMP_EQUAL("No data available", co_sdo_ac2str(CO_SDO_AC_NO_DATA));
  STRCMP_EQUAL("Unknown abort code", co_sdo_ac2str(0x00000042u));
}

TEST(CO_Sdo, CoSdoReqInit) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  POINTERS_EQUAL(nullptr, req.buf);
  POINTERS_EQUAL(&req._membuf, req.membuf);
  POINTERS_EQUAL(req._membuf.begin, req._membuf.cur);
  CHECK(req._membuf.begin != nullptr);
  CHECK(req._membuf.cur != nullptr);
  CHECK(req._membuf.end != nullptr);
  CHECK(req._begin != nullptr);
  CHECK_EQUAL(0, req.size);

  membuf mbuf = MEMBUF_INIT;

  co_sdo_req_init(&req, &mbuf);

  POINTERS_EQUAL(&mbuf, req.membuf);
  CHECK(req._membuf.begin != nullptr);
  CHECK(req._membuf.cur != nullptr);
  CHECK(req._membuf.end != nullptr);
  CHECK(req._begin != nullptr);
  CHECK_EQUAL(0, req.size);
}

TEST(CO_Sdo, CoSdoReqFini) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  co_sdo_req_init(&req, nullptr);

  co_sdo_req_fini(&req);
}

TEST(CO_Sdo, CoSdoReqClear) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);

  co_sdo_req_clear(&req);

  CHECK_EQUAL(0, req.size);
  CHECK_EQUAL(0, req.nbyte);
  CHECK_EQUAL(0, req.offset);
  POINTERS_EQUAL(nullptr, req.buf);
  POINTERS_EQUAL(req.membuf->cur, req.membuf->begin);
}

TEST(CO_Sdo, CoSdoReqDn_Error) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  size_t nbyte = 0;
  co_unsigned32_t ac = 0x00000000u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

TEST(CO_Sdo, CoSdoReqDn_ErrorNoAcVariable) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  size_t nbyte = 0;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, nullptr);

  CHECK_EQUAL(-1, ret);
}

TEST(CO_Sdo, CoSdoReqDn) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  size_t nbyte = 0;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x00000000u, ac);
}

TEST(CO_Sdo, CoSdoReqDnVal_WithOffset) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  req.offset = 1;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned8_t value = 0xFFu;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

TEST(CO_Sdo, CoSdoReqDnVal_Unsigned8) {
  co_unsigned8_t buf[1] = {0xCEu};
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  req.size = 1u;
  req.buf = buf;
  req.nbyte = 1u;
  co_unsigned8_t value = 0x00u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0xCEu, value);
  CHECK_EQUAL(0x00000000u, ac);
}

TEST(CO_Sdo, CoSdoReqDnVal_Unsigned16) {
  co_unsigned8_t buf[2] = {0xCEu, 0x7Bu};
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  req.size = 2u;
  req.buf = buf;
  req.nbyte = 2u;
  co_unsigned16_t value = 0x0000u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x7BCEu, value);
  CHECK_EQUAL(0x00000000u, ac);
}

TEST(CO_Sdo, CoSdoReqDnVal_Integer32) {
  co_unsigned8_t buf[4] = {0xCEu, 0x7Bu, 0x34u, 0xFDu};
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  req.size = 4u;
  req.buf = buf;
  req.nbyte = 4u;
  co_integer32_t value = 0x00000000u;
  co_unsigned16_t type = CO_DEFTYPE_INTEGER32;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(-46892082, value);
  CHECK_EQUAL(0x00000000u, ac);
}

TEST(CO_Sdo, CoSdoReqDnVal_Unsigned64) {
  co_unsigned8_t buf[8] = {0xCEu, 0x7Bu, 0x34u, 0xDBu,
                           0x8Au, 0xC1u, 0x03u, 0x56u};
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  req.size = 8u;
  req.buf = buf;
  req.nbyte = 8u;
  co_unsigned64_t value = 0x0000000000000000u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x5603C18ADB347BCEu, value);
  CHECK_EQUAL(0x00000000u, ac);
}

TEST(CO_Sdo, CoSdoReqDnVal_Unsigned64CoValReadFailed) {
  co_unsigned8_t buf[8] = {0xCEu, 0x7Bu, 0x34u, 0xDBu,
                           0x8Au, 0xC1u, 0x03u, 0x56u};
  co_sdo_req req = CO_SDO_REQ_INIT(req);

  LelyOverride::co_val_read(0);

  req.size = 8u;
  req.buf = buf;
  req.nbyte = 8u;
  co_unsigned64_t value = 0x0000000000000000u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, ac);
  CHECK_EQUAL(0x0000000000000000u, value);
}

TEST(CO_Sdo, CoSdoReqDnVal_Unsigned64CoValReadFailedNoAcVariable) {
  co_unsigned8_t buf[8] = {0xCEu, 0x7Bu, 0x34u, 0xDBu,
                           0x8Au, 0xC1u, 0x03u, 0x56u};
  co_sdo_req req = CO_SDO_REQ_INIT(req);

  LelyOverride::co_val_read(0);

  req.size = 8u;
  req.buf = buf;
  req.nbyte = 8u;
  co_unsigned64_t value = 0x0000000000000000u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0x0000000000000000u, value);
}

TEST(CO_Sdo, CoSdoReqDnVal_IsArrayButCoValReadFailed) {
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0x00u, 0x2Bu};

  CoArrays arrays;
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  req.size = 2u;
  req.buf = buf;
  req.nbyte = 4u;
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0x00000000u;

  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4));

  LelyOverride::co_val_read(0);
  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
  const char16_t* const expected = u"\u0101\u2B00";
  CHECK(memcmp(expected, str, 4u));
}

TEST(CO_Sdo, CoSdoReqDnVal_IsArrayButTransmittedInParts) {
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0xC9u, 0x24u};

  CoArrays arrays;
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  co_sdo_req_up(&req, buf, 4u);
  req.nbyte = 2u;
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0x00000000u;

  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4u));
  CHECK_EQUAL(0, str16ncmp(STR_SRC, str, 2u));

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0x00000000u, ac);
  CHECK_EQUAL(0, str16ncmp(STR_SRC, str, 2u));
  const char16_t* const expected = u"\u0101\u0000";
  // characters are copied to the temporary memory buffer
  CHECK(memcmp(expected, req.membuf->begin, 4u) == 0);

  req.buf = buf + 2u;
  req.offset = 2u;
  req.nbyte = 2u;

  co_sdo_req_dn_val(&req, type, &str, &ac);

  const char16_t* const expected2 = u"āⓉ";
  CHECK_EQUAL(0, str16ncmp(expected2, str, 2u));
}

TEST(CO_Sdo, CoSdoReqDnVal_IsArray) {
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0x2Bu, 0x00u};

  CoArrays arrays;
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  co_sdo_req_up(&req, buf, 4u);
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0x00000000u;

  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4u));
  CHECK_EQUAL(0, str16ncmp(STR_SRC, str, 2u));

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x00000000u, ac);
  const char16_t* const expected = u"\u0101\u002B";
  CHECK_EQUAL(0, str16ncmp(expected, str, 2u));

  CHECK_EQUAL(4u, req.size);
  CHECK_EQUAL(req.offset + req.nbyte, req.size);
}

TEST(CO_Sdo, CoSdoReqUpVal_CoValWriteFail) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  char buf[6] = {0x00u};
  req.buf = buf;
  const co_unsigned16_t val = 0xABCDu;
  co_unsigned32_t ac = 0x00000000u;
  LelyOverride::co_val_write(0);

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(0x00u, buf[0]);
  CHECK_EQUAL(0x00u, buf[1]);
}

TEST(CO_Sdo, CoSdoReqUpVal_NoMemory) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  char buf[6] = {0x00u};
  req.buf = buf;
  const co_unsigned16_t val = 0x797Au;
  co_unsigned32_t ac = 0x00000000u;

  req.membuf->begin = nullptr;
  req.membuf->end = nullptr;

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
}

TEST(CO_Sdo, CoSdoReqUpVal_NoMemoryNoAcVariable) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  char buf[6] = {0x00u};
  req.buf = buf;
  const co_unsigned16_t val = 0x797Au;

  req.membuf->begin = nullptr;
  req.membuf->end = nullptr;

  const auto ret =
      co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, nullptr);

  CHECK_EQUAL(-1, ret);
}

TEST(CO_Sdo, CoSdoReqUpVal_SecondCoValWriteFail) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  char buf[6] = {0x00u};
  req.buf = buf;
  const co_unsigned16_t val = 0x797Au;
  co_unsigned32_t ac = 0x00000000u;
  LelyOverride::co_val_write(1);

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

TEST(CO_Sdo, CoSdoReqUpVal) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  char buf[6] = {0x00u};
  req.buf = buf;
  const co_unsigned16_t val = 0x797Au;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  char expected[2] = {0x79u, 0x7Au};
  CHECK(memcmp(expected, req.membuf->begin, 2u));
}

TEST(CO_Sdo, CoSdoReqDnBuf_ValNotAvailable) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  size_t nbyte = 0;
  co_unsigned32_t ac = 0x00000000u;
  req.offset = 0u;

  req.size = 5u;
  req.nbyte = 4u;
  req.membuf->begin = nullptr;
  req.membuf->end = nullptr;
  req.membuf->cur = nullptr;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
}

TEST(CO_Sdo, CoSdoReqDnBuf_BufCurrentPositionAfterTheEnd) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  size_t nbyte = 0;
  co_unsigned32_t ac = 0x00000000u;

  req.offset = 12u;
  req.size = 6u;
  req.nbyte = 4u;
  req.membuf->cur = req.membuf->begin + 13u;

  char buffer[10] = {0};
  const void** buf_ptr = reinterpret_cast<const void**>(&buffer);

  const auto ret = co_sdo_req_dn(&req, buf_ptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

// TEST(CO_Sdo, CoSdoReqDnBuf_BufCurrentPositionAfterTheEndNoNbyte) {
//   co_sdo_req req = CO_SDO_REQ_INIT(req);

//   req.offset = 0u;
//   req.size = 6u;
//   req.nbyte = 4u;
//   req.membuf->cur = req.membuf->begin;

//   const auto ret = co_sdo_req_dn(&req, nullptr, nullptr, nullptr);

//   CHECK_EQUAL(-1, ret);
// }
