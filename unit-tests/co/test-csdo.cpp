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
#include <libtest/override/lelyco-val.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/array-init.hpp"

class ConciseDcf {
 private:
  ConciseDcf(std::initializer_list<size_t> type_sizes) {
    const size_t size = std::accumulate(
        std::begin(type_sizes), std::end(type_sizes), sizeof(co_unsigned32_t),
        [](const size_t& a, const size_t& b) { return a + EntrySize(b); });
    buffer.assign(size, 0);
  }

 public:
  template <typename... types>
  static ConciseDcf
  MakeForEntries() {
    return ConciseDcf({sizeof(types)...});
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
  Size() {
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
///       with a failing allocator, the pointer to the device and a CSDO number
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
  co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

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

  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

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

  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to the device (co_dev_t) containing 0x1280 object in the
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
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to the device (co_dev_t) containing 0x1280 object in
///        the object dictionary
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and a CSDO number
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
  co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

/// \Given a pointer to the device (co_dev_t) containing 0x1280 object in
///        the object dictionary
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and a CSDO number
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
  co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

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
  CHECK_EQUAL(0, co_csdo_is_stopped(csdo));
  CHECK_EQUAL(1u, co_csdo_is_idle(csdo));

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
  CHECK_EQUAL(0, co_csdo_is_stopped(csdo));
  CHECK_EQUAL(1u, co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the CSDO service (co_csdo_t) containing 0x1280 object in
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
  CHECK_EQUAL(0, co_csdo_is_stopped(csdo));
  CHECK_EQUAL(1u, co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

///@}

/// @name co_csdo_stop()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) containing 0x1280 object in
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

  CHECK_EQUAL(1u, co_csdo_is_stopped(csdo));

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

  CHECK_EQUAL(1u, co_csdo_is_stopped(csdo));

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
  void SetCli00HighestSubidxSupported(co_unsigned8_t subidx) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1280u, 0x00u);
    if (sub != nullptr)
      co_sub_set_val_u8(sub, subidx);
    else
      obj1280->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, subidx);
  }

  // obj 0x1280, sub 0x01 contains COB-ID client -> server
  void SetCli01CobidReq(co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1280u, 0x01u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1280->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1280, sub 0x02 contains COB-ID server -> client
  void SetCli02CobidRes(co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1280u, 0x02u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1280->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  co_unsigned32_t GetCli01CobidReq() {
    return co_dev_get_val_u32(dev, 0x1280u, 0x01u);
  }

  co_unsigned32_t GetCli02CobidRes() {
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
  int data = 0;  // clang-format fix

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
  const auto* ret = co_csdo_get_net(csdo);

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
  const auto* ret = co_csdo_get_dev(csdo);

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
  const auto* par = co_csdo_get_par(csdo);

  CHECK(par != nullptr);
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
  int data = 0;
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
  int data = 0;

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
  int data = 0;
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
  int data = 0;

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
  static membuf ind_mbuf;
  static size_t num_called;
  const co_unsigned16_t SUB_TYPE = CO_DEFTYPE_UNSIGNED16;

  static uint_least16_t LoadLE_U16(const membuf* const mbuf) {
    CHECK(membuf_size(mbuf) >= sizeof(co_unsigned16_t));
    return ldle_u16(static_cast<uint_least8_t*>(membuf_begin(mbuf)));
  }

  const co_unsigned16_t IDX = 0x2020u;
  const co_unsigned8_t SUBIDX = 0x00u;
  const co_unsigned16_t INVALID_IDX = 0xffffu;
  const co_unsigned8_t INVALID_SUBIDX = 0xffu;
  const co_unsigned16_t VAL = 0xabcdu;
  std::unique_ptr<CoObjTHolder> obj2020;
  std::unique_ptr<CoObjTHolder> obj2021;
#if LELY_NO_MALLOC
  char buffer[sizeof(sub_type)] = {0};
  char ext_buffer[sizeof(sub_type)] = {0};
#endif

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

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateAndInsertObj(obj2020, IDX);
    obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));

    CoCsdoUpCon::Clear();
    CanSend::Clear();
  }

  TEST_TEARDOWN() {
    ind_mbuf = MEMBUF_INIT;
    num_called = 0;

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

  CHECK_EQUAL(1, ret);
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

  CHECK_EQUAL(0, ret);
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

  CHECK_EQUAL(0, ret);
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
                                 sizeof(VAL), CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                 CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                 CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                     &VAL, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                     nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                     &data, &mbuf, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
  for (size_t bytes_missing = sizeof(sub_type) + 1u;
       bytes_missing < dcf.Size() - sizeof(sub_type); bytes_missing++) {
    const errnum_t error_num = ERRNUM_FAULT;
    set_errnum(error_num);

    CHECK_EQUAL(dcf.Size(), co_dev_write_dcf(dev, IDX, IDX, dcf.Begin(),
                                             dcf.End() - bytes_missing));

    const auto ret =
        co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End() - bytes_missing,
                          CoCsdoDnCon::func, nullptr);

    CHECK_EQUAL(0, ret);
    CHECK_EQUAL(error_num, get_errnum());
    CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  const auto ret = co_dev_dn_dcf_req(dev, dcf.Begin(), dcf.End() - 1u,
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CoCsdoDnCon::Check(nullptr, IDX, SUBIDX, CO_SDO_AC_NO_SUB, nullptr);
}

/// \Given a pointer to the device (co_dev_t), a concise DCF with many entries
///
/// \When co_dev_dn_dcf_req() is called with pointers to the beginning and the
///       end of the buffer and a pointer to the confirmation function but
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
  obj2021->InsertAndSetSub(0x00u, SUB_TYPE, sub_type(0));
  auto combined_dcf = ConciseDcf::MakeForEntries<sub_type, sub_type>();
  CHECK_EQUAL(combined_dcf.Size(),
              co_dev_write_dcf(dev, IDX, OTHER_IDX, combined_dcf.Begin(),
                               combined_dcf.End()));

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_sub_set_dn_ind(obj2020->GetLastSub(), co_sub_failing_dn_ind, nullptr);
  const auto ret =
      co_dev_dn_dcf_req(dev, combined_dcf.Begin(), combined_dcf.End(),
                        CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
  obj2021->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  obj2021->InsertAndSetSub(ELEMENT_SUBIDX, CO_DEFTYPE_UNSIGNED8,
                           co_unsigned8_t(0));

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
  obj2021->InsertAndSetSub(ELEMENT_SUBIDX, SUB_TYPE, sub_type(0x1234u));

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
  co_sub_up_ind_t* req_up_ind = [](const co_sub_t* sub, co_sdo_req* req,
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
  co_sub_up_ind_t* const req_up_ind = [](const co_sub_t* sub, co_sdo_req* req,
                                         co_unsigned32_t ac,
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
  co_sub_up_ind_t* const req_up_ind = [](const co_sub_t* sub, co_sdo_req* req,
                                         co_unsigned32_t ac,
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
  co_sub_up_ind_t* const req_up_ind = [](const co_sub_t* sub, co_sdo_req* req,
                                         co_unsigned32_t ac,
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
  co_sub_up_ind_t* const req_up_ind = [](const co_sub_t* sub, co_sdo_req* req,
                                         co_unsigned32_t ac,
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
void
SetOneSecOnNet(can_net_t* const net) {
  timespec ts = {1u, 0u};
  can_net_set_time(net, &ts);
}

void
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
  CHECK_EQUAL(0, co_csdo_is_idle(csdo));
  uint_least8_t buffer[sizeof(sub_type)] = {0};

  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, sizeof(sub_type),
                                  CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0u, CanSend::num_called);
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

  uint_least8_t buffer[sizeof(sub_type)] = {0};
  stle_u16(buffer, 0x1234u);

  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, sizeof(sub_type),
                                  CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());

  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type)), IDX,
      SUBIDX, 0x1234u);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CHECK_EQUAL(2u, CanSend::num_called);
  CanSend::CheckSdoMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, CO_SDO_CS_ABORT,
                       IDX, SUBIDX, CO_SDO_AC_TIMEOUT);

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
  uint_least8_t* buffer = nullptr;

  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, buffer_size,
                                  CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());
  CHECK_EQUAL(1u, CanSend::num_called);

  std::vector<uint_least8_t> expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND, IDX, SUBIDX, buffer_size);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
  uint_least8_t buffer[buffer_size] = {0};
  const auto ret = co_csdo_dn_req(csdo, IDX, SUBIDX, buffer, buffer_size,
                                  CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());
  CHECK_EQUAL(1u, CanSend::num_called);

