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

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <CppUTest/TestHarness.h>

#include <lely/can/msg.h>
#include <lely/can/net.h>

#include <lely/co/dev.h>
#include <lely/co/emcy.h>
#include <lely/co/obj.h>

#include <lely/util/endian.h>
#include <lely/util/time.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

using msef_array = std::array<co_unsigned8_t, 5u>;

struct EmcyInd {
  static bool called;
  static co_emcy_t* last_emcy;
  static co_unsigned8_t last_id;
  static co_unsigned16_t last_eec;
  static co_unsigned8_t last_er;
  static msef_array last_msef;
  static void* last_data;

  static void
  Func(co_emcy_t* emcy, co_unsigned8_t id, co_unsigned16_t eec,
       co_unsigned8_t er, co_unsigned8_t* msef, void* data) {
    called = true;
    last_emcy = emcy;
    last_id = id;
    last_eec = eec;
    last_er = er;
    std::copy_n(msef, 5u, last_msef.begin());
    last_data = data;
  }

  static void
  Clear() {
    called = false;
    last_emcy = 0u;
    last_id = 0u;
    last_eec = 0u;
    last_er = 0u;
    std::fill(last_msef.begin(), last_msef.end(), 0u);
    last_data = nullptr;
  }
};

bool EmcyInd::called = false;
co_emcy_t* EmcyInd::last_emcy = 0u;
co_unsigned8_t EmcyInd::last_id = 0u;
co_unsigned16_t EmcyInd::last_eec = 0u;
co_unsigned8_t EmcyInd::last_er = 0u;
msef_array EmcyInd::last_msef;
void* EmcyInd::last_data = nullptr;

TEST_BASE(CO_EmcyBase) {
  TEST_BASE_SUPER(CO_EmcyBase);

  Allocators::Default allocator;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1001;
  std::unique_ptr<CoObjTHolder> obj1003;
  std::unique_ptr<CoObjTHolder> obj1014;
  std::unique_ptr<CoObjTHolder> obj1028;
  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned8_t ERROR_STACK_SIZE = 20u;
  const co_unsigned32_t PRODUCER_CANID = 0x80u + DEV_ID;
  const co_unsigned32_t CONSUMER_CANID = PRODUCER_CANID + 1u;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  void CreateObj1001ErrorRegister(co_unsigned8_t er) {
    CreateObjInDev(obj1001, 0x1001u);
    obj1001->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, er);
  }

  void CreateObj1003PredefinedErrorField() {
    CreateObjInDev(obj1003, 0x1003u);
    co_obj_set_code(obj1003->Get(), CO_OBJECT_ARRAY);
    obj1003->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0u});
    for (co_unsigned8_t i = 0; i < ERROR_STACK_SIZE; ++i) {
      obj1003->InsertAndSetSub(i + 1u, CO_DEFTYPE_UNSIGNED32,
                               co_unsigned32_t{0u});
    }
  }

  void CreateObj1014CobIdEmcy() {
    CreateObjInDev(obj1014, 0x1014u);
    obj1014->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{PRODUCER_CANID});
  }

  void CreateObj1028EmcyConsumerObject() {
    CreateObjInDev(obj1028, 0x1028u);
    co_obj_set_code(obj1028->Get(), CO_OBJECT_ARRAY);
    obj1028->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{1u});
    obj1028->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{CONSUMER_CANID});
  }

  void CheckDefaultEmcyParams(const co_emcy_t* emcy) {
    POINTERS_EQUAL(net, co_emcy_get_net(emcy));
    POINTERS_EQUAL(dev, co_emcy_get_dev(emcy));

    CheckDefaultIndicator(emcy);
  }

  void CheckDefaultIndicator(const co_emcy_t* emcy) {
    int dummy_data = 42;
    co_emcy_ind_t* ind = &EmcyInd::Func;
    void* data = &dummy_data;

    co_emcy_get_ind(emcy, &ind, &data);
    POINTERS_EQUAL(nullptr, ind);
    POINTERS_EQUAL(nullptr, data);
  }

  void CheckEqualObj1001ErrorRegister(co_unsigned8_t er) {
    CHECK_EQUAL(er, co_obj_get_val_u8(obj1001->Get(), 0x00u));
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    EmcyInd::Clear();
  }

  TEST_TEARDOWN() {
    can_net_destroy(net);
    dev_holder.reset();

    set_errnum(0);
  }
};

TEST_GROUP_BASE(CO_EmcyCreate, CO_EmcyBase){};

/// @name co_emcy_alignof()
///@{

/// \Given N/A
///
/// \When co_emcy_alignof() is called
///
/// \Then if \__MINGW32__ and !__MINGW64__: 4 is returned; else 8 is returned
TEST(CO_EmcyCreate, CoEmcyAlignof_Nominal) {
  const auto alignment = co_emcy_alignof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(4u, alignment);
#else
  CHECK_EQUAL(8u, alignment);
#endif
}

///@}

/// @name co_emcy_sizeof()
///@{

/// \Given N/A
///
/// \When co_emcy_sizeof() is called
///
/// \Then if LELY_NO_MALLOC and !LELY_NO_CANFD: 3336 is returned;
///       else if LELY_NO_MALLOC: 2440 is returned;
///       else if \__MINGW32__ and !__MINGW64__: 1080 is returned;
///       else 2160 is returned
TEST(CO_EmcyCreate, CoEmcySizeof_Nominal) {
  const auto size = co_emcy_sizeof();

#ifdef LELY_NO_MALLOC
#if !LELY_NO_CANFD
  CHECK_EQUAL(3336u, size);
#else
  CHECK_EQUAL(2440u, size);
#endif
#else
#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(1080u, size);
#else
  CHECK_EQUAL(2160u, size);
#endif
#endif  // LELY_NO_MALLOC
}

///@}

/// @name co_emcy_create()
///@{

/// \Given pointers to initialized device (co_dev_t) and network (can_net_t)
///
/// \When co_emcy_create() is called with the pointers to the network and the
///       device
///
/// \Then a null pointer is returned and an EMCY service is not created;
///       ERRNUM_NOSYS error number is set
///       \Calls mem_alloc()
///       \Calls co_dev_find_sub()
///       \Calls set_errc()
TEST(CO_EmcyCreate, CoEmcyCreate_NoObj1001) {
  const auto ret = co_emcy_create(net, dev);

  POINTERS_EQUAL(nullptr, ret);

  CHECK_EQUAL(ERRNUM_NOSYS, get_errnum());
}

