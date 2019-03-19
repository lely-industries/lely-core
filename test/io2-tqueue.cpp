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
#include <lely/io2/tqueue.hpp>

using namespace lely::ev;
using namespace lely::io;

#define NUM_OP 4

int
main() {
  tap_plan(NUM_OP + 1);

  IoGuard io_guard;
  Context ctx;
  lely::io::Poll poll(ctx);
#if _WIN32
  // GetQueuedCompletionStatusEx() is not implemented in Wine < 4.0. Since the
  // Windows timer does not use I/O completion ports, we do not need to use the
  // polling instance in the event loop.
  Loop loop;
#else
  Loop loop(poll.get_poll());
#endif
  Timer timer(poll, loop.get_executor(), CLOCK_MONOTONIC);
  TimerQueue tq(timer, loop.get_executor());

  for (int i = 0; i < NUM_OP; i++)
    tq.submit_wait(::std::chrono::seconds(i), [&](::std::error_code ec) {
      if (!ec) {
        auto s = ::std::chrono::duration<double>(
                     timer.get_clock().gettime().time_since_epoch())
                     .count();
        tap_pass("%f s", s);
      }
    });

  loop.run();
  tap_test(loop.stopped());

  return 0;
}
