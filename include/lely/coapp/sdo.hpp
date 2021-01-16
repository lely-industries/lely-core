/**@file
 * This header file is part of the C++ CANopen master library; it contains the
 * Client-SDO queue declarations.
 *
 * @copyright 2018-2021 Lely Industries N.V.
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

#ifndef LELY_COAPP_SDO_HPP_
#define LELY_COAPP_SDO_HPP_

#include <lely/coapp/sdo_error.hpp>
#include <lely/coapp/type_traits.hpp>
#include <lely/ev/future.hpp>

#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <utility>

// The CAN network interface from <lely/can/net.h>.
struct __can_net;

// The CANopen device from <lely/co/dev.h>.
struct __co_dev;

// The CANopen Client-SDO service from <lely/co/csdo.h>.
struct __co_csdo;

namespace lely {

namespace canopen {

/**
 * A helper alias template for the type of promise used to store the result of
 * an asynchronous SDO request.
 *
 * @see SdoFuture<T>
 */
template <class T>
using SdoPromise = ev::Promise<T, ::std::exception_ptr>;

/**
 * A helper alias template for the type of future used to retrieve the result of
 * an asynchronous SDO request.
 *
 * @see SdoPromise<T>
 */
template <class T>
using SdoFuture = ev::Future<T, ::std::exception_ptr>;

/**
 * Returns an SDO future with a shared state that is immediately ready,
 * containing a successful result of type `void`.
 *
 * @see make_ready_sdo_future(), make_error_sdo_future()
 */
inline SdoFuture<void>
make_empty_sdo_future() {
  return ev::make_empty_future<::std::exception_ptr>();
}

/**
 * Returns an SDO future with a shared state that is immediately ready,
 * containing a successful result constructed from `std::forward<T>(value)`.
 *
 * @see make_empty_sdo_future(), make_error_sdo_future()
 */
template <class T, class V = typename ::std::decay<T>::type>
inline SdoFuture<V>
make_ready_sdo_future(T&& value) {
  return ev::make_ready_future<::std::exception_ptr, V>(
      ::std::forward<T>(value));
}

/**
 * Returns an SDO future with a shared state that is immediately ready,
 * containing a failure result constructed with
 * make_sdo_exception_ptr(uint8_t, uint16_t, uint8_t, ::std::error_code).
 *
 * @see make_empty_sdo_future(), make_ready_sdo_future()
 */
template <class T>
inline SdoFuture<T>
make_error_sdo_future(uint8_t id, uint16_t idx, uint8_t subidx,
                      ::std::error_code ec) {
  return ev::make_error_future<T>(make_sdo_exception_ptr(id, idx, subidx, ec));
}

/**
 * Returns an SDO future with a shared state that is immediately ready,
 * containing a failure result constructed with
 * make_sdo_exception_ptr(uint8_t, uint16_t, uint8_t, ::std::error_code, const ::std::string&).
 *
 * @see make_empty_sdo_future(), make_ready_sdo_future()
 */
template <class T>
inline SdoFuture<T>
make_error_sdo_future(uint8_t id, uint16_t idx, uint8_t subidx,
                      ::std::error_code ec, const ::std::string& what_arg) {
  return ev::make_error_future<T>(
      make_sdo_exception_ptr(id, idx, subidx, ec, what_arg));
}

/**
 * Returns an SDO future with a shared state that is immediately ready,
 * containing a failure result constructed with
 * make_sdo_exception_ptr(uint8_t, uint16_t, uint8_t, ::std::error_code, const char*).
 *
 * @see make_empty_sdo_future(), make_ready_sdo_future()
 */
template <class T>
inline SdoFuture<T>
make_error_sdo_future(uint8_t id, uint16_t idx, uint8_t subidx,
                      ::std::error_code ec, const char* what_arg) {
  return ev::make_error_future<T>(
      make_sdo_exception_ptr(id, idx, subidx, ec, what_arg));
}

class Sdo;

