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

#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"
#include "holder/array-init.hpp"

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

TEST_GROUP(CO_Sdo){};

TEST(CO_Sdo, CoSdoAc2Str) {
  STRCMP_EQUAL("Unsupported access to an object",
               co_sdo_ac2str(CO_SDO_AC_NO_ACCESS));
  STRCMP_EQUAL("Client/server command specifier not valid or unknown",
               co_sdo_ac2str(CO_SDO_AC_NO_CS));
  STRCMP_EQUAL("No data available", co_sdo_ac2str(CO_SDO_AC_NO_DATA));
  STRCMP_EQUAL("Out of memory", co_sdo_ac2str(CO_SDO_AC_NO_MEM));
  STRCMP_EQUAL("Object does not exist in the object dictionary",
               co_sdo_ac2str(CO_SDO_AC_NO_OBJ));
  STRCMP_EQUAL(
      "Object dictionary dynamic generation fails or no object dictionary is "
      "present",
      co_sdo_ac2str(CO_SDO_AC_NO_OD));
  STRCMP_EQUAL("Object cannot be mapped to the PDO",
               co_sdo_ac2str(CO_SDO_AC_NO_PDO));
  STRCMP_EQUAL("Attempt to read a write only object",
               co_sdo_ac2str(CO_SDO_AC_NO_READ));
  STRCMP_EQUAL("Resource not available: SDO connection",
               co_sdo_ac2str(CO_SDO_AC_NO_SDO));
  STRCMP_EQUAL("Sub-index does not exist", co_sdo_ac2str(CO_SDO_AC_NO_SUB));
  STRCMP_EQUAL("Attempt to write a read only object",
               co_sdo_ac2str(CO_SDO_AC_NO_WRITE));
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

TEST(CO_Sdo, CoSdoReqDnVal_WithoutOffsetLenLo) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned8_t value = 0xFFu;
  co_unsigned32_t ac = 0x00000000u;

  const auto ret = co_sdo_req_dn_val(&req, type, &value, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, ac);
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

TEST(CO_Sdo, CoSdoReqDnVal_INTEGER32) {
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

TEST(CO_Sdo, CoSdoReqDnVal_IsArrayButSizeNotMatching) {
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0x00u, 0x2Bu};

  CoArrays arrays;

  co_sdo_req req = CO_SDO_REQ_INIT(req);
  req.size = 4u;
  req.buf = buf;
  req.nbyte = 4u;
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  // char16_t value[8] = {0xFFFF, 0xFFFF, 0x0000, 0x0000,
  //                      0x0000, 0x0000, 0x0000, 0x0000};
  co_unsigned32_t ac = 0x00000000u;

  // char16_t src_arr[] = {0x0000, 0x0000, 0x0000, 0x0000,
  //                     0x0000, 0x0000, 0x0000, 0x0000};
  // co_val_init_array(src_arr, &arr);

  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\000\000";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4);

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
  // const char16_t * const expected = u"ā+";
  // CHECK(memcmp(expected, value, 4u));

  CHECK_EQUAL(4u, req.size);
  CHECK_EQUAL(req.offset + req.nbyte, req.size);
}

// TEST(CO_Sdo, CoSdoReqDnVal_WithoutOffsetLenHi) {

// TEST(CO_Sdo, CoSdoReqUp) {
//   co_sdo_req req = CO_SDO_REQ_INIT(req);
//   const size_t n = 2u;
//   char* buf[4] = {0x00u};

//   co_sdo_req_up(&req, buf, n);

//   CHECK_EQUAL(n, req.size);
//   POINTERS_EQUAL(buf, req.buf);
//   CHECK_EQUAL(req.nbyte, req.size);
//   CHECK_EQUAL(0u, req.offset);
// }
