/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen slave declarations.
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_COCPP_SLAVE_HPP_
#define LELY_COCPP_SLAVE_HPP_

#include <lely/coapp/node.hpp>

#include <memory>
#include <string>
#include <utility>

#include <cstddef>

namespace lely {

namespace canopen {

/// The base class for CANopen slaves.
class BasicSlave : public Node {
 public:
  /**
   * Creates a new CANopen slave. After creation, the slave is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * @param exec  the executor used to process I/O and CANopen events. If
   *              <b>exec</b> is a null pointer, the CAN channel executor is
   *              used.
   * @param timer the timer used for CANopen events. This timer MUST NOT be used
   *              for any other purpose.
   * @param chan  a CAN channel. This channel MUST NOT be used for any other
   *              purpose.
   * @param dev   a pointer to an internal device desciption. Ownership of
   *              <b>dev</b> is transfered to the new class instance and the
   *              internal device description will be destroyed when the class
   *              instance is destroyed.
   * @param id    the node-ID (in the range [1..127, 255]). If <b>id</b> is 255
   *              (unconfigured), the node-ID is obtained from the device
   *              description.
   */
  explicit BasicSlave(ev_exec_t* exec, io::TimerBase& timer,
                      io::CanChannelBase& chan, __co_dev* dev,
                      uint8_t id = 0xff);

  /// Creates a new CANopen slave.
  explicit BasicSlave(io::TimerBase& timer, io::CanChannelBase& chan,
                      __co_dev* dev, uint8_t id = 0xff)
      : BasicSlave(nullptr, timer, chan, dev, id) {}

  /**
   * Creates a new CANopen slave. After creation, the slave is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * @param exec    the executor used to process I/O and CANopen events. If
   *                <b>exec</b> is a null pointer, the CAN channel executor is
   *                used.
   * @param timer   the timer used for CANopen events. This timer MUST NOT be
   *                used for any other purpose.
   * @param chan    a CAN channel. This channel MUST NOT be used for any other
   *                purpose.
   * @param dcf_txt the path of the text EDS or DCF containing the device
   *                description.
   * @param dcf_bin the path of the (binary) concise DCF containing the values
   *                of (some of) the objets in the object dictionary. If
   *                <b>dcf_bin</b> is empty, no concise DCF is loaded.
   * @param id      the node-ID (in the range [1..127, 255]). If <b>id</b> is
   *                255 (unconfigured), the node-ID is obtained from the DCF.
   */
  explicit BasicSlave(ev_exec_t* exec, io::TimerBase& timer,
                      io::CanChannelBase& chan, const ::std::string& dcf_txt,
                      const ::std::string& dcf_bin = "", uint8_t id = 0xff);

  /// Creates a new CANopen slave.
  explicit BasicSlave(io::TimerBase& timer, io::CanChannelBase& chan,
                      const ::std::string& dcf_txt,
                      const ::std::string& dcf_bin = "", uint8_t id = 0xff)
      : BasicSlave(nullptr, timer, chan, dcf_txt, dcf_bin, id) {}

  /**
   * Creates a new CANopen slave. After creation, the slave is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * @param exec  the executor used to process I/O and CANopen events. If
   *              <b>exec</b> is a null pointer, the CAN channel executor is
   *              used.
   * @param timer the timer used for CANopen events. This timer MUST NOT be used
   *              for any other purpose.
   * @param chan  a CAN channel. This channel MUST NOT be used for any other
   *              purpose.
   * @param sdev  a pointer to a static device desciption.
   * @param id    the node-ID (in the range [1..127, 255]). If <b>id</b> is 255
   *              (unconfigured), the node-ID is obtained from the device
   *              description.
   */
  explicit BasicSlave(ev_exec_t* exec, io::TimerBase& timer,
                      io::CanChannelBase& chan, const co_sdev* sdev,
                      uint8_t id = 0xff);

  /// Creates a new CANopen slave.
  explicit BasicSlave(io::TimerBase& timer, io::CanChannelBase& chan,
                      const co_sdev* sdev, uint8_t id = 0xff)
      : BasicSlave(nullptr, timer, chan, sdev, id) {}

  virtual ~BasicSlave();

  /**
   * Registers the function to be invoked when a life guarding event occurs or
   * is resolved. Only a single function can be registered at any one time. If
   * <b>on_life_guarding</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnLifeGuarding(bool) completes.
   */
  void OnLifeGuarding(::std::function<void(bool)> on_life_guarding);

