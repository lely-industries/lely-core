/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
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

#include <array>
#include <memory>
#include <vector>

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>
#include <lely/co/crc.h>
#include <lely/co/csdo.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/ssdo.h>
#include <lely/co/val.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

TEST_GROUP(CO_SsdoInit) {
  can_net_t* net = nullptr;
  can_net_t* failing_net = nullptr;
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  Allocators::Default defaultAllocator;
  Allocators::Limited limitedAllocator;
  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned8_t SDO_NUM = 0x01u;
  static const co_unsigned32_t DEFAULT_COBID_REQ = 0x600u + DEV_ID;
  static const co_unsigned32_t DEFAULT_COBID_RES = 0x580u + DEV_ID;

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

/// @name co_ssdo_alignof()
///@{

/// \Given N/A
///
/// \When co_ssdo_alignof() is called
///
/// \Then if \__MINGW32__ and !__MINGW64__: 4 is returned; else 8 is returned
TEST(CO_SsdoInit, CoSsdoAlignof_Nominal) {
  const auto ret = co_ssdo_alignof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(4u, ret);
#else
  CHECK_EQUAL(8u, ret);
#endif
}

///@}

/// @name co_ssdo_sizeof()
///@{

/// \Given N/A
///
/// \When co_ssdo_sizeof() is called
///
/// \Then if LELY_NO_MALLOC: 1088 is returned;
///       else if \__MINGW32__ and !__MINGW64__: 104 is returned;
///       else 184 is returned
TEST(CO_SsdoInit, CoSsdoSizeof_Nominal) {
  const auto ret = co_ssdo_sizeof();

#ifdef LELY_NO_MALLOC
  CHECK_EQUAL(1088u, ret);
#else
#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(104u, ret);
#else
  CHECK_EQUAL(184u, ret);
#endif
#endif  // LELY_NO_MALLOC
}

///@}

/// @name co_ssdo_create()
///@{

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and an SDO number,
///       but SSDO service allocation fails
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_SsdoInit, CoSsdoCreate_FailSsdoAlloc) {
  const co_ssdo_t* const ssdo = co_ssdo_create(failing_net, dev, SDO_NUM);

  POINTERS_EQUAL(nullptr, ssdo);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and an SDO number equal zero
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls errnum2c()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls co_ssdo_get_alloc()
///       \Calls set_errc()
TEST(CO_SsdoInit, CoSsdoCreate_NumZero) {
  const co_ssdo_t* const ssdo = co_ssdo_create(net, dev, 0x00u);

  POINTERS_EQUAL(nullptr, ssdo);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and an SDO number higher than CO_NUM_SDOS
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls errnum2c()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls co_ssdo_get_alloc()
///       \Calls set_errc()
TEST(CO_SsdoInit, CoSsdoCreate_NumTooHigh) {
  const co_ssdo_t* const ssdo = co_ssdo_create(net, dev, CO_NUM_SDOS + 1u);

  POINTERS_EQUAL(nullptr, ssdo);
}

/// \Given a pointer to the device (co_dev_t) with an object dictionary which
///        does not contain the server parameter object
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and an SDO number of a non-default SSDO
///       service
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls co_ssdo_get_alloc()
TEST(CO_SsdoInit, CoSsdoCreate_NonDefault_NoServerParameterObject) {
  const co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM + 1u);

  POINTERS_EQUAL(nullptr, ssdo);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and an SDO number,
///       but can_recv_create() fails
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_get_id()
///       \Calls can_recv_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls mem_free()
///       \Calls co_ssdo_get_alloc()
TEST(CO_SsdoInit, CoSsdoCreate_RecvCreateFail) {
  limitedAllocator.LimitAllocationTo(co_ssdo_sizeof());

  const co_ssdo_t* const ssdo = co_ssdo_create(failing_net, dev, SDO_NUM);

  POINTERS_EQUAL(nullptr, ssdo);
}

/// \Given a pointer to the device (co_dev_t)
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t)
///       with a failing allocator, the pointer to the device and an SDO number,
///       but can_timer_create() fails
///
/// \Then a null pointer is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_get_id()
///       \Calls can_recv_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls get_errc()
///       \Calls can_recv_destroy()
///       \Calls set_errc()
///       \Calls mem_free()
///       \Calls co_ssdo_get_alloc()
TEST(CO_SsdoInit, CoSsdoCreate_TimerCreateFail) {
  limitedAllocator.LimitAllocationTo(co_ssdo_sizeof() + can_recv_sizeof());

  const co_ssdo_t* const ssdo = co_ssdo_create(failing_net, dev, SDO_NUM);

  POINTERS_EQUAL(nullptr, ssdo);
}

/// \Given a pointer to the device (co_dev_t) with an empty object dictionary
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and an SDO number of the default SSDO
///       service
///
/// \Then a pointer to the created SSDO service is returned, the service has
///       default values set
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_get_id()
///       \Calls can_recv_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
///       \Calls membuf_init()
///       \IfCalls{LELY_NO_MALLOC, memset()}
TEST(CO_SsdoInit, CoSsdoCreate_DefaultSsdo_NoServerParameterObject) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);

  CHECK(ssdo != nullptr);
  POINTERS_EQUAL(net, co_ssdo_get_net(ssdo));
  POINTERS_EQUAL(dev, co_ssdo_get_dev(ssdo));
  CHECK_EQUAL(SDO_NUM, co_ssdo_get_num(ssdo));
  const co_sdo_par* const par = co_ssdo_get_par(ssdo);
  CHECK_EQUAL(3u, par->n);
  CHECK_EQUAL(DEV_ID, par->id);
  CHECK_EQUAL(DEFAULT_COBID_REQ, par->cobid_req);
  CHECK_EQUAL(DEFAULT_COBID_RES, par->cobid_res);
  CHECK(co_ssdo_is_stopped(ssdo));
  POINTERS_EQUAL(can_net_get_alloc(net), co_ssdo_get_alloc(ssdo));

  co_ssdo_destroy(ssdo);
}

/// \Given a pointer to the device (co_dev_t) with an object dictionary
///        containing the default server parameter object
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and an SDO number of the default SSDO
///       service
///
/// \Then a pointer to the created SSDO service is returned, the service has
///       default values set
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_get_id()
///       \Calls can_recv_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
///       \Calls membuf_init()
///       \IfCalls{LELY_NO_MALLOC, memset()}
TEST(CO_SsdoInit, CoSsdoCreate_DefaultSsdo_WithServerParameterObject) {
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u));
  co_dev_insert_obj(dev, obj1200->Take());

  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);

  CHECK(ssdo != nullptr);
  POINTERS_EQUAL(net, co_ssdo_get_net(ssdo));
  POINTERS_EQUAL(dev, co_ssdo_get_dev(ssdo));
  CHECK_EQUAL(SDO_NUM, co_ssdo_get_num(ssdo));
  const co_sdo_par* const par = co_ssdo_get_par(ssdo);
  CHECK_EQUAL(3u, par->n);
  CHECK_EQUAL(DEV_ID, par->id);
  CHECK_EQUAL(DEFAULT_COBID_REQ, par->cobid_req);
  CHECK_EQUAL(DEFAULT_COBID_RES, par->cobid_res);
  CHECK(co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

/// \Given a pointer to the device (co_dev_t) with an object dictionary
///        containing a server parameter object
///
/// \When co_ssdo_create() is called with a pointer to the network (can_net_t),
///       the pointer to the device and an SDO number of the non-default SSDO
///       service
///
/// \Then a pointer to the created SSDO service is returned, the service has
///       default values set
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_ssdo_alignof()
///       \Calls co_ssdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_get_id()
///       \Calls can_recv_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls co_ssdo_get_alloc()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
///       \Calls membuf_init()
///       \IfCalls{LELY_NO_MALLOC, memset()}
TEST(CO_SsdoInit, CoSsdoCreate_NonDefaultSsdo_WithServerParameterObject) {
  const size_t num = 1u;
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u + num));
  co_dev_insert_obj(dev, obj1200->Take());
  const co_unsigned8_t sdo_num = SDO_NUM + num;

  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, sdo_num);

  CHECK(ssdo != nullptr);
  POINTERS_EQUAL(net, co_ssdo_get_net(ssdo));
  POINTERS_EQUAL(dev, co_ssdo_get_dev(ssdo));
  CHECK_EQUAL(sdo_num, co_ssdo_get_num(ssdo));
  const co_sdo_par* const par = co_ssdo_get_par(ssdo);
  CHECK_EQUAL(3u, par->n);
  CHECK_EQUAL(DEV_ID, par->id);
  CHECK_EQUAL(DEFAULT_COBID_REQ, par->cobid_req);
  CHECK_EQUAL(DEFAULT_COBID_RES, par->cobid_res);
  CHECK(co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

///@}

/// @name co_ssdo_destroy()
///@{

/// \Given a null pointer to an SDO service (co_ssdo_t)
///
/// \When co_ssdo_destroy() is called
///
/// \Then nothing is changed
TEST(CO_SsdoInit, CoSsdoDestroy_Nullptr) {
  co_ssdo_t* const ssdo = nullptr;

  co_ssdo_destroy(ssdo);
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_destroy() is called
///
/// \Then the SSDO service is destroyed
///       \Calls co_ssdo_stop()
///       \Calls membuf_fini()
///       \Calls co_sdo_req_fini()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_ssdo_get_alloc()
TEST(CO_SsdoInit, CoSsdoDestroy_Nominal) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);

  co_ssdo_destroy(ssdo);
}

///@}

/// @name co_ssdo_start()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t) with an empty object
///        dictionary
///
/// \When co_ssdo_start() is called
///
/// \Then 0 is returned, the service is not stopped
///       \Calls co_ssdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls can_recv_start()
TEST(CO_SsdoInit, CoSsdoStart_DefaultSsdo_NoObj1200) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

/// \Given a pointer to the started SSDO service (co_ssdo_t)
///
/// \When co_ssdo_start() is called
///
/// \Then 0 is returned, the service is not stopped
///       \Calls co_ssdo_is_stopped()
TEST(CO_SsdoInit, CoSsdoStart_AlreadyStarted) {
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u));
  co_dev_insert_obj(dev, obj1200->Take());
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);
  CHECK_EQUAL(0, co_ssdo_start(ssdo));

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing the default server parameter object
///
/// \When co_ssdo_start() is called
///
/// \Then 0 is returned, the service is not stopped
///       \Calls co_ssdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CO_SsdoInit, CoSsdoStart_DefaultSsdo_WithServerParameterObject) {
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u));
  co_dev_insert_obj(dev, obj1200->Take());
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

///@}

