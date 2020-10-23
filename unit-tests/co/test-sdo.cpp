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
#include <lely/util/ustring.h>

#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"
#include "holder/array-init.hpp"

TEST_GROUP(CO_Sdo) { co_sdo_req req = CO_SDO_REQ_INIT(req); };

// given: -
// when: abort code is passed as an argument to co_sdo_ac2str()
// then: string describing the AC is returned
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
  STRCMP_EQUAL("Unknown abort code", co_sdo_ac2str(42u));
}

// given: upload/download request and an empty buffer
// when: request initializer is called with this buffer
// then: req's member variables are initialized with expected values
TEST(CO_Sdo, CoSdoReqInit) {
  co_sdo_req req;
  membuf buf = MEMBUF_INIT;

  co_sdo_req_init(&req, &buf);

  CHECK_EQUAL(0u, req.size);
  POINTERS_EQUAL(nullptr, req.buf);
  CHECK_EQUAL(0u, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
  POINTERS_EQUAL(&buf, req.membuf);
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

// given: upload/download request
// when: request initializer is called with NULL buffer address
// then: req's member variables are initialized with expected values
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

// given: upload/download request
// when: request initializing macro is applied
// then: req's member variables are initialized with expected values
TEST(CO_Sdo, CoSdoReqInit_Macro) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);

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

// given: upload/download request
// when: req's destructor is called
// then: the executable does not crash
TEST(CO_Sdo, CoSdoReqFini) { co_sdo_req_fini(&req); }

// given: upload/download request
// when: request clear is called
// then: req's member variables contains the expected values
TEST(CO_Sdo, CoSdoReqClear) {
  co_sdo_req_clear(&req);

  CHECK_EQUAL(0, req.size);
  CHECK_EQUAL(0, req.nbyte);
  CHECK_EQUAL(0, req.offset);
  POINTERS_EQUAL(nullptr, req.buf);
  POINTERS_EQUAL(req.membuf->cur, req.membuf->begin);
}

// given: invalid (empty with offset 1) download request
// when: copy of the next segment to the internal buffer is requested
// then: download request returns an error
TEST(CO_Sdo, CoSdoReqDn_Error) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

// given: invalid (empty with offset 1) download request
// when: copy of the next segment to the internal buffer is requested
// then: download request returns an error
TEST(CO_Sdo, CoSdoReqDn_ErrorNoAcVariable) {
  size_t nbyte = 0;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, nullptr);

  CHECK_EQUAL(-1, ret);
}

// given: empty download request
// when: copy of the next segment to the internal buffer is requested
// then: download request returns a success
TEST(CO_Sdo, CoSdoReqDn_Empty) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, ac);
}

// given: example download request, lack of internal buffer address variable
// when: copy of the next segment to the internal buffer is requested
// then: download request returns a success and number of bytes in the internal
//       buffer
TEST(CO_Sdo, CoSdoReqDn_NoInternalBuffer) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;

  char buffer[] = {0x03, 0x04, 0x05};
  *req.membuf = {buffer, buffer, buffer + 3u};
  req.size = 3u;
  req.nbyte = 3u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, ac);
  CHECK_EQUAL(3u, nbyte);
}

// given: example download request
// when: copy of the next segment to the internal buffer is requested
// then: download request returns a success and no data was copied to the
//       internal buffer (because data is available right away).
TEST(CO_Sdo, CoSdoReqDn) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  char buffer[] = {0x03, 0x04, 0x05};
  req.size = 3u;
  req.nbyte = 3u;
  req.offset = 0u;
  char internal_buffer[3u] = {0};
  req.buf = internal_buffer;

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, ac);
  POINTERS_EQUAL(internal_buffer, ibuf);
  CHECK_EQUAL(3u, nbyte);
  CHECK_EQUAL(0x03, buffer[0]);
  CHECK_EQUAL(0x04, buffer[1]);
  CHECK_EQUAL(0x05, buffer[2]);
  CHECK_EQUAL(0x00, internal_buffer[0]);
  CHECK_EQUAL(0x00, internal_buffer[1]);
  CHECK_EQUAL(0x00, internal_buffer[2]);
}

