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

#include <memory>

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>
#include "lely/co/crc.h"
#include <lely/co/csdo.h>
#include <lely/co/sdo.h>
#include <lely/co/ssdo.h>
#include <lely/util/endian.h>

#include "allocators/default.hpp"
#include "allocators/limited.hpp"

#include "holder/dev.hpp"
#include "holder/obj.hpp"

#include "lely-unit-test.hpp"
#include "lely-cpputest-ext.hpp"
#include "override/lelyco-val.hpp"

// values described in CiA 301, section 7.2.4.3 SDO protocols
#define CO_SDO_CCS_DN_SEG_REQ 0x00u
#define CO_SDO_CCS_DN_INI_REQ 0x20u
#define CO_SDO_CCS_UP_INI_REQ 0x40u
#define CO_SDO_CCS_UP_SEG_REQ 0x60u
#define CO_SDO_CCS_BLK_UP_REQ 0xa0u
#define CO_SDO_CCS_BLK_DN_REQ 0xc0u

#define CO_SDO_CS_ABORT 0x80u

#define CO_SDO_SCS_DN_SEG_RES 0x20u
#define CO_SDO_SCS_UP_INI_RES 0x40u
#define CO_SDO_SCS_DN_INI_RES 0x60u
#define CO_SDO_SCS_BLK_DN_RES 0xa0u
#define CO_SDO_SCS_BLK_UP_RES 0xc0u

#define CO_SDO_SC_END_BLK 0x01
#define CO_SDO_SC_BLK_RES 0x02
#define CO_SDO_SC_START_UP 0x03

#define CO_SDO_SEG_LAST 0x01u
#define CO_SDO_SEG_TOGGLE 0x10u
#define CO_SDO_SEG_SIZE_SET(n) (((7 - (n)) << 1) & CO_SDO_SEG_SIZE_MASK)
#define CO_SDO_SEQ_LAST 0x80u
#define CO_SDO_SEG_SIZE_MASK 0x0eu
#define CO_SDO_SC_MASK 0x03u

#define CO_SDO_BLK_CRC 0x04u
#define CO_SDO_BLK_SIZE_IND 0x02u
#define CO_SDO_BLK_SIZE_MASK 0x1cu
#define CO_SDO_BLK_SIZE_SET(n) (((7 - (n)) << 2) & CO_SDO_BLK_SIZE_MASK)

#define CO_SDO_INI_SIZE_IND 0x01u
#define CO_SDO_INI_SIZE_EXP 0x02u
#define CO_SDO_INI_SIZE_EXP_SET(n) ((((4 - n) << 2) | 0x03u) & 0x0fu)
#define CO_SDO_INI_SIZE_MASK 0x0fu

#define CO_SDO_MAX_SEQNO 127u

#define CHECK_CMD_CAN_MSG(res, msg) CHECK_EQUAL((res), (msg)[0])
#define CHECK_IDX_CAN_MSG(idx, msg) CHECK_EQUAL((idx), ldle_u16((msg) + 1u))
#define CHECK_SUBIDX_CAN_MSG(subidx, msg) CHECK_EQUAL((subidx), (msg)[3u])
#define CHECK_AC_CAN_MSG(ac, msg) CHECK_EQUAL((ac), ldle_u32((msg) + 4u))
#define CHECK_VAL_CAN_MSG(val, msg) CHECK_EQUAL((val), ldle_u32((msg) + 4u))

#define CO_SDO_MSG_SIZE 8u
#define CO_SDO_INI_DATA_SIZE 4u

TEST_GROUP(CO_SsdoInit) {
  can_net_t* net = nullptr;
  can_net_t* failing_net = nullptr;
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  Allocators::Default defaultAllocator;
  Allocators::Limited limitedAllocator;
  const co_unsigned8_t SSDO_NUM = 0x01u;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(defaultAllocator.ToAllocT());
    CHECK(net != nullptr);

    limitedAllocator.LimitAllocationTo(can_net_sizeof());
    failing_net = can_net_create(limitedAllocator.ToAllocT());
    CHECK(failing_net != nullptr);

    dev_holder.reset(new CoDevTHolder(0x01u));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() {
    dev_holder.reset();
    can_net_destroy(net);
    can_net_destroy(failing_net);
  }
};

TEST(CO_SsdoInit, CoSsdoAlignof_Nominal) {
  const auto ret = co_ssdo_alignof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(4u, ret);
#else
  CHECK_EQUAL(8u, ret);
#endif
}

TEST(CO_SsdoInit, CoSsdoSizeof_Nominal) {
  const auto ret = co_ssdo_sizeof();

#ifdef LELY_NO_MALLOC
  CHECK_EQUAL(1088u, ret);
#else
#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(108u, ret);
#else
  CHECK_EQUAL(184u, ret);
#endif
#endif  // LELY_NO_MALLOC
}

TEST(CO_SsdoInit, CoSsdoCreate_FailSsdoAlloc) {
  co_ssdo_t* const ssdo = co_ssdo_create(failing_net, dev, SSDO_NUM);

  POINTERS_EQUAL(nullptr, ssdo);
}

TEST(CO_SsdoInit, CoSsdoCreate_NumZero) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, 0x00u);

  POINTERS_EQUAL(nullptr, ssdo);
}

TEST(CO_SsdoInit, CoSsdoCreate_NumTooHigh) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, CO_NUM_SDOS + 1u);

  POINTERS_EQUAL(nullptr, ssdo);
}

TEST(CO_SsdoInit, CoSsdoCreate_NoServerParameterObj) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, 2u);

  POINTERS_EQUAL(nullptr, ssdo);
}

TEST(CO_SsdoInit, CoSsdoCreate_RecvCreateFail) {
  limitedAllocator.LimitAllocationTo(co_ssdo_sizeof());

  co_ssdo_t* const ssdo = co_ssdo_create(failing_net, dev, SSDO_NUM);

  POINTERS_EQUAL(nullptr, ssdo);
}

TEST(CO_SsdoInit, CoSsdoCreate_TimerCreateFail) {
  limitedAllocator.LimitAllocationTo(co_ssdo_sizeof() + can_recv_sizeof());

  co_ssdo_t* const ssdo = co_ssdo_create(failing_net, dev, SSDO_NUM);

  POINTERS_EQUAL(nullptr, ssdo);
}

