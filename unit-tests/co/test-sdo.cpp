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

#include <libtest/override/lelyco-val.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/array-init.hpp"

#ifndef CO_SDO_REQ_MEMBUF_SIZE
#define CO_SDO_REQ_MEMBUF_SIZE 8u
#endif

TEST_GROUP(CO_Sdo) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);

  void CheckArrayIsZeroed(const void* const array, const size_t size) const {
    assert(array);
    const auto arr = static_cast<const char*>(array);
    for (size_t i = 0u; i < size; i++) CHECK_EQUAL('\0', arr[i]);
  }

  TEST_TEARDOWN() { co_sdo_req_fini(&req); }
};

/// @name co_sdo_req_first()
///@{

/// \Given SDO request with offset 0
///
/// \When calling co_sdo_req_first() for that request
///
/// \Then the call returns true
TEST(CO_Sdo, CoSdoReqFirst_IsFirst) {
  req.offset = 0u;

  CHECK(co_sdo_req_first(&req));
}

/// \Given SDO request with offset 1
///
/// \When calling co_sdo_req_first() for that request
///
/// \Then the call returns false
TEST(CO_Sdo, CoSdoReqFirst_IsNotFirst) {
  req.offset = 1u;

  CHECK(!co_sdo_req_first(&req));
}

///@}

/// @name co_sdo_req_last()
///@{

/// \Given SDO request with offset + nbyte == size
///
/// \When calling co_sdo_req_last() for that request
///
/// \Then the call returns true
TEST(CO_Sdo, CoSdoReqLast_IsLast) {
  req.offset = 30u;
  req.nbyte = 14u;
  req.size = 44u;

  CHECK(co_sdo_req_last(&req));
}

/// \Given SDO request with offset + nbyte < size
///
/// \When calling co_sdo_req_last() for that request
///
/// \Then the call returns false
TEST(CO_Sdo, CoSdoReqLast_IsNotLast) {
  req.offset = 0u;
  req.nbyte = 14u;
  req.size = 44u;

  CHECK(!co_sdo_req_last(&req));
}

///@}

/// @name co_sdo_ac2str()
///@{

/// \Given SDO abort code (AC)
///
/// \When calling co_sdo_ac2str() for that code
///
/// \Then a string describing the AC is returned
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

///@}

/// @name co_sdo_req_init()
///@{

/// \Given SDO request and an empty memory buffer (#membuf)
///
/// \When calling co_sdo_req_init() with the request and the buffer
///
/// \Then SDO request is initialized with zeroes and given buffer
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
  CHECK(membuf_begin(req_init.membuf) != req_init.begin_);
  CHECK_EQUAL(0u, membuf_capacity(req_init.membuf));
  CheckArrayIsZeroed(membuf_begin(req.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, membuf_begin(req_init.membuf));
#endif

  co_sdo_req_fini(&req_init);
}

/// \Given SDO request
///
/// \When calling co_sdo_req_init() with the request and no buffer (NULL)
///
/// \Then SDO request is initialized with expected values
TEST(CO_Sdo, CoSdoReqInit_BufNull) {
  co_sdo_req req_init;

  co_sdo_req_init(&req_init, nullptr);

  CHECK_EQUAL(0u, req_init.size);
  CHECK_EQUAL(0u, req_init.nbyte);
  CHECK_EQUAL(0u, req_init.offset);
  POINTERS_EQUAL(nullptr, req_init.buf);
  POINTERS_EQUAL(&req_init.membuf_, req_init.membuf);
#if LELY_NO_MALLOC
  POINTERS_EQUAL(req_init.begin_, membuf_begin(req_init.membuf));
  CHECK_EQUAL(CO_SDO_REQ_MEMBUF_SIZE, membuf_capacity(req_init.membuf));
  CheckArrayIsZeroed(membuf_begin(req_init.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, membuf_begin(req_init.membuf));
#endif
}

/// \Given SDO request
///
/// \When initializing it using CO_SDO_REQ_INIT() macro
///
/// \Then SDO request is initialized with expected values
TEST(CO_Sdo, CoSdoReqInit_Macro) {
  co_sdo_req req_init = CO_SDO_REQ_INIT(req_init);

  CHECK_EQUAL(0u, req_init.size);
  CHECK_EQUAL(0u, req_init.nbyte);
  CHECK_EQUAL(0u, req_init.offset);
  POINTERS_EQUAL(nullptr, req_init.buf);
  POINTERS_EQUAL(&req_init.membuf_, req_init.membuf);
#if LELY_NO_MALLOC
  POINTERS_EQUAL(req_init.begin_, membuf_begin(req_init.membuf));
  CHECK_EQUAL(CO_SDO_REQ_MEMBUF_SIZE, membuf_capacity(req_init.membuf));
  CheckArrayIsZeroed(membuf_begin(req_init.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, membuf_begin(req.membuf));
#endif

  co_sdo_req_fini(&req_init);
}

///@}

/// @name co_sdo_req_clear()
///@{

/// \Given SDO request with any contents
///
/// \When calling co_sdo_req_clear() for that request
///
/// \Then SDO request is cleared and reset with default values
///       \Calls membuf_clear()
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

///@}

/// @name co_sdo_req_dn()
///@{

/// \Given malformed SDO request
///
/// \When calling co_sdo_req_dn() for that request with no pointer to store
///       first byte address, pointer to store number of bytes and pointer to
///       store abort code
///
/// \Then the call returns -1 and abort code is set to #CO_SDO_AC_ERROR
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_Error) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

/// \Given malformed SDO request
///
/// \When calling co_sdo_req_dn() for that request with no pointer to store
///       first byte address, pointer to store number of bytes and no pointer to
///       store abort code
///
/// \Then the call returns -1
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_ErrorNoAcPointer) {
  size_t nbyte = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, nullptr);

  CHECK_EQUAL(-1, ret);
}

