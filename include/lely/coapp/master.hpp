/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen master declarations.
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

#ifndef LELY_COCPP_MASTER_HPP_
#define LELY_COCPP_MASTER_HPP_

#include <lely/coapp/node.hpp>
#include <lely/coapp/sdo.hpp>

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <cstddef>

namespace lely {

namespace canopen {

class DriverBase;

/**
 * The CANopen master. The master implements a CANopen node. Handling events for
 * remote CANopen slaves is delegated to drivers (see #lely::canopen::DriverBase
 * and #lely::canopen::BasicDriver), one of which can be registered for each
 * node-ID.
 *
 * For derived classes, the master behaves as an AssociativeContainer for
 * drivers.
 */
class BasicMaster : public Node, protected ::std::map<uint8_t, DriverBase*> {
 public:
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
     * @see Write()
     */
    template <class T>
    SubObject&
    operator=(T&& value) {
      return Write(::std::forward<T>(value));
    }

    /**
     * Returns the value of the sub-object by submitting an SDO upload request
     * to the local object dictionary.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Read()
     */
    template <class T>
    operator T() const {
      return Read<T>();
    }

    /**
     * Reads the value of the sub-object by submitting an SDO upload request to
     * the local object dictionary.
     *
     * @returns the result of the SDO request.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Device::Read(uint16_t idx, uint8_t subidx) const
     * @see Device::TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const
     */
    template <class T>
    T
    Read() const {
      return id_ ? master_->TpdoRead<T>(id_, idx_, subidx_)
                 : master_->Read<T>(idx_, subidx_);
    }

    /**
     * Reads the value of the sub-object by submitting an SDO upload request to
     * the local object dictionary.
     *
     * @param ec on error, the SDO abort code is stored in <b>ec</b>.
     *
     * @returns the result of the SDO request, or an empty value on error.
     *
     * @see Device::Read(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     * @see Device::TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    template <class T>
    T
    Read(::std::error_code& ec) const {
      return id_ ? master_->TpdoRead<T>(id_, idx_, subidx_, ec)
                 : master_->Read<T>(idx_, subidx_, ec);
    }

    /**
     * Writes a value to the sub-object by submitting an SDO download request to
     * the local object dictionary.
     *
     * @param value the value to be written.
     *
     * @returns `*this`.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Device::Write(uint16_t idx, uint8_t subidx, T&& value)
     * @see Device::TpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx, T&& value)
     */
    template <class T>
    SubObject&
    Write(T&& value) {
      if (id_)
        master_->TpdoWrite(id_, idx_, subidx_, ::std::forward<T>(value));
      else
        master_->Write(idx_, subidx_, ::std::forward<T>(value));
      return *this;
    }

    /**
     * Writes a value to the sub-object by submitting an SDO download request to
     * the local object dictionary.
     *
     * @param value the value to be written.
     * @param ec    on error, the SDO abort code is stored in <b>ec</b>.
     *
     * @returns `*this`.
     *
     * @see Device::Write(uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec)
     * @see Device::Write(uint16_t idx, uint8_t subidx, const T& value, ::std::error_code& ec)
     * @see Device::Write(uint16_t idx, uint8_t subidx, const char* value, ::std::error_code& ec)
     * @see Device::Write(uint16_t idx, uint8_t subidx, const char16_t* value, ::std::error_code& ec)
     * @see Device::TpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx, T&& value, ::std::error_code& ec)
     */
    template <class T>
    SubObject&
    Write(T&& value, ::std::error_code& ec) {
      if (id_)
        master_->TpdoWrite(id_, idx_, subidx_, ::std::forward<T>(value), ec);
      else
        master_->Write(idx_, subidx_, ::std::forward<T>(value), ec);
      return *this;
    }

    /**
     * Writes an OCTET_STRING or DOMAIN value to the sub-object by submitting an
     * SDO download request to the local object dictionary.
     *
     * @param p a pointer to the bytes to be written.
     * @param n the number of bytes to write.
     *
     * @returns `*this`.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Device::Write(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n)
     */
    SubObject&
    Write(const void* p, ::std::size_t n) {
      if (!id_) master_->Write(idx_, subidx_, p, n);
      return *this;
    }

    /**
     * Writes an OCTET_STRING or DOMAIN value to the sub-object by submitting an
     * SDO download request to the local object dictionary.
     *
     * @param p  a pointer to the bytes to be written.
     * @param n  the number of bytes to write.
     * @param ec on error, the SDO abort code is stored in <b>ec</b>.
     *
     * @returns `*this`.
     *
     * @see Device::Write(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n, ::std::error_code& ec)
     */
    SubObject&
    Write(const void* p, ::std::size_t n, ::std::error_code& ec) {
      if (!id_) master_->Write(idx_, subidx_, p, n, ec);
      return *this;
    }

    /**
     * Checks if the sub-object can be mapped into a PDO and, if so, triggers
     * the transmission of every acyclic or event-driven Transmit-PDO into which
     * the sub-object is mapped.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Device::WriteEvent(uint16_t idx, uint8_t subidx)
     * @see Device::TpdoWriteEvent(uint8_t id, uint16_t idx, uint8_t subidx)
     */
    void
    WriteEvent() {
      if (id_)
        master_->TpdoWriteEvent(id_, idx_, subidx_);
      else
        master_->WriteEvent(idx_, subidx_);
    }

    /**
     * Checks if the sub-object can be mapped into a PDO and, if so, triggers
     * the transmission of every acyclic or event-driven Transmit-PDO into which
     * the sub-object is mapped.
     *
     * @param ec on error, the SDO abort code is stored in <b>ec</b>.
     *
     * @see Device::WriteEvent(uint16_t idx, uint8_t subidx, ::std::error_code& ec)
     * @see Device::TpdoWriteEvent(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec)
     */
    void
    WriteEvent(::std::error_code& ec) noexcept {
      if (id_)
        master_->TpdoWriteEvent(id_, idx_, subidx_, ec);
      else
        master_->WriteEvent(idx_, subidx_, ec);
    }

   private:
    SubObject(BasicMaster* master, uint16_t idx, uint8_t subidx) noexcept
        : SubObject(master, 0, idx, subidx) {}

    SubObject(BasicMaster* master, uint8_t id, uint16_t idx,
              uint8_t subidx) noexcept
        : master_(master), idx_(idx), subidx_(subidx), id_(id) {}

    BasicMaster* master_;
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
     * Returns the value of the sub-object by submitting an SDO upload request
     * to the local object dictionary.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Read()
     */
    template <class T>
    operator T() const {
      return Read<T>();
    }