/// \Given pointers to initialized device (co_dev_t) and network (can_net_t),
///        the object dictionary contains the Error Register object (0x1001)
///
/// \When co_emcy_create() is called with the pointers to the network and the
///       device
///
/// \Then a pointer to a created EMCY service (co_emcy_t) is returned, it has
///       pointers to the network and the device set properly, indication
///       function is not set
///       \Calls mem_alloc()
///       \Calls co_dev_find_sub()
///       \Calls co_dev_find_obj()
///       \Calls can_buf_init()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(CO_EmcyCreate, CoEmcyCreate_NoObj1003And1028) {
  CreateObj1001ErrorRegister(0u);

  const auto emcy = co_emcy_create(net, dev);

  CHECK(emcy != nullptr);

  CheckDefaultEmcyParams(emcy);

  co_emcy_destroy(emcy);
}

/// \Given pointers to initialized device (co_dev_t) and network (can_net_t),
///        the object dictionary contains the Error Register object (0x1001),
///        the Pre-defined Error Field object (0x1003) and the Emergency
///        Consumer Object (0x1028)
///
/// \When co_emcy_create() is called with the pointers to the network and the
///       device
///
/// \Then a pointer to a created EMCY service (co_emcy_t) is returned, it has
///       pointers to the network and the device set properly, indication
///       function is not set
///       \Calls mem_alloc()
///       \Calls co_dev_find_sub()
///       \Calls co_dev_find_obj()
///       \Calls can_buf_init()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_obj_find_sub()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
TEST(CO_EmcyCreate, CoEmcyCreate_Nominal) {
  CreateObj1001ErrorRegister(0u);
  CreateObj1003PredefinedErrorField();
  CreateObj1028EmcyConsumerObject();

  const auto emcy = co_emcy_create(net, dev);

  CHECK(emcy != nullptr);

  CheckDefaultEmcyParams(emcy);

  co_emcy_destroy(emcy);
}

///@}

/// @name co_emcy_destroy()
///@{

/// \Given N/A
///
/// \When co_emcy_destroy() is called with a null pointer
///
/// \Then nothing is changed
TEST(CO_EmcyCreate, CoEmcyDestroy_NullPtr) { co_emcy_destroy(nullptr); }

/// \Given a pointer to an EMCY service (co_emcy_t)
///
/// \When co_emcy_destroy() is called
///
/// \Then the EMCY service is destroyed
///       \Calls co_emcy_stop()
///       \Calls can_recv_destroy()
///       \Calls can_timer_destroy()
///       \Calls can_buf_fini()
///       \Calls mem_free()
TEST(CO_EmcyCreate, CoEmcyDestroy_Nominal) {
  CreateObj1001ErrorRegister(0u);
  const auto emcy = co_emcy_create(net, dev);

  co_emcy_destroy(emcy);
}

///@}

TEST_GROUP_BASE(CO_EmcyMinimal, CO_EmcyBase) {
  co_emcy_t* emcy = nullptr;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObj1001ErrorRegister(0u);

    emcy = co_emcy_create(net, dev);
    CHECK(emcy != nullptr);
  }

  TEST_TEARDOWN() {
    co_emcy_destroy(emcy);

    TEST_BASE_TEARDOWN();
  }
};

/// @name co_emcy_get_dev()
///@{

/// \Given pointers to a device (co_dev_t) and an EMCY service (co_emcy_t)
///        created on the device
///
/// \When co_emcy_get_dev() is called
///
/// \Then a pointer to the device is returned
TEST(CO_EmcyMinimal, CoEmcyGetDev_Nominal) {
  const co_dev_t* d = co_emcy_get_dev(emcy);

  POINTERS_EQUAL(dev, d);
}

///@}

/// @name co_emcy_get_net()
///@{

/// \Given pointers to a network (can_net_t) and an EMCY service (co_emcy_t)
///        created on the network
///
/// \When co_emcy_get_net() is called
///
/// \Then a pointer to the network is returned
TEST(CO_EmcyMinimal, CoEmcyGetNet_Nominal) {
  const can_net_t* n = co_emcy_get_net(emcy);

  POINTERS_EQUAL(net, n);
}

///@}

/// @name co_emcy_get_ind()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t)
///
/// \When co_emcy_get_ind() is called with null pointers to store the results
///
/// \Then nothing is changed
TEST(CO_EmcyMinimal, CoEmcyGetInd_NullPointers) {
  co_emcy_get_ind(emcy, nullptr, nullptr);
}

/// \Given a pointer to an EMCY service (co_emcy_t)
///
/// \When co_emcy_get_ind() is called with pointers to store an indication
///       function and user-specified data
///
/// \Then the pointers to an indication function and data are set to null
TEST(CO_EmcyMinimal, CoEmcyGetInd_DefaultNull) { CheckDefaultIndicator(emcy); }

///@}

/// @name co_emcy_set_ind()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t)
///
/// \When co_emcy_set_ind() is called with a pointer to a custom indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and a pointer to user-specified data are set
///       in the EMCY service
TEST(CO_EmcyMinimal, CoEmcySetInd_Nominal) {
  int data = 42;

  co_emcy_set_ind(emcy, &EmcyInd::Func, &data);

  co_emcy_ind_t* ind = nullptr;
  void* user_data = nullptr;
  co_emcy_get_ind(emcy, &ind, &user_data);
  POINTERS_EQUAL(&EmcyInd::Func, ind);
  POINTERS_EQUAL(&data, user_data);
}

///@}

/// @name co_emcy_start()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary does
///        not contain the Pre-defined Error Field object (0x1003), the COB-ID
///        EMCY object (0x1014) and the Emergency Consumer Object (0x1028)
///
/// \When co_emcy_start() is called
///
/// \Then 0 is returned, the EMCY service is started
///       \Calls can_net_get_time()
///       \Calls co_dev_find_obj()
TEST(CO_EmcyMinimal, CoEmcyStart_NoObj1003_1014_1028) {
  const int ret = co_emcy_start(emcy);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_emcy_is_stopped(emcy));
}