TEST(CO_SsdoInit, CoSsdoCreate_DefaultSsdo_NoObj1200) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);

  CHECK(ssdo != nullptr);
  POINTERS_EQUAL(net, co_ssdo_get_net(ssdo));
  POINTERS_EQUAL(dev, co_ssdo_get_dev(ssdo));
  CHECK_EQUAL(SSDO_NUM, co_ssdo_get_num(ssdo));
  const co_sdo_par* par = co_ssdo_get_par(ssdo);
  CHECK_EQUAL(3u, par->n);
  CHECK_EQUAL(SSDO_NUM, par->id);
  CHECK_EQUAL(0x600u + SSDO_NUM, par->cobid_req);
  CHECK_EQUAL(0x580u + SSDO_NUM, par->cobid_res);
  CHECK_EQUAL(1, co_ssdo_is_stopped(ssdo));
  POINTERS_EQUAL(can_net_get_alloc(net), co_ssdo_get_alloc(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoCreate_DefaultSsdo_WithObj1200) {
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u));
  co_dev_insert_obj(dev, obj1200->Take());

  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);

  CHECK(ssdo != nullptr);
  POINTERS_EQUAL(net, co_ssdo_get_net(ssdo));
  POINTERS_EQUAL(dev, co_ssdo_get_dev(ssdo));
  CHECK_EQUAL(SSDO_NUM, co_ssdo_get_num(ssdo));
  const co_sdo_par* par = co_ssdo_get_par(ssdo);
  CHECK_EQUAL(3u, par->n);
  CHECK_EQUAL(SSDO_NUM, par->id);
  CHECK_EQUAL(0x600u + SSDO_NUM, par->cobid_req);
  CHECK_EQUAL(0x580u + SSDO_NUM, par->cobid_res);
  CHECK_EQUAL(1, co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoDestroy_Nullptr) {
  co_ssdo_t* const ssdo = nullptr;

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoDestroy_Nominal) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoStart_DefaultSsdo_NoObj1200) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoStart_AlreadyStarted) {
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u));
  co_dev_insert_obj(dev, obj1200->Take());
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);
  CHECK_EQUAL(0, co_ssdo_start(ssdo));

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoStart_DefaultSsdo_WithObj1200) {
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u));
  co_dev_insert_obj(dev, obj1200->Take());
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoStop_NoObj1200) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);

  co_ssdo_stop(ssdo);
  CHECK_EQUAL(1, co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoStop_WithObj1200) {
  std::unique_ptr<CoObjTHolder> obj1200(new CoObjTHolder(0x1200u));
  co_dev_insert_obj(dev, obj1200->Take());
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);

  co_ssdo_stop(ssdo);
  CHECK_EQUAL(1, co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST(CO_SsdoInit, CoSsdoStop_AfterStart) {
  co_ssdo_t* const ssdo = co_ssdo_create(net, dev, SSDO_NUM);
  co_ssdo_start(ssdo);

  co_ssdo_stop(ssdo);
  CHECK_EQUAL(1, co_ssdo_is_stopped(ssdo));

  co_ssdo_destroy(ssdo);
}

TEST_BASE(CO_Ssdo) {
  static const co_unsigned16_t SUB_TYPE = CO_DEFTYPE_UNSIGNED16;
  static const co_unsigned16_t SUB_TYPE64 = CO_DEFTYPE_UNSIGNED64;
  using sub_type = co_unsigned16_t;
  using sub_type8 = co_unsigned8_t;
  using sub_type64 = co_unsigned64_t;
  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned32_t CAN_ID = DEV_ID;
  static const co_unsigned32_t CAN_ID_EXT =
      co_unsigned32_t(DEV_ID) | 0x10000000u;
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;
  static const size_t MSG_BUF_SIZE = 32u;
  const co_unsigned8_t SSDO_NUM = 0x01u;
  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  Allocators::Default defaultAllocator;

  std::unique_ptr<CoObjTHolder> obj1200;
  std::unique_ptr<CoObjTHolder> obj2020;
  co_ssdo_t* ssdo = nullptr;

  can_msg msg_buf[MSG_BUF_SIZE];

  static co_unsigned32_t sub_dn_failing_ind(co_sub_t*, co_sdo_req*, void*) {
    return CO_SDO_AC_NO_READ;
  }

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  can_msg CreateDefaultCanMsg(const co_unsigned16_t idx = IDX,
                              const co_unsigned8_t subidx = SUBIDX) {
    can_msg msg = CAN_MSG_INIT;
    msg.id = 0x600u + SSDO_NUM;
    stle_u16(msg.data + 1u, idx);
    msg.data[3] = subidx;
    msg.len = CO_SDO_MSG_SIZE;

    return msg;
  }

  void StartSSDO() { CHECK_EQUAL(0, co_ssdo_start(ssdo)); }

  // obj 0x1200, sub 0x00 - highest sub-index supported
  void SetSrv00HighestSubidxSupported(co_unsigned8_t subidx) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x00u);
    if (sub != nullptr)
      co_sub_set_val_u8(sub, subidx);
    else
      obj1200->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, subidx);
  }

  // obj 0x1200, sub 0x01 - COB-ID client -> server (rx)
  void SetSrv01CobidReq(co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x01u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1200->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1200, sub 0x02 - COB-ID server -> client (tx)
  void SetSrv02CobidRes(co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x02u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1200->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  co_unsigned32_t GetSrv01CobidReq() {
    return co_dev_get_val_u32(dev, 0x1200u, 0x01u);
  }

  co_unsigned32_t GetSrv02CobidRes() {
    return co_dev_get_val_u32(dev, 0x1200u, 0x02u);
  }

  void ResetCanSend() {
    CanSend::Clear();
    for (unsigned int i = 0; i < MSG_BUF_SIZE; i++) msg_buf[i] = CAN_MSG_INIT;
    CanSend::SetMsgBuf(msg_buf, MSG_BUF_SIZE);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(defaultAllocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    obj1200.reset(new CoObjTHolder(0x1200u));
    co_dev_insert_obj(dev, obj1200->Take());

    SetSrv00HighestSubidxSupported(0x02u);
    SetSrv01CobidReq(0x600u + DEV_ID);
    SetSrv02CobidRes(0x580u + DEV_ID);
    ssdo = co_ssdo_create(net, dev, SSDO_NUM);
    CHECK(ssdo != nullptr);

    can_net_set_send_func(net, CanSend::func, nullptr);
    CanSend::SetMsgBuf(msg_buf, MSG_BUF_SIZE);
    CanSend::Clear();
  }

  TEST_TEARDOWN() {
    CanSend::Clear();

    co_ssdo_destroy(ssdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CoSsdoSetGet, CO_Ssdo){};

TEST(CoSsdoSetGet, CoSsdoGetNet_Nominal) {
  const auto* ret = co_ssdo_get_net(ssdo);

  POINTERS_EQUAL(net, ret);
}

TEST(CoSsdoSetGet, CoSsdoGetDev_Nominal) {
  const auto* ret = co_ssdo_get_dev(ssdo);

  POINTERS_EQUAL(dev, ret);
}

TEST(CoSsdoSetGet, CoSsdoGetNum_Nominal) {
  const auto ret = co_ssdo_get_num(ssdo);

  CHECK_EQUAL(SSDO_NUM, ret);
}

TEST(CoSsdoSetGet, CoSsdoGetPar_Nominal) {
  const auto* ret = co_ssdo_get_par(ssdo);

  CHECK(ret != nullptr);
  CHECK_EQUAL(3u, ret->n);
  CHECK_EQUAL(SSDO_NUM, ret->id);
  CHECK_EQUAL(0x580u + SSDO_NUM, ret->cobid_res);
  CHECK_EQUAL(0x600u + SSDO_NUM, ret->cobid_req);
}

TEST(CoSsdoSetGet, CoSsdoGetTimeout_Nominal) {
  const auto ret = co_ssdo_get_timeout(ssdo);

  CHECK_EQUAL(0u, ret);
}

TEST(CoSsdoSetGet, CoSsdoSetTimeout_Nominal) {
  co_ssdo_set_timeout(ssdo, 1);

  CHECK_EQUAL(1, co_ssdo_get_timeout(ssdo));
}

TEST(CoSsdoSetGet, CoSsdoSetTimeout_InvalidTimeout) {
  co_ssdo_set_timeout(ssdo, -1);

  CHECK_EQUAL(0, co_ssdo_get_timeout(ssdo));
}

TEST(CoSsdoSetGet, CoSsdoSetTimeout_DisableTimeout) {
  co_ssdo_set_timeout(ssdo, 1);

  co_ssdo_set_timeout(ssdo, 0);

  CHECK_EQUAL(0, co_ssdo_get_timeout(ssdo));
}

TEST(CoSsdoSetGet, CoSsdoSetTimeout_UpdateTimeout) {
  co_ssdo_set_timeout(ssdo, 1);

  co_ssdo_set_timeout(ssdo, 4);

  CHECK_EQUAL(4, co_ssdo_get_timeout(ssdo));
}

TEST_GROUP_BASE(CoSsdoUpdate, CO_Ssdo){};

TEST(CoSsdoUpdate, ReqCobidValid_ResCobidInvalid) {
  const co_unsigned32_t new_cobid_res = CAN_ID | CO_SDO_COBID_VALID;
  SetSrv02CobidRes(new_cobid_res);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x600u + DEV_ID, GetSrv01CobidReq());
  CHECK_EQUAL(new_cobid_res, GetSrv02CobidRes());
}

TEST(CoSsdoUpdate, ReqCobidInvalid_ResCobidValid) {
  const co_unsigned32_t new_cobid_req = CAN_ID | CO_SDO_COBID_VALID;
  SetSrv01CobidReq(new_cobid_req);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(new_cobid_req, GetSrv01CobidReq());
  CHECK_EQUAL(0x580u + DEV_ID, GetSrv02CobidRes());
}

TEST(CoSsdoUpdate, ReqResCobidsInvalid) {
  const co_unsigned32_t new_cobid_req = CAN_ID | CO_SDO_COBID_VALID;
  const co_unsigned32_t new_cobid_res = CAN_ID | CO_SDO_COBID_VALID;
  SetSrv01CobidReq(new_cobid_req);
  SetSrv02CobidRes(new_cobid_res);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(new_cobid_req, GetSrv01CobidReq());
  CHECK_EQUAL(new_cobid_res, GetSrv02CobidRes());
}

TEST(CoSsdoUpdate, ReqResCobidsValid_CobidFrameSet) {
  const co_unsigned32_t new_cobid_req = CAN_ID | CO_SDO_COBID_FRAME;
  const co_unsigned32_t new_cobid_res = CAN_ID;
  SetSrv01CobidReq(new_cobid_req);
  SetSrv02CobidRes(new_cobid_res);

  const auto ret = co_ssdo_start(ssdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(new_cobid_req, GetSrv01CobidReq());
  CHECK_EQUAL(new_cobid_res, GetSrv02CobidRes());
}

TEST_GROUP_BASE(CoSsdoTimer, CO_Ssdo){};

TEST(CoSsdoTimer, Timeout) {
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();
  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_DN_INI_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_INI_RES, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST_GROUP_BASE(CoSsdoWaitOnRecv, CO_Ssdo){};

TEST(CoSsdoWaitOnRecv, DnIniReq) {
  StartSSDO();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_DN_INI_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_INI_RES, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
}

TEST(CoSsdoWaitOnRecv, UpIniReq) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_INI_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
      CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_EXP_SET(sizeof(sub_type)),
      CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0xabcdu, CanSend::msg.data);
}

TEST(CoSsdoWaitOnRecv, BlkDnIniReq) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));
  StartSSDO();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(127u, CanSend::msg.data);
}

