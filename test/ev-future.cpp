#include "test.h"
#include <lely/ev/future.hpp>
#include <lely/ev/thrd_loop.hpp>

using namespace lely::ev;

int
main() {
  tap_plan(8);

  auto exec = ThreadLoop::get_executor();

  Promise<int> p;

  auto f1 = p.get_future();
  tap_test(!f1.is_ready());

  auto f2 = when_any(exec, f1);
  tap_test(!f2.is_ready());

  p.set(42);
  tap_test(f1.is_ready());
  tap_test(f1.get().has_value());
  tap_test(f1.get().value() == 42);

  ThreadLoop::run();
  tap_test(ThreadLoop::stopped());

  tap_test(f2.is_ready());
  tap_test(f2.get().value() == 0);

  return 0;
}
