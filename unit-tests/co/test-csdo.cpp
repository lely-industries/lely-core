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

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/array-init.hpp"

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
  const co_sdo_par* par = co_csdo_get_par(csdo);
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
///       \Calle can_recv_destroy()
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
/// \Then 0 is returned, the service is not stopped, the service is idle
///       \Calls co_csdo_is_stopped()
///       \Calls co_csdo_abort_req()
///       \Calls co_csdo_is_valid()
///       \Calls can_recv_start()
TEST(CO_CsdoInit, CoCsdoStart_NoDev) {
  co_csdo_t* const csdo = co_csdo_create(net, nullptr, CSDO_NUM);
  CHECK(csdo != nullptr);

  const auto ret = co_csdo_start(csdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_csdo_is_stopped(csdo));
  CHECK(co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_start() is called
///
/// \Then 0 is returned, the service is not stopped, the service is idle
///       \Calls co_csdo_is_stopped()
TEST(CO_CsdoInit, CoCsdoStart_AlreadyStarted) {
  dev_holder->CreateAndInsertObj(obj1280, 0x1280u);
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK_EQUAL(0, co_csdo_start(csdo));

  const auto ret = co_csdo_start(csdo);

  CHECK_EQUAL(0, ret);
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
/// \Then 0 is returned, the service is not stopped, the service is idle
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

  const auto ret = co_csdo_start(csdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_csdo_is_stopped(csdo));
  CHECK(co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the CSDO service (co_csdo_t) containing object 0x1280 in
///        the object dictionary
///
/// \When co_csdo_start() is called
///
/// \Then 0 is returned, the service is not stopped, the service is idle
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

  const auto ret = co_csdo_start(csdo);

  CHECK_EQUAL(0, ret);
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
  CHECK_EQUAL(0, co_csdo_start(csdo));

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

  static const co_unsigned8_t CSDO_NUM = 0x01u;
  static const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned32_t DEFAULT_COBID_REQ = 0x600u + DEV_ID;
  const co_unsigned32_t DEFAULT_COBID_RES = 0x580u + DEV_ID;
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
  const int32_t data = 0;  // dummy data to workaround clang-format

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

TEST_GROUP_BASE(CO_Csdo, CO_CsdoBase) {
  CoArrays arrays;

  using sub_type64 = co_unsigned64_t;

  static membuf ind_mbuf;
  static size_t num_called;
  static const co_unsigned16_t SUB_TYPE = CO_DEFTYPE_UNSIGNED16;
  static const co_unsigned16_t SUB_TYPE64 = CO_DEFTYPE_UNSIGNED64;
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;
  static const co_unsigned16_t INVALID_IDX = 0xffffu;
  static const co_unsigned8_t INVALID_SUBIDX = 0xffu;
  const sub_type VAL = 0xabcdu;
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

  void StartCSDO() { CHECK_EQUAL(0, co_csdo_start(csdo)); }

  static co_unsigned32_t co_sub_failing_dn_ind(co_sub_t*, co_sdo_req*,
                                               co_unsigned32_t, void*) {
    return CO_SDO_AC_ERROR;
  }

  void InitiateBlockDownloadRequest(const co_unsigned16_t idx = IDX,
                                    const co_unsigned8_t subidx = SUBIDX,
                                    const sub_type val = 0) {
    CHECK_EQUAL(0, co_csdo_blk_dn_val_req(csdo, idx, subidx, SUB_TYPE, &val,
                                          CoCsdoDnCon::Func, nullptr));
    CanSend::Clear();
  }

  void InitiateBlockUploadRequest(
      const co_unsigned16_t idx = IDX, const co_unsigned8_t subidx = SUBIDX,
      const co_unsigned32_t size = sizeof(sub_type)) {
    CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, idx, subidx, 0, nullptr,
                                      CoCsdoUpCon::func, nullptr));

    const can_msg msg_res =
        SdoCreateMsg::BlkUpIniRes(idx, subidx, DEFAULT_COBID_RES, size);
    CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));
    CanSend::Clear();
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateAndInsertObj(obj2020, IDX);
    obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});

    CoCsdoUpCon::Clear();
    CanSend::Clear();
  }

  TEST_TEARDOWN() {
    ind_mbuf = MEMBUF_INIT;
    num_called = 0;

    arrays.Clear();

    TEST_BASE_TEARDOWN();
  }
};
membuf TEST_GROUP_CppUTestGroupCO_Csdo::ind_mbuf = MEMBUF_INIT;
size_t TEST_GROUP_CppUTestGroupCO_Csdo::num_called = 0;

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
///       pointer, an index, a sub-index, CO_SDO_AC_ERROR and
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
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_ERROR, nullptr);
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
  co_sub_up_ind_t* const req_up_ind = [](const co_sub_t* sub, co_sdo_req* req,
                                         co_unsigned32_t ac,
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
         void*) -> co_unsigned32_t {
    req->membuf = &ind_mbuf;

    co_sub_on_up(sub, req, &ac);

    return 0;
  };

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, nullptr);

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
         void*) -> co_unsigned32_t {
    req->membuf = &ind_mbuf;

    co_sub_on_up(sub, req, &ac);
    if (num_called == 0) {
      req->nbyte = 0;
    }
    num_called++;

    return 0;
  };

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, nullptr);

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
         void*) -> co_unsigned32_t {
    req->membuf = &ind_mbuf;

    co_sub_on_up(sub, req, &ac);

    return 0;
  };

  co_dev_set_val_u16(dev, IDX, SUBIDX, 0x1234u);
  co_obj_set_up_ind(obj2020->Get(), req_up_ind, nullptr);

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
  timespec ts = {1u, 0u};
  can_net_set_time(net, &ts);
}