TEST(CoSsdoWaitOnRecv, BlkUpIniReq) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ;
  msg.data[4] = CO_SDO_MAX_SEQNO;
  msg.data[5] = 2u;  // protocol switch
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_EXP_SET(2),
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0xabcdu, CanSend::msg.data);
}

TEST(CoSsdoWaitOnRecv, Abort) {
  StartSSDO();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK(!CanSend::called());
}

TEST(CoSsdoWaitOnRecv, InvalidCS) {
  StartSSDO();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = 0xffu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoWaitOnRecv, NoCS) {
  StartSSDO();

  can_msg msg = CreateDefaultCanMsg();
  msg.len = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST_GROUP_BASE(CoSsdoDnIniOnRecv, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  // download initiate request
  can_msg CreateDnIniReqMsg(co_unsigned16_t idx, co_unsigned8_t subidx,
                            uint_least8_t buffer[CO_SDO_INI_DATA_SIZE],
                            uint_least8_t cs_flags = 0) {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] |= CO_SDO_CCS_DN_INI_REQ;
    msg.data[0] |= cs_flags;
    stle_u16(msg.data + 1u, idx);
    msg.data[3] = subidx;
    if (buffer != nullptr) memcpy(msg.data + 4u, buffer, CO_SDO_INI_DATA_SIZE);

    return msg;
  }
};

TEST(CoSsdoDnIniOnRecv, NoIdxSpecified) {
  StartSSDO();

  can_msg msg = CreateDnIniReqMsg(0xffffu, 0xffu, nullptr);
  msg.len = 1u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_OBJ, CanSend::msg.data);
}

TEST(CoSsdoDnIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = CreateDnIniReqMsg(IDX, 0xffu, nullptr);
  msg.len = 3u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_SUB, CanSend::msg.data);
}

TEST(CoSsdoDnIniOnRecv, TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  co_unsigned8_t size[4] = {0};
  stle_u32(size, sizeof(sub_type64));
  can_msg msg = CreateDnIniReqMsg(IDX, SUBIDX, size, CO_SDO_SEG_SIZE_SET(1u));
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_INI_RES, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoDnIniOnRecv, Expedited_NoObj) {
  StartSSDO();

  co_unsigned8_t val2dn[4] = {0};
  can_msg msg = CreateDnIniReqMsg(IDX, SUBIDX, val2dn, CO_SDO_INI_SIZE_EXP);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_OBJ, CanSend::msg.data);
}

TEST(CoSsdoDnIniOnRecv, Expedited) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));
  StartSSDO();

  co_unsigned8_t val2dn[4] = {0};
  stle_u16(val2dn, 0xabcd);
  const co_unsigned8_t cs = CO_SDO_INI_SIZE_IND | CO_SDO_INI_SIZE_EXP_SET(2u);
  can_msg msg = CreateDnIniReqMsg(IDX, SUBIDX, val2dn, cs);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_INI_RES, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);

  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(ldle_u16(val2dn), co_sub_get_val_u16(sub));
}

TEST_GROUP_BASE(CoSsdoUpIniOnRecv, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  static co_unsigned32_t up_ind_size_zero(const co_sub_t* sub, co_sdo_req* req,
                                          void* data) {
    (void)data;

    co_unsigned32_t ac = 0;
    co_sub_on_up(sub, req, &ac);
    req->size = 0;

    return ac;
  }

  // upload initiate request message
  can_msg CreateUpIniReqMsg(const co_unsigned16_t idx,
                            const co_unsigned8_t subidx) {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_UP_INI_REQ;
    stle_u16(msg.data + 1u, idx);
    msg.data[3] = subidx;
    msg.len = CO_SDO_MSG_SIZE;

    return msg;
  }
};

TEST(CoSsdoUpIniOnRecv, NoIdxSpecified) {
  StartSSDO();
  can_msg msg = CreateUpIniReqMsg(0xffffu, 0xffu);
  msg.len = 1u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_OBJ, CanSend::msg.data);
}

TEST(CoSsdoUpIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = CreateUpIniReqMsg(IDX, 0xffu);
  msg.len = 3u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_SUB, CanSend::msg.data);
}

TEST(CoSsdoUpIniOnRecv, NoAccess) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));
  co_sub_set_access(obj2020->GetLastSub(), CO_ACCESS_WO);
  StartSSDO();

  can_msg msg = CreateUpIniReqMsg(IDX, SUBIDX);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_READ, CanSend::msg.data);
}

TEST(CoSsdoUpIniOnRecv, UploadToSubWithSizeZero) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0x1234u));
  co_sub_set_up_ind(obj2020->GetLastSub(), up_ind_size_zero, nullptr);
  StartSSDO();

  can_msg msg = CreateUpIniReqMsg(IDX, SUBIDX);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND,
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
}

TEST(CoSsdoUpIniOnRecv, TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x0123456789abcdefu));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  can_msg msg = CreateUpIniReqMsg(IDX, SUBIDX);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND,
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(sizeof(sub_type64), CanSend::msg.data);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &tp));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoUpIniOnRecv, Expedited) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateUpIniReqMsg(IDX, SUBIDX);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_EXP_SET(2u),
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0xabcdu, CanSend::msg.data);
}

TEST_GROUP_BASE(CoSsdoBlkDnIniOnRecv, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  // block download initiate request
  can_msg CreateBlkDnIniReqMsg(
      const co_unsigned16_t idx = IDX, const co_unsigned8_t subidx = SUBIDX,
      const co_unsigned8_t cs_flags = 0, const size_t size = 0) {
    can_msg msg = CreateDefaultCanMsg(idx, subidx);
    co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | cs_flags;
    if (size > 0) {
      cs |= CO_SDO_BLK_SIZE_IND;
      stle_u32(msg.data + 4u, size);
    }
    msg.data[0] = cs;

    return msg;
  }
};