namespace detail {

/// Converts an SDO timeout to a duration.
inline ::std::chrono::milliseconds
from_sdo_timeout(int timeout) {
  using ::std::chrono::milliseconds;
  return timeout <= 0 ? milliseconds::max() : milliseconds(timeout);
}

/// Converts a duration to an SDO timeout.
template <class Rep, class Period>
inline int
to_sdo_timeout(const ::std::chrono::duration<Rep, Period>& d) {
  using ::std::chrono::duration;
  using ::std::chrono::duration_cast;
  using ::std::chrono::milliseconds;
  // The maximum duration is interpreted as an infinite timeout.
  if (d == duration<Rep, Period>::max()) return 0;
  auto timeout = duration_cast<milliseconds>(d).count();
  // A timeout less than 1 ms is rounded up to keep it finite.
  if (timeout < 1) return 1;
  if (timeout > ::std::numeric_limits<int>::max())
    return ::std::numeric_limits<int>::max();
  return timeout;
}

class SdoRequestBase : public ev_task {
  friend class canopen::Sdo;

 public:
  SdoRequestBase(ev_exec_t* exec, uint16_t idx_ = 0, uint8_t subidx_ = 0,
                 bool block_ = false,
                 const ::std::chrono::milliseconds& timeout_ = {})
      : ev_task EV_TASK_INIT(exec,
                             [](ev_task* task) noexcept {
                               (*static_cast<SdoRequestBase*>(task))();
                             }),
        idx(idx_),
        subidx(subidx_),
        block(block_),
        timeout(timeout_) {}

  SdoRequestBase(const SdoRequestBase&) = delete;

  SdoRequestBase& operator=(const SdoRequestBase&) = delete;

  virtual ~SdoRequestBase() = default;

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  GetExecutor() const noexcept {
    return ev::Executor(ev_task::exec);
  }

  /// The node-ID.
  uint8_t id{0};
  /// The object index.
  uint16_t idx{0};
  /// The object sub-index.
  uint8_t subidx{0};
  /**
   * A flag specifying whether the request should use a block SDO instead of a
   * segmented (or expedited) SDO.
   */
  bool block{false};
  /**
   * The SDO timeout. If, after the request is initiated, the timeout expires
   * before receiving a response from the server, the client aborts the transfer
   * with abort code #SdoErrc::TIMEOUT.
   */
  ::std::chrono::milliseconds timeout;
  /// The SDO abort code (0 on success).
  ::std::error_code ec;

 private:
  virtual void operator()() noexcept = 0;

  virtual void OnRequest(void* data) noexcept = 0;
};

template <class T>
class SdoDownloadRequestBase : public SdoRequestBase {
 public:
  using SdoRequestBase::SdoRequestBase;

  template <class U>
  SdoDownloadRequestBase(ev_exec_t* exec, uint16_t idx, uint8_t subidx,
                         U&& value_, bool block = false,
                         const ::std::chrono::milliseconds& timeout = {})
      : SdoRequestBase(exec, idx, subidx, block, timeout),
        value(::std::forward<U>(value_)) {}

  /// The value to be written.
  T value{};
};

class SdoDownloadDcfRequestBase : public SdoRequestBase {
 public:
  using SdoRequestBase::SdoRequestBase;

  SdoDownloadDcfRequestBase(ev_exec_t* exec, const uint8_t* begin_,
                            const uint8_t* end_,
                            const ::std::chrono::milliseconds& timeout = {})
      : SdoRequestBase(exec, 0, 0, false, timeout), begin(begin_), end(end_) {}

  SdoDownloadDcfRequestBase(ev_exec_t* exec, const char* path,
                            const ::std::chrono::milliseconds& timeout = {})
      : SdoRequestBase(exec, 0, 0, false, timeout) {
    Read(path);
  }

  virtual ~SdoDownloadDcfRequestBase();

  /// Reads a concise DCF from the specified <b>path</b>.
  void Read(const char* path);

  /**
   * A pointer the the first byte in a concise DCF (see object 1F22 in CiA 302-3
   * version 4.1.0).
   */
  const uint8_t* begin{nullptr};

  /**
   * A pointer to one past the last byte in the concise DCF. At most
   * `end - begin` bytes are read.
   */
  const uint8_t* end{nullptr};

 private:
  void* dom_{nullptr};
};

template <class T>
class SdoUploadRequestBase : public SdoRequestBase {
 public:
  using SdoRequestBase::SdoRequestBase;

  /// The value received from the SDO server.
  T value{};
};

}  // namespace detail