/// @name co_ssdo_stop()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_stop() is called
///
/// \Then the service is stopped
///       \Calls co_ssdo_is_stopped()
TEST(CO_SsdoInit, CoSsdoStop_OnCreated) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);

  co_ssdo_stop(ssdo);

  CHECK(co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

/// \Given a pointer to the started SSDO service (co_ssdo_t)
///
/// \When co_ssdo_stop() is called
///
/// \Then the service is stopped
///       \Calls co_ssdo_is_stopped()
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_find_obj()
TEST(CO_SsdoInit, CoSsdoStop_OnStarted) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SDO_NUM);
  co_ssdo_start(ssdo);

  co_ssdo_stop(ssdo);

  CHECK(co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

///@}

TEST_BASE(CO_Ssdo) {
  TEST_BASE_SUPER(CO_Ssdo);

  using sub_type = co_unsigned16_t;
  using sub_type8 = co_unsigned8_t;
  using sub_type64 = co_unsigned64_t;
  static const co_unsigned16_t SUB_TYPE = CO_DEFTYPE_UNSIGNED16;
  static const co_unsigned16_t SUB_TYPE64 = CO_DEFTYPE_UNSIGNED64;
  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned32_t CAN_ID = DEV_ID;
  static const co_unsigned32_t DEFAULT_COBID_REQ = 0x600u + DEV_ID;
  static const co_unsigned32_t DEFAULT_COBID_RES = 0x580u + DEV_ID;
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;
  static const size_t MSG_BUF_SIZE = 32u;
  static const co_unsigned8_t SDO_NUM = 0x01u;
  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;
  co_ssdo_t* ssdo = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1200;
  std::unique_ptr<CoObjTHolder> obj2020;
  std::array<can_msg, MSG_BUF_SIZE> msg_buf;

  Allocators::Default defaultAllocator;

  static co_unsigned32_t sub_dn_failing_ind(co_sub_t*, co_sdo_req*,
                                            const co_unsigned32_t ac, void*) {
    if (ac != 0) return ac;
    return CO_SDO_AC_NO_READ;
  }

  void StartSSDO() { CHECK_EQUAL(0, co_ssdo_start(ssdo)); }

  // obj 0x1200, sub 0x00 - highest sub-index supported
  void SetSrv00HighestSubidxSupported(const co_unsigned8_t subidx) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x00u);
    if (sub != nullptr)
      co_sub_set_val_u8(sub, subidx);
    else
      obj1200->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, subidx);
  }

  // obj 0x1200, sub 0x01 - COB-ID client -> server (rx)
  void SetSrv01CobidReq(const co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x01u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1200->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1200, sub 0x02 - COB-ID server -> client (tx)
  void SetSrv02CobidRes(const co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x02u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1200->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  co_unsigned32_t GetSrv01CobidReq() const {
    return co_dev_get_val_u32(dev, 0x1200u, 0x01u);
  }

  co_unsigned32_t GetSrv02CobidRes() const {
    return co_dev_get_val_u32(dev, 0x1200u, 0x02u);
  }

  void ResetCanSend() {
    CanSend::Clear();
    msg_buf.fill(CAN_MSG_INIT);
    CanSend::SetMsgBuf(msg_buf.data(), msg_buf.size());
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(defaultAllocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    obj1200.reset(new CoObjTHolder(0x1200u));
    co_dev_insert_obj(dev, obj1200->Take());

    SetSrv00HighestSubidxSupported(0x02u);
    SetSrv01CobidReq(DEFAULT_COBID_REQ);
    SetSrv02CobidRes(DEFAULT_COBID_RES);
    ssdo = co_ssdo_create(net, dev, SDO_NUM);
    CHECK(ssdo != nullptr);

    can_net_set_send_func(net, CanSend::Func, nullptr);
    CanSend::SetMsgBuf(msg_buf.data(), msg_buf.size());
    CanSend::Clear();
  }

  TEST_TEARDOWN() {
    CanSend::Clear();
    CoSubDnInd::Clear();

    co_ssdo_destroy(ssdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CoSsdoSetGet, CO_Ssdo){};

/// @name co_ssdo_get_net()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_get_net() is called
///
/// \Then a pointer to the network (can_net_t) of the SSDO service is returned
TEST(CoSsdoSetGet, CoSsdoGetNet_Nominal) {
  const auto* const ret = co_ssdo_get_net(ssdo);

  POINTERS_EQUAL(net, ret);
}

///@}

/// @name co_ssdo_get_dev()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_get_dev() is called
///
/// \Then a pointer to the device (co_dev_t) of the SSDO service is returned
TEST(CoSsdoSetGet, CoSsdoGetDev_Nominal) {
  const auto* const ret = co_ssdo_get_dev(ssdo);

  POINTERS_EQUAL(dev, ret);
}

///@}

/// @name co_ssdo_get_num()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_get_num() is called
///
/// \Then the service's SDO number is returned
TEST(CoSsdoSetGet, CoSsdoGetNum_Nominal) {
  const auto ret = co_ssdo_get_num(ssdo);

  CHECK_EQUAL(SDO_NUM, ret);
}

///@}

/// @name co_ssdo_get_par()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_get_par() is called
///
/// \Then a pointer to the parameter object of the SSDO service is returned
TEST(CoSsdoSetGet, CoSsdoGetPar_Nominal) {
  const auto* const ret = co_ssdo_get_par(ssdo);

  POINTER_NOT_NULL(ret);
  CHECK_EQUAL(3u, ret->n);
  CHECK_EQUAL(DEV_ID, ret->id);
  CHECK_EQUAL(DEFAULT_COBID_RES, ret->cobid_res);
  CHECK_EQUAL(DEFAULT_COBID_REQ, ret->cobid_req);
}

///@}

/// @name co_ssdo_get_timeout()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_get_timeout() is called
///
/// \Then default service's timeout value of zero is returned
TEST(CoSsdoSetGet, CoSsdoGetTimeout_Nominal) {
  const auto ret = co_ssdo_get_timeout(ssdo);

  CHECK_EQUAL(0u, ret);
}

///@}

/// @name co_ssdo_set_timeout()
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When co_ssdo_set_timeout() is called with a valid timeout value
///
/// \Then the requested timeout is set
TEST(CoSsdoSetGet, CoSsdoSetTimeout_ValidTimeout) {
  co_ssdo_set_timeout(ssdo, 1);

  CHECK_EQUAL(1, co_ssdo_get_timeout(ssdo));
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no timeout set
///
/// \When co_ssdo_set_timeout() is called with an invalid timeout value
///
/// \Then the timeout is not set
TEST(CoSsdoSetGet, CoSsdoSetTimeout_InvalidTimeout) {
  co_ssdo_set_timeout(ssdo, -1);

  CHECK_EQUAL(0, co_ssdo_get_timeout(ssdo));
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with a timeout set
///
/// \When co_ssdo_set_timeout() is called with a zero timeout value
///
/// \Then the timeout is disabled
///       \Calls can_timer_stop()
TEST(CoSsdoSetGet, CoSsdoSetTimeout_DisableTimeout) {
  co_ssdo_set_timeout(ssdo, 1);

  co_ssdo_set_timeout(ssdo, 0);

  CHECK_EQUAL(0, co_ssdo_get_timeout(ssdo));
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with a timeout set
///
/// \When co_ssdo_set_timeout() is called with a different timeout value
///
/// \Then the timeout is updated to the requested value
TEST(CoSsdoSetGet, CoSsdoSetTimeout_UpdateTimeout) {
  co_ssdo_set_timeout(ssdo, 1);

  co_ssdo_set_timeout(ssdo, 4);

  CHECK_EQUAL(4, co_ssdo_get_timeout(ssdo));
}

///@}

TEST_GROUP_BASE(CoSsdoUpdate, CO_Ssdo){};

/// @name Update and (de)activation of a Server-SDO service
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t) with a valid request COB-ID
///        and an invalid response COB-ID set
///
/// \When the SSDO service is updated (co_ssdo_start())
///
/// \Then the SSDO service's CAN frame receiver is deactivated
///       \Calls co_ssdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
TEST(CoSsdoUpdate, ReqCobidValid_ResCobidInvalid) {
  const co_unsigned32_t new_cobid_res = CAN_ID | CO_SDO_COBID_VALID;
  SetSrv02CobidRes(new_cobid_res);
  StartSSDO();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an invalid request
///        COB-ID and a valid response COB-ID set
///
/// \When the SSDO service is updated (co_ssdo_start())
///
/// \Then the SSDO service's CAN frame receiver is deactivated
///       \Calls co_ssdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
TEST(CoSsdoUpdate, ReqCobidInvalid_ResCobidValid) {
  const co_unsigned32_t new_cobid_req = CAN_ID | CO_SDO_COBID_VALID;
  SetSrv01CobidReq(new_cobid_req);
  StartSSDO();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an invalid request
///        COB-ID and an invalid response COB-ID set
///
/// \When the SSDO service is updated (co_ssdo_start())
///
/// \Then the SSDO service's CAN frame receiver is deactivated
///       \Calls co_ssdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
TEST(CoSsdoUpdate, ReqResCobidsInvalid) {
  const co_unsigned32_t new_cobid_req = CAN_ID | CO_SDO_COBID_VALID;
  const co_unsigned32_t new_cobid_res = CAN_ID | CO_SDO_COBID_VALID;
  SetSrv01CobidReq(new_cobid_req);
  SetSrv02CobidRes(new_cobid_res);
  StartSSDO();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with a valid request
///        COB-ID with a valid response COB-ID set
///
/// \When the SSDO service is updated (co_ssdo_start())
///
/// \Then the SSDO service's CAN frame receiver is activated
///       \Calls co_ssdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CoSsdoUpdate, ReqResCobidsValid) {
  const co_unsigned32_t new_cobid_req = CAN_ID;
  const co_unsigned32_t NEW_CAN_ID = CAN_ID + 1u;
  const co_unsigned32_t new_cobid_res = NEW_CAN_ID;
  SetSrv01CobidReq(new_cobid_req);
  SetSrv02CobidRes(new_cobid_res);
  StartSSDO();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  // CAN message is too short - the abort  code will be sent in response
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  std::vector<uint_least8_t> expected = SdoInitExpectedData::U32(
      CO_SDO_CS_ABORT, 0x0000u, 0x00u, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(NEW_CAN_ID, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the started SSDO service (co_ssdo_t) with a valid
///        request COB-ID with CO_SDO_COBID_FRAME and a valid response
///        COB-ID set
///
/// \When the SSDO service is updated (co_ssdo_start())
///
/// \Then the SSDO service's CAN frame receiver is activated
///       \Calls co_ssdo_is_stopped()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_sizeof_val()
///       \Calls memcpy()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CoSsdoUpdate, ReqResCobidsValid_CobidFrameSet) {
  const co_unsigned32_t new_cobid_req = CAN_ID | CO_SDO_COBID_FRAME;
  const co_unsigned32_t new_cobid_res = CAN_ID;
  SetSrv01CobidReq(new_cobid_req);
  SetSrv02CobidRes(new_cobid_res);
  StartSSDO();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = CAN_FLAG_IDE;
  // CAN message is too short - the abort  code will be sent in response
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  std::vector<uint_least8_t> expected = SdoInitExpectedData::U32(
      CO_SDO_CS_ABORT, 0x0000u, 0x00u, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(CAN_ID, 0, CO_SDO_MSG_SIZE, expected.data());
}

///@}

TEST_GROUP_BASE(CoSsdoTimer, CO_Ssdo){};

/// @name SSDO timer
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t) in 'download segment' state
///        with a timeout set
///
/// \When the timeout has expired (can_net_set_time())
///
/// \Then the SSDO service sends an SDO abort transfer message for the active
///       download transfer
///       \IfCalls{!LELY_NO_STDIO && !NDEBUG && !LELY_NO_DIAG, diag_at()}
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CoSsdoTimer, Timeout) {
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();
  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_DN_INI_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_DN_INI_RES, IDX, SUBIDX, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

///@}

TEST_GROUP_BASE(CoSsdoWaitOnRecv, CO_Ssdo){};

/// @name SSDO wait on receive
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t) with no ongoing requests
///
/// \When an SDO request with an expedited download initiate client command
///       specifier is received
///
/// \Then an SDO response with a download initiate server command specifier
///       is sent, requested entry is modified
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sub_dn_ind()
///       \Calls stle_u16()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoWaitOnRecv, DnIniReq) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0u});
  StartSSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_EXP |
                CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type));
  stle_u16(msg.data + 4u, 0x3214u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_DN_INI_RES, IDX, SUBIDX, 0u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());

  const co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(0x3214u, co_sub_get_val_u16(sub));
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no ongoing requests
///
/// \When an SDO request with an upload initiate client command specifier
///       is received
///
/// \Then an SDO response with an expedited upload server command specifier
///       initiate and the requested data is sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_reserve()
///       \Calls membuf_write()
///       \Calls membuf_begin()
///       \Calls membuf_size()
///       \Calls stle_u16()
///       \Calls memcpy()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoWaitOnRecv, UpIniReq) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_UP_INI_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type)), IDX,
      SUBIDX, 0xabcdu);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no ongoing requests
///
/// \When an SDO request with a block download client command specifier
///       is received
///
/// \Then an SDO response with a block download server command specifier
///       is sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls stle_u16()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoWaitOnRecv, BlkDnIniReq) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});
  StartSSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, IDX, SUBIDX, 127u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no ongoing requests
///
/// \When an SDO request with a block upload client command specifier
///       is received
///
/// \Then an SDO response with a block upload initiate server command specifier
///       is sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_reserve()
///       \Calls membuf_write()
///       \Calls membuf_begin()
///       \Calls membuf_size()
///       \Calls stle_u16()
///       \Calls memcpy()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoWaitOnRecv, BlkUpIniReq) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ;
  msg.data[4] = CO_SDO_MAX_SEQNO;
  msg.data[5] = 2u;  // protocol switch
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_EXP_SET(2u), IDX, SUBIDX,
      0xabcdu);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no ongoing requests
///
/// \When an SDO message with an abort command specifier is received
///
/// \Then an SDO response is not sent
TEST(CoSsdoWaitOnRecv, Abort) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no ongoing requests
///
/// \When an SDO message with an invalid client command specifier is received
///
/// \Then an SDO response with an abort transfer command specifier and
///       CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls membuf_clear()
TEST(CoSsdoWaitOnRecv, InvalidCS) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no ongoing requests
///
/// \When an SDO message with no command specifier is received
///
/// \Then an SDO response with an abort transfer command specifier and
///       CO_SDO_AC_NO_CS abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls membuf_clear()
TEST(CoSsdoWaitOnRecv, NoCS) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.len = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

///@}

TEST_GROUP_BASE(CoSsdoDnIniOnRecv, CO_Ssdo){};

/// @name SSDO download initiate
///@{

/// \Given a pointer to a started SSDO service (co_ssdo_t)
///
/// \When an SDO download initiate request is received, but the message does
///       not contain an index to download
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_OBJ abort code is sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls membuf_clear()
TEST(CoSsdoDnIniOnRecv, NoIdxSpecified) {
  StartSSDO();

  can_msg msg =
      SdoCreateMsg::DnIniReq(0xffffu, 0xffu, DEFAULT_COBID_REQ, nullptr);
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_OBJ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to a started SSDO service (co_ssdo_t)
///
/// \When an SDO download initiate request is received, but the message does
///       not contain a sub-index to download
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_SUB abort code is sent
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls membuf_clear()
TEST(CoSsdoDnIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::DnIniReq(IDX, 0xffu, DEFAULT_COBID_REQ, nullptr);
  msg.len = 3u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, 0x00u, CO_SDO_AC_NO_SUB);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to a started SSDO service (co_ssdo_t) with a timeout set,
///        download initiate request is received
///
/// \When the Server-SDO timeout expires before receiving the response from
///       a client
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TIMEOUT abort code is
///       sent
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls can_timer_stop()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls membuf_clear()
TEST(CoSsdoDnIniOnRecv, TimeoutSet) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  co_unsigned8_t size[4] = {0};
  stle_u32(size, sizeof(sub_type64));
  const can_msg msg = SdoCreateMsg::DnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ,
                                             size, CO_SDO_SEG_SIZE_SET(1u));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_DN_INI_RES, IDX, SUBIDX, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

/// \Given a pointer to a started SSDO service (co_ssdo_t)
///
/// \When an SDO expedited download initiate request for an non-existing object
///       is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_OBJ abort code is sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoDnIniOnRecv, Expedited_NoObject) {
  StartSSDO();

  const co_unsigned8_t val2dn[4] = {0};
  const can_msg msg = SdoCreateMsg::DnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ,
                                             val2dn, CO_SDO_INI_SIZE_EXP);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_OBJ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to a started SSDO service (co_ssdo_t)
///
/// \When an SDO expedited download initiate request for an existing entry
///       is received
///
/// \Then an SDO download initiate response is sent and the entry has
///       the requested value
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_sub_dn_ind()
///       \Calls stle_u16()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoDnIniOnRecv, Expedited) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});
  StartSSDO();

  co_unsigned8_t val2dn[4] = {0};
  stle_u16(val2dn, 0xabcd);
  const co_unsigned8_t cs = CO_SDO_INI_SIZE_IND | CO_SDO_INI_SIZE_EXP_SET(2u);
  const can_msg msg =
      SdoCreateMsg::DnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, cs);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_DN_INI_RES, IDX, SUBIDX, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());

  const co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(ldle_u16(val2dn), co_sub_get_val_u16(sub));
}

///@}

TEST_GROUP_BASE(CoSsdoUpIniOnRecv, CO_Ssdo) {
  int32_t ignore = 0;  // clang-format fix

  static co_unsigned32_t up_ind_size_zero(const co_sub_t* const sub,
                                          co_sdo_req* const req,
                                          co_unsigned32_t ac, void*) {
    if (ac != 0) return ac;

    co_sub_on_up(sub, req, &ac);
    req->size = 0;

    return ac;
  }
};

/// @name SSDO upload initiate
///@{

/// \Given a pointer to started SSDO service (co_ssdo_t)
///
/// \When an SDO upload initiate request is received, but the message does not
///       contain an index to upload
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_OBJ abort code is sent
///       \Calls stle_u32()
///       \Calls stle_u16()
///       \Calls can_net_send()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls membuf_clear()
TEST(CoSsdoUpIniOnRecv, NoIdxSpecified) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::UpIniReq(0xffffu, 0xffu, DEFAULT_COBID_REQ);
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_OBJ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to started SSDO service (co_ssdo_t)
///
/// \When an SDO upload initiate request is received, but the message does not
///       contain a sub-index to upload
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_SUB abort code is sent
///       \Calls ldle_u16()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls membuf_clear()
TEST(CoSsdoUpIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::UpIniReq(IDX, 0xffu, DEFAULT_COBID_REQ);
  msg.len = 3u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, 0x00u, CO_SDO_AC_NO_SUB);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to started SSDO service (co_ssdo_t)
///
/// \When an SDO upload initiate request is received, but the requested entry
///       has no read access
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_READ abort code is
///       sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoUpIniOnRecv, NoAccess) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});
  co_sub_set_access(obj2020->GetLastSub(), CO_ACCESS_WO);
  StartSSDO();

  const can_msg msg = SdoCreateMsg::UpIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_READ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to started SSDO service (co_ssdo_t)
///
/// \When an SDO upload initiate request is received for an entry with zero size
///
/// \Then an SDO upload initiate response with an indicated size equal to 0 is
///       sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
TEST(CoSsdoUpIniOnRecv, UploadToSubWithSizeZero) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0x1234u});
  co_sub_set_up_ind(obj2020->GetLastSub(), up_ind_size_zero, nullptr);
  StartSSDO();

  const can_msg msg = SdoCreateMsg::UpIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND, IDX, SUBIDX, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to started SSDO service (co_ssdo_t) with a timeout set,