TEST(CoSsdoBlkDnIniOnRecv, NoIdxSpecified) {
  StartSSDO();

  can_msg msg = CreateBlkDnIniReqMsg(0xffffu, 0xffu);
  msg.len = 1u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_OBJ, CanSend::msg.data);
}

TEST(CoSsdoBlkDnIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = CreateBlkDnIniReqMsg(IDX, 0xffu);
  msg.len = 3u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_SUB, CanSend::msg.data);
}

TEST(CoSsdoBlkDnIniOnRecv, InvalidCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateBlkDnIniReqMsg(IDX, SUBIDX, 0x0fu);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkDnIniOnRecv, BlkSizeSpecified) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));
  StartSSDO();

  can_msg msg = CreateBlkDnIniReqMsg(IDX, SUBIDX, CO_SDO_BLK_SIZE_IND);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(127u, CanSend::msg.data);
}

TEST(CoSsdoBlkDnIniOnRecv, TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  can_msg msg = CreateBlkDnIniReqMsg();
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(127u, CanSend::msg.data);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &tp));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoBlkDnIniOnRecv, Nominal) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));
  StartSSDO();

  can_msg msg = CreateBlkDnIniReqMsg();
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(127u, CanSend::msg.data);
}

TEST_GROUP_BASE(CoSsdoBlkUpIniOnRecv, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  // block upload initiate request
  can_msg CreateBlkUp2020IniReqMsg(
      const co_unsigned8_t subidx = SUBIDX,
      const co_unsigned8_t blksize = CO_SDO_MAX_SEQNO) {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_BLK_UP_REQ;
    msg.data[3] = subidx;
    msg.data[4] = blksize;

    return msg;
  }
};

TEST(CoSsdoBlkUpIniOnRecv, InvalidSC) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, 1u);
  msg.data[0] |= 0x0fu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, NoIdxSpecified) {
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(0xffu);
  msg.len = 1u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_OBJ, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, NoSubidxSpecified) {
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(0xffu);
  msg.len = 3u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_SUB, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, BlocksizeNotSpecified) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  msg.len = 4u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_SIZE, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, BlocksizeMoreThanMaxSeqNum) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, CO_SDO_MAX_SEQNO + 1u);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_SIZE, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, BlocksizeZero) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, 0);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_SIZE, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, ProtocolSwitch) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  msg.len = 5u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_SIZE_IND | CO_SDO_BLK_CRC,
      CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(sizeof(sub_type), CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, NoObjPresent) {
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_OBJ, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, NoSubPresent) {
  CreateObjInDev(obj2020, IDX);
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_SUB, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, BlksizeMoreThanPst) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x0123456789abcdefu));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, sizeof(sub_type64));
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_SIZE_IND | CO_SDO_BLK_CRC,
      CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(sizeof(sub_type64), CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, sizeof(sub_type64));
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_SIZE_IND | CO_SDO_BLK_CRC,
      CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(sizeof(sub_type64), CanSend::msg.data);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, ReqSizeEqualToPst) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX);
  msg.data[5] = sizeof(sub_type64);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND,
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(sizeof(sub_type64), CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, ReqSizeEqualToPst_MoreFrames_TimeoutSet) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, 5u);
  msg.data[5] = sizeof(sub_type64);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_UP_INI_RES | CO_SDO_SC_END_BLK,
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(sizeof(sub_type64), CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, ReqSizeMoreThanPst) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg(SUBIDX, 5u);
  msg.data[5] = 2u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_BLK_SIZE_IND,
      CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(sizeof(sub_type64), CanSend::msg.data);
}

TEST(CoSsdoBlkUpIniOnRecv, Nominal) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  can_msg msg = CreateBlkUp2020IniReqMsg();
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
      CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_BLK_SIZE_IND,
      CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(2u, CanSend::msg.data);
}

TEST_GROUP_BASE(CoSsdoDnSegOnRecv, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  // download segment message
  can_msg CreateDnSegMsg(const uint_least8_t buf[], const size_t size,
                         const uint_least8_t cs_flags = 0) {
    assert(size <= 7u);

    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_DN_SEG_REQ | CO_SDO_SEG_SIZE_SET(size) | cs_flags;
    memcpy(msg.data + 1u, buf, size);

    return msg;
  }

  // send segmented download initiate request to SSDO (0x2020, 0x00)
  void DownloadInitiateReq(const size_t size) {
    co_unsigned8_t size_buf[4] = {0};
    stle_u32(size_buf, size);
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND;
    stle_u16(msg.data + 1u, IDX);
    msg.data[3] = SUBIDX;
    memcpy(msg.data + 4u, size_buf, CO_SDO_INI_DATA_SIZE);

    CHECK_EQUAL(0, can_net_recv(net, &msg));

    CHECK_EQUAL(1u, CanSend::num_called);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_INI_RES, CanSend::msg.data);
    CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
    ResetCanSend();
  }
};

TEST(CoSsdoDnSegOnRecv, NoCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  // receive empty segment
  can_msg msg = CreateDnSegMsg(nullptr, 0u);
  msg.len = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoDnSegOnRecv, AbortCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t val2dn[4u] = {0};
  can_msg msg = CreateDnSegMsg(val2dn, 4u, CO_SDO_CS_ABORT);
  const auto ret_abort = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret_abort);
  CHECK(!CanSend::called());
}

TEST(CoSsdoDnSegOnRecv, InvalidCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t val2dn[4u] = {0};
  can_msg msg = CreateDnSegMsg(val2dn, 4u, 0xffu);
  const auto ret_abort = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret_abort);
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoDnSegOnRecv, NoToggle) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t vals2dn[sizeof(sub_type64)] = {0};

  // send first segment: 4 bytes
  can_msg msg = CreateDnSegMsg(vals2dn, 4u);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_SEG_RES, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  // send last segment: 1 byte
  msg = CreateDnSegMsg(vals2dn + 4u, 1u, CO_SDO_SEG_LAST);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK(!CanSend::called());
}

TEST(CoSsdoDnSegOnRecv, MsgLenLessThanSegmentSize) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t val2dn[8] = {0};
  can_msg msg = CreateDnSegMsg(val2dn, 6u);
  msg.len = 5u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoDnSegOnRecv, SegmentTooBig) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type));

  co_unsigned8_t val2dn[4] = {0};
  can_msg msg = CreateDnSegMsg(val2dn, 4u);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TYPE_LEN_HI, CanSend::msg.data);
}

TEST(CoSsdoDnSegOnRecv, SegmentTooSmall) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t val2dn[sizeof(sub_type64) - 1u] = {0};
  can_msg msg =
      CreateDnSegMsg(val2dn, sizeof(sub_type64) - 1u, CO_SDO_SEG_LAST);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TYPE_LEN_LO, CanSend::msg.data);
}

TEST(CoSsdoDnSegOnRecv, FailDnInd) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  co_sub_set_dn_ind(sub, sub_dn_failing_ind, nullptr);
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t val2dn[4] = {0};
  can_msg msg1 = CreateDnSegMsg(val2dn, 4u);
  CHECK_EQUAL(0, can_net_recv(net, &msg1));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_READ, CanSend::msg.data);
}