static void
AbortTransfer(can_net_t* const net, const co_unsigned32_t can_id) {
  can_msg msg = SdoCreateMsg::Abort(0, 0, can_id, CO_SDO_AC_ERROR);
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
  const co_octet_string_t val2dn = arrays.Init<co_octet_string_t>();
  const auto ret =
      co_csdo_dn_val_req(csdo, IDX, SUBIDX, CO_DEFTYPE_OCTET_STRING, &val2dn,
                         nullptr, CoCsdoDnCon::Func, nullptr);

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
TEST(CO_Csdo, CoCsdoDnDcfReq_Nominal) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_sub_set_val_u16(obj2020->GetLastSub(), VAL);
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));

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
TEST(CO_Csdo, CoCsdoDnDcfReq_InvalidCobidReq) {
  SetCli01CobidReq(DEFAULT_COBID_REQ | CO_SDO_COBID_VALID);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  co_sub_set_val_u16(obj2020->GetLastSub(), VAL);
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));

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
TEST(CO_Csdo, CoCsdoDnDcfReq_IsNotIdle) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, 0xffffu, 0xffu, 0, nullptr, nullptr,
                                    nullptr));
  CanSend::Clear();

  co_sub_set_val_u16(obj2020->GetLastSub(), VAL);
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));

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
///       \Calls co_csdo_dn_req()
TEST(CO_Csdo, CoCsdoDnDcfReq_TooShortBuffer) {
  SetCli01CobidReq(DEFAULT_COBID_REQ);
  SetCli02CobidRes(DEFAULT_COBID_RES);
  StartCSDO();

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_sub_set_val_u16(obj2020->GetLastSub(), VAL);
  auto dcf = ConciseDcf::MakeForEntries<sub_type>();
  CHECK_EQUAL(dcf.Size(),
              co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(), dcf.End()));

  const auto ret = co_csdo_dn_dcf_req(
      csdo, dcf.Begin(), dcf.Begin() + sizeof(co_unsigned32_t) - 1u,
      CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(csdo, 0x0000u, 0x00u, CO_SDO_AC_TYPE_LEN_LO, nullptr);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0u, CanSend::GetNumCalled());
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
///       message was sent
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
///       was sent to the server
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
///       was sent to the server; when the timeout expired, an SDO abort
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
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
///       message was sent
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
///       request was sent to the server
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
///       request was sent to the server; when the timeout expired, an SDO abort
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
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
/// \Then no SDO message is sent
///       \Calls ldle_u32()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_AcNonZero) {
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
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_NO_READ);
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an abort transfer SDO message with abort code equal to zero is
///       received
///
/// \Then no SDO message is sent
///       \Calls ldle_u32()
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_AcZero) {
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
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, 0);
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When an abort transfer SDO message with no abort code is received
///
/// \Then no SDO message is sent
TEST(CO_Csdo, CoCsdoBlkUpIniOnRecv_AcNone) {
  StartCSDO();

  CHECK_EQUAL(0, co_csdo_blk_up_req(csdo, IDX, SUBIDX, 0, nullptr,
                                    CoCsdoUpCon::func, nullptr));

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
                                    CoCsdoUpCon::func, nullptr));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_RES);
  msg_res.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
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
                                    CoCsdoUpCon::func, nullptr));

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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
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
                                    CoCsdoUpCon::func, nullptr));

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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_ERROR);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_ERROR);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_ERROR);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_res =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_MEM);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected_res.data());
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
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

  const co_octet_string_t val2dn = arrays.Init<co_octet_string_t>();
  const auto ret =
      co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, CO_DEFTYPE_OCTET_STRING,
                             &val2dn, CoCsdoDnCon::Func, nullptr);

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
/// \Then -1 is returned and no SDO message is sent
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
TEST(CO_Csdo, CoCsdoBlkDnValReq_CoValWriteFail) {
  StartCSDO();

  LelyOverride::co_val_write(Override::NoneCallsValid);
  const auto ret = co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                          CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_val_req() is called with an index and a sub-index,
///       a data type, a pointer to a buffer with a value to download,
///       a pointer to the download confirmation function and a null
///       user-specified data pointer, but the second internal call to
///       co_val_write() fails
///
/// \Then -1 is returned and no SDO message is sent
///       \Calls co_val_write()
///       \Calls membuf_clear()
///       \Calls membuf_reserve()
///       \Calls membuf_alloc()
TEST(CO_Csdo, CoCsdoBlkDnValReq_SecondCoValWriteFail) {
  StartCSDO();

  LelyOverride::co_val_write(1u);
  const auto ret = co_csdo_blk_dn_val_req(csdo, IDX, SUBIDX, SUB_TYPE, &VAL,
                                          CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_val_req() is called with an index and a sub-index,
///       type, pointer to a buffer with a value to download, pointer to
///       the download confirmation function and a null user-specified data
///       pointer, but the internal call to membuf_reserve() fails
///
/// \Then -1 is returned and no SDO message is sent
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
}
#endif  // HAVE_LELY_OVERRIDE

///@}

/// @name CSDO send 'download initiate' request
///@{

/// TODO(N7s): test cases for co_csdo_send_dn_ini_req()

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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
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

  can_msg msg = SdoCreateMsg::BlkDnSubRes(IDX, SUBIDX, DEFAULT_COBID_RES, 0,
                                          CO_SDO_SC_INI_BLK, sizeof(sub_type));
  msg.data[0] |= 0x01u;  // break the subcommand
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
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

  const can_msg msg =
      SdoCreateMsg::BlkDnSubRes(IDX, SUBIDX + 1u, DEFAULT_COBID_RES, 0,
                                CO_SDO_SC_INI_BLK, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_ERROR);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO abort transfer message is received, abort code is zero
///
/// \Then no SDO message is sent
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
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO abort transfer message is received, abort code is not zero
///
/// \Then no SDO message is sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_CSAbort_AcNonzero) {
  StartCSDO();

  InitiateBlockDownloadRequest();

  const can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_RES, CO_SDO_AC_ERROR);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0u, CanSend::GetNumCalled());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO abort transfer message is received, but does not contain