///        an upload initiate request is received from the server
///
/// \When the Server-SDO timeout expires before receiving the segment from
///       the client
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TIMEOUT abort code is
///       sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls can_timer_timeout()
TEST(CoSsdoUpIniOnRecv, TimeoutSet) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x0123456789abcdefuL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  const can_msg msg = SdoCreateMsg::UpIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND, IDX,
                               SUBIDX, sizeof(sub_type64));
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &tp));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

/// \Given a pointer to started SSDO service (co_ssdo_t)
///
/// \When an SDO upload initiate request for a non-existing object is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_OBJ abort code is sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoUpIniOnRecv, NoObj) {
  StartSSDO();

  const can_msg msg = SdoCreateMsg::UpIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_OBJ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to started SSDO service (co_ssdo_t)
///
/// \When an SDO upload initiate request for an existing entry is received
///
/// \Then an SDO initiate upload response (expedited) with a correct entry value
///       is sent
///       \Calls ldle_u16()
///       \Calls co_sdo_req_fini()
///       \Calls co_sdo_req_init()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_code()
///       \Calls co_sub_up_ind()
///       \Calls membuf_reserve()
///       \Calls membuf_write()
///       \Calls membuf_begin()
///       \Calls membuf_size()
///       \Calls stle_u16()
///       \Calls memcpy()
///       \Calls can_net_send()
///       \Calls membuf_clear()
TEST(CoSsdoUpIniOnRecv, Expedited) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  const can_msg msg = SdoCreateMsg::UpIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_EXP_SET(2u), IDX, SUBIDX,
      0xabcdu);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

///@}

TEST_GROUP_BASE(CoSsdoBlkDnIniOnRecv, CO_Ssdo){};

/// @name SSDO block download initiate on receive
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block download initiate request is received, but the message
///       does not contain an index to download
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_OBJ abort code is sent
TEST(CoSsdoBlkDnIniOnRecv, NoIdxSpecified) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::BlkDnIniReq(0xffffu, 0xffu, DEFAULT_COBID_REQ);
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_OBJ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block download initiate request is received, but the message
///       does not contain a sub-index to download
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_SUB abort code is sent
TEST(CoSsdoBlkDnIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = SdoCreateMsg::BlkDnIniReq(IDX, 0xffu, DEFAULT_COBID_REQ);
  msg.len = 3u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, 0x00u, CO_SDO_AC_NO_SUB);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block download initiate request is received, but the client
///       subcommand is incorrect
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is sent
TEST(CoSsdoBlkDnIniOnRecv, InvalidCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  const can_msg msg =
      SdoCreateMsg::BlkDnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ, 0x0fu);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block download initiate request is received;
///       CO_SDO_BLK_SIZE_IND is set
///
/// \Then an SDO block download response is sent with a default blocksize
TEST(CoSsdoBlkDnIniOnRecv, BlkSizeSpecified) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});
  StartSSDO();

  const can_msg msg = SdoCreateMsg::BlkDnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ,
                                                CO_SDO_BLK_SIZE_IND);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, IDX, SUBIDX, 127u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with a timeout set,