TEST(CoSsdoDnSegOnRecv, TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  co_ssdo_set_timeout(ssdo, 1);
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t vals2dn[4] = {0x01u, 0x23u, 0x45u, 0x67u};

  // send first segment: 4 bytes
  can_msg msg1 = CreateDnSegMsg(vals2dn, 4u);
  CHECK_EQUAL(0, can_net_recv(net, &msg1));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_SEG_RES, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoDnSegOnRecv, Nominal) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  DownloadInitiateReq(sizeof(sub_type64));

  co_unsigned8_t vals2dn[8] = {0x01u, 0x23u, 0x45u, 0x67u,
                               0x89u, 0xabu, 0xcdu, 0xefu};

  // send first segment: 4 bytes
  can_msg msg1 = CreateDnSegMsg(vals2dn, 4u);
  CHECK_EQUAL(0, can_net_recv(net, &msg1));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_SEG_RES, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  // send last segment: next 4 bytes
  can_msg msg2 =
      CreateDnSegMsg(vals2dn + 4u, 4u, CO_SDO_SEG_LAST | CO_SDO_SEG_TOGGLE);
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_DN_SEG_RES | CO_SDO_SEG_TOGGLE,
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(0x0000u, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);

  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  const sub_type64 val_u64 = co_sub_get_val_u64(sub);
  CHECK_EQUAL(0xefcdab8967452301u, val_u64);
}

TEST_GROUP_BASE(CoSsdoUpSegOnRecv, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  static co_unsigned32_t up_ind_failing(const co_sub_t* sub, co_sdo_req* req,
                                        void* data) {
    (void)data;

    co_unsigned32_t ac = 0;
    co_sub_on_up(sub, req, &ac);
    req->size = 10u;

    static int called = 0;
    if (called == 1u) ac = CO_SDO_AC_ERROR;
    called++;

    return ac;
  }

  static co_unsigned32_t up_ind_size_longer(const co_sub_t* sub,
                                            co_sdo_req* req, void* data) {
    (void)data;

    co_unsigned32_t ac = 0;
    co_sub_on_up(sub, req, &ac);
    req->size = 10u;

    return ac;
  }

  // upload initiate request message
  can_msg CreateUpIniReqMsg(const co_unsigned16_t idx,
                            const co_unsigned8_t subidx) {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_UP_INI_REQ;
    stle_u16(msg.data + 1u, idx);
    msg.data[3] = subidx;

    return msg;
  }

  // send segmented upload initiate request to SSDO (0x2020, 0x00)
  void UploadInitiateReq(const co_unsigned8_t size,
                         const co_unsigned32_t can_id = 0x580u + DEV_ID,
                         const co_unsigned8_t flags = 0u) {
    can_msg msg = CreateUpIniReqMsg(IDX, SUBIDX);

    CHECK_EQUAL(0, can_net_recv(net, &msg));

    CHECK_EQUAL(1u, CanSend::num_called);
    CHECK_EQUAL(can_id, CanSend::msg.id);
    CHECK_EQUAL(flags, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_CMD_CAN_MSG(CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND,
                      CanSend::msg.data);
    CHECK_VAL_CAN_MSG(size, CanSend::msg.data);
    ResetCanSend();
  }
};

TEST(CoSsdoUpSegOnRecv, NoCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  msg.len = 0u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoUpSegOnRecv, CSAbort) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK(!CanSend::called());
}

TEST(CoSsdoUpSegOnRecv, InvalidCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = 0xffu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoUpSegOnRecv, NoToggle) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x9876543210abcdefu));
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg2 = CreateDefaultCanMsg();
  msg2.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TOGGLE, CanSend::msg.data);
}

TEST(CoSsdoUpSegOnRecv, TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x9876543210abcdefu));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(0x00u, CanSend::msg.data);
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

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoUpSegOnRecv, Nominal) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x9876543210abcdefu));
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64));

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg2 = CreateDefaultCanMsg();
  msg2.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
      CO_SDO_SEG_SIZE_SET(1) | CO_SDO_SEG_TOGGLE | CO_SDO_SEG_LAST,
      CanSend::msg.data);
  CHECK_EQUAL(0x98u, CanSend::msg.data[1]);
}

TEST(CoSsdoUpSegOnRecv, CoSsdoCreateSegRes_ExtendedId) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x9876543210abcdefu));
  const co_unsigned32_t new_can_id = 0x1ffff580u + DEV_ID;
  const co_unsigned32_t new_cobid_res = new_can_id | CO_SDO_COBID_FRAME;
  SetSrv02CobidRes(new_cobid_res);
  StartSSDO();

  UploadInitiateReq(sizeof(sub_type64), new_can_id, CAN_FLAG_IDE);

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(new_can_id, CanSend::msg.id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg.flags);
  CHECK_CMD_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg2 = CreateDefaultCanMsg();
  msg2.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(new_can_id, CanSend::msg.id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg.flags);
  CHECK_CMD_CAN_MSG(CO_SDO_SEG_TOGGLE | CO_SDO_SEG_LAST | 0x0c,
                    CanSend::msg.data);
  CHECK_EQUAL(0x98u, CanSend::msg.data[1]);
}

TEST(CoSsdoUpSegOnRecv, IndError) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x9876543210abcdefu));
  co_obj_set_up_ind(obj2020->Get(), up_ind_failing, nullptr);
  StartSSDO();

  UploadInitiateReq(10u);
  // UploadInitiateReq(8u);

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(0x00u, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg2 = CreateDefaultCanMsg();
  msg2.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_ERROR, CanSend::msg.data);
}

TEST(CoSsdoUpSegOnRecv, IndReqSizeLonger) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x9876543210abcdefu));
  co_obj_set_up_ind(obj2020->Get(), up_ind_size_longer, nullptr);
  StartSSDO();

  UploadInitiateReq(10u);

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_UP_SEG_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(0, CanSend::msg.data);
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x76u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg2 = CreateDefaultCanMsg();
  msg2.data[0] = CO_SDO_CCS_UP_SEG_REQ | CO_SDO_SEG_TOGGLE;
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEG_TOGGLE, CanSend::msg.data);
  CHECK_EQUAL(0x98u, CanSend::msg.data[1]);
  CHECK_EQUAL(0xefu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[3]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[4]);
  CHECK_EQUAL(0x10u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x32u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x54u, CanSend::msg.data[7]);
}

TEST_GROUP_BASE(CoSsdoBlkDn, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  /**
   * create block download subobject request
   *
   * @param seqno sequence number
   * @param last  is this the last segment?
   */
  can_msg CreateBlkDnSubReq(const co_unsigned8_t seqno,
                            const co_unsigned8_t last = 0) {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = last | seqno;

    return msg;
  }

  /**
   * create block download end message
   *
   * @param crc cyclic redundancy check (checksum) value
   */
  can_msg CreateBlkDnEndCanMsg(const co_unsigned16_t crc,
                               const co_unsigned8_t cs_flags = 0) {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK | cs_flags;
    stle_u16(msg.data + 1u, crc);

    return msg;
  }

  /**
   * Initiate block download subobject
   *
   * @param size     size of data to download
   * @param cs_flags additional flags for command specifier
   */
  void InitBlkDn2020Sub00(const co_unsigned32_t size,
                          const co_unsigned8_t cs_flags = CO_SDO_BLK_CRC) {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_SIZE_IND | cs_flags;
    stle_u32(msg.data + 4, size);

    CHECK_EQUAL(0, can_net_recv(net, &msg));

    CHECK_EQUAL(1u, CanSend::num_called);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC,
                      CanSend::msg.data);
    CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
    CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
    CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[4]);
    ResetCanSend();
  }

  /**
   * End block download
   *
   * @param crc cyclic redundancy check (checksum) value
   * @param size how many bytes were sent in a last segment
   */
  void EndBlkDn(const co_unsigned16_t crc, const size_t size = 0) {
    can_msg msg_end = CAN_MSG_INIT;
    if (size != 0)
      msg_end = CreateBlkDnEndCanMsg(crc, CO_SDO_BLK_SIZE_SET(size));
    else
      msg_end = CreateBlkDnEndCanMsg(crc);

    CHECK_EQUAL(0, can_net_recv(net, &msg_end));

    CHECK_EQUAL(1u, CanSend::num_called);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_END_BLK,
                      CanSend::msg.data);
    CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
    ResetCanSend();
  }

  /**
   * Change state of the SSDO service to blk_dn_sub_end
   */
  void ChangeStateToEnd() {
    can_msg msg_first_blk = CreateBlkDnSubReq(1u);
    msg_first_blk.data[1] = 0x01u;
    msg_first_blk.data[2] = 0x23u;
    msg_first_blk.data[3] = 0x45u;
    msg_first_blk.data[4] = 0x67u;
    msg_first_blk.data[5] = 0x89u;
    msg_first_blk.data[6] = 0xabu;
    msg_first_blk.data[7] = 0xcdu;
    CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

    CHECK(!CanSend::called());

    can_msg msg_last_blk = CreateBlkDnSubReq(2u, CO_SDO_SEQ_LAST);
    msg_last_blk.data[1] = 0xefu;
    CHECK_EQUAL(0, can_net_recv(net, &msg_last_blk));

    CHECK_EQUAL(1u, CanSend::num_called);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                      CanSend::msg.data);
    CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
    CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
    CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
    ResetCanSend();
  }
};