 protected:
  class Object;
  class ConstObject;

  /**
   * A mutator providing read/write access to a CANopen sub-object in a local
   * object dictionary.
   */
  class SubObject {
    friend class Object;

   public:
    SubObject(const SubObject&) = default;
    SubObject(SubObject&&) = default;

    SubObject& operator=(const SubObject&) = default;
    SubObject& operator=(SubObject&&) = default;

    /**
     * Sets the value of the sub-object.
     *
     * @param value the value to be written.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * @see Set()
     */
    template <class T>
    SubObject&
    operator=(T&& value) {
      return Set(::std::forward<T>(value));
    }

    /**
     * Returns a copy of the value of the sub-object.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * @see Get()
     */
    template <class T>
    operator T() const {
      return Get<T>();
    }

    /**
     * Returns the type of the sub-object.
     *
     * @returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist.
     *
     * @see Device::Type(uint16_t idx, uint8_t subidx) const
     */
    const ::std::type_info&
    Type() const {
      return slave_->Type(idx_, subidx_);
    }

    /**
     * Returns the type of the sub-object.
     *
     * @param ec if the sub-object does not exist, the SDO abort code is stored
     *           in <b>ec</b>.
     *
     * @returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * @see Device::Type(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    const ::std::type_info&
    Type(::std::error_code& ec) const noexcept {
      return slave_->Type(idx_, subidx_, ec);
    }

    /**
     * Reads the value of the sub-object.
     *
     * @returns a copy of the value of the sub-object.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * @see Device::Get(uint16_t idx, uint8_t subidx) const
     * @see Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const
     */
    template <class T>
    T
    Get() const {
      return id_ ? slave_->TpdoGet<T>(id_, idx_, subidx_)
                 : slave_->Get<T>(idx_, subidx_);
    }

    /**
     * Reads the value of the sub-object.
     *
     * @param ec if the sub-object does not exist or the type does not match,
     *           the SDO abort code is stored in <b>ec</b>.
     *
     * @returns a copy of the value of the sub-object, or an empty value on
     * error.
     *
     * @see Device::Get(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     * @see Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    template <class T>
    T
    Get(::std::error_code& ec) const noexcept {
      return id_ ? slave_->TpdoGet<T>(id_, idx_, subidx_, ec)
                 : slave_->Get<T>(idx_, subidx_, ec);
    }

    /**
     * Writes a value to the sub-object.
     *
     * @param value the value to be written.
     *
     * @returns `*this`.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * @see Device::Set(uint16_t idx, uint8_t subidx, T&& value)
     * @see Device::TpdoSet(uint8_t id, uint16_t idx, uint8_t subidx, T&& value)
     */
    template <class T>
    SubObject&
    Set(T&& value) {
      if (id_)
        slave_->TpdoSet(id_, idx_, subidx_, ::std::forward<T>(value));
      else
        slave_->Set(idx_, subidx_, ::std::forward<T>(value));
      return *this;
    }

    /**
     * Writes a value to the sub-object.
     *
     * @param value the value to be written.
     * @param ec    if the sub-object does not exist or the type does not match,
     *              the SDO abort code is stored in <b>ec</b>.
     *
     * @returns `*this`.
     *
     * @see Device::Set(uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec)
     * @see Device::Set(uint16_t idx, uint8_t subidx, const T& value, ::std::error_code& ec)
     * @see Device::Set(uint16_t idx, uint8_t subidx, const char* value, ::std::error_code& ec)
     * @see Device::Set(uint16_t idx, uint8_t subidx, const char16_t* value, ::std::error_code& ec)
     * @see Device::TpdoSet(uint8_t id, uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec)
     */
    template <class T>
    SubObject&
    Set(T&& value, ::std::error_code& ec) noexcept {
      if (id_)
        slave_->TpdoSet(id_, idx_, subidx_, ::std::forward<T>(value), ec);
      else
        slave_->Set(idx_, subidx_, ::std::forward<T>(value), ec);
      return *this;
    }

