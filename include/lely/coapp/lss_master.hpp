/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen Layer Setting Services (LSS) master declarations.
 *
 * For more information about the LSS protocol, see CiA 305 v3.0.0.
 *
 * @copyright 2020 Lely Industries N.V.
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

#ifndef LELY_COAPP_LSS_MASTER_HPP_
#define LELY_COAPP_LSS_MASTER_HPP_

#include <lely/coapp/node.hpp>
#include <lely/libc/functional.hpp>
#include <lely/libc/type_traits.hpp>

#include <chrono>
#include <memory>
#include <utility>

namespace lely {

namespace canopen {

/**
 * A helper alias template for the type of promise used to store the result of
 * an asynchronous LSS request.
 *
 * @see LssFuture<T>
 */
template <class T>
using LssPromise = ev::Promise<T, ::std::exception_ptr>;

/**
 * A helper alias template for the type of future used to retrieve the result of
 * an asynchronous LSS request.
 *
 * @see LssPromise<T>
 */
template <class T>
using LssFuture = ev::Future<T, ::std::exception_ptr>;

/**
 * The 128-bit number uniquely identifying each CANopen node. The fields of this
 * struct correspond to the sub-indices of object 1018 (Identity object).
 */
struct LssAddress {
  LssAddress(uint32_t vendor_id_ = 0, uint32_t product_code_ = 0,
             uint32_t revision_ = 0, uint32_t serial_nr_ = 0)
      : vendor_id(vendor_id_),
        product_code(product_code_),
        revision(revision_),
        serial_nr(serial_nr_) {}

  /// The vendor-ID.
  uint32_t vendor_id;
  /// THe product code.
  uint32_t product_code;
  /// The revision number.
  uint32_t revision;
  /// THe serial number.
  uint32_t serial_nr;
};

/// The states of the LSS finite state automaton (FSA) of a slave device.
enum class LssState {
  /// The state in which a slave may be identified.
  WAITING = 0,
  /**
   * The state in which the node-ID and bit timing parameters of a slave may be
   * configured.
   */
  CONFIG = 1
};

class LssMaster;

namespace detail {

class LssRequestBase : public ev_task {
  friend class canopen::LssMaster;

 public:
  explicit LssRequestBase(ev_exec_t* exec)
      : ev_task EV_TASK_INIT(exec, [](ev_task* task) noexcept {
          (*static_cast<LssRequestBase*>(task))();
        }) {}

  LssRequestBase(const LssRequestBase&) = delete;
  LssRequestBase& operator=(const LssRequestBase&) = delete;

  virtual ~LssRequestBase() = default;

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  GetExecutor() const noexcept {
    return ev::Executor(ev_task::exec);
  }

  /// The error code (0 on success).
  ::std::error_code ec{};

 private:
  virtual void operator()() noexcept = 0;

  virtual void OnRequest(void* data) noexcept = 0;
};

class LssSwitchRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /// The desired state of the LSS slave devices.
  LssState state{LssState::WAITING};

 private:
  void OnRequest(void* data) noexcept final;
};

class LssSwitchSelectiveRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /**
   * The address of the LSS slave device to be switched into the configuration
   * state.
   */
  LssAddress address{0, 0, 0, 0};

 private:
  void OnRequest(void* data) noexcept final;
};

class LssSetIdRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /// The requested pending node-ID of the LSS slave device.
  uint8_t id{0};

 private:
  void OnRequest(void* data) noexcept final;
};

class LssSetBitrateRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /// The requested pending bit rate (in bit/s) of the LSS slave device.
  int bitrate{0};

 private:
  void OnRequest(void* data) noexcept final;
};

class LssSwitchBitrateRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /**
   * The delay (in milliseconds) before and after the switch, during which CAN
   * frames MUST NOT be sent.
   */
  int delay{0};

 private:
  void OnRequest(void* data) noexcept final;
};

class LssStoreRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

 private:
  void OnRequest(void* data) noexcept final;
};

class LssGetNumberRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /// The LSS number reported by the slave device.
  uint32_t number{0};
};

class LssGetVendorIdRequestBase : public LssGetNumberRequestBase {
 public:
  using LssGetNumberRequestBase::LssGetNumberRequestBase;

 private:
  void OnRequest(void* data) noexcept final;
};

class LssGetProductCodeRequestBase : public LssGetNumberRequestBase {
 public:
  using LssGetNumberRequestBase::LssGetNumberRequestBase;

 private:
  void OnRequest(void* data) noexcept final;
};

class LssGetRevisionRequestBase : public LssGetNumberRequestBase {
 public:
  using LssGetNumberRequestBase::LssGetNumberRequestBase;

 private:
  void OnRequest(void* data) noexcept final;
};

class LssGetSerialNrRequestBase : public LssGetNumberRequestBase {
 public:
  using LssGetNumberRequestBase::LssGetNumberRequestBase;

 private:
  void OnRequest(void* data) noexcept final;
};

class LssGetIdRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /// The active node-ID reported by the LSS slave device.
  uint8_t id{0};

 private:
  void OnRequest(void* data) noexcept final;
};

class LssIdNonConfigRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

 private:
  void OnRequest(void* data) noexcept final;
};

class LssScanRequestBase : public LssRequestBase {
 public:
  using LssRequestBase::LssRequestBase;

  /**
   * On success, the LSS address of the detected slave device. If a slave is
   * detected, it is switched to the LSS configuration state.
   */
  LssAddress address{0, 0, 0, 0};
};

class LssSlowscanRequestBase : public LssScanRequestBase {
 public:
  using LssScanRequestBase::LssScanRequestBase;

  /// The lower bound of the LSS address of the slave device.
  LssAddress lo{0, 0, 0, 0};
  /**
   * The upper bound of the LSS address of the slave device. The vendor-ID and
   * product code MUST be equal to those in #lo.
   */
  LssAddress hi{0, 0, 0, 0};

 private:
  void OnRequest(void* data) noexcept final;
};

class LssFastscanRequestBase : public LssScanRequestBase {
 public:
  using LssScanRequestBase::LssScanRequestBase;

  /**
   * A mask specifying which bits in the LSS address of the slave device are
   * already known and can be skipped during scanning. If a bit in the mask is
   * 1, the corresponding bit in the LSS address is _not_ checked.
   */
  LssAddress mask{0, 0, 0, 0};

 private:
  void OnRequest(void* data) noexcept final;
};

}  // namespace detail

/// An LSS 'switch state global' request.
class LssSwitchRequest : public detail::LssSwitchRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'switch state global' request. Note that the callback function SHOULD NOT
   * throw exceptions. Since it is invoked from C, any exception that is thrown
   * cannot be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   */
  using Signature = void(::std::error_code ec);

  /**
   * Constructs an empty LSS 'switch state global' request with a completion
   * task. The desired state of the LSS slave devices has to be set before the
   * request can be submitted.
   */
  template <class F>
  explicit LssSwitchRequest(ev_exec_t* exec, F&& con)
      : detail::LssSwitchRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssSwitchRequest(nullptr, con)`.
  template <class F>
  explicit LssSwitchRequest(F&& con)
      : LssSwitchRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'switch state selective' request.
class LssSwitchSelectiveRequest : public detail::LssSwitchSelectiveRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'switch state selective' request. Note that the callback function SHOULD
   * NOT throw exceptions. Since it is invoked from C, any exception that is
   * thrown cannot be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   */
  using Signature = void(::std::error_code ec);

  /**
   * Constructs an empty LSS 'switch state selective' request with a completion
   * task. The address of the LSS slave device to be switched into the
   * configuration state has to be set before the request can be submitted.
   */
  template <class F>
  explicit LssSwitchSelectiveRequest(ev_exec_t* exec, F&& con)
      : detail::LssSwitchSelectiveRequestBase(exec),
        con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssSwitchSelectiveRequest(nullptr, con)`.
  template <class F>
  explicit LssSwitchSelectiveRequest(F&& con)
      : LssSwitchSelectiveRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'configure node-ID' request.
