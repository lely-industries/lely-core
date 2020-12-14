#include "test.h"
#include <lely/ev/loop.hpp>
#include <lely/ev/co_task.hpp>

using namespace lely::ev;

#define NUM_OP (16 * 1024 * 1024)

class MyOp : public CoTask {
 public:
  virtual void
  operator()() noexcept override {
    co_reenter (*this) {
      for (; n < NUM_OP; n++) co_yield get_executor().post((ev_task&)(*this));
    }
  }

 private:
  ::std::size_t n{0};
};

int
main() {
  tap_plan(3);

  Loop loop;
  tap_test(!loop.stopped());

  MyOp op;
  loop.get_executor().post(op);

  auto t1 = ::std::chrono::high_resolution_clock::now();
  auto nop = loop.run();
  auto t2 = ::std::chrono::high_resolution_clock::now();

  tap_test(nop == NUM_OP + 1);
  tap_test(loop.stopped());

  auto ns = ::std::chrono::nanoseconds(t2 - t1).count();
  tap_diag("%f ns per op", double(ns) / nop);

  return 0;
}