/// \Given a pointer to an already started EMCY service (co_emcy_t)
///
/// \When co_emcy_start() is called
///
/// \Then 0 is returned, nothing is changed
TEST(CO_EmcyMinimal, CoEmcyStart_AlreadyStarted) {
  co_emcy_start(emcy);

  const int ret = co_emcy_start(emcy);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_emcy_is_stopped(emcy));
}

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with at least one
///        declared consumer COB-ID missing
///
/// \When co_emcy_start() is called
///
/// \Then 0 is returned, the EMCY service is started, missing consumer COB-IDs
///       are ignored
///       \Calls can_net_get_time()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_val_u32()
///       \Calls can_recv_start()
TEST(CO_EmcyCreate, CoEmcyStart_Obj1028_WithMissingSubObject) {
  CreateObj1001ErrorRegister(0u);
  CreateObjInDev(obj1028, 0x1028u);
  co_obj_set_code(obj1028->Get(), CO_OBJECT_ARRAY);
  obj1028->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{2u});
  obj1028->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});
  // missing 0x02 sub-indx

  auto* emcy = co_emcy_create(net, dev);
  CHECK(emcy != nullptr);

  const int ret = co_emcy_start(emcy);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_emcy_is_stopped(emcy));

  co_emcy_destroy(emcy);
}

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) set with more than
///        127 consumer COB-IDs
///
/// \When co_emcy_start() is called
///
/// \Then 0 is returned, the EMCY service is started, excess consumer COB-IDs
///       are ignored
///       \Calls can_net_get_time()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
///       \Calls co_obj_get_val_u32()
///       \Calls can_recv_start()
TEST(CO_EmcyCreate, CoEmcyStart_Obj1028_BiggerThanMaxNodes) {
  CreateObj1001ErrorRegister(0u);
  CreateObjInDev(obj1028, 0x1028u);
  co_obj_set_code(obj1028->Get(), CO_OBJECT_ARRAY);
  obj1028->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                           co_unsigned8_t{CO_NUM_NODES + 1u});
  for (co_unsigned8_t i = 0; i < CO_NUM_NODES + 1u; ++i)
    obj1028->InsertAndSetSub(i + 1u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{i + 1u});

  auto* emcy = co_emcy_create(net, dev);
  CHECK(emcy != nullptr);
  co_emcy_set_ind(emcy, &EmcyInd::Func, nullptr);

  const int ret = co_emcy_start(emcy);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_emcy_is_stopped(emcy));

  can_msg msg = CAN_MSG_INIT;
  msg.id = CO_NUM_NODES + 1;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CHECK_FALSE(EmcyInd::called);

  msg.id = CO_NUM_NODES;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CHECK_TRUE(EmcyInd::called);

  co_emcy_destroy(emcy);
}

///@}

/// @name co_emcy_stop()
///@{

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary does not contain the Pre-defined Error Field object
///        (0x1003), the COB-ID EMCY object (0x1014) and the Emergency Consumer
///        Object (0x1028)
///
/// \When co_emcy_stop() is called
///
/// \Then the EMCY service is stopped
///       \Calls can_timer_stop()
///       \Calls co_dev_find_obj()
TEST(CO_EmcyMinimal, CoEmcyStop_NoObj1003_1028_1014) {
  co_emcy_start(emcy);

  co_emcy_stop(emcy);

  CHECK_EQUAL(1, co_emcy_is_stopped(emcy));
}

/// \Given a pointer to a not started EMCY service (co_emcy_t)
///
/// \When co_emcy_stop() is called
///
/// \Then nothing is changed
TEST(CO_EmcyMinimal, CoEmcyStop_NotStarted) {
  co_emcy_stop(emcy);

  CHECK_EQUAL(1, co_emcy_is_stopped(emcy));
}

///@}

/// @name co_emcy_is_stopped()
///@{

/// \Given a pointer to a not started EMCY service (co_emcy_t)
///
/// \When co_emcy_is_stopped() is called
///
/// \Then 1 is returned
TEST(CO_EmcyMinimal, CoEmcyIsStopped_NotStarted) {
  CHECK_EQUAL(1, co_emcy_is_stopped(emcy));
}

/// \Given a pointer to a started EMCY service (co_emcy_t)
///
/// \When co_emcy_is_stopped() is called
///
/// \Then 0 is returned
TEST(CO_EmcyMinimal, CoEmcyIsStopped_AfterStart) {
  co_emcy_start(emcy);

  CHECK_EQUAL(0, co_emcy_is_stopped(emcy));
}

/// \Given a pointer to a stopped EMCY service (co_emcy_t)
///
/// \When co_emcy_is_stopped() is called
///
/// \Then 1 is returned
TEST(CO_EmcyMinimal, CoEmcyIsStopped_AfterStop) {
  co_emcy_start(emcy);
  co_emcy_stop(emcy);

  CHECK_EQUAL(1, co_emcy_is_stopped(emcy));
}

///@}

struct EmcySend : CanSend {
  static void
  CheckMsg(co_unsigned32_t msg_id, co_unsigned16_t eec, co_unsigned8_t er,
           const co_unsigned8_t* msef) {
    CHECK_EQUAL(1u, num_called);
    CHECK_EQUAL(msg_id, EmcySend::msg.id);
    CHECK_EQUAL(8u, EmcySend::msg.len);
    CHECK_EQUAL(0u, EmcySend::msg.flags);

    co_unsigned8_t eec_buf[2] = {0u};
    stle_u16(eec_buf, eec);
    CHECK_EQUAL(eec_buf[0], EmcySend::msg.data[0]);
    CHECK_EQUAL(eec_buf[1], EmcySend::msg.data[1]);
    CHECK_EQUAL(er, EmcySend::msg.data[2]);

    if (!msef) {
      co_unsigned8_t zeroes[5] = {0u};
      CheckMsef(zeroes);
      return;
    }

    CheckMsef(msef);
  }

  static void
  CheckMsef(const co_unsigned8_t* msef) {
    CHECK_EQUAL(msef[0], EmcySend::msg.data[3]);
    CHECK_EQUAL(msef[1], EmcySend::msg.data[4]);
    CHECK_EQUAL(msef[2], EmcySend::msg.data[5]);
    CHECK_EQUAL(msef[3], EmcySend::msg.data[6]);
    CHECK_EQUAL(msef[4], EmcySend::msg.data[7]);
  }
};

TEST_GROUP_BASE(CO_Emcy, CO_EmcyBase) {
  co_emcy_t* emcy = nullptr;

  void CheckEqualObj1003PredefinedErrorField(
      const std::vector<co_unsigned32_t>& error_codes) {
    CHECK_EQUAL(error_codes.size(), co_obj_get_val_u8(obj1003->Get(), 0x00u));

    co_unsigned8_t subidx = 1u;
    for (const auto eec : error_codes) {
      const auto desc = "sub-idx: " + std::to_string(subidx);
      CHECK_EQUAL_TEXT(eec, co_obj_get_val_u32(obj1003->Get(), subidx),
                       desc.c_str());
      ++subidx;
    }
  }

  void CheckEmptyObj1003PredefinedErrorField() {
    CheckEqualObj1003PredefinedErrorField({});
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObj1001ErrorRegister(0u);
    CreateObj1003PredefinedErrorField();
    CreateObj1014CobIdEmcy();
    CreateObj1028EmcyConsumerObject();

    emcy = co_emcy_create(net, dev);
    CHECK(emcy != nullptr);

    EmcySend::Clear();
  }

  TEST_TEARDOWN() {
    co_emcy_destroy(emcy);

    TEST_BASE_TEARDOWN();
  }
};

