#include "test.h"
#include <lely/util/stop.hpp>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

using namespace lely::util;

int
main() {
  tap_plan(7);

  StopSource source;
  tap_test(source.stop_possible());
  tap_test(!source.stop_requested());

  auto token = source.get_token();
  tap_test(token.stop_possible());
  tap_test(!token.stop_requested());
#if __MINGW32__
  // Threads are not supported on MinGW.
  tap_skip();
#else
  ::std::mutex mtx;
  ::std::condition_variable cond;

  ::std::thread thr([token, &mtx, &cond]() {
    ::std::unique_lock<::std::mutex> lock(mtx);
    while (!token.stop_requested()) cond.wait(lock);
  });

  {
    StopCallback<::std::function<void()>> callback(
        token, [source, &mtx, &cond]() mutable {
          tap_test(source.stop_requested());
          ::std::lock_guard<::std::mutex> lock(mtx);
          cond.notify_one();
        });
#endif
  // clang-format off
    token = {};
    tap_test(!token.stop_possible());
#if !__MINGW32__
    ::std::this_thread::sleep_for(::std::chrono::seconds(1));
#endif
    source.request_stop();
    tap_test(source.stop_requested());
#if !__MINGW32__
  }
  thr.join();
// clang-format on
#endif

return 0;
}
