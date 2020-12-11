#include "test.h"
#include <lely/util/fiber.hpp>

using namespace lely::util;

int
main() {
  tap_plan(3);

#if __MINGW32__ && !_WIN64
  // On 32-bit platforms, either MinGW or Wine causes terminate to be called if
  // an exception is thrown from Fiber::resume(). This prevents the use of
  // fiber_unwind and therefore the proper destruction of fibers.
  for (int i = 0; i < 3; i++) tap_skip();
#else
  FiberThread thrd;

  Fiber f1, f2, f3;
  f3 = Fiber{[&](Fiber&& f) -> Fiber {
    f2 = ::std::move(f);
    for (;;) {
      tap_pass("in f3");
      f2 = ::std::move(f1).resume();
    }
    return {};
  }};
  f2 = Fiber{[&](Fiber&& f) -> Fiber {
    f1 = ::std::move(f);
    for (;;) {
      tap_pass("in f2");
      f1 = ::std::move(f3).resume();
    }
    return {};
  }};
  f1 = Fiber{[&](Fiber&& f) -> Fiber {
    tap_pass("in f1");
    f3 = ::std::move(f2).resume();
    tap_diag("exiting f1");
    return ::std::move(f);
  }};
  ::std::move(f1).resume();
  tap_diag("back in main()");
#endif

  return 0;
}
