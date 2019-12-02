#include "test.h"
#include <lely/ev/loop.hpp>
#include <lely/io2/sys/clock.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/co_can.hpp>
#include <lely/io2/vcan.hpp>

using namespace lely::ev;
using namespace lely::io;

#define NUM_OP 4

struct MyOp : public CoCanChannelRead {
  MyOp(CanChannelBase& chan_, uint_least8_t id_)
      : CoCanChannelRead(&msg, &err), chan(chan_), id(id_) {}

  void
  operator()(int result, ::std::error_code ec) noexcept override {
    co_reenter (*this) {
      tap_test(result == 0 && err.state == CAN_STATE_ACTIVE, "error frame");
      for (; n < NUM_OP; n++) {
        msg.id = id;
        chan.write(msg);
        co_yield chan.submit_read(*this);
        tap_test(result == 1 && !ec && msg.id != id, "CAN frame");
      }
    }
  }

  can_msg msg CAN_MSG_INIT;
  can_err err CAN_ERR_INIT;
  CanChannelBase& chan;
  uint_least8_t id{0};
  ::std::size_t n{0};
};

int
main() {
  tap_plan(4 + 2 * (NUM_OP + 1) + 1);

  IoGuard io_guard;
  Context ctx;
  Loop loop;
  VirtualCanController ctrl(clock_monotonic);
  VirtualCanChannel chan1(ctx, loop.get_executor());
  VirtualCanChannel chan2(ctx, loop.get_executor());

  chan1.open(ctrl);
  tap_test(chan1.is_open());

  chan2.open(ctrl);
  tap_test(chan2.is_open());

  ctrl.stop();
  tap_test(ctrl.stopped());
  ctrl.restart();
  tap_test(!ctrl.stopped());

  MyOp op1(chan1, 1);
  chan1.submit_read(op1);

  MyOp op2(chan2, 2);
  chan2.submit_read(op2);

  loop.run();
  tap_test(loop.stopped());

  return 0;
}
