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

#include <CppUTest/TestHarness.h>

#include <lely/co/pdo.h>
#include <lely/co/sdo.h>
#include <lely/co/type.h>
#include <lely/util/endian.h>
#include <lely/util/ustring.h>

#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"
#include "holder/array-init.hpp"

TEST_GROUP(CO_Sdo) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);

  void CheckArrayIsZeroed(const char* const array, const size_t size) {
    CHECK(array != nullptr);
    for (size_t i = 0u; i < size; i++) CHECK_EQUAL(0, array[i]);
  }
};

// given: SDO request with offset 0
// when: co_sdo_req_first()
// then: returns true
TEST(CO_Sdo, CoSdoReqFirst_IsFirst) { CHECK(co_sdo_req_first(&req)); }

// given: SDO request with offset 1
// when: co_sdo_req_first()
// then: returns false
TEST(CO_Sdo, CoSdoReqFirst_IsNotFirst) {
  req.offset = 1u;

  CHECK(!co_sdo_req_first(&req));
}

// given: SDO request with offset + nbyte == size
// when: co_sdo_req_first()
// then: returns false
TEST(CO_Sdo, CoSdoReqLast_IsLast) { CHECK(co_sdo_req_last(&req)); }

// given: SDO request with offset + nbyte < size
// when: co_sdo_req_first()
// then: returns false
TEST(CO_Sdo, CoSdoReqLast_IsNotLast) {
  req.size = 1u;

  CHECK(!co_sdo_req_last(&req));
}

// given: SDO abort code (AC)
// when: co_sdo_ac2str()
// then: a string describing the AC is returned
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
  STRCMP_EQUAL("Unknown abort code", co_sdo_ac2str(0xffffffffu));
}

// given: SDO request and an empty buffer
// when: co_sdo_req_init()
// then: SDO request is initialized with expected values and given buffer
TEST(CO_Sdo, CoSdoReqInit) {
  co_sdo_req req_init;
  membuf buf = MEMBUF_INIT;

  co_sdo_req_init(&req_init, &buf);

  CHECK_EQUAL(0u, req_init.size);
  POINTERS_EQUAL(nullptr, req_init.buf);
  CHECK_EQUAL(0u, req_init.nbyte);
  CHECK_EQUAL(0u, req_init.offset);
  POINTERS_EQUAL(&buf, req_init.membuf);
#if LELY_NO_MALLOC
  POINTERS_EQUAL(req_init._begin, req_init._membuf.begin);
  POINTERS_EQUAL(req_init._begin, req_init._membuf.cur);
  POINTERS_EQUAL(req_init._begin + CO_SDO_REQ_MEMBUF_SIZE,
                 req_init._membuf.end);
  CHECK(req_init._begin != nullptr);
#else
  POINTERS_EQUAL(nullptr, req_init.membuf->begin);
  POINTERS_EQUAL(nullptr, req_init.membuf->cur);
  POINTERS_EQUAL(nullptr, req_init.membuf->end);
#endif
}

