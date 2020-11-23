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

#include <lely/co/sdo.h>
#include <lely/co/type.h>
#include <lely/util/endian.h>
#include <lely/util/ustring.h>

#include "holder/array-init.hpp"
#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"

TEST_GROUP(CO_Sdo) {
#ifndef CO_SDO_REQ_MEMBUF_SIZE
#define CO_SDO_REQ_MEMBUF_SIZE 8u
#endif

  co_sdo_req req = CO_SDO_REQ_INIT(req);

  void CheckArrayIsZeroed(const void* const array, const size_t size) {
    assert(array);
    const char* const arr = static_cast<const char*>(array);
    for (size_t i = 0u; i < size; i++) CHECK_EQUAL('\0', arr[i]);
  }

  TEST_TEARDOWN() { co_sdo_req_fini(&req); }
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
// when: co_sdo_req_last()
// then: returns false
TEST(CO_Sdo, CoSdoReqLast_IsLast) { CHECK(co_sdo_req_last(&req)); }

// given: SDO request with offset + nbyte < size
// when: co_sdo_req_last()
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
  STRCMP_EQUAL("Unknown abort code", co_sdo_ac2str(0xFFFFFFFFu));
}

// given: SDO request and an empty buffer
// when: co_sdo_req_init()
// then: SDO request is initialized with expected values and given buffer
TEST(CO_Sdo, CoSdoReqInit) {
  co_sdo_req req_init;
  membuf mbuf = MEMBUF_INIT;

  co_sdo_req_init(&req_init, &mbuf);

  CHECK_EQUAL(0u, req_init.size);
  CHECK_EQUAL(0u, req_init.nbyte);
  CHECK_EQUAL(0u, req_init.offset);
  POINTERS_EQUAL(nullptr, req_init.buf);
  POINTERS_EQUAL(&mbuf, req_init.membuf);
#if LELY_NO_MALLOC
  CHECK(membuf_begin(req_init.membuf) != req_init._begin);
  CHECK_EQUAL(0u, membuf_capacity(req_init.membuf));
  CheckArrayIsZeroed(membuf_begin(req.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, membuf_begin(req_init.membuf));
#endif

  co_sdo_req_fini(&req_init);
}

// given: SDO request
// when: co_sdo_req_init()
// then: SDO request is initialized with expected values
TEST(CO_Sdo, CoSdoReqInit_BufNull) {
  co_sdo_req req_init;

  co_sdo_req_init(&req_init, nullptr);

  CHECK_EQUAL(0u, req_init.size);
  CHECK_EQUAL(0u, req_init.nbyte);
  CHECK_EQUAL(0u, req_init.offset);
  POINTERS_EQUAL(nullptr, req_init.buf);
  POINTERS_EQUAL(&req_init._membuf, req_init.membuf);
#if LELY_NO_MALLOC
  POINTERS_EQUAL(req_init._begin, membuf_begin(req_init.membuf));
  CHECK_EQUAL(CO_SDO_REQ_MEMBUF_SIZE, membuf_capacity(req_init.membuf));
  CheckArrayIsZeroed(membuf_begin(req_init.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, membuf_begin(req_init.membuf));
#endif
}

// given: SDO request
// when: CO_SDO_REQ_INIT()
// then: SDO request is initialized with expected values
TEST(CO_Sdo, CoSdoReqInit_Macro) {
  co_sdo_req req_init = CO_SDO_REQ_INIT(req_init);

  CHECK_EQUAL(0u, req_init.size);
  CHECK_EQUAL(0u, req_init.nbyte);
  CHECK_EQUAL(0u, req_init.offset);
  POINTERS_EQUAL(nullptr, req_init.buf);
  POINTERS_EQUAL(&req_init._membuf, req_init.membuf);
#if LELY_NO_MALLOC
  POINTERS_EQUAL(req_init._begin, membuf_begin(req_init.membuf));
  CHECK_EQUAL(CO_SDO_REQ_MEMBUF_SIZE, membuf_capacity(req_init.membuf));
  CheckArrayIsZeroed(membuf_begin(req_init.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, membuf_begin(req.membuf));
#endif

  co_sdo_req_fini(&req_init);
}

// given: SDO request
// when: co_sdo_req_clear()
// then: SDO request is cleared and set with expected values
TEST(CO_Sdo, CoSdoReqClear) {
  const size_t VAL_SIZE = 1u;
  char buf[VAL_SIZE] = {'X'};
  req.buf = buf;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;
  req.offset = VAL_SIZE;
#if !LELY_NO_MALLOC
  membuf mbuf = MEMBUF_INIT;
  req.membuf = &mbuf;
#endif
  membuf_init(req.membuf, buf, VAL_SIZE);

  co_sdo_req_clear(&req);

  CHECK_EQUAL(0u, req.size);
  CHECK_EQUAL(0u, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
  POINTERS_EQUAL(nullptr, req.buf);
#if !LELY_NO_MALLOC
  POINTERS_EQUAL(1u, membuf_capacity(req.membuf));
  POINTERS_EQUAL(buf, membuf_begin(req.membuf));
#endif
}

// given: invalid SDO request
// when: co_sdo_req_dn()
// then: error is returned (CO_SDO_AC_ERROR)
TEST(CO_Sdo, CoSdoReqDn_Error) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

// given: invalid SDO request
// when: co_sdo_req_dn()
// then: error is returned
TEST(CO_Sdo, CoSdoReqDn_ErrorNoAcPointer) {
  size_t nbyte = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, nullptr);

  CHECK_EQUAL(-1, ret);
}

// given: empty SDO request
// when: co_sdo_req_dn()
// then: success is returned
TEST(CO_Sdo, CoSdoReqDn_Empty) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
}

// given: SDO request
// when: co_sdo_req_dn()
// then: incomplete data code is returned
TEST(CO_Sdo, CoSdoReqDn_NotAllDataAvailable) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  const size_t VAL_SIZE = 3u;
  char buffer[VAL_SIZE] = {0x03, 0x04, 0x7F};
  req.buf = buffer;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE - 1u;
  req.offset = 0u;
  char internal_buffer[VAL_SIZE] = {0};
#if !LELY_NO_MALLOC
  membuf mbuf = MEMBUF_INIT;
  req.membuf = &mbuf;
#endif
  membuf_init(req.membuf, &internal_buffer, VAL_SIZE);

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(nullptr, ibuf);
  CHECK_EQUAL(0u, nbyte);
  CHECK_EQUAL(0x03, buffer[0]);
  CHECK_EQUAL(0x04, buffer[1]);
  CHECK_EQUAL(0x7F, buffer[2]);
  CHECK_EQUAL(0x03, internal_buffer[0]);
  CHECK_EQUAL(0x04, internal_buffer[1]);
  CHECK_EQUAL(0x00, internal_buffer[2]);
}

// given: SDO request with the whole value available right away
// when: co_sdo_req_dn()
// then: success is returned and no data copied to the internal memory buffer
TEST(CO_Sdo, CoSdoReqDn) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  const size_t VAL_SIZE = 3u;
  char buffer[VAL_SIZE] = {0x03, 0x04, 0x05};
  req.buf = buffer;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;
  req.offset = 0u;
#if !LELY_NO_MALLOC
  membuf mbuf = MEMBUF_INIT;
  req.membuf = &mbuf;
#endif
  char internal_buffer[VAL_SIZE] = {0};
  membuf_init(req.membuf, &internal_buffer, VAL_SIZE);

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(buffer, ibuf);
  CHECK_EQUAL(VAL_SIZE, nbyte);
  CHECK_EQUAL(0x03, buffer[0]);
  CHECK_EQUAL(0x04, buffer[1]);
  CHECK_EQUAL(0x05, buffer[2]);
}