///       the abort code
///
/// \Then no SDO message is sent
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

  can_msg msg = SdoCreateMsg::BlkDnSubRes(IDX, SUBIDX, DEFAULT_COBID_RES, 0,
                                          CO_SDO_SC_INI_BLK, sizeof(sub_type));
  msg.len = 3u;  // no index
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_ERROR);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
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

  const can_msg msg =
      SdoCreateMsg::BlkDnSubRes(IDX + 1u, SUBIDX, DEFAULT_COBID_RES, 0,
                                CO_SDO_SC_INI_BLK, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_ERROR);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
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

  can_msg msg = SdoCreateMsg::BlkDnSubRes(IDX, SUBIDX, DEFAULT_COBID_RES, 0,
                                          CO_SDO_SC_INI_BLK, sizeof(sub_type));
  msg.len = 4u;  // no number of segments per block
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_abort = SdoInitExpectedData::U32(
      CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_BLK_SIZE);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
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

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO block download sub-block response is received
///
/// \Then an SDO message with CO_SDO_SEQ_LAST command specifier with correct
///       sequence number and segment data was sent
///       \Calls ldle_u16()
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_Nominal) {
  StartCSDO();

  InitiateBlockDownloadRequest(IDX, SUBIDX, 0x1234u);

  uint_least8_t sequence_number = 0;
  const can_msg msg =
      SdoCreateMsg::BlkDnSubRes(IDX, SUBIDX, DEFAULT_COBID_RES, sequence_number,
                                CO_SDO_SC_INI_BLK, sizeof(sub_type));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  ++sequence_number;
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_last = SdoInitExpectedData::Segment(
      CO_SDO_SEQ_LAST | sequence_number, {0x34u, 0x12u});
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_last.data());
}

