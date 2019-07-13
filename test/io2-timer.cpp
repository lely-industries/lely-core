#include "test.h"
#include <lely/ev/loop.hpp>
#if _WIN32
#include <lely/io2/win32/poll.hpp>
#elif _POSIX_C_SOURCE >= 200112L
#include <lely/io2/posix/poll.hpp>
#else
#error This file requires Windows or POSIX.
#endif
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/io2/co_timer.hpp>

using namespace lely::ev;
using namespace lely::io;

#define NUM_OP 4

struct MyOp : public CoTimerWait {
  MyOp(TimerBase& timer_) : timer(timer_) {}

  void
  operator()(int overrun, ::std::error_code ec) noexcept override {
    co_reenter (*this) {
      for (; n < NUM_OP; n++) {
        if (!ec) {
          auto s = ::std::chrono::duration<double>(
                       timer.get_clock().gettime().time_since_epoch())
                       .count();
          tap_pass("%f s (%d)", s, overrun);
        }
        co_yield timer.submit_wait(*this);
      }
    }
  }

  TimerBase& timer;
  ::std::size_t n{0};
};

int
main() {
  tap_plan(NUM_OP + 1);

  IoGuard io_guard;
  Context ctx;
  lely::io::Poll poll(ctx);
#if _WIN32
  // GetQueuedCompletionStatusEx() is not implemented in Wine. Since the Windows
  // timer does not use I/O completion ports, we do not need to use the polling
  // instance in the event loop.
  Loop loop;
#else
  Loop loop(poll.get_poll());
#endif
  Timer timer(poll, loop.get_executor(), CLOCK_MONOTONIC);

  timer.settime(::std::chrono::seconds(1), ::std::chrono::seconds(1));

  MyOp op(timer);
  timer.submit_wait(op);

  loop.run();
  tap_test(loop.stopped());

  return 0;
}
