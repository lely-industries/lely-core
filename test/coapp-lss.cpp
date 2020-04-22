#include "test.h"
#include <lely/coapp/lss_master.hpp>
#include <lely/coapp/master.hpp>
#include <lely/coapp/slave.hpp>
#include <lely/ev/fiber_exec.hpp>
#include <lely/ev/loop.hpp>
#include <lely/ev/strand.hpp>
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

namespace lely {
namespace canopen {

namespace detail {

class FiberLssMasterBase {
 protected:
  FiberLssMasterBase(ev_exec_t* exec_)
      : thrd(ev::FiberFlag::SAVE_ERROR), exec(exec_), strand(exec) {}

  ev::FiberThread thrd;
  ev::FiberExecutor exec;
  ev::Strand strand;
};

}  // namespace detail

class FiberLssMaster : detail::FiberLssMasterBase, public LssMaster {
 public:
  FiberLssMaster(ev_exec_t* exec, Node& node,
                 io::CanControllerBase* ctrl = nullptr)
      : FiberLssMasterBase(exec ? exec
                                : static_cast<ev_exec_t*>(node.GetExecutor())),
        LssMaster(FiberLssMasterBase::exec, node, ctrl) {}

  FiberLssMaster(Node& node, io::CanControllerBase* ctrl = nullptr)
      : FiberLssMaster(nullptr, node, ctrl) {}

  template <class T, class E>
  T
  Wait(ev::Future<T, E> f) {
    fiber_await(f);
    return f.get().value();
  }
};

}  // namespace canopen
}  // namespace lely

using namespace lely::ev;
using namespace lely::io;
using namespace lely::canopen;

class MyLssMaster : public FiberLssMaster {
 public:
  MyLssMaster(BasicMaster& master) : FiberLssMaster(master) {}

 private:
  void
  OnStart(::std::function<void(::std::error_code ec)> res) noexcept override {
    auto& master = static_cast<BasicMaster&>(GetNode());

    try {
      // Wait for the slave to finish restarting.
      Wait(GetNode().AsyncWait(::std::chrono::milliseconds(100)));

      LssAddress lo{0x360, 0x2, 0, 0};
      LssAddress hi{0x360, 0x2, 0xffffffff, 0xffffffff};
      LssAddress address = Wait(AsyncSlowscan(lo, hi));
      tap_test(address.vendor_id == 0x360, "Slowscan: vendor-ID");
      tap_test(address.product_code == 0x2, "Slowscan: product-code");
      tap_test(address.revision == 0x3, "Slowscan: revision-number");
      tap_test(address.serial_nr == 0x4, "Slowscan: serial-number");
      Wait(AsyncSwitch());

      address = Wait(AsyncFastscan());
      tap_test(address.vendor_id == 0x360, "Fastscan: vendor-ID");
      tap_test(address.product_code == 0x2, "Fastscan: product-code");
      tap_test(address.revision == 0x3, "Fastscan: revision-number");
      tap_test(address.serial_nr == 0x4, "Fastscan: serial-number");
      Wait(AsyncSwitch());

      Wait(AsyncSwitchSelective(address));

      tap_test(Wait(AsyncGetVendorId()) == 0x360, "inquire vendor-ID");
      tap_test(Wait(AsyncGetProductCode()) == 0x2, "inquire product-code");
      tap_test(Wait(AsyncGetRevision()) == 0x3, "inquire revision-number");
      tap_test(Wait(AsyncGetSerialNr()) == 0x4, "inquire serial-number");

      tap_test(Wait(AsyncGetId()) == 2, "inquire node-ID");
      Wait(AsyncSetId(3));
      try {
        Wait(AsyncStore());
        tap_todo(true, "store configuration");
      } catch (::std::system_error& e) {
        tap_todo(false, "store configuration");
      }
      tap_test(Wait(AsyncGetId()) == 2, "inquire node-ID");

      Wait(AsyncSwitch());

      master.Command(NmtCommand::RESET_COMM, 2);
      Wait(GetNode().AsyncWait(::std::chrono::milliseconds(100)));

      tap_test(Wait(AsyncIdNonConfig()) == false,
               "identify non-configured remote slave");

      Wait(AsyncSwitchSelective(address));
      tap_test(Wait(AsyncGetId()) == 3, "inquire node-ID");
      Wait(AsyncSetId(0xff));
      Wait(AsyncSwitch());

      master.Command(NmtCommand::RESET_COMM, 3);
      Wait(GetNode().AsyncWait(::std::chrono::milliseconds(100)));

      tap_test(Wait(AsyncIdNonConfig()) == true,
               "identify non-configured remote slave");

      res({});
    } catch (::std::system_error& e) {
      tap_abort("exception thrown with error code %d", e.code().value());
      res(e.code());
    }

    master.GetContext().shutdown();
  }
};

int
main() {
  tap_plan(2 + 4 + 4 + 4 + 3 + 1 + 1 + 1);

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
  BasicSlave slave(stimer, schan, TEST_SRCDIR "/coapp-lss-slave.dcf", "", 2);

  Timer mtimer(poll, exec, CLOCK_MONOTONIC);
  VirtualCanChannel mchan(ctx, exec);
  mchan.open(ctrl);
  tap_test(mchan.is_open(), "master: opened virtual CAN channel");
  BasicMaster master(mtimer, mchan, TEST_SRCDIR "/coapp-lss-master.dcf", "", 1);

  MyLssMaster lss_master(master);
  // Reduce the LSS timeouts to speed up the test.
  lss_master.SetInhibit(::std::chrono::milliseconds(0));
  lss_master.SetTimeout(::std::chrono::milliseconds(10));

  slave.Reset();
  master.Reset();

  loop.run();

  return 0;
}