///@}

/// @name CSDO send 'block download sub-block' request
///@{

/// TODO(N7S): test cases for co_csdo_send_blk_dn_sub_req()

///@}

/// @name CSDO block download sub-block
///@{

/// TODO(N7S): test cases for co_csdo_blk_dn_sub_on_enter()
/// TODO(N7S): test cases for co_csdo_blk_dn_sub_on_abort()
/// TODO(N7S): test cases for co_csdo_blk_dn_sub_on_time()
/// TODO(N7S): test cases for co_csdo_blk_dn_sub_on_recv()

///@}

/// @name CSDO send 'block download end' request
///@{

/// TODO(N7S): test cases for co_csdo_send_blk_dn_end_req()

///@}

/// @name CSDO block download end
///@{

/// TODO(N7S): test cases for co_csdo_blk_dn_end_on_abort()
/// TODO(N7S): test cases for co_csdo_blk_dn_end_on_time()
/// TODO(N7S): test cases for co_csdo_blk_dn_end_on_recv()

///@}

/// @name CSDO block upload sub-block
///@{

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
        void* const data) {
    POINTERS_EQUAL(csdo, csdo_);
    CHECK_EQUAL(idx, idx_);
    CHECK_EQUAL(subidx, subidx_);
    CHECK_EQUAL(size, size_);
    CHECK_EQUAL(nbyte, nbyte_);
    if ((data != nullptr) && (data_ != nullptr)) {
      for (size_t i = 0; i < nbyte; i++) {
        CHECK_EQUAL(static_cast<uint_least8_t*>(data)[i],
                    static_cast<uint_least8_t*>(data_)[i]);
      }
    }
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

class SampleValue {
  using sub_type64 = co_unsigned64_t;
  using segment_data_t = std::vector<uint_least8_t>;

 public:
  static segment_data_t
  GetFirstSegment() {
    segment_data_t segment(CO_SDO_SEG_MAX_DATA_SIZE, 0);
    std::copy(val2dn.begin(),
              std::next(val2dn.begin(), CO_SDO_SEG_MAX_DATA_SIZE),
              segment.begin());

    return segment;
  }

  static segment_data_t
  GetLastSegment() {
    return segment_data_t(1u, val2dn.back());
  }

  static std::array<uint_least8_t, sizeof(sub_type64)>
  StLe64InArray(const sub_type64 val) {
    std::array<uint_least8_t, sizeof(sub_type64)> array = {0};
    stle_u64(array.data(), val);

    return array;
  }

  static void*
  GetVal2DnPtr() {
    return val2dn.data();
  }

 private:
  static const sub_type64 VAL = 0x1234567890abcdefu;
  static std::array<uint_least8_t, sizeof(sub_type64)> val2dn;
};
std::array<uint_least8_t, sizeof(SampleValue::sub_type64)> SampleValue::val2dn =
    SampleValue::StLe64InArray(SampleValue::VAL);

/// \Given a pointer to the CSDO service (co_csdo_t) which has initiated block
///        upload transfer (the correct request was sent by the client)
///
/// \When all required SDO segments are received
///
/// \Then an SDO message with a client command specifier block upload request
///       and a subcommand block upload response, last received sequence number
///       and the block size is sent; custom CSDO block upload indication
///       function is called with a pointer to the buffer containing
///       the received bytes
TEST(CO_Csdo, CoCsdoBlkUpSubOnRecv_Nominal) {
  const co_unsigned8_t subidx_u64 = SUBIDX + 1u;
  obj2020->InsertAndSetSub(subidx_u64, SUB_TYPE64, co_unsigned64_t{0});
  co_csdo_set_up_ind(csdo, CoCsdoInd::Func, nullptr);
  StartCSDO();

  InitiateBlockUploadRequest(IDX, subidx_u64, sizeof(sub_type64));
  CHECK_EQUAL(1, CoCsdoInd::GetNumCalled());
  CoCsdoInd::Check(csdo, IDX, subidx_u64, sizeof(sub_type64), 0, nullptr);
  CoCsdoInd::Clear();

  co_unsigned8_t seqno = 1u;
  const can_msg msg = SdoCreateMsg::UpSeg(DEFAULT_COBID_RES, seqno,
                                          SampleValue::GetFirstSegment());
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoCsdoInd::GetNumCalled());

  ++seqno;

  can_msg last_msg = SdoCreateMsg::UpSeg(DEFAULT_COBID_RES, seqno,
                                         SampleValue::GetLastSegment());
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
  CoCsdoInd::Check(csdo, IDX, subidx_u64, sizeof(sub_type64),
                   sizeof(sub_type64), SampleValue::GetVal2DnPtr());
  CoCsdoInd::Clear();
}