class LssSetIdRequest : public detail::LssSetIdRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'configure node-ID' request. Note that the callback function SHOULD NOT
   * throw exceptions. Since it is invoked from C, any exception that is thrown
   * cannot be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   */
  using Signature = void(::std::error_code ec);

  /**
   * Constructs an empty LSS 'configure node-ID' request with a completion task.
   * The requested pending node-ID of the LSS slave device has to be set before
   * the request can be submitted.
   */
  template <class F>
  explicit LssSetIdRequest(ev_exec_t* exec, F&& con)
      : detail::LssSetIdRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssSetIdRequest(nullptr, con)`.
  template <class F>
  explicit LssSetIdRequest(F&& con)
      : LssSetIdRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'configure bit timing parameters' request.
class LssSetBitrateRequest : public detail::LssSetBitrateRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'configure bit timing parameters' request. Note that the callback function
   * SHOULD NOT throw exceptions. Since it is invoked from C, any exception that
   * is thrown cannot be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   */
  using Signature = void(::std::error_code ec);

  /**
   * Constructs an empty LSS 'configure bit timing parameters' request with a
   * completion task. The requested pending bit rate of the LSS slave device has
   * to be set before the request can be submitted.
   */
  template <class F>
  explicit LssSetBitrateRequest(ev_exec_t* exec, F&& con)
      : detail::LssSetBitrateRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssSetBitrateRequest(nullptr, con)`.
  template <class F>
  explicit LssSetBitrateRequest(F&& con)
      : LssSetBitrateRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'activate bit timing parameters' request.
class LssSwitchBitrateRequest : public detail::LssSwitchBitrateRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'activate bit timing parameters' request. Note that the callback function
   * SHOULD NOT throw exceptions. Since it is invoked from C, any exception that
   * is thrown cannot be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   */
  using Signature = void(::std::error_code ec);

  /**
   * Constructs an empty LSS 'activate bit timing parameters' request with a
   * completion task. The delay before and after the switch, during which CAN
   * frames MUST NOT be sent, has to be set before the request can be submitted.
   */
  template <class F>
  explicit LssSwitchBitrateRequest(ev_exec_t* exec, F&& con)
      : detail::LssSwitchBitrateRequestBase(exec),
        con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssSwitchBitrateRequest(nullptr, con)`.
  template <class F>
  explicit LssSwitchBitrateRequest(F&& con)
      : LssSwitchBitrateRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'store configuration' request.
class LssStoreRequest : public detail::LssStoreRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'store configuration' request. Note that the callback function SHOULD NOT
   * throw exceptions. Since it is invoked from C, any exception that is thrown
   * cannot be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   */
  using Signature = void(::std::error_code ec);

  /**
   * Constructs an empty LSS 'store configuration' request with a completion
   * task.
   */
  template <class F>
  explicit LssStoreRequest(ev_exec_t* exec, F&& con)
      : detail::LssStoreRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssStoreRequest(nullptr, con)`.
  template <class F>
  explicit LssStoreRequest(F&& con)
      : LssStoreRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'inquire identity vendor-ID' request.
class LssGetVendorIdRequest : public detail::LssGetVendorIdRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'inquire identity vendor-ID' request. Note that the callback function
   * SHOULD NOT throw exceptions. Since it is invoked from C, any exception
   * that is thrown cannot be caught and will result in a call to
   * `std::terminate()`.
   *
   * @param ec     the error code (0 on success).
   * @param number the vendor-ID reported by the slave device.
   */
  using Signature = void(::std::error_code ec, uint32_t number);

  /**
   * Constructs an empty LSS 'store configuration' request with a completion
   * task.
   */
  template <class F>
  explicit LssGetVendorIdRequest(ev_exec_t* exec, F&& con)
      : detail::LssGetVendorIdRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssGetVendorIdRequest(nullptr, con)`.
  template <class F>
  explicit LssGetVendorIdRequest(F&& con)
      : LssGetVendorIdRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec, number);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'inquire identity product-code' request.
class LssGetProductCodeRequest : public detail::LssGetProductCodeRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'inquire identity product-code' request. Note that the callback function
   * SHOULD NOT throw exceptions. Since it is invoked from C, any exception
   * that is thrown cannot be caught and will result in a call to
   * `std::terminate()`.
   *
   * @param ec     the error code (0 on success).
   * @param number the product code reported by the slave device.
   */
  using Signature = void(::std::error_code ec, uint32_t number);

  /**
   * Constructs an empty LSS 'store configuration' request with a completion
   * task.
   */
  template <class F>
  explicit LssGetProductCodeRequest(ev_exec_t* exec, F&& con)
      : detail::LssGetProductCodeRequestBase(exec),
        con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssGetProductCodeRequest(nullptr, con)`.
  template <class F>
  explicit LssGetProductCodeRequest(F&& con)
      : LssGetProductCodeRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec, number);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'inquire identity revision-number' request.
class LssGetRevisionRequest : public detail::LssGetRevisionRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'inquire identity revision-number' request. Note that the callback function
   * SHOULD NOT throw exceptions. Since it is invoked from C, any exception
   * that is thrown cannot be caught and will result in a call to
   * `std::terminate()`.
   *
   * @param ec     the error code (0 on success).
   * @param number the revision number reported by the slave device.
   */
  using Signature = void(::std::error_code ec, uint32_t number);

  /**
   * Constructs an empty LSS 'store configuration' request with a completion
   * task.
   */
  template <class F>
  explicit LssGetRevisionRequest(ev_exec_t* exec, F&& con)
      : detail::LssGetRevisionRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssGetRevisionRequest(nullptr, con)`.
  template <class F>
  explicit LssGetRevisionRequest(F&& con)
      : LssGetRevisionRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec, number);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'inquire identity serial-number' request.
class LssGetSerialNrRequest : public detail::LssGetSerialNrRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'inquire identity serial-number' request. Note that the callback function
   * SHOULD NOT throw exceptions. Since it is invoked from C, any exception
   * that is thrown cannot be caught and will result in a call to
   * `std::terminate()`.
   *
   * @param ec     the error code (0 on success).
   * @param number the serial number reported by the slave device.
   */
  using Signature = void(::std::error_code ec, uint32_t number);

  /**
   * Constructs an empty LSS 'store configuration' request with a completion
   * task.
   */
  template <class F>
  explicit LssGetSerialNrRequest(ev_exec_t* exec, F&& con)
      : detail::LssGetSerialNrRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssGetSerialNrRequest(nullptr, con)`.
  template <class F>
  explicit LssGetSerialNrRequest(F&& con)
      : LssGetSerialNrRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec, number);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'inquire node-ID' request.
class LssGetIdRequest : public detail::LssGetIdRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'inquire node-ID' request. Note that the callback function SHOULD NOT
   * throw exceptions. Since it is invoked from C, any exception that is thrown
   * cannot be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   * @param id the active node-ID reported by the slave device.
   */
  using Signature = void(::std::error_code ec, uint8_t id);

  /**
   * Constructs an empty LSS 'store configuration' request with a completion
   * task.
   */
  template <class F>
  explicit LssGetIdRequest(ev_exec_t* exec, F&& con)
      : detail::LssGetIdRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssGetIdRequest(nullptr, con)`.
  template <class F>
  explicit LssGetIdRequest(F&& con)
      : LssGetIdRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec, id);
  }

  ::std::function<Signature> con_;
};

/// An LSS 'identify non-configured remote slave' request.
class LssIdNonConfigRequest : public detail::LssStoreRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an LSS
   * 'identify non-configured remote slave' request. Note that the callback
   * function SHOULD NOT throw exceptions. Since it is invoked from C, any
   * exception that is thrown cannot be caught and will result in a call to
   * `std::terminate()`.
   *
   * @param ec    the error code (0 on success).
   * @param found true if a non-configured slave was found, false if not.
   */
  using Signature = void(::std::error_code ec, bool found);

  /**
   * Constructs an empty LSS 'identify non-configured remote slave' request with
   * a completion task.
   */
  template <class F>
  explicit LssIdNonConfigRequest(ev_exec_t* exec, F&& con)
      : detail::LssStoreRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssIdNonConfigRequest(nullptr, con)`.
  template <class F>
  explicit LssIdNonConfigRequest(F&& con)
      : LssIdNonConfigRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    bool found = !ec;
    // A timeout means no slave was found, but is otherwise not an error.
    if (ec == ::std::errc::timed_out) ec.clear();
    if (con_) con_(ec, found);
  }

  ::std::function<Signature> con_;
};

