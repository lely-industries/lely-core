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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <vector>

#include <CppUTest/TestHarness.h>

#include <lely/co/csdo.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/type.h>
#include <lely/co/val.h>
#include <lely/compat/string.h>
#include <lely/util/endian.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/override/lelyutil-membuf.hpp>
#include <libtest/override/lelyco-val.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/sdo-consts.hpp>
#include <libtest/tools/sdo-create-message.hpp>
#include <libtest/tools/sdo-init-expected-data.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/array-init.hpp"

/// Concise DCF buffer builder.
class ConciseDcf {
 private:
  explicit ConciseDcf(std::initializer_list<size_t> type_sizes) {
    const size_t size = std::accumulate(
        std::begin(type_sizes), std::end(type_sizes), sizeof(co_unsigned32_t),
        [](const size_t& a, const size_t& b) { return a + EntrySize(b); });
    buffer.assign(size, 0);
  }

 public:
  template <typename... types>
  static ConciseDcf
  MakeForEntries() {
    return ConciseDcf{sizeof(types)...};
  }

  uint_least8_t*
  Begin() {
    return buffer.data();
  }

  uint_least8_t*
  End() {
    return buffer.data() + buffer.size();
  }

  size_t
  Size() const {
    return buffer.size();
  }

 private:
  std::vector<uint_least8_t> buffer;

  static constexpr size_t
  EntrySize(const size_t type_size) {
    return sizeof(co_unsigned16_t)    // index
           + sizeof(co_unsigned8_t)   // subidx
           + sizeof(co_unsigned32_t)  // data size of parameter
           + type_size;
  }
};

TEST_GROUP(CO_CsdoInit) {
  const co_unsigned8_t CSDO_NUM = 0x01u;
  static const co_unsigned8_t DEV_ID = 0x01u;
  co_csdo_t* csdo = nullptr;
  co_dev_t* dev = nullptr;
  can_net_t* failing_net = nullptr;
  can_net_t* net = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1280;
  Allocators::Default defaultAllocator;
  Allocators::Limited limitedAllocator;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(defaultAllocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    limitedAllocator.LimitAllocationTo(can_net_sizeof());
    failing_net = can_net_create(limitedAllocator.ToAllocT(), 0);
    CHECK(failing_net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() {
    dev_holder.reset();
    can_net_destroy(net);
    can_net_destroy(failing_net);

#if HAVE_LELY_OVERRIDE
    LelyOverride::membuf_reserve(Override::AllCallsValid);
    LelyOverride::co_val_write(Override::AllCallsValid);
#endif  // HAVE_LELY_OVERRIDE
  }
};

/// @name co_csdo_alignof()
///@{

/// \Given N/A
///
/// \When co_csdo_alignof() is called
///
/// \Then if \__MINGW32__ and !__MINGW64__, 4 is returned; else 8 is returned
TEST(CO_CsdoInit, CoCsdoAlignof_Nominal) {
  const auto ret = co_csdo_alignof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(4u, ret);
#else
  CHECK_EQUAL(8u, ret);
#endif
}

///@}

/// @name co_csdo_sizeof()
///@{

/// \Given N/A
///
/// \When co_csdo_sizeof() is called
///
/// \Then if LELY_NO_MALLOC or \__MINGW64__: 256 is returned;
///       else if \__MINGW32__ and !__MINGW64__: 144 is returned;
///       else: 248 is returned
TEST(CO_CsdoInit, CoCsdoSizeof_Nominal) {
  const auto ret = co_csdo_sizeof();

#if defined(LELY_NO_MALLOC) || defined(__MINGW64__)
  CHECK_EQUAL(256u, ret);
#else
#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(144u, ret);
#else
  CHECK_EQUAL(248u, ret);
#endif
#endif  // LELY_NO_MALLOC
}

///@}

/// @name co_csdo_create()
///@{

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and a CSDO number,
///       but CSDO allocation fails
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_csdo_alignof()
///       \Calls co_csdo_sizeof()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_CsdoInit, CoCsdoCreate_FailCsdoAlloc) {
  const co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number equal zero
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_csdo_alignof()
///       \Calls co_csdo_sizeof()
///       \Calls errnum2c()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls co_csdo_get_alloc()
///       \Calls set_errc()
TEST(CO_CsdoInit, CoCsdoCreate_NumZero) {
  const co_unsigned8_t CSDO_NUM = 0;

  const co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number higher than CO_NUM_SDOS
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_csdo_alignof()
///       \Calls co_csdo_sizeof()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls co_csdo_get_alloc()
///       \Calls set_errc()
TEST(CO_CsdoInit, CoCsdoCreate_NumTooHigh) {
  const co_unsigned8_t CSDO_NUM = CO_NUM_SDOS + 1u;

  const co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to the device (co_dev_t) containing object 0x1280 in the
///        object dictionary
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number
///
/// \Then a non-null pointer is returned, default values are set
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_csdo_alignof()
///       \Calls co_csdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
///       \Calls co_csdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls co_csdo_get_alloc()
///       \Calls can_timer_set_func()
///       \Calls membuf_init()
///       \IfCalls{!LELY_NO_MALLOC, membuf_init()}
TEST(CO_CsdoInit, CoCsdoCreate_WithObj1280) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);

  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  CHECK(csdo != nullptr);
  POINTERS_EQUAL(dev, co_csdo_get_dev(csdo));
  POINTERS_EQUAL(net, co_csdo_get_net(csdo));
  CHECK_EQUAL(CSDO_NUM, co_csdo_get_num(csdo));
  POINTERS_EQUAL(can_net_get_alloc(net), co_csdo_get_alloc(csdo));
  const co_sdo_par* const par = co_csdo_get_par(csdo);
  CHECK_EQUAL(3u, par->n);
  CHECK_EQUAL(DEV_ID, par->id);
  CHECK_EQUAL(0x580u + CSDO_NUM, par->cobid_res);
  CHECK_EQUAL(0x600u + CSDO_NUM, par->cobid_req);

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the device (co_dev_t) without server parameter object in
///        the object dictionary
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_csdo_alignof()
///       \Calls co_csdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls co_csdo_get_alloc()
///       \Calls set_errc()
TEST(CO_CsdoInit, CoCsdoCreate_NoServerParameterObj) {
  const co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to the device (co_dev_t) containing object 0x1280 in
///        the object dictionary
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and a CSDO number,
///       but can_recv_create() fails
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_csdo_alignof()
///       \Calls co_csdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
///       \Calls co_csdo_get_alloc()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls co_csdo_get_alloc()
///       \Calls set_errc()
TEST(CO_CsdoInit, CoCsdoCreate_RecvCreateFail) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);

  limitedAllocator.LimitAllocationTo(co_csdo_sizeof());
  const co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

/// \Given a pointer to the device (co_dev_t) containing object 0x1280 in
///        the object dictionary
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and a CSDO number,
///       but can_timer_create() fails
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_csdo_alignof()
///       \Calls co_csdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
///       \Calls co_csdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls co_csdo_get_alloc()
///       \Calls get_errc()
///       \Calls can_recv_destroy()
///       \Calls set_errc()
///       \Calls mem_free()
///       \Calls co_csdo_get_alloc()
TEST(CO_CsdoInit, CoCsdoCreate_TimerCreateFail) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);

  limitedAllocator.LimitAllocationTo(co_csdo_sizeof() + can_recv_sizeof());
  const co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

///@}

/// @name co_csdo_destroy()
///@{

/// \Given a null CSDO service pointer (co_csdo_t)
///
/// \When co_csdo_destroy() is called
///
/// \Then nothing is changed
TEST(CO_CsdoInit, CoCsdoDestroy_Nullptr) {
  co_csdo_t* const csdo = nullptr;

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_destroy() is called
///
/// \Then the CSDO is destroyed
///       \Calls co_csdo_stop()
///       \Calls membuf_fini()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_csdo_get_alloc()
TEST(CO_CsdoInit, CoCsdoDestroy_Nominal) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK(csdo != nullptr);

  co_csdo_destroy(csdo);
}

///@}

/// @name co_csdo_start()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_start() is called
///
/// \Then the service is not stopped, the service is idle
///       \Calls co_csdo_is_stopped()
///       \Calls co_csdo_abort_req()
///       \Calls co_csdo_is_valid()
///       \Calls can_recv_start()
TEST(CO_CsdoInit, CoCsdoStart_NoDev) {
  co_csdo_t* const csdo = co_csdo_create(net, nullptr, CSDO_NUM);
  CHECK(csdo != nullptr);

  co_csdo_start(csdo);

  CHECK_FALSE(co_csdo_is_stopped(csdo));
  CHECK(co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_start() is called
///
/// \Then the service is not stopped, the service is idle
///       \Calls co_csdo_is_stopped()
TEST(CO_CsdoInit, CoCsdoStart_AlreadyStarted) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  co_csdo_start(csdo);

  co_csdo_start(csdo);

  CHECK_FALSE(co_csdo_is_stopped(csdo));
  CHECK(co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the CSDO service (co_csdo_t) containing object 0x1280 in
///        the object dictionary; "COB-ID client -> server" entry contains
///        an extended CAN ID
///
/// \When co_csdo_start() is called
///
/// \Then the service is not stopped, the service is idle
///       \Calls co_csdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_csdo_abort_req()
///       \Calls co_csdo_is_valid()
///       \Calls can_recv_start()
TEST(CO_CsdoInit, CoCsdoStart_CobidRes_ExtendedId) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  obj1280->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0x02u});
  obj1280->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t{0x600u + CSDO_NUM});
  const co_unsigned32_t cobid_res = DEV_ID | (1u << 28u) | CO_SDO_COBID_FRAME;
  obj1280->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED32, cobid_res);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  co_csdo_start(csdo);

  CHECK_FALSE(co_csdo_is_stopped(csdo));
  CHECK(co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the CSDO service (co_csdo_t) containing object 0x1280 in
///        the object dictionary
///
/// \When co_csdo_start() is called
///
/// \Then the service is not stopped, the service is idle
///       \Calls co_csdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls co_csdo_is_valid()
///       \Calls can_recv_start()
TEST(CO_CsdoInit, CoCsdoStart_DefaultCSDO_WithObj1280) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  co_csdo_start(csdo);

  CHECK_FALSE(co_csdo_is_stopped(csdo));
  CHECK(co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

///@}

/// @name co_csdo_stop()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) containing object 0x1280 in
///        the object dictionary
///
/// \When co_csdo_stop() is called
///
/// \Then the service is stopped
///       \Calls co_csdo_is_stopped()
TEST(CO_CsdoInit, CoCsdoStop_OnCreated) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK(csdo != nullptr);

  co_csdo_stop(csdo);

  CHECK(co_csdo_is_stopped(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) containing 0x1280
///        object in the object dictionary
///
/// \When co_csdo_stop() is called
///
/// \Then the service is stopped
///       \Calls co_csdo_is_stopped()
///       \Calls co_csdo_abort_req()
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
TEST(CO_CsdoInit, CoCsdoStop_OnStarted) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK(csdo != nullptr);
  co_csdo_start(csdo);

  co_csdo_stop(csdo);

  CHECK(co_csdo_is_stopped(csdo));

  co_csdo_destroy(csdo);
}

///@}

/// @name co_csdo_abort_req()
///@{

/// \Given a pointer to the stopped SSDO service (co_ssdo_t)
///
/// \When co_csdo_abort_req() is called with an abort code
///
/// \Then nothing is changed
TEST(CO_CsdoInit, CoCsdoAbortReq_Stopped) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK(csdo != nullptr);
  co_csdo_stop(csdo);

  co_csdo_abort_req(csdo, CO_SDO_AC_ERROR);

  co_csdo_destroy(csdo);
}

///@}

TEST_BASE(CO_CsdoBase) {
  TEST_BASE_SUPER(CO_CsdoBase);

  using sub_type = co_unsigned16_t;

  uint_least8_t data = 0;

  static const co_unsigned8_t CSDO_NUM = 0x01u;
  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned32_t DEFAULT_COBID_REQ = 0x600u + DEV_ID;
  static const co_unsigned32_t DEFAULT_COBID_RES = 0x580u + DEV_ID;
  co_csdo_t* csdo = nullptr;
  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  Allocators::Default defaultAllocator;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1280;

  // obj 0x1280, sub 0x00 - highest sub-index supported
  void SetCli00HighestSubidxSupported(const co_unsigned8_t subidx) const {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1280u, 0x00u);
    if (sub != nullptr)
      co_sub_set_val_u8(sub, subidx);
    else
      obj1280->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, subidx);
  }

  // obj 0x1280, sub 0x01 contains COB-ID client -> server
  void SetCli01CobidReq(const co_unsigned32_t cobid) const {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1280u, 0x01u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1280->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1280, sub 0x02 contains COB-ID server -> client
  void SetCli02CobidRes(const co_unsigned32_t cobid) const {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1280u, 0x02u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1280->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  co_unsigned32_t GetCli01CobidReq() const {
    return co_dev_get_val_u32(dev, 0x1280u, 0x01u);
  }

  co_unsigned32_t GetCli02CobidRes() const {
    return co_dev_get_val_u32(dev, 0x1280u, 0x02u);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(defaultAllocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    can_net_set_send_func(net, CanSend::Func, nullptr);

    dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
    SetCli00HighestSubidxSupported(0x02u);
    SetCli01CobidReq(DEFAULT_COBID_REQ);
    SetCli02CobidRes(DEFAULT_COBID_RES);
    csdo = co_csdo_create(net, dev, CSDO_NUM);
    CHECK(csdo != nullptr);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_csdo_destroy(csdo);

    dev_holder.reset();
    can_net_destroy(net);

    set_errnum(ERRNUM_SUCCESS);
  }
};

TEST_GROUP_BASE(CoCsdoSetGet, CO_CsdoBase) {
  const int32_t dummy = 0;  // clang-format workaround

  static void co_csdo_ind_func(const co_csdo_t*, co_unsigned16_t,
                               co_unsigned8_t, size_t, size_t, void*) {}
};

/// @name co_csdo_get_net()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_net() is called
///
/// \Then a pointer to the network (can_net_t) is returned
TEST(CoCsdoSetGet, CoCsdoGetNet_Nominal) {
  const auto* const ret = co_csdo_get_net(csdo);

  POINTERS_EQUAL(net, ret);
}

///@}

/// @name co_csdo_get_dev()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_dev() is called
///
/// \Then a pointer to the device (co_dev_t) is returned
TEST(CoCsdoSetGet, CoCsdoGetDev_Nominal) {
  const auto* const ret = co_csdo_get_dev(csdo);

  POINTERS_EQUAL(dev, ret);
}

///@}

/// @name co_csdo_get_num()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_num() is called
///
/// \Then the service's CSDO number is returned
TEST(CoCsdoSetGet, CoCsdoGetNum_Nominal) {
  const auto ret = co_csdo_get_num(csdo);

  CHECK_EQUAL(CSDO_NUM, ret);
}

///@}

/// @name co_csdo_get_par()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_par() is called
///
/// \Then a pointer to the parameter object is returned
TEST(CoCsdoSetGet, CoCsdoGetPar_Nominal) {
  const auto* const par = co_csdo_get_par(csdo);

  POINTER_NOT_NULL(par);
  CHECK_EQUAL(3u, par->n);
  CHECK_EQUAL(CSDO_NUM, par->id);
  CHECK_EQUAL(0x580u + CSDO_NUM, par->cobid_res);
  CHECK_EQUAL(0x600u + CSDO_NUM, par->cobid_req);
}

///@}

/// @name co_csdo_get_dn_ind()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_dn_ind() is called with a memory area to store the
///       results
///
/// \Then null pointers are returned
TEST(CoCsdoSetGet, CoCsdoGetDnInd_Nominal) {
  int32_t data = 0;
  co_csdo_ind_t* pind = co_csdo_ind_func;
  void* pdata = &data;

  co_csdo_get_dn_ind(csdo, &pind, &pdata);

  POINTERS_EQUAL(nullptr, pind);
  POINTERS_EQUAL(nullptr, pdata);
}

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_dn_ind() is called with no memory area to store the
///       results
///
/// \Then nothing is changed
TEST(CoCsdoSetGet, CoCsdoGetDnInd_NoMemoryArea) {
  co_csdo_get_dn_ind(csdo, nullptr, nullptr);
}

///@}

/// @name co_csdo_set_dn_ind()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_set_dn_ind() is called with a pointer to the function and
///       a pointer to data
///
/// \Then CSDO download indication function and user-specified data pointers
///       are set
TEST(CoCsdoSetGet, CoCsdoSetDnInd_Nominal) {
  int32_t data = 0;

  co_csdo_set_dn_ind(csdo, co_csdo_ind_func, &data);

  co_csdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_csdo_get_dn_ind(csdo, &pind, &pdata);
  POINTERS_EQUAL(co_csdo_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

///@}

/// @name co_csdo_get_up_ind()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_up_ind() is called with a memory area to store the results
///
/// \Then null pointers are returned
TEST(CoCsdoSetGet, CoCsdoGetUpInd_Nominal) {
  int32_t data = 0;
  co_csdo_ind_t* pind = co_csdo_ind_func;
  void* pdata = &data;

  co_csdo_get_up_ind(csdo, &pind, &pdata);

  POINTERS_EQUAL(nullptr, pind);
  POINTERS_EQUAL(nullptr, pdata);
}

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_up_ind() is called with no memory to store the results
///
/// \Then nothing is changed
TEST(CoCsdoSetGet, CoCsdoGetUpInd_NoMemoryArea) {
  co_csdo_get_up_ind(csdo, nullptr, nullptr);
}

///@}

/// @name co_csdo_set_up_ind()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_set_up_ind() is called with a pointer to the function and
///       a pointer to data
///
/// \Then CSDO upload indication function and user-specified data pointers
///       are set
TEST(CoCsdoSetGet, CoCsdoSetUpInd_Nominal) {
  int32_t data = 0;

  co_csdo_set_up_ind(csdo, co_csdo_ind_func, &data);

  co_csdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_csdo_get_up_ind(csdo, &pind, &pdata);
  POINTERS_EQUAL(co_csdo_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

///@}

/// @name co_csdo_get_timeout()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_get_timeout() is called
///
/// \Then default timeout value of zero is returned
TEST(CoCsdoSetGet, CoCsdoGetTimeout_Nominal) {
  const auto ret = co_csdo_get_timeout(csdo);

  CHECK_EQUAL(0u, ret);
}

///@}

/// @name co_csdo_set_timeout()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) with no timeout set
///
/// \When co_csdo_set_timeout() is called with a valid timeout value
///
/// \Then timeout is set
TEST(CoCsdoSetGet, CoCsdoSetTimeout_ValidTimeout) {
  co_csdo_set_timeout(csdo, 20);

  CHECK_EQUAL(20, co_csdo_get_timeout(csdo));
}

/// \Given a pointer to the CSDO service (co_csdo_t) with no timeout set
///
/// \When co_csdo_set_timeout() is called with an invalid timeout value
///
/// \Then timeout is not set
TEST(CoCsdoSetGet, CoCsdoSetTimeout_InvalidTimeout) {
  co_csdo_set_timeout(csdo, -1);

  CHECK_EQUAL(0, co_csdo_get_timeout(csdo));
}

/// \Given a pointer to the CSDO service (co_csdo_t) with a timeout set
///
/// \When co_csdo_set_timeout() is called with a zero timeout value
///
/// \Then timeout is disabled
///       \Calls can_timer_stop()
TEST(CoCsdoSetGet, CoCsdoSetTimeout_DisableTimeout) {
  co_csdo_set_timeout(csdo, 1);

  co_csdo_set_timeout(csdo, 0);

  CHECK_EQUAL(0, co_csdo_get_timeout(csdo));
}

/// \Given a pointer to the CSDO service (co_csdo_t) with a timeout set
///
/// \When co_csdo_set_timeout() is called with a different timeout value
///
/// \Then timeout is updated
TEST(CoCsdoSetGet, CoCsdoSetTimeout_UpdateTimeout) {
  co_csdo_set_timeout(csdo, 1);

  co_csdo_set_timeout(csdo, 4);

  CHECK_EQUAL(4, co_csdo_get_timeout(csdo));
}

///@}

/// #co_csdo_ind_t mock.
class CoCsdoInd {
 public:
  static void
  Func(const co_csdo_t* const csdo, const co_unsigned16_t idx,
       const co_unsigned8_t subidx, const size_t size, const size_t nbyte,
       void* const data) {
    num_called++;

    csdo_ = csdo;
    idx_ = idx;
    subidx_ = subidx;
    size_ = size;
    nbyte_ = nbyte;
    data_ = data;
  }

  static void
  Check(const co_csdo_t* const csdo, const co_unsigned16_t idx,
        const co_unsigned8_t subidx, const size_t size, const size_t nbyte,
        const void* const data) {
    POINTERS_EQUAL(csdo, csdo_);
    CHECK_EQUAL(idx, idx_);
    CHECK_EQUAL(subidx, subidx_);
    CHECK_EQUAL(size, size_);
    CHECK_EQUAL(nbyte, nbyte_);
    POINTERS_EQUAL(data, data_);
  }

  static void
  CheckAndClear(const co_csdo_t* const csdo, const co_unsigned16_t idx,
                const co_unsigned8_t subidx, const size_t size,
                const size_t nbyte, const void* const data) {
    Check(csdo, idx, subidx, size, nbyte, data);
    Clear();
  }

  static size_t
  GetNumCalled() {
    return num_called;
  }

  static void
  Clear() {
    num_called = 0;

    csdo_ = nullptr;
    idx_ = 0;
    subidx_ = 0;
    size_ = 0;
    nbyte_ = 0;
    data_ = nullptr;
  }

 private:
  static const co_csdo_t* csdo_;
  static co_unsigned16_t idx_;
  static co_unsigned8_t subidx_;
  static size_t size_;
  static size_t nbyte_;
  static void* data_;
  static size_t num_called;
};
const co_csdo_t* CoCsdoInd::csdo_ = nullptr;
co_unsigned16_t CoCsdoInd::idx_ = 0;
co_unsigned8_t CoCsdoInd::subidx_ = 0;
size_t CoCsdoInd::size_ = 0;
size_t CoCsdoInd::nbyte_ = 0;
void* CoCsdoInd::data_ = nullptr;
size_t CoCsdoInd::num_called = 0;

/// Unsigned 64-bit sample value wrapper.
class SampleValueU64 {
  using sub_type64 = co_unsigned64_t;
  using segment_data_t = std::vector<uint_least8_t>;

 public:
  explicit SampleValueU64(const sub_type64 val = 0x1234567890abcdefu)
      : VAL(val) {
    buf = StLe64InArray(VAL);
  }

  segment_data_t
  GetFirstSegment() {
    segment_data_t segment(CO_SDO_SEG_MAX_DATA_SIZE, 0);
    std::copy(buf.begin(), std::next(buf.begin(), CO_SDO_SEG_MAX_DATA_SIZE),
              segment.begin());

    return segment;
  }

  segment_data_t
  GetLastSegment() {
    return segment_data_t(1u, buf.back());
  }

  const void*
  GetValPtr() const {
    return &VAL;
  }

  void*
  GetBufPtr() {
    return buf.data();
  }

  static std::array<uint_least8_t, sizeof(sub_type64)>
  StLe64InArray(const sub_type64 val) {
    std::array<uint_least8_t, sizeof(sub_type64)> array = {0};
    stle_u64(array.data(), val);

    return array;
  }

 private:
  sub_type64 VAL = 0;
  std::array<uint_least8_t, sizeof(sub_type64)> buf;
};

/// Unsigned 16-bit sample value wrapper.
class SampleValueU16 {
  using sub_type = co_unsigned16_t;
  using segment_data_t = std::vector<uint_least8_t>;

 public:
  explicit SampleValueU16(const sub_type val = 0x1234u) : VAL(val) {
    buf = StLe16InArray(VAL);
  }

  segment_data_t
  GetSegmentData() {
    segment_data_t segment(CO_SDO_SEG_MAX_DATA_SIZE, 0);
    segment[0] = buf[0];
    segment[1] = buf[1];

    return segment;
  }

  const void*
  GetValPtr() const {
    return &VAL;
  }

  void*
  GetBufPtr() {
    return buf.data();
  }

  sub_type
  GetVal() const {
    return VAL;
  }

  static std::array<uint_least8_t, sizeof(sub_type)>
  StLe16InArray(const sub_type val) {
    std::array<uint_least8_t, sizeof(sub_type)> array = {0};
    stle_u16(array.data(), val);

    return array;
  }

 private:
  const sub_type VAL;
  std::array<uint_least8_t, sizeof(sub_type)> buf;
};

/// OCTET STRING sample value wrapper.
class SampleValueOctetString {
 public:
  const void*
  GetValPtr() {
    return &val2dn;
  }

  co_unsigned16_t
  GetDataType() const {
    return CO_DEFTYPE_OCTET_STRING;
  }

 private:
  CoArrays arrays;
  const co_octet_string_t val2dn = arrays.Init<co_octet_string_t>();
};

TEST_GROUP_BASE(CO_Csdo, CO_CsdoBase) {
  CoArrays arrays;

  using sub_type64 = co_unsigned64_t;

  membuf ind_mbuf;
  size_t num_called;
  static const co_unsigned16_t SUB_TYPE = CO_DEFTYPE_UNSIGNED16;
  static const co_unsigned16_t SUB_TYPE64 = CO_DEFTYPE_UNSIGNED64;
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;
  static const co_unsigned16_t INVALID_IDX = 0xffffu;
  static const co_unsigned8_t INVALID_SUBIDX = 0xffu;
  const sub_type VAL = 0xabcdu;
  SampleValueOctetString val_os;
  SampleValueU64 val_u64;
  SampleValueU16 val_u16;
  std::unique_ptr<CoObjTHolder> obj2020;
  std::unique_ptr<CoObjTHolder> obj2021;
#if LELY_NO_MALLOC
  char buffer[sizeof(sub_type)] = {0};
  char ext_buffer[sizeof(sub_type)] = {0};
#endif

  static uint_least16_t LoadLE_U16(const membuf* const mbuf) {
    CHECK(membuf_size(mbuf) >= sizeof(co_unsigned16_t));
    return ldle_u16(static_cast<uint_least8_t*>(membuf_begin(mbuf)));
  }

  void MembufInitSubType(membuf* const mbuf) {
#if LELY_NO_MALLOC
    membuf_init(mbuf, buffer, sizeof(sub_type));
#endif
    CHECK_COMPARE(membuf_reserve(mbuf, sizeof(sub_type)), >=, sizeof(sub_type));
  }

  void MembufInitSubTypeExt(membuf* const mbuf) {
#if LELY_NO_MALLOC
    membuf_init(mbuf, ext_buffer, sizeof(sub_type));
#else
    CHECK_COMPARE(membuf_reserve(mbuf, sizeof(sub_type)), >=, sizeof(sub_type));
#endif
  }

  void StartCSDO() { co_csdo_start(csdo); }

  static co_unsigned32_t co_sub_failing_dn_ind(co_sub_t*, co_sdo_req*,
                                               co_unsigned32_t, void*) {
    return CO_SDO_AC_HARDWARE;
  }

  void InitiateBlockDownloadRequest(const co_unsigned16_t idx = IDX,
                                    const co_unsigned8_t subidx = SUBIDX,
                                    const sub_type val = 0) {
    CHECK_TRUE(co_csdo_is_idle(csdo));

    CHECK_EQUAL(0, co_csdo_blk_dn_val_req(csdo, idx, subidx, SUB_TYPE, &val,
                                          CoCsdoDnCon::Func, &data));
    CanSend::Clear();
  }

  void InitiateBlockUploadRequest(
      const co_unsigned16_t idx = IDX, const co_unsigned8_t subidx = SUBIDX,
      const co_unsigned32_t size = sizeof(sub_type)) {
    CHECK_TRUE(co_csdo_is_idle(csdo));

    CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, idx, subidx, 0, nullptr,
                                      CoCsdoUpCon::func, &data));

    const can_msg msg_res =
        SdoCreateMsg::BlkUpIniRes(idx, subidx, DEFAULT_COBID_RES, size);
    CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));
    CanSend::Clear();
  }

  void InitiateBlockUploadRequestWithCrc(
      const co_unsigned16_t idx = IDX, const co_unsigned8_t subidx = SUBIDX,
      const co_unsigned32_t size = sizeof(sub_type)) {
    CHECK_TRUE(co_csdo_is_idle(csdo));

    CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, idx, subidx, 0, nullptr,
                                      CoCsdoUpCon::func, nullptr));

    can_msg msg_res =
        SdoCreateMsg::BlkUpIniRes(idx, subidx, DEFAULT_COBID_RES, size);
    msg_res.data[0] |= CO_SDO_BLK_CRC;
    CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));
    CanSend::Clear();
  }

  void ReceiveBlkUpSegReq() {
    const uint_least8_t sequence_number = 1u;
    const auto msg_up_seg =
        SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, sequence_number,
                                  val_u16.GetSegmentData(), CO_SDO_SEQ_LAST);
    CHECK_EQUAL(1, can_net_recv(net, &msg_up_seg, 0));
  }

  void ReceiveBlockDownloadSubInitiateResponse(
      const co_unsigned16_t idx, const co_unsigned8_t subidx,
      const uint_least8_t block_size = CO_SDO_MAX_SEQNO,
      const uint_least8_t cs_flags = 0) {
    const can_msg msg = SdoCreateMsg::BlkDnIniRes(
        idx, subidx, DEFAULT_COBID_RES, cs_flags, block_size);
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  void ReceiveBlockDownloadResponse(const uint_least8_t seqno,
                                    const uint_least8_t blksize) {
    const auto blk_dn_seg_res = SdoCreateMsg::BlkDnSubRes(
        seqno, blksize, DEFAULT_COBID_RES, CO_SDO_SC_BLK_RES);
    CHECK_EQUAL(1, can_net_recv(net, &blk_dn_seg_res, 0));
  }

  void CheckBlockDownloadSubRequestSent(
      const uint_least8_t seqno, const std::vector<uint_least8_t>& data = {})
      const {
    const auto last_segment_req = SdoCreateMsg::BlkDnSubReq(
        DEFAULT_COBID_REQ, seqno, CO_SDO_SEQ_LAST, data);
    CanSend::CheckMsg(last_segment_req);
    CanSend::Clear();
  }

  void CheckBlockDownloadEndRequestSent(const uint_least8_t size,
                                        const co_unsigned16_t crc = 0) const {
    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    const auto expected = SdoCreateMsg::BlkDnEndReq(
        DEFAULT_COBID_REQ, crc, CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(size));
    CanSend::CheckMsg(expected);
    CanSend::Clear();
  }

  void CheckSdoAbortSent(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                         const co_unsigned32_t abort_code,
                         const uint_least32_t recipient_id = DEFAULT_COBID_REQ)
      const {
    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    const auto expected_timeout =
        SdoInitExpectedData::U32(CO_SDO_CS_ABORT, idx, subidx, abort_code);
    CanSend::CheckMsg(recipient_id, 0, CO_SDO_MSG_SIZE,
                      expected_timeout.data());
    CanSend::Clear();
  }

  void CheckLastSegmentSent(const uint_least8_t seqno,
                            const std::vector<uint_least8_t>& data) const {
    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    const auto expected_last =
        SdoInitExpectedData::Segment(CO_SDO_SEQ_LAST | seqno, data);
    CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                      expected_last.data());
    CanSend::Clear();
  }

  void InitiateOsBlockDownloadValRequest(const co_unsigned16_t idx,
                                         const co_unsigned8_t subidx,
                                         const void* const val) {
    CHECK_TRUE(co_csdo_is_idle(csdo));

    CHECK_EQUAL(
        0, co_csdo_blk_dn_val_req(csdo, idx, subidx, CO_DEFTYPE_OCTET_STRING,
                                  val, CoCsdoDnCon::Func, &data));
    CanSend::Clear();
  }

  void AdvanceToBlkDnEndState(const co_unsigned16_t idx,
                              const co_unsigned8_t subidx) {
    CHECK_TRUE(co_csdo_is_idle(csdo));

    InitiateOsBlockDownloadValRequest(idx, subidx, val_os.GetValPtr());
    ReceiveBlockDownloadSubInitiateResponse(idx, subidx);
    CheckBlockDownloadEndRequestSent(0);
  }

  void AdvanceToBlkUpEndState() {
    CHECK_TRUE(co_csdo_is_idle(csdo));

    InitiateBlockUploadRequest();
    ReceiveBlkUpSegReq();
    CanSend::Clear();
  }

  void AdvanceToBlkDnSubState(const co_unsigned16_t idx,
                              const co_unsigned8_t subidx) {
    InitiateBlockDownloadRequest(idx, subidx, val_u16.GetVal());
    ReceiveBlockDownloadSubInitiateResponse(idx, subidx, 1u);
    const uint_least8_t sequence_number = 1u;
    CheckLastSegmentSent(sequence_number, val_u16.GetSegmentData());
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateAndInsertObj(obj2020, IDX);
    obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});

    CoCsdoUpCon::Clear();
    CanSend::Clear();
    CoCsdoInd::Clear();
  }

  TEST_TEARDOWN() {
    arrays.Clear();

    TEST_BASE_TEARDOWN();
  }
};