    /**
     * Writes an OCTET_STRING or DOMAIN value to the sub-object.
     *
     * @param p a pointer to the bytes to be written.
     * @param n the number of bytes to write.
     *
     * @returns `*this`.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * @see Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n)
     */
    SubObject&
    Set(const void* p, ::std::size_t n) {
      if (!id_) slave_->Set(idx_, subidx_, p, n);
      return *this;
    }

    /**
     * Writes an OCTET_STRING or DOMAIN value to the sub-object.
     *
     * @param p  a pointer to the bytes to be written.
     * @param n  the number of bytes to write.
     * @param ec if the sub-object does not exist or the type does not match,
     *           the SDO abort code is stored in <b>ac</b>.
     *
     * @returns `*this`.
     *
     * @see Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n, ::std::error_code& ec)
     */
    SubObject&
    Set(const void* p, ::std::size_t n, ::std::error_code& ec) noexcept {
      if (!id_) slave_->Set(idx_, subidx_, p, n, ec);
      return *this;
    }

    /**
     * Checks if the sub-object can be mapped into a PDO and, if so, triggers
     * the transmission of every acyclic or event-driven Transmit-PDO into which
     * the sub-object is mapped.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Device::SetEvent(uint16_t idx, uint8_t subidx)
     * @see Device::TpdoSetEvent(uint8_t id, uint16_t idx, uint8_t subidx)
     */
    void
    SetEvent() {
      if (id_)
        slave_->TpdoSetEvent(id_, idx_, subidx_);
      else
        slave_->SetEvent(idx_, subidx_);
    }

    /**
     * Checks if the sub-object can be mapped into a PDO and, if so, triggers
     * the transmission of every acyclic or event-driven Transmit-PDO into which
     * the sub-object is mapped.
     *
     * @param ec on error, the SDO abort code is stored in <b>ec</b>.
     *
     * @see Device::SetEvent(uint16_t idx, uint8_t subidx, ::std::error_code& ec)
     * @see Device::TpdoSetEvent(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec)
     */
    void
    SetEvent(::std::error_code& ec) noexcept {
      if (id_)
        slave_->TpdoSetEvent(id_, idx_, subidx_, ec);
      else
        slave_->SetEvent(idx_, subidx_, ec);
    }

   private:
    SubObject(BasicSlave* slave, uint8_t id, uint16_t idx,
              uint8_t subidx) noexcept
        : slave_(slave), idx_(idx), subidx_(subidx), id_(id) {}

    BasicSlave* slave_;
    uint16_t idx_;
    uint8_t subidx_;
    uint8_t id_;
  };

  /**
   * An accessor providing read-only access to a CANopen sub-object in a local
   * object dictionary.
   */
  class ConstSubObject {
    friend class Object;
    friend class ConstObject;

   public:
    /**
     * Returns a copy of the value of the sub-object.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * @see Get()
     */
    template <class T>
    operator T() const {
      return Get<T>();
    }

    /**
     * Returns the type of the sub-object.
     *
     * @returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist.
     *
     * @see Device::Type(uint16_t idx, uint8_t subidx) const
     */
    const ::std::type_info&
    Type() const {
      return slave_->Type(idx_, subidx_);
    }

    /**
     * Returns the type of the sub-object.
     *
     * @param ec if the sub-object does not exist, the SDO abort code is stored
     *           in <b>ec</b>.
     *
     * @returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * @see Device::Type(uint16_t idx, uint8_t subidx, ::std::error_code& ec)
     * const
     */
    const ::std::type_info&
    Type(::std::error_code& ec) const {
      return slave_->Type(idx_, subidx_, ec);
    }

    /**
     * Reads the value of the sub-object.
     *
     * @returns a copy of the value of the sub-object.
     *
     * @throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * @see Device::Get(uint16_t idx, uint8_t subidx) const
     * @see Device::RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const
     * @see Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const
     */
    template <class T>
    T
    Get() const {
      return id_ ? (is_rpdo_ ? slave_->RpdoGet<T>(id_, idx_, subidx_)
                             : slave_->TpdoGet<T>(id_, idx_, subidx_))
                 : slave_->Get<T>(idx_, subidx_);
    }