/// @name co_emcy_start()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Pre-defined Error Field object (0x1003), the COB-ID EMCY
///        object (0x1014) and the Emergency Consumer Object (0x1028)
///
/// \When co_emcy_start() is called
///
/// \Then 0 is returned, the EMCY service is started and download indication
///       functions for the 0x1003, 0x1014 and 0x1028 objects are set
///       \Calls can_net_get_time()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
///       \Calls can_recv_start()
TEST(CO_Emcy, CoEmcyStart_Nominal) {
  const int ret = co_emcy_start(emcy);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_emcy_is_stopped(emcy));
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1003u, emcy);
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1014u, emcy);
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1028u, emcy);
}

///@}

/// @name co_emcy_stop()
///@{

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the Pre-defined Error Field object (0x1003), the
///        COB-ID EMCY object (0x1014) and the Emergency Consumer Object
///        (0x1028)
///
/// \When co_emcy_stop() is called
///
/// \Then the EMCY service is stopped and download indication functions for the
///       0x1003, 0x1014 and 0x1028 objects are set to default
///       \Calls can_timer_stop()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
///       \Calls can_recv_stop()
///       \Calls co_obj_set_dn_ind()
TEST(CO_Emcy, CoEmcyStop_Nominal) {
  co_emcy_start(emcy);

  co_emcy_stop(emcy);

  CHECK_EQUAL(1, co_emcy_is_stopped(emcy));
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1003u);
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1014u);
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1028u);
}

///@}

TEST_GROUP_BASE(CO_EmcyProducerNoObj1003, CO_EmcyBase) {
  co_emcy_t* emcy = nullptr;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObj1001ErrorRegister(0u);
    CreateObj1014CobIdEmcy();
    CreateObj1028EmcyConsumerObject();

    emcy = co_emcy_create(net, dev);
    CHECK(emcy != nullptr);

    can_net_set_send_func(net, &EmcySend::func, nullptr);
    CHECK_EQUAL(0, co_emcy_start(emcy));

    EmcySend::Clear();
  }

  TEST_TEARDOWN() {
    co_emcy_destroy(emcy);

    TEST_BASE_TEARDOWN();
  }
};

TEST_GROUP_BASE(CO_EmcyInhibitTime, CO_EmcyBase) {
  std::unique_ptr<CoObjTHolder> obj1015;
  co_emcy_t* emcy = nullptr;

  void CreateObj1015InhibitTimeEmcy(co_unsigned16_t time) {
    CreateObjInDev(obj1015, 0x1015u);
    obj1015->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                             time);  // N x 100us
  }

  void SetCurrentTimeMs(uint_least64_t ms) {
    timespec tp = {0, 0u};
    timespec_add_msec(&tp, ms);
    CHECK_EQUAL(0, can_net_set_time(net, &tp));
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObj1001ErrorRegister(0u);
    CreateObj1014CobIdEmcy();
    CreateObj1028EmcyConsumerObject();

    emcy = co_emcy_create(net, dev);
    CHECK(emcy != nullptr);

    can_net_set_send_func(net, &EmcySend::func, nullptr);
    CHECK_EQUAL(0, co_emcy_start(emcy));

    EmcySend::Clear();
  }

  TEST_TEARDOWN() {
    co_emcy_destroy(emcy);

    TEST_BASE_TEARDOWN();
  }
};

TEST_GROUP_BASE(CO_EmcyReceiver, CO_EmcyBase) {
  co_emcy_t* emcy = nullptr;
  int data = 42;

  void CheckEmcyIndCall(co_unsigned16_t eec, co_unsigned8_t er,
                        const msef_array& msef) {
    CHECK_TRUE(EmcyInd::called);
    POINTERS_EQUAL(emcy, EmcyInd::last_emcy);
    CHECK_EQUAL(0x01u, EmcyInd::last_id);
    POINTERS_EQUAL(&data, EmcyInd::last_data);

    CHECK_EQUAL(eec, EmcyInd::last_eec);
    CHECK_EQUAL(er, EmcyInd::last_er);

    for (std::size_t i = 0; i < msef.size(); ++i) {
      CHECK_EQUAL(msef[i], EmcyInd::last_msef[i]);
    }
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObj1001ErrorRegister(0u);
    CreateObj1003PredefinedErrorField();
    CreateObj1028EmcyConsumerObject();

    emcy = co_emcy_create(net, dev);
    CHECK(emcy != nullptr);

    co_emcy_set_ind(emcy, &EmcyInd::Func, &data);
    CHECK_EQUAL(0, co_emcy_start(emcy));
  }

  TEST_TEARDOWN() {
    co_emcy_destroy(emcy);

    TEST_BASE_TEARDOWN();
  }
};

/// @name co_emcy_push()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Pre-defined Error Field object (0x1003)
///
/// \When co_emcy_push() is called with a zero emergency error code, any error
///       register and any pointer to manufacturer-specific error code
///
/// \Then -1 is returned, ERRNUM_INVAL error number is set
///       \Calls set_errnum()
TEST(CO_Emcy, CoEmcyPush_ZeroErrorCode) {
  const co_unsigned16_t eec = 0u;
  const co_unsigned8_t er = 0x01u;

  const auto ret = co_emcy_push(emcy, eec, er, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003) with no errors recorded
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then 0 is returned, the error register value is set in the object 0x1001
///       and the emergency error code is set in the object 0x1003
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
TEST(CO_Emcy, CoEmcyPush_EmptyErrorStack) {
  const co_unsigned16_t eec = 0x1000u;
  const co_unsigned8_t er = 0x01u;

  const auto ret = co_emcy_push(emcy, eec, er, nullptr);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(er);
  CheckEqualObj1003PredefinedErrorField({eec});
}

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003) with multiple errors recorded
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then 0 is returned, the emergency error code is stored at sub-index 0x01 in
///       the object 0x1003, the older error codes are moved to their next
///       higher sub-objects in the object 0x1003 and the error register in the
///       object 0x1001 is updated with the requested value
///       \Calls memmove()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
TEST(CO_Emcy, CoEmcyPush_MultipleErrors) {
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));

  const auto ret = co_emcy_push(emcy, 0x3000u, 0x04u, nullptr);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(0x01u | 0x02u | 0x04u);
  CheckEqualObj1003PredefinedErrorField({0x3000u, 0x2000u, 0x1000u});
}

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003)
///
/// \When co_emcy_push() is called with a non-zero emergency error code, an
///       error register with zeroed Generic Error bit (0x01) and any pointer to
///       manufacturer-specific error code
///
/// \Then 0 is returned, the requested error register is set in the object
///       0x1001 with the Generic Error bit set
///       \Calls memmove()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
TEST(CO_Emcy, CoEmcyPush_ErrorRegister_GenericErrorBit) {
  const auto ret = co_emcy_push(emcy, 0x1234u, 0x04u, nullptr);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(0x04u | 0x01u);
}

