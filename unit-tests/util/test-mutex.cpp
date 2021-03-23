/**file
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

#include <CppUTest/TestHarness.h>

#include <lely/util/mutex.hpp>

#if !LELY_NO_CXX

TEST_GROUP(Util_Mutex) {
  class Mutex : lely::util::BasicLockable {
   public:
    bool lock_was_called = false;
    bool unlock_was_called = false;

    void
    lock() override {
      lock_was_called = true;
    }
    void
    unlock() override {
      unlock_was_called = true;
    }
  };

  Mutex m;
};

/// @name unlock guard creation and destruction
///@{

/// \Given a mutex conforming to the BasicLockable concept
///
/// \When UnlockGuard<Mutex> is created with the mutex and destroyed
///
/// \Then unlock is called after the creation, lock is called after
///       the destruction
TEST(Util_Mutex, UnlockGuard_LocksAndUnlocksMutex) {
  {
    lely::util::UnlockGuard<Mutex> unlock_guard_mutex(m);

    CHECK_EQUAL(true, m.unlock_was_called);
    CHECK_EQUAL(false, m.lock_was_called);
  }

  CHECK_EQUAL(true, m.lock_was_called);
}

///@}

#endif  // !LELY_NO_CXX