  std::vector<uint_least8_t> expected;
  expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND, IDX, SUBIDX, buffer_size);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
                                  CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());
  CHECK_EQUAL(1u, CanSend::num_called);

  std::vector<uint_least8_t> expected;
  expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type)), IDX,
      SUBIDX, 0x1234u);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());

  CoCsdoUpDnReq::AbortTransfer(net, DEFAULT_COBID_RES);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
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
///       the confirmation function and a null user-specified data
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
                                      nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::num_called);
  const auto expected = SdoInitExpectedData::U16(
      CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_EXP_SET(sizeof(co_unsigned16_t)),
      IDX, SUBIDX, VAL);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
}

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
                                      CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, CanSend::num_called);
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
                                      CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected = SdoInitExpectedData::U16(
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
                                      CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0u, CanSend::num_called);
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
  CHECK(!CanSend::Called());
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
  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected =
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
  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected =
      SdoInitExpectedData::Empty(CO_SDO_CCS_UP_INI_REQ, IDX, SUBIDX);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected_timeout =
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
  CHECK(!CanSend::Called());
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
  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected = SdoInitExpectedData::U32(
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
  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected = SdoInitExpectedData::U32(
      CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC | CO_SDO_SC_INI_BLK, IDX, SUBIDX,
      CO_SDO_MAX_SEQNO);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

///@}

/// @name CSDO: receive SDO message with no ongoing transfer
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) with no ongoing transfer
///
/// \When a non-abort SDO message is received
///
/// \Then 1 is returned by can_net_recv() and an SDO response is not sent
TEST(CO_Csdo, RecvWhenWaiting_Default) {
  StartCSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ;
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!CanSend::Called());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with no ongoing transfer
///
/// \When an abort SDO message is received
///
/// \Then 1 is returned by can_net_recv() and an SDO response is not sent
TEST(CO_Csdo, RecvWhenWaiting_CsAbort) {
  StartCSDO();

  can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, CO_SDO_AC_NO_DATA);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!CanSend::Called());
}

