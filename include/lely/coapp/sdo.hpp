/*!\file
 * This header file is part of the C++ CANopen master library; it contains the
 * Client-SDO queue declarations.
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include <lely/aio/exec.hpp>
#include <lely/aio/future.hpp>
#include <lely/coapp/detail/type_traits.hpp>
#include <lely/coapp/sdo_error.hpp>

#include <chrono>
#include <memory>
#include <tuple>

namespace lely {

// The CAN network interface from <lely/can/net.hpp>.
class CANNet;

// The CANopen device from <lely/co/dev.hpp>.
class CODev;

// The CANopen Client-SDO service from <lely/co/csdo.hpp>.
class COCSDO;

namespace canopen {

//! The Client-SDO queue.
class LELY_COAPP_EXTERN Sdo {
 public:
  //! The type used to represent an SDO timeout duration.
  using duration = ::std::chrono::milliseconds;

 private:
  struct Impl_;

  class RequestBase_ : public aio_task {
    friend class Sdo;

   public:
    RequestBase_(aio::ExecutorBase& exec, aio_task_func_t *func)
        : aio_task AIO_TASK_INIT(exec, func) {}

    RequestBase_(aio::ExecutorBase& exec, aio_task_func_t *func,
                 uint16_t idx_, uint8_t subidx_, const duration& timeout_)
        : aio_task AIO_TASK_INIT(exec, func), idx(idx_), subidx(subidx_),
          timeout(timeout_) {}

    RequestBase_(const RequestBase_&) = delete;

    RequestBase_& operator=(const RequestBase_&) = delete;

    virtual ~RequestBase_() = default;

    aio::ExecutorBase
    GetExecutor() const noexcept { return aio::ExecutorBase(exec); }

    uint16_t idx { 0 };
    uint8_t subidx { 0 };
    duration timeout {};
    uint32_t ac { 0 };

   private:
    virtual void OnRequest(Impl_* impl) noexcept = 0;
  };

  template <class T>
  class DownloadRequestBase_ : public RequestBase_ {
   public:
    DownloadRequestBase_(aio::ExecutorBase& exec, aio_task_func_t *func)
        : RequestBase_(exec, func) {}

    template <class U>
    DownloadRequestBase_(aio::ExecutorBase& exec, aio_task_func_t *func,
        uint16_t idx, uint8_t subidx, U&& value_, const duration& timeout)
        : RequestBase_(exec, func, idx, subidx, timeout),
          value(::std::forward<U>(value_)) {}

    T value {};
  };

  template <class T>
  class UploadRequestBase_ : public RequestBase_ {
    friend Impl_;

   public:
    using RequestBase_::RequestBase_;

    T value {};
  };

 public:
  /*!
   * The signature of the callback function invoked on completion of an SDO
   * download request. Note that the callback function SHOULD NOT throw
   * exceptions. Since it is invoked from C, any exception that is thrown cannot
   * be caught and will result in a call to `std::terminate()`.
   *
   * \param idx    the object index.
   * \param subidx the object sub-index.
   * \param ec     the SDO abort code (0 on success).
   */
  using DownloadSignature = void(uint16_t idx, uint8_t subidx,
                                 ::std::error_code ec);

  //! An SDO download request.
  template <class T>
  class LELY_COAPP_EXTERN DownloadRequest : public DownloadRequestBase_<T> {
   public:
    /*!
     * Constructs an empty SDO download request. The object index and sub-index,
     * the value to be written and, optionally, the SDO timeout have to be set
     * before the request can be submitted.
     *
     * \param exec    the executor used to execute the confirmation function.
     * \param con     the confirmation function to be called on completion of
     *                the SDO request.
     *
     * \see Sdo::SubmitDownload<T>()
     */
    template <class F>
    DownloadRequest(aio::ExecutorBase& exec, F&& con)
        : DownloadRequestBase_<T>(exec, &Func_), con_(::std::forward<F>(con)) {}

    /*!
     * Constructs an SDO download request.
     *
     * \param idx     the object index.
     * \param subidx  the object sub-index.
     * \param value   the value to be written.
     * \param exec    the executor used to execute the confirmation function.
     * \param con     the confirmation function to be called on completion of
     *                the SDO request.
     * \param timeout the SDO timeout. If, after the request is initiated, the
     *                timeout expires before receiving a response from the
     *                server, the client aborts the transfer with abort code
     *                #SdoErrc::TIMEOUT.
     *
     * \see Sdo::SubmitDownload<T>()
     */
    template <class U, class F>
    DownloadRequest(uint16_t idx, uint8_t subidx, U&& value,
                    aio::ExecutorBase& exec, F&& con, const duration& timeout)
        : DownloadRequestBase_<T>(exec, &Func_, idx, subidx,
                                  ::std::forward<U>(value), timeout),
          con_(::std::forward<F>(con)) {}

   private:
    void OnRequest(Impl_* impl) noexcept override;

    static void Func_(aio_task* task) noexcept;

    ::std::function<DownloadSignature> con_ { nullptr };
  };

  /*!
   * The signature of the callback function invoked on completion of an SDO
   * upload request. Note that the callback function SHOULD NOT throw
   * exceptions. Since it is invoked from C, any exception that is thrown cannot
   * be caught and will result in a call to `std::terminate()`.
   *
   * \param idx    the object index.
   * \param subidx the object sub-index.
   * \param ec     the SDO abort code (0 on success).
   * \param value  the value received from the SDO server.
   */
  template <class T>
  using UploadSignature = void(uint16_t idx, uint8_t subidx,
                               ::std::error_code ec, T value);

  //! An SDO upload request.
  template <class T>
  class LELY_COAPP_EXTERN UploadRequest : public UploadRequestBase_<T> {
   public:
    /*!
     * Constructs an empty SDO upload request. The object index and sub-index
     * and, optionally, the SDO timeout have to be set before the request can be
     * submitted.
     *
     * \param exec    the executor used to execute the confirmation function.
     * \param con     the confirmation function to be called on completion of
     *                the SDO request.
     *
     * \see Sdo::SubmitUpload<T>()
     */
    template <class F>
    UploadRequest(aio::ExecutorBase& exec, F&& con)
        : UploadRequestBase_<T>(exec, &Func_), con_(::std::forward<F>(con)) {}

    /*!
     * Constructs an SDO upload request.
     *
     * \param idx     the object index.
     * \param subidx  the object sub-index.
     * \param exec    the executor used to execute the confirmation function.
     * \param con     the confirmation function to be called on completion of
     *                the SDO request.
     * \param timeout the SDO timeout. If, after the request is initiated, the
     *                timeout expires before receiving a response from the
     *                server, the client aborts the transfer with abort code
     *                #SdoErrc::TIMEOUT.
     *
     * \see Sdo::SubmitUpload<T>()
     */
    template <class F>
    UploadRequest(uint16_t idx, uint8_t subidx, aio::ExecutorBase& exec,
                  F&& con, const duration& timeout)
        : UploadRequestBase_<T>(exec, &Func_, idx, subidx, timeout),
          con_(::std::forward<F>(con)) {}

   private:
    void OnRequest(Impl_* impl) noexcept override;

    static void Func_(aio_task* task) noexcept;

    ::std::function<UploadSignature<T>> con_ { nullptr };
  };

  //! Default-constructs an invalid Client-SDO queue.
  Sdo();

  /*!
   * Constructs a Client-SDO queue for a Client-SDO from the predefined
   * connection set (the default SDO). In general, only a CANopen master is
   * allowed to use the default SDO.
   *
   * \param net a pointer to a CAN network interface (from <lely/can/net.hpp>).
   * \param id  the node-ID of the SDO server (in the range [1..127]).
   */
  Sdo(CANNet* net, uint8_t id);

  /*!
   * Constructs a Client-SDO queue for a pre-configured Client-SDO. The SDO
   * client parameter record MUST exist in the object dictionary (object 1280 to
   * 12FF).
   *
   * \param net a pointer to a CAN network interface (from <lely/can/net.hpp>).
   * \param dev a pointer to a CANopen device (from <lely/co/dev.hpp>).
   * \param num the SDO number (in the range [1..128]).
   */
  Sdo(CANNet* net, CODev* dev, uint8_t num);

  /*!
   * Constructs a Client-SDO queue from an existing Client-SDO service. It is
   * the responsibility of the caller to ensure that the SDO service remains
   * available during the lifetime of the queue.
   *
   * \param sdo a pointer to a CANopen Client-SDO service (from
                <lely/co/csdo.hpp>).
   */
  explicit Sdo(COCSDO* sdo);

  Sdo(const Sdo&) = delete;
  Sdo(Sdo&&) = default;

  Sdo& operator=(const Sdo&) = delete;
  Sdo& operator=(Sdo&&);

  /*!
   * Destructs the Client-SDO queue. Any ongoing or pending SDO requests are
   * terminated with abort code #SdoErrc::DATA_CTL.
   */
  ~Sdo();

  //! Checks whether `*this` is a valid Client-SDO queue.
  explicit operator bool() const noexcept { return !!impl_; }

  //! Queues an SDO download request.
  template <class T>
  typename ::std::enable_if<detail::IsCanopenType<T>::value>::type
  SubmitDownload(DownloadRequest<T>& req) {
    Submit(static_cast<RequestBase_&>(req));
  }

  /*!
   * Queues an SDO download request. This function writes a value to a
   * sub-object in a remote object dictionary.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param value   the value to be written.
   * \param exec    the executor used to execute the confirmation function.
   * \param con     the confirmation function to be called on completion of the
   *                SDO request.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   */
  template <class T, class F>
  typename ::std::enable_if<
      detail::IsCanopenType<typename ::std::decay<T>::type>::value
  >::type
  SubmitDownload(uint16_t idx, uint8_t subidx, T&& value,
                 aio::ExecutorBase& exec, F&& con, const duration& timeout) {
    using U = typename ::std::decay<T>::type;
    auto req = new DownloadRequestWrapper<U>(idx, subidx,
        ::std::forward<T>(value), exec, ::std::forward<F>(con), timeout);
    Submit(*req);
  }

  /*!
   * Aborts an SDO download request.
   *
   * \param req the request to be aborted.
   * \param ac  the SDO abort code in case of an ongoing request.
   */
  template <class T>
  typename ::std::enable_if<detail::IsCanopenType<T>::value,
      ::std::size_t
  >::type
  CancelDownload(DownloadRequest<T>& req, SdoErrc ac) {
    return Cancel(req, ac);
  }

  //! Queues an SDO upload request.
  template <class T>
  typename ::std::enable_if<detail::IsCanopenType<T>::value>::type
  SubmitUpload(UploadRequest<T>& req) {
    Submit(static_cast<RequestBase_&>(req));
  }

  /*!
   * Queues an SDO upload request. This function reads the value of a sub-object
   * in a remote object dictionary.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param exec    the executor used to execute the confirmation function.
   * \param con     the confirmation function to be called on completion of the
   *                SDO request.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   */
  template <class T, class F>
  typename ::std::enable_if<detail::IsCanopenType<T>::value>::type
  SubmitUpload(uint16_t idx, uint8_t subidx, aio::ExecutorBase& exec, F&& con,
               const duration& timeout) {
    auto req = new UploadRequestWrapper<T>(idx, subidx, exec,
                                           ::std::forward<F>(con), timeout);
    Submit(*req);
  }

  /*!
   * Aborts an SDO upload request.
   *
   * \param req the request to be aborted.
   * \param ac  the SDO abort code in case of an ongoing request.
   */
  template <class T>
  typename ::std::enable_if<detail::IsCanopenType<T>::value,
      ::std::size_t
  >::type
  CancelUpload(UploadRequest<T>& req, SdoErrc ac) { return Cancel(req, ac); }

  /*!
   * Aborts the ongoing and all pending SDO requests.
   *
   * \param ac the SDO abort code.
   */
  ::std::size_t Cancel(SdoErrc ac);

  /*!
   * Queues an asynchronous SDO download request and returns a future.
   *
   * \param loop    the event loop used to create the future.
   * \param exec    the executor used to create the future. The executor SHOULD
   *                be based on \a loop.
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param value   the value to be written.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * \returns a future which, on completion, holds the SDO abort code.
   */
  template <class T>
  typename ::std::enable_if<detail::IsCanopenType<T>::value,
      aio::Future<::std::error_code>
  >::type
  AsyncDownload(aio::LoopBase& loop, aio::ExecutorBase& exec, int16_t idx,
                uint8_t subidx, T&& value, const duration& timeout) {
    auto req = new AsyncDownloadRequest<T>(loop, exec, idx, subidx,
                                           ::std::forward<T>(value), timeout);
    Submit(*req);
    return req->GetFuture();
  }

  /*!
   * Queues an asynchronous SDO upload request and returns a future.
   *
   * \param loop    the event loop used to create the future.
   * \param exec    the executor used to create the future. The executor SHOULD
   *                be based on \a loop.
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * \returns a future which, on completion, holds the SDO abort code and the
   * received value.
   */
  template <class T>
  typename ::std::enable_if<detail::IsCanopenType<T>::value,
      aio::Future<::std::tuple<::std::error_code, T>>
  >::type
  AsyncUpload(aio::LoopBase& loop, aio::ExecutorBase& exec, int16_t idx,
              uint8_t subidx, const duration& timeout) {
    auto req = new AsyncUploadRequest<T>(loop, exec, idx, subidx, timeout);
    Submit(*req);
    return req->GetFuture();
  }

 private:
  template <class T>
  class LELY_COAPP_EXTERN DownloadRequestWrapper
      : public DownloadRequestBase_<T> {
   public:
    template <class U, class F>
    DownloadRequestWrapper(uint16_t idx, int8_t subidx, U&& value,
        aio::ExecutorBase& exec, F&& con, const duration& timeout)
        : DownloadRequestBase_<T>(exec, &Func_, idx, subidx,
                                  ::std::forward<U>(value), timeout),
          con_(::std::forward<F>(con)) {}

   private:
    ~DownloadRequestWrapper() = default;

    void OnRequest(Impl_* impl) noexcept override;

    static void Func_(aio_task* task) noexcept;

    ::std::function<DownloadSignature> con_ { nullptr };
  };

  template <class T>
  class LELY_COAPP_EXTERN UploadRequestWrapper : public UploadRequestBase_<T> {
   public:
    template <class F>
    UploadRequestWrapper(uint16_t idx, uint8_t subidx, aio::ExecutorBase& exec,
                         F&& con, const duration& timeout)
        : UploadRequestBase_<T>(exec, &Func_, idx, subidx, timeout),
          con_(::std::forward<F>(con)) {}

   private:
    ~UploadRequestWrapper() = default;

    void OnRequest(Impl_* impl) noexcept override;

    static void Func_(aio_task* task) noexcept;

    ::std::function<UploadSignature<T>> con_ { nullptr };
  };

  template <class T>
  class LELY_COAPP_EXTERN AsyncDownloadRequest
      : public DownloadRequestBase_<T> {
   public:
    template <class U>
    AsyncDownloadRequest(aio::LoopBase& loop, aio::ExecutorBase& exec,
        uint16_t idx, int8_t subidx, U&& value, const duration& timeout)
        : DownloadRequestBase_<T>(exec, &Func_, idx, subidx,
                                  ::std::forward<U>(value), timeout),
          promise_(loop, exec) {}

    aio::Future<::std::error_code>
    GetFuture() noexcept { return promise_.GetFuture(); }

   private:
    ~AsyncDownloadRequest() = default;

    void OnRequest(Impl_* impl) noexcept override;

    static void Func_(aio_task* task) noexcept;

    aio::Promise<::std::error_code> promise_;
  };

  template <class T>
  class LELY_COAPP_EXTERN AsyncUploadRequest : public UploadRequestBase_<T> {
   public:
    AsyncUploadRequest(aio::LoopBase& loop, aio::ExecutorBase& exec,
                       uint16_t idx, int8_t subidx, const duration& timeout)
        : UploadRequestBase_<T>(exec, &Func_, idx, subidx, timeout),
          promise_(loop, exec) {}

    aio::Future<::std::tuple<::std::error_code, T>>
    GetFuture() noexcept { return promise_.GetFuture(); }

   private:
    ~AsyncUploadRequest() = default;

    void OnRequest(Impl_* impl) noexcept override;

    static void Func_(aio_task* task) noexcept;

    aio::Promise<::std::tuple<::std::error_code, T>> promise_;
  };

  void Submit(RequestBase_& req);

  ::std::size_t Cancel(RequestBase_& req, SdoErrc ac);

  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_SDO_HPP_