    /**
     * Reads the value of the sub-object.
     *
     * @param ec if the sub-object does not exist or the type does not match,
     *           the SDO abort code is stored in <b>ec</b>.
     *
     * @returns a copy of the value of the sub-object, or an empty value on
     * error.
     *
     * @see Device::Get(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     * @see Device::RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     * @see Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    template <class T>
    T
    Get(::std::error_code& ec) const noexcept {
      return id_ ? (is_rpdo_ ? slave_->RpdoGet<T>(id_, idx_, subidx_, ec)
                             : slave_->TpdoGet<T>(id_, idx_, subidx_, ec))
                 : slave_->Get<T>(idx_, subidx_, ec);
    }

   private:
    ConstSubObject(const BasicSlave* slave, uint8_t id, uint16_t idx,
                   uint8_t subidx, bool is_rpdo) noexcept
        : slave_(slave),
          idx_(idx),
          subidx_(subidx),
          id_(id),
          is_rpdo_(is_rpdo) {}

    const BasicSlave* slave_;
    uint16_t idx_;
    uint8_t subidx_;
    uint8_t id_ : 7;
    uint8_t is_rpdo_ : 1;
  };

  class RpdoMapped;
  class TpdoMapped;

  /**
   * A mutator providing read/write access to a CANopen object in a local object
   * dictionary.
   */
  class Object {
    class Mapped;
    friend class BasicSlave;

   public:
    /**
     * Returns a mutator object that provides read/write access to the specified
     * CANopen sub-object in the local object dictionary (or the TPDO-mapped
     * sub-object in the remote object dictionary). Note that this function
     * succeeds even if the sub-object does not exist.
     *
     * @param subidx the object sub-index.
     *
     * @returns a mutator object for a CANopen sub-object in the local object
     * dictionary.
     */
    SubObject
    operator[](uint8_t subidx) noexcept {
      return SubObject(slave_, id_, idx_, subidx);
    }

    /**
     * Returns an accessor object that provides read-only access to the
     * specified CANopen sub-object in the local object dictionary (or the
     * TPDO-mapped sub-object in the remote object dictionary). Note that this
     * function succeeds even if the object does not exist.
     *
     * @param subidx the object sub-index.
     *
     * @returns an accessor object for a CANopen sub-object in the local object
     * dictionary.
     */
    ConstSubObject
    operator[](uint8_t subidx) const noexcept {
      return ConstSubObject(slave_, id_, idx_, subidx, false);
    }

   private:
    Object(BasicSlave* slave, uint16_t idx) noexcept : Object(slave, 0, idx) {}

    Object(BasicSlave* slave, uint8_t id, uint16_t idx) noexcept
        : slave_(slave), idx_(idx), id_(id) {}

    BasicSlave* slave_;
    uint16_t idx_;
    uint8_t id_;
  };

  /**
   * An accessor providing read-only access to a CANopen object in a local
   * object dictionary.
   */
  class ConstObject {
    class RpdoMapped;
    class TpdoMapped;
    friend class BasicSlave;

   public:
    /**
     * Returns an accessor object that provides read-only access to the
     * specified CANopen sub-object in the local object dictionary (or the
     * PDO-mapped sub-object in the remote object dictionary). Note that this
     * function succeeds even if the object does not exist.
     *
     * @param subidx the object sub-index.
     *
     * @returns an accessor object for a CANopen sub-object in the local object
     * dictionary.
     */
    ConstSubObject
    operator[](uint8_t subidx) const noexcept {
      return ConstSubObject(slave_, id_, idx_, subidx, is_rpdo_);
    }

   private:
    ConstObject(const BasicSlave* slave, uint16_t idx) noexcept
        : ConstObject(slave, 0, idx, false) {}

    ConstObject(const BasicSlave* slave, uint8_t id, uint16_t idx,
                bool is_rpdo) noexcept
        : slave_(slave), idx_(idx), id_(id), is_rpdo_(is_rpdo) {}

    const BasicSlave* slave_;
    uint16_t idx_;
    uint8_t id_ : 7;
    uint8_t is_rpdo_ : 1;
  };

  /**
   * An accessor providing read-only access to RPDO-mapped objects in a remote
   * object dictionary.
   */
  class RpdoMapped {
    friend class BasicSlave;

   public:
    /**
     * Returns an accessor object that provides read-only access to the
     * specified RPDO-mapped object in the remote object dictionary. Note that
     * this function succeeds even if the object does not exist.
     *
     * @param idx the object index.
     *
     * @returns an accessor object for a CANopen object in the remote object
     * dictionary.
     */
    ConstObject
    operator[](uint16_t idx) const noexcept {
      return ConstObject(slave_, id_, idx, true);
    }