TEST(CoSsdoBlkDn, Sub_NoCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg = CreateBlkDnSubReq(1u);
  msg.len = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, Sub_CSAbort) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  msg_first_blk.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK(!CanSend::called());
}

TEST(CoSsdoBlkDn, Sub_SeqnoZero) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  msg_first_blk.data[0] = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_SEQ, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, Sub_NoCrc) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64), 0);

  can_msg msg = CreateBlkDnSubReq(1u);
  msg.data[1] = 0x01u;
  msg.data[2] = 0x23u;
  msg.data[3] = 0x45u;
  msg.data[4] = 0x67u;
  msg.data[5] = 0x89u;
  msg.data[6] = 0xabu;
  msg.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK(!CanSend::called());

  can_msg msg_last_blk = CreateBlkDnSubReq(2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_last_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  ResetCanSend();

  EndBlkDn(0, 1u);  // no CRC in this transfer

  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(0xefcdab8967452301u, co_sub_get_val_u64(sub));
}

TEST(CoSsdoBlkDn, Sub_NoSub) {
  CreateObjInDev(obj2020, IDX);
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg = CreateBlkDnSubReq(1u);
  msg.data[1] = 0x01u;
  msg.data[2] = 0x23u;
  msg.data[3] = 0x45u;
  msg.data[4] = 0x67u;
  msg.data[5] = 0x89u;
  msg.data[6] = 0xabu;
  msg.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_SUB, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, Sub_RequestLessThanSize) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(6u);

  can_msg msg = CreateBlkDnSubReq(1u);
  msg.data[1] = 0x01u;
  msg.data[2] = 0x23u;
  msg.data[3] = 0x45u;
  msg.data[4] = 0x67u;
  msg.data[5] = 0x89u;
  msg.data[6] = 0xabu;
  msg.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TYPE_LEN_HI, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, Sub_Nominal) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  sub_type64 val = 0xefcdab9078563412u;
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  memcpy(msg_first_blk.data + 1u, val_buf, 7u);
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK(!CanSend::called());

  can_msg msg_last_blk = CreateBlkDnSubReq(2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = val_buf[7];
  CHECK_EQUAL(0, can_net_recv(net, &msg_last_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(2, CanSend::msg.data[1]);    // ackseq
  CHECK_EQUAL(127, CanSend::msg.data[2]);  // blksize
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  EndBlkDn(co_crc(0, val_buf, sizeof(sub_type64)), 1u);

  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(val, co_sub_get_val_u64(sub));
}

TEST(CoSsdoBlkDn, Sub_InvalidSeqno_LastInBlk) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = CreateBlkDnSubReq(CO_SDO_MAX_SEQNO);
  msg_first_blk.data[1] = 0x12u;
  msg_first_blk.data[2] = 0x34u;
  msg_first_blk.data[3] = 0x56u;
  msg_first_blk.data[4] = 0x78u;
  msg_first_blk.data[5] = 0x90u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(0, CanSend::msg.data[1]);
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, Sub_CrcError) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  msg_first_blk.data[1] = 0x01u;
  msg_first_blk.data[2] = 0x23u;
  msg_first_blk.data[3] = 0x45u;
  msg_first_blk.data[4] = 0x67u;
  msg_first_blk.data[5] = 0x89u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK(!CanSend::called());

  can_msg msg_last_blk = CreateBlkDnSubReq(2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_last_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  can_msg msg_end = CreateBlkDnEndCanMsg(0);
  msg_end.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(0, can_net_recv(net, &msg_end));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_CRC, CanSend::msg.data);

  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  CHECK_EQUAL(0, co_sub_get_val_u64(sub));
}

TEST(CoSsdoBlkDn, Sub_TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  msg_first_blk.data[1] = 0x01u;
  msg_first_blk.data[2] = 0x23u;
  msg_first_blk.data[3] = 0x45u;
  msg_first_blk.data[4] = 0x67u;
  msg_first_blk.data[5] = 0x89u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK(!CanSend::called());

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndAbort) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  msg_first_blk.data[1] = 0x12u;
  msg_first_blk.data[2] = 0x34u;
  msg_first_blk.data[3] = 0x56u;
  msg_first_blk.data[4] = 0x78u;
  msg_first_blk.data[5] = 0x90u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK(!CanSend::called());

  can_msg msg_last_blk = CreateBlkDnSubReq(1, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;

  CHECK_EQUAL(0, can_net_recv(net, &msg_last_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(1u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize

  co_ssdo_destroy(ssdo);
  ssdo = nullptr;
}

TEST(CoSsdoBlkDn, End_TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndRecv_NoCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = CreateDefaultCanMsg();
  msg.len = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndRecv_CSAbort) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK(!CanSend::called());
}

TEST(CoSsdoBlkDn, EndRecv_InvalidCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = 0xffu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndRecv_InvalidSC) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndRecv_InvalidLen) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  msg_first_blk.data[1] = 0x01u;
  msg_first_blk.data[2] = 0x23u;
  msg_first_blk.data[3] = 0x45u;
  msg_first_blk.data[4] = 0x67u;
  msg_first_blk.data[5] = 0x89u;
  msg_first_blk.data[6] = 0xabu;
  msg_first_blk.data[7] = 0xcdu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK(!CanSend::called());

  can_msg msg_last_blk = CreateBlkDnSubReq(1u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = 0xefu;
  CHECK_EQUAL(0, can_net_recv(net, &msg_last_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(1u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TYPE_LEN_LO, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndRecv_InvalidSize) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));
  ChangeStateToEnd();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  msg.data[0] |= CO_SDO_BLK_SIZE_SET(sizeof(sub_type64) - 2u);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndRecv_ReqZero) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(0);

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  msg.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(0, CanSend::msg.data[1]);
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);
  ResetCanSend();

  // end, req zero
  msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK;
  msg.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkDn, EndRecv_FailingDnInd) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0));
  StartSSDO();

  InitBlkDn2020Sub00(sizeof(sub_type64));

  sub_type64 val = 0xffffffffffffffffu;
  uint_least8_t val_buf[sizeof(sub_type64)] = {0};
  stle_u64(val_buf, val);
  can_msg msg_first_blk = CreateBlkDnSubReq(1u);
  memcpy(msg_first_blk.data + 1u, val_buf, 7u);
  CHECK_EQUAL(0, can_net_recv(net, &msg_first_blk));

  CHECK(!CanSend::called());

  can_msg msg_last_blk = CreateBlkDnSubReq(2u, CO_SDO_SEQ_LAST);
  msg_last_blk.data[1] = val_buf[7u];

  CHECK_EQUAL(0, can_net_recv(net, &msg_last_blk));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_EQUAL(2u, CanSend::msg.data[1]);                // ackseq
  CHECK_EQUAL(CO_SDO_MAX_SEQNO, CanSend::msg.data[2]);  // blksize
  CHECK_VAL_CAN_MSG(0, CanSend::msg.data);
  ResetCanSend();

  co_sub_t* const sub = co_dev_find_sub(dev, IDX, SUBIDX);
  co_sub_set_dn_ind(sub, sub_dn_failing_ind, nullptr);
  can_msg msg_end =
      CreateBlkDnEndCanMsg(co_crc(0, val_buf, sizeof(sub_type64)));
  msg_end.data[0] |= CO_SDO_BLK_SIZE_SET(1u);
  CHECK_EQUAL(0, can_net_recv(net, &msg_end));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_READ, CanSend::msg.data);

  CHECK_EQUAL(0, co_sub_get_val_u64(sub));
}