/// An SDO download (i.e., write) request.
template <class T>
class SdoDownloadRequest : public detail::SdoDownloadRequestBase<T> {
 public:
  /**
   * The signature of the callback function invoked on completion of an SDO
   * download request. Note that the callback function SHOULD NOT throw
   * exceptions. Since it is invoked from C, any exception that is thrown cannot
   * be caught and will result in a call to `std::terminate()`.
   *
   * @param id     the node-ID.
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     the SDO abort code (0 on success).
   */
  using Signature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                         ::std::error_code ec);

  /**
   * Constructs an empty SDO download request. The object index and sub-index,
   * the value to be written and, optionally, the SDO block flag and timeout
   * have to be set before the request can be submitted.
   */
  template <class F>
  SdoDownloadRequest(ev_exec_t* exec, F&& con)
      : detail::SdoDownloadRequestBase<T>(exec), con_(::std::forward<F>(con)) {}

 private:
  void operator()() noexcept final;

  void OnRequest(void* data) noexcept final;

  ::std::function<Signature> con_;
};

/**
 * A series of SDO download (i.e., write) requests corresponding to the entries
 * in a concise DCF.
 */
class SdoDownloadDcfRequest : public detail::SdoDownloadDcfRequestBase {
 public:
  /**
   * The signature of the callback function invoked when all SDO download
   * requests are successfully completed, or when an error occurs. Note that the
   * callback function SHOULD NOT throw exceptions. Since it is invoked from C,
   * any exception that is thrown cannot be caught and will result in a call to
   * `std::terminate()`.
   *
   * @param id     the node-ID.
   * @param idx    the object index of the last SDO download request, whether it
   *               completed successfully (`ec == 0`) or not (`ec != 0`).
   *               <b>idx</b> may be 0 in case of an error in the DCF.
   * @param subidx the object sub-index of the last SDO download request.
   *               <b>subidx</b> may be 0 in case of an error in the DCF.
   * @param ec     the SDO abort code (0 on success). If the concise DCF is
   *               incomplete, the client aborts the transfer with abort code
   *               #SdoErrc::TYPE_LEN_LO.
   */
  using Signature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                         ::std::error_code ec);

  /**
   * Constructs an empty SDO download DCF request. The concise DCF and,
   * optionally, the SDO timeout have to be set before the request can be
   * submitted.
   *
   * @see SdoDownloadDcfRequestBase::Read()
   */
  template <class F>
  SdoDownloadDcfRequest(ev_exec_t* exec, F&& con)
      : detail::SdoDownloadDcfRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void operator()() noexcept final;

  void OnRequest(void* data) noexcept final;

  ::std::function<Signature> con_;
};

/// An SDO upload (i.e., read) request.
template <class T>
class SdoUploadRequest : public detail::SdoUploadRequestBase<T> {
 public:
  /**
   * The signature of the callback function invoked on completion of an SDO
   * upload request. Note that the callback function SHOULD NOT throw
   * exceptions. Since it is invoked from C, any exception that is thrown cannot
   * be caught and will result in a call to `std::terminate()`.
   *
   * @param id     the node-ID.
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     the SDO abort code (0 on success).
   * @param value  the value received from the SDO server.
   */
  using Signature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                         ::std::error_code ec, T value);

  /**
   * Constructs an empty SDO upload request. The object index and sub-index and,
   * optionally, the SDO block flag and timeout have to be set before the
   * request can be submitted.
   */
  template <class F>
  SdoUploadRequest(ev_exec_t* exec, F&& con)
      : detail::SdoUploadRequestBase<T>(exec), con_(::std::forward<F>(con)) {}

 private:
  void operator()() noexcept final;

  void OnRequest(void* data) noexcept final;

  ::std::function<Signature> con_;
};

namespace detail {

template <class T>
class SdoDownloadRequestWrapper : public SdoDownloadRequestBase<T> {
 public:
  using Signature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                         ::std::error_code ec);

  template <class U, class F>
  SdoDownloadRequestWrapper(ev_exec_t* exec, uint16_t idx, uint8_t subidx,
                            U&& value, F&& con, bool block,
                            const ::std::chrono::milliseconds& timeout)
      : SdoDownloadRequestBase<T>(exec, idx, subidx, ::std::forward<U>(value),
                                  block, timeout),
        con_(::std::forward<F>(con)) {}

 private:
  void operator()() noexcept final;

  void OnRequest(void* data) noexcept final;

  ::std::function<Signature> con_;
};