    /**
     * Reads the value of the sub-object by submitting an SDO upload request to
     * the local object dictionary.
     *
     * @returns the result of the SDO request.
     *
     * @throws #lely::canopen::SdoError on error.
     *
     * @see Device::Read(uint16_t idx, uint8_t subidx) const
     * @see Device::RpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const
     * @see Device::TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const
     */
    template <class T>
    T
    Read() const {
      return id_ ? (is_rpdo_ ? master_->RpdoRead<T>(id_, idx_, subidx_)
                             : master_->TpdoRead<T>(id_, idx_, subidx_))
                 : master_->Read<T>(idx_, subidx_);
    }

    /**
     * Reads the value of the sub-object by submitting an SDO upload request to
     * the local object dictionary.
     *
     * @param ec on error, the SDO abort code is stored in <b>ec</b>.
     *
     * @returns the result of the SDO request, or an empty value on error.
     *
     * @see Device::Read(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     * @see Device::RpdoRead(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     * @see Device::TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    template <class T>
    T
    Read(::std::error_code& ec) const {
      return id_ ? (is_rpdo_ ? master_->RpdoRead<T>(id_, idx_, subidx_, ec)
                             : master_->TpdoRead<T>(id_, idx_, subidx_, ec))
                 : master_->Read<T>(idx_, subidx_, ec);
    }

   private:
    ConstSubObject(const BasicMaster* master, uint16_t idx,
                   uint8_t subidx) noexcept
        : ConstSubObject(master, 0, idx, subidx, false) {}

    ConstSubObject(const BasicMaster* master, uint8_t id, uint16_t idx,
                   uint8_t subidx, bool is_rpdo) noexcept
        : master_(master),
          idx_(idx),
          subidx_(subidx),
          id_(id),
          is_rpdo_(is_rpdo) {}

    const BasicMaster* master_;
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
    friend class TpdoMapped;
    friend class BasicMaster;

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
      return SubObject(master_, id_, idx_, subidx);
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
      return ConstSubObject(master_, id_, idx_, subidx, false);
    }

   private:
    Object(BasicMaster* master, uint16_t idx) noexcept
        : Object(master, 0, idx) {}

    Object(BasicMaster* master, uint8_t id, uint16_t idx) noexcept
        : master_(master), idx_(idx), id_(id) {}

    BasicMaster* master_;
    uint16_t idx_;
    uint8_t id_;
  };

  /**
   * An accessor providing read-only access to a CANopen object in a local
   * object dictionary.
   */
  class ConstObject {
    friend class RpdoMapped;
    friend class TpdoMapped;
    friend class BasicMaster;

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
      return ConstSubObject(master_, id_, idx_, subidx, is_rpdo_);
    }

   private:
    ConstObject(const BasicMaster* master, uint16_t idx) noexcept
        : ConstObject(master, 0, idx, false) {}

    ConstObject(const BasicMaster* master, uint8_t id, uint16_t idx,
                bool is_rpdo) noexcept
        : master_(master), idx_(idx), id_(id), is_rpdo_(is_rpdo) {}

    const BasicMaster* master_;
    uint16_t idx_;
    uint8_t id_ : 7;
    uint8_t is_rpdo_ : 1;
  };

  /**
   * An accessor providing read-only access to TPDO-mapped objects in a remote
   * object dictionary.
   */
  class RpdoMapped {
    friend class BasicMaster;

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
      return ConstObject(master_, id_, idx, true);
    }

   private:
    RpdoMapped(const BasicMaster* master, uint8_t id) noexcept
        : master_(master), id_(id) {}

    const BasicMaster* master_;
    uint8_t id_;
  };

  /**
   * A mutator providing read/write access to TPDO-mapped objects in a remote
   * object dictionary.
   */
  class TpdoMapped {
    friend class BasicMaster;

   public:
    /**
     * Returns a mutator object that provides read/write access to the specified
     * PDO-mapped object in the remote object dictionary. Note that this
     * function succeeds even if the object does not exist.
     *
     * @param idx the object index.
     *
     * @returns a mutator object for a CANopen object in the remote object
     * dictionary.
     */
    Object
    operator[](uint16_t idx) noexcept {
      return Object(master_, id_, idx);
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
      return ConstObject(master_, id_, idx, false);
    }

   private:
    TpdoMapped(BasicMaster* master, uint8_t id) noexcept
        : master_(master), id_(id) {}

    BasicMaster* master_;
    uint8_t id_;
  };

  /// @see Node::TpdoEventMutex
  class TpdoEventMutex : public Node::TpdoEventMutex {
    friend class BasicMaster;

   public:
    void lock() override;
    void unlock() override;

   protected:
    using Node::TpdoEventMutex::TpdoEventMutex;
  };