/// \Given a pointer to the CSDO service (co_csdo_t) with no ongoing transfer
///
/// \When an abort SDO message is received, the message is shorter than 8 bytes
///
/// \Then 1 is returned by can_net_recv() and an SDO response is not sent
TEST(CO_Csdo, RecvWhenWaiting_CsAbort_TooShortMsg) {
  StartCSDO();

  can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, 0u);
  msg.len = 7u;
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!CanSend::Called());
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
/// \Then -1 is returned and SDO message is not sent
TEST(CO_Csdo, CoCsdoBlkDnReq_NotStarted) {
  uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK(!CanSend::Called());
}

/// \Given a pointer to the CSDO service (co_csdo_t)
///
/// \When co_csdo_blk_dn_req() is called with an index and a sub-index of
///       the entry to download, a pointer to the bytes to be downloaded, size
///       of the entry, a pointer to the confirmation function and a null
///       user-specified data pointer
///
/// \Then 0 is returned and SDO block download request is sent
TEST(CO_Csdo, CoCsdoBlkDnReq_Nominal) {
  StartCSDO();

  uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CanSend::num_called);
  const co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                            CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;
  std::vector<uint_least8_t> expected =
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
TEST(CO_Csdo, CoCsdoBlkDnReq_TimeoutSet) {
  co_csdo_set_timeout(csdo, 999);
  StartCSDO();

  uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CanSend::num_called);
  const co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                            CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;
  std::vector<uint_least8_t> expected =
      SdoInitExpectedData::U32(cs, IDX, SUBIDX, sizeof(sub_type));
  CanSend::Clear();

  CoCsdoUpDnReq::SetOneSecOnNet(net);

  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

///@}

/// @name CSDO block download initiate
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) which initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO message with an incorrect command specifier
///       (not CO_SDO_SCS_BLK_DN_RES) is received
///
/// \Then 1 is returned by can_net_recv() and abort SDO transfer message is sent
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_IncorrectCS) {
  StartCSDO();

  uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CanSend::num_called);
  const co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                            CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;
  std::vector<uint_least8_t> expected =
      SdoInitExpectedData::U32(cs, IDX, SUBIDX, sizeof(sub_type));
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
}

/// \Given a pointer to the CSDO service (co_csdo_t) which initiated block
///        download transfer (the correct request was sent by the client)
///
/// \When an SDO message with no command specifier is received
///
/// \Then 1 is returned by can_net_recv() and abort SDO transfer message is sent
TEST(CO_Csdo, CoCsdoBlkDnIniOnRecv_NoCS) {
  StartCSDO();

  uint_least8_t bytes2dn[sizeof(sub_type)] = {0};
  const auto ret =
      co_csdo_blk_dn_req(csdo, IDX, SUBIDX, bytes2dn, sizeof(sub_type),
                         CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CanSend::num_called);
  const co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC |
                            CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;
  std::vector<uint_least8_t> expected =
      SdoInitExpectedData::U32(cs, IDX, SUBIDX, sizeof(sub_type));
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE, expected.data());
  CanSend::Clear();

  can_msg msg = SdoCreateMsg::Default(0xffffu, 0xffu, DEFAULT_COBID_RES);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::num_called);
  std::vector<uint_least8_t> expected_abort =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_REQ, 0, CO_SDO_MSG_SIZE,
                    expected_abort.data());
}

///@}