/// @name co_csdo_is_valid()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) with valid COB-ID
///        client -> server and valid COB-ID server -> client set
///
/// \When co_csdo_is_valid() is called
///
/// \Then 1 is returned
TEST(CO_Csdo, CoCsdoIsValid_ReqResValid) {
  const auto ret = co_csdo_is_valid(csdo);

  CHECK(ret);
}

/// \Given a pointer to the CSDO service (co_csdo_t) with invalid COB-ID
///        client -> server and valid COB-ID server -> client set
///
/// \When co_csdo_is_valid() is called
///
/// \Then 0 is returned
TEST(CO_Csdo, CoCsdoIsValid_ReqInvalid) {
  SetCli01CobidReq(DEFAULT_COBID_REQ | CO_SDO_COBID_VALID);
  StartCSDO();

  const auto ret = co_csdo_is_valid(csdo);

  CHECK_FALSE(ret);
}

/// \Given a pointer to the CSDO service (co_csdo_t) with valid COB-ID
///        client -> server and invalid COB-ID server -> client set
///
/// \When co_csdo_is_valid() is called
///
/// \Then 0 is returned
TEST(CO_Csdo, CoCsdoIsValid_ResInvalid) {
  SetCli02CobidRes(DEFAULT_COBID_RES | CO_SDO_COBID_VALID);
  StartCSDO();

  const auto ret = co_csdo_is_valid(csdo);

  CHECK_FALSE(ret);
}

///@}

/// @name co_csdo_up_req()
///@{

/// \Given a pointer to the started CSDO service (co_csdo_t) with an invalid
///        "COB-ID client -> server (rx)" entry
///
/// \When co_csdo_up_req() is called with a multiplexer, a null buffer pointer,
///       a pointer to the confirmation function and a pointer to
///       a user-specified data
///
/// \Then -1 is returned, ERRNUM_INVAL is set as the error number, upload
///       confirmation function is not called
///       \Calls co_csdo_is_valid()
///       \Calls set_errnum()
TEST(CO_Csdo, CoCsdoUpReq_InvalidCobIdReq) {
  SetCli01CobidReq(DEFAULT_COBID_REQ | CO_SDO_COBID_VALID);
  co_csdo_stop(csdo);
  co_csdo_start(csdo);

  const auto ret =
      co_csdo_up_req(csdo, IDX, SUBIDX, nullptr, &CoCsdoUpCon::func, &data);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CoCsdoUpCon::num_called);
}

///@}

/// @name co_csdo_dn_req()
///@{