// given: SDO request
// when: co_sdo_req_init()
// then: SDO request is initialized with expected values
TEST(CO_Sdo, CoSdoReqInit_BufNull) {
  co_sdo_req_init(&req, nullptr);

  CHECK_EQUAL(0u, req.size);
  POINTERS_EQUAL(nullptr, req.buf);
  CHECK_EQUAL(0u, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
  POINTERS_EQUAL(&req._membuf, req.membuf);
#if LELY_NO_MALLOC
  POINTERS_EQUAL(req._begin, req._membuf.begin);
  POINTERS_EQUAL(req._begin, req._membuf.cur);
  POINTERS_EQUAL(req._begin + CO_SDO_REQ_MEMBUF_SIZE, req._membuf.end);
  CHECK(req._begin != nullptr);
#else
  POINTERS_EQUAL(nullptr, req.membuf->begin);
  POINTERS_EQUAL(nullptr, req.membuf->cur);
  POINTERS_EQUAL(nullptr, req.membuf->end);
#endif
}

// given: SDO request
// when: CO_SDO_REQ_INIT()
// then: req's member variables are initialized with expected values
TEST(CO_Sdo, CoSdoReqInit_Macro) {
  co_sdo_req req_init = CO_SDO_REQ_INIT(req_init);

  CHECK_EQUAL(0u, req_init.size);
  POINTERS_EQUAL(nullptr, req_init.buf);
  CHECK_EQUAL(0u, req_init.nbyte);
  CHECK_EQUAL(0u, req_init.offset);
  POINTERS_EQUAL(&req_init._membuf, req_init.membuf);
#if LELY_NO_MALLOC
  POINTERS_EQUAL(req_init._begin, req_init._membuf.begin);
  POINTERS_EQUAL(req_init._begin, req_init._membuf.cur);
  POINTERS_EQUAL(req_init._begin + CO_SDO_REQ_MEMBUF_SIZE,
                 req_init._membuf.end);
  CHECK(req_init._begin != nullptr);
#else
  POINTERS_EQUAL(nullptr, req_init.membuf->begin);
  POINTERS_EQUAL(nullptr, req_init.membuf->cur);
  POINTERS_EQUAL(nullptr, req_init.membuf->end);
#endif
}

// given: SDO request
// when: co_sdo_req_fini()
// then: the executable does not crash
TEST(CO_Sdo, CoSdoReqFini) { co_sdo_req_fini(&req); }

// given: SDO request
// when: co_sdo_req_clear()
// then: req's member variables contains the expected values
TEST(CO_Sdo, CoSdoReqClear) {
  char buf = 'X';
  req.buf = &buf;
  req.size = 1u;
  req.nbyte = 1u;
  req.offset = 1u;
  *req.membuf = {&buf + 1u, &buf, &buf + 1u};

  co_sdo_req_clear(&req);

  CHECK_EQUAL(0u, req.size);
  CHECK_EQUAL(0u, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
  POINTERS_EQUAL(nullptr, req.buf);
  POINTERS_EQUAL(req.membuf->cur, req.membuf->begin);
  POINTERS_EQUAL(req.membuf->begin + 1u, req.membuf->end);
}

// given: invalid (empty with offset 1) download request
// when: co_sdo_req_dn()
// then: download request returns an error
TEST(CO_Sdo, CoSdoReqDn_Error) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

// given: invalid (empty with offset 1) download request
// when: co_sdo_req_dn()
// then: download request returns an error
TEST(CO_Sdo, CoSdoReqDn_ErrorNoAcPointer) {
  size_t nbyte = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, nullptr);

  CHECK_EQUAL(-1, ret);
}

// given: empty download request
// when: co_sdo_req_dn()
// then: download request returns a success
TEST(CO_Sdo, CoSdoReqDn_Empty) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
}

// given: SDO download request
// when: co_sdo_req_dn()
// then: -1 was returned but no abort code
TEST(CO_Sdo, CoSdoReqDn_NotAllDataAvailable) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  char buffer[] = {0x03, 0x04, 0x05};
  req.buf = buffer;
  req.size = 3u;
  req.nbyte = 2u;
  req.offset = 0u;
  char internal_buffer[3u] = {0};
  *req.membuf = {internal_buffer, internal_buffer, internal_buffer + 3u};

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(nullptr, ibuf);
  CHECK_EQUAL(0u, nbyte);
  CHECK_EQUAL(0x03, buffer[0]);
  CHECK_EQUAL(0x04, buffer[1]);
  CHECK_EQUAL(0x05, buffer[2]);
  CHECK_EQUAL(0x03, internal_buffer[0]);
  CHECK_EQUAL(0x04, internal_buffer[1]);
  CHECK_EQUAL(0x00, internal_buffer[2]);
}

