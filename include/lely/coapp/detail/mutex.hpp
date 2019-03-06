/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the mutual exclusion facilities.
 *
 * @copyright 2018-2019 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LELY_COAPP_DETAIL_MUTEX_H_
#define LELY_COAPP_DETAIL_MUTEX_H_

#include <lely/coapp/coapp.hpp>

#include <mutex>

namespace lely {

namespace canopen {

namespace detail {

/**
 * A mutex wrapper that provides a convenient RAII-style mechanism for releasing
 * a mutex for the duration of a scoped block. When an `UnlockGuard` object is
 * created, it attempts to release ownership of the mutex it is given. When
 * control leaves the scope in which the `UnlockGuard` object was created, the
 * `UnlockGuard` is destructed and the mutex reacquired.
 */
template <class Mutex>
class UnlockGuard {
 public:
  /**
   * The type of the mutex to unlock. The type must meet the BasicLockable
   * requirements.
   */
  typedef Mutex MutexType;

  /**
   * Releases ownership of <b>m</b> and calls `m.unlock()`. The behavior is
   * undefined if the current thread does not own <b>m</b>.
   */
  explicit UnlockGuard(MutexType& m) : m_(m) { m_.unlock(); }

  /**
   * Releases ownership of <b>m</b> without attempting to unlock it. The
   * behavior is undefined if the current thread already owns <b>m</b>.
   */
  UnlockGuard(MutexType& m, ::std::adopt_lock_t) : m_(m) {}

  UnlockGuard(const UnlockGuard&) = delete;

  UnlockGuard& operator=(const UnlockGuard&) = delete;

  /**
   * Acquires ownership of <b>m</b> and calls `m.lock()`, where <b>m</b> is the
   * mutex passed to the constructor.
   */
  ~UnlockGuard() { m_.lock(); }

 private:
  MutexType& m_;
};

}  // namespace detail

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_DETAIL_MUTEX_H_