/// \Given a pointer to the started CSDO service (co_csdo_t) with an invalid
///        "COB-ID client -> server (rx)" entry
///
/// \When co_csdo_dn_req() is called with a multiplexer, a null
///       bytes-to-be-downloaded pointer, zero, a pointer to the confirmation
///       function and a pointer to a user-specified data
///
/// \Then -1 is returned, ERRNUM_INVAL is set as the error number, download
///       confirmation function is not called
///       \Calls co_csdo_is_valid()
///       \Calls set_errnum()
TEST(CO_Csdo, CoCsdoDnReq_InvalidCobIdReq) {
  SetCli01CobidReq(DEFAULT_COBID_REQ | CO_SDO_COBID_VALID);
  co_csdo_stop(csdo);
  co_csdo_start(csdo);

  const auto ret =
      co_csdo_dn_req(csdo, IDX, SUBIDX, nullptr, 0, &CoCsdoDnCon::Func, &data);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CoCsdoDnCon::GetNumCalled());
}

///@}

/// @name co_dev_dn_req()
///@{

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index of the existing object and
///       a sub-index of a non-existing sub-object, a pointer to a value,
///       the length of the value and a pointer to a download confirmation
///       function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an invalid index, an invalid sub-index, CO_SDO_AC_NO_OBJ and
///       a null pointer; the error number is not changed
///       \Calls co_dev_dn_req()
///       \Calls co_dev_find_obj()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnReq_NoObj) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_req(dev, INVALID_IDX, INVALID_SUBIDX, &VAL,
                                 sizeof(VAL), CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, INVALID_IDX, INVALID_SUBIDX, CO_SDO_AC_NO_OBJ,
                     nullptr);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index of the existing object and
///       a sub-index of a non-existing sub-object, a pointer to a value,
///       the length of the value and a download confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, invalid sub-index, CO_SDO_AC_NO_SUB and
///       a null pointer; the error number is not changed
///       \Calls get_errc()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnReq_NoSub) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_req(dev, IDX, INVALID_SUBIDX, &VAL, sizeof(VAL),
                                 CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, INVALID_SUBIDX, CO_SDO_AC_NO_SUB, nullptr);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index and a sub-index of an existing
///       entry, a pointer to a value, the length of the value and no download
///       confirmation function
///
/// \Then 0 is returned, the requested value is set; the error number is not
///       changed
///       \Calls get_errc()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_up()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnReq_NoCsdoDnConFunc) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_dn_req(dev, IDX, SUBIDX, &VAL, sizeof(VAL), nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(VAL, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index and a sub-index of an existing
///       entry, a pointer to a value, the length of the value and a download
///       confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, a sub-index, 0 as the abort code and a null pointer
///       and the requested value is set; the error number is not changed
///       \Calls get_errc()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_up()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnReq_Nominal) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_req(dev, IDX, SUBIDX, &VAL, sizeof(VAL),
                                 CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, 0u, nullptr);
  CHECK_EQUAL(VAL, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

///@}

/// @name co_dev_dn_val_req()
///@{

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_val_req() is called with an index and a sub-index of
///       a non-existing sub-object, a pointer to a value, a type of the value,
///       no memory buffer and a download confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an invalid index, an invalid sub-index, CO_SDO_AC_NO_OBJ and
///       a null pointer; the error number is not changed
///       \Calls get_errc()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnValReq_NoObj) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_val_req(dev, INVALID_IDX, INVALID_SUBIDX, SUB_TYPE,
                                     &VAL, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, INVALID_IDX, INVALID_SUBIDX, CO_SDO_AC_NO_OBJ,
                     nullptr);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index of the existing object and
///       a sub-index of a non-existing sub-object, a pointer to a value, a type
///       of the value, no memory buffer and a download confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, invalid sub-index, CO_SDO_AC_NO_SUB and
///       a null pointer; the error number is not changed
///       \Calls get_errc()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnValReq_NoSub) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_val_req(dev, IDX, INVALID_SUBIDX, SUB_TYPE, &VAL,
                                     nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, INVALID_SUBIDX, CO_SDO_AC_NO_SUB, nullptr);
}

#if LELY_NO_MALLOC
/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index and a sub-index of an existing
///       entry, a pointer to a value, 64-bit type, no memory buffer and
///       a download confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, a sub-index, CO_SDO_AC_NO_MEM and a null pointer,
///       the requested value is not set; the error number is not changed
///       \Calls get_errc()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_up_val()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnValReq_DnTooLong) {
  const uint_least64_t data = 0xffffffffu;
  membuf mbuf = MEMBUF_INIT;

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_val_req(dev, IDX, SUBIDX, CO_DEFTYPE_UNSIGNED64,
                                     &data, &mbuf, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_NO_MEM, nullptr);
  CHECK_EQUAL(0u, co_dev_get_val_u8(dev, IDX, SUBIDX));

  membuf_fini(&mbuf);
}
#endif  // LELY_NO_MALLOC

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index and a sub-index of an existing
///       entry, a pointer to a value, a type of the value, no memory buffer and
///       no download confirmation function
///
/// \Then 0 is returned, the requested value is set; the error number is not
///       changed
///       \Calls get_errc()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_up_val()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnValReq_NoCsdoDnConFunc) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_val_req(dev, IDX, SUBIDX, SUB_TYPE, &VAL, nullptr,
                                     nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(VAL, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_dn_req() is called with an index and a sub-index of an existing
///       entry, a pointer to a value, a type of the value, no memory buffer and
///       a download confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, a sub-index, 0 as the abort code and a null
///       pointer, the requested value is set; the error number is not changed
///       \Calls get_errc()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_up_val()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnValReq_Nominal) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_val_req(dev, IDX, SUBIDX, SUB_TYPE, &VAL, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, 0u, nullptr);
  CHECK_EQUAL(VAL, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

///@}

/// @name co_dev_dn_dcf_req()
///@{

/// \Given a pointer to the device (co_dev_t), a too short concise DCF buffer
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and the
///       end of the buffer and a pointer to the confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, 0, 0, CO_SDO_AC_TYPE_LEN_LO abort code and a null pointer,
///       the requested value is not changed; the error number is not changed
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnDcfReq_ConciseBufTooShort) {
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  const auto max_missing_bytes = dcf.Size() - sizeof(sub_type);
  for (size_t bytes_missing = sizeof(sub_type) + 1u;
       bytes_missing < max_missing_bytes; bytes_missing++) {
    const errnum_t error_num = ERRNUM_FAULT;
    set_errnum(error_num);

    CHECK_EQUAL(dcf.Size(), co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(),
                                             dcf.End() - bytes_missing));

    const auto ret =
        co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End() - bytes_missing,
                          CoCsdoDnCon::Func, nullptr);

    CHECK_EQUAL(0, ret);
    CHECK_EQUAL(error_num, get_errnum());
    CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
    CoCsdoDnCon::Check(nullptr, 0, 0, CO_SDO_AC_TYPE_LEN_LO, nullptr);
    CHECK_EQUAL(0, co_dev_get_val_u16(dev, IDX, SUBIDX));

    CoCsdoDnCon::Clear();
  }
}

/// \Given a pointer to the device (co_dev_t), an invalid concise DCF buffer
///        that is too small for a declared entry value
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and the
///       end of the buffer and a pointer to the confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, a sub-index, CO_SDO_AC_TYPE_LEN_LO and a null
///       pointer, the requested value is not changed; the error number is not
///       changed
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnDcfReq_DatasizeMismatch) {
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));

  obj2020->RemoveAndDestroyLastSub();
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End() - 1u,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_TYPE_LEN_LO, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

/// \Given a pointer to the device (co_dev_t), a concise DCF buffer with
///        an index of an object which is not present in a device
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and the
///       end of the buffer and a pointer to the confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, index of a non-existing object, a sub-index, CO_SDO_AC_NO_OBJ
///       and a null pointer; the error number is not changed
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls co_dev_find_obj()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnDcfReq_NoObj) {
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));

  CHECK_EQUAL(0, co_dev_remove_obj(dev, obj2020->Get()));
  POINTERS_EQUAL(obj2020->Get(), obj2020->Reclaim());

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End(),
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_NO_OBJ, nullptr);
}

/// \Given a pointer to the device (co_dev_t), a concise DCF buffer with
///        an existing object index but non-existing sub-index
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and the
///       end of the buffer with concise DCF and a pointer to the confirmation
///       function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, the index and the sub-index, CO_SDO_AC_NO_SUB and
///       a null pointer; the error number is not changed
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnDcfReq_NoSub) {
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));
  obj2020->RemoveAndDestroyLastSub();

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End(),
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_NO_SUB, nullptr);
}

/// \Given a pointer to the device (co_dev_t), a concise DCF with many entries
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and the
///       end of the buffer and a pointer to the confirmation function, but
///       download indication function returns an abort code
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, a sub-index, the abort code and
///       a null pointer, the requested value is not set; the error number is
///       not changed
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_clear()
///       \Calls co_sdo_req_up()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnDcfReq_ManyEntriesButDnIndFail) {
  const co_unsigned16_t OTHER_IDX = 0x2021u;
  dev_holder->CreateAndInsertObj(obj2021, OTHER_IDX);
  obj2021->InsertAndSetSub(0x00u, SUB_TYPE, sub_type{0});
  auto combined_dcf = ConciseDcf::MakeForEntries<sub_type, sub_type>();
  CHECK_EQUAL(combined_dcf.Size(),
              co_dev_write_dcf(dev, IDX, OTHER_IDX, combined_dcf.Begin(),
                               combined_dcf.End()));

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_sub_set_dn_ind(obj2020->GetLastSub(), co_sub_failing_dn_ind, nullptr);
  const auto ret =
      co_dev_dn_dcf_req(dev, combined_dcf.Begin(), combined_dcf.End(),
                        CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_HARDWARE, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

/// \Given a pointer to the device (co_dev_t), a concise DCF buffer
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and the
///       end of the buffer and no confirmation function
///
/// \Then 0 is returned and the requested value is set; the error number is not
///       changed
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_clear()
///       \Calls co_sdo_req_up()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnDcfReq_NoCoCsdoDnCon) {
  co_sub_set_val_u16(obj2020->GetLastSub(), VAL);
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));
  co_sub_set_val_u16(obj2020->GetLastSub(), 0);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End(), nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(VAL, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

/// \Given a pointer to the device (co_dev_t), a concise DCF buffer
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and
///       the end of the buffer and a pointer to the confirmation function
///
/// \Then 0 is returned, the confirmation function is called once with a null
///       pointer, an index, a sub-index, 0 as the abort code and a null
///       pointer, the requested value is set; the error number is not changed
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_clear()
///       \Calls co_sdo_req_up()
///       \Calls co_sub_dn_ind()
///       \Calls co_sdo_req_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevDnDcfReq_Nominal) {
  co_sub_set_val_u16(obj2020->GetLastSub(), VAL);
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));
  co_sub_set_val_u16(obj2020->GetLastSub(), 0);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End(),
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, 0, nullptr);
  CHECK_EQUAL(VAL, co_dev_get_val_u16(dev, IDX, SUBIDX));
}

///@}

/// @name co_dev_up_req()
///@{

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///        dictionary, the entry has no read access
///
/// \When co_dev_up_req() is called with an index and a sub-index of the
///       existing entry, a pointer to the memory buffer to store the requested
///       value and a pointer to the confirmation function
///
/// \Then 0 is returned, the memory buffer remains empty, the confirmation
///       function is called with a null pointer, the index and the sub-index
///       of the entry, CO_SDO_AC_NO_READ abort code, no memory buffer and
///       a null user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_size()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_NoReadAccess) {
  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_sub_set_access(obj2020->GetLastSub(), CO_ACCESS_WO);

  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, &mbuf, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_NO_READ, nullptr, 0,
                     nullptr);
  CHECK_EQUAL(0, membuf_size(&mbuf));

  membuf_fini(&mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///       dictionary
///
/// \When co_dev_up_req() is called with an index and a sub-index of an existing
///       entry, a pointer to the memory buffer to store the requested value and
///       no confirmation function
///
/// \Then 0 is returned, the memory buffer contains the requested value; the
///       error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls co_sdo_req_last()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_NoConfirmationFunction) {
  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);

  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_up_req(dev, IDX, SUBIDX, &mbuf, nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(sizeof(sub_type), membuf_size(&mbuf));
  CHECK_EQUAL(0x1234u, LoadLE_U16(&mbuf));

  membuf_fini(&mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///        dictionary
///
/// \When co_dev_up_req() is called with an index and a sub-index of an existing
///       entry, no memory buffer to store the requested value and a pointer to
///       the confirmation function
///
/// \Then 0 is returned, the confirmation function is called with a null
///       pointer, the index and the sub-index of the entry, 0 as the abort
///       code, a pointer to the uploaded bytes, the number of the uploaded
///       bytes and a null user-specified data pointer; the error number is
///       not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_NoBufPtr) {
  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, nullptr, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  POINTERS_EQUAL(nullptr, CoCsdoUpCon::sdo);
  CHECK_EQUAL(IDX, CoCsdoUpCon::idx);
  CHECK_EQUAL(SUBIDX, CoCsdoUpCon::subidx);
  CHECK_EQUAL(0, CoCsdoUpCon::ac);
  CHECK(CoCsdoUpCon::ptr != nullptr);
  CHECK_EQUAL(sizeof(sub_type), CoCsdoUpCon::n);
  POINTERS_EQUAL(nullptr, CoCsdoUpCon::data);
  CHECK_EQUAL(0x1234u, ldle_u16(CoCsdoUpCon::buf));
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_dev_up_req() is called with an index and a sub-index of
///       a non-existing entry, a pointer to the memory buffer to store the
///       requested value and a pointer to the confirmation function
///
/// \Then 0 is returned, the memory buffer remains empty, the confirmation
///       function is called with a null pointer, the index and the sub-index
///       of the entry, CO_SDO_AC_NO_OBJ abort code, a null uploaded bytes
///       pointer, 0 as the number of the uploaded bytes and a null
///       user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_NoObj) {
  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_up_req(dev, INVALID_IDX, INVALID_SUBIDX, &mbuf,
                                 CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, INVALID_IDX, INVALID_SUBIDX, CO_SDO_AC_NO_OBJ,
                     nullptr, 0, nullptr);
  CHECK_EQUAL(0, membuf_size(&mbuf));

  membuf_fini(&mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_up_req() is called with an index of the existing object and
///       a sub-index of a non-existing sub-object, a pointer to the memory
///       buffer to store the requested value and a pointer to the confirmation
///       function
///
/// \Then 0 is returned, the memory buffer remains empty, the confirmation
///       function is called with a null pointer, the index and the sub-index
///       of the entry, CO_SDO_AC_NO_SUB abort code, a null uploaded bytes
///       pointer, 0 as the number of the uploaded bytes and a null
///       user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_NoSub) {
  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_up_req(dev, IDX, INVALID_SUBIDX, &mbuf,
                                 CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, INVALID_SUBIDX, CO_SDO_AC_NO_SUB, nullptr, 0,
                     nullptr);
  CHECK_EQUAL(0, membuf_size(&mbuf));

  membuf_fini(&mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an array object in
///        the object dictionary
///
/// \When co_dev_up_req() is called with an index and a sub-index of an existing
///       element, but the sub-index is greater than the length of the array,
///       a pointer to the memory buffer to store the requested value and
///       a pointer to the confirmation function
///
/// \Then 0 is returned, the memory buffer remains empty, the confirmation
///       function is called with a null pointer, the index and the sub-index
///       of the entry, CO_SDO_AC_NO_DATA abort code, a null uploaded bytes
///       pointer, 0 as the number of the uploaded bytes and a null
///       user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_ArrayObject_NoElement) {
  const co_unsigned16_t ARRAY_IDX = 0x2021u;
  const co_unsigned8_t ELEMENT_SUBIDX = 0x01u;

  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  dev_holder->CreateAndInsertObj(obj2021, ARRAY_IDX);
  co_obj_set_code(obj2021->Get(), CO_OBJECT_ARRAY);
  obj2021->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0x00u});
  obj2021->InsertAndSetSub(ELEMENT_SUBIDX, CO_DEFTYPE_UNSIGNED8,
                           co_unsigned8_t{0});

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_up_req(dev, ARRAY_IDX, ELEMENT_SUBIDX, &mbuf,
                                 CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, ARRAY_IDX, ELEMENT_SUBIDX, CO_SDO_AC_NO_DATA,
                     nullptr, 0, nullptr);
  CHECK_EQUAL(0, membuf_size(&mbuf));

  membuf_fini(&mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an array object in
///        the object dictionary, the array object contains at least one element
///
/// \When co_dev_up_req() is called with an index and a sub-index of an existing
///       element of the array, a pointer to the memory buffer to store
///       the requested value and a pointer to the confirmation function
///
/// \Then 0 is returned, the memory buffer contains the requested value,
///       the confirmation function is called with a null pointer, the index and
///       the sub-index of the entry, 0 as the abort code, a pointer to
///       the uploaded bytes, the number of the uploaded bytes and a null
///       user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_obj_get_val_u8()
///       \Calls membuf_begin()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_ArrayObject) {
  const co_unsigned16_t ARRAY_IDX = 0x2021u;
  const co_unsigned8_t ELEMENT_SUBIDX = 0x01u;

  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  dev_holder->CreateAndInsertObj(obj2021, ARRAY_IDX);
  co_obj_set_code(obj2021->Get(), CO_OBJECT_ARRAY);
  obj2021->InsertAndSetSub(0x00, CO_DEFTYPE_UNSIGNED8, ELEMENT_SUBIDX);
  obj2021->InsertAndSetSub(ELEMENT_SUBIDX, SUB_TYPE, sub_type{0x1234u});

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_up_req(dev, ARRAY_IDX, ELEMENT_SUBIDX, &mbuf,
                                 CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, ARRAY_IDX, ELEMENT_SUBIDX, 0, membuf_begin(&mbuf),
                     sizeof(sub_type), nullptr);
  CHECK_EQUAL(sizeof(sub_type), membuf_size(&mbuf));
  CHECK_EQUAL(0x1234u, LoadLE_U16(&mbuf));

  membuf_fini(&mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///        dictionary, the entry has an upload indication function set,
///        the function sets 0 as the requested size and a null pointer as
///        the next-bytes-to-download pointer
///
/// \When co_dev_up_req() is called with an index and a sub-index of the entry,
///       a pointer to the memory buffer to store the requested value and
///       a pointer to the confirmation function
///
/// \Then 0 is returned, the confirmation function is called with a null
///       pointer, the index and the sub-index of the entry, 0 as the abort
///       code, a pointer to the uploaded bytes, a number of the uploaded bytes
///       and a null user-specified data pointer; the error number is
///       not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls membuf_size()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_ReqZero) {
  co_sub_up_ind_t* const req_up_ind =
      [](const co_sub_t* const sub, co_sdo_req* const req, co_unsigned32_t ac,
         void*) -> co_unsigned32_t {
    co_sub_on_up(sub, req, &ac);
    req->buf = nullptr;
    req->size = 0;

    return 0;
  };
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, nullptr);

  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, &mbuf, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, SUBIDX, 0, membuf_begin(&mbuf),
                     sizeof(sub_type), nullptr);

  membuf_fini(&mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///        dictionary, the entry has an upload indication function set,
///        the function is unable to read any bytes from the buffer
///
/// \When co_dev_up_req() is called with an index and a sub-index of the entry,
///       a pointer to the memory buffer to store the requested value and
///       a pointer to the confirmation function
///
/// \Then 0 is returned, the confirmation function is called with a null
///       pointer, the index and the sub-index of the entry, CO_SDO_AC_NO_MEM
///       abort code, a null memory buffer pointer, 0 as a number of
///       the uploaded bytes and a null user-specified data pointer; the error
///       number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls co_sdo_req_last()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_NotAbleToComplete) {
  co_sub_up_ind_t* const req_up_ind =
      [](const co_sub_t* const sub, co_sdo_req* const req, co_unsigned32_t ac,
         void*) -> co_unsigned32_t {
    co_sub_on_up(sub, req, &ac);
    req->nbyte = 0;  // the function is unable to read any bytes from the buffer

    return 0;
  };

  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, nullptr);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, &mbuf, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_NO_MEM, nullptr, 0,
                     nullptr);

  membuf_fini(&mbuf);
}