#if LELY_NO_MALLOC
// given: invalid SDO request (too small buffer)
// when: co_sdo_req_dn_buf()
// then: error is returned (CO_SDO_AC_NO_MEM)
TEST(CO_Sdo, CoSdoReqDnBuf_NoMem) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  req.offset = 0u;
  req.size = CO_SDO_REQ_MEMBUF_SIZE + 1u;
  req.nbyte = 0u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
}
#else
// given: SDO request
// when: co_sdo_req_dn_buf()
// then: incomplete data code is returned
TEST(CO_Sdo, CoSdoReqDnBuf_ReallocateBuffer) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  req.offset = 0u;
  req.size = 5u;
  req.nbyte = 0u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, ac);
}
#endif  // LELY_NO_MALLOC

// given: SDO request with empty buffer
// when: co_sdo_req_dn_buf()
// then: success is returned and ibuf is equal begin of the membuf
TEST(CO_Sdo, CoSdoReqDnBuf_RequestWithNoDataButDataAlreadyTransferred) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  const size_t VAL_SIZE = 5u;
  req.offset = VAL_SIZE + 1u;
  req.size = VAL_SIZE;
  req.nbyte = 0u;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif
  membuf_seek(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(membuf_begin(req.membuf), ibuf);
}

// given: invalid SDO request (data exceeding internal buffer capacity)
// when: co_sdo_req_dn_buf()
// then: error is returned (CO_SDO_AC_ERROR) and ibuf is NULL
TEST(CO_Sdo, CoSdoReqDnBuf_DataExceedsBufferSize) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  req.offset = 5u;
  req.size = 6u;
  req.nbyte = 4u;
  membuf_seek(req.membuf, req.offset);

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
  POINTERS_EQUAL(nullptr, ibuf);
}