/// \Given empty SDO request
///
/// \When calling co_sdo_req_dn() for that request with no pointer to store
///       first byte address, pointer to store number of bytes and pointer to
///       store abort code
///
/// \Then the call returns 0 and abort code is set not modified
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_Empty) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 42u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(42u, ac);
}

/// \Given SDO request with incomplete data
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, pointer to store number of bytes and pointer to store
///       abort code
///
/// \Then the call returns -1, abort code is set to 0, first byte address is set
///       to NULL and available data is copied to memory buffer
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_NotAllDataAvailable) {
  size_t nbyte = 0;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  const size_t VAL_SIZE = 3u;
  const uint8_t buffer[VAL_SIZE] = {0x03u, 0x04u, 0x7Fu};
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

/// \Given SDO request with complete data
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, pointer to store number of bytes and pointer to store
///       abort code
///
/// \Then the call returns 0, abort code is set to 0, first byte address is set
///       to address of the buffer and available data is copied to memory
///       buffer; number of bytes is set to complete count of bytes
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  const size_t VAL_SIZE = 3u;
  const uint8_t buffer[VAL_SIZE] = {0x03u, 0x04u, 0x05u};
  req.buf = buffer;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;
  req.offset = 0u;
#if !LELY_NO_MALLOC
  membuf mbuf = MEMBUF_INIT;
  req.membuf = &mbuf;
#endif
  uint8_t internal_buffer[VAL_SIZE] = {0u};
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

/// \Given SDO request with complete data
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, no pointer to store number of bytes and pointer to store
///       abort code
///
/// \Then the call returns 0, abort code is set to 0, first byte address is set
///       to address of the buffer and available data is copied to memory buffer
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_NoNBytePointer) {
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  const size_t VAL_SIZE = 3u;
  const uint8_t buffer[VAL_SIZE] = {0x03u, 0x04u, 0x05u};
  req.buf = buffer;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;
  req.offset = 0u;
#if !LELY_NO_MALLOC
  membuf mbuf = MEMBUF_INIT;
  req.membuf = &mbuf;
#endif
  uint8_t internal_buffer[VAL_SIZE] = {0u};
  membuf_init(req.membuf, &internal_buffer, VAL_SIZE);

  const auto ret = co_sdo_req_dn(&req, &ibuf, nullptr, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(buffer, ibuf);
  CHECK_EQUAL(0x03, buffer[0]);
  CHECK_EQUAL(0x04, buffer[1]);
  CHECK_EQUAL(0x05, buffer[2]);
}

#if LELY_NO_MALLOC
/// \Given SDO request with request size larger than attached buffer
///
/// \When calling co_sdo_req_dn() for that request with no pointer to store
///       first byte address, pointer to store number of bytes and pointer to
///       store abort code
///
/// \Then call returns -1 and abort code is set to #CO_SDO_AC_NO_MEM
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_NoMem) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  req.offset = 0u;
  req.size = CO_SDO_REQ_MEMBUF_SIZE + 1u;
  req.nbyte = 0u;

  const auto ret = co_sdo_req_dn(&req, nullptr, &nbyte, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ac);
}
#endif  // LELY_NO_MALLOC