#if LELY_NO_MALLOC
/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///        dictionary, the entry has an upload indication function set,
///        the function sets a custom memory buffer for bytes to be uploaded;
///        an external buffer which is too small to hold a requested value
///
/// \When co_dev_up_req() is called with an index and a sub-index of the entry,
///       the external memory buffer to store the requested value and a pointer
///       to the confirmation function
///
/// \Then 0 is returned, the memory buffer remains empty, the confirmation
///       function is called with a null pointer, the index and the sub-index of
///       the entry, CO_SDO_AC_NO_MEM abort code, a pointer to the memory
///       buffer, the number of the uploaded bytes and a null user-specified
///       data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls membuf_reserve()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_ExternalBufferTooSmall) {
  co_sub_up_ind_t* const req_up_ind =
      [](const co_sub_t* const sub, co_sdo_req* const req, co_unsigned32_t ac,
         void* const data) -> co_unsigned32_t {
    req->membuf = static_cast<membuf*>(data);

    co_sub_on_up(sub, req, &ac);

    return 0;
  };

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, &ind_mbuf);

  MembufInitSubType(&ind_mbuf);
  membuf ext_mbuf = MEMBUF_INIT;
  const size_t EXT_BUFSIZE = sizeof(sub_type) - 1u;
  char ext_buffer[EXT_BUFSIZE] = {0};
  membuf_init(&ext_mbuf, ext_buffer, EXT_BUFSIZE);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, &ext_mbuf, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_NO_MEM, nullptr, 0,
                     nullptr);

  membuf_fini(&ext_mbuf);
  membuf_fini(&ind_mbuf);
}
#endif

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///        dictionary, the entry has an upload indication function set,
///        the function provides no data on the first call and sets a custom
///        memory buffer for bytes to be uploaded
///
/// \When co_dev_up_req() is called with an index and a sub-index of the entry,
///       a pointer to the memory buffer to store the requested value and
///       a pointer to the confirmation function
///
/// \Then 0 is returned, the memory buffer contains the requested value,
///       the confirmation function is called with a null pointer, the index
///       and the sub-index of the entry, 0 as the abort code, a pointer to
///       the memory buffer, the number of the uploaded bytes and a null
///       user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls membuf_reserve()
///       \Calls membuf_size()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_ExternalBuffer_NoDataOnFirstCall) {
  co_sub_up_ind_t* const req_up_ind =
      [](const co_sub_t* const sub, co_sdo_req* const req, co_unsigned32_t ac,
         void* const data) -> co_unsigned32_t {
    const auto test_group = static_cast<TEST_GROUP_CppUTestGroupCO_Csdo*>(data);
    req->membuf = &test_group->ind_mbuf;

    co_sub_on_up(sub, req, &ac);
    if (test_group->num_called == 0) {
      req->nbyte = 0;
    }
    test_group->num_called++;

    return 0;
  };

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, this);

  MembufInitSubType(&ind_mbuf);
  membuf ext_mbuf = MEMBUF_INIT;
  MembufInitSubTypeExt(&ext_mbuf);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, &ext_mbuf, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, SUBIDX, 0, membuf_begin(&ext_mbuf),
                     sizeof(sub_type), nullptr);
  CHECK_EQUAL(sizeof(sub_type), membuf_size(&ext_mbuf));
  CHECK_EQUAL(0x1234u, LoadLE_U16(&ext_mbuf));

  membuf_fini(&ext_mbuf);
  membuf_fini(&ind_mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///       dictionary, the entry has an upload indication function set,
///       the function sets a custom memory buffer
///
/// \When co_dev_up_req() is called with an index and a sub-index of the entry,
///       a pointer to the memory buffer to store the requested value and
///       a pointer to the confirmation function
///
/// \Then 0 is returned, the memory buffer contains the requested value,
///       the confirmation function is called with a null pointer, the index
///       and the sub-index of the entry, 0 as the abort code, a pointer to
///       the memory buffer, the number of the uploaded bytes and a null
///       user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls membuf_reserve()
///       \Calls membuf_size()
///       \Calls membuf_write()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_ExternalBuffer) {
  co_sub_up_ind_t* const req_up_ind =
      [](const co_sub_t* const sub, co_sdo_req* const req, co_unsigned32_t ac,
         void* const data) -> co_unsigned32_t {
    req->membuf = static_cast<membuf*>(data);

    co_sub_on_up(sub, req, &ac);

    return 0;
  };

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, &ind_mbuf);

  MembufInitSubType(&ind_mbuf);
  membuf ext_mbuf = MEMBUF_INIT;
  MembufInitSubTypeExt(&ext_mbuf);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, &ext_mbuf, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, SUBIDX, 0, membuf_begin(&ext_mbuf),
                     sizeof(sub_type), nullptr);
  CHECK_EQUAL(sizeof(sub_type), membuf_size(&ext_mbuf));
  CHECK_EQUAL(0x1234u, LoadLE_U16(&ext_mbuf));

  membuf_fini(&ext_mbuf);
  membuf_fini(&ind_mbuf);
}

/// \Given a pointer to the device (co_dev_t) containing an entry in the object
///        dictionary, the entry has the default upload indication function set
///
/// \When co_dev_up_req() is called with an index and a sub-index of the entry,
///       a pointer to the memory buffer to store the requested value and
///       a pointer to the confirmation function
///
/// \Then 0 is returned, the memory buffer contains the requested value,
///       the confirmation function is called with a null pointer, the index and
///       the sub-index of the entry, 0 as the abort code, a pointer to
///       the memory buffer, the number of the uploaded bytes and a null
///       user-specified data pointer; the error number is not changed
///       \Calls get_errc()
///       \IfCalls{LELY_NO_MALLOC, membuf_init()}
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_begin()
///       \Calls co_sdo_req_last()
///       \Calls membuf_size()
///       \Calls co_sdo_req_fini()
///       \Calls membuf_fini()
///       \Calls set_errc()
TEST(CO_Csdo, CoDevUpReq_Nominal) {
  membuf mbuf = MEMBUF_INIT;
  MembufInitSubType(&mbuf);

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret =
      co_dev_up_req(dev, IDX, SUBIDX, &mbuf, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CoCsdoUpCon::Check(nullptr, IDX, SUBIDX, 0, membuf_begin(&mbuf),
                     sizeof(sub_type), nullptr);
  CHECK_EQUAL(sizeof(sub_type), membuf_size(&mbuf));
  CHECK_EQUAL(0x1234u, LoadLE_U16(&mbuf));

  membuf_fini(&mbuf);
}

///@}

/// @name co_csdo_dn_req()
///@{

namespace CoCsdoUpDnReq {
static void
SetOneSecOnNet(can_net_t* const net) {
  const timespec ts = {1u, 0u};
  can_net_set_time(net, &ts);
}

static void
AbortTransfer(can_net_t* const net, const co_unsigned32_t can_id) {
  const can_msg msg = SdoCreateMsg::Abort(0, 0, can_id, CO_SDO_AC_HARDWARE);
  can_net_recv(net, &msg, 0);
}
}  // namespace CoCsdoUpDnReq

/// \Given a pointer to the CSDO service (co_csdo_t) which is not idle,
///        the object dictionary contains an entry
///
/// \When co_csdo_dn_req() is called with an index and a sub-index of the entry,
///       a pointer to the bytes to be downloaded, a size of the entry,
///       a download confirmation function and a null user-specified data
///       pointer
///
/// \Then -1 is returned, ERRNUM_INVAL is set as the error number, CAN message
///       is not sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls set_errnum()
TEST(CO_Csdo, CoCsdoDnReq_ServiceIsBusy) {
  CHECK_FALSE(co_csdo_is_idle(csdo));

  const uint_least8_t buffer[sizeof(sub_type)] = {0};
  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, sizeof(sub_type),
                                  CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with a timeout set,
///        the object dictionary contains an entry
///
/// \When co_csdo_dn_req() is called with an index and a sub-index of the entry,
///       a pointer to the bytes to be downloaded, a size of the entry,
///       a download confirmation function and a null user-specified data
///       pointer
///
/// \Then 0 is returned, the error number is not changed, expedited download
///       initiate request is sent to the server;
///       after the timeout value elapses and no response from the server
///       is received - the timeout message is sent;
///       when the abort transfer message is received the download confirmation
///       function is called
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_init()
///       \Calls can_timer_timeout()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoDnReq_TimeoutSet) {
  StartCSDO();
  co_csdo_set_timeout(csdo, 999);  // 999 ms

  uint_least8_t buffer[CO_SDO_INI_DATA_SIZE] = {0};
  stle_u16(buffer, 0x1234u);

  const CanSend::MsgSeq expected_msg_seq = {
      SdoCreateMsg::DnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ, buffer,
                             CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type))),
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, CO_SDO_AC_TIMEOUT)};
  CanSend::SetCheckSeq(expected_msg_seq);

  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, sizeof(sub_type),
                                  CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());

  CHECK_EQUAL(1u, CanSend::GetNumCalled());

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CHECK_EQUAL(2u, CanSend::GetNumCalled());

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t), the object dictionary
///        contains an entry
///
/// \When co_csdo_dn_req() is called with an index and a sub-index of the entry,
///       a pointer to the bytes to be downloaded, a size equal to zero,
///       a download confirmation function and a null user-specified data
///       pointer
///
/// \Then 0 is returned, the error number is not changed, download initiate
///       request is sent to the server, when the abort transfer message is
///       received the download confirmation function is called
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_init()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoDnReq_SizeZero) {
  StartCSDO();

  const uint_least8_t buffer_size = 0;
  const uint_least8_t* const buffer = nullptr;

  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, buffer_size,
                                  CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());

  const auto expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND, IDX, SUBIDX, buffer_size);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t), the object dictionary
///        contains an entry
///
/// \When co_csdo_dn_req() is called with an index and a sub-index of the entry,
///       a pointer to the bytes to be downloaded, a size of the entry greater
///       than the expedited transfer maximum size, a download confirmation
///       function and a null user-specified data pointer
///
/// \Then 0 is returned, the error number is not changed, download initiate
///       request is sent to the server, when the abort transfer message
///       is received the download confirmation function is called
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_init()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoDnReq_DownloadInitiate) {
  StartCSDO();

  const uint_least8_t buffer_size = 10u;

  const uint_least8_t buffer[buffer_size] = {0};
  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, buffer_size,
                                  CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());

  const auto expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND, IDX, SUBIDX, buffer_size);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t), the object dictionary
///        contains an entry
///
/// \When co_csdo_dn_req() is called with an index and a sub-index of the entry,
///       a pointer to the bytes to be downloaded, a size of the entry,
///       a download confirmation function and a null user-specified data
///       pointer
///
/// \Then 0 is returned, the error number is not changed, expedited download
///       initiate request is sent to the server, when the abort transfer
///       message is received the download confirmation function is called
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
///       \Calls stle_u16()
///       \Calls memcpy()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoDnReq_Expedited) {
  StartCSDO();

  uint_least8_t buffer[sizeof(sub_type)] = {0};
  stle_u16(buffer, 0x1234u);
  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, sizeof(sub_type),
                                  CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());

  const auto expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type)), IDX,
      SUBIDX, 0x1234u);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
}

///@}

/// @name co_csdo_dn_val_req()
///@{

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_sdo_dn_val_req() is called with an index, a subindex, a valid type
///       of the value, a value, a null buffer pointer, a pointer to
///       the confirmation function and a null user-specified data pointer
///
/// \Then 0 is returned and the request is sent
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls co_val_write()
///       \Calls co_csdo_dn_req()
TEST(CO_Csdo, CoCsdoDnValReq_Nominal) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  const auto ret = co_csdo_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                      nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_EXP_SET(sizeof(co_unsigned16_t)),
      IDX, SUBIDX, VAL);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

#if HAVE_LELY_OVERRIDE

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_sdo_dn_val_req() is called with an index, a subindex, a valid type
///       of the value, a value, a null buffer pointer, a pointer to
///       the confirmation function and a null user-specified data pointer, but
///       the first internal call to co_val_write() fails
///
/// \Then -1 is returned and the request is not sent
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
TEST(CO_Csdo, CoCsdoDnValReq_CoValWriteFail) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  LelyOverride::co_val_write(Override::NoneCallsValid);
  const auto ret = co_csdo_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                      nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_sdo_dn_val_req() is called with an index, a subindex, a valid type
///       of the value, a value, a null buffer pointer, a pointer to
///       the confirmation function and a null user-specified data pointer, but
///       the second internal call to co_val_write() fails
///
/// \Then -1 is returned and the request is not sent
///       \Calls co_val_write()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_alloc()
TEST(CO_Csdo, CoCsdoDnValReq_SecondCoValWriteFail) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  LelyOverride::co_val_write(1u);
  const auto ret = co_csdo_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                      nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_sdo_dn_val_req() is called with an index, a subindex, a valid array
///       type, an empty array, a null buffer pointer, a pointer to
///       the confirmation function and a null user-specified data pointer, but
///       the first internal call to co_val_write() fails
///
/// \Then 0 is returned and the empty request is sent
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_alloc()
///       \Calls co_csdo_dn_req()
TEST(CO_Csdo, CoCsdoDnValReq_SizeofZero) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  LelyOverride::co_val_write(Override::NoneCallsValid);
  const auto ret = co_csdo_dn_val_req(csdo, IDX, SUBIDX, val_os.GetDataType(),
                                      val_os.GetValPtr(), nullptr,
                                      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND, IDX, SUBIDX, 0);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

#endif  // HAVE_LELY_OVERRIDE

#if LELY_NO_MALLOC
/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_sdo_dn_val_req() is called with an index, a subindex, a valid type
///       of the value, a value, an empty memory buffer pointer, a pointer to
///       the confirmation function and a null user-specified data
///
/// \Then -1 is returned and the request is not sent
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
TEST(CO_Csdo, CoCsdoDnValReq_EmptyExternalBuffer) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  membuf mbuf = MEMBUF_INIT;
  const auto ret = co_csdo_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL, &mbuf,
                                      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}
#endif

///@}

TEST_GROUP_BASE(CO_CsdoDnDcf, CO_CsdoBase) {
  static const co_unsigned16_t SUB_TYPE = CO_DEFTYPE_UNSIGNED16;
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;
  const sub_type VAL = 0xabcdu;

  std::unique_ptr<CoObjTHolder> obj;
  ConciseDcf dcf = ConciseDcf::MakeForEntries<sub_type>();

  void RestartCsdo() const {
    co_csdo_stop(csdo);
    co_csdo_start(csdo);
  }

  void CheckDcfReadFailure(const co_unsigned16_t idx,
                           const co_unsigned8_t subidx,
                           const errnum_t error_num) const {
    CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
    CoCsdoDnCon::Check(csdo, idx, subidx, CO_SDO_AC_TYPE_LEN_LO, nullptr);
    CHECK_EQUAL(error_num, get_errnum());
    CHECK_EQUAL(0u, CanSend::GetNumCalled());
  }

  TEST_SETUP() override {
    TEST_BASE_SETUP();

    dev_holder->CreateAndInsertObj(obj, IDX);
    obj->InsertAndSetSub(SUBIDX, SUB_TYPE, VAL);

    CHECK_EQUAL(dcf.Size(),
                co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));

    co_csdo_start(csdo);
    CanSend::Clear();
  }
};

/// @name co_csdo_dn_dcf_req()
///@{

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries; a concise DCF buffer
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer, a pointer to the end of the buffer,
///       a pointer to the confirmation function and a null user-specified data
///       pointer
///
/// \Then 0 is returned, confirmation function is not called, error number is
///       not changed, expedited download request with the requested values is
///       sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
///       \Calls co_csdo_dn_req()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_Nominal) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_csdo_dn_dcf_req(csdo, dcf.Begin(), dcf.End(),
                                      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, CoCsdoDnCon::GetNumCalled());
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type)), IDX,
      SUBIDX, VAL);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0u, CO_SDO_MSG_SIZE, expected.data());

  // complete started SDO download transfer
  const can_msg dn_ini_res =
      SdoCreateMsg::DnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  const can_msg dn_seg_res = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &dn_ini_res, 0));
  CHECK_EQUAL(1, can_net_recv(net, &dn_seg_res, 0));
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, 0u, nullptr);
}