#if LELY_NO_MALLOC

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003) with CO_EMCY_MAX_NMSG (default 8) errors
///        recorded
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then -1 is returned, the objects 0x1001 and 0x1003 are not updated,
///       ERRNUM_NOMEM error number is set
///       \Calls set_errnum()
TEST(CO_Emcy, CoEmcyPush_AtEmcyMessageLimit) {
#if LELY_NO_MALLOC
#ifndef CO_EMCY_MAX_NMSG
#define CO_EMCY_MAX_NMSG 8
#endif
#endif  // LELY_NO_MALLOC

  for (std::size_t i = 0u; i < CO_EMCY_MAX_NMSG; ++i) {
    CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  }

  const auto ret = co_emcy_push(emcy, 0x1000u, 0x01u, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

#endif  // LELY_NO_MALLOC

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014)
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and a null manufacturer-specific error code pointer
///
/// \Then 0 is returned, an EMCY message with CAN-ID from the object 0x1014 is
///       sent with the error register, the emergency error code and zeroes as
///       manufacturer-specific error code
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyPush_SendNullMsef) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  const co_unsigned16_t eec = 0x1000u;
  const co_unsigned8_t er = 0x01u;

  const auto ret = co_emcy_push(emcy, eec, er, nullptr);

  CHECK_EQUAL(0, ret);
  EmcySend::CheckMsg(PRODUCER_CANID, eec, er, nullptr);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014)
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and a pointer to an manufacturer-specific error code
///
/// \Then 0 is returned, an EMCY message with CAN-ID from the object 0x1014 is
///       sent with the error register and both error codes
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyPush_SendMsef) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  const co_unsigned16_t eec = 0x1000u;
  const co_unsigned8_t er = 0x01u;
  co_unsigned8_t msef[5] = {0x10u, 0x11u, 0x12u, 0x13u, 0x14u};

  const auto ret = co_emcy_push(emcy, eec, er, msef);

  CHECK_EQUAL(0, ret);
  EmcySend::CheckMsg(PRODUCER_CANID, eec, er, msef);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014)
///
/// \When co_emcy_push() is called multiple times with non-zero emergency error
///       codes, any error register values and pointers to manufacturer-specific
///       error codes
///
/// \Then 0 is returned and one EMCY message is sent for each call
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyPush_SendMultipleAtOnce) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x01u, nullptr));

  CHECK_EQUAL(3u, EmcySend::num_called);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014) with an invalid
///        COB-ID set
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then 0 is returned, no EMCY message is sent
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
TEST(CO_Emcy, CoEmcyPush_SendInvalidCobidEmcy) {
  co_obj_set_val_u32(obj1014->Get(), 0x00u,
                     PRODUCER_CANID | CO_EMCY_COBID_VALID);
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  const auto ret = co_emcy_push(emcy, 0x1000u, 0x01u, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, EmcySend::num_called);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014) with a valid
///        COB-ID using Extended Identifier
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then 0 is returned, an EMCY message with CAN-ID from the object 0x1014 is
///       sent
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyPush_SendExtendedId) {
  const co_unsigned32_t eid = PRODUCER_CANID | (1 << 28u);
  co_obj_set_val_u32(obj1014->Get(), 0x00u, eid | CO_EMCY_COBID_FRAME);
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  const auto ret = co_emcy_push(emcy, 0x1000u, 0x01u, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, EmcySend::num_called);
  CHECK_EQUAL(eid, EmcySend::msg.id);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the Error Register object (0x1001), the COB-ID
///        EMCY object (0x1014) and the Pre-defined Error Field object (0x1003)
///        with no sub-objects
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then 0 is returned, the error register in the object 0x1001 is updated with
///       the requested value, an EMCY message with CAN-ID from the object
///       0x1014 is sent with the error register and both error codes
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyPush_EmptyObj1003_SendAndSetErrorRegister) {
  while (!obj1003->GetSubs().empty()) obj1003->RemoveAndDestroyLastSub();

  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  const co_unsigned16_t eec = 0x1000u;
  const co_unsigned8_t er = 0x01u;

  const auto ret = co_emcy_push(emcy, eec, er, nullptr);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(er);
  EmcySend::CheckMsg(PRODUCER_CANID, eec, er, nullptr);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the Error Register object (0x1001) and the COB-ID
///        EMCY object (0x1014) but does not contain the Pre-defined Error Field
///        object (0x1003)
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then 0 is returned, the error register in the object 0x1001 is updated with
///       the requested value and an EMCY message with CAN-ID from the object
///       0x1014 is sent with the error register and both error codes
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_EmcyProducerNoObj1003, CoEmcyPush_SendAndSetErrorRegister) {
  const co_unsigned16_t eec = 0x1000u;
  const co_unsigned8_t er = 0x01u;
  co_unsigned8_t msef[5] = {0x10u, 0x11u, 0x12u, 0x13u, 0x14u};

  const auto ret = co_emcy_push(emcy, eec, er, msef);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(er);
  EmcySend::CheckMsg(PRODUCER_CANID, eec, er, msef);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014) and the Inhibit