class SdoDownloadDcfRequestWrapper : public SdoDownloadDcfRequestBase {
 public:
  using Signature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                         ::std::error_code ec);

  template <class F>
  SdoDownloadDcfRequestWrapper(ev_exec_t* exec, const uint8_t* begin,
                               const uint8_t* end, F&& con,
                               const ::std::chrono::milliseconds& timeout)
      : SdoDownloadDcfRequestBase(exec, begin, end, timeout),
        con_(::std::forward<F>(con)) {}

  template <class F>
  SdoDownloadDcfRequestWrapper(ev_exec_t* exec, const char* path, F&& con,
                               const ::std::chrono::milliseconds& timeout)
      : SdoDownloadDcfRequestBase(exec, path, timeout),
        con_(::std::forward<F>(con)) {}

  template <class F>
  SdoDownloadDcfRequestWrapper(ev_exec_t* exec, const ::std::string& path,
                               F&& con,
                               const ::std::chrono::milliseconds& timeout)
      : SdoDownloadDcfRequestBase(exec, path.c_str(), timeout),
        con_(::std::forward<F>(con)) {}

 private:
  void operator()() noexcept final;

  void OnRequest(void* data) noexcept final;

  ::std::function<Signature> con_;
};

template <class T>
class SdoUploadRequestWrapper : public SdoUploadRequestBase<T> {
 public:
  using Signature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                         ::std::error_code ec, T value);

  template <class F>
  SdoUploadRequestWrapper(ev_exec_t* exec, uint16_t idx, uint8_t subidx,
                          F&& con, bool block,
                          const ::std::chrono::milliseconds& timeout)
      : SdoUploadRequestBase<T>(exec, idx, subidx, block, timeout),
        con_(::std::forward<F>(con)) {}

 private:
  void operator()() noexcept final;

  void OnRequest(void* data) noexcept final;

  ::std::function<Signature> con_;
};

}  // namespace detail

/**
 * Creates an SDO download request with a completion task. The request deletes
 * itself after it is completed, so it MUST NOT be deleted once it is submitted
 * to a Client-SDO queue.
 *
 * @param exec    the executor used to execute the completion task.
 * @param idx     the object index.
 * @param subidx  the object sub-index.
 * @param value   the value to be written.
 * @param con     the confirmation function to be called on completion of the
 *                SDO request.
 * @param block   a flag specifying whether the request should use a block SDO
 *                instead of a segmented (or expedited) SDO.
 * @param timeout the SDO timeout. If, after the request is initiated, the
 *                timeout expires before receiving a response from the server,
 *                the client aborts the transfer with abort code
 *                #SdoErrc::TIMEOUT.
 *
 * @returns a pointer to a new SDO download request.
 */
template <class T, class U, class F>
inline detail::SdoDownloadRequestWrapper<T>*
make_sdo_download_request(ev_exec_t* exec, uint16_t idx, uint8_t subidx,
                          U&& value, F&& con, bool block = false,
                          const ::std::chrono::milliseconds& timeout = {}) {
  return new detail::SdoDownloadRequestWrapper<T>(
      exec, idx, subidx, ::std::forward<U>(value), ::std::forward<F>(con),
      block, timeout);
}

/**
 * Creates a series of SDO download requests, corresponding to the entries in a
 * concise DCF, with a completion task. The request deletes itself after it is
 * completed, so it MUST NOT be deleted once it is submitted to a Client-SDO
 * queue.
 *
 * @param exec    the executor used to execute the completion task.
 * @param begin   a pointer the the first byte in a concise DCF (see object 1F22
 *                in CiA 302-3 version 4.1.0).
 * @param end     a pointer to one past the last byte in the concise DCF. At
 *                most `end - begin` bytes are read.
 * @param con     the confirmation function to be called when all SDO download
 *                requests are successfully completed, or when an error occurs.
 * @param timeout the SDO timeout. If, after a single request is initiated, the
 *                timeout expires before receiving a response from the server,
 *                the client aborts the transfer with abort code
 *                #SdoErrc::TIMEOUT.
 *
 * @returns a pointer to a new SDO download DCF request.
 */
template <class F>
inline detail::SdoDownloadDcfRequestWrapper*
make_sdo_download_dcf_request(ev_exec_t* exec, const uint8_t* begin,
                              const uint8_t* end, F&& con,
                              const ::std::chrono::milliseconds& timeout = {}) {
  return new detail::SdoDownloadDcfRequestWrapper(
      exec, begin, end, ::std::forward<F>(con), timeout);
}