   private:
    RpdoMapped(const BasicSlave* slave, uint8_t id) noexcept
        : slave_(slave), id_(id) {}

    const BasicSlave* slave_;
    uint8_t id_;
  };

  /**
   * A mutator providing read/write access to TPDO-mapped objects in a remote
   * object dictionary.
   */
  class TpdoMapped {
    friend class BasicSlave;

   public:
    /**
     * Returns a mutator object that provides read/write access to the specified
     * TPDO-mapped object in the remote object dictionary. Note that this
     * function succeeds even if the object does not exist.
     *
     * @param idx the object index.
     *
     * @returns a mutator object for a CANopen object in the remote object
     * dictionary.
     */
    Object
    operator[](uint16_t idx) noexcept {
      return Object(slave_, id_, idx);
    }

    /**
     * Returns an accessor object that provides read-only access to the
     * specified TPDO-mapped object in the remote object dictionary. Note that
     * this function succeeds even if the object does not exist.
     *
     * @param idx the object index.
     *
     * @returns an accessor object for a CANopen object in the remote object
     * dictionary.
     */
    ConstObject
    operator[](uint16_t idx) const noexcept {
      return ConstObject(slave_, id_, idx, false);
    }

   private:
    TpdoMapped(BasicSlave* slave, uint8_t id) noexcept
        : slave_(slave), id_(id) {}

    BasicSlave* slave_;
    uint8_t id_;
  };

  /**
   * The signature of the callback function invoked on read (SDO upload) access
   * to the local object dictionary. Note that the callback function SHOULD NOT
   * throw exceptions. Since it is invoked from C, any exception that is thrown
   * cannot be caught and will result in a call to `std::terminate()`. The
   * #lely::util::BasicLockable mutex implemented by this class is held for the
   * duration of the call.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the current value in the object dictionary. This value can be
   *               modified before it is returned to the client.
   *
   * @returns 0 on success, or an SDO abort code on error.
   */
  template <class T>
  using OnReadSignature = ::std::error_code(uint16_t idx, uint8_t subdx,
                                            T& value);

  /**
   * The signature of the callback function invoked on write (SDO download)
   * access to the local object dictionary. Note that the callback function
   * SHOULD NOT throw exceptions. Since it is invoked from C, any exception that
   * is thrown cannot be caught and will result in a call to `std::terminate()`.
   * The #lely::util::BasicLockable mutex implemented by this class is held for
   * the duration of the call.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param new_val the value to be written to the object dictionary. This value
   *                can be modified before it is committed.
   * @param old_val the current value in the object dictionary (only for
   *                CANopen basic data types).
   *
   * @returns 0 on success, or an SDO abort code on error.
   */
  template <class T>
  using OnWriteSignature = typename ::std::conditional<
      is_canopen_basic<T>::value,
      ::std::error_code(uint16_t idx, uint8_t subidx, T& new_val, T old_val),
      ::std::error_code(uint16_t idx, uint8_t subidx, T& new_val)>::type;

  /**
   * Returns a mutator object that provides read/write access to the specified
   * CANopen object in the local object dictionary. Note that this function
   * succeeds even if the object does not exist.
   *
   * @param idx the object index.
   *
   * @returns a mutator object for a CANopen object in the local object
   * dictionary.
   */
  Object
  operator[](::std::ptrdiff_t idx) noexcept {
    return Object(this, idx);
  }

  /**
   * Returns an accessor object that provides read-only access to the specified
   * CANopen object in the local object dictionary. Note that this function
   * succeeds even if the object does not exist.
   *
   * @param idx the object index.
   *
   * @returns an accessor object for a CANopen object in the local object
   * dictionary.
   */
  ConstObject
  operator[](::std::ptrdiff_t idx) const noexcept {
    return ConstObject(this, idx);
  }

  /**
   * Returns an accessor object that provides read-only access to RPDO-mapped
   * objects in the remote object dictionary of the specified node. Note that
   * this function succeeds even if no RPDO-mapped objects exist.
   *
   * @param id the node-ID.
   *
   * @returns an accessor object for RPDO-mapped objects in a remote object
   * dictionary.
   */
  RpdoMapped
  RpdoMapped(uint8_t id) const noexcept {
    return {this, id};
  }