/// TODO(N7S): test cases for co_csdo_blk_up_sub_on_time()
/// TODO(N7S): test cases for co_csdo_blk_up_sub_on_recv()
/// TODO(N7S): test cases for co_csdo_blk_up_sub_on_abort()

///@}

/// @name CSDO block upload end
///@{

/// TODO(N7S): test cases for co_csdo_blk_up_end_on_abort()
/// TODO(N7S): test cases for co_csdo_blk_up_end_on_time()
/// TODO(N7S): test cases for co_csdo_blk_up_end_on_recv()

///@}

/// @name CSDO send 'download segment' request
///@{

/// TODO(N7S): test cases for co_csdo_send_dn_seg_req()

///@}

/// @name CSDO download segment
///@{

/// TODO(N7S): test cases for co_csdo_dn_seg_on_enter()
/// TODO(N7S): test cases for co_csdo_dn_seg_on_recv()
/// TODO(N7S): test cases for co_csdo_dn_seg_on_abort()

///@}

/// @name CSDO send block upload sub-block response
///@{

/// TODO(N7S): test cases for co_csdo_send_blk_up_sub_res()
/// TODO(N7S): test cases for co_csdo_send_blk_up_end_res()
/// TODO(N7S): test cases for co_csdo_blk_up_end_res()

///@}

/// @name CSDO send start upload request
///@{

/// TODO(N7S): test cases for co_csdo_send_start_up_req()

///@}

/// @name CSDO upload segment
///@{

/// TODO(N7S): test cases for co_csdo_up_seg_on_time()
/// TODO(N7S): test cases for co_csdo_up_seg_on_recv()

///@}