/**
 * Creates a series of SDO download requests, corresponding to the entries in a
 * concise DCF, with a completion task. The request deletes itself after it is
 * completed, so it MUST NOT be deleted once it is submitted to a Client-SDO
 * queue.
 *
 * @param exec    the executor used to execute the completion task.
 * @param path    the path of the concise DCF.
 * @param con     the confirmation function to be called when all SDO download
 *                requests are successfully completed, or when an error occurs.
 * @param timeout the SDO timeout. If, after a single request is initiated, the
 *                timeout expires before receiving a response from the server,
 *                the client aborts the transfer with abort code
 *                #SdoErrc::TIMEOUT.
 *
 * @returns a pointer to a new SDO download DCF request.
 */
template <class F>
inline detail::SdoDownloadDcfRequestWrapper*
make_sdo_download_dcf_request(ev_exec_t* exec, const char* path, F&& con,
                              const ::std::chrono::milliseconds& timeout = {}) {
  return new detail::SdoDownloadDcfRequestWrapper(
      exec, path, ::std::forward<F>(con), timeout);
}

/**
 * Creates an SDO upload request with a completion task. The request deletes
 * itself after it is completed, so it MUST NOT be deleted once it is submitted
 * to a Client-SDO queue.
 *
 * @param exec    the executor used to execute the completion task.
 * @param idx     the object index.
 * @param subidx  the object sub-index.
 * @param con     the confirmation function to be called on completion of the
 *                SDO request.
 * @param block   a flag specifying whether the request should use a block SDO
 *                instead of a segmented (or expedited) SDO.
 * @param timeout the SDO timeout. If, after the request is initiated, the
 *                timeout expires before receiving a response from the server,
 *                the client aborts the transfer with abort code
 *                #SdoErrc::TIMEOUT.
 *
 * @returns a pointer to a new SDO upload request.
 */
template <class T, class F>
inline detail::SdoUploadRequestWrapper<T>*
make_sdo_upload_request(ev_exec_t* exec, uint16_t idx, uint8_t subidx, F&& con,
                        bool block = false,
                        const ::std::chrono::milliseconds& timeout = {}) {
  return new detail::SdoUploadRequestWrapper<T>(
      exec, idx, subidx, ::std::forward<F>(con), block, timeout);
}

/// A Client-SDO queue.
class Sdo {
  template <class>
  friend class SdoDownloadRequest;

  friend class SdoDownloadDcfRequest;

  template <class>
  friend class SdoUploadRequest;

  template <class>
  friend class detail::SdoDownloadRequestWrapper;

  friend class detail::SdoDownloadDcfRequestWrapper;

  template <class>
  friend class detail::SdoUploadRequestWrapper;

 public:
  /// Default-constructs an invalid Client-SDO queue.
  Sdo();

  /**
   * Constructs a Client-SDO queue for a Client-SDO from the predefined
   * connection set (the default SDO). In general, only a CANopen master is
   * allowed to use the default SDO.
   *
   * @param net a pointer to a CAN network interface (from <lely/can/net.hpp>).
   * @param id  the node-ID of the SDO server (in the range [1..127]).
   */
  Sdo(__can_net* net, uint8_t id);

  /**
   * Constructs a Client-SDO queue for a pre-configured Client-SDO. The SDO
   * client parameter record MUST exist in the object dictionary (object 1280 to
   * 12FF).
   *
   * @param net a pointer to a CAN network interface (from <lely/can/net.h>).
   * @param dev a pointer to a CANopen device (from <lely/co/dev.h>).
   * @param num the SDO number (in the range [1..128]).
   */
  Sdo(__can_net* net, __co_dev* dev, uint8_t num);

  /**
   * Constructs a Client-SDO queue from an existing Client-SDO service. It is
   * the responsibility of the caller to ensure that the SDO service remains
   * available during the lifetime of the queue.
   *
   * @param sdo a pointer to a CANopen Client-SDO service (from
   *            <lely/co/csdo.h>).
   */
  Sdo(__co_csdo* sdo);

  Sdo(const Sdo&) = delete;
  Sdo(Sdo&&) = default;

  Sdo& operator=(const Sdo&) = delete;
  Sdo& operator=(Sdo&&);

  /**
   * Destructs the Client-SDO queue. Any ongoing or pending SDO requests are
   * terminated with abort code #SdoErrc::DATA_CTL.
   */
  ~Sdo();

  /// Checks whether `*this` is a valid Client-SDO queue.
  explicit operator bool() const noexcept { return !!impl_; }

  /// Queues an SDO download request.
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type
  SubmitDownload(SdoDownloadRequest<T>& req) {
    Submit(req);
  }