/// \Given a pointer to a CSDO service (co_csdo_t) with an invalid server
///        parameter "COB-ID client -> server (rx)" and a valid
///        "COB-ID server -> client (tx)" entries; a concise DCF buffer
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer, a pointer to the end of the buffer,
///       a pointer to the confirmation function and a null user-specified data
///       pointer
///
/// \Then -1 is returned, confirmation function is not called, ERRNUM_INVAL
///       is set as the error number, expedited download request is not sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls set_errnum()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_InvalidCobidReq) {
  SetCli01CobidReq(DEFAULT_COBID_REQ | CO_SDO_COBID_VALID);
  RestartCsdo();

  const auto ret = co_csdo_dn_dcf_req(csdo, dcf.Begin(), dcf.End(),
                                      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, CoCsdoDnCon::GetNumCalled());
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries; an SDO transfer is in progress
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, pointers
///       to the beginning and the end of a buffer containing a concise DCF,
///       a pointer to the confirmation function and a null user-specified data
///       pointer
///
/// \Then -1 is returned, confirmation function is not called, ERRNUM_INVAL
///       is set as the error number, no SDO message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls set_errnum()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_IsNotIdle) {
  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, 0xffffu, 0xffu, 0, nullptr, nullptr,
                                    nullptr));
  CanSend::Clear();

  const auto ret = co_csdo_dn_dcf_req(csdo, dcf.Begin(), dcf.End(),
                                      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, CoCsdoDnCon::GetNumCalled());
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer containing a concise DCF and a pointer
///       to the incorrect end of the buffer (incomplete total number of
///       sub-indices), a pointer to the confirmation function and a null
///       user-specified data pointer
///
/// \Then 0 is returned, confirmation function is called once with a pointer to
///       the service, an index and a sub-index equal to 0,
///       CO_SDO_AC_TYPE_LEN_LO as the abort code and a null pointer; error
///       number is not changed, no SDO message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_NumEntriesReadFailure) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_csdo_dn_dcf_req(
      csdo, dcf.Begin(), dcf.Begin() + sizeof(co_unsigned32_t) - 1u,
      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDcfReadFailure(0x0000u, 0x00u, error_num);
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer containing a concise DCF and a pointer
///       to the incorrect end of the buffer (incomplete total number of
///       sub-indices), a null confirmation function pointer and a null
///       user-specified data pointer
///
/// \Then 0 is returned, error number is not changed, no SDO message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_NumEntriesReadFailure_NoConFunc) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_csdo_dn_dcf_req(
      csdo, dcf.Begin(), dcf.Begin() + sizeof(co_unsigned32_t) - 1u, nullptr,
      nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer containing a concise DCF and a pointer
///       to the incorrect end of the buffer (incomplete object index), a
///       pointer to the confirmation function and a null user-specified data
///       pointer
///
/// \Then 0 is returned, confirmation function is called once with a pointer to
///       the service, an index and a sub-index equal to 0,
///       CO_SDO_AC_TYPE_LEN_LO as the abort code and a null pointer; error
///       number is not changed, no SDO message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_IndexReadFailure) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_csdo_dn_dcf_req(
      csdo, dcf.Begin(),
      dcf.Begin() + sizeof(co_unsigned32_t) + sizeof(co_unsigned16_t) - 1u,
      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDcfReadFailure(0x0000u, 0x00u, error_num);
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer containing a concise DCF and a pointer
///       to the incorrect end of the buffer (incomplete sub-index), a pointer
///       to the confirmation function and a null user-specified data pointer
///
/// \Then 0 is returned, confirmation function is called once with a pointer to
///       the service, an index and a sub-index equal to 0,
///       CO_SDO_AC_TYPE_LEN_LO as the abort code and a null pointer; error
///       number is not changed, no SDO message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_SubIndexReadFailure) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_csdo_dn_dcf_req(csdo, dcf.Begin(),
                                      dcf.Begin() + sizeof(co_unsigned32_t) +
                                          sizeof(co_unsigned16_t) +
                                          sizeof(co_unsigned8_t) - 1u,
                                      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDcfReadFailure(IDX, 0x00u, error_num);
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer containing a concise DCF and a pointer
///       to the incorrect end of the buffer (incomplete entry size), a pointer
///       to the confirmation function and a null user-specified data pointer
///
/// \Then 0 is returned, confirmation function is called once with a pointer to
///       the service, an index and a sub-index equal to 0,
///       CO_SDO_AC_TYPE_LEN_LO as the abort code and a null pointer; error
///       number is not changed, no SDO message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_SizeReadFailure) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_csdo_dn_dcf_req(
      csdo, dcf.Begin(),
      dcf.Begin() + sizeof(co_unsigned32_t) + sizeof(co_unsigned16_t) +
          sizeof(co_unsigned8_t) + sizeof(co_unsigned32_t) - 1u,
      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDcfReadFailure(IDX, SUBIDX, error_num);
}

/// \Given a pointer to a CSDO service (co_csdo_t) with a valid server parameter
///        "COB-ID client -> server (rx)" and "COB-ID server -> client (tx)"
///        entries
///
/// \When co_csdo_dn_dcf_req() is called with the pointer to the CSDO, a pointer
///       to the beginning of the buffer containing a concise DCF and a pointer
///       to the incorrect end of the buffer (incomplete entry value), a pointer
///       to the confirmation function and a null user-specified data pointer
///
/// \Then 0 is returned, confirmation function is called once with a pointer to
///       the service, an index and a sub-index equal to 0,
///       CO_SDO_AC_TYPE_LEN_LO as the abort code and a null pointer; error
///       number is not changed, no SDO message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls co_val_read()
TEST(CO_CsdoDnDcf, CoCsdoDnDcfReq_ValueReadFailure) {
  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_csdo_dn_dcf_req(
      csdo, dcf.Begin(),
      dcf.Begin() + sizeof(co_unsigned32_t) + sizeof(co_unsigned16_t) +
          sizeof(co_unsigned8_t) + sizeof(co_unsigned32_t) + sizeof(VAL) - 1u,
      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDcfReadFailure(IDX, SUBIDX, error_num);
}
///@}

/// @name co_csdo_up_req()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which is not started
///
/// \When co_csdo_up_req() is called with a pointer to the CSDO service,
///       an index, a sub-index, a null buffer pointer, a pointer to
///       the confirmation function and a null user-specified data pointer
///
/// \Then -1 is returned, ERRNUM_INVAL is set as an error number and no SDO
///       message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls set_errnum()
TEST(CO_Csdo, CoCsdoUpReq_ServiceNotStarted) {
  const auto ret =
      co_csdo_up_req(csdo, IDX, SUBIDX, nullptr, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with no ongoing transfer
///
/// \When co_csdo_up_req() is called with a pointer to the CSDO service,
///       an index, a sub-index, a null buffer pointer, a pointer to
///       the confirmation function and a null user-specified data pointer
///
/// \Then 0 is returned, the error number is not changed and the upload request
///       is sent to the server
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_clear()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoUpReq_Nominal) {
  StartCSDO();

  const errnum_t error_number = get_errnum();
  const auto ret =
      co_csdo_up_req(csdo, IDX, SUBIDX, nullptr, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_number, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::Empty(CO_SDO_CCS_UP_INI_REQ, IDX, SUBIDX);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with no ongoing transfer,
///        the timeout of the service was set
///
/// \When co_csdo_up_req() is called with a pointer to the CSDO service,
///       an index, a sub-index, a null buffer pointer, a pointer to
///       the confirmation function and a null user-specified data pointer
///
/// \Then 0 is returned, the error number is not changed and the upload request
///       is sent to the server; when the timeout expired, an SDO abort
///       transfer message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_clear()
///       \Calls can_timer_timeout()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoUpReq_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);
  StartCSDO();

  const errnum_t error_number = get_errnum();
  const auto ret =
      co_csdo_up_req(csdo, IDX, SUBIDX, nullptr, CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_number, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::Empty(CO_SDO_CCS_UP_INI_REQ, IDX, SUBIDX);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
}

///@}

/// @name co_csdo_blk_up_req()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which is not started
///
/// \When co_csdo_blk_up_req() is called with an index, a sub-index, 0 protocol
///       switch threshold, null buffer pointer, a pointer to the confirmation
///       function and a null user-specified data pointer
///
/// \Then -1 is returned, ERRNUM_INVAL is set as the error number and no SDO
///       message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls set_errnum()
TEST(CO_Csdo, CoCsdoBlkUpReq_ServiceNotStarted) {
  const auto ret = co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                      CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with no ongoing transfer
///
/// \When co_csdo_blk_up_req() is called with an index, a sub-index, 0 protocol
///       switch threshold, null buffer pointer, a pointer to the confirmation
///       function and a null user-specified data pointer
///
/// \Then 0 is returned, the error number is not changed and a block upload
///       request is sent to the server
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_clear()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpReq_Nominal) {
  StartCSDO();

  const errnum_t error_number = get_errnum();
  const auto ret = co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                      CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_number, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with no ongoing transfer,
///       the timeout of the service was set
///
/// \When co_csdo_blk_up_req() is called with an index, a sub-index, 0 protocol
///       switch threshold, null buffer pointer, a pointer to the confirmation
///       function and a null user-specified data pointer
///
/// \Then 0 is returned, the error number is not changed and a block upload
///       request is sent to the server; when the timeout expired, an SDO abort
///       transfer message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_clear()
///       \Calls can_timer_timeout()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpReq_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);
  StartCSDO();

  const errnum_t error_number = get_errnum();
  const auto ret = co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                      CoCsdoUpCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_number, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
}

///@}

/// @name CSDO: receive when idle
///@{

/// \Given a pointer to the started CSDO service (co_csdo_t) which has not
///        initiated a transfer
///
/// \When an SDO message with length zero is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoWaitOnRecv_NoCs) {
  StartCSDO();

  can_msg msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(0u, 0u, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) which has not
///        initiated a transfer
///
/// \When an SDO message with an incorrect command specifier is received
///
/// \Then no SDO message is sent
TEST(CO_Csdo, CoCsdoWaitOnRecv_IncorrectCs) {
  StartCSDO();

  can_msg msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.data[0] |= 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to the started CSDO service (co_csdo_t) which has not
///        initiated a transfer
///
/// \When an abort transfer SDO message with a zero abort code is received
///
/// \Then no SDO message is sent
///       \Calls ldle_u32()
TEST(CO_Csdo, CoCsdoWaitOnRecv_AbortCs_ZeroAc) {
  StartCSDO();

  const can_msg msg = SdoCreateMsg::Abort(0xffffu, 0xffu, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to the started CSDO service (co_csdo_t) which has not
///        initiated a transfer
///
/// \When an abort transfer SDO message with an incomplete abort code is
///       received
///
/// \Then no SDO message is sent
TEST(CO_Csdo, CoCsdoWaitOnRecv_AbortCs_IncompleteAc) {
  StartCSDO();

  can_msg msg = SdoCreateMsg::Abort(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

///@}

/// @name CSDO: block upload initiate on receive
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When a correct block upload initiate response is received
///
/// \Then an SDO message with a client command specifier block upload request
///       and subcommand start upload is sent
///       \Calls ldle_u16()
///       \Calls memcpy()
///       \Calls ldle_u32()
///       \Calls membuf_reserve()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_Nominal) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res = SdoCreateMsg::BlkUpIniRes(
      IDX, SUBIDX, DEFAULT_COBID_RES, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO message with a length 0 is received
///
/// \Then an abort transfer SDO message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_NoCS) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg_res.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When a correct upload initiate response is received
///
/// \Then an SDO message with a client command specifier upload segment request
///       is sent
///       \Calls ldle_u16()
///       \Calls memcpy()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_ProtocolSwitch) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res =
      SdoCreateMsg::UpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res = SdoInitExpectedData::U32(CO_SDO_CCS_UP_SEG_REQ);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When a correct block upload initiate response is received with a data size
///       set to 0
///
/// \Then an SDO message with a client command specifier block upload request
///       and subcommand start upload is sent
///       \Calls ldle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_Nominal_SizeIsZero) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res =
      SdoCreateMsg::BlkUpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES, 0);
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an abort transfer SDO message with a non-zero abort code is received
///
/// \Then no SDO message is sent, upload confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, the received
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
///       \Calls ldle_u32()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_AcNonZero) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, &data));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_NO_READ);
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_NO_READ, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an abort transfer SDO message with abort code equal to zero is
///       received
///
/// \Then no SDO message is sent, upload confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, CO_SDO_AC_ERROR
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
///       \Calls ldle_u32()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_AcZero) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, &data));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, 0);
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an abort transfer SDO message with no abort code is received
///
/// \Then no SDO message is sent, upload confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, CO_SDO_AC_ERROR
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_AcNone) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, &data));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg_res = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg_res.len = 4u;
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO message with an incorrect command specifier is received
///
/// \Then an abort transfer SDO message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_IncorrectCS) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, &data));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg_res.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_NO_CS, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO block upload initiate response with an incorrect server
///       subcommand is received
///
/// \Then an abort transfer SDO message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_IncorrectSC) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, &data));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg_res = SdoCreateMsg::BlkUpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES,
                                              sizeof(sub_type));
  msg_res.data[0] |= 0x01u;
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_NO_CS, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO block upload initiate response with too little bytes is
///       received
///
/// \Then an abort transfer SDO message with CO_SDO_AC_ERROR abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_TooShortMsg) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, &data));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg_res = SdoCreateMsg::BlkUpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES,
                                              sizeof(sub_type));
  msg_res.len = 3u;
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_ERROR);
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO block upload reponse with an index not matching the requested
///       index
///
/// \Then an abort transfer SDO message with CO_SDO_AC_ERROR abort code is sent
///       \Calls ldle_u16()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_IncorrectIdx) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res = SdoCreateMsg::BlkUpIniRes(
      0xffffu, SUBIDX, DEFAULT_COBID_RES, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_ERROR);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO block upload reponse with a sub-index not matching
///       the requested sub-index
///
/// \Then an abort transfer SDO message with CO_SDO_AC_ERROR abort code is sent
///       \Calls ldle_u16()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_IncorrectSubidx) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res = SdoCreateMsg::BlkUpIniRes(
      IDX, 0xffu, DEFAULT_COBID_RES, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_ERROR);
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When a correct block upload initiate response is received, but the internal
///       call to membuf_reserve() fails
///
/// \Then an abort transfer SDO message with CO_SDO_AC_NO_MEM abort code is sent
///       \Calls ldle_u16()
///       \Calls memcpy()
///       \Calls ldle_u32()
///       \Calls membuf_reserve()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_MembufReserveFail) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  LelyOverride::membuf_reserve(Override::NoneCallsValid);

  const can_msg msg_res = SdoCreateMsg::BlkUpIniRes(
      IDX, SUBIDX, DEFAULT_COBID_RES, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_MEM);
}
#endif  // HAVE_LELY_OVERRIDE

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client);
///        the service has a timeout set
///
/// \When a correct block upload initiate response is received
///
/// \Then an SDO message with a client command specifier block upload request
///       and subcommand start upload is sent
///       \Calls ldle_u16()
///       \Calls memcpy()
///       \Calls ldle_u32()
///       \Calls membuf_reserve()
///       \Calls can_timer_timeout()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);  // 999 ms
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  const can_msg msg_res = SdoCreateMsg::BlkUpIniRes(
      IDX, SUBIDX, DEFAULT_COBID_RES, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
}

///@}

/// @name CSDO block upload sub-block
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) with a timeout set,
///        the service has initiated block upload transfer
///        (the correct request was sent by the client)
///
/// \When the Client-SDO timeout expires before receiving the next SDO message
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TIMEOUT abort code is
///       sent
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_TimeoutTriggered) {
  co_csdo_set_timeout(csdo, 999);
  StartCSDO();
  InitiateBlockUploadRequest();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO message with a length 0 is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is
///       sent
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_NoCs) {
  StartCSDO();
  InitiateBlockUploadRequest();

  can_msg msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.len = 0;
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an abort transfer SDO message with a non-zero abort code is received
///
/// \Then no SDO message is sent, upload confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, the received
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_AbortNonZero) {
  StartCSDO();
  InitiateBlockUploadRequest();

  const can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_HARDWARE);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_HARDWARE, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an abort transfer SDO message with an abort code equal to zero
///       is received
///
/// \Then no SDO message is sent, upload confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, CO_SDO_AC_ERROR
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_AbortZero) {
  StartCSDO();
  InitiateBlockUploadRequest();

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO abort transfer message is received, but does not contain
///       the abort code
///
/// \Then no SDO message is sent, upload confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, CO_SDO_AC_ERROR
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_IncompleteAbortCode) {
  StartCSDO();
  InitiateBlockUploadRequest();

  can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.len = CO_SDO_MSG_SIZE - 1u;
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoUpCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, nullptr, 0, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client); the
///        service has a timeout set
///
/// \When the timeout expires after the reception of the first segment
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TIMEOUT abort code is
///       sent
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);
  const co_unsigned8_t subidx_u64 = SUBIDX + 1u;
  obj2020->InsertAndSetSub(subidx_u64, SUB_TYPE64, co_unsigned64_t{0});
  StartCSDO();
  InitiateBlockUploadRequest(IDX, subidx_u64, sizeof(sub_type64));

  co_unsigned8_t seqno = 1u;
  const can_msg msg = SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno,
                                                val_u64.GetFirstSegment());
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  ++seqno;
  can_msg last_msg = SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno,
                                               val_u64.GetLastSegment());
  last_msg.data[0] |= CO_SDO_SEQ_LAST;
  CHECK_EQUAL(1u, can_net_recv(net, &last_msg, 0));

  CheckSdoAbortSent(IDX, subidx_u64, CO_SDO_AC_TIMEOUT);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO segment with a sequence number equal to the number of segments
