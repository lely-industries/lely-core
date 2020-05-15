
#include <CppUTest/TestHarness.h>
#include <lely/util/mutex.hpp>

TEST_GROUP(UtilMutex) {
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

TEST(UtilMutex, UnlockGuard_LocksAndUnlocksMutex) {
  { lely::util::UnlockGuard<Mutex> unlock_guard_mutex(m); }
  CHECK_EQUAL(true, m.lock_was_called);
  CHECK_EQUAL(true, m.unlock_was_called);
}