TEST_GROUP_BASE(CoSsdoBlkUp, CO_Ssdo) {
  int ignore = 0;  // clang-format fix

  static co_unsigned32_t up_ind_inc_req_offset(const co_sub_t* sub,
                                               co_sdo_req* req, void*) {
    co_unsigned32_t ac = 0;
    co_sub_on_up(sub, req, &ac);
    req->offset++;

    return ac;
  }

  static co_unsigned32_t up_ind_ret_err(const co_sub_t* sub, co_sdo_req* req,
                                        void*) {
    co_unsigned32_t ac = 0;
    co_sub_on_up(sub, req, &ac);

    return CO_SDO_AC_DATA;
  }

  static co_unsigned32_t up_ind_big_reqsiz(const co_sub_t* sub, co_sdo_req* req,
                                           void*) {
    co_unsigned32_t ac = 0;
    co_sub_on_up(sub, req, &ac);
    req->size = 3u;
    req->offset = 1u;

    return ac;
  }

  /**
   * Initiate block upload request
   * 
   * @param subidx  size of data to upload
   * @param blksize block size (number of segments client is able to receive)
   */
  void InitBlkUp2020Req(const co_unsigned8_t subidx = SUBIDX,
                        const co_unsigned8_t blksize = CO_SDO_MAX_SEQNO) {
    can_msg msg = CreateDefaultCanMsg();
    msg.id = 0x600u + DEV_ID;
    msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC;
    stle_u16(msg.data + 1u, IDX);
    msg.data[3] = subidx;
    msg.data[4] = blksize;

    CHECK_EQUAL(0, can_net_recv(net, &msg));
  }

  /**
   * check initiate block upload response
   * 
   * @param subidx  expected subindex
   * @param size expected data size
   */
  void CheckInitBlkUp2020ResData(const co_unsigned8_t subidx,
                                 const size_t size) {
    CHECK_EQUAL(1u, CanSend::num_called);
    CHECK_CMD_CAN_MSG(
        CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_BLK_SIZE_IND,
        CanSend::msg.data);
    CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
    CHECK_SUBIDX_CAN_MSG(subidx, CanSend::msg.data);
    CHECK_VAL_CAN_MSG(size, CanSend::msg.data);
  }

  void ChangeStateToEnd() {
    can_msg msg = CreateDefaultCanMsg();
    msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
    CHECK_EQUAL(0, can_net_recv(net, &msg));

    // uploaded value from the server
    CHECK_EQUAL(2u, CanSend::num_called);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[0].id);
    CHECK_EQUAL(0, CanSend::msg_buf[0].flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
    CHECK_CMD_CAN_MSG(1u, CanSend::msg_buf[0].data);
    CHECK_EQUAL(0xefu, CanSend::msg_buf[0].data[1]);
    CHECK_EQUAL(0xcdu, CanSend::msg_buf[0].data[2]);
    CHECK_EQUAL(0xabu, CanSend::msg_buf[0].data[3]);
    CHECK_EQUAL(0x89u, CanSend::msg_buf[0].data[4]);
    CHECK_EQUAL(0x67u, CanSend::msg_buf[0].data[5]);
    CHECK_EQUAL(0x45u, CanSend::msg_buf[0].data[6]);
    CHECK_EQUAL(0x23u, CanSend::msg_buf[0].data[7]);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[1].id);
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
    can_msg msg_con_res = CreateDefaultCanMsg();
    msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
    msg_con_res.data[1] = 2;                 // ackseq
    msg_con_res.data[2] = CO_SDO_MAX_SEQNO;  // blksize
    CHECK_EQUAL(0, can_net_recv(net, &msg_con_res));

    CHECK_EQUAL(1u, CanSend::num_called);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
    CHECK_EQUAL(0, CanSend::msg.flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
    CHECK_CMD_CAN_MSG(
        CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_END_BLK | CO_SDO_BLK_SIZE_SET(1u),
        CanSend::msg.data);
    CHECK_EQUAL(10916u, ldle_u16(CanSend::msg.data + 1u));  // check CRC
    CHECK_EQUAL(0, CanSend::msg.data[3]);
    CHECK_EQUAL(0, CanSend::msg.data[4]);
    CHECK_EQUAL(0, CanSend::msg.data[5]);
    CHECK_EQUAL(0, CanSend::msg.data[6]);
    ResetCanSend();
  }
};

TEST(CoSsdoBlkUp, Sub_Nominal) {
  CreateObjInDev(obj2020, IDX);
  sub_type64 val = 0x5423456789abcdefu;
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, val);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(2u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[0].id);
  CHECK_EQUAL(0, CanSend::msg_buf[0].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
  CHECK_CMD_CAN_MSG(1u, CanSend::msg_buf[0].data);
  CHECK_EQUAL(0xefu, CanSend::msg_buf[0].data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg_buf[0].data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg_buf[0].data[3]);
  CHECK_EQUAL(0x89u, CanSend::msg_buf[0].data[4]);
  CHECK_EQUAL(0x67u, CanSend::msg_buf[0].data[5]);
  CHECK_EQUAL(0x45u, CanSend::msg_buf[0].data[6]);
  CHECK_EQUAL(0x23u, CanSend::msg_buf[0].data[7]);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[1].id);
  CHECK_EQUAL(0, CanSend::msg_buf[1].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[1].len);
  CHECK_CMD_CAN_MSG(2u | CO_SDO_SEQ_LAST, CanSend::msg_buf[1].data);
  CHECK_EQUAL(0x54u, CanSend::msg_buf[1].data[1]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[2]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[3]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[4]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[5]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[6]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[7]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = CreateDefaultCanMsg();
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg_con_res.data[1] = 2u;                // ackseq
  msg_con_res.data[2] = CO_SDO_MAX_SEQNO;  // blksize
  CHECK_EQUAL(0, can_net_recv(net, &msg_con_res));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
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
  can_msg msg_con_end = CreateDefaultCanMsg();
  msg_con_end.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_END_BLK;
  CHECK_EQUAL(0, can_net_recv(net, &msg_con_end));

  CHECK_EQUAL(0, CanSend::num_called);
}

TEST(CoSsdoBlkUp, Sub_BlksizeOne_MsgWithNoLastByte) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX, 1u);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(1u, CanSend::msg.data);  // ackseq
  CHECK_EQUAL(0xefu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[3]);
  CHECK_EQUAL(0x89u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x67u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x45u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x23u, CanSend::msg.data[7]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = CreateDefaultCanMsg();
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
  msg_con_res.data[1] = 1u;  // ackseq
  msg_con_res.data[2] = 1u;  // blksize
  CHECK_EQUAL(0, can_net_recv(net, &msg_con_res));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
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
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  co_obj_set_up_ind(obj2020->Get(), up_ind_ret_err, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX, 1u);
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(CO_SDO_AC_DATA, CanSend::msg.data);
  ResetCanSend();
}