/// \Given SDO request with no new data in the buffer, but complete data was
///        already processed
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, pointer to store number of bytes and pointer to store
///       abort code
///
/// \Then call returns 0, abort code is 0 and pointer to first byte is set to
///       beginning of the request buffer
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_RequestWithNoDataButDataAlreadyTransferred) {
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
  CHECK_EQUAL(VAL_SIZE, nbyte);
  POINTERS_EQUAL(membuf_begin(req.membuf), ibuf);
}

/// \Given SDO request with new data in the buffer, but complete data size
///        exceeds expected one
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, no pointer to store number of bytes and pointer to store
///       abort code
///
/// \Then call returns -1, abort code is set to #CO_SDO_AC_ERROR and pointer to
///       first byte is set to NULL
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_DataExceedsBufferSize) {
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  req.offset = 5u;
  req.size = 6u;
  req.nbyte = 4u;
  membuf_seek(req.membuf, static_cast<ptrdiff_t>(req.offset));

  const auto ret = co_sdo_req_dn(&req, &ibuf, nullptr, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
  POINTERS_EQUAL(nullptr, ibuf);
}

/// \Given SDO request with new data in the buffer, but complete data size
///        exceeds expected one
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, pointer to store number of bytes and no pointer to store
///       abort code
///
/// \Then call returns -1 and pointer to first byte is set to NULL
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_NoAcPointer) {
  size_t nbyte = 0u;
  const void* ibuf = nullptr;

  req.offset = 5u;
  req.size = 6u;
  req.nbyte = 4u;
  membuf_seek(req.membuf, static_cast<ptrdiff_t>(req.offset));

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, nullptr);

  CHECK_EQUAL(-1, ret);
  POINTERS_EQUAL(nullptr, ibuf);
}

/// \Given correct "last" SDO request
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, pointer to store number of bytes and pointer to store
///       abort code
///
/// \Then call returns 0, abort code is 0 and pointer to first byte is set to
///       request's buffer; data is copied to the buffer and number of available
///       bytes is set to request size
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_OffsetEqualToMembufSize) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;
  const uint_least8_t data = 0x44u;

  req.offset = 1u;
  req.nbyte = 1u;
  req.size = 2u;
  req.buf = &data;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, CO_SDO_REQ_MEMBUF_SIZE);
  memset(membuf_begin(req.membuf), 0, membuf_capacity(req.membuf));
#endif
  membuf_seek(req.membuf, static_cast<ptrdiff_t>(req.offset));

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  POINTERS_EQUAL(membuf_begin(req.membuf), ibuf);
  CHECK_EQUAL(req.size, nbyte);
  const uint_least8_t expected[] = {0x00u, data};
  MEMCMP_EQUAL(expected, ibuf, nbyte);
}

/// \Given empty SDO request (size equal zero, no pointer to data)
///
/// \When calling co_sdo_req_dn() for that request with pointer to store first
///       byte address, pointer to store number of bytes and pointer to store
///       abort code
///
/// \Then call returns 0, abort code is 0 and pointer to first byte is set to
///       NULL; number of available bytes is set to 0
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_capacity()
///       \Calls membuf_seek()
TEST(CO_Sdo, CoSdoReqDn_EmptyRequest) {
  size_t nbyte = 0u;
  co_unsigned32_t ac = 0u;
  const void* ibuf = nullptr;

  const auto ret = co_sdo_req_dn(&req, &ibuf, &nbyte, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(0u, nbyte);
  POINTERS_EQUAL(nullptr, ibuf);
}

///@}

/// @name co_sdo_req_dn_val()
///@{

/// \Given invalid SDO request
///
/// \When calling co_sdo_req_dn_val() for that request with any value type, any
///       value pointer and pointer to store abort code
///
/// \Then call returns -1 and abort code is set to #CO_SDO_AC_ERROR
///       \Calls co_sdo_req_dn()
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_val_fini()
TEST(CO_Sdo, CoSdoReqDnVal_InvalidRequest) {
  co_unsigned8_t val = 0xFFu;
  const co_unsigned16_t type = CO_DEFTYPE_UNSIGNED8;
  co_unsigned32_t ac = 0u;
  req.offset = 1u;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_SDO_AC_ERROR, ac);
}