///       in a block is received
///
/// \Then an SDO message with a client command specifier block upload request
///       and subcommand block response is sent
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_LastSegmentInBlock) {
  const co_unsigned8_t subidx_u64 = SUBIDX + 1u;
  obj2020->InsertAndSetSub(subidx_u64, SUB_TYPE64, co_unsigned64_t{0});
  StartCSDO();
  InitiateBlockUploadRequest(IDX, subidx_u64, sizeof(sub_type64));

  const co_unsigned8_t seqno = CO_SDO_MAX_SEQNO;
  const can_msg msg =
      SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno, {0, 0, 0, 0});
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1, CanSend::GetNumCalled());
  auto expected =
      SdoInitExpectedData::Empty(CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES);
  expected[1] = 0;
  expected[2] = CO_SDO_MAX_SEQNO;
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an SDO segment with too many bytes for a requested entry but
///       with CO_SDO_LAST flag not set
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TYPE_LEN_HI abort code
///       is sent
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_NotLastButTooManyBytes) {
  StartCSDO();
  InitiateBlockUploadRequest();

  const co_unsigned8_t seqno = 1u;
  const can_msg msg =
      SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno, {0, 0, 0, 0});
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TYPE_LEN_HI);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When all required SDO segments are received; between them a segment with
///       an incorrect sequence number is received
///
/// \Then an SDO message with a client command specifier block upload request
///       and subcommand block upload response, last received sequence number
///       and the block size is sent
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_IncorrectSeqno) {
  const co_unsigned8_t subidx_u64 = SUBIDX + 1u;
  obj2020->InsertAndSetSub(subidx_u64, SUB_TYPE64, co_unsigned64_t{0});
  StartCSDO();
  InitiateBlockUploadRequest(IDX, subidx_u64, sizeof(sub_type64));

  co_unsigned8_t seqno = 1u;
  const can_msg msg = SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno,
                                                val_u64.GetFirstSegment());
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  const co_unsigned8_t incorrect_seqno = 126u;
  const can_msg faulty_msg = SdoCreateMsg::BlkUpSegReq(
      DEFAULT_COBID_RES, incorrect_seqno,
      std::vector<uint_least8_t>(CO_SDO_SEG_MAX_DATA_SIZE, 0));
  CHECK_EQUAL(1u, can_net_recv(net, &faulty_msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  ++seqno;
  can_msg last_msg = SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno,
                                               val_u64.GetLastSegment());
  last_msg.data[0] |= CO_SDO_SEQ_LAST;
  CHECK_EQUAL(1u, can_net_recv(net, &last_msg, 0));

  CHECK_EQUAL(1, CanSend::GetNumCalled());
  auto last_expected =
      SdoInitExpectedData::Empty(CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES);
  last_expected[1] = seqno;
  last_expected[2] = CO_SDO_MAX_SEQNO;
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    last_expected.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When all required SDO segments are received
///
/// \Then an SDO message with a client command specifier block upload request
///       and a subcommand block upload response, last received sequence number
///       and the block size is sent; custom CSDO block upload indication
///       function is called with a user-specified data pointer
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_Nominal) {
  const co_unsigned8_t subidx_u64 = SUBIDX + 1u;
  obj2020->InsertAndSetSub(subidx_u64, SUB_TYPE64, co_unsigned64_t{0});
  char user_specified_data = 'a';
  co_csdo_set_up_ind(csdo, CoCsdoInd::Func, &user_specified_data);
  StartCSDO();

  InitiateBlockUploadRequest(IDX, subidx_u64, sizeof(sub_type64));
  CHECK_EQUAL(1, CoCsdoInd::GetNumCalled());
  CoCsdoInd::CheckAndClear(csdo, IDX, subidx_u64, sizeof(sub_type64), 0,
                           &user_specified_data);

  co_unsigned8_t seqno = 1u;
  const can_msg msg = SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno,
                                                val_u64.GetFirstSegment());
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoCsdoInd::GetNumCalled());

  ++seqno;
  can_msg last_msg = SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, seqno,
                                               val_u64.GetLastSegment());
  last_msg.data[0] |= CO_SDO_SEQ_LAST;
  CHECK_EQUAL(1u, can_net_recv(net, &last_msg, 0));

  CHECK_EQUAL(1, CanSend::GetNumCalled());
  auto last_expected =
      SdoInitExpectedData::Empty(CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES);
  last_expected[1] = seqno;
  last_expected[2] = CO_SDO_MAX_SEQNO;
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    last_expected.data());

  CHECK_EQUAL(1, CoCsdoInd::GetNumCalled());
  CoCsdoInd::CheckAndClear(csdo, IDX, subidx_u64, sizeof(sub_type64),
                           sizeof(sub_type64), &user_specified_data);
}

///@}

/// @name CSDO block upload end
///@{

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnTime_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);
  StartCSDO();
  AdvanceToBlkUpEndState();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_NoCs) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  auto msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_CsAbort_ZeroAc) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  const auto msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_CsAbort_AcNonZero) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  const auto msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_HARDWARE);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_CsAbort_IncompleteAc) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  auto msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_HARDWARE);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_IncorrectCs) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  auto msg = SdoCreateMsg::BlkUpRes(DEFAULT_COBID_RES);
  msg.data[0] |= 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_IncorrectSc) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  auto msg = SdoCreateMsg::BlkUpRes(DEFAULT_COBID_RES);
  msg.data[0] |= 0x03u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_CheckCrcIncorrect) {
  StartCSDO();
  InitiateBlockUploadRequestWithCrc();
  ReceiveBlkUpSegReq();
  CanSend::Clear();

  auto msg = SdoCreateMsg::BlkUpRes(DEFAULT_COBID_RES, sizeof(sub_type),
                                    CO_SDO_SC_END_BLK);
  stle_u16(msg.data + 1u, 0xffffu);  // incorrect CRC
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_BLK_CRC);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_CheckCrcCorrect) {
  StartCSDO();
  InitiateBlockUploadRequestWithCrc();
  ReceiveBlkUpSegReq();
  CanSend::Clear();

  auto msg = SdoCreateMsg::BlkUpRes(DEFAULT_COBID_RES, sizeof(sub_type),
                                    CO_SDO_SC_END_BLK);
  stle_u16(msg.data + 1u, 0xfb22u);  // correct CRC for the example value
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_msg =
      SdoCreateMsg::BlkUpReq(DEFAULT_COBID_REQ, CO_SDO_SC_END_BLK);
  CanSend::CheckMsg(expected_msg);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_EndBlkWithoutSegment) {
  StartCSDO();
  InitiateBlockUploadRequest();

  const auto msg = SdoCreateMsg::BlkUpRes(
      DEFAULT_COBID_RES, sizeof(co_unsigned8_t), CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CanSend::Clear();

  const auto msg2 = SdoCreateMsg::BlkUpRes(
      DEFAULT_COBID_RES, sizeof(co_unsigned8_t), CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg2, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TYPE_LEN_LO);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_RequestZeroBytes) {
  StartCSDO();
  InitiateBlockUploadRequest(IDX, SUBIDX, 0u);
  ReceiveBlkUpSegReq();
  CanSend::Clear();

  const auto msg =
      SdoCreateMsg::BlkUpRes(DEFAULT_COBID_RES, 0, CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_msg =
      SdoCreateMsg::BlkUpReq(DEFAULT_COBID_REQ, CO_SDO_SC_END_BLK);
  CanSend::CheckMsg(expected_msg);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_NoBytesInLastSegment) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  const auto msg =
      SdoCreateMsg::BlkUpRes(DEFAULT_COBID_RES, 0, CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkUpEndOnRecv_Nominal) {
  StartCSDO();
  AdvanceToBlkUpEndState();

  const auto msg = SdoCreateMsg::BlkUpRes(DEFAULT_COBID_RES, sizeof(sub_type),
                                          CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_msg =
      SdoCreateMsg::BlkUpReq(DEFAULT_COBID_REQ, CO_SDO_SC_END_BLK);
  CanSend::CheckMsg(expected_msg);
}

///@}

/// @name co_csdo_blk_dn_req()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which is not started
///
/// \When co_csdo_blk_dn_req() is called with an index and a sub-index of
///       the entry to download, a pointer to the bytes to be downloaded, size
///       of the entry, a pointer to the confirmation function and a null
///       user-specified data pointer
///
/// \Then -1 is returned and SDO message is not sent, the ERRNUM_INVAL is set
///       as an error number
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls set_errnum()
TEST(CO_Csdo, CoCsdoBlkDnReq_NotStarted) {
  const uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_req() is called with an index and a sub-index of
///       the entry to download, a pointer to the bytes to be downloaded, size
///       of the entry, a pointer to the confirmation function and a null
///       user-specified data pointer
///
/// \Then 0 is returned and SDO block download request is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_init()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnReq_Nominal) {
  StartCSDO();

  const uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CanSend::GetNumCalled());
  const co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                            CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;
  const auto expected =
      SdoInitExpectedData::U32(cs, IDX, SUBIDX, sizeof(sub_type));
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with a timeout set
///
/// \When co_csdo_blk_dn_req() is called with an index and a sub-index of
///       the entry to download, a pointer to the bytes to be downloaded, size
///       of the entry, a pointer to the confirmation function and a null
///       user-specified data pointer
///
/// \Then 0 is returned and SDO block download request is sent;
///       after the timeout value elapses and no response from the server
///       is received - the timeout message is sent
///       \Calls co_csdo_is_valid()
///       \Calls co_csdo_is_idle()
///       \Calls membuf_init()
///       \Calls can_timer_timeout()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnReq_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);
  StartCSDO();

  const uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CanSend::GetNumCalled());
  const co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                            CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;
  const auto expected =
      SdoInitExpectedData::U32(cs, IDX, SUBIDX, sizeof(sub_type));
  CanSend::Clear();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
}

///@}

/// @name co_csdo_blk_dn_val_req()
///@{

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_val_req() is called with an index and a sub-index,
///       a data type, a pointer to a buffer with a value to download, a pointer
///       to the download confirmation function and a null user-specified data
///       pointer
///
/// \Then 0 is returned and a correct SDO block download value request is sent
///       \Calls co_val_write()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_alloc()
///       \Calls co_val_write()
///       \Calls co_csdo_blk_dn_req()
TEST(CO_Csdo, CoCsdoBlkDnValReq_Nominal) {
  StartCSDO();

  const auto ret = co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                          CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                                   CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK,
                               IDX, SUBIDX, sizeof(SUB_TYPE));
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_val_req() is called with an index and a sub-index,
///       an array data type, a pointer to a buffer with an empty array,
///       a pointer to the download confirmation function and a null
///       user-specified data pointer
///
/// \Then 0 is returned and a correct SDO block download value request is sent
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_alloc()
///       \Calls co_csdo_blk_dn_req()
TEST(CO_Csdo, CoCsdoBlkDnValReq_DnEmptyArray) {
  StartCSDO();

  const auto ret =
      co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, val_os.GetDataType(),
                             val_os.GetValPtr(), CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                                   CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK,
                               IDX, SUBIDX, 0);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_val_req() is called with an index and a sub-index,
///       a data type, a pointer to a buffer with a value to download,
///       a pointer to the download confirmation function and a null
///       user-specified data pointer, but the internal call to co_val_write()
///       fails
///
/// \Then -1 is returned and no SDO message is sent, download confirmation
///       function is not called
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
TEST(CO_Csdo, CoCsdoBlkDnValReq_CoValWriteFail) {
  StartCSDO();

  LelyOverride::co_val_write(Override::NoneCallsValid);
  const auto ret = co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                          CoCsdoDnCon::Func, &data);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoCsdoDnCon::GetNumCalled());
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_val_req() is called with an index and a sub-index,
///       a data type, a pointer to a buffer with a value to download,
///       a pointer to the download confirmation function and a null
///       user-specified data pointer, but the second internal call to
///       co_val_write() fails
///
/// \Then -1 is returned and no SDO message is sent, download confirmation
///       function is not called
///       \Calls co_val_write()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_alloc()
TEST(CO_Csdo, CoCsdoBlkDnValReq_SecondCoValWriteFail) {
  StartCSDO();

  LelyOverride::co_val_write(1u);
  const auto ret = co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                          CoCsdoDnCon::Func, &data);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoCsdoDnCon::GetNumCalled());
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_val_req() is called with an index and a sub-index,
///       type, pointer to a buffer with a value to download, pointer to
///       the download confirmation function and a null user-specified data
///       pointer, but the internal call to membuf_reserve() fails
///
/// \Then -1 is returned and no SDO message is sent, download confirmation
///       function is not called
///       \Calls co_val_write()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
TEST(CO_Csdo, CoCsdoBlkDnValReq_MembufReserveFail) {
  StartCSDO();

  LelyOverride::membuf_reserve(Override::NoneCallsValid);
  const auto ret = co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                          CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoCsdoDnCon::GetNumCalled());
}
#endif  // HAVE_LELY_OVERRIDE

///@}

/// @name CSDO block download initiate
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO message with an incorrect command specifier
///       (not CO_SDO_SCS_BLK_DN_RES) is received
///
/// \Then an abort transfer SDO message is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_IncorrectCS) {
  StartCSDO();
  const uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CanSend::GetNumCalled());
  const co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                            CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;
  const auto expected =
      SdoInitExpectedData::U32(cs, IDX, SUBIDX, sizeof(sub_type));
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response with an incorrect
///       sub-command is received
///
/// \Then an abort transfer SDO message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_IncorrectSC) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  can_msg msg = SdoCreateMsg::BlkDnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES, 0,
                                          CO_SDO_SC_INI_BLK);
  msg.data[0] |= 0x01u;  // break the subcommand
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received, but
///       the sub-index is different from the requested
///
/// \Then an abort transfer SDO message with CO_SDO_AC_ERROR abort code is sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_IncorrectSubidx) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  const can_msg msg = SdoCreateMsg::BlkDnIniRes(
      IDX, SUBIDX + 1u, DEFAULT_COBID_RES, 0, CO_SDO_SC_INI_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_ERROR);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO abort transfer message is received, abort code is zero
///
/// \Then no SDO message is sent, download confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, CO_SDO_AC_ERROR
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_CSAbort_AcZero) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO abort transfer message is received, abort code is not zero
///
/// \Then no SDO message is sent, download confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, the received
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_CSAbort_AcNonzero) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  const can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_HARDWARE);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_HARDWARE, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO abort transfer message is received, but does not contain
///       the abort code
///
/// \Then no SDO message is sent, download confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, CO_SDO_AC_ERROR
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_CSAbort_MissingAc) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.len = CO_SDO_MSG_SIZE - 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received, but the message
///       does not contain an index to download
///
/// \Then an abort transfer SDO message with CO_SDO_AC_ERROR abort code is sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_MissingIdx) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  can_msg msg = SdoCreateMsg::BlkDnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES, 0,
                                          CO_SDO_SC_INI_BLK);
  msg.len = 3u;  // no index
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_ERROR);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received, but the index
///       is different from the requested
///
/// \Then an abort transfer SDO message with CO_SDO_AC_ERROR abort code is sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_IncorrectIdx) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  const can_msg msg = SdoCreateMsg::BlkDnIniRes(
      IDX + 1u, SUBIDX, DEFAULT_COBID_RES, 0, CO_SDO_SC_INI_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_ERROR);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received, but the message
///       does not contain a number of segments per block
///
/// \Then an abort transfer SDO message with CO_SDO_AC_BLK_SIZE abort code is
///       sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_MissingNumOfSegments) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  can_msg msg = SdoCreateMsg::BlkDnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES,
                                          CO_SDO_SC_INI_BLK, sizeof(sub_type));
  msg.len = 4u;  // no number of segments per block
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_BLK_SIZE);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO message with no command specifier is received
///
/// \Then an abort transfer SDO message is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_NoCS) {
  StartCSDO();
  InitiateBlockDownloadRequest();

  can_msg msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_NoAckseq) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  auto msg =
      SdoCreateMsg::BlkDnSubRes(0u, 1u, DEFAULT_COBID_RES, CO_SDO_SC_BLK_RES);
  msg.len = 1u;  // no ackseq
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_BLK_SEQ);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_NoBlksize) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  auto msg =
      SdoCreateMsg::BlkDnSubRes(0u, 1u, DEFAULT_COBID_RES, CO_SDO_SC_BLK_RES);
  msg.len = 2u;  // no blksize
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_BLK_SIZE);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_TooBigAckSeq) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  const auto msg =
      SdoCreateMsg::BlkDnSubRes(255u, 1u, DEFAULT_COBID_RES, CO_SDO_SC_BLK_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_BLK_SEQ);
}

/// \Given a pointer to the CSDO service (co_csdo_t) in the 'block download
///        sub-block' state
///
/// \When an SDO block download sub-block response is received
///
/// \Then an SDO message with CO_SDO_SEQ_LAST command specifier with correct
///       sequence number and segment data is sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_Nominal) {
  StartCSDO();
  InitiateBlockDownloadRequest(IDX, SUBIDX, val_u16.GetVal());

  const can_msg msg = SdoCreateMsg::BlkDnIniRes(
      IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_SC_INI_BLK, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const uint_least8_t sequence_number = 1u;
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_last = SdoInitExpectedData::Segment(
      CO_SDO_SEQ_LAST | sequence_number, val_u16.GetSegmentData());
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_last.data());
}

///@}

/// @name CSDO send 'block download sub-block' request
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client); the
///        transfer contains at least two segments
///
/// \When an SDO block download sub-block response is received
///
/// \Then two SDO download segment requests are sent; the last one has
///       CO_SDO_SEQ_LAST flag set in command specifier and the former does not
TEST(CO_Csdo, CoCsdoSendBlkDnSubReq_IsNotLast) {
  const co_unsigned8_t subidx_u64 = SUBIDX + 1u;
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_dn_val_req(csdo, IDX, subidx_u64, SUB_TYPE64,
                                        val_u64.GetValPtr(), CoCsdoDnCon::Func,
                                        nullptr));

  CanSend::Clear();

  const CanSend::MsgSeq expected_msg_seq = {
      SdoCreateMsg::DnSegReq(
          IDX, subidx_u64, DEFAULT_COBID_REQ, val_u64.GetFirstSegment().data(),
          static_cast<uint_least8_t>(val_u64.GetFirstSegment().size())),
      SdoCreateMsg::DnSegReq(
          IDX, subidx_u64, DEFAULT_COBID_REQ, val_u64.GetLastSegment().data(),
          static_cast<uint_least8_t>(val_u64.GetLastSegment().size()),
          CO_SDO_SEQ_LAST),
  };

  ReceiveBlockDownloadSubInitiateResponse(IDX, subidx_u64);
}