  /**
   * Queues an SDO download request. This function writes a value to a
   * sub-object in a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param block   a flag specifying whether the request should use a block SDO
   *                instead of a segmented (or expedited) SDO.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   */
  template <class T, class F, class U = typename ::std::decay<T>::type>
  typename ::std::enable_if<is_canopen<U>::value>::type
  SubmitDownload(ev_exec_t* exec, uint16_t idx, uint8_t subidx, T&& value,
                 F&& con, bool block = false,
                 const ::std::chrono::milliseconds& timeout = {}) {
    Submit(*make_sdo_download_request<U>(
        exec, idx, subidx, ::std::forward<T>(value), ::std::forward<F>(con),
        block, timeout));
  }

  /// Cancels an SDO download request. @see Cancel()
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value, bool>::type
  CancelDownload(SdoDownloadRequest<T>& req, SdoErrc ac) {
    return Cancel(req, ac);
  }

  /// Aborts an SDO download request. @see Abort()
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value, bool>::type
  AbortDownload(SdoDownloadRequest<T>& req) {
    return Abort(req);
  }

  /// Queues an SDO download DCF request.
  void
  SubmitDownloadDcf(SdoDownloadDcfRequest& req) {
    Submit(req);
  }

  /**
   * Queues an SDO download DCF request. This function writes each entry in the
   * specified concise DCF to a sub-object in a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param begin   a pointer the the first byte in a concise DCF (see object
   *                1F22 in CiA 302-3 version 4.1.0).
   * @param end     a pointer to one past the last byte in the concise DCF. At
   *                most `end - begin` bytes are read.
   * @param con     the confirmation function to be called when all SDO download
   *                requests are successfully completed, or when an error
   *                occurs.
   * @param timeout the SDO timeout. If, after a single request is initiated,
   *                the timeout expires before receiving a response from the
   *                server, the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   */
  template <class F>
  void
  SubmitDownloadDcf(ev_exec_t* exec, const uint8_t* begin, const uint8_t* end,
                    F&& con, const ::std::chrono::milliseconds& timeout = {}) {
    Submit(*make_sdo_download_dcf_request(exec, begin, end,
                                          ::std::forward<F>(con), timeout));
  }

  /**
   * Queues an SDO download DCF request. This function writes each entry in the
   * specified concise DCF to a sub-object in a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param path    the path of the concise DCF.
   * @param con     the confirmation function to be called when all SDO download
   *                requests are successfully completed, or when an error
   *                occurs.
   * @param timeout the SDO timeout. If, after a single request is initiated,
   *                the timeout expires before receiving a response from the
   *                server, the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   */
  template <class F>
  void
  SubmitDownloadDcf(ev_exec_t* exec, const char* path, F&& con,
                    const ::std::chrono::milliseconds& timeout = {}) {
    Submit(*make_sdo_download_dcf_request(exec, path, ::std::forward<F>(con),
                                          timeout));
  }

  /// Cancels an SDO download request. @see Cancel()
  bool
  CancelDownloadDcf(SdoDownloadDcfRequest& req, SdoErrc ac) {
    return Cancel(req, ac);
  }

  /// Aborts an SDO download DCF request. @see Abort()
  bool
  AbortDownloadDcf(SdoDownloadDcfRequest& req) {
    return Abort(req);
  }

  /// Queues an SDO upload request.
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type
  SubmitUpload(SdoUploadRequest<T>& req) {
    Submit(req);
  }

  /**
   * Queues an SDO upload request. This function reads the value of a sub-object
   * in a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param block   a flag specifying whether the request should use a block SDO
   *                instead of a segmented (or expedited) SDO.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   */
  template <class T, class F>
  typename ::std::enable_if<is_canopen<T>::value>::type
  SubmitUpload(ev_exec_t* exec, uint16_t idx, uint8_t subidx, F&& con,
               bool block = false,
               const ::std::chrono::milliseconds& timeout = {}) {
    Submit(*make_sdo_upload_request<T>(exec, idx, subidx,
                                       ::std::forward<F>(con), block, timeout));
  }

  /// Cancels an SDO upload request. @see Cancel()
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value, bool>::type
  CancelUpload(SdoUploadRequest<T>& req, SdoErrc ac) {
    return Cancel(req, ac);
  }