// given: invalid download request and an example variable
// when: copy of the next segment to the specified buffer is requested
// then: an error is returned
TEST(CO_Sdo, CoSdoReqDnVal_WithOffset) {
  co_unsigned8_t val = 0xFFu;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned32_t ac = 0u;
  req.offset = 1;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

// given: buffer and correct download request
// when: download of the buffer to the value address is requested
// then: download request returns a success and a variable has a value
//       specified by the buffer
TEST(CO_Sdo, CoSdoReqDnVal_Unsigned16) {
  co_unsigned8_t buf[2] = {0xCEu, 0x7Bu};
  co_unsigned16_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0u;
  req.size = 2u;
  req.buf = buf;
  req.nbyte = 2u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(0x7BCEu, val);
}

// given: request to download 4-bytes long buffer to 2-byte variable
// when: request download value is issued
// then: an error and abort code: type length too high is returned
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooManyBytes) {
  char buf[] = {0x12, 0x34, 0x56, 0x78};
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned16_t val = 0u;
  co_unsigned32_t ac = 0u;
  req.size = 4u;
  req.buf = buf;
  req.nbyte = 4u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, ac);
  CHECK_EQUAL(0x3412u, val);
  CHECK_EQUAL(0, req.membuf->begin[0]);
  CHECK_EQUAL(0, req.membuf->begin[1]);
  CHECK_EQUAL(0, req.membuf->begin[2]);
  CHECK_EQUAL(0, req.membuf->begin[3]);
  CHECK_EQUAL(0, req.membuf->begin[4]);
  CHECK_EQUAL(0, req.membuf->begin[5]);
  CHECK_EQUAL(0, req.membuf->begin[6]);
  CHECK_EQUAL(0, req.membuf->begin[7]);
}

// given: request to download 4-bytes long buffer to 8-byte variable
// when: request download value is issued
// then: an error and abort code: type length too low is returned
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooLittleBytes) {
  char buf[4] = {0x7Eu, 0x7Bu, 0x34u, 0x7Bu};
  co_unsigned64_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;
  co_unsigned32_t ac = 0u;
  req.size = 4u;
  req.buf = buf;
  req.nbyte = 4u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, ac);
  CHECK_EQUAL(0u, val);
  CHECK_EQUAL(0, req.membuf->begin[0]);
  CHECK_EQUAL(0, req.membuf->begin[1]);
  CHECK_EQUAL(0, req.membuf->begin[2]);
  CHECK_EQUAL(0, req.membuf->begin[3]);
  CHECK_EQUAL(0, req.membuf->begin[4]);
  CHECK_EQUAL(0, req.membuf->begin[5]);
  CHECK_EQUAL(0, req.membuf->begin[6]);
  CHECK_EQUAL(0, req.membuf->begin[7]);
}

// given: request to download 4-bytes long buffer to 8-byte variable
// when: request download value is issued
// then: an error and abort code: type length too low is returned
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooLittleBytesNoAcPointer) {
  co_unsigned8_t buf[4] = {0xCEu, 0x7Bu, 0x34u, 0xDBu};
  co_unsigned64_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;
  req.size = 4u;
  req.buf = buf;
  req.nbyte = 4u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, val);
  CHECK_EQUAL(0, req.membuf->begin[0]);
  CHECK_EQUAL(0, req.membuf->begin[1]);
  CHECK_EQUAL(0, req.membuf->begin[2]);
  CHECK_EQUAL(0, req.membuf->begin[3]);
  CHECK_EQUAL(0, req.membuf->begin[4]);
  CHECK_EQUAL(0, req.membuf->begin[5]);
  CHECK_EQUAL(0, req.membuf->begin[6]);
  CHECK_EQUAL(0, req.membuf->begin[7]);
}

