/*!\file
 * This header file is part of the C++ asynchronous I/O library; it contains ...
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

#ifndef LELY_AIO_CAN_BUS_HPP_
#define LELY_AIO_CAN_BUS_HPP_

#include <lely/aio/detail/timespec.hpp>
#include <lely/aio/exec.hpp>

#include <lely/aio/can_bus.h>

#include <string>

namespace lely {

namespace aio {

class LELY_AIO_EXTERN CanBusBase : public detail::CBase<aio_can_bus_t> {
 public:
  using CBase::CBase;

  //! The type of a CAN or CAN FD format frame.
  using Frame = can_msg;

  using Info = can_msg_info;

  //! The states of a CAN node, dependening on the TX/RX error count.
  enum class State {
    //! The error active state (TX/RX error count < 128).
    ACTIVE,
    //! The error passive state (TX/RX error count < 256).
    PASSIVE,
    //! The bus off state (TX/RX error count >= 256).
    BUSOFF
  };

  //! The error flags of a CAN bus, which are not mutually exclusive.
  enum class Error {
    //! A single bit error.
    BIT =   1 << 0,
    //! A bit stuffing error.
    STUFF = 1 << 1,
    //! A CRC sequence error.
    CRC =   1 << 2,
    //! A form error.
    FORM =  1 << 3,
    //! An acknowledgment error.
    ACK =   1 << 4,
    //! One or more other errors.
    OTHER = 1 << 5
  };

  using ReadSignature = void(::std::error_code ec, int result);

  class LELY_AIO_EXTERN ReadOperation : public aio_can_bus_read_op {
   public:
    template <class F>
    ReadOperation(Frame* msg, Info* info, F&& f)
        : aio_can_bus_read_op { msg, info, AIO_TASK_INIT(nullptr, &Func_), 0 },
          func_(::std::forward<F>(f)) {}

    ReadOperation(const ReadOperation&) = delete;

    ReadOperation& operator=(const ReadOperation&) = delete;

    ExecutorBase
    GetExecutor() const noexcept { return ExecutorBase(task.exec); }

   private:
    static void Func_(aio_task* task) noexcept;

    ::std::function<ReadSignature> func_ { nullptr };
  };

  using WriteSignature = void(::std::error_code ec, int result);

  class LELY_AIO_EXTERN WriteOperation : public aio_can_bus_write_op {
   public:
    template <class F>
    WriteOperation(const Frame& msg, F&& f)
        : aio_can_bus_write_op { &msg, AIO_TASK_INIT(nullptr, &Func_), 0 },
          func_(::std::forward<F>(f)) {}

    WriteOperation(const WriteOperation&) = delete;

    WriteOperation& operator=(const WriteOperation&) = delete;

    ExecutorBase
    GetExecutor() const noexcept { return ExecutorBase(task.exec); }

   private:
    static void Func_(aio_task* task) noexcept;

    ::std::function<WriteSignature> func_ { nullptr };
  };

  ExecutorBase GetExecutor() const noexcept;

  int Read(Frame* msg = nullptr, Info* info = nullptr);

  int Read(Frame* msg, Info* info, ::std::error_code& ec);

  void SubmitRead(aio_can_bus_read_op& op);

  void
  SubmitRead(ReadOperation& op) {
    SubmitRead(static_cast<aio_can_bus_read_op&>(op));
  }

  template <class F>
  void
  SubmitRead(Frame* msg, Info* info, F&& f) {
    auto op = new ReadOperationWrapper(msg, info, ::std::forward<F>(f));
    SubmitRead(*op);
  }

  ::std::size_t CancelRead();

  ::std::size_t CancelRead(aio_can_bus_read_op& op);

  int Write(const Frame& msg);

  int Write(const Frame& msg, ::std::error_code& ec);

  void SubmitWrite(aio_can_bus_write_op& op);

  void
  SubmitWrite(WriteOperation& op) {
    SubmitWrite(static_cast<aio_can_bus_write_op&>(op));
  }

  template <class F>
  void
  SubmitWrite(const Frame& msg, F&& f) {
    auto op = new WriteOperationWrapper(msg, ::std::forward<F>(f));
    SubmitWrite(*op);
  }

  ::std::size_t CancelWrite();

  ::std::size_t CancelWrite(aio_can_bus_write_op& op);

  ::std::size_t Cancel();

  Future<aio_can_bus_read_op> AsyncRead(LoopBase& loop, Frame* msg = nullptr,
      Info* info = nullptr, aio_can_bus_read_op** pop = nullptr);

  int RunRead(LoopBase& loop, Frame* msg = nullptr, Info* info = nullptr);

  int RunRead(LoopBase& loop, Frame* msg, Info* info, ::std::error_code& ec);

  int
  RunReadFor(LoopBase& loop, Frame* msg = nullptr, Info* info = nullptr) {
    return RunReadUntil(loop, msg, info, nullptr);
  }

  int
  RunReadFor(LoopBase& loop, Frame* msg, Info* info, ::std::error_code& ec) {
    return RunReadUntil(loop, msg, info, nullptr, ec);
  }

  template <class Rep, class Period>
  int
  RunReadFor(LoopBase& loop, Frame* msg, Info* info,
      const ::std::chrono::duration<Rep, Period>& rel_time) {
    auto ts = AbsTime(rel_time);
    return RunReadUntil(loop, msg, info, &ts);
  }

  template <class Rep, class Period>
  int
  RunReadFor(LoopBase& loop, Frame* msg, Info* info,
      const ::std::chrono::duration<Rep, Period>& rel_time,
      ::std::error_code& ec) {
    auto ts = AbsTime(rel_time);
    return RunReadUntil(loop, msg, info, &ts, ec);
  }

  template <class Clock, class Duration>
  int
  RunReadUntil(LoopBase& loop, Frame* msg, Info* info,
      const ::std::chrono::time_point<Clock, Duration>& abs_time) {
    auto ts = AbsTime(abs_time);
    return RunReadUntil(loop, msg, info, &ts);
  }

  template <class Clock, class Duration>
  int
  RunReadUntil(LoopBase& loop, Frame* msg, Info* info,
      const ::std::chrono::time_point<Clock, Duration>& abs_time,
      ::std::error_code& ec) {
    auto ts = AbsTime(abs_time);
    return RunReadUntil(loop, msg, info, &ts, ec);
  }

  Future<aio_can_bus_write_op> AsyncWrite(LoopBase& loop,
      const Frame* msg = nullptr, aio_can_bus_write_op** pop = nullptr);

  int RunWrite(LoopBase& loop, const Frame& msg);

  int RunWrite(LoopBase& loop, const Frame& msg, ::std::error_code& ec);

  int
  RunWriteFor(LoopBase& loop, const Frame& msg) {
    return RunWriteUntil(loop, msg, nullptr);
  }

  int
  RunWriteFor(LoopBase& loop, const Frame& msg, ::std::error_code& ec) {
    return RunWriteUntil(loop, msg, nullptr, ec);
  }

  template <class Rep, class Period>
  int
  RunWriteFor(LoopBase& loop, const Frame& msg,
      const ::std::chrono::duration<Rep, Period>& rel_time) {
    auto ts = AbsTime(rel_time);
    return RunWriteUntil(loop, msg, &ts);
  }

  template <class Rep, class Period>
  int
  RunWriteFor(LoopBase& loop, const Frame& msg,
      const ::std::chrono::duration<Rep, Period>& rel_time,
      ::std::error_code& ec) {
    auto ts = AbsTime(rel_time);
    return RunWriteUntil(loop, msg, &ts, ec);
  }

  template <class Clock, class Duration>
  int
  RunWriteUntil(LoopBase& loop, const Frame& msg,
      const ::std::chrono::time_point<Clock, Duration>& abs_time) {
    auto ts = AbsTime(abs_time);
    return RunWriteUntil(loop, msg, &ts);
  }

  template <class Clock, class Duration>
  int
  RunWriteUntil(LoopBase& loop, const Frame& msg,
      const ::std::chrono::time_point<Clock, Duration>& abs_time,
      ::std::error_code& ec) {
    auto ts = AbsTime(abs_time);
    return RunWriteUntil(loop, msg, &ts, ec);
  }

 protected:
  class LELY_AIO_EXTERN ReadOperationWrapper : public aio_can_bus_read_op {
   public:
    template <class F>
    ReadOperationWrapper(Frame* msg, Info* info, F&& f)
        : aio_can_bus_read_op { msg, info, AIO_TASK_INIT(nullptr, &Func_), 0 },
          func_(::std::forward<F>(f)) {}

    ReadOperationWrapper(const ReadOperationWrapper&) = delete;

    ReadOperationWrapper& operator=(const ReadOperationWrapper&) = delete;

   private:
    ~ReadOperationWrapper() = default;

    static void Func_(aio_task* task) noexcept;

    ::std::function<ReadSignature> func_ { nullptr };
  };

  class LELY_AIO_EXTERN WriteOperationWrapper : public aio_can_bus_write_op {
   public:
    template <class F>
    WriteOperationWrapper(const Frame& msg, F&& f)
        : aio_can_bus_write_op { &msg, AIO_TASK_INIT(nullptr, &Func_), 0 },
          func_(::std::forward<F>(f)) {}

    WriteOperationWrapper(const WriteOperationWrapper&) = delete;

    WriteOperationWrapper& operator=(const WriteOperationWrapper&) = delete;

   private:
    ~WriteOperationWrapper() = default;

    static void Func_(aio_task* task) noexcept;

    ::std::function<WriteSignature> func_ { nullptr };
  };

  int RunReadUntil(LoopBase& loop, Frame* msg, Info* info, const timespec* tp);

  int RunReadUntil(LoopBase& loop, Frame* msg, Info* info, const timespec* tp,
                   ::std::error_code& ec);

  int RunWriteUntil(LoopBase& loop, const Frame& msg, const timespec* tp);

  int RunWriteUntil(LoopBase& loop, const Frame& msg, const timespec* tp,
                    ::std::error_code& ec);
};

constexpr CanBusBase::Error
operator&(CanBusBase::Error lhs, CanBusBase::Error rhs) {
  return static_cast<CanBusBase::Error>(
      static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr CanBusBase::Error
operator|(CanBusBase::Error lhs, CanBusBase::Error rhs) {
  return static_cast<CanBusBase::Error>(
      static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr CanBusBase::Error
operator^(CanBusBase::Error lhs, CanBusBase::Error rhs) {
  return static_cast<CanBusBase::Error>(
      static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr CanBusBase::Error
operator~(CanBusBase::Error lhs) {
  return static_cast<CanBusBase::Error>(~static_cast<int>(lhs));
}

inline CanBusBase::Error&
operator&=(CanBusBase::Error& lhs, CanBusBase::Error rhs) noexcept {
  return lhs = lhs & rhs;
}

inline CanBusBase::Error&
operator|=(CanBusBase::Error& lhs, CanBusBase::Error rhs) noexcept {
  return lhs = lhs | rhs;
}

inline CanBusBase::Error&
operator^=(CanBusBase::Error& lhs, CanBusBase::Error rhs) noexcept {
  return lhs = lhs ^ rhs;
}

class LELY_AIO_EXTERN CanBus : public CanBusBase {
  class BooleanOption {
    friend class CanBus;

   public:
    explicit BooleanOption(bool value = false) noexcept : value_(value) {}

    BooleanOption&
    operator=(bool value) noexcept { value_ = value; return *this; }

    explicit operator bool() const noexcept { return value_; }
    bool operator!() const noexcept { return !value_; }

   private:
    int value_ { 0 };
  };

 public:
  class FdFrames : public BooleanOption {
   public:
    using BooleanOption::BooleanOption;
    using BooleanOption::operator=;
  };

  class ErrorFrames : public BooleanOption {
   public:
    using BooleanOption::BooleanOption;
    using BooleanOption::operator=;
  };

  CanBus(ExecutorBase& exec, ReactorBase& reactor);

  CanBus(const CanBus&) = delete;

  CanBus(CanBus&& other) : CanBusBase(other.c_ptr) { other.c_ptr = nullptr; }

  CanBus& operator=(const CanBus&) = delete;

  CanBus&
  operator=(CanBus&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~CanBus();

  aio_handle_t GetHandle() const noexcept;

  void Open(const ::std::string& ifname);
  void Open(const ::std::string& ifname, ::std::error_code& ec);

  void Assign(aio_handle_t handle);
  void Assign(aio_handle_t handle, ::std::error_code& ec);

  aio_handle_t Release();
  aio_handle_t Release(::std::error_code& ec);

  bool IsOpen() const noexcept;

  void Close();
  void Close(::std::error_code& ec);

  template <class GettableCanBusOption>
  void GetOption(GettableCanBusOption& option) const;

  template <class GettableCanBusOption>
  void GetOption(GettableCanBusOption& option, ::std::error_code& ec) const;

  template <class SettableCanBusOption>
  void SetOption(const SettableCanBusOption& option);

  template <class SettableCanBusOption>
  void SetOption(const SettableCanBusOption& option, ::std::error_code& ec);
};

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_CAN_BUS_HPP_