///        download initiate request is received
///
/// \When the Server-SDO timeout expires before receiving the next SDO message
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TIMEOUT abort code is
///       sent
TEST(CoSsdoBlkDnIniOnRecv, TimeoutSet) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  const can_msg msg = SdoCreateMsg::BlkDnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, IDX, SUBIDX, 127u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &tp));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block download initiate request is received
///
/// \Then an SDO block download response is sent
TEST(CoSsdoBlkDnIniOnRecv, Nominal) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});
  StartSSDO();

  const can_msg msg = SdoCreateMsg::BlkDnIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, IDX, SUBIDX, 127u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

///@}

TEST_GROUP_BASE(CoSsdoBlkUpIniOnRecv, CO_Ssdo) {
  int32_t ignore = 0;  // clang-format fix

  // block upload initiate request
  static can_msg CreateBlkUp2020IniReqMsg(
      const co_unsigned8_t subidx = SUBIDX,
      const co_unsigned8_t blksize = CO_SDO_MAX_SEQNO) {
    const can_msg msg =
        SdoCreateMsg::BlkUpIniReq(IDX, subidx, DEFAULT_COBID_REQ, blksize);

    return msg;
  }
};

/// @name SSDO block upload initiate on receive
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received, but the client
///       subcommand is incorrect
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is sent
TEST(CoSsdoBlkUpIniOnRecv, InvalidSC) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, 1u);
  msg.data[0] |= 0x0fu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received, but the message
///       does not contain an index for upload
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_OBJ abort code is sent
TEST(CoSsdoBlkUpIniOnRecv, NoIdxSpecified) {
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(0xffu);
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, 0x0000u,
                                                 0x00u, CO_SDO_AC_NO_OBJ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received, but the message
///       does not contain a sub-index for upload
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_SUB abort code is sent
TEST(CoSsdoBlkUpIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(0xffu);
  msg.len = 3u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, 0x00u, CO_SDO_AC_NO_SUB);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received, but the message
///       does not contain a block size
///
/// \Then an SDO abort transfer message with CO_SDO_AC_BLK_SIZE abort code is
///       sent
TEST(CoSsdoBlkUpIniOnRecv, BlocksizeNotSpecified) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  msg.len = 4u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_BLK_SIZE);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received, but the specified
///       block size is greater than maximum block size
///
/// \Then an SDO abort transfer message with CO_SDO_AC_BLK_SIZE abort code is
///       sent
TEST(CoSsdoBlkUpIniOnRecv, BlocksizeMoreThanMaxSeqNum) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  const can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, CO_SDO_MAX_SEQNO + 1u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_BLK_SIZE);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received, but the specified
///       block size is zero
///
/// \Then an SDO abort transfer message with CO_SDO_AC_BLK_SIZE abort code is
///       sent
TEST(CoSsdoBlkUpIniOnRecv, BlocksizeZero) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  const can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, 0);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_BLK_SIZE);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received, but the message
///       does not contain a protocol switch threshold value
///
/// \Then an SDO block upload response is sent
TEST(CoSsdoBlkUpIniOnRecv, MissingProtocolSwitchThreshold) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  msg.len = 5u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_SIZE_IND | CO_SDO_BLK_CRC, IDX, SUBIDX,
      sizeof(sub_type));
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request for a non-existing object is
///       received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_OBJ abort code is sent
TEST(CoSsdoBlkUpIniOnRecv, NoObjPresent) {
  StartSSDO();

  const can_msg msg = CreateBlkUp2020IniReqMsg();
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_OBJ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request for a non-existing sub-object is
///       received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_SUB abort code is sent
TEST(CoSsdoBlkUpIniOnRecv, NoSubPresent) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  StartSSDO();

  const can_msg msg = CreateBlkUp2020IniReqMsg();
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_SUB);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with a timeout set,
///        block upload request is received
///
/// \When the Server-SDO timeout expires before receiving the next SDO message
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TIMEOUT abort code is
///       sent
TEST(CoSsdoBlkUpIniOnRecv, TimeoutSet) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  const can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, sizeof(sub_type64));
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_SIZE_IND | CO_SDO_BLK_CRC, IDX, SUBIDX,
      sizeof(sub_type64));
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with no timeout set
///
/// \When an SDO block upload initiate request is received; protocol switch
///       threshold value is equal to the size of the requested value in bytes
///
/// \Then an SDO upload initiate response is sent
TEST(CoSsdoBlkUpIniOnRecv, ReqSizeEqualToPst_TimeoutNotSet) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX);
  msg.data[5] = sizeof(sub_type64);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND, IDX,
                               SUBIDX, sizeof(sub_type64));
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with a timeout set
///
/// \When an SDO block upload initiate request is received; protocol switch
///       threshold value is equal to the size of the requested value in bytes;
///       block size is set as lower than the size of the value to upload
///
/// \Then an SDO upload initiate response with SO_SDO_SC_END_BLK flag set is
///       sent
TEST(CoSsdoBlkUpIniOnRecv, ReqSizeEqualToPst_MoreFrames_TimeoutSet) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, 5u);
  msg.data[5] = sizeof(sub_type64);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_UP_INI_RES | CO_SDO_SC_END_BLK, IDX,
                               SUBIDX, sizeof(sub_type64));
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received; protocol switch
///       threshold value is smaller than the size of the requested value;
///       block size is set as lower than the size of the value to upload
///
/// \Then an SDO upload initiate response with SO_SDO_SC_END_BLK flag set is
///       sent
TEST(CoSsdoBlkUpIniOnRecv, ReqSizeMoreThanPst) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX);
  msg.data[5] = sizeof(sub_type64) - 6u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_BLK_SIZE_IND, IDX, SUBIDX,
      sizeof(sub_type64));
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

#if !LELY_NO_CO_OBJ_UPLOAD
/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request is received; protocol switch
///       threshold value is equal to the size of the requested value in bytes;
///       the requested value has a custom upload indication function set which
///       claims that the value size is zero
///
/// \Then a segmented SDO upload initiate response with indicated size of zero
///       sent
TEST(CoSsdoBlkUpIniOnRecv, ReqSizeZero_NonZeroPst) {
  auto zero_req_size = [](const co_sub_t* const sub, co_sdo_req* const req,
                          co_unsigned32_t ac, void* const) -> co_unsigned32_t {
    if (ac != 0) return ac;

    const auto ret = co_sub_on_up(sub, req, &ac);
    assert((ret == 0 && ac == 0) || (ret == -1 && ac != 0));
    (void)ret;

    req->size = 0u;

    return ac;
  };

  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  co_obj_set_up_ind(obj2020->Get(), zero_req_size, nullptr);
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  msg.data[5] = sizeof(sub_type);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND, IDX, SUBIDX, 0u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}
#endif  // !LELY_NO_CO_OBJ_UPLOAD

/// \Given a pointer to the SSDO service (co_ssdo_t)
///
/// \When an SDO block upload initiate request for an existing entry is received
///
/// \Then an SDO block upload response is sent
TEST(CoSsdoBlkUpIniOnRecv, Nominal) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  const can_msg msg = CreateBlkUp2020IniReqMsg();
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_BLK_SIZE_IND, IDX, SUBIDX,
      2u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

///@}

TEST_GROUP_BASE(CoSsdoDnSegOnRecv, CO_Ssdo) {
  int32_t ignore = 0;  // clang-format fix

  // send segmented download initiate request to SSDO (0x2020, 0x00)
  void DownloadInitiateReq(const size_t size) {
    co_unsigned8_t size_buf[4] = {0};
    stle_u32(size_buf, static_cast<uint32_t>(size));
    can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
    msg.data[0] = CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND;
    stle_u16(msg.data + 1u, IDX);
    msg.data[3] = SUBIDX;
    memcpy(msg.data + 4u, size_buf, CO_SDO_INI_DATA_SIZE);

    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_DN_INI_RES, CanSend::msg.data);
    CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
    ResetCanSend();
  }
};

/// @name SSDO download segment on receive
///@{

/// \Given a pointer to the SSDO service (co_ssdo_t), segmented download
///        transfer is in progress
///
/// \When an SDO message with empty data section is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code was sent
TEST(CoSsdoDnSegOnRecv, NoCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  // receive empty segment
  can_msg msg =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, nullptr, 0u);
  msg.len = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t), segmented download
///        transfer is in progress
///
/// \When an SDO abort transfer message was received
///
/// \Then no SDO message is sent
TEST(CoSsdoDnSegOnRecv, AbortCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ);
  const auto ret_abort = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret_abort);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress
///
/// \When a message with CO_SDO_CS_ABORT command specifier is received
///
/// \Then CAN message is not sent, download indication function is called with
///       the requested abort code, the requested entry is not changed
TEST(CoSsdoDnSegOnRecv, AbortAfterFirstSegment) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();
  co_obj_t* const obj = co_dev_find_obj(dev, IDX);
  co_obj_set_dn_ind(obj, CoSubDnInd::Func, nullptr);

  DownloadInitiateReq(sizeof(sub_type64));

  const uint8_t bytes_per_segment = 4u;
  const uint_least8_t val2dn[sizeof(sub_type64)] = {0x12u, 0x34u, 0x56u, 0x78u,
                                                    0x90u, 0xabu, 0xcdu, 0xefu};

  const can_msg first_segment = SdoCreateMsg::DnSegReq(
      IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, bytes_per_segment);
  CHECK_EQUAL(1, can_net_recv(net, &first_segment, 0));
  CanSend::Clear();
  CoSubDnInd::Clear();

  const can_msg abort_transfer =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, CO_SDO_AC_NO_DATA);
  CHECK_EQUAL(1, can_net_recv(net, &abort_transfer, 0));
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK(CoSubDnInd::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_DATA, CoSubDnInd::ac);

  CHECK_EQUAL(0u, co_dev_get_val_u64(dev, IDX, SUBIDX));
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress
///
/// \When a message with CO_SDO_CS_ABORT command specifier is received;
///       the message's length is less than 8 bytes
///
/// \Then CAN message is not sent, download indication function is called with
///       CO_SDO_AC_ERROR abort code, the requested entry is not changed
TEST(CoSsdoDnSegOnRecv, AbortAfterFirstSegment_MsgTooShort) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();
  co_obj_t* const obj = co_dev_find_obj(dev, IDX);
  co_obj_set_dn_ind(obj, CoSubDnInd::Func, nullptr);

  DownloadInitiateReq(sizeof(sub_type64));

  const uint8_t bytes_per_segment = 4u;
  const uint_least8_t val2dn[sizeof(sub_type64)] = {0x12u, 0x34u, 0x56u, 0x78u,
                                                    0x90u, 0xabu, 0xcdu, 0xefu};

  const can_msg first_segment = SdoCreateMsg::DnSegReq(
      IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, bytes_per_segment);
  CHECK_EQUAL(1, can_net_recv(net, &first_segment, 0));
  CanSend::Clear();
  CoSubDnInd::Clear();

  can_msg abort_transfer =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, CO_SDO_AC_NO_DATA);
  abort_transfer.len -= 1u;
  CHECK_EQUAL(1, can_net_recv(net, &abort_transfer, 0));
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK(CoSubDnInd::Called());
  CHECK_EQUAL(CO_SDO_AC_ERROR, CoSubDnInd::ac);

  CHECK_EQUAL(0u, co_dev_get_val_u64(dev, IDX, SUBIDX));
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress
///
/// \When an SDO message with invalid command specifier is received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is sent
TEST(CoSsdoDnSegOnRecv, InvalidCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const co_unsigned8_t val2dn[4u] = {0};
  const can_msg msg =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, 4u, 0xffu);
  const auto ret_abort = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret_abort);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress
///
/// \When two following SDO segments are received with toggle bit not changed
///
/// \Then no SDO message is sent
TEST(CoSsdoDnSegOnRecv, NoToggle) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const co_unsigned8_t val2dn[sizeof(sub_type64)] = {0};

  // send first segment: 4 bytes
  const can_msg msg =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, 4u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_DN_SEG_RES, 0x0000u, 0x00u, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  // send last segment: next 4 bytes
  const can_msg msg2 = SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ,
                                              val2dn + 4u, 4u, CO_SDO_SEG_LAST);
  CHECK_EQUAL(1, can_net_recv(net, &msg2, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry; segmented download transfer is in progress
///
/// \When an SDO segment is received, but the message contains less bytes than
///       the declared size
///
/// \Then an SDO abort transfer message with CO_SDO_AC_NO_CS abort code is
///       sent
TEST(CoSsdoDnSegOnRecv, MsgLenLessThanSegmentSize) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const co_unsigned8_t val2dn[8] = {0};
  can_msg msg =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, 6u);
  msg.len = 5u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry; segmented download transfer is in progress
///
/// \When an SDO segment with more bytes than expected in this transfer is
///       received
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TYPE_LEN_HI abort code is
///       sent
TEST(CoSsdoDnSegOnRecv, SegmentTooBig) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type));

  const co_unsigned8_t val2dn[4] = {0};
  const can_msg msg =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, 4u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_TYPE_LEN_HI);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress
///
/// \When a too short SDO segment is received but the CO_SDO_SEG_LAST bit is set
///
/// \Then an SDO abort transfer message with CO_SDO_AC_TYPE_LEN_LO abort code is
///       sent
TEST(CoSsdoDnSegOnRecv, SegmentTooShort) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const co_unsigned8_t val2dn[sizeof(sub_type64) - 1u] = {0};
  const can_msg msg = SdoCreateMsg::DnSegReq(
      IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn,
      static_cast<uint8_t>(sizeof(sub_type64) - 1u), CO_SDO_SEG_LAST);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_TYPE_LEN_LO);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress
///
/// \When an SDO segment is received but the download indication function
///       returns an abort code
///
/// \Then an SDO abort transfer message with the abort code returned by
///       the download indication function is sent
TEST(CoSsdoDnSegOnRecv, FailDnInd) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  co_sub_set_dn_ind(sub, sub_dn_failing_ind, nullptr);
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const co_unsigned8_t val2dn[4] = {0};
  const can_msg msg =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, 4u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_READ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress; an SSDO timeout is set
///
/// \When an SDO segment is received; Server-SDO timeout expires before
///       receiving the next segment from the client
///
/// \Then an SDO abort message with CO_SDO_AC_TIMEOUT abort code is sent
TEST(CoSsdoDnSegOnRecv, TimeoutSet) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_ssdo_set_timeout(ssdo, 1);
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const co_unsigned8_t val2dn[4] = {0x01u, 0x23u, 0x45u, 0x67u};

  // send first segment: 4 bytes
  const can_msg msg =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, 4u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_DN_SEG_RES, 0x0000u, 0x00u, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_timeout =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_timeout.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an entry which is at least 8 bytes long; segmented
///        download transfer is in progress
///
/// \When all required SDO segments with a data to download are received
///
/// \Then an SDO download segment reponse is sent and the entry's value is
///       changed
TEST(CoSsdoDnSegOnRecv, Nominal) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  const co_unsigned8_t val2dn[8] = {0x01u, 0x23u, 0x45u, 0x67u,
                                    0x89u, 0xabu, 0xcdu, 0xefu};

  // send first segment: 4 bytes
  const can_msg msg_first =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn, 4u);
  CHECK_EQUAL(1, can_net_recv(net, &msg_first, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SCS_DN_SEG_RES, 0x0000u, 0x00u, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  ResetCanSend();

  // send last segment: next 4 bytes
  const can_msg msg_last =
      SdoCreateMsg::DnSegReq(IDX, SUBIDX, DEFAULT_COBID_REQ, val2dn + 4u, 4u,
                             CO_SDO_SEG_LAST | CO_SDO_SEG_TOGGLE);
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected_response = SdoInitExpectedData::U32(
      CO_SDO_SCS_DN_SEG_RES | CO_SDO_SEG_TOGGLE, 0x0000u, 0x00u, 0);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_response.data());

  const co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  const sub_type64 val_u64 = co_sub_get_val_u64(sub);
  CHECK_EQUAL(0xefcdab8967452301u, val_u64);
}

///@}

struct AcTrackingUpInd {
  static co_unsigned32_t last_ac;

  static co_unsigned32_t
  Func(const co_sub_t* const sub, struct co_sdo_req* const req,
       co_unsigned32_t ac, void* const) {
    last_ac = ac;

    if (ac != 0) {
      return ac;
    }

    const auto ret = co_sub_on_up(sub, req, &ac);
    assert((ret == 0 && ac == 0) || (ret == -1 && ac != 0));
    (void)ret;

    return ac;
  }

  static void
  Clear() {
    last_ac = 0u;
  }
};

co_unsigned32_t AcTrackingUpInd::last_ac = 0u;

TEST_GROUP_BASE(CoSsdoUpSegOnRecv, CO_Ssdo) {
  int32_t ignore = 0;  // clang-format fix

  static const size_t INVALID_REQSIZE = 10u;

  static co_unsigned32_t up_ind_failing(const co_sub_t* const sub,
                                        co_sdo_req* const req,
                                        co_unsigned32_t ac, void*) {
    if (ac != 0) return ac;

    co_sub_on_up(sub, req, &ac);
    req->size = INVALID_REQSIZE;

    static size_t called = 0;
    if (called == 1u) ac = CO_SDO_AC_ERROR;
    called++;

    return ac;
  }

  static co_unsigned32_t up_ind_size_longer(const co_sub_t* const sub,
                                            co_sdo_req* const req,
                                            co_unsigned32_t ac, void*) {
    if (ac != 0) return ac;

    co_sub_on_up(sub, req, &ac);
    req->size = 10u;

    return ac;
  }

  // send segmented upload initiate request to SSDO (0x2020, 0x00)
  void UploadInitiateReq(const co_unsigned8_t size,
                         const co_unsigned32_t can_id = DEFAULT_COBID_RES,
                         const co_unsigned8_t flags = 0u) {
    const can_msg msg = SdoCreateMsg::UpIniReq(IDX, SUBIDX, DEFAULT_COBID_REQ);

    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    CHECK_EQUAL(can_id, CanSend::msg.id);
    CHECK_EQUAL(flags, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND,
                          CanSend::msg.data);
    CHECK_SDO_CAN_MSG_VAL(size, CanSend::msg.data);
    ResetCanSend();
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    AcTrackingUpInd::Clear();
  }
};

/// @name SSDO upload segment on receive
///@{

TEST(CoSsdoUpSegOnRecv, NoCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoUpSegOnRecv, CSAbort) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_obj_set_up_ind(obj2020->Get(), &AcTrackingUpInd::Func, nullptr);
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  const co_unsigned32_t ac = CO_SDO_AC_TIMEOUT;

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, ac);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(ac, AcTrackingUpInd::last_ac);
}

/// \Given a pointer to the SSDO service (co_ssdo_t), segmented upload transfer
///        is in progress
///
/// \When an SDO abort transfer message was received, the message did not
///       contain a complete abort code value
///
/// \Then no SDO message was sent, requested objects' upload indication function
///       was called with the CO_SDO_AC_ERROR abort code
TEST(CoSsdoUpSegOnRecv, CSAbort_NoAbortCode) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_obj_set_up_ind(obj2020->Get(), &AcTrackingUpInd::Func, nullptr);
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(CO_SDO_AC_ERROR, AcTrackingUpInd::last_ac);
}

TEST(CoSsdoUpSegOnRecv, InvalidCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoUpSegOnRecv, NoToggle) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x9876543210abcdefuL});
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg_last = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_last.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TOGGLE);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoUpSegOnRecv, TimeoutTriggered) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x9876543210abcdefuL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_IDX(IDX, CanSend::msg.data);
  CHECK_SDO_CAN_MSG_SUBIDX(SUBIDX, CanSend::msg.data);
  CHECK_SDO_CAN_MSG_AC(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoUpSegOnRecv, Nominal) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x9876543210abcdefuL});
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg_last = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_last.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(
      CO_SDO_SEG_SIZE_SET(1u) | CO_SDO_SEG_TOGGLE | CO_SDO_SEG_LAST,
      CanSend::msg.data);
  CHECK_EQUAL(0x98u, CanSend::msg.data[1]);
}

TEST(CoSsdoUpSegOnRecv, CoSsdoCreateSegRes_ExtendedId) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x9876543210abcdefuL});
  const co_unsigned32_t new_can_id = 0x1ffff580u + DEV_ID;
  const co_unsigned32_t new_cobid_res = new_can_id | CO_SDO_COBID_FRAME;
  SetSrv02CobidRes(new_cobid_res);
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64), new_can_id, CAN_FLAG_IDE);

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(new_can_id, CanSend::msg.id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg.flags);
  CHECK_SDO_CAN_MSG_CMD(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg_last = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_last.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(new_can_id, CanSend::msg.id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg.flags);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEG_TOGGLE | CO_SDO_SEG_LAST | 0x0c,
                        CanSend::msg.data);
  CHECK_EQUAL(0x98u, CanSend::msg.data[1]);
}

TEST(CoSsdoUpSegOnRecv, IndError) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x9876543210abcdefuL});
  co_obj_set_up_ind(obj2020->Get(), up_ind_failing, nullptr);
  StartSSDO();

  UploadInitiateReq(INVALID_REQSIZE);

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg_last = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_last.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_ERROR);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoUpSegOnRecv, IndReqSizeLonger) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x9876543210abcdefuL});
  co_obj_set_up_ind(obj2020->Get(), up_ind_size_longer, nullptr);
  StartSSDO();

  UploadInitiateReq(INVALID_REQSIZE);

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(0, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg_last = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_last.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEG_TOGGLE, CanSend::msg.data);
  CHECK_EQUAL(0x98u, CanSend::msg.data[1]);
  CHECK_EQUAL(0xefu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[3]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[4]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[7]);
}

///@}

struct AcTrackingDnInd {
  static co_unsigned32_t last_ac;

  static co_unsigned32_t
  Func(co_sub_t* const sub, struct co_sdo_req* const req, co_unsigned32_t ac,
       void* const) {
    last_ac = ac;

    if (ac != 0) {
      return ac;
    }

    // Capture and ignore the return value to suppress a Coverity Scan warning.
    // Any error can be detected by the caller by checking whether 'ac'
    // is non-zero.
    const auto ignored_result = co_sub_on_dn(sub, req, &ac);
    (void)ignored_result;

    return ac;
  }

  static void
  Clear() {
    last_ac = 0u;
  }
};