  /**
   * The signature of the callback function invoked on completion of an
   * asynchronous read (SDO upload) operation from a remote object dictionary.
   * Note that the callback function SHOULD NOT throw exceptions. Since it is
   * invoked from C, any exception that is thrown cannot be caught and will
   * result in a call to `std::terminate()`.
   *
   * @param id     the node-ID (in the range[1..127]).
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     the SDO abort code (0 on success).
   * @param value  the value received from the SDO server.
   */
  template <class T>
  using ReadSignature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                             ::std::error_code ec, T value);

  /**
   * The signature of the callback function invoked on completion of an
   * asynchronous write (SDO download) operation to a remote object dictionary.
   * Note that the callback function SHOULD NOT throw exceptions. Since it is
   * invoked from C, any exception that is thrown cannot be caught and will
   * result in a call to `std::terminate()`.
   *
   * @param id     the node-ID (in the range[1..127]).
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     the SDO abort code (0 on success).
   */
  using WriteSignature = void(uint8_t id, uint16_t idx, uint8_t subidx,
                              ::std::error_code ec);

  /**
   * Creates a new CANopen master. After creation, the master is in the NMT
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
  explicit BasicMaster(ev_exec_t* exec, io::TimerBase& timer,
                       io::CanChannelBase& chan, __co_dev* dev,
                       uint8_t id = 0xff);

  /// Creates a new CANopen master.
  explicit BasicMaster(io::TimerBase& timer, io::CanChannelBase& chan,
                       __co_dev* dev, uint8_t id = 0xff)
      : BasicMaster(nullptr, timer, chan, dev, id) {}

  /**
   * Creates a new CANopen master. After creation, the master is in the NMT
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
  explicit BasicMaster(ev_exec_t* exec, io::TimerBase& timer,
                       io::CanChannelBase& chan, const ::std::string& dcf_txt,
                       const ::std::string& dcf_bin = "", uint8_t id = 0xff);

  /// Creates a new CANopen master.
  explicit BasicMaster(io::TimerBase& timer, io::CanChannelBase& chan,
                       const ::std::string& dcf_txt,
                       const ::std::string& dcf_bin = "", uint8_t id = 0xff)
      : BasicMaster(nullptr, timer, chan, dcf_txt, dcf_bin, id) {}

  /**
   * Creates a new CANopen master. After creation, the master is in the NMT
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
  explicit BasicMaster(ev_exec_t* exec, io::TimerBase& timer,
                       io::CanChannelBase& chan, const co_sdev* sdev,
                       uint8_t id = 0xff);

  /// Creates a new CANopen master.
  explicit BasicMaster(io::TimerBase& timer, io::CanChannelBase& chan,
                       const co_sdev* sdev, uint8_t id = 0xff)
      : BasicMaster(nullptr, timer, chan, sdev, id) {}

  virtual ~BasicMaster();

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
   * Requests the NMT 'boot slave' process for the specified node. OnBoot() is
   * invoked once the boot-up process completes.
   */
  bool Boot(uint8_t id);

  /**
   * Returns true if the remote node is ready (i.e., the NMT 'boot slave'
   * process has successfully completed and no subsequent boot-up event has been
   * received) and false if not. Invoking AsyncDeconfig() will also mark a node
   * as not ready.
   *
   * If this function returns true, the default client-SDO service is available
   * for the given node.
   *
   * @see IsConfig()
   */
  bool IsReady(uint8_t id) const;

  /**
   * Queues the DriverBase::OnDeconfig() method for the driver with the
   * specified node-ID and creates a future which becomes ready once the
   * deconfiguration process completes.
   *
   * @returns a future which holds an error code on failure.
   *
   * @post IsReady(id) returns false.
   */
  ev::Future<void> AsyncDeconfig(uint8_t id);

  /**
   * Queues the DriverBase::OnDeconfig() method for all registered drivers and
   * creates a future which becomes ready once all deconfiguration processes
   * complete.
   *
   * @returns a future which holds the number of deconfigured drivers.
   *
   * @post IsReady() returns false for all remote nodes.
   */
  ev::Future<::std::size_t, void> AsyncDeconfig();

  /**
   * Indicates the occurrence of an error event on a remote node and triggers
   * the error handling process (see Fig. 12 in CiA 302-2 v4.1.0). Note that
   * depending on the value of objects 1F80 (NMT startup) and 1F81 (NMT slave
   * assignment), the occurrence of an error event MAY trigger an NMT state
   * transition of the entire network, including the master.
   *
   * @param id the node-ID (in the range[1..127]).
   */
  void Error(uint8_t id);

  /*
   * Generates an EMCY error and triggers the error handling behavior according
   * to object 1029:01 (Error behavior object) in case of a communication error
   * (emergency error code 0x81xx).
   *
   * @param eec  the emergency error code.
   * @param er   the error register.
   * @param msef the manufacturer-specific error code.
   */
  void Error(uint16_t eec, uint8_t er,
             const uint8_t msef[5] = nullptr) noexcept;

  /**
   * Issues an NMT command to a slave.
   *
   * @param cs the NMT command specifier.
   * @param id the node-ID (0 for all nodes, [1..127] for a specific slave).
   */
  void Command(NmtCommand cs, uint8_t id = 0);

  /// @see Node::RpdoRtr()
  void RpdoRtr(int num = 0) noexcept;

  /// @see Node::TpdoEvent()
  void TpdoEvent(int num = 0) noexcept;

  /// @see Node::DamMpdoEvent()
  template <class T>
  void
  DamMpdoEvent(int num, uint8_t id, uint16_t idx, uint8_t subidx, T value) {
    ::std::lock_guard<BasicLockable> lock(*this);

    Node::DamMpdoEvent(num, id, idx, subidx, value);
  }

  /**
   * Returns the SDO timeout used during the NMT 'boot slave' and 'check
   * configuration' processes.
   *
   * @see SetTimeout()
   */
  ::std::chrono::milliseconds GetTimeout() const;

  /**
   * Sets the SDO timeout used during the NMT 'boot slave' and 'check
   * configuration' processes.
   *
   * @see GetTimeout()
   */
  void SetTimeout(const ::std::chrono::milliseconds& timeout);

  /**
   * Equivalent to
   * #SubmitRead(uint8_t id, SdoUploadRequest<T>& req, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  void
  SubmitRead(uint8_t id, SdoUploadRequest<T>& req) {
    ::std::error_code ec;
    SubmitRead(id, req, ec);
    if (ec) throw SdoError(id, req.idx, req.subidx, ec, "SubmitRead");
  }

  /**
   * Queues an asynchronous read (SDO upload) operation.
   *
   * @param id  the node-ID (in the range[1..127]).
   * @param req the SDO upload request.
   * @param ec  the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *            client-SDO is available.
   */
  template <class T>
  void
  SubmitRead(uint8_t id, SdoUploadRequest<T>& req, ::std::error_code& ec) {
    ::std::lock_guard<BasicLockable> lock(*this);

    ec.clear();
    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      sdo->SubmitUpload(req);
    } else {
      ec = SdoErrc::NO_SDO;
    }
  }

  /**
   * Equivalent to
   * #SubmitRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
             F&& con) {
    SubmitRead<T>(exec, id, idx, subidx, ::std::forward<F>(con), GetTimeout());
  }

  /**
   * Equivalent to
   * #SubmitRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T, class F>
  void
  SubmitRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con,
             ::std::error_code& ec) {
    SubmitRead<T>(exec, id, idx, subidx, ::std::forward<F>(con), GetTimeout(),
                  ec);
  }

  /**
   * Equivalent to
   * #SubmitRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con,
             const ::std::chrono::milliseconds& timeout) {
    ::std::error_code ec;
    SubmitRead<T>(exec, id, idx, subidx, ::std::forward<F>(con), timeout, ec);
    if (ec) throw SdoError(id, idx, subidx, ec, "SubmitRead");
  }

  /**
   * Queues an asynchronous read (SDO upload) operation. This function reads the
   * value of a sub-object in a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con,
             const ::std::chrono::milliseconds& timeout,
             ::std::error_code& ec) {
    SubmitUpload<T>(exec, id, idx, subidx, ::std::forward<F>(con), false,
                    timeout, ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                  F&& con) {
    SubmitBlockRead<T>(exec, id, idx, subidx, ::std::forward<F>(con),
                       GetTimeout());
  }

  /**
   * Equivalent to
   * #SubmitBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T, class F>
  void
  SubmitBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                  F&& con, ::std::error_code& ec) {
    SubmitBlockRead<T>(exec, id, idx, subidx, ::std::forward<F>(con),
                       GetTimeout(), ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                  F&& con, const ::std::chrono::milliseconds& timeout) {
    ::std::error_code ec;
    SubmitBlockRead<T>(exec, id, idx, subidx, ::std::forward<F>(con), timeout,
                       ec);
    if (ec) throw SdoError(id, idx, subidx, ec, "SubmitBlockRead");
  }

  /**
   * Queues an asynchronous read (SDO block upload) operation. This function
   * reads the value of a sub-object in a remote object dictionary using SDO
   * block transfer. SDO block transfer is more effecient than segmented
   * transfer for large values, but may not be supported by the remote server.
   * If not, the operation will most likely fail with the #SdoErrc::NO_CS abort
   * code.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                  F&& con, const ::std::chrono::milliseconds& timeout,
                  ::std::error_code& ec) {
    SubmitUpload<T>(exec, id, idx, subidx, ::std::forward<F>(con), true,
                    timeout, ec);
  }

  /**
   * Queues an asynchronous SDO upload operation.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
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
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitUpload(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
               F&& con, bool block, const ::std::chrono::milliseconds& timeout,
               ::std::error_code& ec) {
    ::std::lock_guard<BasicLockable> lock(*this);

    ec.clear();
    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      sdo->SubmitUpload<T>(exec, idx, subidx, ::std::forward<F>(con), block,
                           timeout);
    } else {
      ec = SdoErrc::NO_SDO;
    }
  }

  /**
   * Equivalent to
   * #SubmitWrite(uint8_t id, SdoDownloadRequest<T>& req, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  void
  SubmitWrite(uint8_t id, SdoDownloadRequest<T>& req) {
    ::std::error_code ec;
    SubmitWrite(id, req, ec);
    if (ec) throw SdoError(id, req.idx, req.subidx, ec, "SubmitWrite");
  }

  /**
   * Queues an asynchronous write (SDO download) operation.
   *
   * @param id  the node-ID (in the range[1..127]).
   * @param req the SDO download request.
   * @param ec  the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *            client-SDO is available.
   */
  template <class T>
  void
  SubmitWrite(uint8_t id, SdoDownloadRequest<T>& req, ::std::error_code& ec) {
    ::std::lock_guard<BasicLockable> lock(*this);

    ec.clear();
    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      sdo->SubmitDownload(req);
    } else {
      ec = SdoErrc::NO_SDO;
    }
  }

  /**
   * Equivalent to
   * #SubmitWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
              T&& value, F&& con) {
    SubmitWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                ::std::forward<F>(con), GetTimeout());
  }

  /**
   * Equivalent to
   * #SubmitWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T, class F>
  void
  SubmitWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
              T&& value, F&& con, ::std::error_code& ec) {
    SubmitWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                ::std::forward<F>(con), GetTimeout(), ec);
  }

  /**
   * Equivalent to
   * #SubmitWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
              T&& value, F&& con, const ::std::chrono::milliseconds& timeout) {
    ::std::error_code ec;
    SubmitWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                ::std::forward<F>(con), timeout, ec);
    if (ec) throw SdoError(id, idx, subidx, ec, "SubmitWrite");
  }

  /**
   * Queues an asynchronous write (SDO download) operation. This function writes
   * a value to a sub-object in a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
              T&& value, F&& con, const ::std::chrono::milliseconds& timeout,
              ::std::error_code& ec) {
    SubmitDownload(exec, id, idx, subidx, ::std::forward<T>(value),
                   ::std::forward<F>(con), false, timeout, ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                   T&& value, F&& con) {
    SubmitBlockWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                     ::std::forward<F>(con), GetTimeout());
  }

  /**
   * Equivalent to
   * #SubmitBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T, class F>
  void
  SubmitBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                   T&& value, F&& con, ::std::error_code& ec) {
    SubmitBlockWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                     ::std::forward<F>(con), GetTimeout(), ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                   T&& value, F&& con,
                   const ::std::chrono::milliseconds& timeout) {
    ::std::error_code ec;
    SubmitBlockWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                     ::std::forward<F>(con), timeout, ec);
    if (ec) throw SdoError(id, idx, subidx, ec, "SubmitBlockWrite");
  }

  /**
   * Queues an asynchronous write (SDO block download) operation. This function
   * writes a value to a sub-object in a remote object dictionary using SDO
   * block transfer. SDO block transfer is more effecient than segmented
   * transfer for large values, but may not be supported by the remote server.
   * If not, the operation will most likely fail with the #SdoErrc::NO_CS abort
   * code.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                   T&& value, F&& con,
                   const ::std::chrono::milliseconds& timeout,
                   ::std::error_code& ec) {
    SubmitDownload(exec, id, idx, subidx, ::std::forward<T>(value),
                   ::std::forward<F>(con), true, timeout, ec);
  }

  /**
   * Queues an asynchronous SDO download operation.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
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
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitDownload(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                 T&& value, F&& con, bool block,
                 const ::std::chrono::milliseconds& timeout,
                 ::std::error_code& ec) {
    ::std::lock_guard<BasicLockable> lock(*this);

    ec.clear();
    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      sdo->SubmitDownload(exec, idx, subidx, ::std::forward<T>(value),
                          ::std::forward<F>(con), block, timeout);
    } else {
      ec = SdoErrc::NO_SDO;
    }
  }

  /**
   * Equivalent to
   * #SubmitWriteDcf(uint8_t id, SdoDownloadDcfRequest& req, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  void SubmitWriteDcf(uint8_t id, SdoDownloadDcfRequest& req);

  /**
   * Queues an asynchronous write (SDO download) operation.
   *
   * @param id  the node-ID (in the range[1..127]).
   * @param req the SDO download request.
   * @param ec  the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *            client-SDO is available.
   */
  void SubmitWriteDcf(uint8_t id, SdoDownloadDcfRequest& req,
                      ::std::error_code& ec);

  /**
   * Equivalent to
   * #SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t *begin, const uint8_t *end, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t* begin,
                 const uint8_t* end, F&& con) {
    SubmitWriteDcf(exec, id, begin, end, ::std::forward<F>(con), GetTimeout());
  }

  /**
   * Equivalent to
   * #SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t *begin, const uint8_t *end, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t* begin,
                 const uint8_t* end, F&& con, ::std::error_code& ec) {
    SubmitWriteDcf(exec, id, begin, end, ::std::forward<F>(con), GetTimeout(),
                   ec);
  }

  /**
   * Equivalent to
   * #SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t *begin, const uint8_t *end, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t* begin,
                 const uint8_t* end, F&& con,
                 const ::std::chrono::milliseconds& timeout) {
    ::std::error_code ec;
    SubmitWriteDcf(exec, id, begin, end, ::std::forward<F>(con), timeout, ec);
    if (ec) throw SdoError(id, 0, 0, ec, "SubmitWriteDcf");
  }

  /**
   * Queues a series of asynchronous write (SDO download) operations. This
   * function writes each entry in the specified concise DCF to a sub-object in
   * a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
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
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t* begin,
                 const uint8_t* end, F&& con,
                 const ::std::chrono::milliseconds& timeout,
                 ::std::error_code& ec) {
    ::std::lock_guard<BasicLockable> lock(*this);

    ec.clear();
    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      sdo->SubmitDownloadDcf(exec, begin, end, ::std::forward<F>(con), timeout);
    } else {
      ec = SdoErrc::NO_SDO;
    }
  }

  /**
   * Equivalent to
   * #SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, F&& con) {
    SubmitWriteDcf(exec, id, path, ::std::forward<F>(con), GetTimeout());
  }

  /**
   * Equivalent to
   * #SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, F&& con,
                 ::std::error_code& ec) {
    SubmitWriteDcf(exec, id, path, ::std::forward<F>(con), GetTimeout(), ec);
  }

  /**
   * Equivalent to
   * #SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, F&& con,
                 const ::std::chrono::milliseconds& timeout) {
    ::std::error_code ec;
    SubmitWriteDcf(exec, id, path, ::std::forward<F>(con), timeout, ec);
    if (ec) throw SdoError(id, 0, 0, ec, "SubmitWriteDcf");
  }

  /**
   * Queues a series of asynchronous write (SDO download) operations. This
   * function writes each entry in the specified concise DCF to a sub-object in
   * a remote object dictionary.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param path    the path of the concise DCF.
   * @param con     the confirmation function to be called when all SDO download
   *                requests are successfully completed, or when an error
   *                occurs.
   * @param timeout the SDO timeout. If, after a single request is initiated,
   *                the timeout expires before receiving a response from the
   *                server, the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class F>
  void
  SubmitWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, F&& con,
                 const ::std::chrono::milliseconds& timeout,
                 ::std::error_code& ec) {
    ::std::lock_guard<BasicLockable> lock(*this);

    ec.clear();
    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      sdo->SubmitDownloadDcf(exec, path, ::std::forward<F>(con), timeout);
    } else {
      ec = SdoErrc::NO_SDO;
    }
  }

  /**
   * Equivalent to
   * #AsyncRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T>
  SdoFuture<T>
  AsyncRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx) {
    return AsyncRead<T>(exec, id, idx, subidx, GetTimeout());
  }

  /**
   * Queues an asynchronous read (SDO upload) operation and creates a future
   * which becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the received value on success and the SDO
   * error on failure.
   */
  template <class T>
  SdoFuture<T>
  AsyncRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
            const ::std::chrono::milliseconds& timeout) {
    return AsyncUpload<T>(exec, id, idx, subidx, false, timeout);
  }

  /**
   * Equivalent to
   * #AsyncBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T>
  SdoFuture<T>
  AsyncBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx) {
    return AsyncBlockRead<T>(exec, id, idx, subidx, GetTimeout());
  }

  /**
   * Queues an asynchronous read (SDO block upload) operation and creates a
   * future which becomes ready once the request completes (or is canceled).
   * This function uses SDO block transfer, which is more effecient than
   * segmented transfer for large values, but may not be supported by the remote
   * server. If not, the operation will most likely fail with the
   * #SdoErrc::NO_CS abort code.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the received value on success and the SDO
   * error on failure.
   */
  template <class T>
  SdoFuture<T>
  AsyncBlockRead(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                 const ::std::chrono::milliseconds& timeout) {
    return AsyncUpload<T>(exec, id, idx, subidx, true, timeout);
  }

  /**
   * Queues an asynchronous SDO upload operation and creates a future which
   * becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
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
   * error on failure.
   */
  template <class T>
  SdoFuture<T>
  AsyncUpload(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
              bool block, const ::std::chrono::milliseconds& timeout) {
    if (!exec) exec = GetExecutor();

    ::std::lock_guard<BasicLockable> lock(*this);

    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      return sdo->AsyncUpload<T>(exec, idx, subidx, block, timeout);
    } else {
      return make_error_sdo_future<T>(id, idx, subidx, SdoErrc::NO_SDO,
                                      "AsyncRead");
    }
  }

  /**
   * Equivalent to
   * #AsyncWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T>
  SdoFuture<void>
  AsyncWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
             T&& value) {
    return AsyncWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                      GetTimeout());
  }

  /**
   * Queues an asynchronous write (SDO download) operation and creates a future
   * which becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO error on failure.
   */
  template <class T>
  SdoFuture<void>
  AsyncWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
             T&& value, const ::std::chrono::milliseconds& timeout) {
    return AsyncDownload(exec, id, idx, subidx, ::std::forward<T>(value), false,
                         timeout);
  }

  /**
   * Equivalent to
   * #AsyncBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx, T&& value, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  template <class T>
  SdoFuture<void>
  AsyncBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                  T&& value) {
    return AsyncBlockWrite(exec, id, idx, subidx, ::std::forward<T>(value),
                           GetTimeout());
  }

  /**
   * Queues an asynchronous write (SDO block download) operation and creates a
   * future which becomes ready once the request completes (or is canceled).
   * This function uses SDO block transfer, which is more effecient than
   * segmented transfer for large values, but may not be supported by the remote
   * server. If not, the operation will most likely fail with the
   * #SdoErrc::NO_CS abort code.
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO error on failure.
   */
  template <class T>
  SdoFuture<void>
  AsyncBlockWrite(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                  T&& value, const ::std::chrono::milliseconds& timeout) {
    return AsyncDownload(exec, id, idx, subidx, ::std::forward<T>(value), true,
                         timeout);
  }

  /**
   * Queues an asynchronous SDO download operation and creates a future which
   * becomes ready once the request completes (or is canceled).
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
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
   * @returns a future which holds the SDO error on failure.
   */
  template <class T>
  SdoFuture<void>
  AsyncDownload(ev_exec_t* exec, uint8_t id, uint16_t idx, uint8_t subidx,
                T&& value, bool block,
                const ::std::chrono::milliseconds& timeout) {
    if (!exec) exec = GetExecutor();

    ::std::lock_guard<BasicLockable> lock(*this);

    auto sdo = GetSdo(id);
    if (sdo) {
      SetTime();
      return sdo->AsyncDownload<T>(exec, idx, subidx, ::std::forward<T>(value),
                                   block, timeout);
    } else {
      return make_error_sdo_future<void>(id, idx, subidx, SdoErrc::NO_SDO,
                                         "AsyncWrite");
    }
  }

  /**
   * Equivalent to
   * #AsyncWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t* begin, const uint8_t* end, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  SdoFuture<void>
  AsyncWriteDcf(ev_exec_t* exec, uint8_t id, const uint8_t* begin,
                const uint8_t* end) {
    return AsyncWriteDcf(exec, id, begin, end, GetTimeout());
  }

  /**
   * Queues a series of asynchronous write (SDO download) operations,
   * corresponding to the entries in the specified concise DCF, and creates a
   * future which becomes ready once all requests complete (or an error occurs).
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param begin   a pointer the the first byte in a concise DCF (see object
   *                1F22 in CiA 302-3 version 4.1.0).
   * @param end     a pointer to one past the last byte in the concise DCF. At
   *                most `end - begin` bytes are read.
   * @param timeout the SDO timeout. If, after a single request is initiated,
   *                the timeout expires before receiving a response from the
   *                server, the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO error on failure.
   */
  SdoFuture<void> AsyncWriteDcf(ev_exec_t* exec, uint8_t id,
                                const uint8_t* begin, const uint8_t* end,
                                const ::std::chrono::milliseconds& timeout);

  /**
   * Equivalent to
   * #AsyncWriteDcf(ev_exec_t* exec, uint8_t id, const char* path, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by #GetTimeout().
   */
  SdoFuture<void>
  AsyncWriteDcf(ev_exec_t* exec, uint8_t id, const char* path) {
    return AsyncWriteDcf(exec, id, path, GetTimeout());
  }

  /**
   * Queues a series of asynchronous write (SDO download) operations,
   * corresponding to the entries in the specified concise DCF, and creates a
   * future which becomes ready once all requests complete (or an error occurs).
   *
   * @param exec    the executor used to execute the completion task.
   * @param id      the node-ID (in the range[1..127]).
   * @param path    the path to a concise DCF.
   * @param timeout the SDO timeout. If, after a single request is initiated,
   *                the timeout expires before receiving a response from the
   *                server, the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO error on failure.
   */
  SdoFuture<void> AsyncWriteDcf(ev_exec_t* exec, uint8_t id, const char* path,
                                const ::std::chrono::milliseconds& timeout);

  /**
   * Registers a driver for a remote CANopen node. If an event occurs for that
   * node, or for the entire CANopen network, the corresponding method of the
   * driver will be invoked.
   *
   * @throws std::out_of_range if the node-ID is invalid or already registered.
   *
   * @see Erase()
   */
  void Insert(DriverBase& driver);

  /**
   * Unregisters a driver for a remote CANopen node.
   *
   * @see Insert()
   */
  void Erase(DriverBase& driver);

  /// @see Node::OnCanState()
  void
  OnCanState(::std::function<void(io::CanState, io::CanState)> on_can_state) {
    Node::OnCanState(on_can_state);
  }

  /// @see Node::OnCanError()
  void
  OnCanError(::std::function<void(io::CanError)> on_can_error) {
    Node::OnCanError(on_can_error);
  }

  /// @see Device::OnRpdoWrite()
  void
  OnRpdoWrite(::std::function<void(uint8_t, uint16_t, uint8_t)> on_rpdo_write) {
    Node::OnRpdoWrite(on_rpdo_write);
  }

  /// @see Node::OnCommand()
  void
  OnCommand(::std::function<void(NmtCommand)> on_command) {
    Node::OnCommand(on_command);
  }

  /// @see Node::OnHeartbeat()
  void
  OnHeartbeat(::std::function<void(uint8_t, bool)> on_heartbeat) {
    Node::OnHeartbeat(on_heartbeat);
  }

  /// @see Node::OnState()
  void
  OnState(::std::function<void(uint8_t, NmtState)> on_state) {
    Node::OnState(on_state);
  }

  /// @see Node::OnSync()
  void
  OnSync(::std::function<void(uint8_t, const time_point&)> on_sync) {
    Node::OnSync(on_sync);
  }

  /// @see Node::OnSyncError()
  void
  OnSyncError(::std::function<void(uint16_t, uint8_t)> on_sync_error) {
    Node::OnSyncError(on_sync_error);
  }

  /// @see Node::OnTime()
  void
  OnTime(::std::function<void(const ::std::chrono::system_clock::time_point&)>
             on_time) {
    Node::OnTime(on_time);
  }

  /// @see Node::OnEmcy()
  void
  OnEmcy(
      ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy) {
    Node::OnEmcy(on_emcy);
  }

  /**
   * Registers the function invoked when a node guarding timeout event occurs or
   * is resolved. Only a single function can be registered at any one time. If
   * <b>on_node_guarding</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnNodeGuarding(uint8_t, bool) completes.
   */
  void OnNodeGuarding(::std::function<void(uint8_t, bool)> on_node_guarding);

  /**
   * Registers the function invoked when the NMT 'boot slave' process completes.
   * Only a single function can be registered at any one time. If <b>on_boot</b>
   * contains a callable function target, a copy of the target is invoked
   * _after_ OnBoot(uint8_t, NmtState, char, const ::std::string&) completes.
   */
  void OnBoot(
      ::std::function<void(uint8_t, NmtState, char, const ::std::string&)>
          on_boot);

  /// @see Node::tpdo_event_mutex
  TpdoEventMutex tpdo_event_mutex;

 protected:
  using MapType = ::std::map<uint8_t, DriverBase*>;

  /**
   * The default implementation invokes #lely::canopen::Node::OnCanState() and
   * notifies each registered driver.
   *
   * @see Node::OnCanState(), DriverBase::OnCanState()
   */
  void OnCanState(io::CanState new_state,
                  io::CanState old_state) noexcept override;

  /**
   * The default implementation notifies all registered drivers.
   *
   * @see Node::OnCanError(), DriverBase::OnCanError()
   */
  void OnCanError(io::CanError error) noexcept override;

  /**
   * The default implementation notifies the driver registered for node
   * <b>id</b>.
   *
   * @see Device::OnRpdoWrite(), DriverBase::OnRpdoWrite()
   */
  void OnRpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx) noexcept override;

  /**
   * The default implementation notifies all registered drivers. Unless the
   * master enters the pre-operational or operational state, all ongoing and
   * pending SDO requests are aborted.
   *
   * @see Node::OnCommand(), DriverBase::OnCommand()
   */
  void OnCommand(NmtCommand cs) noexcept override;

  /**
   * The default implementation notifies the driver registered for node
   * <b>id</b>.
   *
   * @see Node::OnHeartbeat(), DriverBase::OnHeartbeat()
   */
  void OnHeartbeat(uint8_t id, bool occurred) noexcept override;

  /**
   * The default implementation notifies the driver registered for node
   * <b>id</b>. If a boot-up event (`st == NmtState::BOOTUP`) is detected, any
   * ongoing or pending SDO requests for the slave are aborted. This is
   * necessary because the master MAY need the Client-SDO service for the NMT
   * 'boot slave' process.
   *
   * @see Node::OnState(), DriverBase::OnState()
   */
  void OnState(uint8_t id, NmtState st) noexcept override;

  /**
   * The default implementation notifies all registered drivers.
   *
   * @see Node::OnSync(), DriverBase::OnSync()
   */
  void OnSync(uint8_t cnt, const time_point& t) noexcept override;

  /**
   * The default implementation notifies all registered drivers.
   *
   * @see Node::OnSyncError(), DriverBase::OnSyncError()
   */
  void OnSyncError(uint16_t eec, uint8_t er) noexcept override;

  /**
   * The default implementation notifies all registered drivers.
   *
   * @see Node::OnTime(), DriverBase::OnTime()
   */
  void OnTime(const ::std::chrono::system_clock::time_point& abs_time) noexcept
      override;

  /**
   * The default implementation notifies the driver registered for node
   * <b>id</b>.
   *
   * @see Node::OnEmcy(), DriverBase::OnEmcy()
   */
  void OnEmcy(uint8_t id, uint16_t eec, uint8_t er,
              uint8_t msef[5]) noexcept override;

  /**
   * The function invoked when a node guarding timeout event occurs or is
   * resolved. Note that depending on the value of object 1029:01 (Error
   * behavior object), the occurrence of a node guarding event MAY trigger an
   * NMT state transition. If so, this function is called _after_ the state
   * change completes.
   *
   * The default implementation notifies the driver registered for node
   * <b>id</b>.
   *
   * @param id       the node-ID (in the range [1..127]).
   * @param occurred `true` if the node guarding event occurred, `false` if it
   *                 was resolved.
   *
   * @see DriverBase::OnNodeGuarding()
   */
  virtual void OnNodeGuarding(uint8_t id, bool occurred) noexcept;

  /**
   * The function invoked when the NMT 'boot slave' process completes.
   *
   * The default implementation notifies the driver registered for node
   * <b>id</b>.
   *
   * @param id   the node-ID (in the range [1..127]).
   * @param st   the state of the remote node (including the toggle bit
   *             (#NmtState::TOGGLE) if node guarding is enabled).
   * @param es   the error status (in the range ['A'..'O'], or 0 on success):
   *             - 'A': The CANopen device is not listed in object 1F81.
   *             - 'B': No response received for upload request of object 1000.
   *             - 'C': Value of object 1000 from CANopen device is different to
   *               value in object 1F84 (%Device type).
   *             - 'D': Value of object 1018:01 from CANopen device is different
   *               to value in object 1F85 (Vendor-ID).
   *             - 'E': Heartbeat event. No heartbeat message received from
   *               CANopen device.
   *             - 'F': Node guarding event. No confirmation for guarding
   *               request received from CANopen device.
   *             - 'G': Objects for program download are not configured or
   *               inconsistent.
   *             - 'H': Software update is required, but not allowed because of
   *               configuration or current status.
   *             - 'I': Software update is required, but program download
   *               failed.
   *             - 'J': Configuration download failed.
   *             - 'K': Heartbeat event during start error control service. No
   *               heartbeat message received from CANopen device during start
   *               error control service.
   *             - 'L': NMT slave was initially operational. (CANopen manager
   *               may resume operation with other CANopen devices)
   *             - 'M': Value of object 1018:02 from CANopen device is different
   *               to value in object 1F86 (Product code).
   *             - 'N': Value of object 1018:03 from CANopen device is different
   *               to value in object 1F87 (Revision number).
   *             - 'O': Value of object 1018:04 from CANopen device is different
   *               to value in object 1F88 (Serial number).
   * @param what if <b>es</b> is non-zero, contains a string explaining the
   *             error.
   *
   * @see DriverBase::OnBoot()
   */
  virtual void OnBoot(uint8_t id, NmtState st, char es,
                      const ::std::string& what) noexcept;

  /**
   * Marks a remote note as ready or not ready.
   *
   * @post IsReady(id) returns <b>ready</b>.
   */
  void IsReady(uint8_t id, bool ready) noexcept;

  /**
   * The function invoked when the 'update configuration' step is reached during
   * the NMT 'boot slave' process. The 'boot slave' process is halted until the
   * result of the 'update configuration' step is communicated to the NMT
   * service with #ConfigResult().
   *
   * The default implementation delegates the configuration update to the
   * driver, if one is registered for node <b>id</b>. If not, a successful
   * result is communicated to the NMT service.
   *
   * @see IsConfig(), DriverBase::OnConfig()
   */
  virtual void OnConfig(uint8_t id) noexcept;

  /**
   * Reports the result of the 'update configuration' step to the NMT service.
   *
   * @param id the node-ID (in the range [1..127]).
   * @param ec the SDO abort code (0 on success).
   */
  void ConfigResult(uint8_t id, ::std::error_code ec) noexcept;

  /**
   * Returns true if the remote node is configuring (i.e., the 'update
   * configuration' step of the NMT 'boot slave' is reached but not yet
   * completed) and false if not.
   *
   * If this function returns true, the default client-SDO service is available
   * for the given node.
   *
   * @see IsReady(), OnConfig()
   */
  bool IsConfig(uint8_t id) const;

  /**
   * Returns a pointer to the default client-SDO service for the given node. If
   * the master is not in the pre-operational or operational state, or if the
   * master needs the client-SDO to boot the node, a null pointer is returned.
   */
  Sdo* GetSdo(uint8_t id);

  /**
   * Aborts any ongoing or pending SDO requests for the specified slave.
   *
   * @param id the node-ID (0 for all nodes, [1..127] for a specific slave).
   */
  void CancelSdo(uint8_t id = 0);

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