  /**
   * Returns a mutator object that provides read/write access to TPDO-mapped
   * objects in the remote object dictionary of the specified node. Note that
   * this function succeeds even if no TPDO-mapped objects exist.
   *
   * @param id the node-ID.
   *
   * @returns a mutator object for TPDO-mapped objects in a remote object
   * dictionary.
   */
  TpdoMapped
  TpdoMapped(uint8_t id) noexcept {
    return {this, id};
  }

  /**
   * Registers a callback function to be invoked on read (SDO upload) access to
   * the specified CANopen sub-object in the local object dictionary. Note that
   * the callback function is not invoked if the access checks fail.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ind    the indication function to be called on write access to the
   *               specified sub-object.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnRead(
      uint16_t idx, uint8_t subidx, ::std::function<OnReadSignature<T>> ind);

  /**
   * Registers a callback function to be invoked on read (SDO upload) access to
   * the specified CANopen sub-object in the local object dictionary. Note that
   * the callback function is not invoked if the access checks fail.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ind    the indication function to be called on write access to the
   *               specified sub-object.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnRead(
      uint16_t idx, uint8_t subidx, ::std::function<OnReadSignature<T>> ind,
      ::std::error_code& ec);

  /**
   * Registers a callback function to be invoked on read (SDO upload) access to
   * each member of the specified CANopen record or array object in the local
   * object dictionary. Note that the callback function is not invoked if the
   * access checks fail.
   *
   * @param idx the object index.
   * @param ind the indication function to be called on read access to each
   *            member of the specified object.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnRead(
      uint16_t idx, ::std::function<OnReadSignature<T>> ind);

  /**
   * Registers a callback function to be invoked on read (SDO upload) access to
   * each member of the specified CANopen record or array object in the local
   * object dictionary. Note that the callback function is not invoked if the
   * access checks fail.
   *
   * @param idx the object index.
   * @param ind the indication function to be called on read access to each
   *            member of the specified object.
   * @param ec  on error, the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnRead(
      uint16_t idx, ::std::function<OnReadSignature<T>> ind,
      ::std::error_code& ec);

  /**
   * Registers a callback function to be invoked on write (SDO download) access
   * to the specified CANopen sub-object in the local object dictionary. Note
   * that the callback function is not invoked if the access or range checks
   * fail.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ind    the indication function to be called on read access to the
   *               specified sub-object.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnWrite(
      uint16_t idx, uint8_t subidx, ::std::function<OnWriteSignature<T>> ind);

  /**
   * Registers a callback function to be invoked on write (SDO download) access
   * to the specified CANopen sub-object in the local object dictionary. Note
   * that the callback function is not invoked if the access or range checks
   * fail.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ind    the indication function to be called on read access to the
   *               specified sub-object.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnWrite(
      uint16_t idx, uint8_t subidx, ::std::function<OnWriteSignature<T>> ind,
      ::std::error_code& ec);

  /**
   * Registers a callback function to be invoked on write (SDO download) access
   * to each member of the specified CANopen record or array object in the local
   * object dictionary. Note that the callback function is not invoked if the
   * access or range checks fail.
   *
   * @param idx the object index.
   * @param ind the indication function to be called on write access to each
   *            member of the specified object.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnWrite(
      uint16_t idx, ::std::function<OnWriteSignature<T>> ind);

  /**
   * Registers a callback function to be invoked on write (SDO download) access
   * to each member of the specified CANopen record or array object in the local
   * object dictionary. Note that the callback function is not invoked if the
   * access or range checks fail.
   *
   * @param idx the object index.
   * @param ind the indication function to be called on write access to each
   *            member of the specified object.
   * @param ec  on error, the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type OnWrite(
      uint16_t idx, ::std::function<OnWriteSignature<T>> ind,
      ::std::error_code& ec);

 protected:
  using Device::OnWrite;

 private:
  /**
   * The function invoked when a life guarding event occurs or is resolved. Note
   * that depending on the value of object 1029:01 (Error behavior object), the
   * occurrence of a life guarding event MAY trigger an NMT state transition. If
   * so, this function is called _after_ the state change completes.
   *
   * @param occurred `true` if the life guarding event occurred, `false` if it
   *                 was resolved.
   */
  virtual void
  OnLifeGuarding(bool occurred) noexcept {
    (void)occurred;
  };

  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COCPP_SLAVE_HPP_