co_unsigned32_t AcTrackingDnInd::last_ac = 0u;

TEST_GROUP_BASE(CoSsdoBlkDn, CO_Ssdo) {
  int32_t ignore = 0;  // clang-format fix

  void InitBlkDn2020Sub00(const co_unsigned32_t size,
                          const co_unsigned8_t cs_flags = CO_SDO_BLK_CRC) {
    can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
    msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_SIZE_IND | cs_flags;
    stle_u32(msg.data + 4, size);

    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC,
                          CanSend::msg.data);
    CHECK_SDO_CAN_MSG_IDX(IDX, CanSend::msg.data);
    CHECK_SDO_CAN_MSG_SUBIDX(SUBIDX, CanSend::msg.data);
    CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[4]);
    ResetCanSend();
  }

  void EndBlkDn(const co_unsigned16_t crc, const uint8_t size = 0) {
    can_msg msg_end = CAN_MSG_INIT;
    if (size != 0)
      msg_end = SdoCreateMsg::BlkDnEnd(IDX, SUBIDX, DEFAULT_COBID_REQ, crc,
                                       CO_SDO_BLK_SIZE_SET(size));
    else
      msg_end = SdoCreateMsg::BlkDnEnd(IDX, SUBIDX, DEFAULT_COBID_REQ, crc);

    CHECK_EQUAL(1, can_net_recv(net, &msg_end, 0));

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_END_BLK,
                          CanSend::msg.data);
    CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
    ResetCanSend();
  }

  void ChangeStateToEnd() {
    can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
    msg_first_blk.data[1] = 0x01u;
    msg_first_blk.data[2] = 0x23u;
    msg_first_blk.data[3] = 0x45u;
    msg_first_blk.data[4] = 0x67u;
    msg_first_blk.data[5] = 0x89u;
    msg_first_blk.data[6] = 0xabu;
    msg_first_blk.data[7] = 0xcdu;
    CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

    CHECK_EQUAL(0, CanSend::GetNumCalled());

    can_msg msg_last_blk =
        SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 2u, CO_SDO_SEQ_LAST);
    msg_last_blk.data[1] = 0xefu;
    CHECK_EQUAL(1, can_net_recv(net, &msg_last_blk, 0));

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                          CanSend::msg.data);
    CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
    CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
    CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
    ResetCanSend();
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();
    AcTrackingDnInd::Clear();
  }
};

/// @name SSDO block download
///@{

TEST(CoSsdoBlkDn, Sub_NoCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg.len = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, Sub_CSAbort_OnFirstSeg) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg_first_blk.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CoSsdoBlkDn, Sub_SeqnoZero) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg_first_blk.data[0] = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_BLK_SEQ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, Sub_NoCrc) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64), 0);

  can_msg msg = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg.data[1] = 0x01u;
  msg.data[2] = 0x23u;
  msg.data[3] = 0x45u;
  msg.data[4] = 0x67u;
  msg.data[5] = 0x89u;
  msg.data[6] = 0xabu;
  msg.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  can_msg msg_last_blk =
      SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  ResetCanSend();

  EndBlkDn(0, 1u);  // no CRC in this transfer

  const co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(0xefcdab8967452301u, co_sub_get_val_u64(sub));
}

TEST(CoSsdoBlkDn, Sub_NoSub) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg.data[1] = 0x01u;
  msg.data[2] = 0x23u;
  msg.data[3] = 0x45u;
  msg.data[4] = 0x67u;
  msg.data[5] = 0x89u;
  msg.data[6] = 0xabu;
  msg.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_SUB);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, Sub_RequestLessThanSize) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(6u);

  can_msg msg = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg.data[1] = 0x01u;
  msg.data[2] = 0x23u;
  msg.data[3] = 0x45u;
  msg.data[4] = 0x67u;
  msg.data[5] = 0x89u;
  msg.data[6] = 0xabu;
  msg.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_TYPE_LEN_HI);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, Sub_Nominal) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  const sub_type64 val = 0xefcdab9078563412u;
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  memcpy(msg_first_blk.data + 1u, val_buf, 7u);
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  can_msg msg_last_blk =
      SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = val_buf[7];
  CHECK_EQUAL(1, can_net_recv(net, &msg_last_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(2, CanSend::msg.data[1]);    // ackseq
  CHECK_EQUAL(127, CanSend::msg.data[2]);  // blksize
  CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
  ResetCanSend();

  EndBlkDn(co_crc(0, val_buf, sizeof(sub_type64)), 1u);

  const co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(val, co_sub_get_val_u64(sub));
}

/// \Given a pointer to the SSDO service (co_ssdo_t), block download transfer
///        is in progress, first sub-block was already received
///
/// \When an SDO abort transfer message was received
///
/// \Then no SDO message was sent, requested objects' download indication
///       function was called with the received abort code
TEST(CoSsdoBlkDn, Sub_CSAbort_OnSubsequentSeg) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_obj_set_dn_ind(obj2020->Get(), &AcTrackingDnInd::Func, nullptr);
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  const sub_type64 val = 0xefcdab9078563412uL;
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  memcpy(msg_first_blk.data + 1u, val_buf, 7u);
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  const co_unsigned32_t ac = CO_SDO_AC_TIMEOUT;

  const can_msg msg_abort =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, ac);
  CHECK_EQUAL(1, can_net_recv(net, &msg_abort, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(ac, AcTrackingDnInd::last_ac);
}

/// \Given a pointer to the SSDO service (co_ssdo_t), block download transfer
///        is in progress, first sub-block was already received
///
/// \When an SDO abort transfer message was received, the message did not
///       contain a complete abort code value
///
/// \Then no SDO message was sent, requested objects' download indication
///       function was called with the CO_SDO_AC_ERROR abort code
TEST(CoSsdoBlkDn, Sub_CSAbort_NoAbortCodeOnSubsequentSeg) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_obj_set_dn_ind(obj2020->Get(), &AcTrackingDnInd::Func, nullptr);
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  const sub_type64 val = 0xefcdab9078563412uL;
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  memcpy(msg_first_blk.data + 1u, val_buf, 7u);
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  can_msg msg_abort = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_abort.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg_abort, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(CO_SDO_AC_ERROR, AcTrackingDnInd::last_ac);
}

TEST(CoSsdoBlkDn, Sub_InvalidSeqno_LastInBlk) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk =
      SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, CO_SDO_MAX_SEQNO);
  msg_first_blk.data[1] = 0x12u;
  msg_first_blk.data[2] = 0x34u;
  msg_first_blk.data[3] = 0x56u;
  msg_first_blk.data[4] = 0x78u;
  msg_first_blk.data[5] = 0x90u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(0, CanSend::msg.data[1]);
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, Sub_CrcError) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg_first_blk.data[1] = 0x01u;
  msg_first_blk.data[2] = 0x23u;
  msg_first_blk.data[3] = 0x45u;
  msg_first_blk.data[4] = 0x67u;
  msg_first_blk.data[5] = 0x89u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  can_msg msg_last_blk =
      SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
  ResetCanSend();

  can_msg msg_end = SdoCreateMsg::BlkDnEnd(IDX, SUBIDX, DEFAULT_COBID_REQ, 0);
  msg_end.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(1, can_net_recv(net, &msg_end, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_BLK_CRC);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());

  const co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(0, co_sub_get_val_u64(sub));
}

TEST(CoSsdoBlkDn, Sub_TimeoutTriggered) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg_first_blk.data[1] = 0x01u;
  msg_first_blk.data[2] = 0x23u;
  msg_first_blk.data[3] = 0x45u;
  msg_first_blk.data[4] = 0x67u;
  msg_first_blk.data[5] = 0x89u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndAbort) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg_first_blk.data[1] = 0x12u;
  msg_first_blk.data[2] = 0x34u;
  msg_first_blk.data[3] = 0x56u;
  msg_first_blk.data[4] = 0x78u;
  msg_first_blk.data[5] = 0x90u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  can_msg msg_last_blk =
      SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;

  CHECK_EQUAL(1, can_net_recv(net, &msg_last_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(1u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize

  co_ssdo_destroy(ssdo);
  ssdo = nullptr;
}

TEST(CoSsdoBlkDn, End_TimeoutTriggered) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndRecv_NoCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.len = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndRecv_CSAbort) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_obj_set_dn_ind(obj2020->Get(), &AcTrackingDnInd::Func, nullptr);
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  const co_unsigned32_t ac = CO_SDO_AC_TIMEOUT;

  const can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, ac);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(ac, AcTrackingDnInd::last_ac);
}

/// \Given a pointer to the SSDO service (co_ssdo_t), block download transfer
///        is in an end state
///
/// \When an SDO abort transfer message was received, the message did not
///       contain a complete abort code value
///
/// \Then no SDO message was sent, requested objects' download indication
///       function was called with the CO_SDO_AC_ERROR abort code
TEST(CoSsdoBlkDn, EndRecv_CSAbort_NoAbortCode) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  co_obj_set_dn_ind(obj2020->Get(), &AcTrackingDnInd::Func, nullptr);
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(CO_SDO_AC_ERROR, AcTrackingDnInd::last_ac);
}

