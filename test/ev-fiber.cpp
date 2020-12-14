#include "test.h"
#include <lely/ev/fiber_exec.hpp>
#include <lely/ev/thrd_loop.hpp>

#include <chrono>

using namespace lely::ev;

#define NUM_YIELD (8 * 1024 * 1024)

int
main() {
  tap_plan(5);

  FiberThread thrd;
  FiberExecutor exec(ThreadLoop::get_executor());
  FiberMutex mtx;
  FiberConditionVariable cond;

  Promise<int> p;
  auto f = p.get_future();

  exec.post([&]() {
    tap_diag("1: before lock");
    {
      ::std::unique_lock<FiberMutex> lock(mtx);
      fiber_yield();
    }
    tap_diag("1: after unlock, before yield");
    fiber_yield();
    tap_diag("1: after yield, before wait");
    {
      ::std::unique_lock<FiberMutex> lock(mtx);
      cond.wait(lock);
    }
    tap_diag("1: after wait, before future await");
    fiber_await(f);
    tap_diag("1: after future await");
    tap_test(f.is_ready());
    tap_test(f.get().value() == 42);
  });

  exec.post([&]() {
    tap_diag("2: before lock");
    { ::std::unique_lock<FiberMutex> lock(mtx); }
    tap_diag("2: after unlock, before yield");
    fiber_yield();
    tap_diag("2: after yield, before notify");
    {
      ::std::unique_lock<FiberMutex> lock(mtx);  // optional
      cond.notify_one();
    }
    fiber_yield();
    tap_diag("2: after yield, before promise set");
    p.set(42);
    fiber_yield();
    tap_diag("2: after yield");
  });

  ThreadLoop::run();
  tap_test(ThreadLoop::stopped());
  ThreadLoop::restart();
  tap_test(!ThreadLoop::stopped());

  exec.post([&]() {
    for (::std::size_t i = 0; i < NUM_YIELD; i++) fiber_yield();
  });

  exec.post([&]() {
    for (::std::size_t i = 0; i < NUM_YIELD; i++) fiber_yield();
  });

  auto t1 = ::std::chrono::high_resolution_clock::now();
  ThreadLoop::run();
  auto t2 = ::std::chrono::high_resolution_clock::now();
  tap_test(ThreadLoop::stopped());

  auto ns = ::std::chrono::nanoseconds(t2 - t1).count();
  tap_diag("%f ns per switch", double(ns) / (NUM_YIELD * 2));

  return 0;
}