// given: SDO request
// when: co_sdo_req_dn_buf()
// then: success is returned
TEST(CO_Sdo, CoSdoReqDnBuf_OffsetEqualToMembufSize) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  req.offset = 1u;
  req.nbyte = 0u;
  req.size = 1u;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif
  membuf_seek(req.membuf, req.offset);

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
#if LELY_NO_MALLOC
  const char* const mbuf = reinterpret_cast<char*>(membuf_begin(req.membuf));
  CHECK(mbuf != nullptr);
  CheckArrayIsZeroed(mbuf, membuf_size(req.membuf));
#endif
}

// given: invalid SDO request (source not specified)
// when: co_sdo_req_dn_buf()
// then: success is returned
TEST(CO_Sdo, CoSdoReqDnBuf_EmptyRequest_NoBufferPointerNoNbytePointer) {
  co_unsigned32_t ac = 0u;

  const auto ret = co_sdo_req_dn(&req, nullptr, nullptr, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
#if LELY_NO_MALLOC
  const char* const mbuf = reinterpret_cast<char*>(membuf_begin(req.membuf));
  CHECK(mbuf != nullptr);
  CheckArrayIsZeroed(mbuf, membuf_size(req.membuf));
#endif
}

// given: invalid SDO request and a value to be downloaded
// when: co_sdo_req_dn_val()
// then: error is returned (CO_SDO_AC_ERROR)
TEST(CO_Sdo, CoSdoReqDnVal_InvalidRequest) {
  co_unsigned8_t val = 0xFFu;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

// given: SDO request
// when: co_sdo_req_dn_val()
// then: success is returned and a variable has a value specified by the buffer
TEST(CO_Sdo, CoSdoReqDnVal_BasicDataType) {
  co_unsigned16_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0u;

  const size_t VAL_SIZE = 2u;
  char buf[VAL_SIZE] = {0x4E, 0x7B};
  req.buf = buf;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(ldle_u16(reinterpret_cast<uint_least8_t*>(buf)), val);
}

// given: SDO request to download 4-bytes long variable to 2-byte variable
// when: co_sdo_req_dn_val()
// then: error is returned (CO_SDO_AC_TYPE_LEN_HI) but part of the variable was
//       downloaded
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooManyBytes) {
  co_unsigned16_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0u;

  const size_t INVALID_VAL_SIZE = 4u;
  char buf[INVALID_VAL_SIZE] = {0x12, 0x34, 0x56, 0x78};
  req.buf = buf;
  req.size = INVALID_VAL_SIZE;
  req.nbyte = INVALID_VAL_SIZE;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, ac);
  CHECK_EQUAL(ldle_u16(reinterpret_cast<uint_least8_t*>(buf)), val);
}