// given: SDO download request
// when: co_sdo_req_dn()
// then: download request returns a success and no data was copied to the
//       memory buffer (because data is available right away).
TEST(CO_Sdo, CoSdoReqDn) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  char buffer[] = {0x03, 0x04, 0x05};
  req.buf = buffer;
  req.size = 3u;
  req.nbyte = 3u;
  req.offset = 0u;
  char internal_buffer[3u] = {0};
  *req.membuf = {internal_buffer, internal_buffer, internal_buffer + 3u};

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(buffer, ibuf);
  CHECK_EQUAL(3u, nbyte);
  CHECK_EQUAL(0x03, buffer[0]);
  CHECK_EQUAL(0x04, buffer[1]);
  CHECK_EQUAL(0x05, buffer[2]);
  CheckArrayIsZeroed(internal_buffer, 3u);
}

// given: request to download value which is unavailable
// when: co_sdo_req_dn_buf()
// then: -1 is returned with NO_MEM in LELY_NO_MALLOC and 0 in !LELY_NO_MALLOC
TEST(CO_Sdo, CoSdoReqDnBuf_ValNotAvailable) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  req.offset = 0u;
  req.size = 5u;
  req.nbyte = 0u;
  *req.membuf = MEMBUF_INIT;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
#if LELY_NO_MALLOC
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
#else
  CHECK_EQUAL(0u, ac);
#endif
}

// given: request to download bytes with current buffer position
//        after the end of the buffer
// when: co_sdo_req_dn_buf()
// then: error is returned
TEST(CO_Sdo, CoSdoReqDnBuf_BufCurrentPositionAfterTheEnd) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  req.offset = 12u;
  req.size = 6u;
  req.nbyte = 4u;
  req.membuf->cur = req.membuf->begin + 13u;

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
  POINTERS_EQUAL(nullptr, ibuf);
}

// given: request to download bytes but source buffer is not specified
// when: co_sdo_req_dn_buf()
// then: success is returned
TEST(CO_Sdo, CoSdoReqDnBuf_NoBufferPointerNoNbytePointer) {
  co_unsigned32_t ac = 0u;
  for (co_unsigned8_t i = 0u; i < membuf_size(req.membuf); i++)
    req.membuf->begin[i] = i + 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, nullptr, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  for (co_unsigned8_t i = 0u; i < membuf_size(req.membuf); i++)
    CHECK_EQUAL(static_cast<char>(i + 1), req.membuf->begin[i]);
}

// given: request to download bytes
// when: co_sdo_req_dn_buf()
// then: success is returned and ibuf is equal begin of the membuf
TEST(CO_Sdo, CoSdoReqDnBuf_BufCurrentPositionAfterTheEnd_BufferEmpty) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  req.offset = 12u;
  req.size = 6u;
  req.nbyte = 0u;
  req.membuf->cur = req.membuf->begin + 13u;

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(req.membuf->begin, ibuf);
}

// given: SDO request and an example value
// when: co_sdo_req_dn_buf()
// then: success is returned
TEST(CO_Sdo, CoSdoReqDnBuf_WithOffsetZero) {
  co_unsigned8_t val = 0xFFu;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;
  req.nbyte = 0u;
  req.size = 1u;
#if !LELY_NO_MALLOC
  char membuf[8] = {0};
  *req.membuf = {membuf, membuf, membuf + 8u};
#endif
  req.membuf->cur = req.membuf->begin + 1u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
}