///        Time EMCY object (0x1015) with a non-zero value set
///
/// \When co_emcy_push() is called multiple times at the same time with non-zero
///       emergency error codes, any error register values and pointers to
///       manufacturer-specific error codes
///
/// \Then 0 is returned from each call, an EMCY message is sent for the first
///       call only
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_timer_start()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_EmcyInhibitTime, CoEmcyPush_SendOnlyOne) {
  CreateObj1015InhibitTimeEmcy(1u);

  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x04u, nullptr));

  CheckEqualObj1001ErrorRegister(0x04u | 0x02u | 0x01u);
  CHECK_EQUAL(1u, EmcySend::num_called);
  CHECK_EQUAL(0x01u, EmcySend::msg.data[2]);
}

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014) and the Inhibit
///        Time EMCY object (0x1015) with a non-zero value set
///
/// \When co_emcy_push() is called multiple times at the same time with non-zero
///       emergency error codes, any error register values and pointers to
///       manufacturer-specific error codes
///
/// \Then 0 is returned from each call, the next EMCY message is sent after the
///       inhibit time has passed
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_timer_start()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_EmcyInhibitTime, CoEmcyPush_SendOneOnTimerTick) {
  CreateObj1015InhibitTimeEmcy(10u);
  SetCurrentTimeMs(0u);

  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x04u, nullptr));

  CHECK_EQUAL(1u, EmcySend::num_called);
  CHECK_EQUAL(0x01u, EmcySend::msg.data[2]);

  SetCurrentTimeMs(1u);
  CHECK_EQUAL(2u, EmcySend::num_called);
  CHECK_EQUAL(0x02u | 0x01u, EmcySend::msg.data[2]);

  SetCurrentTimeMs(2u);
  CHECK_EQUAL(3u, EmcySend::num_called);
  CHECK_EQUAL(0x04u | 0x02u | 0x01u, EmcySend::msg.data[2]);
}

#if LELY_NO_MALLOC

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the COB-ID EMCY object (0x1014) and the Inhibit
///        Time EMCY object (0x1015) with a non-zero value set, there are
///        CO_EMCY_CAN_BUF_SIZE (default 16) EMCY messages waiting to be sent
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then -1 is returned, ERRNUM_NOMEM error number is set
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls set_errnum()
TEST(CO_EmcyInhibitTime, CoEmcyPush_MessageBufferFull) {
  CreateObj1015InhibitTimeEmcy(1u);

#if LELY_NO_MALLOC
#ifndef CO_EMCY_CAN_BUF_SIZE
#define CO_EMCY_CAN_BUF_SIZE 16
#endif
#endif  // LELY_NO_MALLOC

  for (std::size_t i = 0u; i < CO_EMCY_CAN_BUF_SIZE / 2u; ++i) {
    CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
    CHECK_EQUAL(0, co_emcy_clear(emcy));
  }
  if (CO_EMCY_CAN_BUF_SIZE % 2u == 1u) {
    CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  }

  const auto ret = co_emcy_push(emcy, 0x1000u, 0x01u, nullptr);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

#endif  // LELY_NO_MALLOC

/// \Given a pointer to a started EMCY service (co_emcy_t), the object
///        dictionary contains the Error Register object (0x1001) but does not
///        contain the COB-ID EMCY object (0x1014)
///
/// \When co_emcy_push() is called with a non-zero emergency error code, any
///       error register and any pointer to manufacturer-specific error code
///
/// \Then 0 is returned, the error register in the object 0x1001 is updated with
///       the requested value, no EMCY message is sent
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
TEST(CO_EmcyReceiver, CoEmcyPush_CannotSend) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  EmcySend::Clear();

  const auto ret = co_emcy_push(emcy, 0x1000u, 0x01u, nullptr);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(0x01u);
  CHECK_EQUAL(0u, EmcySend::num_called);
}

///@}

/// @name co_emcy_peek()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t)
///
/// \When co_emcy_peek() is called with null pointers to store the results
///
/// \Then nothing is changed
TEST(CO_Emcy, CoEmcyPeek_NullPointers) { co_emcy_peek(emcy, nullptr, nullptr); }

/// \Given a pointer to an EMCY service (co_emcy_t) with no errors recorded
///
/// \When co_emcy_peek() is called with pointers to store an emergency
///       error code and an error register
///
/// \Then the error code and error register are set to 0
TEST(CO_Emcy, CoEmcyPeek_EmptyErrorStack) {
  co_unsigned16_t eec = 0xffffu;
  co_unsigned8_t er = 0xffu;

  co_emcy_peek(emcy, &eec, &er);

  CHECK_EQUAL(0u, eec);
  CHECK_EQUAL(0u, er);
}

/// \Given a pointer to an EMCY service (co_emcy_t) with multiple errors
///        recorded
///
/// \When co_emcy_peek() is called with pointers to store an emergency
///       error code and an error register value
///
/// \Then the error code and error register are set to last recorded values
TEST(CO_Emcy, CoEmcyPeek_MultipleErrors) {
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x04u, nullptr));

  co_unsigned16_t eec = 0xffffu;
  co_unsigned8_t er = 0xffu;

  co_emcy_peek(emcy, &eec, &er);

  CHECK_EQUAL(0x3000u, eec);
  CHECK_EQUAL(0x04u | 0x02u | 0x01u, er);
}

///@}

/// @name co_emcy_clear()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003) with no errors recorded
///
/// \When co_emcy_clear() is called
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Emcy, CoEmcyClear_EmptyErrorStack) {
  const auto ret = co_emcy_clear(emcy);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(0x00u);
  CheckEmptyObj1003PredefinedErrorField();
}

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003) with multiple errors recorded
///
/// \When co_emcy_clear() is called
///
/// \Then 0 is returned, the error register is set to 0 in the object 0x1001 and
///       the object 0x1003 is empty
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
TEST(CO_Emcy, CoEmcyClear_MultipleErrors) {
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x04u, nullptr));

  const auto ret = co_emcy_clear(emcy);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(0x00u);
  CheckEmptyObj1003PredefinedErrorField();
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with no errors
///        recorded, the object dictionary contains the COB-ID EMCY object
///        (0x1014)
///
/// \When co_emcy_clear() is called
///
/// \Then 0 is returned, no EMCY message is sent
TEST(CO_Emcy, CoEmcyClear_NotSentOnEmptyErrorStack) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  const auto ret = co_emcy_clear(emcy);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(EmcySend::Called());
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with multiple errors
///        recorded, the object dictionary contains the COB-ID EMCY object
///        (0x1014)
///
/// \When co_emcy_clear() is called
///
/// \Then 0 is returned, an EMCY message with CAN-ID from the object 0x1014 is
///       sent with all error values zeroed
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyClear_SendResetMessage) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x04u, nullptr));
  EmcySend::Clear();

  const auto ret = co_emcy_clear(emcy);

  CHECK_EQUAL(0, ret);
  EmcySend::CheckMsg(PRODUCER_CANID, 0u, 0u, nullptr);
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with at least one
///        error recorded, the object dictionary contains the Error Register
///        object (0x1001) and the COB-ID EMCY object (0x1014) but does not
///        contain the Pre-defined Error Field object (0x1003)
///
/// \When co_emcy_clear() is called
///
/// \Then 0 is returned, the error register is set to 0 in the object 0x1001 and
///       an EMCY message with CAN-ID from the object 0x1014 is sent with all
///       values zeroed
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_EmcyProducerNoObj1003, CoEmcyClear_SendAndSetErrorRegister) {
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  EmcySend::Clear();

  const auto ret = co_emcy_clear(emcy);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(0x00u);
  EmcySend::CheckMsg(PRODUCER_CANID, 0u, 0u, nullptr);
}

