/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the futures and promises.
 *
 * @see lely/ev/future.h
 *
 * @copyright 2018-2022 Lely Industries N.V.
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

#ifndef LELY_EV_FUTURE_HPP_
#define LELY_EV_FUTURE_HPP_

#include <lely/ev/exec.hpp>
#include <lely/ev/future.h>
#include <lely/libc/type_traits.hpp>
#include <lely/util/invoker.hpp>
#include <lely/util/result.hpp>

#include <utility>
#include <vector>

namespace lely {
namespace ev {

/**
 * The exception thrown when retrieving the result of a future which is not
 * ready or does not contain a reference to a shared state.
 *
 * @see Future<T, E>::get()
 */
class future_not_ready : public ::std::runtime_error {
  using ::std::runtime_error::runtime_error;
};

template <class, class = ::std::error_code>
class Future;

/**
 * A promise. <b>T</b> and <b>E</b> should be nothrow default-constructible. Any
 * exceptions thrown during the default construction of the shared state will
 * result in a call to std::terminate().
 *
 * @see ev_promise_t
 */
template <class T, class E = ::std::error_code>
class Promise {
 public:
  /// The type of the result stored in the shared state.
  using result_type = util::Result<T, E>;

  /**
   * Constructs a promise with (a reference to) an empty shared state.
   *
   * @see ev_promise_create()
   */
  Promise()
      : promise_(ev_promise_create(sizeof(result_type), [](void* ptr) noexcept {
          static_cast<result_type*>(ptr)->~result_type();
        })) {
    if (!promise_) util::throw_errc("Promise");
    // Since we already registered the destructor for result_type(), we cannot
    // handle an exception from the default constructor. Wrapping the placement
    // new in a noexcept lambda prevents undefined behavior.
    [](void* ptr) noexcept {
      new (ptr) result_type();
    }(ev_promise_data(promise_));
  }

  explicit Promise(ev_promise_t* promise) noexcept : promise_(promise) {}

  /// Constructs a promise with a reference to the shared state of <b>other</b>.
  Promise(const Promise& other) noexcept
      : promise_(ev_promise_acquire(other.promise_)) {}