// given: invalid download request and an example variable
// when: co_sdo_req_dn_val()
// then: an error is returned
TEST(CO_Sdo, CoSdoReqDnVal_WithOffset) {
  co_unsigned8_t val = 0xFFu;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

// given: buffer and correct download request
// when: co_sdo_req_dn_val()
// then: download request returns a success and a variable has a value
//       specified by the buffer
TEST(CO_Sdo, CoSdoReqDnVal_BasicDataType) {
  co_unsigned16_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0u;

  co_unsigned8_t buf[2] = {0xCEu, 0x7Bu};
  req.buf = buf;
  req.size = 2u;
  req.nbyte = 2u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(ldle_u16(buf), val);
}

// given: request to download 4-bytes long buffer to 2-byte variable
// when: co_sdo_req_dn_val()
// then: an error and abort code: type length too high is returned
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooManyBytes) {
  co_unsigned16_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0u;

  char buf[] = {0x12, 0x34, 0x56, 0x78};
  req.buf = buf;
  req.size = 4u;
  req.nbyte = 4u;
#if !LELY_NO_MALLOC
  char membuf[8] = {0};
  *req.membuf = {membuf, membuf, membuf + 8u};
#endif

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, ac);
  CHECK_EQUAL(0x3412u, val);
  CheckArrayIsZeroed(req.membuf->begin, 8u);
}

// given: request to download 4-bytes long buffer to 8-byte variable
// when: co_sdo_req_dn_val()
// then: an error and abort code: type length too low is returned
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooLittleBytes) {
  co_unsigned64_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;
  co_unsigned32_t ac = 0u;

  char buf[4] = {0x7Eu, 0x7Bu, 0x34u, 0x7Bu};
  req.buf = buf;
  req.size = 4u;
  req.nbyte = 4u;
#if !LELY_NO_MALLOC
  char membuf[8] = {0};
  *req.membuf = {membuf, membuf, membuf + 8u};
#endif

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, ac);
  CHECK_EQUAL(0u, val);
  CheckArrayIsZeroed(req.membuf->begin, 8u);
}

// given: request to download 4-bytes long buffer to 8-byte variable
// when: co_sdo_req_dn_val()
// then: -1 is returned and buffer is not modified
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooLittleBytesNoAcPointer) {
  co_unsigned64_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;

  co_unsigned8_t buf[4] = {0xCEu, 0x7Bu, 0x34u, 0xDBu};
  req.buf = buf;
  req.size = 4u;
  req.nbyte = 4u;
#if !LELY_NO_MALLOC
  char membuf[8] = {0};
  *req.membuf = {membuf, membuf, membuf + 8u};
#endif

  const auto ret = co_sdo_req_dn_val(&req, type, &val, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, val);
  CheckArrayIsZeroed(req.membuf->begin, 8u);
}

#if HAVE_LELY_OVERRIDE
// given: download request
// when: co_sdo_req_dn_val()
// then: nothing is downloaded
TEST(CO_Sdo, CoSdoReqDnVal_ArrayDataType_ReadValueFailed) {
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0u;

  char buf[4] = {0x01u, 0x01u, 0x00u, 0x2Bu};
  req.buf = buf;
  req.size = 2u;
  req.nbyte = 4u;
  LelyOverride::co_val_read(0);
#if !LELY_NO_MALLOC
  char membuf[8] = {0};
  *req.membuf = {membuf, membuf, membuf + 8u};
#endif

  CoArrays arrays;
  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4));

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);

  CheckArrayIsZeroed(req.membuf->begin, 8u);
#if LELY_NO_MALLOC
  const char16_t EXPECTED[2] = {0x0000, 0x0000};
  CHECK_EQUAL(0, memcmp(EXPECTED, str, 4u));
#else
  POINTERS_EQUAL(nullptr, str);
#endif
}
#endif  // HAVE_LELY_OVERRIDE

// given: request to download 4 bytes to an array, "FF" unicode string
// when: co_sdo_req_up_val()
// then: bytes were downloaded in correct order
TEST(CO_Sdo, CoSdoReqDnVal_ArrayDataType) {
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0u;
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0x2Bu, 0x00u};

  co_sdo_req_up(&req, buf, 4u);
  CoArrays arrays;
  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4u));
  CHECK_EQUAL(0, str16ncmp(STR_SRC, str, 2u));

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(4u, req.size);
  CHECK_EQUAL(4u, req.offset + req.nbyte);

  const char16_t EXPECTED[3] = {ldle_u16(buf), ldle_u16(buf + 2u), 0x0000};
  CHECK_EQUAL(0, str16ncmp(EXPECTED, str, 3u));
}