#if HAVE_LELY_OVERRIDE
// given: download request
// when: co_val_read() fails
// then: nothing is downloaded
TEST(CO_Sdo, CoSdoReqDnVal_IsArrayButCoValReadFailed) {
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0x00u, 0x2Bu};
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0u;
  req.size = 2u;
  req.buf = buf;
  req.nbyte = 4u;
  LelyOverride::co_val_read(0);

  CoArrays arrays;
  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4));

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
  CHECK_EQUAL(0, req.membuf->begin[0]);
  CHECK_EQUAL(0, req.membuf->begin[1]);
  CHECK_EQUAL(0, req.membuf->begin[2]);
  CHECK_EQUAL(0, req.membuf->begin[3]);
  CHECK_EQUAL(0, req.membuf->begin[4]);
  CHECK_EQUAL(0, req.membuf->begin[5]);
  CHECK_EQUAL(0, req.membuf->begin[6]);
  CHECK_EQUAL(0, req.membuf->begin[7]);
#if LELY_NO_MALLOC
  const char16_t* const expected = u"\u0000\u0000";
  CHECK_EQUAL(0, memcmp(expected, str, 4u));
#else
  POINTERS_EQUAL(nullptr, str);
#endif
}
#endif  // HAVE_LELY_OVERRIDE

// given: request to download 4 bytes but only 2 are available
// when: request is issued
// then: -1 is returned and first 2 bytes are written
// when: offset is added - 2 additional bytes became available
//       and second request is issued
// then: bytes are downloaded to the internal buffer
TEST(CO_Sdo, CoSdoReqDnVal_IsArrayButTransmittedInParts) {
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0xC9u, 0x24u};
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0u;
  co_sdo_req_up(&req, buf, 4u);
  req.nbyte = 2u;

  CoArrays arrays;
  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4u));
  CHECK_EQUAL(0, str16ncmp(STR_SRC, str, 2u));

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, ac);
  CHECK_EQUAL(0, str16ncmp(STR_SRC, str, 2u));
#if LELY_NO_MALLOC
  const char16_t* const expected = u"\u0101\u0000";
  CHECK_EQUAL(0u, memcmp(expected, req.membuf->begin, 4u));
#endif

  req.buf = buf + 2u;
  req.offset = 2u;
  req.nbyte = 2u;

  co_sdo_req_dn_val(&req, type, &str, &ac);

  const char16_t* const expected2 = u"\u0101\u24C9";
  CHECK_EQUAL(0, str16ncmp(expected2, str, 2u));
}

// given: request to download 4 bytes to an array, "FF" unicode string
// when: request is issued
// then: bytes were downloaded in correct order
TEST(CO_Sdo, CoSdoReqDnVal_IsArray) {
  co_unsigned8_t buf[4] = {0x01u, 0x01u, 0x2Bu, 0x00u};
  co_unsigned16_t type = CO_DEFTYPE_UNICODE_STRING;
  co_unsigned32_t ac = 0u;

  co_sdo_req_up(&req, buf, 4u);
  CoArrays arrays;
  co_unicode_string_t str = arrays.Init<co_unicode_string_t>();
  const char16_t* const STR_SRC = u"\u0046\u0046";
  CHECK_EQUAL(str16len(STR_SRC),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &str, STR_SRC, 4u));
  CHECK_EQUAL(0, str16ncmp(STR_SRC, str, 2u));

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, ac);
  const char16_t* const expected = u"\u0101\u002B";
  CHECK_EQUAL(0, str16ncmp(expected, str, 2u));

  CHECK_EQUAL(4u, req.size);
  CHECK_EQUAL(req.offset + req.nbyte, req.size);
}

#if HAVE_LELY_OVERRIDE
// given: request to upload 2 bytes
// when: request is issued but co_val_write does not write anything
// then:
TEST(CO_Sdo, CoSdoReqUpVal_NoValWrite) {
  char buf[2] = {0x00u};
  const co_unsigned16_t val = 0x4B7Du;
  co_unsigned32_t ac = 0u;
  req.buf = buf;
  req.size = 2u;
  req.offset = 0u;
  req.nbyte = 2u;
  LelyOverride::co_val_write(0);

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(0x00u, buf[0]);
  CHECK_EQUAL(0x00u, buf[1]);
}
#endif  // HAVE_LELY_OVERRIDE

