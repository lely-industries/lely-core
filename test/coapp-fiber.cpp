#include "test.h"
#include <lely/coapp/fiber_driver.hpp>
#include <lely/coapp/slave.hpp>
#include <lely/ev/loop.hpp>
#if _WIN32
#include <lely/io2/win32/poll.hpp>
#elif _POSIX_C_SOURCE >= 200112L
#include <lely/io2/posix/poll.hpp>
#else
#error This file requires Windows or POSIX.
#endif
#include <lely/io2/sys/clock.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/io2/vcan.hpp>

using namespace lely::ev;
using namespace lely::io;
using namespace lely::canopen;

#define NUM_OP 8

class MySlave : public BasicSlave {
 public:
  using BasicSlave::BasicSlave;

 private:
  void
  OnSync(uint8_t cnt, const time_point&) noexcept override {
    tap_pass("slave: received SYNC #%d", cnt);

    uint32_t val = (*this)[0x2002][0];
    tap_diag("slave: sent PDO with value %d", val);

    val = (*this)[0x2001][0];
    tap_diag("slave: received PDO with value %d", val);
    // Echo the value back to the master on the next SYNC.
    (*this)[0x2002][0] = val;
  }
};

class MyDriver : public FiberDriver {
 public:
  using FiberDriver::FiberDriver;

 private:
  void
  OnBoot(NmtState, char es, const ::std::string&) noexcept override {
    tap_test(!es, "master: slave #%d successfully booted", id());
  }

  void
  OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
    try {
      tap_pass("master: configuring slave #%d", id());

      Wait(AsyncWrite<::std::string>(0x2000, 0, "Hello, world!"));
      auto value = Wait(AsyncRead<::std::string>(0x2000, 0));
      tap_test(value == "Hello, world!");

      res({});
    } catch (SdoError& e) {
      res(e.code());
    }
  }

  void
  OnDeconfig(
      ::std::function<void(::std::error_code ec)> res) noexcept override {
    tap_pass("master: deconfiguring slave #%d", id());
    res({});
  }

  void
  OnSync(uint8_t cnt, const time_point&) noexcept override {
    tap_pass("master: sent SYNC #%d", cnt);

    uint32_t val;
    val = master[0x2001][0];
    tap_diag("master: sent PDO with value %d", val);
    // Increment the value for the next SYNC.
    master[0x2001][0] = ++val;

    // Object 2001 was just updated by a PDO from the slave.
    val = master[0x2002][0];
    tap_diag("master: received PDO with value %d", val);
    tap_test(val == (n_ > 3 ? n_ - 3 : 0));

    // Initiate a clean shutdown.
    if (++n_ >= NUM_OP)
      master.AsyncDeconfig(id()).submit(
          GetExecutor(), [&]() { master.GetContext().shutdown(); });
  }

  uint32_t n_{0};
};

int
main() {
  tap_plan(6 + 3 * NUM_OP);

  IoGuard io_guard;
  Context ctx;
  lely::io::Poll poll(ctx);
  Loop loop(poll.get_poll());
  auto exec = loop.get_executor();
  Timer timer(poll, exec, CLOCK_MONOTONIC);
  VirtualCanController ctrl(clock_monotonic);

  VirtualCanChannel schan(ctx, exec);
  schan.open(ctrl);
  tap_test(schan.is_open());
  MySlave slave(timer, schan, TEST_SRCDIR "/coapp-fiber-slave.dcf", "", 127);

  VirtualCanChannel mchan(ctx, exec);
  mchan.open(ctrl);
  tap_test(mchan.is_open());
  AsyncMaster master(timer, mchan, TEST_SRCDIR "/coapp-fiber-master.dcf", "",
                     1);
  MyDriver driver(exec, master, 127);

  slave.Reset();
  master.Reset();

  loop.run();

  return 0;
}