///@}

/// @name co_emcy_pop()
///@{

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003) with no errors recorded
///
/// \When co_emcy_pop() is called with pointers to store an emergency
///       error code and an error register value
///
/// \Then 0 is returned, the error code and error register are set to 0, the
///       objects 0x1001 and 0x1003 are not changed
///       \Calls co_emcy_peek()
TEST(CO_Emcy, CoEmcyPop_EmptyErrorStack) {
  co_unsigned16_t eec = 0xffffu;
  co_unsigned8_t er = 0xffu;

  const auto ret = co_emcy_pop(emcy, &eec, &er);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, eec);
  CHECK_EQUAL(0u, er);
  CheckEqualObj1001ErrorRegister(0x00u);
  CheckEmptyObj1003PredefinedErrorField();
}

/// \Given a pointer to an EMCY service (co_emcy_t), the object dictionary
///        contains the Error Register object (0x1001) and the Pre-defined Error
///        Field object (0x1003) with multiple errors recorded
///
/// \When co_emcy_pop() is called with pointers to store an emergency
///       error code and an error register value
///
/// \Then 0 is returned, the error code and error register are set to last
///       recorded values, contents of the objects 0x1001 and 0x1003 are
///       restored to the state before recording the last error
///       \Calls co_emcy_peek()
///       \Calls memmove()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
TEST(CO_Emcy, CoEmcyPop_MultipleErrors) {
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x04u, nullptr));

  co_unsigned16_t eec = 0xffffu;
  co_unsigned8_t er = 0xffu;

  const auto ret = co_emcy_pop(emcy, &eec, &er);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x3000u, eec);
  CHECK_EQUAL(0x04u | 0x02u | 0x01u, er);

  CheckEqualObj1001ErrorRegister(0x02u | 0x01u);
  CheckEqualObj1003PredefinedErrorField({0x2000u, 0x1000u});
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with no errors
///        recorded, the object dictionary contains the COB-ID EMCY object
///        (0x1014)
///
/// \When co_emcy_pop() is called with any pointers to store an emergency
///       error code and an error register value
///
/// \Then 0 is returned, no EMCY message is sent
///       \Calls co_emcy_peek()
TEST(CO_Emcy, CoEmcyPop_NotSentOnEmptyErrorStack) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  const auto ret = co_emcy_pop(emcy, nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, EmcySend::num_called);
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with multiple errors
///        recorded, the object dictionary contains the COB-ID EMCY object
///        (0x1014)
///
/// \When co_emcy_pop() is called with any pointers to store an emergency
///       error code and an error register value
///
/// \Then 0 is returned, an EMCY message with CAN-ID from the object 0x1014 is
///       sent with zeroed emergency error code, the error register value and
///       the manufacturer-specific error code containing the emergency error
///       code in little-endian byte order in first two bytes, both taken from
///       remaining recorded errors
///       \Calls co_emcy_peek()
///       \Calls memmove()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls stle_u16()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyPop_SendAfterPoppingOne) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x2000u, 0x02u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x3000u, 0x04u, nullptr));
  EmcySend::Clear();

  const auto ret = co_emcy_pop(emcy, nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  const co_unsigned8_t expected_msef[5] = {0x00u, 0x20u, 0u, 0u, 0u};
  EmcySend::CheckMsg(PRODUCER_CANID, 0u, 0x02u | 0x01u, expected_msef);
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with one error
///        recorded, the object dictionary contains the COB-ID EMCY object
///        (0x1014)
///
/// \When co_emcy_pop() is called with any pointers to store an emergency
///       error code and an error register value
///
/// \Then 0 is returned, an EMCY message with CAN-ID from the object 0x1014 is
///       sent with all error values zeroed
///       \Calls co_emcy_peek()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_Emcy, CoEmcyPop_SendAfterPoppingLast) {
  can_net_set_send_func(net, &EmcySend::func, nullptr);
  CHECK_EQUAL(0, co_emcy_start(emcy));

  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  EmcySend::Clear();

  const auto ret = co_emcy_pop(emcy, nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  EmcySend::CheckMsg(PRODUCER_CANID, 0u, 0u, nullptr);
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with at least one
///        error recorded, the object dictionary contains the Error Register
///        object (0x1001) and the COB-ID EMCY object (0x1014) but does not
///        contain the Pre-defined Error Field object (0x1003)
///
/// \When co_emcy_pop() is called with any pointers to store an emergency error
///       code and an error register value
///
/// \Then 0 is returned, the value in the object 0x1001 is restored to the state
///       before recording the last error, an EMCY message with CAN-ID from the
///       object 0x1014 is sent with the details taken from remaining recorded
///       errors
///       \Calls co_emcy_peek()
///       \Calls memmove()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
///       \Calls stle_u16()
///       \Calls co_sub_set_val_u8()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_net_get_time()
///       \Calls can_buf_size()
///       \Calls timespec_cmp()
///       \Calls timespec_add_usec()
///       \Calls can_buf_read()
///       \Calls can_net_send()
TEST(CO_EmcyProducerNoObj1003, CoEmcyPop_SendAndSetErrorRegister) {
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x1000u, 0x01u, nullptr));
  EmcySend::Clear();

  const auto ret = co_emcy_pop(emcy, nullptr, nullptr);

  CHECK_EQUAL(0, ret);
  CheckEqualObj1001ErrorRegister(0x00u);
  EmcySend::CheckMsg(PRODUCER_CANID, 0u, 0u, nullptr);
}

///@}

/// @name EMCY message receiver
///@{

