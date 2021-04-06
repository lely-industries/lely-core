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
  OnSync(uint8_t, const time_point&) noexcept override {
    uint32_t val = (*this)[0x2002][0];
    tap_diag("slave: sent PDO with value %d", val);

    val = (*this)[0x2001][0];
    tap_diag("slave: received PDO with value %d", val);
    // Echo the value back to the master on the next SYNC.
    (*this)[0x2002][0] = val;

#if !LELY_NO_CO_MPDO
    // Send the value with a SAM-MPDO.
    (*this)[0x2003][0] = val;
    (*this)[0x2003][0].SetEvent();
#endif
  }
};

class MyDriver : public FiberDriver {
 public:
  using FiberDriver::FiberDriver;

 private:
  void
  OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override {
    switch (idx) {
      case 0x2002: {
        tap_test(subidx == 0, "master: received object 2002:00");
        uint32_t val = rpdo_mapped[idx][subidx];
        tap_test(val == (n_ > 3 ? n_ - 3 : 0));
        break;
      }
#if !LELY_NO_CO_MPDO
      case 0x2003:
        uint32_t val = rpdo_mapped[idx][subidx];
        tap_diag("master: received object 2003:00: %d", val);
        break;
#endif
    }
  }

  void
  OnBoot(NmtState, char es, const ::std::string&) noexcept override {
    tap_test(!es, "master: slave #%d successfully booted", id());
    // Start SYNC production.
    master[0x1006][0] = UINT32_C(1000000);
  }

  void
  OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
    try {
      tap_pass("master: configuring slave #%d", id());

      Wait(AsyncWrite<::std::string>(0x2000, 0, "Hello, world!"));
      auto value = Wait(AsyncRead<::std::string>(0x2000, 0));
      tap_test(value == "Hello, world!");

      // Sleep for 100 ms before reporting success.
      USleep(100000);
      res({});
    } catch (SdoError& e) {
      res(e.code());
    }
  }

  void
  OnDeconfig(
      ::std::function<void(::std::error_code ec)> res) noexcept override {
    tap_pass("master: deconfiguring slave #%d", id());

    // Sleep for 100 ms before reporting success.
    USleep(100000);
    res({});
  }

  void
  OnSync(uint8_t cnt, const time_point&) noexcept override {
    tap_pass("master: sent SYNC #%d", cnt);

    // Object 2001:00 on the slave was updated by a PDO from the master.
    uint32_t val = tpdo_mapped[0x2001][0];
    tap_diag("master: sent PDO with value %d", val);
    // Increment the value for the next SYNC.
    tpdo_mapped[0x2001][0] = ++val;

    // Initiate a clean shutdown.
    if (++n_ >= NUM_OP)
      master.AsyncDeconfig(id()).submit(
          GetExecutor(), [&]() { master.GetContext().shutdown(); });
  }

  uint32_t n_{0};
};

int
main() {
  tap_plan(2 + 3 + NUM_OP + 2 * (NUM_OP - 1) + 1);

  IoGuard io_guard;
  Context ctx;
  lely::io::Poll poll(ctx);
  Loop loop(poll.get_poll());
  auto exec = loop.get_executor();
  VirtualCanController ctrl(clock_monotonic);

  Timer stimer(poll, exec, CLOCK_MONOTONIC);
  VirtualCanChannel schan(ctx, exec);
  schan.open(ctrl);
  tap_test(schan.is_open(), "slave: opened virtual CAN channel");
  MySlave slave(stimer, schan, TEST_SRCDIR "/coapp-fiber-slave.dcf", "", 127);

  Timer mtimer(poll, exec, CLOCK_MONOTONIC);
  VirtualCanChannel mchan(ctx, exec);
  mchan.open(ctrl);
  tap_test(mchan.is_open(), "master: opened virtual CAN channel");
  AsyncMaster master(mtimer, mchan, TEST_SRCDIR "/coapp-fiber-master.dcf", "",
                     1);
  MyDriver driver(exec, master, 127);

  slave.Reset();
  master.Reset();

  loop.run();

  return 0;
}