#if HAVE_LELY_OVERRIDE
// given: request to upload 2 bytes
// when: co_sdo_req_up_val()
// then: success is returned
TEST(CO_Sdo, CoSdoReqUpVal_NoValueWrite) {
  const co_unsigned16_t val = 0x4B7Du;
  co_unsigned32_t ac = 0u;

  char buf[2] = {0u};
  req.buf = buf;
  req.size = 2u;
  req.offset = 0u;
  req.nbyte = 2u;
  LelyOverride::co_val_write(0);

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CheckArrayIsZeroed(buf, 2u);
}
#endif  // HAVE_LELY_OVERRIDE

#if LELY_NO_MALLOC
// given: 2-byte value to upload
// when: co_sdo_req_up_val()
// then: CO_SDO_AC_NO_MEM error is returned
TEST(CO_Sdo, CoSdoReqUpVal_NoMemory) {
  uint_least8_t val_buffer[] = {0x7Au, 0x79u};
  const co_unsigned16_t val = ldle_u16(val_buffer);
  co_unsigned32_t ac = 0u;

  char buf[2] = {0u};
  req.buf = buf;
  *req.membuf = MEMBUF_INIT;

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);
  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
  CheckArrayIsZeroed(buf, 2u);
}
#endif  // LELY_NO_MALLOC

#if LELY_NO_MALLOC
// given: 2-byte value to upload
// when: co_sdo_req_up_val()
// then: error is returned
TEST(CO_Sdo, CoSdoReqUpVal_NoMemoryNoAcPointer) {
  const co_unsigned16_t val = 0x797Au;

  char buf[2] = {0u};
  req.buf = buf;
  req.membuf->begin = nullptr;
  req.membuf->end = nullptr;

  const auto ret =
      co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, nullptr);

  CHECK_EQUAL(-1, ret);
  CheckArrayIsZeroed(buf, 2u);
}
#endif  // LELY_NO_MALLOC

#if HAVE_LELY_OVERRIDE
// given: 2-byte value to upload
// when: co_sdo_req_up_val()
// then: an error is returned
TEST(CO_Sdo, CoSdoReqUpVal_SecondCoValWriteFail) {
  uint_least8_t val_buffer[] = {0x7Au, 0x79u};
  const co_unsigned16_t val = ldle_u16(val_buffer);
  co_unsigned32_t ac = 0u;

  char buf[2] = {0u};
  req.buf = buf;
  LelyOverride::co_val_write(1);
#if !LELY_NO_MALLOC
  char membuf[8] = {0};
  *req.membuf = {membuf, membuf, membuf + 8u};
#endif

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
  CheckArrayIsZeroed(req.membuf->begin, 2u);
}
#endif  // HAVE_LELY_OVERRIDE

// given: 2-byte value to upload
// when: co_sdo_req_up_val()
// then: 0 is returned, buffer contains suitable bytes
TEST(CO_Sdo, CoSdoReqUpVal) {
  uint_least8_t val_buffer[] = {0x7Au, 0x79u};
  const co_unsigned16_t val = ldle_u16(val_buffer);
  co_unsigned32_t ac = 0u;

  char buf[8] = {0u};
  req.buf = buf;
  *req.membuf = {buf, buf, buf + 8u};
  for (unsigned int i = 0; i < 8u; i++) req.membuf->begin[i] = 0u;

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(0x7Au, req.membuf->begin[0]);
  CHECK_EQUAL(0x79u, req.membuf->begin[1]);
  CheckArrayIsZeroed(req.membuf->begin + 2u, 6u);
  CHECK_EQUAL(2u, req.size);
  POINTERS_EQUAL(req.membuf->begin, req.buf);
  CHECK_EQUAL(2u, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
}