/**
 * An asynchronous CANopen master. When a CANopen event occurs, this master
 * queues a notification to (the executor of) each registered driver. The master
 * itself does not block waiting for events to be handled.
 */
class AsyncMaster : public BasicMaster {
 public:
  using BasicMaster::BasicMaster;

  /// @see Node::OnCanState()
  void
  OnCanState(::std::function<void(io::CanState, io::CanState)> on_can_state) {
    BasicMaster::OnCanState(on_can_state);
  }

  /// @see Node::OnCanError()
  void
  OnCanError(::std::function<void(io::CanError)> on_can_error) {
    BasicMaster::OnCanError(on_can_error);
  }

  /// @see Device::OnRpdoWrite()
  void
  OnRpdoWrite(::std::function<void(uint8_t, uint16_t, uint8_t)> on_rpdo_write) {
    BasicMaster::OnRpdoWrite(on_rpdo_write);
  }

  /// @see Node::OnCommand()
  void
  OnCommand(::std::function<void(NmtCommand)> on_command) {
    BasicMaster::OnCommand(on_command);
  }

  /// @see Node::OnHeartbeat()
  void
  OnHeartbeat(::std::function<void(uint8_t, bool)> on_heartbeat) {
    BasicMaster::OnHeartbeat(on_heartbeat);
  }