  /**
   * Moves the shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   */
  Promise(Promise&& other) noexcept : promise_(other.promise_) {
    other.promise_ = nullptr;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Promise(), then creates a
   * reference to the shared state of <b>other</b>.
   *
   * @returns `*this`.
   */
  Promise&
  operator=(const Promise& other) noexcept {
    if (promise_ != other.promise_) {
      ev_promise_release(promise_);
      promise_ = ev_promise_acquire(other.promise_);
    }
    return *this;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Promise(), then moves the
   * shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   *
   * @returns `*this`;
   */
  Promise&
  operator=(Promise&& other) noexcept {
    using ::std::swap;
    swap(promise_, other.promise_);
    return *this;
  }

  /**
   * Abandons the shared state. If no references remain to the shared state, it
   * is destroyed.
   *
   * @see ev_promise_release()
   */
  ~Promise() { ev_promise_release(promise_); }

  operator ev_promise_t*() const noexcept { return promise_; }

  /// Checks whether `*this` contains (a reference to) a shared state.
  explicit operator bool() const noexcept { return promise_ != nullptr; }

  /**
   * Checks whether `*this` contains a unique reference to its shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @see ev_promise_is_unique()
   */
  bool
  is_unique() const noexcept {
    return ev_promise_is_unique(*this) != 0;
  }

  /**
   * Satisfies a promise, if it was not aready satisfied, and stores the
   * specified value as the result in the shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @returns true if the promise is successfully satisfied, and false if it was
   * already satisfied.
   *
   * @see ev_promise_set()
   */
  template <class U>
  bool
  set(U&& u) {
    if (ev_promise_set_acquire(*this)) {
      auto p = reinterpret_cast<result_type*>(ev_promise_data(*this));
      try {
        *p = result_type{::std::forward<U>(u)};
        ev_promise_set_release(*this, static_cast<void*>(p));
      } catch (...) {
        ev_promise_set_release(*this, nullptr);
        throw;
      }
      return true;
    }
    return false;
  }

  /**
   * Returns a #lely::ev::Future with (a reference to) the same shared state as
   * `*this`.
   *
   * @pre #operator bool() returns true.
   */
  Future<T, E>
  get_future() const noexcept {
    return Future<T, E>(ev_promise_get_future(*this));
  }

 private:
  ev_promise_t* promise_{nullptr};
};

namespace detail {

template <class F, class... Args, class R = compat::invoke_result_t<F, Args...>>
inline typename ::std::enable_if<!::std::is_void<R>::value,
                                 util::Result<R, ::std::exception_ptr>>::type
catch_result(F&& f, Args&&... args) {
  try {
    return util::success(
        compat::invoke(::std::forward<F>(f), ::std::forward<Args>(args)...));
  } catch (...) {
    return util::failure(::std::current_exception());
  }
}

template <class F, class... Args, class R = compat::invoke_result_t<F, Args...>>
inline typename ::std::enable_if<::std::is_void<R>::value,
                                 util::Result<R, ::std::exception_ptr>>::type
catch_result(F&& f, Args&&... args) {
  try {
    compat::invoke(::std::forward<F>(f), ::std::forward<Args>(args)...);
    return util::success();
  } catch (...) {
    return util::failure(::std::current_exception());
  }
}

template <class>
struct is_future : ::std::false_type {};

template <class T, class E>
struct is_future<Future<T, E>> : ::std::true_type {};

template <class, class = void>
class AsyncTask;

template <class Invoker>
class AsyncTask<Invoker, typename ::std::enable_if<!is_future<
                             compat::invoke_result_t<Invoker>>::value>::type>
    : public ev_task {
 public:
  using value_type = compat::invoke_result_t<Invoker>;
  using error_type = ::std::exception_ptr;
  using future_type = Future<value_type, error_type>;

  template <class F, class... Args>
  AsyncTask(ev_promise_t* promise, ev_exec_t* exec, F&& f,
            Args&&... args) noexcept
      : ev_task EV_TASK_INIT(exec,
                             [](ev_task* task) noexcept {
                               auto self = static_cast<AsyncTask*>(task);
                               auto promise = ::std::move(self->promise_);
                               if (ev_promise_set_acquire(promise)) {
                                 self->result_ = catch_result(self->invoker_);
                                 ev_promise_set_release(
                                     promise, ::std::addressof(self->result_));
                               }
                             }),
        promise_(promise),
        invoker_(::std::forward<F>(f), ::std::forward<Args>(args)...) {}

  AsyncTask(const AsyncTask&) = delete;

  AsyncTask& operator=(const AsyncTask&) = delete;

  Executor
  get_executor() const noexcept {
    return exec;
  }

  future_type
  get_future() const noexcept {
    return promise_.get_future();
  }

 private:
  Promise<value_type, error_type> promise_;
  Invoker invoker_;
  util::Result<value_type, error_type> result_;
};

template <class Invoker>
class AsyncTask<Invoker, typename ::std::enable_if<is_future<
                             compat::invoke_result_t<Invoker>>::value>::type>
    : public ev_task {
  using inner_future_type = compat::invoke_result_t<Invoker>;

 public:
  using value_type = typename inner_future_type::result_type::value_type;
  using error_type = ::std::exception_ptr;
  using future_type = Future<value_type, error_type>;

  template <class F, class... Args>
  AsyncTask(ev_promise_t* promise, ev_exec_t* exec, F&& f,
            Args&&... args) noexcept
      : ev_task
        EV_TASK_INIT(exec,
                     [](ev_task* task) noexcept {
                       auto self = static_cast<AsyncTask*>(task);
                       // Prepare the task for the next future.
                       task->func = [](ev_task* task) noexcept {
                         auto self = static_cast<AsyncTask*>(task);
                         auto promise = ::std::move(self->promise_);
                         // Satisfy the promise with the result of the future
                         // (and convert the error value, if any, to an
                         // exception pointer).
                         if (ev_promise_set_acquire(promise)) {
                           self->result_ = catch_result(
                               [](inner_future_type future) {
                                 return future.get().value();
                               },
                               ::std::move(self->inner_future_));
                           ev_promise_set_release(
                               promise, ::std::addressof(self->result_));
                         }
                       };
                       try {
                         // Store (a reference to) the future returned by the
                         // function.
                         self->inner_future_ = self->invoker_();
                         // Resubmit the task to the new future.
                         self->inner_future_.submit(*self);
                       } catch (...) {
                         // If the function threw an exception, satisfy the
                         // promise immediately.
                         auto promise = ::std::move(self->promise_);
                         if (ev_promise_set_acquire(promise)) {
                           self->result_ = ::std::current_exception();
                           ev_promise_set_release(
                               promise, ::std::addressof(self->result_));
                         }
                       }
                     }),
        promise_(promise),
        invoker_(::std::forward<F>(f), ::std::forward<Args>(args)...) {}

  AsyncTask(const AsyncTask&) = delete;

  AsyncTask& operator=(const AsyncTask&) = delete;

  Executor
  get_executor() const noexcept {
    return exec;
  }

  future_type
  get_future() const noexcept {
    return promise_.get_future();
  }

 private:
  Promise<value_type, error_type> promise_;
  Invoker invoker_;
  inner_future_type inner_future_;
  util::Result<value_type, error_type> result_;
};

}  // namespace detail

/**
 * A helper alias template for the result of
 * #lely::ev::make_async_task<F, Args...>().
 */
template <class F, class... Args>
using AsyncTask = detail::AsyncTask<util::invoker_t<F, Args...>>;

/**
 * Creates a task containing a Callable and its arguments and a future that will
 * eventually hold the result (or the exception, if thrown) of the invocation.
 */
template <class F, class... Args>
inline AsyncTask<F, Args...>*
make_async_task(ev_exec_t* exec, F&& f, Args&&... args) {
  // Create a promise with enough space to store the task and register the
  // destructor.
  auto promise =
      ev_promise_create(sizeof(AsyncTask<F, Args...>), [](void* ptr) noexcept {
        static_cast<AsyncTask<F, Args...>*>(ptr)->~AsyncTask();
      });
  if (!promise) util::throw_errc("make_async_task");
  // Construct the task. Since the destructor is already registered, we cannot
  // handle exceptions thrown by the constructor, so it is defined with the
  // noexcept attribute (see above).
  return new (ev_promise_data(promise)) AsyncTask<F, Args...>(
      promise, exec, ::std::forward<F>(f), ::std::forward<Args>(args)...);
}

/// A future. @see ev_future_t
template <class T, class E>
class Future {
 public:
  /// The type of the result stored in the shared state.
  using result_type = util::Result<T, E>;

  /**
   * Constructs future without (a reference to) a shared state.
   *
   * @post #operator bool() returns false.
   */
  Future() noexcept = default;

  explicit Future(ev_future_t* future) noexcept : future_(future) {}

  /// Constructs a future with a reference to the shared state of <b>other</b>.
  Future(const Future& other) noexcept
      : future_(ev_future_acquire(other.future_)) {}

  /**
   * Moves the shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   */
  Future(Future&& other) noexcept : future_(other.future_) {
    other.future_ = nullptr;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Future(), then creates a
   * reference to the shared state of <b>other</b>.
   *
   * @returns `*this`.
   */
  Future&
  operator=(const Future& other) noexcept {
    if (future_ != other.future_) {
      ev_future_release(future_);
      future_ = ev_future_acquire(other.future_);
    }
    return *this;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Future(), then moves the
   * shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   *
   * @returns `*this`;
   */
  Future&
  operator=(Future&& other) noexcept {
    using ::std::swap;
    swap(future_, other.future_);
    return *this;
  }

  /**
   * Abandons the shared state. If no references remain to the shared state, it
   * is destroyed.
   *
   * @see ev_future_release()
   */
  ~Future() { ev_future_release(future_); }

  operator ev_future_t*() const noexcept { return future_; }

  /// Checks whether `*this` contains (a reference to) a shared state.
  explicit operator bool() const noexcept { return future_ != nullptr; }

  /**
   * Checks whether `*this` contains a unique reference to its shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @see ev_future_is_unique()
   */
  bool
  is_unique() const noexcept {
    return ev_future_is_unique(*this) != 0;
  }

  /**
   * Checks whether the future is ready, i.e., its associated promise has been
   * satisfied and a result has been stored in the shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @see ev_future_is_ready()
   */
  bool
  is_ready() const noexcept {
    return ev_future_is_ready(*this);
  }

  /**
   * Returns the result of a ready future.
   *
   * @returns a reference to the result stored in the shared state.
   *
   * @throws #lely::ev::future_not_ready if #operator bool() or #is_ready()
   * returns false.
   *
   * @see ev_future_get()
   */
  result_type&
  get() {
    if (!*this || !is_ready()) throw future_not_ready("get");
    return *reinterpret_cast<result_type*>(ev_future_get(*this));
  }

  /**
   * Returns the result of a ready future.
   *
   * @returns a reference to the result stored in the shared state.
   *
   * @throws #lely::ev::future_not_ready if #operator bool() or #is_ready()
   * returns false.
   *
   * @see ev_future_get()
   */
  const result_type&
  get() const {
    if (!*this || !is_ready()) throw future_not_ready("get");
    return *reinterpret_cast<result_type*>(ev_future_get(*this));
  }

  /// @see ev_future_submit()
  void
  submit(ev_task& task) noexcept {
    ev_future_submit(*this, &task);
  }

  /// @see ev_future_submit()
  template <class F>
  void
  submit(ev_exec_t* exec, F&& f) {
    submit(*make_task_wrapper(exec, ::std::forward<F>(f)));
  }

  /// @see ev_future_cancel()
  bool
  cancel(ev_task& task) noexcept {
    return !!ev_future_cancel(*this, &task);
  }

  /// @see ev_future_cancel()
  ::std::size_t
  cancel_all() noexcept {
    return ev_future_cancel(*this, nullptr);
  }

  /// @see ev_future_abort()
  bool
  abort(ev_task& task) noexcept {
    return !!ev_future_abort(*this, &task);
  }

  /// @see ev_future_abort()
  ::std::size_t
  abort_all() noexcept {
    return ev_future_abort(*this, nullptr);
  }

  /**
   * Attaches a continuation function to a future and returns a new future which
   * becomes ready once the continuation completes. The continuation receives a
   * single parameter: `*this`. The result of the continuation (or any exception
   * thrown during invocation) is stored in the future, unless the continuation
   * returns a future. In that case, the result of that future (as obtained by
   * `get().value()`) is used instead. This is known as _implicit unwrapping_.
   */
  template <class F>
  typename AsyncTask<F, Future>::future_type
  then(ev_exec_t* exec, F&& f) {
    auto task = make_async_task(exec, ::std::forward<F>(f), *this);
    // Obtain a reference to the future before submitting the continuation to
    // avoid a potential race condition.
    auto future = task->get_future();
    submit(*task);
    return future;
  }

 private:
  ev_future_t* future_{nullptr};
};

/// @see ev_future_when_all_n()
inline Future<::std::size_t, void>
when_all(ev_exec_t* exec, ::std::size_t n, ev_future_t* const* futures) {
  auto f = ev_future_when_all_n(exec, n, futures);
  if (!f) util::throw_errc("when_all");
  return Future<::std::size_t, void>(f);
}

/// @see ev_future_when_all_n()
template <class Allocator>
inline Future<::std::size_t, void>
when_all(ev_exec_t* exec,
         const ::std::vector<ev_future_t*, Allocator>& futures) {
  return when_all(exec, futures.size(), futures.data());
}

/// @see ev_future_when_all_n()
template <class InputIt>
inline typename ::std::enable_if<
    !::std::is_convertible<InputIt, ev_future_t*>::value,
    Future<::std::size_t, void>>::type
when_all(ev_exec_t* exec, InputIt first, InputIt last) {
  return when_all(exec, ::std::vector<ev_future_t*>(first, last));
}

/// @see ev_future_when_all()
template <class... Futures>
inline typename ::std::enable_if<
    compat::conjunction<::std::is_convertible<Futures, ev_future_t*>...>::value,
    Future<::std::size_t, void>>::type
when_all(ev_exec_t* exec, Futures&&... futures) {
  auto f =
      ev_future_when_all(exec, static_cast<ev_future_t*>(futures)..., nullptr);
  if (!f) util::throw_errc("when_all");
  return Future<::std::size_t, void>(f);
}

/// @see ev_future_when_any_n()
inline Future<::std::size_t, void>
when_any(ev_exec_t* exec, ::std::size_t n, ev_future_t* const* futures) {
  auto f = ev_future_when_any_n(exec, n, futures);
  if (!f) util::throw_errc("when_any");
  return Future<::std::size_t, void>(f);
}

/// @see ev_future_when_any_n()
template <class Allocator>
inline Future<::std::size_t, void>
when_any(ev_exec_t* exec,
         const ::std::vector<ev_future_t*, Allocator>& futures) {
  return when_any(exec, futures.size(), futures.data());
}

/// @see ev_future_when_any_n()
template <class InputIt>
inline typename ::std::enable_if<
    !::std::is_convertible<InputIt, ev_future_t*>::value,
    Future<::std::size_t, void>>::type
when_any(ev_exec_t* exec, InputIt first, InputIt last) {
  return when_any(exec, ::std::vector<ev_future_t*>(first, last));
}

/// @see ev_future_when_any()
template <class... Futures>
inline typename ::std::enable_if<
    compat::conjunction<::std::is_convertible<Futures, ev_future_t*>...>::value,
    Future<::std::size_t, void>>::type
when_any(ev_exec_t* exec, Futures&&... futures) {
  auto f =
      ev_future_when_any(exec, static_cast<ev_future_t*>(futures)..., nullptr);
  if (!f) util::throw_errc("when_any");
  return Future<::std::size_t, void>(f);
}

/**
 * Creates a shared state of type `#lely::util::Result<void, E>` that is
 * immediately ready, with a successful result, then returns a future associated
 * with that shared state.
 *
 * @see make_ready_future(), make_error_future()
 */
template <class E = ::std::error_code>
inline Future<void, E>
make_empty_future() {
  Promise<void, E> p;
  p.set(util::success());
  return p.get_future();
}

/**
 * Creates a shared state of type `#lely::util::Result<V, E>` that is
 * immediately ready, with a successul result constructed from
 * `std::forward<T>(value)`, then returns a future associated with that shared
 * state.
 *
 * @see make_empty_future(), make_error_future()
 */
template <class T, class E = ::std::error_code,
          class V = typename ::std::decay<T>::type>
inline Future<V, E>
make_ready_future(T&& value) {
  Promise<V, E> p;
  p.set(util::success(::std::forward<T>(value)));
  return p.get_future();
}

/**
 * Creates a shared state of type `#lely::util::Result<T, V>` that is
 * immediately ready, with a failure result constructed from
 * `std::forward<E>(error)`, then returns a future associated with that shared
 * state.
 *
 * @see make_empty_future(), make_ready_future()
 */
template <class T, class E, class V = typename ::std::decay<E>::type>
inline Future<T, V>
make_error_future(E&& error) {
  Promise<T, V> p;
  p.set(util::failure(::std::forward<E>(error)));
  return p.get_future();
}

/**
 * Creates a task containing a Callable and its arguments, submits it for
 * execution to the specified executor and returns a future that will eventually
 * hold the result (or the exception, if thrown) of the invocation.
 */
template <class F, class... Args>
inline typename AsyncTask<F, Args...>::future_type
async(ev_exec_t* exec, F&& f, Args&&... args) {
  auto task = make_async_task(exec, ::std::forward<F>(f),
                              ::std::forward<Args>(args)...);
  // Obtain a reference to the future before submitting the task to avoid a
  // potential race condition.
  auto future = task->get_future();
  task->get_executor().submit(*task);
  return future;
}

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_FUTURE_HPP_