/// \Given a pointer to a started EMCY service (co_emcy_t) with an indication
///        function set, the object dictionary contains the Emergency Consumer
///        Object (0x1028)
///
/// \When an EMCY message with data length of 0 is received
///
/// \Then the indication function is called with zeroed emergency error code,
///       error register and manufacturer-specific error code
TEST(CO_EmcyReceiver, CoEmcyNodeRecv_EmptyMessageData) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = CONSUMER_CANID;

  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckEmcyIndCall(0u, 0u, {0u, 0u, 0u, 0u, 0u});
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with no indication
///        function set, the object dictionary contains the Emergency Consumer
///        Object (0x1028)
///
/// \When an EMCY message is received
///
/// \Then nothing is changed
TEST(CO_EmcyReceiver, CoEmcyNodeRecv_NoIndFunc) {
  co_emcy_set_ind(emcy, nullptr, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CONSUMER_CANID;

  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with an indication
///        function set, the object dictionary contains the Emergency Consumer
///        Object (0x1028)
///
/// \When an EMCY message with data length of 8 is received
///
/// \Then the indication function is called with an emergency error code, an
///       error register and a manufacturer-specific error code all read from
///       the message
///       \Calls ldle_u16()
///       \Calls memcpy()
TEST(CO_EmcyReceiver, CoEmcyNodeRecv_Nominal) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = CONSUMER_CANID;
  msg.len = 8u;
  msg.data[0] = 0x01;
  msg.data[1] = 0x02;
  msg.data[2] = 0x03;
  msg.data[3] = 0x04;
  msg.data[4] = 0x05;
  msg.data[5] = 0x06;
  msg.data[6] = 0x07;
  msg.data[7] = 0x08;

  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckEmcyIndCall(0x0201u, 0x03u, {0x04u, 0x05u, 0x06u, 0x07u, 0x08u});
}

/// \Given a pointer to a started EMCY service (co_emcy_t) with an indication
///        function set, the object dictionary contains the Emergency Consumer
///        Object (0x1028)
///
/// \When an EMCY message with data length greater than 8 is received
///
/// \Then the indication function is called with an emergency error code, an
///       error register and a manufacturer-specific error code all read from
///       the first 8 bytes of the message
///       \Calls ldle_u16()
///       \Calls memcpy()
TEST(CO_EmcyReceiver, CoEmcyNodeRecv_TooLargeMessageLength) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = CONSUMER_CANID;
  msg.len = 10u;
  msg.data[0] = 0x01;
  msg.data[1] = 0x02;
  msg.data[2] = 0x03;
  msg.data[3] = 0x04;
  msg.data[4] = 0x05;
  msg.data[5] = 0x06;
  msg.data[6] = 0x07;
  msg.data[7] = 0x08;

  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CheckEmcyIndCall(0x0201u, 0x03u, {0x04u, 0x05u, 0x06u, 0x07u, 0x08u});
}

///@}

TEST_GROUP_BASE(CO_EmcyAllocation, CO_EmcyBase) {
  Allocators::Limited allocator;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    can_net_destroy(net);
    net = can_net_create(allocator.ToAllocT(), 0);

    CreateObj1001ErrorRegister(0u);
  }
};

/// @name co_emcy_get_alloc()
///@{

/// \Given pointers to an EMCY service (co_emcy_t) and a network (can_net_t)
///        with a memory allocator
///
/// \When co_emcy_get_alloc() is called
///
/// \Then a pointer to the memory allocator is returned
TEST(CO_EmcyAllocation, CoEmcyGetAlloc_Nominal) {
  co_emcy_t* emcy = co_emcy_create(net, dev);

  const alloc_t* alloc = co_emcy_get_alloc(emcy);

  POINTERS_EQUAL(allocator.ToAllocT(), alloc);

  co_emcy_destroy(emcy);
}

///@}

/// @name co_emcy_create()
///@{

/// \Given pointers to initialized device (co_dev_t) and network (can_net_t)
///        with a memory allocator limited to 0 bytes
///
/// \When co_emcy_create() is called with the pointers to the network and the
///       device
///
/// \Then a null pointer is returned and an EMCY service is not created
///       \Calls mem_alloc()
TEST(CO_EmcyAllocation, CoEmcyCreate_NoMemory) {
  allocator.LimitAllocationTo(0u);

  const auto* emcy = co_emcy_create(net, dev);

  POINTERS_EQUAL(nullptr, emcy);
}

/// \Given pointers to initialized device (co_dev_t) and network (can_net_t)
///        with a memory allocator limited to only allocate an EMCY service
///        instance
///
/// \When co_emcy_create() is called with the pointers to the network and the
///       device
///
/// \Then a null pointer is returned and an EMCY service is not created
///       \Calls mem_alloc()
///       \Calls co_dev_find_sub()
///       \Calls co_dev_find_obj()
///       \Calls can_buf_init()
///       \Calls can_timer_create()
TEST(CO_EmcyAllocation, CoEmcyCreate_MemoryOnlyForEmcy) {
  allocator.LimitAllocationTo(co_emcy_sizeof());

  const auto* emcy = co_emcy_create(net, dev);

  POINTERS_EQUAL(nullptr, emcy);
}

/// \Given pointers to initialized device (co_dev_t) and network (can_net_t),
///        the Emergency Consumer Object (0x1280) with at least one consumer
///        COB-ID defined and a memory allocator limited to only allocate
///        instances of an EMCY service and a timer (can_timer_t)
///
/// \When co_emcy_create() is called with the pointers to the network and the
///       device
///
/// \Then a null pointer is returned and an EMCY service is not created
///       \Calls mem_alloc()
///       \Calls co_dev_find_sub()
///       \Calls co_dev_find_obj()
///       \Calls can_buf_init()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_obj_find_sub()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls can_timer_destroy()
///       \Calls can_buf_fini()
TEST(CO_EmcyAllocation, CoEmcyCreate_MemoryOnlyForEmcyAndTimer) {
  CreateObj1028EmcyConsumerObject();
  allocator.LimitAllocationTo(co_emcy_sizeof() + can_timer_sizeof());

  const auto* emcy = co_emcy_create(net, dev);

  POINTERS_EQUAL(nullptr, emcy);
}

/// \Given pointers to initialized device (co_dev_t) and network (can_net_t),
///        the Emergency Consumer Object (0x1028) with at least one consumer
///        COB-ID configured and a memory allocator limited exactly to allocate
///        instances of an EMCY service, a timer (can_timer_t) and one frame
///        receiver (can_recv_t) for every consumer COB-ID
///
/// \When co_emcy_create() is called with the pointers to the network and the
///       device
///
/// \Then a pointer to a created EMCY service (co_emcy_t) is returned
///       \Calls mem_alloc()
///       \Calls co_dev_find_sub()
///       \Calls co_dev_find_obj()
///       \Calls can_buf_init()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_obj_find_sub()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
TEST(CO_EmcyAllocation, CoEmcyCreate_ExactMemory) {
  CreateObj1028EmcyConsumerObject();
  allocator.LimitAllocationTo(co_emcy_sizeof() + can_timer_sizeof() +
                              can_recv_sizeof());

  co_emcy_t* emcy = co_emcy_create(net, dev);

  CHECK(emcy != nullptr);

  co_emcy_destroy(emcy);
}

///@}
