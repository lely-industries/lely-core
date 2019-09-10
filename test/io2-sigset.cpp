#include "test.h"
#include <lely/ev/loop.hpp>
#if _POSIX_C_SOURCE >= 200112L
#include <lely/io2/posix/poll.hpp>
#else
#error This file requires POSIX.
#endif
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/sigset.hpp>
#include <lely/io2/co_sigset.hpp>

using namespace lely::ev;
using namespace lely::io;

#define NUM_OP 4

struct MyOp : public CoSignalSetWait {
  MyOp(SignalSetBase& sigset_) : sigset(sigset_) {}

  void
  operator()(int signo) noexcept override {
    co_reenter (*this) {
      for (; n < NUM_OP; n++) {
        tap_pass();
        raise(signo);
        co_yield sigset.submit_wait(*this);
      }
    }
  }

  SignalSetBase& sigset;
  ::std::size_t n{0};
};

int
main() {
  tap_plan(1 + NUM_OP);

  IoGuard io_guard;
  Context ctx;
  lely::io::Poll poll(ctx);
  Loop loop(poll.get_poll());
  SignalSet sigset(poll, loop.get_executor());

  sigset.insert(SIGRTMIN);
  raise(SIGRTMIN);

  MyOp op(sigset);
  sigset.submit_wait(op);

  loop.run();
  tap_test(loop.stopped());

  return 0;
}