  /// Aborts an SDO upload request. @see Abort()
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value, bool>::type
  AbortUpload(SdoUploadRequest<T>& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous SDO download request and creates a future which
   * becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param block   a flag specifying whether the request should use a block SDO
   *                instead of a segmented (or expedited) SDO.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO abort code on failure.
   */
  template <class T, class U = typename ::std::decay<T>::type>
  typename ::std::enable_if<is_canopen<U>::value, SdoFuture<void>>::type
  AsyncDownload(ev_exec_t* exec, uint16_t idx, uint8_t subidx, T&& value,
                bool block = false,
                const ::std::chrono::milliseconds& timeout = {}) {
    SdoPromise<void> p;
    SubmitDownload(
        exec, idx, subidx, ::std::forward<T>(value),
        [p](uint8_t id, uint16_t idx, uint8_t subidx,
            ::std::error_code ec) mutable {
          if (ec)
            p.set(util::failure(
                make_sdo_exception_ptr(id, idx, subidx, ec, "AsyncDownload")));
          else
            p.set(util::success());
        },
        block, timeout);
    return p.get_future();
  }

  /**
   * Queues a series asynchronous SDO download requests, corresponding to the
   * entries in the specified concise DCF, and creates a future which
   * becomes ready once all requests complete (or an error occurs).
   *
   * @param exec    the executor used to execute the completion task.
   * @param begin   a pointer the the first byte in a concise DCF (see object
   *                1F22 in CiA 302-3 version 4.1.0).
   * @param end     a pointer to one past the last byte in the concise DCF. At
   *                most `end - begin` bytes are read.
   * @param timeout the SDO timeout. If, after a single request is initiated,
   *                the timeout expires before receiving a response from the
   *                server, the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO abort code on failure.
   */
  SdoFuture<void> AsyncDownloadDcf(
      ev_exec_t* exec, const uint8_t* begin, const uint8_t* end,
      const ::std::chrono::milliseconds& timeout = {});

  /**
   * Queues a series asynchronous SDO download requests, corresponding to the
   * entries in the specified concise DCF, and creates a future which
   * becomes ready once all requests complete (or an error occurs).
   *
   * @param exec    the executor used to execute the completion task.
   * @param path    the path of the concise DCF.
   * @param timeout the SDO timeout. If, after a single request is initiated,
   *                the timeout expires before receiving a response from the
   *                server, the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO abort code on failure.
   */
  SdoFuture<void> AsyncDownloadDcf(
      ev_exec_t* exec, const char* path,
      const ::std::chrono::milliseconds& timeout = {});

  /**
   * Queues an asynchronous SDO upload request and creates a future which
   * becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param block   a flag specifying whether the request should use a block SDO
   *                instead of a segmented (or expedited) SDO.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the received value on success and the SDO
   * abort code on failure.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value, SdoFuture<T>>::type
  AsyncUpload(ev_exec_t* exec, uint16_t idx, uint8_t subidx, bool block = false,
              const ::std::chrono::milliseconds& timeout = {}) {
    SdoPromise<T> p;
    SubmitUpload<T>(
        exec, idx, subidx,
        [p](uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code ec,
            T value) mutable {
          if (ec)
            p.set(util::failure(
                make_sdo_exception_ptr(id, idx, subidx, ec, "AsyncUpload")));
          else
            p.set(util::success(::std::move(value)));
        },
        block, timeout);
    return p.get_future();
  }

  /// Queues an LSS request.
  void Submit(detail::SdoRequestBase& req);

  /**
   * Cancels a pending SDO request.
   *
   * @param req the request to be aborted.
   * @param ac  the SDO abort code.
   *
   * @returns true if the request was canceled, and false if it is ongoing or
   * already completed.
   */
  bool Cancel(detail::SdoRequestBase& req, SdoErrc ac);

  /**
   * Cancels all pending SDO requests and stops the ongoing request, if any.
   *
   * @param ac the SDO abort code.
   *
   * @returns the number of canceled requests.
   */
  ::std::size_t CancelAll(SdoErrc ac);

  /**
   * Aborts a pending SDO request. The completion task is _not_ submitted for
   * execution.
   *
   * @returns true if the request was aborted, and false if it is ongoing or
   * already completed.
   */
  bool Abort(detail::SdoRequestBase& req);

  /**
   * Aborts all pending SDO requests. The completion tasks of aborted requests
   * are _not_ submitted for execution.
   *
   * @returns the number of aborted requests.
   */
  ::std::size_t AbortAll();

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_SDO_HPP_
