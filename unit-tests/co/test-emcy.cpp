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

#include <lely/co/emcy.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

struct EmcyInd {
  static void
  func(co_emcy_t*, co_unsigned8_t, co_unsigned16_t, co_unsigned8_t,
       co_unsigned8_t*, void*) {}
};

TEST_BASE(CO_EmcyBase) {
  TEST_BASE_SUPER(CO_EmcyBase);

  Allocators::Default allocator;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1001;
  std::unique_ptr<CoObjTHolder> obj1003;
  std::unique_ptr<CoObjTHolder> obj1028;
  const co_unsigned8_t DEV_ID = 0x01u;

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

  void CreateObj1003EmptyPredefinedErrorField() {
    CreateObjInDev(obj1003, 0x1003u);
    co_obj_set_code(obj1003->Get(), CO_OBJECT_ARRAY);
    obj1003->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0u});
    obj1003->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});
  }

  void CreateObj1028EmcyConsumerObject() {
    CreateObjInDev(obj1028, 0x1028u);
    co_obj_set_code(obj1028->Get(), CO_OBJECT_ARRAY);
    obj1028->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{1u});
    obj1028->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});
  }

  void CheckDefaultEmcyParams(const co_emcy_t* emcy) {
    POINTERS_EQUAL(net, co_emcy_get_net(emcy));
    POINTERS_EQUAL(dev, co_emcy_get_dev(emcy));

    CheckDefaultIndicator(emcy);
  }

  void CheckDefaultIndicator(const co_emcy_t* emcy) {
    int dummy_data = 42;
    co_emcy_ind_t* ind = &EmcyInd::func;
    void* data = &dummy_data;

    co_emcy_get_ind(emcy, &ind, &data);
    POINTERS_EQUAL(nullptr, ind);
    POINTERS_EQUAL(nullptr, data);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);
  }

  TEST_TEARDOWN() {
    can_net_destroy(net);
    dev_holder.reset();
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
///       else if \__MINGW32__ and !__MINGW64__: 1368 is returned;
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
  CHECK_EQUAL(1368u, size);
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
///        the Error Register object (0x1001) present in the object dictionary
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
///        the Error Register object (0x1001), the Pre-defined Error Field
///        object (0x1003) and the Emergency Consumer Object (0x1028) present in
///        the object dictionary
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
  CreateObj1003EmptyPredefinedErrorField();
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

TEST_GROUP_BASE(CO_Emcy, CO_EmcyBase) {
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
TEST(CO_Emcy, CoEmcyGetDev_Nominal) {
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
TEST(CO_Emcy, CoEmcyGetNet_Nominal) {
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
TEST(CO_Emcy, CoEmcyGetInd_NullPointers) {
  co_emcy_get_ind(emcy, nullptr, nullptr);
}

/// \Given a pointer to an EMCY service (co_emcy_t)
///
/// \When co_emcy_get_ind() is called with pointers to store an indication
///       function and user-specified data
/// \Then the pointers to an indication function and data are set to null
TEST(CO_Emcy, CoEmcyGetInd_DefaultNull) { CheckDefaultIndicator(emcy); }

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
TEST(CO_Emcy, CoEmcySetInd_Nominal) {
  int data = 42;

  co_emcy_set_ind(emcy, &EmcyInd::func, &data);

  co_emcy_ind_t* ind = nullptr;
  void* user_data = nullptr;
  co_emcy_get_ind(emcy, &ind, &user_data);
  POINTERS_EQUAL(&EmcyInd::func, ind);
  POINTERS_EQUAL(&data, user_data);
}

///@}

TEST_GROUP_BASE(CO_EmcyAllocation, CO_EmcyBase) {
  Allocators::Limited allocator;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    can_net_destroy(net);
    net = can_net_create(allocator.ToAllocT());

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