// given: SDO request to download 4-bytes long buffer to 8-byte variable
// when: co_sdo_req_dn_val()
// then: error is returned (CO_SDO_AC_TYPE_LEN_LO)
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooFewBytes) {
  co_unsigned64_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;
  co_unsigned32_t ac = 0u;

  const size_t INVALID_VAL_SIZE = 4u;
  char buf[INVALID_VAL_SIZE] = {0x7E, 0x7B, 0x34, 0x7B};
  req.buf = buf;
  req.size = INVALID_VAL_SIZE;
  req.nbyte = INVALID_VAL_SIZE;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, ac);
  CHECK_EQUAL(0u, val);
}

// given: SDO request to download 4-bytes long buffer to 8-byte variable
// when: co_sdo_req_dn_val()
// then: incomplete data code is returned
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooFewBytesNoAcPointer) {
  co_unsigned64_t val = 0u;
  co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;

  const size_t INVALID_VAL_SIZE = 4u;
  char buf[INVALID_VAL_SIZE] = {0x5E, 0x7B, 0x34, 0x3B};
  req.buf = buf;
  req.size = INVALID_VAL_SIZE;
  req.nbyte = INVALID_VAL_SIZE;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif

  const auto ret = co_sdo_req_dn_val(&req, type, &val, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, val);
}

