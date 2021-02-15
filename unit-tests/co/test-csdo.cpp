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

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>
#include <lely/co/csdo.h>
#include <lely/co/sdo.h>
#include <lely/util/endian.h>

#include "allocators/default.hpp"
#include "allocators/limited.hpp"

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/array-init.hpp"

#include "lely-unit-test.hpp"
#include "lely-cpputest-ext.hpp"
#include "override/lelyco-val.hpp"

TEST_GROUP(CO_CsdoInit) {
  const co_unsigned8_t CSDO_NUM = 0x01u;
  static const co_unsigned8_t DEV_ID = 0x01u;
  co_csdo_t* csdo = nullptr;
  co_dev_t* dev = nullptr;
  can_net_t* failing_net = nullptr;
  can_net_t* net = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  Allocators::Default defaultAllocator;
  Allocators::Limited limitedAllocator;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(defaultAllocator.ToAllocT());
    CHECK(net != nullptr);

    limitedAllocator.LimitAllocationTo(can_net_sizeof());
    failing_net = can_net_create(limitedAllocator.ToAllocT());
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
///       else if \__MINGW32__ and !__MINGW64__: 108 is returned;
///       else: 248 is returned
TEST(CO_CsdoInit, CoCsdoSizeof_Nominal) {
  const auto ret = co_csdo_sizeof();

#if defined(LELY_NO_MALLOC) || defined(__MINGW64__)
  CHECK_EQUAL(256u, ret);
#else
#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(108u, ret);
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
TEST(CO_CsdoInit, CoCsdoCreate_FailCsdoAlloc) {
  co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number equal zero
///
/// \Then a null pointer is returned
TEST(CO_CsdoInit, CoCsdoCreate_NumZero) {
  const co_unsigned8_t CSDO_NUM = 0;

  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number higher than CO_NUM_SDOS
///
/// \Then a null pointer is returned
TEST(CO_CsdoInit, CoCsdoCreate_NumTooHigh) {
  const co_unsigned8_t CSDO_NUM = CO_NUM_SDOS + 1u;

  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
}

/// \Given a pointer to the device (co_dev_t) with 0x1280 object inserted
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number
///
/// \Then a non-null pointer is returned, default values are set
TEST(CO_CsdoInit, CoCsdoCreate_WithObj1280) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());

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

/// \Given a pointer to the device (co_dev_t) without server parameter object
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and a CSDO number
///
/// \Then a null pointer is returned
TEST(CO_CsdoInit, CoCsdoCreate_NoServerParameterObj) {
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
}

/// \Given a pointer to the device (co_dev_t) with 0x1280 object inserted
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and a CSDO number
///       but can_recv_create() fails
///
/// \Then a null pointer is returned
TEST(CO_CsdoInit, CoCsdoCreate_RecvCreateFail) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());

  limitedAllocator.LimitAllocationTo(co_csdo_sizeof());
  co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
}

/// \Given a pointer to the device (co_dev_t) with 0x1280 object inserted
///
/// \When co_csdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and a CSDO number
///       but can_timer_create() fails
///
/// \Then a null pointer is returned
TEST(CO_CsdoInit, CoCsdoCreate_TimerCreateFail) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());

  limitedAllocator.LimitAllocationTo(co_csdo_sizeof() + can_recv_sizeof());
  co_csdo_t* const csdo = co_csdo_create(failing_net, dev, CSDO_NUM);

  POINTERS_EQUAL(nullptr, csdo);
}

///@}

/// @name co_csdo_destroy()
///@{

/// \Given a null pointer to the CSDO service (co_csdo_t)
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
TEST(CO_CsdoInit, CoCsdoDestroy_Nominal) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());
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
TEST(CO_CsdoInit, CoCsdoStart_NoDev) {
  co_csdo_t* const csdo = co_csdo_create(net, nullptr, CSDO_NUM);
  CHECK(csdo != nullptr);

  const auto ret = co_csdo_start(csdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_csdo_is_stopped(csdo));
  CHECK_EQUAL(1, co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the started CSDO service (co_csdo_t)
///
/// \When co_csdo_start() is called
///
/// \Then 0 is returned, the service is not stopped, the service is idle
TEST(CO_CsdoInit, CoCsdoStart_AlreadyStarted) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK_EQUAL(0, co_csdo_start(csdo));

  const auto ret = co_csdo_start(csdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_csdo_is_stopped(csdo));
  CHECK_EQUAL(1, co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the CSDO service (co_csdo_t) with 0x1280 object
///        inserted
///
/// \When co_csdo_start() is called
///
/// \Then 0 is returned, the service is not stopped, the service is idle
TEST(CO_CsdoInit, CoCsdoStart_DefaultCSDO_WithObj1280) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);

  const auto ret = co_csdo_start(csdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_csdo_is_stopped(csdo));
  CHECK_EQUAL(1, co_csdo_is_idle(csdo));

  co_csdo_destroy(csdo);
}

///@}

/// @name co_csdo_stop()
///@{

/// \Given a pointer to the CSDO service (co_csdo_t) with 0x1280 object
///        inserted
///
/// \When co_csdo_stop() is called
///
/// \Then the service is stopped
TEST(CO_CsdoInit, CoCsdoStop_OnCreated) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK(csdo != nullptr);

  co_csdo_stop(csdo);

  CHECK_EQUAL(1, co_csdo_is_stopped(csdo));

  co_csdo_destroy(csdo);
}

/// \Given a pointer to the started CSDO service (co_csdo_t) with 0x1280 object
///        inserted
///
/// \When co_csdo_stop() is called
///
/// \Then the service is stopped
TEST(CO_CsdoInit, CoCsdoStop_OnStarted) {
  std::unique_ptr<CoObjTHolder> obj1280(new CoObjTHolder(0x1280u));
  co_dev_insert_obj(dev, obj1280->Take());
  co_csdo_t* const csdo = co_csdo_create(net, dev, CSDO_NUM);
  CHECK(csdo != nullptr);
  CHECK_EQUAL(0, co_csdo_start(csdo));

  co_csdo_stop(csdo);

  CHECK_EQUAL(1, co_csdo_is_stopped(csdo));

  co_csdo_destroy(csdo);
}

///@}

TEST_BASE(CO_Csdo) {
  static const co_unsigned8_t CSDO_NUM = 0x01u;
  static const co_unsigned8_t DEV_ID = 0x01u;
  co_csdo_t* csdo = nullptr;
  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  Allocators::Default defaultAllocator;

  std::unique_ptr<CoObjTHolder> obj1280;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

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
    net = can_net_create(defaultAllocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    can_net_set_send_func(net, CanSend::func, nullptr);

    CreateObjInDev(obj1280, 0x1280u);
    SetCli00HighestSubidxSupported(0x02u);
    SetCli01CobidReq(0x600u + DEV_ID);
    SetCli02CobidRes(0x580u + DEV_ID);
    csdo = co_csdo_create(net, dev, CSDO_NUM);
    CHECK(csdo != nullptr);
  }

  TEST_TEARDOWN() {
    co_csdo_destroy(csdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CoCsdoSetGet, CO_Csdo) {
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
/// \When co_csdo_get_up_ind() is called with a memory area to store
///       the results
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