///@}

/// @name CSDO block download sub-block
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received, but
///       specified block size is incorrect
///
/// \Then an abort transfer SDO message with CO_SDO_AC_BLK_SIZE abort code
///       is sent
TEST(CO_Csdo, CoCsdoBlkDnSubOnEnter_IncorrectBlksize) {
  StartCSDO();

  InitiateBlockDownloadRequest(IDX, SUBIDX, val_u16.GetVal());

  const co_unsigned32_t blksize = 0;  // incorrect
  const can_msg msg = SdoCreateMsg::BlkDnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES,
                                                CO_SDO_SC_INI_BLK, blksize);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_BLK_SIZE);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received, but
///       specified block size is too large (greater than CO_SDO_MAX_SEQNO)
///
/// \Then an abort transfer SDO message with CO_SDO_AC_BLK_SIZE abort code
///       is sent
///       \Calls ldle_u16()
TEST(CO_Csdo, CoCsdoBlkDnSubOnEnter_TooLargeBlksize) {
  StartCSDO();
  InitiateBlockDownloadRequest(IDX, SUBIDX, val_u16.GetVal());

  const co_unsigned32_t blksize = CO_SDO_MAX_SEQNO + 1u;
  const can_msg msg = SdoCreateMsg::BlkDnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES,
                                                CO_SDO_SC_INI_BLK, blksize);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_BLK_SIZE);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client),
///        a custom download indication function is set
///
/// \When an SDO block download sub-block response is received
///
/// \Then an SDO segment with the expected data is sent, download indication
///       function is called once with the pointer to the CSDO service,
///       the index, the sub-index, a size of the value, 0 and a user-specified
///       data pointer
TEST(CO_Csdo, CoCsdoBlkDnSubOnEnter_WithDnInd) {
  int32_t data = 0;
  co_csdo_set_dn_ind(csdo, CoCsdoInd::Func, &data);
  StartCSDO();

  InitiateBlockDownloadRequest(IDX, SUBIDX, val_u16.GetVal());

  const can_msg msg = SdoCreateMsg::BlkDnIniRes(
      IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_SC_INI_BLK, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CoCsdoInd::GetNumCalled());
  CoCsdoInd::Check(csdo, IDX, SUBIDX, sizeof(sub_type), 0, &data);

  const uint_least8_t sequence_number = 1u;
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_last = SdoInitExpectedData::Segment(
      CO_SDO_SEQ_LAST | sequence_number, val_u16.GetSegmentData());
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_last.data());
}

//// \Given a pointer to the CSDO service (co_csdo_t) with a timeout set; the
///         service has initiated block download transfer (the correct request
///         was sent by the client); all segments were sent
///
/// \When the Client-SDO timeout expires before receiving the response from the
///       server
///
/// \Then an abort transfer SDO message with CO_SDO_AC_TIMEOUT abort code is
///       sent
TEST(CO_Csdo, CoCsdoBlkDnSubOnEnter_TimeoutSet) {
  co_csdo_set_dn_ind(csdo, CoCsdoInd::Func, nullptr);
  co_csdo_set_timeout(csdo, 999);
  StartCSDO();

  InitiateBlockDownloadRequest(IDX, SUBIDX, val_u16.GetVal());

  const can_msg msg = SdoCreateMsg::BlkDnIniRes(
      IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_SC_INI_BLK, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CoCsdoInd::GetNumCalled());

  const uint_least8_t sequence_number = 1u;
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_last = SdoInitExpectedData::Segment(
      CO_SDO_SEQ_LAST | sequence_number, val_u16.GetSegmentData());
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_last.data());
  CanSend::Clear();

  CHECK_EQUAL(1u, CoCsdoInd::GetNumCalled());
  CoCsdoInd::Check(csdo, IDX, SUBIDX, sizeof(sub_type), 0, nullptr);

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
}

/// \Given a pointer to the CSDO service (co_csdo_t) in the 'block download
///        sub-block' state
///
/// \When an SDO message with length zero is received
///
/// \Then an SDO abort message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_NoCs) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  auto msg = SdoCreateMsg::Default(0, 0, DEFAULT_COBID_RES);
  msg.len = 0;  // no command specifier
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_NO_CS, &data);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_CsAbort_AcZero) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  const auto msg = SdoCreateMsg::Abort(0, 0, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, &data);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_CsAbort_NonZeroAc) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  const auto msg =
      SdoCreateMsg::Abort(0, 0, DEFAULT_COBID_RES, CO_SDO_AC_HARDWARE);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_HARDWARE, &data);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_CsAbort_IncompleteAc) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  auto msg = SdoCreateMsg::Abort(0, 0, DEFAULT_COBID_RES);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_ERROR, &data);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_IncorrectCs) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  auto msg = SdoCreateMsg::Default(0, 0, DEFAULT_COBID_RES);
  msg.data[0] = 0xffu;  // break the command specifier
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_NO_CS, &data);
}

// TODO(N7S): GWT
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_IncorrectSc) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  auto msg =
      SdoCreateMsg::BlkDnSubRes(0u, 1u, DEFAULT_COBID_RES, CO_SDO_SC_BLK_RES);
  msg.data[0] |= 0x03u;  // break the subcommand
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, CO_SDO_AC_NO_CS, &data);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response with ackseq equal to blksize
///       is received
///
/// \Then an SDO message with CO_SDO_CCS_BLK_DN_REQ command specifier with
///       CO_SDO_SC_END_BLK subcommand and the size of the requested data
///       is sent
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_AckseqEqualToBlksize) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  const uint_least8_t blksize = 1u;
  const uint_least8_t ackseq = 1u;
  const auto msg = SdoCreateMsg::BlkDnSubRes(ackseq, blksize, DEFAULT_COBID_RES,
                                             CO_SDO_SC_BLK_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto expected_req = SdoCreateMsg::BlkDnEndReq(
      DEFAULT_COBID_REQ, 0,
      CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(sizeof(val_u16.GetVal())));
  CanSend::CheckMsg(expected_req);
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received
///
/// \Then an SDO message with CO_SDO_SEQ_LAST command specifier with correct
///       sequence number and segment data is sent
TEST(CO_Csdo, CoCsdoBlkDnSubOnRecv_Nominal) {
  StartCSDO();
  AdvanceToBlkDnSubState(IDX, SUBIDX);

  const auto msg =
      SdoCreateMsg::BlkDnSubRes(0u, 1u, DEFAULT_COBID_RES, CO_SDO_SC_BLK_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const uint_least8_t sequence_number = 1u;
  CheckLastSegmentSent(sequence_number, val_u16.GetSegmentData());
}

///@}

/// @name CSDO block download end
///@{

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When an SDO message with length zero is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_NoCs) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  auto msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, subidx_os, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When an SDO abort transfer message with a non-zero abort code
///
/// \Then no SDO message is sent, download confirmation function is called
///       with the pointer to the CSDO service, the requested multiplexer,
///       the received abort code and a user-specified data pointer
///       \Calls ldle_u32()
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_CsAbort_NonZeroAc) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  const auto msg =
      SdoCreateMsg::Abort(IDX, subidx_os, DEFAULT_COBID_RES, CO_SDO_AC_PARAM);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, subidx_os, CO_SDO_AC_PARAM, &data);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When an SDO abort transfer message with an abort code equal to zero
///
/// \Then no SDO message is sent, download confirmation function is called
///       with the pointer to the CSDO service, the requested multiplexer,
///       CO_SDO_AC_ERROR abort code and a user-specified data pointer
///       \Calls ldle_u32()
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_CsAbort_AcZero) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  const auto msg = SdoCreateMsg::Abort(IDX, subidx_os, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, subidx_os, CO_SDO_AC_ERROR, &data);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When an SDO abort transfer message with an incomplete abort code
///
/// \Then no SDO message is sent, download confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, CO_SDO_AC_ERROR
///       abort code, a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_CsAbort_IncompleteAc) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  auto msg = SdoCreateMsg::Abort(IDX, subidx_os, DEFAULT_COBID_RES);
  msg.len = CO_SDO_MSG_SIZE - 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, subidx_os, CO_SDO_AC_ERROR, &data);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When an SDO message with an incorrect command specifier is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_IncorrectCs) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  auto msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, subidx_os, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state (end of a non-empty octet string download); the
///        server supports CRC calculation and the request with a correct
///        CRC was sent after the block transfer
///
/// \When a correct block download end response is received
///
/// \Then no SDO message is sent, download confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, no abort code,
///       a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data, the CSDO service is idle
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_OsWithCrc) {
  StartCSDO();

  const uint_least8_t n = 1u;
  const uint_least8_t os[n] = {0xd3u};
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  CHECK_EQUAL(0, co_val_init_os(&val, os, n));

  InitiateOsBlockDownloadValRequest(IDX, SUBIDX, &val);
  ReceiveBlockDownloadSubInitiateResponse(IDX, SUBIDX, n,
                                          CO_SDO_SC_INI_BLK | CO_SDO_BLK_CRC);
  const uint_least8_t sequence_number = 1u;
  CheckBlockDownloadSubRequestSent(sequence_number, {0xd3u});
  ReceiveBlockDownloadResponse(sequence_number, n);
  CheckBlockDownloadEndRequestSent(n, 0xfb1eu);
  CanSend::Clear();

  const auto msg =
      SdoCreateMsg::BlkDnEndRes(DEFAULT_COBID_RES, CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, SUBIDX, 0, &data);
  CHECK(co_csdo_is_idle(csdo));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When an SDO message with an incorrect subcommand is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_IncorrectSc) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  auto msg = SdoCreateMsg::BlkDnEndRes(DEFAULT_COBID_RES);
  msg.data[0] |= 0x03u;  // break the subcommand
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(IDX, subidx_os, CO_SDO_AC_NO_CS);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When a correct block download end response is received
///
/// \Then no SDO message is sent, download confirmation function is called once
///       with the pointer to the CSDO service, the multiplexer, no abort code,
///       a null uploaded bytes pointer, zero and a pointer to the
///       user-specified data, the CSDO service is idle
TEST(CO_Csdo, CoCsdoBlkDnEndOnRecv_Nominal) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  const auto msg =
      SdoCreateMsg::BlkDnEndRes(DEFAULT_COBID_RES, CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(csdo, IDX, subidx_os, 0, &data);
  CHECK(co_csdo_is_idle(csdo));
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state
///
/// \When co_csdo_abort_req() is called with an abort code
///
/// \Then an abort transfer SDO message with the specified abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnEndOnAbort_Nominal) {
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  co_csdo_abort_req(csdo, CO_SDO_AC_HARDWARE);

  CheckSdoAbortSent(IDX, subidx_os, CO_SDO_AC_HARDWARE);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) in the 'block
///        download end' state; the service has a timeout set
///
/// \When the timeout expires before any SDO message is received
///
/// \Then an abort transfer SDO message with CO_SDO_AC_TIMEOUT abort code is
///       sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnEndOnTime_Nominal) {
  co_csdo_set_timeout(csdo, 999u);
  StartCSDO();
  const co_unsigned8_t subidx_os = SUBIDX + 1u;
  AdvanceToBlkDnEndState(IDX, subidx_os);

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(IDX, subidx_os, CO_SDO_AC_TIMEOUT);
}

///@}

/// @name CSDO send block upload sub-block response
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client) and
///        an SDO upload last segment request was received
///
/// \When an SDO block upload response with a CO_SDO_SC_END_BLK subcommand and
///       the correct size set is received
///
/// \Then an SDO block upload request with CO_SDO_SC_END_BLK command specifier
///       is sent
///       \Calls membuf_size()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoSendBlkUpEndRes_Nominal) {
  StartCSDO();
  InitiateBlockUploadRequest();

  const uint_least8_t sequence_number = 1u;
  const auto msg_up_seg =
      SdoCreateMsg::BlkUpSegReq(DEFAULT_COBID_RES, sequence_number,
                                val_u16.GetSegmentData(), CO_SDO_SEQ_LAST);
  CHECK_EQUAL(1, can_net_recv(net, &msg_up_seg, 0));
  CanSend::Clear();

  const auto msg_blk_up_res = SdoCreateMsg::BlkUpRes(
      DEFAULT_COBID_RES, sizeof(sub_type), CO_SDO_SC_END_BLK);
  CHECK_EQUAL(1, can_net_recv(net, &msg_blk_up_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_msg =
      SdoCreateMsg::BlkUpReq(DEFAULT_COBID_REQ, CO_SDO_SC_END_BLK);
  CanSend::CheckMsg(expected_msg);
}

///@}

TEST_GROUP_BASE(CO_CsdoUpload, CO_CsdoBase) {
  using small_type = co_unsigned16_t;
  using large_type = co_unsigned64_t;

  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;
#if LELY_NO_MALLOC
  static const std::size_t POOL_SIZE = sizeof(large_type);
  co_unsigned8_t pool[POOL_SIZE] = {0};
#endif
  membuf buffer = MEMBUF_INIT;

  void CheckSdoAbortSent(const co_unsigned32_t ac) const {
    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    const auto expected_abort =
        SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, ac);
    CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                      expected_abort.data());
  }

  void CheckSentSegReq(const bool toggle) const {
    co_unsigned8_t cs = CO_SDO_CCS_UP_SEG_REQ;
    if (toggle) cs |= CO_SDO_SEG_TOGGLE;

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    const auto expected = SdoInitExpectedData::Empty(cs, 0u, 0u);
    CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  }

  void CheckTransferAbortedLocally(const co_unsigned32_t ac) const {
    CHECK_EQUAL(0u, CanSend::GetNumCalled());
    CoCsdoUpCon::Check(csdo, IDX, SUBIDX, ac, nullptr, 0u, nullptr);
    CHECK(co_csdo_is_idle(csdo));
  }

  void CheckDoneTransfer(const std::size_t expected_size,
                         const std::vector<co_unsigned8_t>& expected_value)
      const {
    CHECK_EQUAL(0u, CanSend::GetNumCalled());
    CoCsdoUpCon::Check(csdo, IDX, SUBIDX, 0u, membuf_begin(&buffer),
                       expected_size, nullptr);
    CHECK_EQUAL(expected_size, membuf_size(&buffer));
    MEMCMP_EQUAL(expected_value.data(), membuf_begin(&buffer), expected_size);
    CHECK(co_csdo_is_idle(csdo));
  }

  void ReceiveExpeditedUpIni(const co_unsigned8_t exp_flags,
                             const std::vector<co_unsigned8_t>& value) const {
    const can_msg msg = SdoCreateMsg::UpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES,
                                               exp_flags, value);
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  void ReceiveSegmentedUpIni(const std::size_t size) const {
    const can_msg msg =
        SdoCreateMsg::UpIniResWithSize(IDX, SUBIDX, DEFAULT_COBID_RES, size);
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  void AdvanceToUpSegState(const std::size_t size) const {
    ReceiveSegmentedUpIni(size);
    CoCsdoUpCon::Clear();
    CoCsdoInd::Clear();
    CanSend::Clear();
  }

  TEST_SETUP() override {
    TEST_BASE_SETUP();

#if LELY_NO_MALLOC
    membuf_init(&buffer, pool, POOL_SIZE);
    memset(pool, 0, POOL_SIZE);
#else
    membuf_init(&buffer, nullptr, 0u);
#endif

    co_csdo_start(csdo);
    CHECK_EQUAL(0, co_csdo_up_req(csdo, IDX, SUBIDX, &buffer, CoCsdoUpCon::func,
                                  nullptr));

    CoCsdoUpCon::Clear();
    CoCsdoInd::Clear();
    CanSend::Clear();
  }

  TEST_TEARDOWN() override {
    membuf_fini(&buffer);

    TEST_BASE_TEARDOWN();
  }
};

/// @name CSDO initiate segmented upload
///@{

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_NoCs) {
  can_msg msg = SdoCreateMsg::UpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_CsAbort_IncompleteAc) {
  can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_TIMEOUT);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_CsAbort) {
  const co_unsigned32_t ac = CO_SDO_AC_TIMEOUT;

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, ac);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(ac);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_InvalidCs) {
  can_msg msg = SdoCreateMsg::UpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.data[0] = CO_SDO_SCS_DN_INI_RES;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_TooShortMultiplexer) {
  can_msg msg = SdoCreateMsg::UpIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.len = 3u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_IncorrectIdx) {
  const can_msg msg =
      SdoCreateMsg::UpIniRes(IDX + 1u, SUBIDX, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_IncorrectSubidx) {
  const can_msg msg =
      SdoCreateMsg::UpIniRes(IDX, SUBIDX + 1u, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_ERROR);
}

#if LELY_NO_MALLOC
/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_Expedited_BufferTooSmall) {
  const co_unsigned8_t zeroes[POOL_SIZE] = {0};
  membuf_write(&buffer, zeroes, POOL_SIZE - 1u);

  ReceiveExpeditedUpIni(CO_SDO_INI_SIZE_EXP, {0x12u, 0x34u});

  CheckSdoAbortSent(CO_SDO_AC_NO_MEM);
}
#endif  // LELY_NO_MALLOC

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_Expedited_NoSizeInd) {
  const std::vector<co_unsigned8_t> expected_value = {0x12u, 0x34u, 0x00u,
                                                      0x00u};

  ReceiveExpeditedUpIni(CO_SDO_INI_SIZE_EXP, expected_value);

  CheckDoneTransfer(4u, expected_value);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_Expedited_Nominal) {
  const std::vector<co_unsigned8_t> expected_value = {0x12u, 0x34u};

  ReceiveExpeditedUpIni(CO_SDO_INI_SIZE_EXP | CO_SDO_INI_SIZE_IND |
                            CO_SDO_INI_SIZE_EXP_SET(sizeof(small_type)),
                        expected_value);

  CheckDoneTransfer(sizeof(small_type), expected_value);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_Segmented_NoInd) {
  co_csdo_set_up_ind(csdo, nullptr, nullptr);

  ReceiveSegmentedUpIni(sizeof(small_type));

  CHECK_EQUAL(0u, CoCsdoInd::GetNumCalled());
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, IniOnRecv_Segmented_Nominal) {
  co_csdo_set_up_ind(csdo, CoCsdoInd::Func, nullptr);

  ReceiveSegmentedUpIni(sizeof(small_type));

  CoCsdoInd::Check(csdo, IDX, SUBIDX, sizeof(small_type), 0u, nullptr);
  CheckSentSegReq(false);
}