#if HAVE_LELY_OVERRIDE
// given: SDO request
// when: co_sdo_req_dn_val()
// then: error is returned (CO_SDO_AC_NO_MEM)
TEST(CO_Sdo, CoSdoReqDnVal_ArrayDataType_ReadValueFailed) {
  co_unsigned16_t type = CO_DEFTYPE_OCTET_STRING;
  co_unsigned32_t ac = 0u;

  const size_t VAL_SIZE = 4u;
  char buf[VAL_SIZE] = {0x01, 0x01, 0x00, 0x2B};
  req.buf = buf;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;
  LelyOverride::co_val_read(0);
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif

  CoArrays arrays;
  co_octet_string_t str = arrays.Init<co_octet_string_t>();

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
#if LELY_NO_MALLOC
  const uint_least8_t EXPECTED[VAL_SIZE] = {0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0, memcmp(EXPECTED, str, 4u));
  CheckArrayIsZeroed(membuf_begin(req.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, str);
#endif
}
#endif  // HAVE_LELY_OVERRIDE

// given: SDO request to download 4 bytes to an array, "FF" unicode string
// when: co_sdo_req_up_val()
// then: success is returned
TEST(CO_Sdo, CoSdoReqDnVal_ArrayDataType) {
  co_unsigned16_t type = CO_DEFTYPE_OCTET_STRING;
  co_unsigned32_t ac = 0u;
  const size_t VAL_SIZE = 4u;
  uint_least8_t buf[VAL_SIZE] = {0x01u, 0x01u, 0x2Bu, 0x00u};
  req.buf = buf;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;
  req.offset = 0;
  CoArrays arrays;
  co_octet_string_t str = arrays.Init<co_octet_string_t>();

  const auto ret = co_sdo_req_dn_val(&req, type, &str, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(VAL_SIZE, req.size);
  CHECK_EQUAL(VAL_SIZE, req.offset + req.nbyte);
  const uint_least8_t EXPECTED[VAL_SIZE] = {0x01u, 0x01u, 0x2Bu, 0x00u};
  CHECK_EQUAL(0, memcmp(EXPECTED, str, VAL_SIZE));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &str);
}

// given: empty SDO request and a buffer with value to be uploaded
// when: co_sdo_req_up()
// then: request is initialized with given values
TEST(CO_Sdo, CoSdoReqUp) {
  co_sdo_req req_up = CO_SDO_REQ_INIT(req_up);
  const size_t VAL_SIZE = 2u;
  char src_buf[VAL_SIZE] = {0x34, 0x56};

  co_sdo_req_up(&req_up, src_buf, 2u);

  POINTERS_EQUAL(src_buf, req_up.buf);
  CHECK_EQUAL(VAL_SIZE, req_up.size);
  CHECK_EQUAL(VAL_SIZE, req_up.nbyte);
  CHECK_EQUAL(0, req_up.offset);
}

#if HAVE_LELY_OVERRIDE
// given: empty SDO request and a value
// when: co_sdo_req_up_val()
// then: success is returned
TEST(CO_Sdo, CoSdoReqUpVal_NoValueWrite) {
  const co_unsigned16_t val = 0x4B7Du;
  co_unsigned32_t ac = 0u;
  LelyOverride::co_val_write(0);
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(0, req.size);
  CHECK_EQUAL(0, req.offset);
  CHECK_EQUAL(0, req.nbyte);
}
#endif  // HAVE_LELY_OVERRIDE

#if LELY_NO_MALLOC
// given: empty SDO request and a value
// when: co_sdo_req_up_val()
// then: error is returned (CO_SDO_AC_NO_MEM)
TEST(CO_Sdo, CoSdoReqUpVal_NoMemory) {
  const co_unsigned16_t val = 0x7A79u;
  co_unsigned32_t ac = 0u;
  membuf_init(req.membuf, nullptr, 0u);

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
  POINTERS_EQUAL(nullptr, membuf_begin(req.membuf));
}
#endif  // LELY_NO_MALLOC

#if LELY_NO_MALLOC
// given: empty SDO request and a value
// when: co_sdo_req_up_val()
// then: error is returned
TEST(CO_Sdo, CoSdoReqUpVal_NoMemoryNoAcPointer) {
  const co_unsigned16_t val = 0x797Au;
  membuf_init(req.membuf, nullptr, 0u);

  const auto ret =
      co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, nullptr);

  CHECK_EQUAL(-1, ret);
  POINTERS_EQUAL(nullptr, membuf_begin(req.membuf));
}
#endif  // LELY_NO_MALLOC

#if HAVE_LELY_OVERRIDE
// given: SDO request and a value to upload
// when: co_sdo_req_up_val()
// then: error is returned (CO_SDO_AC_ERROR)
TEST(CO_Sdo, CoSdoReqUpVal_SecondCoValWriteFail) {
  const co_unsigned16_t val = 0x7A79u;
  co_unsigned32_t ac = 0u;
  LelyOverride::co_val_write(1);
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
#endif

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}
#endif  // HAVE_LELY_OVERRIDE

// given: SDO request and a value to upload
// when: co_sdo_req_up_val()
// then: 0 is returned, buffer contains suitable bytes
TEST(CO_Sdo, CoSdoReqUpVal) {
  const co_unsigned16_t val = 0x797Au;
  const size_t VAL_SIZE = sizeof(val);
  co_unsigned32_t ac = 0u;

  const size_t BUF_SIZE = CO_SDO_REQ_MEMBUF_SIZE;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, BUF_SIZE);
#else
  char buf[BUF_SIZE] = {0};
  membuf_init(req.membuf, buf, BUF_SIZE);
#endif

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  const char* const mbuf = static_cast<char*>(membuf_begin(req.membuf));
  uint_least8_t val_buffer[VAL_SIZE] = {0};
  stle_u16(val_buffer, val);
  CHECK_EQUAL(val_buffer[0], mbuf[0]);
  CHECK_EQUAL(val_buffer[1], mbuf[1]);
  POINTERS_EQUAL(mbuf, req.buf);
  CHECK_EQUAL(VAL_SIZE, req.size);
  CHECK_EQUAL(VAL_SIZE, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
}