  /// @see Node::OnState()
  void
  OnState(::std::function<void(uint8_t, NmtState)> on_state) {
    BasicMaster::OnState(on_state);
  }

  /// @see Node::OnSync()
  void
  OnSync(::std::function<void(uint8_t, const time_point&)> on_sync) {
    BasicMaster::OnSync(on_sync);
  }

  /// @see Node::OnSyncError()
  void
  OnSyncError(::std::function<void(uint16_t, uint8_t)> on_sync_error) {
    BasicMaster::OnSyncError(on_sync_error);
  }

  /// @see Node::OnTime()
  void
  OnTime(::std::function<void(const ::std::chrono::system_clock::time_point&)>
             on_time) {
    BasicMaster::OnTime(on_time);
  }

  /// @see Node::OnEmcy()
  void
  OnEmcy(
      ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy) {
    BasicMaster::OnEmcy(on_emcy);
  }

  /// @see BasicMaster::OnNodeGuarding()
  void
  OnNodeGuarding(::std::function<void(uint8_t, bool)> on_node_guarding) {
    BasicMaster::OnNodeGuarding(on_node_guarding);
  }

  /// @see BasicMaster::OnBoot()
  void
  OnBoot(::std::function<void(uint8_t, NmtState, char, const ::std::string&)>
             on_boot) {
    BasicMaster::OnBoot(on_boot);
  }