TEST(CoSsdoBlkDn, EndRecv_InvalidCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndRecv_InvalidSC) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndRecv_InvalidLen) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  msg_first_blk.data[1] = 0x01u;
  msg_first_blk.data[2] = 0x23u;
  msg_first_blk.data[3] = 0x45u;
  msg_first_blk.data[4] = 0x67u;
  msg_first_blk.data[5] = 0x89u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  can_msg msg_last_blk =
      SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(1u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_TYPE_LEN_LO);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndRecv_InvalidSize) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  msg.data[0] |= CO_SDO_BLK_SIZE_SET(sizeof(sub_type64) - 2u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndRecv_ReqZero) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(0);

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  msg.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(0, CanSend::msg.data[1]);
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);
  ResetCanSend();

  // end, req zero
  msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  msg.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkDn, EndRecv_FailingDnInd) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64{0uL});
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  const sub_type64 val = 0xffffffffffffffffu;
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  can_msg msg_first_blk = SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 1u);
  memcpy(msg_first_blk.data + 1u, val_buf, 7u);
  CHECK_EQUAL(1, can_net_recv(net, &msg_first_blk, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());

  can_msg msg_last_blk =
      SdoCreateMsg::BlkDnSubReq(DEFAULT_COBID_REQ, 2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = val_buf[7u];

  CHECK_EQUAL(1, can_net_recv(net, &msg_last_blk, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                        CanSend::msg.data);
  CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_SDO_CAN_MSG_VAL(0, CanSend::msg.data);
  ResetCanSend();

  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  co_sub_set_dn_ind(sub, sub_dn_failing_ind, nullptr);
  can_msg msg_end = SdoCreateMsg::BlkDnEnd(
      IDX, SUBIDX, DEFAULT_COBID_REQ, co_crc(0, val_buf, sizeof(sub_type64)));
  msg_end.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(1, can_net_recv(net, &msg_end, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_READ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());

  CHECK_EQUAL(0, co_sub_get_val_u64(sub));
}

///@}

struct StreamingUpInd {
  static co_unsigned8_t valid_calls;
  static co_unsigned8_t num_called;
  static const co_unsigned8_t segment_size = 2u;

  static co_unsigned32_t
  Func(const co_sub_t* const sub, struct co_sdo_req* const req,
       const co_unsigned32_t ac, void* const) {
    if (ac != 0) {
      return ac;
    }

    if (num_called > valid_calls) {
      return CO_SDO_AC_DATA;
    }

    const void* const val = co_sub_get_val(sub);
    const co_unsigned16_t type = co_sub_get_type(sub);

    const size_t full_size = co_val_write(type, val, nullptr, nullptr);
    req->size = full_size;

    membuf* const buf = req->membuf;
    membuf_clear(buf);

    CHECK_COMPARE(membuf_reserve(buf, segment_size), >, 0uL);

    const auto* const bp = static_cast<const uint_least8_t*>(val);
    membuf_write(buf, bp + num_called * segment_size, segment_size);
    req->offset = num_called * segment_size;
    req->nbyte = segment_size;
    req->buf = membuf_begin(buf);

    ++num_called;

    return ac;
  }

  static void
  Clear() {
    valid_calls = 0u;
    num_called = 0u;
  }
};

co_unsigned8_t StreamingUpInd::valid_calls = 0u;
co_unsigned8_t StreamingUpInd::num_called = 0u;

TEST_GROUP_BASE(CoSsdoBlkUp, CO_Ssdo) {
  int32_t ignore = 0;  // clang-format fix

  static co_unsigned32_t up_ind_inc_req_offset(const co_sub_t* const sub,
                                               co_sdo_req* const req,
                                               co_unsigned32_t ac, void*) {
    if (ac != 0) return ac;

    co_sub_on_up(sub, req, &ac);
    req->offset++;

    return ac;
  }

  static co_unsigned32_t up_ind_big_reqsiz(const co_sub_t* const sub,
                                           co_sdo_req* const req,
                                           co_unsigned32_t ac, void*) {
    if (ac != 0) return ac;

    co_sub_on_up(sub, req, &ac);
    req->size = 3u;
    req->offset = 1u;

    return ac;
  }

  void InitBlkUp2020Req(const co_unsigned8_t subidx = SUBIDX,
                        const co_unsigned8_t blksize = CO_SDO_MAX_SEQNO) {
    can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
    msg.id = DEFAULT_COBID_REQ;
    msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC;
    stle_u16(msg.data + 1u, IDX);
    msg.data[3] = subidx;
    msg.data[4] = blksize;

    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  void CheckInitBlkUp2020ResData(const co_unsigned8_t subidx, const size_t size)
      const {
    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    CHECK_SDO_CAN_MSG_CMD(
        CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_BLK_SIZE_IND,
        CanSend::msg.data);
    CHECK_SDO_CAN_MSG_IDX(IDX, CanSend::msg.data);
    CHECK_SDO_CAN_MSG_SUBIDX(subidx, CanSend::msg.data);
    CHECK_SDO_CAN_MSG_VAL(size, CanSend::msg.data);
  }

  void ChangeStateToEnd() {
    can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
    msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

    // uploaded value from the server
    CHECK_EQUAL(2u, CanSend::GetNumCalled());
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[0].id);
    CHECK_EQUAL(0, CanSend::msg_buf[0].flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
    CHECK_SDO_CAN_MSG_CMD(1u, CanSend::msg_buf[0].data);
    CHECK_EQUAL(0xefu, CanSend::msg_buf[0].data[1]);
    CHECK_EQUAL(0xcdu, CanSend::msg_buf[0].data[2]);
    CHECK_EQUAL(0xabu, CanSend::msg_buf[0].data[3]);
    CHECK_EQUAL(0x89u, CanSend::msg_buf[0].data[4]);
    CHECK_EQUAL(0x67u, CanSend::msg_buf[0].data[5]);
    CHECK_EQUAL(0x45u, CanSend::msg_buf[0].data[6]);
    CHECK_EQUAL(0x23u, CanSend::msg_buf[0].data[7]);
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[1].id);
    CHECK_EQUAL(0, CanSend::msg_buf[1].flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[1].len);
    CHECK_EQUAL(CO_SDO_SEQ_LAST | CO_SDO_BLK_SIZE_IND,
                CanSend::msg_buf[1].data[0]);
    CHECK_EQUAL(0x54u, CanSend::msg_buf[1].data[1]);
    CHECK_EQUAL(0, CanSend::msg_buf[1].data[2]);
    CHECK_EQUAL(0, CanSend::msg_buf[1].data[3]);
    CHECK_EQUAL(0, CanSend::msg_buf[1].data[4]);
    CHECK_EQUAL(0, CanSend::msg_buf[1].data[5]);
    CHECK_EQUAL(0, CanSend::msg_buf[1].data[6]);
    CHECK_EQUAL(0, CanSend::msg_buf[1].data[7]);
    ResetCanSend();

    // client's confirmation response
    can_msg msg_con_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
    msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
    msg_con_res.data[1] = 2;                 // ackseq
    msg_con_res.data[2] = CO_SDO_MAX_SEQNO;  // blksize
    CHECK_EQUAL(1, can_net_recv(net, &msg_con_res, 0));

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_SDO_CAN_MSG_CMD(
        CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(1u),
        CanSend::msg.data);
    CHECK_EQUAL(10916u, ldle_u16(CanSend::msg.data + 1u));  // check CRC
    CHECK_EQUAL(0, CanSend::msg.data[3]);
    CHECK_EQUAL(0, CanSend::msg.data[4]);
    CHECK_EQUAL(0, CanSend::msg.data[5]);
    CHECK_EQUAL(0, CanSend::msg.data[6]);
    ResetCanSend();
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    StreamingUpInd::Clear();
  }
};

/// @name SSDO block upload
///@{

TEST(CoSsdoBlkUp, Sub_Nominal) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  const sub_type64 val = 0x5423456789abcdefu;
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, val);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(2u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[0].id);
  CHECK_EQUAL(0, CanSend::msg_buf[0].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
  CHECK_SDO_CAN_MSG_CMD(1u, CanSend::msg_buf[0].data);
  CHECK_EQUAL(0xefu, CanSend::msg_buf[0].data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg_buf[0].data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg_buf[0].data[3]);
  CHECK_EQUAL(0x89u, CanSend::msg_buf[0].data[4]);
  CHECK_EQUAL(0x67u, CanSend::msg_buf[0].data[5]);
  CHECK_EQUAL(0x45u, CanSend::msg_buf[0].data[6]);
  CHECK_EQUAL(0x23u, CanSend::msg_buf[0].data[7]);
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[1].id);
  CHECK_EQUAL(0, CanSend::msg_buf[1].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[1].len);
  CHECK_SDO_CAN_MSG_CMD(2u | CO_SDO_SEQ_LAST, CanSend::msg_buf[1].data);
  CHECK_EQUAL(0x54u, CanSend::msg_buf[1].data[1]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[2]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[3]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[4]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[5]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[6]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[7]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg_con_res.data[1] = 2u;                // ackseq
  msg_con_res.data[2] = CO_SDO_MAX_SEQNO;  // blksize
  CHECK_EQUAL(1, can_net_recv(net, &msg_con_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(1u),
      CanSend::msg.data);
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  CHECK_EQUAL(co_crc(0, val_buf, sizeof(sub_type64)),
              ldle_u16(CanSend::msg.data + 1u));
  CHECK_EQUAL(0, CanSend::msg.data[3]);
  CHECK_EQUAL(0, CanSend::msg.data[4]);
  CHECK_EQUAL(0, CanSend::msg.data[5]);
  CHECK_EQUAL(0, CanSend::msg.data[6]);
  ResetCanSend();

  // end transmission
  can_msg msg_con_end = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_con_end.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_END_BLK;
  CHECK_EQUAL(1, can_net_recv(net, &msg_con_end, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to the SSDO service (co_ssdo_t), block upload transfer
///        is in progress, first sub-block was already sent
///
/// \When an SDO upload sub-block request was received with the same acknowledge
///       sequence value as in the previous request
///
/// \Then the requested SDO upload sub-block is resent
TEST(CoSsdoBlkUp, Sub_ResendBlock) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  const sub_type64 val = 0x5423456789abcdefuL;
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, val);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX, 1u);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const std::array<uint_least8_t, CO_SDO_MSG_SIZE> expected_first = {
      0x01u, 0xefu, 0xcdu, 0xabu, 0x89u, 0x67u, 0x45u, 0x23u};
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_first.data());
  ResetCanSend();

  // client's request to resend last block
  msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg.data[1] = 0u;  // ackseq
  msg.data[2] = 1u;  // blksize
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // check resent uploaded value
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_first.data());
  ResetCanSend();

  // client's request for next block
  msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg.data[1] = 1u;  // ackseq
  msg.data[2] = 1u;  // blksize
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // check final byte
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const std::array<uint_least8_t, CO_SDO_MSG_SIZE> expected_next = {
      0x01u | CO_SDO_SEQ_LAST, 0x54u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE,
                    expected_next.data());
  ResetCanSend();

  // client's confirmation response
  msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg.data[1] = 2u;                // ackseq
  msg.data[2] = CO_SDO_MAX_SEQNO;  // blksize
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, nullptr);
  CHECK_SDO_CAN_MSG_CMD(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(1u),
      CanSend::msg.data);
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  CHECK_EQUAL(co_crc(0, val_buf, sizeof(sub_type64)),
              ldle_u16(CanSend::msg.data + 1u));
  CHECK_EQUAL(0, CanSend::msg.data[3]);
  CHECK_EQUAL(0, CanSend::msg.data[4]);
  CHECK_EQUAL(0, CanSend::msg.data[5]);
  CHECK_EQUAL(0, CanSend::msg.data[6]);
  CHECK_EQUAL(0, CanSend::msg.data[7]);
  ResetCanSend();

  // end transmission
  msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_END_BLK;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CoSsdoBlkUp, Sub_BlksizeOne_MsgWithNoLastByte) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX, 1u);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(1u, CanSend::msg.data);  // ackseq
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x89u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x67u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x45u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x23u, CanSend::msg.data[7]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
  msg_con_res.data[1] = 1u;  // ackseq
  msg_con_res.data[2] = 1u;  // blksize
  CHECK_EQUAL(1, can_net_recv(net, &msg_con_res, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0x54u, CanSend::msg.data[1]);
  CHECK_EQUAL(0, CanSend::msg.data[2]);
  CHECK_EQUAL(0, CanSend::msg.data[3]);
  CHECK_EQUAL(0, CanSend::msg.data[4]);
  CHECK_EQUAL(0, CanSend::msg.data[5]);
  CHECK_EQUAL(0, CanSend::msg.data[6]);
  CHECK_EQUAL(0, CanSend::msg.data[7]);
  ResetCanSend();
}