/// \Given SDO request containing correct value of a selected type
///
/// \When calling co_sdo_req_dn_val() for that request with matching value type,
///       correct value pointer and pointer to store abort code
///
/// \Then call returns 0, abort code is zero and correct value is read
///       \Calls co_sdo_req_dn()
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_val_fini()
TEST(CO_Sdo, CoSdoReqDnVal_BasicDataType) {
  co_unsigned16_t val = 0u;
  const co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0u;

  const size_t VAL_SIZE = 2u;
  uint8_t buf[VAL_SIZE] = {0x4Eu, 0x7Bu};
  req.buf = buf;
  req.size = VAL_SIZE;
  req.nbyte = VAL_SIZE;

  const auto ret = co_sdo_req_dn_val(&req, type, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  CHECK_EQUAL(ldle_u16(reinterpret_cast<uint_least8_t*>(buf)), val);
}

/// \Given SDO request containing too many bytes for the selected value type
///
/// \When calling co_sdo_req_dn_val() for that request with the selected value
///       type, correct value pointer and pointer to store abort code
///
/// \Then call returns -1, abort code is set to #CO_SDO_AC_TYPE_LEN_HI and the
///       value is read
///       \Calls co_sdo_req_dn()
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_val_fini()
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooManyBytes) {
  co_unsigned16_t val = 0u;
  const co_unsigned16_t type = CO_DEFTYPE_UNSIGNED16;
  co_unsigned32_t ac = 0u;

  const size_t INVALID_VAL_SIZE = 4u;
  uint8_t buf[INVALID_VAL_SIZE] = {0x12u, 0x34u, 0x56u, 0x78u};
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

/// \Given SDO request containing too few bytes for the selected value type
///
/// \When calling co_sdo_req_dn_val() for that request with the selected value
///       type, correct value pointer and pointer to store abort code
///
/// \Then call returns -1, abort code is set to #CO_SDO_AC_TYPE_LEN_LO and the
///       value is not read
///       \Calls co_sdo_req_dn()
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_val_fini()
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooFewBytes) {
  co_unsigned64_t val = 0u;
  const co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;
  co_unsigned32_t ac = 0u;

  const size_t INVALID_VAL_SIZE = 4u;
  const uint8_t buf[INVALID_VAL_SIZE] = {0x7Eu, 0x7Bu, 0x34u, 0x7Bu};
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

/// \Given SDO request containing too few bytes for the selected value type
///
/// \When calling co_sdo_req_dn_val() for that request with the selected value
///       type, correct value pointer and no pointer to store abort code
///
/// \Then call returns -1 and the value is not read
///       \Calls co_sdo_req_dn()
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_val_fini()
TEST(CO_Sdo, CoSdoReqDnVal_DownloadTooFewBytesNoAcPointer) {
  co_unsigned64_t val = 0u;
  const co_unsigned16_t type = CO_DEFTYPE_UNSIGNED64;

  const size_t INVALID_VAL_SIZE = 4u;
  const uint8_t buf[INVALID_VAL_SIZE] = {0x5Eu, 0x7Bu, 0x34u, 0x3Bu};
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
/// \Given SDO request containing bytes of an array which will cause failure of
///        co_val_read()
///
/// \When calling co_sdo_req_dn_val() for that request with the selected value
///       type, correct value pointer and no pointer to store abort code
///
/// \Then call returns -1, abort code is set to #CO_SDO_AC_NO_MEM and array is
///       not read
///       \Calls co_sdo_req_dn()
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_val_fini()
TEST(CO_Sdo, CoSdoReqDnVal_ArrayDataType_ReadValueFailed) {
  const co_unsigned16_t type = CO_DEFTYPE_OCTET_STRING;
  co_unsigned32_t ac = 0u;

  const size_t VAL_SIZE = 4u;
  const uint8_t buf[VAL_SIZE] = {0x01u, 0x01u, 0x00u, 0x2Bu};
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
  MEMORY_IS_ZEROED(str, VAL_SIZE);
  CheckArrayIsZeroed(membuf_begin(req.membuf), CO_SDO_REQ_MEMBUF_SIZE);
#else
  POINTERS_EQUAL(nullptr, str);
#endif
}
#endif  // HAVE_LELY_OVERRIDE

/// \Given SDO request containing bytes of an array
///
/// \When calling co_sdo_req_dn_val() for that request with the selected value
///       type, correct value pointer and no pointer to store abort code
///
/// \Then call returns 0, abort code is 0 and array is read
///       \Calls co_sdo_req_dn()
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_val_fini()
TEST(CO_Sdo, CoSdoReqDnVal_ArrayDataType) {
  const co_unsigned16_t type = CO_DEFTYPE_OCTET_STRING;
  co_unsigned32_t ac = 0u;
  const size_t VAL_SIZE = 4u;
  const uint_least8_t buf[VAL_SIZE] = {0x01u, 0x01u, 0x2Bu, 0x00u};
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
  MEMCMP_EQUAL(EXPECTED, str, VAL_SIZE);

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &str);
}

///@}

/// @name co_sdo_req_up()
///@{

/// \Given an empty SDO request and a buffer with value to be uploaded
///
/// \When calling co_sdo_req_up() for that request and buffer
///
/// \Then correct upload request for the given value is prepared
TEST(CO_Sdo, CoSdoReqUp) {
  co_sdo_req req_up = CO_SDO_REQ_INIT(req_up);
  const size_t VAL_SIZE = 2u;
  const uint8_t src_buf[VAL_SIZE] = {0x34u, 0x56u};

  co_sdo_req_up(&req_up, src_buf, sizeof(src_buf));

  POINTERS_EQUAL(src_buf, req_up.buf);
  CHECK_EQUAL(VAL_SIZE, req_up.size);
  CHECK_EQUAL(VAL_SIZE, req_up.nbyte);
  CHECK_EQUAL(0, req_up.offset);
}

///@}

/// @name co_sdo_req_up_val()
///@{

#if HAVE_LELY_OVERRIDE
/// \Given an empty SDO request and a value that fails to be processed using
///        co_val_write()
///
/// \When calling co_sdo_req_up_val() for that request, matching value type, the
///       value and pointer to store abort code
///
/// \Then the call returns 0, abort code is zero and request remains empty
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
/// \Given an empty SDO request with buffer with no capacity
///
/// \When calling co_sdo_req_up_val() for that request, any value type, any
///       matching value and pointer to store abort code
///
/// \Then the call returns -1 and abort code is set to #CO_SDO_AC_NO_MEM
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
/// \Given an empty SDO request with buffer with no capacity
///
/// \When calling co_sdo_req_up_val() for that request, any value type, any
///       matching value and no pointer to store abort code
///
/// \Then the call returns -1
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
/// \Given an empty SDO request and a value that fails to be serialized using
///        co_val_write()
///
/// \When calling co_sdo_req_up_val() for that request, matching value type, the
///       value and pointer to store abort code
///
/// \Then the call returns -1 and abort code is set to #CO_SDO_AC_ERROR
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

/// \Given an empty SDO request and any value
///
/// \When calling co_sdo_req_up_val() for that request, matching value type, the
///       value and pointer to store abort code
///
/// \Then the call returns 0, abort code is 0 and request is properly filled
///       with the value
TEST(CO_Sdo, CoSdoReqUpVal) {
  const co_unsigned16_t val = 0x797Au;
  const size_t VAL_SIZE = sizeof(val);
  co_unsigned32_t ac = 0u;

  const size_t BUF_SIZE = CO_SDO_REQ_MEMBUF_SIZE;
#if !LELY_NO_MALLOC
  membuf_reserve(req.membuf, BUF_SIZE);
#else
  uint8_t buf[BUF_SIZE] = {0u};
  membuf_init(req.membuf, buf, BUF_SIZE);
#endif

  const auto ret = co_sdo_req_up_val(&req, CO_DEFTYPE_UNSIGNED16, &val, &ac);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, ac);
  const char* const mbuf = static_cast<char*>(membuf_begin(req.membuf));
  uint8_t val_buffer[VAL_SIZE] = {0u};
  stle_u16(val_buffer, val);
  CHECK_EQUAL(val_buffer[0], mbuf[0]);
  CHECK_EQUAL(val_buffer[1], mbuf[1]);
  POINTERS_EQUAL(mbuf, req.buf);
  CHECK_EQUAL(VAL_SIZE, req.size);
  CHECK_EQUAL(VAL_SIZE, req.nbyte);
  CHECK_EQUAL(0u, req.offset);
}

///@}

/// @name SDO_PAR_INIT
///@{

/// \Given N/A
///
/// \When initializing #co_sdo_par using #CO_SDO_PAR_INIT macro
///
/// \Then the initialized structure is filled with the correct default values
TEST(CO_Sdo, CoSdoParInit) {
  const co_sdo_par item = CO_SDO_PAR_INIT;

  CHECK_EQUAL(3u, item.n);
  CHECK_EQUAL(CO_SDO_COBID_VALID, item.cobid_req);
  CHECK_EQUAL(CO_SDO_COBID_VALID, item.cobid_res);
  CHECK_EQUAL(0u, item.id);
}

///@}