#if LELY_NO_MALLOC
// given: 2-byte value to upload
// when: upload request is issued but buffer is empty
// then: error is returned
TEST(CO_Sdo, CoSdoReqUpVal_NoMemory) {
  char buf[2] = {0x00u};
  const co_unsigned16_t val = 0x797Au;
  co_unsigned32_t ac = 0u;
  req.buf = buf;
  *req.membuf = {nullptr, nullptr, nullptr};

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);
  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
  CHECK_EQUAL(0x00, buf[0]);
  CHECK_EQUAL(0x00, buf[1]);
}
#endif  // LELY_NO_MALLOC

#if LELY_NO_MALLOC
// given: 2-byte value to upload
// when: upload request is issued but buffer is empty
// then: error is returned
TEST(CO_Sdo, CoSdoReqUpVal_NoMemoryNoAcPointer) {
  char buf[2] = {0x00u};
  const co_unsigned16_t val = 0x797Au;
  req.buf = buf;
  req.membuf->begin = nullptr;
  req.membuf->end = nullptr;

  const auto ret =
      co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0x00, buf[0]);
  CHECK_EQUAL(0x00, buf[1]);
}
#endif  // LELY_NO_MALLOC

#if HAVE_LELY_OVERRIDE
// given: 2-byte value to upload
// when: upload request is issued but second call to co_val_read fails
// then: an error is returned
TEST(CO_Sdo, CoSdoReqUpVal_SecondCoValWriteFail) {
  char buf[2] = {0x00u};
  const co_unsigned16_t val = 0x797Au;
  co_unsigned32_t ac = 0u;
  req.buf = buf;
  LelyOverride::co_val_write(1);

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}
#endif  // HAVE_LELY_OVERRIDE

// given: 2-byte value to upload
// when: upload request is issued
// then: 0 is returned, buffer contains suitable bytes
TEST(CO_Sdo, CoSdoReqUpVal) {
  char buf[2] = {0x00u};
  const co_unsigned16_t val = 0x797Au;
  co_unsigned32_t ac = 0u;
  req.buf = buf;

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(0x7Au, req.membuf->begin[0]);
  CHECK_EQUAL(0x79u, req.membuf->begin[1]);
  CHECK_EQUAL(0x00u, req.membuf->begin[2]);
  CHECK_EQUAL(0x00u, req.membuf->begin[3]);
  CHECK_EQUAL(0x00u, req.membuf->begin[4]);
  CHECK_EQUAL(0x00u, req.membuf->begin[5]);
  CHECK_EQUAL(0x00u, req.membuf->begin[6]);
  CHECK_EQUAL(0x00u, req.membuf->begin[7]);
  CHECK_EQUAL(2u, req.size);
  POINTERS_EQUAL(req.membuf->begin, req.buf);
  CHECK_EQUAL(2u, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
}

// given: request to download value which is unavailable
// when: request is issued
// then: -1 is returned with NO_MEM in LELY_NO_MALLOC and 0 in !LELY_NO_MALLOC
TEST(CO_Sdo, CoSdoReqDnBuf_ValNotAvailable) {
  size_t nbyte = 0;
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
// when: download request is issued
// then: error is returned
TEST(CO_Sdo, CoSdoReqDnBuf_BufCurrentPositionAfterTheEnd) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;
  req.offset = 12u;
  req.size = 6u;
  req.nbyte = 4u;
  req.membuf->cur = req.membuf->begin + 13u;

  const void* ibuf = nullptr;

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
  POINTERS_EQUAL(nullptr, ibuf);
}

// given: request to download bytes but source buffer is not specified
// when: request is issued
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
// when: request is issued
// then: success is returned and ibuf is equal begin of the membuf
TEST(CO_Sdo, CoSdoReqDnBuf_BufCurrentPositionAfterTheEndButBufferEmpty) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;
  req.offset = 12u;
  req.size = 6u;
  req.nbyte = 0u;
  req.membuf->cur = req.membuf->begin + 13u;

  const void* ibuf = nullptr;

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(req.membuf->begin, ibuf);
}