///@}

/// @name CSDO upload segment
///@{

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnAbort_Nominal) {
  AdvanceToUpSegState(sizeof(small_type));

  const co_unsigned32_t ac = CO_SDO_AC_HARDWARE;

  co_csdo_abort_req(csdo, ac);

  CheckSdoAbortSent(ac);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnTime_Nominal) {
  co_csdo_set_timeout(csdo, 999);
  AdvanceToUpSegState(sizeof(small_type));

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(CO_SDO_AC_TIMEOUT);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_NoCs) {
  AdvanceToUpSegState(sizeof(small_type));

  can_msg msg = SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, {});
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_CsAbort_IncompleteAc) {
  AdvanceToUpSegState(sizeof(small_type));

  can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_TIMEOUT);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_CsAbort) {
  AdvanceToUpSegState(sizeof(small_type));

  const co_unsigned32_t ac = CO_SDO_AC_TIMEOUT;

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, ac);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(ac);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_InvalidCs) {
  AdvanceToUpSegState(sizeof(small_type));

  can_msg msg = SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, {});
  msg.data[0] = CO_SDO_SCS_DN_SEG_RES;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_UnexpectedToggleBit) {
  AdvanceToUpSegState(sizeof(small_type));

  const can_msg msg =
      SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, {}, CO_SDO_SEG_TOGGLE);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
  CHECK_FALSE(CoCsdoUpCon::Called());
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_InvalidSegmentSize) {
  AdvanceToUpSegState(sizeof(small_type));

  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(sizeof(small_type));
  can_msg msg = SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, {}, seg_size);
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_TooLargeSegment) {
  AdvanceToUpSegState(sizeof(small_type));

  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(sizeof(small_type) + 1u);
  const can_msg msg = SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, {}, seg_size);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_TYPE_LEN_HI);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_LastSegment_TooSmall) {
  AdvanceToUpSegState(sizeof(small_type));

  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(sizeof(small_type) - 1u);
  const can_msg msg =
      SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, {}, seg_size | CO_SDO_SEG_LAST);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_TYPE_LEN_LO);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_LastSegment_NoInd) {
  co_csdo_set_up_ind(csdo, nullptr, nullptr);
  AdvanceToUpSegState(sizeof(small_type));

  const std::vector<co_unsigned8_t> expected_value = {0x12u, 0x34u};
  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(sizeof(small_type));
  const can_msg msg = SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, expected_value,
                                             seg_size | CO_SDO_SEG_LAST);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CoCsdoInd::GetNumCalled());
  CheckDoneTransfer(sizeof(small_type), expected_value);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_LastSegment) {
  co_csdo_set_up_ind(csdo, CoCsdoInd::Func, nullptr);
  AdvanceToUpSegState(sizeof(small_type));

  const std::size_t size = sizeof(small_type);
  const std::vector<co_unsigned8_t> expected_value = {0x12u, 0x34u};
  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(size);
  const can_msg msg = SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, expected_value,
                                             seg_size | CO_SDO_SEG_LAST);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CoCsdoInd::Check(csdo, IDX, SUBIDX, size, size, nullptr);
  CheckDoneTransfer(size, expected_value);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_NotLast) {
  AdvanceToUpSegState(sizeof(large_type));

  const std::vector<co_unsigned8_t> expected_value(CO_SDO_SEG_MAX_DATA_SIZE,
                                                   0xffu);
  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(CO_SDO_SEG_MAX_DATA_SIZE);
  const can_msg msg =
      SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, expected_value, seg_size);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSentSegReq(true);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);
  AdvanceToUpSegState(sizeof(large_type));

  const std::vector<co_unsigned8_t> expected_value(CO_SDO_SEG_MAX_DATA_SIZE,
                                                   0xffu);
  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(CO_SDO_SEG_MAX_DATA_SIZE);
  const can_msg msg =
      SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, expected_value, seg_size);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSentSegReq(true);
  CanSend::Clear();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(CO_SDO_AC_TIMEOUT);
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_LargeDataSet_PeriodicInd) {
  co_csdo_set_up_ind(csdo, CoCsdoInd::Func, nullptr);

  const std::size_t segments = 2u * CO_SDO_MAX_SEQNO + 1u;
  const std::size_t size = segments * CO_SDO_SEG_MAX_DATA_SIZE;

#if LELY_NO_MALLOC
  std::vector<co_unsigned8_t> buf(size);
  membuf_fini(&buffer);
  membuf_init(&buffer, buf.data(), size);
#endif

  AdvanceToUpSegState(size);

  const std::vector<co_unsigned8_t> seg_data(CO_SDO_SEG_MAX_DATA_SIZE, 0xffu);
  const co_unsigned8_t seg_size = CO_SDO_SEG_SIZE_SET(CO_SDO_SEG_MAX_DATA_SIZE);

  for (std::size_t i = 0; i < segments; ++i) {
    co_unsigned8_t flags = seg_size;
    if (i % 2 == 1) flags |= CO_SDO_SEG_TOGGLE;
    if (i == segments - 1) flags |= CO_SDO_SEG_LAST;

    const can_msg msg =
        SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, seg_data, flags);
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  CHECK(co_csdo_is_idle(csdo));

  // 2 middle and 1 final upload progress indication function calls
  CHECK_EQUAL(3u, CoCsdoInd::GetNumCalled());
  CoCsdoInd::Check(csdo, IDX, SUBIDX, size, size, nullptr);

  CHECK_EQUAL(size, membuf_size(&buffer));
  for (std::size_t i = 0; i < size; ++i) {
    CHECK_EQUAL(0xffu, static_cast<co_unsigned8_t>(buffer.begin[i]));
  }
}

/// TODO(N7S): GWT
TEST(CO_CsdoUpload, SegOnRecv_SizeZero) {
  co_csdo_set_up_ind(csdo, CoCsdoInd::Func, nullptr);
  AdvanceToUpSegState(0u);

  const co_unsigned8_t size_zero = CO_SDO_SEG_SIZE_SET(0u);
  const can_msg msg = SdoCreateMsg::UpSegRes(DEFAULT_COBID_RES, {},
                                             size_zero | CO_SDO_SEG_LAST);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CoCsdoInd::GetNumCalled());
  CheckDoneTransfer(0u, {});
}

///@}

TEST_GROUP_BASE(CO_CsdoDownload, CO_CsdoBase) {
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;

  const std::vector<co_unsigned8_t> buffer = {0x01u, 0x23u, 0x45u, 0x67u,
                                              0x89u, 0xabu, 0xcdu, 0xefu};
  std::vector<co_unsigned8_t> first_segment;
  std::vector<co_unsigned8_t> last_segment;

  void SendDownloadRequest(const std::vector<co_unsigned8_t>& buf) {
    CHECK_EQUAL(0, co_csdo_dn_req(csdo, IDX, SUBIDX, buf.data(), buf.size(),
                                  &CoCsdoDnCon::Func, &data));

    CoCsdoDnCon::Clear();
    CoCsdoInd::Clear();
    CanSend::Clear();
  }

  void CheckSdoAbortSent(const co_unsigned32_t ac) const {
    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    const auto expected_abort =
        SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, ac);
    CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                      expected_abort.data());
    CHECK(co_csdo_is_idle(csdo));
  }

  void CheckTransferAbortedLocally(const co_unsigned32_t ac) const {
    CHECK_EQUAL(0u, CanSend::GetNumCalled());
    CoCsdoDnCon::Check(csdo, IDX, SUBIDX, ac, &data);
    CHECK(co_csdo_is_idle(csdo));
  }

  void ReceiveSegmentedDnIni() const {
    const can_msg msg = SdoCreateMsg::DnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  void CheckSentSegReq(const std::vector<co_unsigned8_t>& seg_data,
                       const co_unsigned8_t cs_flags = 0) const {
    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    co_unsigned8_t cs = CO_SDO_CCS_DN_SEG_REQ;
    cs |= CO_SDO_SEG_SIZE_SET(static_cast<co_unsigned8_t>(seg_data.size()));
    cs |= cs_flags;
    const auto expected = SdoInitExpectedData::Segment(cs, seg_data);
    CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  }

  void AdvanceToDnSegState() {
    SendDownloadRequest(buffer);
    ReceiveSegmentedDnIni();
    CoCsdoDnCon::Clear();
    CoCsdoInd::Clear();
    CanSend::Clear();
  }

  void CheckDoneTransfer() const {
    CHECK_EQUAL(0u, CanSend::GetNumCalled());
    CoCsdoDnCon::Check(csdo, IDX, SUBIDX, 0u, &data);
    CHECK(co_csdo_is_idle(csdo));
  }

  TEST_SETUP() override {
    TEST_BASE_SETUP();

    first_segment.assign(buffer.data(), buffer.data() + 7u);
    last_segment.assign(buffer.data() + 7u, buffer.data() + 8u);

    co_csdo_start(csdo);
  }
};

/// @name CSDO initiate segmented download
///@{

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_NoCs) {
  SendDownloadRequest(buffer);

  can_msg msg = SdoCreateMsg::DnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_CsAbort_IncompleteAc) {
  SendDownloadRequest(buffer);

  can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_HARDWARE);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_CsAbort) {
  SendDownloadRequest(buffer);

  const co_unsigned32_t ac = CO_SDO_AC_HARDWARE;

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, ac);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(ac);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_InvalidCs) {
  SendDownloadRequest(buffer);

  can_msg msg = SdoCreateMsg::DnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_IncompleteMultiplexer) {
  SendDownloadRequest(buffer);

  can_msg msg = SdoCreateMsg::DnIniRes(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg.len = 3u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_IncorrectIdx) {
  SendDownloadRequest(buffer);

  const can_msg msg =
      SdoCreateMsg::DnIniRes(IDX + 1u, SUBIDX, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_IncorrectSubidx) {
  SendDownloadRequest(buffer);

  const can_msg msg =
      SdoCreateMsg::DnIniRes(IDX, SUBIDX + 1u, DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_Nominal) {
  SendDownloadRequest(buffer);

  ReceiveSegmentedDnIni();

  CheckSentSegReq(first_segment);
  CHECK_FALSE(co_csdo_is_idle(csdo));
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, IniOnRecv_SizeZero) {
  co_csdo_set_dn_ind(csdo, &CoCsdoInd::Func, nullptr);
  SendDownloadRequest({});

  ReceiveSegmentedDnIni();

  CheckSentSegReq({}, CO_SDO_SEG_LAST);
  CHECK_EQUAL(0u, CoCsdoInd::GetNumCalled());
  CHECK_FALSE(co_csdo_is_idle(csdo));
}

///@}

/// @name CSDO download segment request and response handling
///@{

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnAbort_Nominal) {
  AdvanceToDnSegState();

  const co_unsigned32_t ac = CO_SDO_AC_HARDWARE;

  co_csdo_abort_req(csdo, ac);

  CheckSdoAbortSent(ac);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnTime_Nominal) {
  co_csdo_set_timeout(csdo, 999);
  AdvanceToDnSegState();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CheckSdoAbortSent(CO_SDO_AC_TIMEOUT);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_NoCs) {
  AdvanceToDnSegState();

  can_msg msg = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_CsAbort_IncompleteAc) {
  AdvanceToDnSegState();

  can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_HARDWARE);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(CO_SDO_AC_ERROR);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_CsAbort) {
  AdvanceToDnSegState();

  const co_unsigned32_t ac = CO_SDO_AC_HARDWARE;

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, ac);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckTransferAbortedLocally(ac);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_InvalidCs) {
  AdvanceToDnSegState();

  can_msg msg = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_NO_CS);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_UnexpectedToggleBit) {
  AdvanceToDnSegState();

  const can_msg msg =
      SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES, CO_SDO_SEG_TOGGLE);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSdoAbortSent(CO_SDO_AC_TOGGLE);
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_LastSegment_NoInd) {
  co_csdo_set_dn_ind(csdo, nullptr, nullptr);
  AdvanceToDnSegState();

  const can_msg msg = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSentSegReq(last_segment, CO_SDO_SEG_TOGGLE | CO_SDO_SEG_LAST);
  CHECK_EQUAL(0u, CoCsdoInd::GetNumCalled());
  CHECK_FALSE(co_csdo_is_idle(csdo));
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_LastSegment) {
  co_csdo_set_dn_ind(csdo, &CoCsdoInd::Func, nullptr);
  AdvanceToDnSegState();

  const can_msg msg = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckSentSegReq(last_segment, CO_SDO_SEG_TOGGLE | CO_SDO_SEG_LAST);
  CoCsdoInd::Check(csdo, IDX, SUBIDX, buffer.size(), buffer.size(), nullptr);
  CHECK_FALSE(co_csdo_is_idle(csdo));
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_LastSegmentConfirmed) {
  AdvanceToDnSegState();

  const can_msg first_res = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &first_res, 0));
  CanSend::Clear();

  const can_msg second_res =
      SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES, CO_SDO_SEG_TOGGLE);
  CHECK_EQUAL(1, can_net_recv(net, &second_res, 0));

  CheckDoneTransfer();
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_SizeZeroConfirmed) {
  SendDownloadRequest({});
  ReceiveSegmentedDnIni();
  CanSend::Clear();

  const can_msg msg = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckDoneTransfer();
}

/// TODO(N7S): GWT
TEST(CO_CsdoDownload, SegOnRecv_LargeDataSet_PeriodicInd) {
  co_csdo_set_dn_ind(csdo, &CoCsdoInd::Func, nullptr);

  const std::size_t segments = 2u * CO_SDO_MAX_SEQNO + 1u;
  const std::size_t size = segments * CO_SDO_SEG_MAX_DATA_SIZE;
  const std::vector<co_unsigned8_t> large_buffer(size, 0xffu);
  const std::vector<co_unsigned8_t> expected_segment(CO_SDO_SEG_MAX_DATA_SIZE,
                                                     0xffu);

  SendDownloadRequest(large_buffer);
  ReceiveSegmentedDnIni();

  for (std::size_t i = 0; i < segments; ++i) {
    const co_unsigned8_t toggle = (i % 2 == 1) ? CO_SDO_SEG_TOGGLE : 0u;
    const co_unsigned8_t last = (i == segments - 1) ? CO_SDO_SEG_LAST : 0u;

    CheckSentSegReq(expected_segment, toggle | last);
    CanSend::Clear();

    const can_msg msg = SdoCreateMsg::DnSegRes(DEFAULT_COBID_RES, toggle);
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  CHECK_EQUAL(3u, CoCsdoInd::GetNumCalled());
  CoCsdoInd::Check(csdo, IDX, SUBIDX, size, size, nullptr);
  CheckDoneTransfer();
}

///@}

TEST_GROUP_BASE(CO_CsdoIde, CO_CsdoBase) {
  static const co_unsigned32_t REQ_EID_CANID = (0x600u + DEV_ID) | (1 << 28u);
  static const co_unsigned32_t RES_EID_CANID = (0x580u + DEV_ID) | (1 << 28u);
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;

  const std::vector<co_unsigned8_t> buffer = {0x01u, 0x23u, 0x45u,
                                              0x67u, 0x89u, 0xabu};

  void SendDnReq() {
    CHECK_EQUAL(0, co_csdo_dn_req(csdo, IDX, SUBIDX, buffer.data(),
                                  buffer.size(), &CoCsdoDnCon::Func, &data));
  }

  TEST_SETUP() override {
    TEST_BASE_SETUP();

    SetCli01CobidReq(REQ_EID_CANID | CO_SDO_COBID_FRAME);
    SetCli02CobidRes(RES_EID_CANID | CO_SDO_COBID_FRAME);

    co_csdo_start(csdo);

    CoCsdoDnCon::Clear();
    CanSend::Clear();
  }
};

/// @name SDO transfer with Extended CAN Identifier
///@{

/// TODO(N7S): GWT
TEST(CO_CsdoIde, InitIniReq_ExtendedId) {
  SendDnReq();

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto exp_ini_req = SdoInitExpectedData::U32(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND, IDX, SUBIDX,
      static_cast<co_unsigned32_t>(buffer.size()));
  CanSend::CheckMsg(REQ_EID_CANID, CAN_FLAG_IDE, CO_SDO_MSG_SIZE,
                    exp_ini_req.data());

  can_msg abort = SdoCreateMsg::Abort(0u, 0u, RES_EID_CANID);
  abort.flags = CAN_FLAG_IDE;
  can_net_recv(net, &abort, 0);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
}

/// TODO(N7S): GWT
TEST(CO_CsdoIde, InitSegReq_ExtendedId) {
  SendDnReq();
  CanSend::Clear();

  can_msg ini_res = SdoCreateMsg::DnIniRes(IDX, SUBIDX, RES_EID_CANID);
  ini_res.flags = CAN_FLAG_IDE;
  CHECK_EQUAL(1, can_net_recv(net, &ini_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  co_unsigned8_t cs = CO_SDO_CCS_DN_SEG_REQ;
  cs |= CO_SDO_SEG_SIZE_SET(static_cast<co_unsigned8_t>(buffer.size()));
  cs |= CO_SDO_SEG_LAST;
  const auto expected = SdoInitExpectedData::Segment(cs, buffer);
  CanSend::CheckMsg(REQ_EID_CANID, CAN_FLAG_IDE, CO_SDO_MSG_SIZE,
                    expected.data());
  CanSend::Clear();

  can_msg seg_res = SdoCreateMsg::DnSegRes(RES_EID_CANID);
  seg_res.flags = CAN_FLAG_IDE;
  CHECK_EQUAL(1, can_net_recv(net, &seg_res, 0));

  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
}

///@}