/// An 'LSS Slowscan' request.
class LssSlowscanRequest : public detail::LssSlowscanRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an
   * 'LSS Slowscan' request. Note that the callback function SHOULD NOT throw
   * exceptions. Since it is invoked from C, any exception that is thrown cannot
   * be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   * @param id the node-ID reported by the slave device.
   */
  using Signature = void(::std::error_code ec, LssAddress address);

  /**
   * Constructs an empty 'LSS Slowscan' request with a completion task. The
   * upper and lower bound of the LSS address of the slave device have to be set
   * before the request can be submitted.
   */
  template <class F>
  explicit LssSlowscanRequest(ev_exec_t* exec, F&& con)
      : detail::LssSlowscanRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssSlowscanRequest(nullptr, con)`.
  template <class F>
  explicit LssSlowscanRequest(F&& con)
      : LssSlowscanRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec, address);
  }

  ::std::function<Signature> con_;
};

/// An 'LSS Fastscan' request.
class LssFastscanRequest : public detail::LssFastscanRequestBase {
 public:
  /**
   * The signature of the callback function invoked on completion of an
   * 'LSS Fastscan' request. Note that the callback function SHOULD NOT throw
   * exceptions. Since it is invoked from C, any exception that is thrown cannot
   * be caught and will result in a call to `std::terminate()`.
   *
   * @param ec the error code (0 on success).
   * @param id the node-ID reported by the slave device.
   */
  using Signature = void(::std::error_code ec, LssAddress address);

  /**
   * Constructs an empty 'LSS Fastscan' request with a completion task. The bits
   * of the LSS address of the slave device which are already known and a mask
   * specifying which which can be skipped have to be set before the request can
   * be submitted.
   */
  template <class F>
  explicit LssFastscanRequest(ev_exec_t* exec, F&& con)
      : detail::LssFastscanRequestBase(exec), con_(::std::forward<F>(con)) {}

  /// Equivalent to `LssSlowscanRequest(nullptr, con)`.
  template <class F>
  explicit LssFastscanRequest(F&& con)
      : LssFastscanRequest(nullptr, ::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    if (con_) con_(ec, address);
  }

  ::std::function<Signature> con_;
};

namespace detail {

template <class F>
class LssSwitchRequestWrapper : public LssSwitchRequestBase {
 public:
  explicit LssSwitchRequestWrapper(ev_exec_t* exec, LssState state, F&& con)
      : LssSwitchRequestBase(exec), con_(::std::forward<F>(con)) {
    this->state = state;
  }

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssSwitchSelectiveRequestWrapper : public LssSwitchSelectiveRequestBase {
 public:
  explicit LssSwitchSelectiveRequestWrapper(ev_exec_t* exec,
                                            const LssAddress& address, F&& con)
      : LssSwitchSelectiveRequestBase(exec), con_(::std::forward<F>(con)) {
    this->address = address;
  }

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssSetIdRequestWrapper : public LssSetIdRequestBase {
 public:
  explicit LssSetIdRequestWrapper(ev_exec_t* exec, uint8_t id, F&& con)
      : LssSetIdRequestBase(exec), con_(::std::forward<F>(con)) {
    this->id = id;
  }

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssSetBitrateRequestWrapper : public LssSetBitrateRequestBase {
 public:
  explicit LssSetBitrateRequestWrapper(ev_exec_t* exec, int bitrate, F&& con)
      : LssSetBitrateRequestBase(exec), con_(::std::forward<F>(con)) {
    this->bitrate = bitrate;
  }

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssSwitchBitrateRequestWrapper : public LssSwitchBitrateRequestBase {
 public:
  explicit LssSwitchBitrateRequestWrapper(ev_exec_t* exec, int delay, F&& con)
      : LssSwitchBitrateRequestBase(exec), con_(::std::forward<F>(con)) {
    this->delay = delay;
  }

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssStoreRequestWrapper : public LssStoreRequestBase {
 public:
  explicit LssStoreRequestWrapper(ev_exec_t* exec, F&& con)
      : LssStoreRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssGetVendorIdRequestWrapper : public LssGetVendorIdRequestBase {
 public:
  explicit LssGetVendorIdRequestWrapper(ev_exec_t* exec, F&& con)
      : LssGetVendorIdRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec, number);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssGetProductCodeRequestWrapper : public LssGetProductCodeRequestBase {
 public:
  explicit LssGetProductCodeRequestWrapper(ev_exec_t* exec, F&& con)
      : LssGetProductCodeRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec, number);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssGetRevisionRequestWrapper : public LssGetRevisionRequestBase {
 public:
  explicit LssGetRevisionRequestWrapper(ev_exec_t* exec, F&& con)
      : LssGetRevisionRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec, number);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssGetSerialNrRequestWrapper : public LssGetSerialNrRequestBase {
 public:
  explicit LssGetSerialNrRequestWrapper(ev_exec_t* exec, F&& con)
      : LssGetSerialNrRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec, number);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssGetIdRequestWrapper : public LssGetIdRequestBase {
 public:
  explicit LssGetIdRequestWrapper(ev_exec_t* exec, F&& con)
      : LssGetIdRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec, id);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssIdNonConfigRequestWrapper : public LssIdNonConfigRequestBase {
 public:
  explicit LssIdNonConfigRequestWrapper(ev_exec_t* exec, F&& con)
      : LssIdNonConfigRequestBase(exec), con_(::std::forward<F>(con)) {}

 private:
  void
  operator()() noexcept final {
    bool found = !ec;
    // A timeout means no slave was found, but is otherwise not an error.
    if (ec == ::std::errc::timed_out) ec.clear();
    compat::invoke(::std::move(con_), ec, found);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssSlowscanRequestWrapper : public LssSlowscanRequestBase {
 public:
  explicit LssSlowscanRequestWrapper(ev_exec_t* exec, const LssAddress& lo,
                                     const LssAddress& hi, F&& con)
      : LssSlowscanRequestBase(exec), con_(::std::forward<F>(con)) {
    this->lo = lo;
    this->hi = hi;
  }

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec, address);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

template <class F>
class LssFastscanRequestWrapper : public LssFastscanRequestBase {
 public:
  explicit LssFastscanRequestWrapper(ev_exec_t* exec, const LssAddress& address,
                                     const LssAddress& mask, F&& con)
      : LssFastscanRequestBase(exec), con_(::std::forward<F>(con)) {
    this->address = address;
    this->mask = mask;
  }

 private:
  void
  operator()() noexcept final {
    compat::invoke(::std::move(con_), ec, address);
    delete this;
  }

  typename ::std::decay<F>::type con_;
};

}  // namespace detail

/**
 * Creates an LSS 'switch state global' request with a completion task. The
 * request deletes itself after it is completed, so it MUST NOT be deleted once
 * it is submitted to an LSS master.
 *
 * @param exec  the executor used to execute the completion task.
 * @param state the desired state of the LSS slave devices.
 * @param con   the confirmation function to be called on completion of the LSS
 *              request.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::LssSwitchRequestWrapper<F>*>::type
make_lss_switch_request(ev_exec_t* exec, LssState state, F&& con) {
  // clang-format on
  return new detail::LssSwitchRequestWrapper<F>(exec, state,
                                                ::std::forward<F>(con));
}

/**
 * Creates an LSS 'switch state selective' request with a completion task. The
 * request deletes itself after it is completed, so it MUST NOT be deleted once
 * it is submitted to an LSS master.
 *
 * @param exec    the executor used to execute the completion task.
 * @param address the address of the LSS slave device to be switched into the
 *                configuration state.
 * @param con     the confirmation function to be called on completion of the
 *                LSS request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::LssSwitchSelectiveRequestWrapper<F>*>::type
make_lss_switch_selective_request(ev_exec_t* exec, const LssAddress& address,
                                  F&& con) {
  return new detail::LssSwitchSelectiveRequestWrapper<F>(
      exec, address, ::std::forward<F>(con));
}

/**
 * Creates an LSS 'configure node-ID' request with a completion task. The
 * request deletes itself after it is completed, so it MUST NOT be deleted once
 * it is submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param id   the requested pending node-ID of the LSS slave device.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::LssSetIdRequestWrapper<F>*>::type
make_lss_set_id_request(ev_exec_t* exec, uint8_t id, F&& con) {
  // clang-format on
  return new detail::LssSetIdRequestWrapper<F>(exec, id,
                                               ::std::forward<F>(con));
}

/**
 * Creates an LSS 'configure bit timing parameters' request with a completion
 * task. The request deletes itself after it is completed, so it MUST NOT be
 * deleted once it is submitted to an LSS master.
 *
 * @param exec    the executor used to execute the completion task.
 * @param bitrate the requested pending bit rate (in bit/s) of the LSS slave
 *                device.
 * @param con     the confirmation function to be called on completion of the
 *                LSS request.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::LssSetBitrateRequestWrapper<F>*>::type
make_lss_set_bitrate_request(ev_exec_t* exec, int bitrate, F&& con) {
  // clang-format on
  return new detail::LssSetBitrateRequestWrapper<F>(exec, bitrate,
                                                    ::std::forward<F>(con));
}

/**
 * Creates an LSS 'activate bit timing parameters' request with a completion
 * task. The request deletes itself after it is completed, so it MUST NOT be
 * deleted once it is submitted to an LSS master.
 *
 * @param exec  the executor used to execute the completion task.
 * @param delay the delay (in milliseconds) before and after the switch, during
 *              which CAN frames MUST NOT be sent.
 * @param con   the confirmation function to be called on completion of the LSS
 *              request.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::LssSwitchBitrateRequestWrapper<F>*>::type
make_lss_switch_bitrate_request(ev_exec_t* exec, int delay, F&& con) {
  // clang-format on
  return new detail::LssSwitchBitrateRequestWrapper<F>(exec, delay,
                                                       ::std::forward<F>(con));
}

/**
 * Creates an LSS 'store configuration' request with a completion task. The
 * request deletes itself after it is completed, so it MUST NOT be deleted once
 * it is submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::LssStoreRequestWrapper<F>*>::type
make_lss_store_request(ev_exec_t* exec, F&& con) {
  // clang-format on
  return new detail::LssStoreRequestWrapper<F>(exec, ::std::forward<F>(con));
}

/**
 * Creates an LSS 'inquire identity vendor-ID' request with a completion task.
 * The request deletes itself after it is completed, so it MUST NOT be deleted
 * once it is submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, uint32_t>::value,
    detail::LssGetVendorIdRequestWrapper<F>*>::type
make_lss_get_vendor_id_request(ev_exec_t* exec, F&& con) {
  return new detail::LssGetVendorIdRequestWrapper<F>(exec,
                                                     ::std::forward<F>(con));
}

/**
 * Creates an LSS 'inquire identity product-code' request with a completion
 * task. The request deletes itself after it is completed, so it MUST NOT be
 * deleted once it is submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, uint32_t>::value,
    detail::LssGetProductCodeRequestWrapper<F>*>::type
make_lss_get_product_code_request(ev_exec_t* exec, F&& con) {
  return new detail::LssGetProductCodeRequestWrapper<F>(exec,
                                                        ::std::forward<F>(con));
}

/**
 * Creates an LSS 'inquire identity revision-number' request with a completion
 * task. The request deletes itself after it is completed, so it MUST NOT be
 * deleted once it is submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, uint32_t>::value,
    detail::LssGetRevisionRequestWrapper<F>*>::type
make_lss_get_revision_request(ev_exec_t* exec, F&& con) {
  return new detail::LssGetRevisionRequestWrapper<F>(exec,
                                                     ::std::forward<F>(con));
}

/**
 * Creates an LSS 'inquire identity serial-number' request with a completion
 * task. The request deletes itself after it is completed, so it MUST NOT be
 * deleted once it is submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, uint32_t>::value,
    detail::LssGetSerialNrRequestWrapper<F>*>::type
make_lss_get_serial_nr_request(ev_exec_t* exec, F&& con) {
  return new detail::LssGetSerialNrRequestWrapper<F>(exec,
                                                     ::std::forward<F>(con));
}

/**
 * Creates an LSS 'inquire node-ID' request with a completion task. The request
 * deletes itself after it is completed, so it MUST NOT be deleted once it is
 * submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, uint8_t>::value,
    detail::LssGetIdRequestWrapper<F>*>::type
make_lss_get_id_request(ev_exec_t* exec, F&& con) {
  return new detail::LssGetIdRequestWrapper<F>(exec, ::std::forward<F>(con));
}

/**
 * Creates an LSS 'identify non-configured remote slave' request with a
 * completion task. The request deletes itself after it is completed, so it MUST
 * NOT be deleted once it is submitted to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, bool>::value,
    detail::LssIdNonConfigRequestWrapper<F>*>::type
make_lss_id_non_config_request(ev_exec_t* exec, F&& con) {
  // clang-format on
  return new detail::LssIdNonConfigRequestWrapper<F>(exec,
                                                     ::std::forward<F>(con));
}

/**
 * Creates an 'LSS Slowscan' request with a completion task. The request deletes
 * itself after it is completed, so it MUST NOT be deleted once it is submitted
 * to an LSS master.
 *
 * @param exec the executor used to execute the completion task.
 * @param lo   the lower bound of the LSS address of the slave device.
 * @param hi   the upper bound of the LSS address of the slave device. The
 *             vendor-ID and product code MUST be equal to those in <b>lo</b>.
 * @param con  the confirmation function to be called on completion of the LSS
 *             request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, LssAddress>::value,
    detail::LssSlowscanRequestWrapper<F>*>::type
make_lss_slowscan_request(ev_exec_t* exec, const LssAddress& lo,
                          const LssAddress& hi, F&& con) {
  return new detail::LssSlowscanRequestWrapper<F>(exec, lo, hi,
                                                  ::std::forward<F>(con));
}

/**
 * Creates an 'LSS Fastscan' request with a completion task. The request deletes
 * itself after it is completed, so it MUST NOT be deleted once it is submitted
 * to an LSS master.
 *
 * @param exec    the executor used to execute the completion task.
 * @param address the bits of the LSS address of a slave device which are
 *                already known and can be skipped during scanning.
 * @param mask    a mask specifying which bits in <b>address</b> are already
 *                known and can be skipped during scanning. If a bit in the mask
 *                is 1, the corresponding bit in <b>address</b> is _not_
 *                checked.
 * @param con     the confirmation function to be called on completion of the
 *                LSS request.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code, LssAddress>::value,
    detail::LssFastscanRequestWrapper<F>*>::type
make_lss_fastscan_request(ev_exec_t* exec, const LssAddress& address,
                          const LssAddress& mask, F&& con) {
  return new detail::LssFastscanRequestWrapper<F>(exec, address, mask,
                                                  ::std::forward<F>(con));
}

/**
 * The base class for CANopen LSS masters.
 *
 * This class inherits the mutex protecting the corresponding CANopen master
 * node.
 */
class LssMaster : protected util::BasicLockable {
  friend class detail::LssSwitchRequestBase;
  friend class detail::LssSwitchSelectiveRequestBase;
  friend class detail::LssSetIdRequestBase;
  friend class detail::LssSetBitrateRequestBase;
  friend class detail::LssSwitchBitrateRequestBase;
  friend class detail::LssStoreRequestBase;
  friend class detail::LssGetVendorIdRequestBase;
  friend class detail::LssGetProductCodeRequestBase;
  friend class detail::LssGetRevisionRequestBase;
  friend class detail::LssGetSerialNrRequestBase;
  friend class detail::LssGetIdRequestBase;
  friend class detail::LssIdNonConfigRequestBase;
  friend class detail::LssSlowscanRequestBase;
  friend class detail::LssFastscanRequestBase;

 public:
  /**
   * Creates a new CANopen LSS master.
   *
   * @param exec the executor used to execute LSS requests. If <b>exec</b> is a
   *             null pointer, the executor of the CANopen node is used.
   * @param node a CANopen master node.
   * @param ctrl a pointer to the CAN controller for <b>node</b> (can be a null
   *             pointer).
   */
  explicit LssMaster(ev_exec_t* exec, Node& node,
                     io::CanControllerBase* ctrl = nullptr);

  /// Creates a new CANopen LSS master.
  explicit LssMaster(Node& node, io::CanControllerBase* ctrl = nullptr)
      : LssMaster(nullptr, node, ctrl) {}

  LssMaster(const LssMaster&) = delete;
  LssMaster& operator=(const LssMaster&) = delete;

  virtual ~LssMaster();

  /**
   * Returns the default executor used to execute completion tasks of LSS
   * requests.
   */
  ev::Executor GetExecutor() const noexcept;

  /// Returns the CANopen master node.
  Node& GetNode() const noexcept;

  /**
   * Returns the pointer to the CAN controller for this node passed to the
   * constructor (may be a null pointer).
   */
  io::CanControllerBase* GetController() const noexcept;

  /// Returns the inhibit time between successive CAN frames. @see SetInhibit()
  ::std::chrono::microseconds GetInhibit() const;

  /// Sets the inhibit time between successive CAN frames. @see GetInhibit()
  void SetInhibit(const ::std::chrono::microseconds& inhibit);

  /**
   * Returns the timeout when waiting for a slave to respond to an LSS request.
   *
   * @see SetTimeout()
   */
  ::std::chrono::milliseconds GetTimeout() const;

  /**
   * Sets the timeout when waiting for a slave to respond to an LSS request.
   *
   * @see SetTimeout()
   */
  void SetTimeout(const ::std::chrono::milliseconds& timeout);

  /**
   * Queues an LSS 'switch state global' request. This function switches all
   * slave devices to the specified LSS state.
   *
   * @param req   the request to be submitted.
   * @param state the desired state of the LSS slave devices.
   */
  void
  SubmitSwitch(detail::LssSwitchRequestBase& req,
               LssState state = LssState::WAITING) {
    req.state = state;
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'switch state global' request. This function
   * switches all slave devices to the specified LSS state.
   *
   * @param exec  the executor used to execute the completion task.
   * @param state the desired state of the LSS slave devices.
   * @param con   the confirmation function to be called on completion of the
   *              LSS request.
   */
  template <class F>
  void
  SubmitSwitch(ev_exec_t* exec, LssState state, F&& con) {
    Submit(*make_lss_switch_request(exec, state, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitSwitch(nullptr, state, con)`.
  template <class F>
  void
  SubmitSwitch(LssState state, F&& con) {
    SubmitSwitch(nullptr, state, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'switch state global' request. @see Cancel()
  bool
  CancelSwitch(detail::LssSwitchRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'switch state global' request. @see Abort()
  bool
  AbortSwitch(detail::LssSwitchRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'switch state global' request and creates a
   * future which becomes ready once the request completes (or is canceled).
   *
   * @param exec  the executor used to execute the completion task.
   * @param state the desired state of the LSS slave devices.
   * @param preq  the address at which to store a pointer to the request (can be
   *              a null pointer).
   *
   * @returns an empty future on success, or a future containing an
   * std::system_error on failure.
   */
  LssFuture<void> AsyncSwitch(ev_exec_t* exec,
                              LssState state = LssState::WAITING,
                              detail::LssSwitchRequestBase** preq = nullptr);

  /// Equivalent to `AsyncSwitch(nullptr, state, preq)`.
  LssFuture<void>
  AsyncSwitch(LssState state = LssState::WAITING,
              detail::LssSwitchRequestBase** preq = nullptr) {
    return AsyncSwitch(nullptr, state, preq);
  }

  /**
   * Queues an LSS 'switch state selective' request. This function switches the
   * slave device with the specified LSS address to the LSS configuration state.
   *
   * @param req     the request to be submitted.
   * @param address the address of the LSS slave device to be switched into the
   *                configuration state.
   */
  void
  SubmitSwitchSelective(detail::LssSwitchSelectiveRequestBase& req,
                        const LssAddress& address) {
    req.address = address;
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'switch state selective' request. This function
   * switches the slave device with the specified LSS address to the LSS
   * configuration state.
   *
   * @param exec    the executor used to execute the completion task.
   * @param address the address of the LSS slave device to be switched into the
   *                configuration state.
   * @param con     the confirmation function to be called on completion of the
   *                LSS request.
   */
  template <class F>
  void
  SubmitSwitchSelective(ev_exec_t* exec, const LssAddress& address, F&& con) {
    Submit(*make_lss_switch_selective_request(exec, address,
                                              ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitSwitchSelective(nullptr, state, con)`.
  template <class F>
  void
  SubmitSwitchSelective(const LssAddress& address, F&& con) {
    SubmitSwitchSelective(nullptr, address, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'switch state selective' request. @see Cancel()
  bool
  CancelSwitchSelective(detail::LssSwitchSelectiveRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'switch state selective' request. @see Abort()
  bool
  AbortSwitchSelective(detail::LssSwitchSelectiveRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'switch state selective' request and creates a
   * future which becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param address the address of the LSS slave device to be switched into the
   *                configuration state.
   * @param preq    the address at which to store a pointer to the request (can
   *                be a null pointer).
   *
   * @returns an empty future on success, or a future containing an
   * std::system_error on failure.
   */
  LssFuture<void> AsyncSwitchSelective(
      ev_exec_t* exec, const LssAddress& address,
      detail::LssSwitchSelectiveRequestBase** preq = nullptr);

  /// Equivalent to `AsyncSwitchSelective(nullptr, address, preq)`.
  LssFuture<void>
  AsyncSwitchSelective(const LssAddress& address,
                       detail::LssSwitchSelectiveRequestBase** preq = nullptr) {
    return AsyncSwitchSelective(nullptr, address, preq);
  }

  /**
   * Queues an LSS 'configure node-ID' request. This function configures the
   * pending node-ID of an LSS slave device. It is the responsibility of the
   * caller to ensure that a single slave device is in the LSS configuration
   * state.
   *
   * @param req the request to be submitted.
   * @param id  the requested pending node-ID of the LSS slave device.
   */
  void
  SubmitSetId(detail::LssSetIdRequestBase& req, uint8_t id) {
    req.id = id;
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'configure node-ID' request. This function
   * configures the pending node-ID of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param id   the requested pending node-ID of the LSS slave device.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitSetId(ev_exec_t* exec, uint8_t id, F&& con) {
    Submit(*make_lss_set_id_request(exec, id, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitSetId(nullptr, id, con)`.
  template <class F>
  void
  SubmitSetId(uint8_t id, F&& con) {
    SubmitSetId(nullptr, id, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'configure node-ID' request. @see Cancel()
  bool
  CancelSetId(detail::LssSetIdRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'configure node-ID' request. @see Abort()
  bool
  AbortSetId(detail::LssSetIdRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'configure node-ID' request and creates a future
   * which becomes ready once the request completes (or is canceled). It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param id   the requested pending node-ID of the LSS slave device.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns an empty future on success, or a future containing an
   * std::system_error on failure.
   */
  LssFuture<void> AsyncSetId(ev_exec_t* exec, uint8_t id,
                             detail::LssSetIdRequestBase** preq = nullptr);

  /// Equivalent to `AsyncSetId(nullptr, id, preq)`.
  LssFuture<void>
  AsyncSetId(uint8_t id, detail::LssSetIdRequestBase** preq = nullptr) {
    return AsyncSetId(nullptr, id, preq);
  }

  /**
   * Queues an LSS 'configure bit timing parameters' request. This function
   * configures the pending bit rate of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param req     the request to be submitted.
   * @param bitrate the requested pending bit rate (in bit/s) of the LSS slave
   *                device.
   */
  void
  SubmitSetBitrate(detail::LssSetBitrateRequestBase& req, int bitrate) {
    req.bitrate = bitrate;
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'configure bit timing parameters' request. This
   * function configures the pending bit rate of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec    the executor used to execute the completion task.
   * @param bitrate the requested pending bit rate (in bit/s) of the LSS slave
   *                device.
   * @param con     the confirmation function to be called on completion of the
   *                LSS request.
   */
  template <class F>
  void
  SubmitSetBitrate(ev_exec_t* exec, int bitrate, F&& con) {
    Submit(
        *make_lss_set_bitrate_request(exec, bitrate, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitSetBitrate(nullptr, id, con)`.
  template <class F>
  void
  SubmitSetBitrate(int bitrate, F&& con) {
    SubmitSetBitrate(nullptr, bitrate, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'configure bit timing parameters' request. @see Cancel()
  bool
  CancelSetBitrate(detail::LssSetBitrateRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'configure bit timing parameters' request. @see Abort()
  bool
  AbortSetBitrate(detail::LssSetBitrateRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'configure bit timing parameters' request and
   * creates a future which becomes ready once the request completes (or is
   * canceled). It is the responsibility of the caller to ensure that a single
   * slave device is in the LSS configuration state.
   *
   * @param exec    the executor used to execute the completion task.
   * @param bitrate the requested pending bit rate (in bit/s) of the LSS slave
   *                device.
   * @param preq    the address at which to store a pointer to the request (can
   *                be a null pointer).
   *
   * @returns an empty future on success, or a future containing an
   * std::system_error on failure.
   */
  LssFuture<void> AsyncSetBitrate(
      ev_exec_t* exec, int bitrate,
      detail::LssSetBitrateRequestBase** preq = nullptr);

  /// Equivalent to `AsyncSetBitrate(nullptr, id, preq)`.
  LssFuture<void>
  AsyncSetBitrate(int bitrate,
                  detail::LssSetBitrateRequestBase** preq = nullptr) {
    return AsyncSetBitrate(nullptr, bitrate, preq);
  }

  /**
   * Queues an LSS 'activate bit timing parameters' request. This function
   * configures the pending bit rate of an LSS slave device. It is the
   * responsibility of the caller to ensure that all slave devices are in the
   * LSS configuration state.
   *
   * @param req   the request to be submitted.
   * @param delay the requested pending bit rate (in bit/s) of the LSS slave
   *              device.
   */
  void
  SubmitSwitchBitrate(detail::LssSwitchBitrateRequestBase& req, int delay) {
    req.delay = delay;
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'activate bit timing parameters' request. This
   * function activates the bit rate of all CANopen devices in the network. It
   * is the responsibility of the caller to ensure that all slave devices are in
   * the LSS configuration state.
   *
   * @param exec  the executor used to execute the completion task.
   * @param delay the delay (in milliseconds) before and after the switch,
   * during which CAN frames MUST NOT be sent.
   * @param con   the confirmation function to be called on completion of the
   *              LSS request.
   */
  template <class F>
  void
  SubmitSwitchBitrate(ev_exec_t* exec, int delay, F&& con) {
    Submit(
        *make_lss_switch_bitrate_request(exec, delay, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitSwitchBitrate(nullptr, id, con)`.
  template <class F>
  void
  SubmitSwitchBitrate(int delay, F&& con) {
    SubmitSwitchBitrate(nullptr, delay, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'activate bit timing parameters' request. @see Cancel()
  bool
  CancelSwitchBitrate(detail::LssSwitchBitrateRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'activate bit timing parameters' request. @see Abort()
  bool
  AbortSwitchBitrate(detail::LssSwitchBitrateRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'activate bit timing parameters' request and
   * creates a future which becomes ready once the request completes (or is
   * canceled). It is the responsibility of the caller to ensure that all slave
   * devices are in the LSS configuration state.
   *
   * @param exec  the executor used to execute the completion task.
   * @param delay the delay (in milliseconds) before and after the switch,
   *              during which CAN frames MUST NOT be sent.
   * @param preq  the address at which to store a pointer to the request (can be
   *              a null pointer).
   *
   * @returns an empty future on success, or a future containing an
   * std::system_error on failure.
   */
  LssFuture<void> AsyncSwitchBitrate(
      ev_exec_t* exec, int delay,
      detail::LssSwitchBitrateRequestBase** preq = nullptr);

  /// Equivalent to `AsyncSwitchBitrate(nullptr, id, preq)`.
  LssFuture<void>
  AsyncSwitchBitrate(int delay,
                     detail::LssSwitchBitrateRequestBase** preq = nullptr) {
    return AsyncSwitchBitrate(nullptr, delay, preq);
  }

  /**
   * Queues an LSS 'store configuration' request. This function configures the
   * pending node-ID of an LSS slave device. It is the responsibility of the
   * caller to ensure that a single slave device is in the LSS configuration
   * state.
   */
  void
  SubmitStore(detail::LssStoreRequestBase& req) {
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'store configuration' request. This function
   * configures the pending node-ID of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitStore(ev_exec_t* exec, F&& con) {
    Submit(*make_lss_store_request(exec, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitStore(nullptr, con)`.
  template <class F>
  void
  SubmitStore(F&& con) {
    SubmitStore(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'store configuration' request. @see Cancel()
  bool
  CancelStore(detail::LssStoreRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'store configuration' request. @see Abort()
  bool
  AbortStore(detail::LssStoreRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'store configuration' request and creates a
   * future which becomes ready once the request completes (or is canceled). It
   * is the responsibility of the caller to ensure that a single slave device is
   * in the LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns an empty future on success, or a future containing an
   * std::system_error on failure.
   */
  LssFuture<void> AsyncStore(ev_exec_t* exec,
                             detail::LssStoreRequestBase** preq = nullptr);

  /// Equivalent to `AsyncStore(nullptr, preq)`.
  LssFuture<void>
  AsyncStore(detail::LssStoreRequestBase** preq = nullptr) {
    return AsyncStore(nullptr, preq);
  }

  /**
   * Queues an LSS 'inquire identity vendor-ID' request. This function
   * inquires the vendor-ID of an LSS slave device. It is the responsibility of
   * the caller to ensure that a single slave device is in the LSS configuration
   * state.
   */
  void
  SubmitGetVendorId(detail::LssGetVendorIdRequestBase& req) {
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'inquire identity vendor-ID' request. This
   * function inquires the vendor-ID of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitGetVendorId(ev_exec_t* exec, F&& con) {
    Submit(*make_lss_get_vendor_id_request(exec, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitGetVendorId(nullptr, con)`.
  template <class F>
  void
  SubmitGetVendorId(F&& con) {
    SubmitGetVendorId(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'inquire identity vendor-ID' request. @see Cancel()
  bool
  CancelGetVendorId(detail::LssGetVendorIdRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'inquire identity vendor-ID' request. @see Abort()
  bool
  AbortGetVendorId(detail::LssGetVendorIdRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'inquire identity vendor-ID' request and creates
   * a future which becomes ready once the request completes (or is canceled).
   * It is the responsibility of the caller to ensure that a single slave device
   * is in the LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns a future which holds the vendor-ID on success, or an
   * std::system_error on failure.
   */
  LssFuture<uint32_t> AsyncGetVendorId(
      ev_exec_t* exec, detail::LssGetVendorIdRequestBase** preq = nullptr);

  /// Equivalent to `AsyncGetVendorId(nullptr, preq)`.
  LssFuture<uint32_t>
  AsyncGetVendorId(detail::LssGetVendorIdRequestBase** preq = nullptr) {
    return AsyncGetVendorId(nullptr, preq);
  }

  /**
   * Queues an LSS 'inquire identity product-code' request. This function
   * inquires the product code of an LSS slave device. It is the responsibility
   * of the caller to ensure that a single slave device is in the LSS
   * configuration state.
   */
  void
  SubmitGetProductCode(detail::LssGetProductCodeRequestBase& req) {
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'inquire identity product-code' request. This
   * function inquires the product code of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitGetProductCode(ev_exec_t* exec, F&& con) {
    Submit(*make_lss_get_product_code_request(exec, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitGetProductCode(nullptr, con)`.
  template <class F>
  void
  SubmitGetProductCode(F&& con) {
    SubmitGetProductCode(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'inquire identity product-code' request. @see Cancel()
  bool
  CancelGetProductCode(detail::LssGetProductCodeRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'inquire identity product-code' request. @see Abort()
  bool
  AbortGetProductCode(detail::LssGetProductCodeRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'inquire identity product-code' request and
   * creates a future which becomes ready once the request completes (or is
   * canceled). It is the responsibility of the caller to ensure that a single
   * slave device is in the LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns a future which holds the product code on success, or an
   * std::system_error on failure.
   */
  LssFuture<uint32_t> AsyncGetProductCode(
      ev_exec_t* exec, detail::LssGetProductCodeRequestBase** preq = nullptr);

  /// Equivalent to `AsyncGetProductCode(nullptr, preq)`.
  LssFuture<uint32_t>
  AsyncGetProductCode(detail::LssGetProductCodeRequestBase** preq = nullptr) {
    return AsyncGetProductCode(nullptr, preq);
  }

  /**
   * Queues an LSS 'inquire identity revision-number' request. This function
   * inquires the revision number of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   */
  void
  SubmitGetRevision(detail::LssGetRevisionRequestBase& req) {
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'inquire identity revision-number' request. This
   * function inquires the revision number of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitGetRevision(ev_exec_t* exec, F&& con) {
    Submit(*make_lss_get_revision_request(exec, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitGetRevision(nullptr, con)`.
  template <class F>
  void
  SubmitGetRevision(F&& con) {
    SubmitGetRevision(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'inquire identity revision-number' request. @see Cancel()
  bool
  CancelGetRevision(detail::LssGetRevisionRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'inquire identity revision-number' request. @see Abort()
  bool
  AbortGetRevision(detail::LssGetRevisionRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'inquire identity revision-number' request and
   * creates a future which becomes ready once the request completes (or is
   * canceled). It is the responsibility of the caller to ensure that a single
   * slave device is in the LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns a future which holds the revision number on success, or an
   * std::system_error on failure.
   */
  LssFuture<uint32_t> AsyncGetRevision(
      ev_exec_t* exec, detail::LssGetRevisionRequestBase** preq = nullptr);

  /// Equivalent to `AsyncGetRevision(nullptr, preq)`.
  LssFuture<uint32_t>
  AsyncGetRevision(detail::LssGetRevisionRequestBase** preq = nullptr) {
    return AsyncGetRevision(nullptr, preq);
  }

  /**
   * Queues an LSS 'inquire identity serial-number' request. This function
   * inquires the serial number of an LSS slave device. It is the responsibility
   * of the caller to ensure that a single slave device is in the LSS
   * configuration state.
   */
  void
  SubmitGetSerialNr(detail::LssGetSerialNrRequestBase& req) {
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'inquire identity serial-number' request. This
   * function inquires the serial number of an LSS slave device. It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitGetSerialNr(ev_exec_t* exec, F&& con) {
    Submit(*make_lss_get_serial_nr_request(exec, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitGetSerialNr(nullptr, con)`.
  template <class F>
  void
  SubmitGetSerialNr(F&& con) {
    SubmitGetSerialNr(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'inquire identity serial-number' request. @see Cancel()
  bool
  CancelGetSerialNr(detail::LssGetSerialNrRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'inquire identity serial-number' request. @see Abort()
  bool
  AbortGetSerialNr(detail::LssGetSerialNrRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'inquire identity serial-number' request and
   * creates a future which becomes ready once the request completes (or is
   * canceled). It is the responsibility of the caller to ensure that a single
   * slave device is in the LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns a future which holds the serial number on success, or an
   * std::system_error on failure.
   */
  LssFuture<uint32_t> AsyncGetSerialNr(
      ev_exec_t* exec, detail::LssGetSerialNrRequestBase** preq = nullptr);

  /// Equivalent to `AsyncGetSerialNr(nullptr, preq)`.
  LssFuture<uint32_t>
  AsyncGetSerialNr(detail::LssGetSerialNrRequestBase** preq = nullptr) {
    return AsyncGetSerialNr(nullptr, preq);
  }

  /**
   * Queues an LSS 'inquire node-ID' request. This function inquires the active
   * node-ID of an LSS slave device. It is the responsibility of the caller to
   * ensure that a single slave device is in the LSS configuration state.
   */
  void
  SubmitGetId(detail::LssGetIdRequestBase& req) {
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'inquire node-ID' request. This function inquires
   * the active node-ID of an LSS slave device. It is the responsibility of the
   * caller to ensure that a single slave device is in the LSS configuration
   * state.
   *
   * @param exec the executor used to execute the completion task.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitGetId(ev_exec_t* exec, F&& con) {
    Submit(*make_lss_get_id_request(exec, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitGetId(nullptr, con)`.
  template <class F>
  void
  SubmitGetId(F&& con) {
    SubmitGetId(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an LSS 'inquire node-ID' request. @see Cancel()
  bool
  CancelGetId(detail::LssGetIdRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'inquire node-ID' request. @see Abort()
  bool
  AbortGetId(detail::LssGetIdRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'inquire node-ID' request and creates a future
   * which becomes ready once the request completes (or is canceled). It is the
   * responsibility of the caller to ensure that a single slave device is in the
   * LSS configuration state.
   *
   * @param exec the executor used to execute the completion task.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns a future which holds the node-ID on success, or an
   * std::system_error on failure.
   */
  LssFuture<uint8_t> AsyncGetId(ev_exec_t* exec,
                                detail::LssGetIdRequestBase** preq = nullptr);

  /// Equivalent to `AsyncGetId(nullptr, preq)`.
  LssFuture<uint8_t>
  AsyncGetId(detail::LssGetIdRequestBase** preq = nullptr) {
    return AsyncGetId(nullptr, preq);
  }

  /**
   * Queues an LSS 'identify non-configured remote slave' request. This function
   * checks if there is an LSS slave device with an invalid (255) pending
   * node-ID and no active node-ID.
   */
  void
  SubmitIdNonConfig(detail::LssIdNonConfigRequestBase& req) {
    Submit(req);
  }

  /**
   * Creates and queues an LSS 'identify non-configured remote slave' request.
   * This function checks if there is an LSS slave device with an invalid (255)
   * pending node-ID and no active node-ID.
   *
   * @param exec the executor used to execute the completion task.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitIdNonConfig(ev_exec_t* exec, F&& con) {
    Submit(*make_lss_id_non_config_request(exec, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitIdNonConfig(nullptr, con)`.
  template <class F>
  void
  SubmitIdNonConfig(F&& con) {
    SubmitIdNonConfig(nullptr, ::std::forward<F>(con));
  }

  /**
   * Cancels an LSS 'identify non-configured remote slave' request.
   *
   * @see Cancel()
   */
  bool
  CancelIdNonConfig(detail::LssIdNonConfigRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an LSS 'identify non-configured remote slave' request. @see Abort()
  bool
  AbortIdNonConfig(detail::LssIdNonConfigRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous LSS 'identify non-configured remote slave' request
   * and creates a future which becomes ready once the request completes (or is
   * canceled).
   *
   * @param exec the executor used to execute the completion task.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns a future which holds a boolean specfying whether a non-configured
   * slave was found on success, or an std::system_error on failure.
   */
  LssFuture<bool> AsyncIdNonConfig(
      ev_exec_t* exec, detail::LssIdNonConfigRequestBase** preq = nullptr);

  /// Equivalent to `AsyncIdNonConfig(nullptr, preq)`.
  LssFuture<bool>
  AsyncIdNonConfig(detail::LssIdNonConfigRequestBase** preq = nullptr) {
    return AsyncIdNonConfig(nullptr, preq);
  }

  /**
   * Queues an 'LSS Slowscan' request. This function performs a binary search
   * using the 'identify remote slave' service to obtain a single LSS address,
   * followed by the 'switch state selective' service if a slave was found.
   *
   * @param req the request to be submitted.
   * @param lo  the lower bound of the LSS address of the slave device.
   * @param hi   the upper bound of the LSS address of the slave device. The
   *             vendor-ID and product code MUST be equal to those in <b>lo</b>.
   */
  void
  SubmitSlowscan(detail::LssSlowscanRequestBase& req, const LssAddress& lo,
                 const LssAddress& hi) {
    req.lo = lo;
    req.hi = hi;
    Submit(req);
  }

  /**
   * Creates and queues an 'LSS Slowscan' request. This function performs a
   * binary search using the 'identify remote slave' service to obtain a single
   * LSS address, followed by the 'switch state selective' service if a slave
   * was found.
   *
   * @param exec the executor used to execute the completion task.
   * @param lo   the lower bound of the LSS address of the slave device.
   * @param hi   the upper bound of the LSS address of the slave device. The
   *             vendor-ID and product code MUST be equal to those in <b>lo</b>.
   * @param con  the confirmation function to be called on completion of the LSS
   *             request.
   */
  template <class F>
  void
  SubmitSlowscan(ev_exec_t* exec, const LssAddress& lo, const LssAddress& hi,
                 F&& con) {
    Submit(*make_lss_slowscan_request(exec, lo, hi, ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitSlowscan(nullptr, con)`.
  template <class F>
  void
  SubmitSlowscan(F&& con) {
    SubmitSlowscan(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an 'LSS Slowscan' request. @see Cancel()
  bool
  CancelSlowscan(detail::LssSlowscanRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an 'LSS Slowscan' request. @see Abort()
  bool
  AbortSlowscan(detail::LssSlowscanRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous 'LSS Slowscan' request and creates a future which
   * becomes ready once the request completes (or is canceled).
   *
   * @param exec the executor used to execute the completion task.
   * @param lo   the lower bound of the LSS address of the slave device.
   * @param hi   the upper bound of the LSS address of the slave device. The
   *             vendor-ID and product code MUST be equal to those in <b>lo</b>.
   * @param preq the address at which to store a pointer to the request (can be
   *             a null pointer).
   *
   * @returns a future which holds the LSS address of the slave if found, or an
   * std::system_error if not (containing the std::errc::timed_out error code)
   * or if an error occurred.
   */
  LssFuture<LssAddress> AsyncSlowscan(
      ev_exec_t* exec, const LssAddress& lo, const LssAddress& hi,
      detail::LssSlowscanRequestBase** preq = nullptr);

  /// Equivalent to `AsyncSlowscan(nullptr, lo, hi, preq)`.
  LssFuture<LssAddress>
  AsyncSlowscan(const LssAddress& lo, const LssAddress& hi,
                detail::LssSlowscanRequestBase** preq = nullptr) {
    return AsyncSlowscan(nullptr, lo, hi, preq);
  }

  /**
   * Queues an 'LSS Fastscan' request. This function scans the bits in an LSS
   * address to find a single matching slave. If a slave is identified, it is
   * switched to the LSS configuration state.
   *
   * @param         req the request to be submitted.
   * @param address the bits of the LSS address of a slave device which are
   *                already known and can be skipped during scanning.
   * @param mask    a mask specifying which bits in <b>address</b> are already
   *                known and can be skipped during scanning. If a bit in the
   *                mask is 1, the corresponding bit in <b>address</b> is _not_
   *                checked.
   */
  void
  SubmitFastscan(detail::LssFastscanRequestBase& req, const LssAddress& address,
                 const LssAddress& mask) {
    req.address = address;
    req.mask = mask;
    Submit(req);
  }

  /**
   * Creates and queues an 'LSS Fastscan' request. This function scans the bits
   * in an LSS address to find a single matching slave. If a slave is
   * identified, it is switched to the LSS configuration state.
   *
   * @param exec    the executor used to execute the completion task.
   * @param address the bits of the LSS address of a slave device which are
   *                already known and can be skipped during scanning.
   * @param mask    a mask specifying which bits in <b>address</b> are already
   *                known and can be skipped during scanning. If a bit in the
   *                mask is 1, the corresponding bit in <b>address</b> is _not_
   *                checked.
   * @param con     the confirmation function to be called on completion of the
   *                LSS request.
   */
  template <class F>
  void
  SubmitFastscan(ev_exec_t* exec, const LssAddress& address,
                 const LssAddress& mask, F&& con) {
    Submit(*make_lss_fastscan_request(exec, address, mask,
                                      ::std::forward<F>(con)));
  }

  /// Equivalent to `SubmitFastscan(nullptr, con)`.
  template <class F>
  void
  SubmitFastscan(F&& con) {
    SubmitFastscan(nullptr, ::std::forward<F>(con));
  }

  /// Cancels an 'LSS Fastscan' request. @see Cancel()
  bool
  CancelFastscan(detail::LssFastscanRequestBase& req) {
    return Cancel(req);
  }

  /// Aborts an 'LSS Fastscan' request. @see Abort()
  bool
  AbortFastscan(detail::LssFastscanRequestBase& req) {
    return Abort(req);
  }

  /**
   * Queues an asynchronous 'LSS Fastscan' request and creates a future which
   * becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param address the bits of the LSS address of a slave device which are
   *                already known and can be skipped during scanning.
   * @param mask    a mask specifying which bits in <b>address</b> are already
   *                known and can be skipped during scanning. If a bit in the
   *                mask is 1, the corresponding bit in <b>address</b> is _not_
   *                checked.
   * @param preq    the address at which to store a pointer to the request (can
   *                be a null pointer).
   *
   * @returns a future which holds the LSS address of the slave if found, or an
   * std::system_error if not (containing the std::errc::timed_out error code)
   * or if an error occurred.
   */
  LssFuture<LssAddress> AsyncFastscan(
      ev_exec_t* exec, const LssAddress& address = {0, 0, 0, 0},
      const LssAddress& mask = {0, 0, 0, 0},
      detail::LssFastscanRequestBase** preq = nullptr);

  /// Equivalent to `AsyncFastscan(nullptr, address, mask, preq)`.
  LssFuture<LssAddress>
  AsyncFastscan(const LssAddress& address = {0, 0, 0, 0},
                const LssAddress& mask = {0, 0, 0, 0},
                detail::LssFastscanRequestBase** preq = nullptr) {
    return AsyncFastscan(nullptr, address, mask, preq);
  }

  /// Queues an LSS request.
  void Submit(detail::LssRequestBase& req);

  /**
   * Cancels a pending LSS request. If the request was canceled, the completion
   * task is submitted for execution with `ec == std::errc::operation_canceled`.
   *
   * @returns true if the request was canceled, and false if it is ongoing or
   * already completed.
   */
  bool Cancel(detail::LssRequestBase& req);

  /**
   * Cancels all pending LSS requests and stops the ongoing request, if any. The
   * completion tasks of canceled requests are submitted for execution with
   * `ec == std::errc::operation_canceled`
   *
   * @returns the number of canceled requests.
   */
  ::std::size_t CancelAll();

  /**
   * Aborts a pending LSS request. The completion task is _not_ submitted for
   * execution.
   *
   * @returns true if the request was aborted, and false if it is ongoing or
   * already completed.
   */
  bool Abort(detail::LssRequestBase& req);

  /**
   * Aborts all pending LSS requests. The completion tasks of aborted requests
   * are _not_ submitted for execution.
   *
   * @returns the number of aborted requests.
   */
  ::std::size_t AbortAll();

  /**
   * The function invoked when the LSS master services are executed during the
   * NMT startup process. The startup process is halted until all LSS requests
   * complete.
   *
   * The default implementation issues no LSS requests.
   *
   * @param res the function to onvoke on completion of the LSS requests. The
   *            arggument to <b>res</b> is the result: 0 on succes, or an error
   *            code on failure.
   */
  virtual void
  OnStart(::std::function<void(::std::error_code ec)> res) noexcept {
    res(::std::error_code{});
  }

  /**
   * The function invoked when the master activates the bit rate of all CANopen
   * devices in the network.
   *
   * If GetController() returns a valid pointer to a CAN controller, the default
   * implementation stops the controller after half a delay period has passed
   * (to give the CAN channel time to send the LSS requests), sets the bit rate
   * after another half of the delay period, and finally restarts the CAN
   * controller after the second delay period.
   *
   * @param bitrate the new bit rate (in bit/s).
   * @param delay   the delay before and after the switch, during which CAN
   *                frames MUST NOT be sent.
   * @param res     the function to invoke once the new bit rate has been
   *                activated and the delay periods have passed. The argument to
   *                <b>res</b> is the result: 0 on succes, or an error code on
   *                failure.
   */
  virtual void OnSwitchBitrate(
      int bitrate, ::std::chrono::milliseconds delay,
      ::std::function<void(::std::error_code ec)> res) noexcept;

 protected:
  void lock() final;
  void unlock() final;

  /**
   * Update the CAN network time. The mutex inherited by this class MUST be
   * locked for the duration of this call.
   */
  void SetTime();

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_LSS_MASTER_HPP_