TEST(CoSsdoBlkUp, Sub_IndError) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  co_obj_set_up_ind(obj2020->Get(), &StreamingUpInd::Func, nullptr);
  StartSSDO();

  StreamingUpInd::valid_calls = 1u;  // fail in sub-block recv, not initiate

  InitBlkUp2020Req(SUBIDX, 1u);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_DATA);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_StartButReqNotFirst) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  co_obj_set_up_ind(obj2020->Get(), up_ind_inc_req_offset, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_RequestIncremented) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  co_obj_set_up_ind(obj2020->Get(), up_ind_inc_req_offset, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(2u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[0].id);
  CHECK_EQUAL(0, CanSend::msg_buf[0].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
  CHECK_SDO_CAN_MSG_CMD(1u, CanSend::msg_buf[0].data);
  CHECK_EQUAL(0xefu, CanSend::msg_buf[0].data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg_buf[0].data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg_buf[0].data[3]);
  CHECK_EQUAL(0x89u, CanSend::msg_buf[0].data[4]);
  CHECK_EQUAL(0x67u, CanSend::msg_buf[0].data[5]);
  CHECK_EQUAL(0x45u, CanSend::msg_buf[0].data[6]);
  CHECK_EQUAL(0x23u, CanSend::msg_buf[0].data[7]);
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[1].id);
  CHECK_EQUAL(0, CanSend::msg_buf[1].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[1].len);
  CHECK_SDO_CAN_MSG_CMD(2u | CO_SDO_SEQ_LAST, CanSend::msg_buf[1].data);
  CHECK_EQUAL(0x54u, CanSend::msg_buf[1].data[1]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[2]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[3]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[4]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[5]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[6]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[7]);
}

TEST(CoSsdoBlkUp, Sub_ArrNominal) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{1u});
  const sub_type val = 0xabcdu;
  obj2020->InsertAndSetSub(0x01u, SUB_TYPE, sub_type{val});
  co_obj_set_code(co_dev_find_obj(dev, IDX), CO_OBJECT_ARRAY);
  StartSSDO();

  InitBlkUp2020Req(0x01u, 2u);
  CheckInitBlkUp2020ResData(0x01u, sizeof(sub_type));
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  uint_least8_t val_buf[sizeof(sub_type)] = {0};
  stle_u16(val_buf, val);
  CHECK_EQUAL(val_buf[0], CanSend::msg.data[1]);
  CHECK_EQUAL(val_buf[1], CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[7]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
  msg_con_res.data[1] = 1u;
  msg_con_res.data[2] = CO_SDO_MAX_SEQNO;
  CHECK_EQUAL(1, can_net_recv(net, &msg_con_res, 0));

  // upload end
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(2),
      CanSend::msg.data);
  CHECK_EQUAL(co_crc(0, val_buf, sizeof(sub_type)),
              ldle_u16(CanSend::msg.data + 1u));
  CHECK_EQUAL(0, CanSend::msg.data[3]);
  CHECK_EQUAL(0, CanSend::msg.data[4]);
  CHECK_EQUAL(0, CanSend::msg.data[5]);
  CHECK_EQUAL(0, CanSend::msg.data[6]);
  CHECK_EQUAL(0, CanSend::msg.data[7]);
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with an object dictionary
///        containing an array; the array has a custom upload indication
///        function set; the function claims that the array is empty
///
/// \When block upload of the array is performed
///
/// \Then the size 0 is encoded in server command specifier on block upload end
///       response byte
///       \Calls membuf_flush()
///       \Calls membuf_reserve()
///       \Calls membuf_size()
///       \Calls stle_u16()
///       \Calls can_net_send()
TEST(CoSsdoBlkUp, Sub_EmptyArray) {
  co_sub_up_ind_t* const up_ind = [](const co_sub_t* const sub,
                                     co_sdo_req* const req, co_unsigned32_t ac,
                                     void*) -> co_unsigned32_t {
    co_sub_on_up(sub, req, &ac);
    req->size = 0u;  // the array is empty

    return 0;
  };

  const co_unsigned8_t element_subindex = 0x01u;
  const uint_least32_t res_canid = DEFAULT_COBID_RES;
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED8, element_subindex);
  obj2020->InsertAndSetSub(element_subindex, SUB_TYPE,
                           sub_type{0});  // the sub-object must exist
  co_obj_t* const obj = co_dev_find_obj(dev, IDX);
  co_obj_set_code(obj, CO_OBJECT_ARRAY);
  co_obj_set_up_ind(obj, up_ind, nullptr);
  StartSSDO();

  InitBlkUp2020Req(element_subindex);
  CheckInitBlkUp2020ResData(element_subindex, 0u);
  CHECK_EQUAL(res_canid, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(res_canid, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0x00u, CanSend::msg.data[1]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[7]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
  msg_con_res.data[1] = 1u;
  msg_con_res.data[2] = CO_SDO_MAX_SEQNO;
  CHECK_EQUAL(1, can_net_recv(net, &msg_con_res, 0));

  // upload end
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(res_canid, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(0),
      CanSend::msg.data);
  CHECK_EQUAL(0, CanSend::msg.data[1]);
  CHECK_EQUAL(0, CanSend::msg.data[2]);
  CHECK_EQUAL(0, CanSend::msg.data[3]);
  CHECK_EQUAL(0, CanSend::msg.data[4]);
  CHECK_EQUAL(0, CanSend::msg.data[5]);
  CHECK_EQUAL(0, CanSend::msg.data[6]);
  CHECK_EQUAL(0, CanSend::msg.data[7]);
}

TEST(CoSsdoBlkUp, Sub_ByteNotLast) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});
  co_obj_set_code(co_dev_find_obj(dev, IDX), CO_OBJECT_ARRAY);
  co_obj_set_up_ind(obj2020->Get(), up_ind_big_reqsiz, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX, 3u);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_SC_BLK_RES, IDX, SUBIDX,
      3u);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  ResetCanSend();

  can_msg msg_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  CHECK_EQUAL(1, can_net_recv(net, &msg_res, 0));
  CHECK_EQUAL(32u, CanSend::GetNumCalled());
  for (size_t i = 0; i < 32u; i++) {
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[i].id);
    CHECK_EQUAL(0, CanSend::msg_buf[i].flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[i].len);
    CHECK_SDO_CAN_MSG_CMD(i + 1u, CanSend::msg_buf[i].data);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[1]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[2]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[3]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[4]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[5]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[6]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[7]);
    CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[i].id);
    CHECK_EQUAL(0, CanSend::msg_buf[i].flags);
  }
}

TEST(CoSsdoBlkUp, Sub_ArrInvalidMaxSubidx) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});
  obj2020->InsertAndSetSub(0x01u, SUB_TYPE, sub_type{0xffffu});
  obj2020->InsertAndSetSub(0x02u, SUB_TYPE, sub_type{0xffffu});
  co_obj_set_code(co_dev_find_obj(dev, IDX), CO_OBJECT_ARRAY);
  StartSSDO();

  InitBlkUp2020Req(0x02u, 4u);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, 0x02u, CO_SDO_AC_NO_DATA);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_TimeoutTriggered) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  // upload end
  // server's request
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, InitIniRes_CoSdoCobidFrame) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  const co_unsigned32_t cobid_res = DEFAULT_COBID_RES | CO_SDO_COBID_FRAME;
  SetSrv02CobidRes(cobid_res);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(1u, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());

  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg_buf[0].id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg_buf[0].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
}

TEST(CoSsdoBlkUp, Sub_InvalidCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x0123456789abcdefuL});
  SetSrv02CobidRes(DEFAULT_COBID_RES);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());

  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_NoCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.len = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with object 0x2020 in
///        the object dictionary, block upload of this entry is ongoing
///
/// \When a too short SDO abort message is received
///
/// \Then an SDO response is not sent, upload indication function is called
///       once with a correct abort code
TEST(CoSsdoBlkUp, Sub_CSAbort_NoAC) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  co_sub_set_up_ind(obj2020->GetLastSub(), CoSubUpInd::Func, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();
  CoSubUpInd::Clear();

  can_msg msg = SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, 0u);
  msg.len = 7u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(1u, CoSubUpInd::num_called);
  POINTERS_EQUAL(co_dev_find_sub(dev, IDX, SUBIDX), CoSubUpInd::sub);
  CHECK(CoSubUpInd::req != nullptr);
  CHECK_EQUAL(CO_SDO_AC_ERROR, CoSubUpInd::ac);
  POINTERS_EQUAL(nullptr, CoSubUpInd::data);
}

/// \Given a pointer to the SSDO service (co_ssdo_t) with object 0x2020 in
///        the object dictionary, block upload of this entry is ongoing
///
/// \When an SDO abort message with an abort code set is received
///
/// \Then an SDO response is not sent, upload indication function is called
///       once with a correct abort code
TEST(CoSsdoBlkUp, Sub_CSAbort_AC) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  co_sub_set_up_ind(obj2020->GetLastSub(), CoSubUpInd::Func, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();
  CoSubUpInd::Clear();

  const can_msg msg =
      SdoCreateMsg::Abort(IDX, SUBIDX, DEFAULT_COBID_REQ, CO_SDO_AC_ERROR);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(1u, CoSubUpInd::num_called);
  POINTERS_EQUAL(co_dev_find_sub(dev, IDX, SUBIDX), CoSubUpInd::sub);
  CHECK(CoSubUpInd::req != nullptr);
  CHECK_EQUAL(CO_SDO_AC_ERROR, CoSubUpInd::ac);
  POINTERS_EQUAL(nullptr, CoSubUpInd::data);
}

TEST(CoSsdoBlkUp, Sub_InvalidSC) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = 0xffu;
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_SDO_CAN_MSG_IDX(IDX, CanSend::msg.data);
  CHECK_SDO_CAN_MSG_SUBIDX(SUBIDX, CanSend::msg.data);
  CHECK_SDO_CAN_MSG_AC(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_EmptyRequest) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_NoBlkSeqNum) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x0123456789abcdefuL});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(2u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_BLK_RES, CanSend::msg.data);
  CHECK_EQUAL(0x01u, CanSend::msg.data[1]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg_last = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_last.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg_last.len = 2u;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_SEQ_LAST, IDX, SUBIDX, CO_SDO_AC_BLK_SEQ);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_TooManySegments) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
  msg_con_res.data[1] = 1u;
  msg_con_res.data[2] = CO_SDO_MAX_SEQNO + 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg_con_res, 0));

  // upload end
  // server's request
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_BLK_SIZE);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_NoSegments) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg_con_res.data[1] = 1u;
  msg_con_res.data[2] = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg_con_res, 0));

  // upload end
  // server's request
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected = SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX,
                                                 CO_SDO_AC_BLK_SIZE);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, Sub_StartUp_ButAlreadyStarted) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type{0xabcdu});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CHECK_EQUAL(DEFAULT_COBID_RES, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_SDO_CAN_MSG_CMD(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_last = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg_last.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(1, can_net_recv(net, &msg_last, 0));

  // server's request
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, End_TimeoutTriggered) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_TIMEOUT);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, EndOnRecv_TooShortMsg) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.len = 0;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, EndOnRecv_InvalidCS) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = 0xffu;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, EndOnRecv_InvalidSC) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | 0x03u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const auto expected =
      SdoInitExpectedData::U32(CO_SDO_CS_ABORT, IDX, SUBIDX, CO_SDO_AC_NO_CS);
  CanSend::CheckMsg(DEFAULT_COBID_RES, 0, CO_SDO_MSG_SIZE, expected.data());
}

TEST(CoSsdoBlkUp, EndOnRecv_CSAbort) {
  dev_holder->CreateAndInsertObj(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64,
                           sub_type64{0x5423456789abcdefuL});
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = SdoCreateMsg::Default(IDX, SUBIDX, DEFAULT_COBID_REQ);
  msg.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

///@}
