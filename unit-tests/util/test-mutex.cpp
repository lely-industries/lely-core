
#include <CppUTest/TestHarness.h>
#include <lely/util/mutex.hpp>

typedef enum {
	Function_status_NOT_CALLED = 0,
	Function_status_CALLED = 1,
} Function_status;

class Mutex : lely::util::BasicLockable {
	public:
		static int lock_was_called;
		static int unlock_was_called;
		void lock();
		void unlock();
};

int Mutex::lock_was_called = Function_status_NOT_CALLED;
int Mutex::unlock_was_called = Function_status_NOT_CALLED;
void Mutex::lock() { 
	lock_was_called = Function_status_CALLED; 
}
void Mutex::unlock() { 
	unlock_was_called = Function_status_CALLED; 
}

TEST_GROUP(UtilMutexGroup){
	void teardown() {
		Mutex::lock_was_called = Function_status_NOT_CALLED;
		Mutex::unlock_was_called = Function_status_NOT_CALLED;
	}
};

TEST(UtilMutexGroup, mutexNotCreatedSoNotCalled) {
	LONGS_EQUAL(Function_status_NOT_CALLED, Mutex::lock_was_called);
	LONGS_EQUAL(Function_status_NOT_CALLED, Mutex::unlock_was_called);
}

TEST(UtilMutexGroup, mutexConstructorDoesNotLockAndUnlock) {
	Mutex m;
	LONGS_EQUAL(Function_status_NOT_CALLED, Mutex::lock_was_called);
	LONGS_EQUAL(Function_status_NOT_CALLED, Mutex::unlock_was_called);
}

TEST(UtilMutexGroup, unlockGuardLocksAndUnlocksMutex) {
	Mutex m;
	{ 
		lely::util::UnlockGuard<Mutex> unlock_guard_mutex(m);
	}
	LONGS_EQUAL(Function_status_CALLED, Mutex::lock_was_called);
	LONGS_EQUAL(Function_status_CALLED, Mutex::unlock_was_called);
}