TEST(CoSsdoBlkUp, Sub_StartButReqNotFirst) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  co_obj_set_up_ind(obj2020->Get(), up_ind_inc_req_offset, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_RequestIncremented) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  co_obj_set_up_ind(obj2020->Get(), up_ind_inc_req_offset, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(2u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[0].id);
  CHECK_EQUAL(0, CanSend::msg_buf[0].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
  CHECK_CMD_CAN_MSG(1u, CanSend::msg_buf[0].data);
  CHECK_EQUAL(0xefu, CanSend::msg_buf[0].data[1]);
  CHECK_EQUAL(0xcdu, CanSend::msg_buf[0].data[2]);
  CHECK_EQUAL(0xabu, CanSend::msg_buf[0].data[3]);
  CHECK_EQUAL(0x89u, CanSend::msg_buf[0].data[4]);
  CHECK_EQUAL(0x67u, CanSend::msg_buf[0].data[5]);
  CHECK_EQUAL(0x45u, CanSend::msg_buf[0].data[6]);
  CHECK_EQUAL(0x23u, CanSend::msg_buf[0].data[7]);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[1].id);
  CHECK_EQUAL(0, CanSend::msg_buf[1].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[1].len);
  CHECK_CMD_CAN_MSG(2u | CO_SDO_SEQ_LAST, CanSend::msg_buf[1].data);
  CHECK_EQUAL(0x54u, CanSend::msg_buf[1].data[1]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[2]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[3]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[4]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[5]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[6]);
  CHECK_EQUAL(0, CanSend::msg_buf[1].data[7]);
}

TEST(CoSsdoBlkUp, Sub_ArrNominal) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(1u));
  sub_type val = 0xabcdu;
  obj2020->InsertAndSetSub(0x01u, SUB_TYPE, sub_type(val));
  co_obj_set_code(co_dev_find_obj(dev, IDX), CO_OBJECT_ARRAY);
  StartSSDO();

  InitBlkUp2020Req(0x01u, 2u);
  CheckInitBlkUp2020ResData(0x01u, sizeof(sub_type));
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
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
  can_msg msg_con_res = CreateDefaultCanMsg();
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
  msg_con_res.data[1] = 1u;
  msg_con_res.data[2] = CO_SDO_MAX_SEQNO;
  CHECK_EQUAL(0, can_net_recv(net, &msg_con_res));

  // upload end
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(
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

TEST(CoSsdoBlkUp, Sub_ByteNotLast) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));
  co_obj_set_code(co_dev_find_obj(dev, IDX), CO_OBJECT_ARRAY);
  co_obj_set_up_ind(obj2020->Get(), up_ind_big_reqsiz, nullptr);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX, 3u);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC | CO_SDO_SC_BLK_RES,
                    CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_VAL_CAN_MSG(3u, CanSend::msg.data);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  ResetCanSend();

  can_msg msg_res = CreateDefaultCanMsg();
  msg_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  CHECK_EQUAL(0, can_net_recv(net, &msg_res));
  CHECK_EQUAL(32u, CanSend::num_called);
  for (size_t i = 0; i < 32u; i++) {
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[i].id);
    CHECK_EQUAL(0, CanSend::msg_buf[i].flags);
    CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[i].len);
    CHECK_CMD_CAN_MSG(i + 1u, CanSend::msg_buf[i].data);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[1]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[2]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[3]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[4]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[5]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[6]);
    CHECK_EQUAL(0, CanSend::msg_buf[i].data[7]);
    CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[i].id);
    CHECK_EQUAL(0, CanSend::msg_buf[i].flags);
  }
}

TEST(CoSsdoBlkUp, Sub_ArrInvalidMaxSubidx) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));
  obj2020->InsertAndSetSub(0x01u, SUB_TYPE, sub_type(0xffffu));
  obj2020->InsertAndSetSub(0x02u, SUB_TYPE, sub_type(0xffffu));
  co_obj_set_code(co_dev_find_obj(dev, IDX), CO_OBJECT_ARRAY);
  StartSSDO();

  InitBlkUp2020Req(0x02u, 4u);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(0x02u, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_DATA, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  // upload end
  // server's request
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, InitIniRes_CoSdoCobidFrame) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  co_unsigned32_t cobid_res = (0x580u + DEV_ID) | CO_SDO_COBID_FRAME;
  SetSrv02CobidRes(cobid_res);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(1u, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);

  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg_buf[0].id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg_buf[0].flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg_buf[0].len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
}

TEST(CoSsdoBlkUp, Sub_InvalidCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x0123456789abcdefu));
  SetSrv02CobidRes(0x580u + DEV_ID);
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);

  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_NoCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.len = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_CSAbort) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CS_ABORT;
  msg.len = 1u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK(!CanSend::called());
}

TEST(CoSsdoBlkUp, Sub_InvalidSC) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = 0xffu;
  msg.len = 1u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_EmptyRequest) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_NoBlkSeqNum) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x0123456789abcdefu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(2u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_BLK_RES, CanSend::msg.data);
  CHECK_EQUAL(0x01u, CanSend::msg.data[1]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[4]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[5]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[6]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[7]);
  ResetCanSend();

  can_msg msg2 = CreateDefaultCanMsg();
  msg2.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg2.len = 2u;
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_SEQ, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_TooManySegments) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = CreateDefaultCanMsg();
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_SIZE_IND;
  msg_con_res.data[1] = 1u;
  msg_con_res.data[2] = CO_SDO_MAX_SEQNO + 1u;
  CHECK_EQUAL(0, can_net_recv(net, &msg_con_res));

  // upload end
  // server's request
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_SIZE, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_NoSegments) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg_con_res = CreateDefaultCanMsg();
  msg_con_res.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;
  msg_con_res.data[1] = 1u;
  msg_con_res.data[2] = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg_con_res));

  // upload end
  // server's request
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_BLK_SIZE, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, Sub_StartUp_ButAlreadyStarted) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE, sub_type(0xabcdu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type));
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  // uploaded value from the server
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_SEQ_LAST | CO_SDO_SC_END_BLK, CanSend::msg.data);
  CHECK_EQUAL(0xcdu, CanSend::msg.data[1]);
  CHECK_EQUAL(0xabu, CanSend::msg.data[2]);
  CHECK_EQUAL(0x00u, CanSend::msg.data[3]);
  ResetCanSend();

  // client's confirmation response
  can_msg msg2 = CreateDefaultCanMsg();
  msg2.data[0] = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;
  CHECK_EQUAL(0, can_net_recv(net, &msg2));

  // server's request
  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, End_TimeoutTriggered) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  co_ssdo_set_timeout(ssdo, 1u);  // 1 ms
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();

  const timespec tp = {0L, 1000000L};  // 1 ms
  can_net_set_time(net, &tp);

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_TIMEOUT, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, EndOnRecv_TooShortMsg) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.len = 0;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, EndOnRecv_InvalidCS) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = 0xffu;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, EndOnRecv_InvalidSC) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | 0x03u;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(1u, CanSend::num_called);
  CHECK_EQUAL(0x580u + DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0, CanSend::msg.flags);
  CHECK_EQUAL(CO_SDO_MSG_SIZE, CanSend::msg.len);
  CHECK_CMD_CAN_MSG(CO_SDO_CS_ABORT, CanSend::msg.data);
  CHECK_IDX_CAN_MSG(IDX, CanSend::msg.data);
  CHECK_SUBIDX_CAN_MSG(SUBIDX, CanSend::msg.data);
  CHECK_AC_CAN_MSG(CO_SDO_AC_NO_CS, CanSend::msg.data);
}

TEST(CoSsdoBlkUp, EndOnRecv_CSAbort) {
  CreateObjInDev(obj2020, IDX);
  obj2020->InsertAndSetSub(SUBIDX, SUB_TYPE64, sub_type64(0x5423456789abcdefu));
  StartSSDO();

  InitBlkUp2020Req(SUBIDX);
  CheckInitBlkUp2020ResData(SUBIDX, sizeof(sub_type64));
  ResetCanSend();
  ChangeStateToEnd();
  ResetCanSend();

  can_msg msg = CreateDefaultCanMsg();
  msg.data[0] = CO_SDO_CS_ABORT;
  CHECK_EQUAL(0, can_net_recv(net, &msg));

  CHECK_EQUAL(0, CanSend::num_called);
}
