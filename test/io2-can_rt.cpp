#include "test.h"
#include <lely/ev/loop.hpp>
#include <lely/io2/can_rt.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/user/can.hpp>

using namespace lely::ev;
using namespace lely::io;

#define NUM_OP 4

int
main() {
  tap_plan(NUM_OP + 1);

  IoGuard io_guard;
  Context ctx;
  Loop loop;
  UserCanChannel chan(ctx, loop.get_executor());
  CanRouter rt(chan, loop.get_executor());

  for (uint_least32_t i = 0; i < NUM_OP; i++)
    rt.submit_read_frame(i, CanFlag::NONE,
                         [=](const can_msg* msg, ::std::error_code ec) {
                           if (!ec) tap_test(msg->id == i, "%03x", i);
                         });

  for (uint_least32_t i = 0; i < NUM_OP; i++) {
    can_msg msg CAN_MSG_INIT;
    msg.id = i;
    chan.on_read(&msg);
  }

  loop.run();
  tap_test(loop.stopped());

  return 0;
}