 protected:
  /**
   * The default implementation invokes #lely::canopen::Node::OnCanState() and
   * queues a notification for each registered driver.
   *
   * @see Node::OnCanState(), DriverBase::OnCanState()
   */
  void OnCanState(io::CanState new_state,
                  io::CanState old_state) noexcept override;

  /**
   * The default implementation queues a notification for all registered
   * drivers.
   *
   * @see Node::OnCanError(), DriverBase::OnCanError()
   */
  void OnCanError(io::CanError error) noexcept override;

  /**
   * The default implementation queues a notification for the driver registered
   * for node <b>id</b>.
   *
   * @see Device::OnRpdoWrite(), DriverBase::OnRpdoWrite()
   */
  void OnRpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx) noexcept override;

  /**
   * The default implementation queues a notification for all registered
   * drivers. Unless the master enters the pre-operational or operational state,
   * all ongoing and pending SDO requests are aborted.
   *
   * @see Node::OnCommand(), DriverBase::OnCommand()
   */
  void OnCommand(NmtCommand cs) noexcept override;

  /**
   * The default implementation queues a notification for the driver registered
   * for node <b>id</b>.
   *
   * @see Node::OnHeartbeat(), DriverBase::OnHeartbeat()
   */
  void OnHeartbeat(uint8_t id, bool occurred) noexcept override;

  /**
   * The default implementation queues a notification for the driver registered
   * for node <b>id</b>. If a boot-up event (`st == NmtState::BOOTUP`) is
   * detected, any ongoing or pending SDO requests for the slave are aborted.
   *
   * @see Node::OnState(), DriverBase::OnState()
   */
  void OnState(uint8_t id, NmtState st) noexcept override;

  /**
   * The default implementation queues a notification for all registered
   * drivers.
   *
   * @see Node::OnSync(), DriverBase::OnSync()
   */
  void OnSync(uint8_t cnt, const time_point& t) noexcept override;

  /**
   * The default implementation queues a notification for all registered
   * drivers.
   *
   * @see Node::OnSyncError(), DriverBase::OnSyncError()
   */
  void OnSyncError(uint16_t eec, uint8_t er) noexcept override;

  /**
   * The default implementation queues a notification for all registered
   * drivers.
   *
   * @see Node::OnTime(), DriverBase::OnTime()
   */
  void OnTime(const ::std::chrono::system_clock::time_point& abs_time) noexcept
      override;

  /**
   * The default implementation queues a notification for the driver registered
   * for node <b>id</b>.
   *
   * @see Node::OnEmcy(), DriverBase::OnEmcy()
   */
  void OnEmcy(uint8_t id, uint16_t eec, uint8_t er,
              uint8_t msef[5]) noexcept override;

  /**
   * The default implementation queues a notification for all registered
   * drivers.
   *
   * @see BasicMaster::OnNodeGuarding(), DriverBase::OnNodeGuarding()
   */
  void OnNodeGuarding(uint8_t id, bool occurred) noexcept override;

  /**
   * The default implementation queues a notification for the driver registered
   * for node <b>id</b>.
   *
   * @see BasicMaster::OnBoot(), DriverBase::OnBoot()
   */
  void OnBoot(uint8_t id, NmtState st, char es,
              const ::std::string& what) noexcept override;

  /**
   * The default implementation queues a notification for the driver registered
   * for node <b>id</b>.
   *
   * @see BasicMaster::OnConfig(), DriverBase::OnConfig()
   */
  void OnConfig(uint8_t id) noexcept override;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COCPP_MASTER_HPP_
