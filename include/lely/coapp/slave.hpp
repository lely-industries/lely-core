/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen slave declarations.
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

#ifndef LELY_COCPP_SLAVE_HPP_
#define LELY_COCPP_SLAVE_HPP_

#include <lely/coapp/node.hpp>

namespace lely {

namespace canopen {

//! The base class for CANopen slaves.
class LELY_COAPP_EXTERN BasicSlave : public Node {
 public:
  /*!
   * Creates a new CANopen slave. After creation, the slave is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * \param timer   the timer used for CANopen events.
   * \param bus     a handle to the CAN bus.
   * \param dcf_txt the path of the text EDS or DCF containing the device
   *                description.
   * \param dcf_bin the path of the (binary) concise DCF containing the values
   *                of (some of) the objets in the object dictionary. If
   *                \a dcf_bin is empty, no concise DCF is loaded.
   * \param id      the node-ID (in the range [1..127, 255]). If \a id is 255
   *                (unconfigured), the node-ID is obtained from the DCF.
   */
  BasicSlave(aio::TimerBase& timer, aio::CanBusBase& bus,
      const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
      uint8_t id = 0xff);

  virtual ~BasicSlave();

 protected:
  class Object;
  class ConstObject;

  /*!
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

    /*!
     * Sets the value of the sub-object.
     *
     * \param value the value to be written.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * \see Set()
     */
    template <class T>
    SubObject&
    operator=(T&& value) {
      Set(::std::forward<T>(value));
      return *this;
    }

    /*!
     * Returns a copy of the value of the sub-object.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * \see Get()
     */
    template <class T> operator T() const { return Get<T>(); }

    /*!
     * Returns the type of the sub-object.
     *
     * \returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist.
     *
     * \see Device::Type(uint16_t idx, uint8_t subidx) const
     */
    const ::std::type_info&
    Type() const { return slave_->Type(idx_, subidx_); }

    /*!
     * Returns the type of the sub-object.
     *
     * \param ec if the sub-object does not exist, the SDO abort code is stored
     *           in \a ec.
     *
     * \returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * \see Device::Type(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    const ::std::type_info&
    Type(::std::error_code& ec) const {
      return slave_->Type(idx_, subidx_, ec);
    }

    /*!
     * Reads the value of the sub-object.
     *
     * \returns a copy of the value of the sub-object.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * \see Device::Get(uint16_t idx, uint8_t subidx) const
     */
    template <class T>
    T Get() const { return slave_->Get<T>(idx_, subidx_); }

    /*!
     * Reads the value of the sub-object.
     *
     * \param ec if the sub-object does not exist or the type does not match,
     *           the SDO abort code is stored in \a ec.
     *
     * \returns a copy of the value of the sub-object, or an empty value on
     * error.
     *
     * \see Device::Get(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    template <class T>
    T
    Get(::std::error_code& ec) const {
      return slave_->Get<T>(idx_, subidx_, ec);
    }

    /*!
     * Writes a value to the sub-object.
     *
     * \param value the value to be written.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * \see Device::Set(uint16_t idx, uint8_t subidx, T&& value)
     */
    template <class T>
    void
    Set(T&& value) {
      slave_->Set(idx_, subidx_, ::std::forward<T>(value));
    }

    /*!
     * Writes a value to the sub-object.
     *
     * \param value the value to be written.
     * \param ec    if the sub-object does not exist or the type does not match,
     *              the SDO abort code is stored in \a ec.
     *
     * \see Device::Set(uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec),
     * Device::Set(uint16_t idx, uint8_t subidx, const T& value, ::std::error_code& ec),
     * Device::Set(uint16_t idx, uint8_t subidx, const char* value, ::std::error_code& ec),
     * Device::Set(uint16_t idx, uint8_t subidx, const char16_t* value, ::std::error_code& ec)
     */
    template <class T>
    void
    Set(T&& value, ::std::error_code& ec) {
      slave_->Set(idx_, subidx_, ::std::forward<T>(value), ec);
    }

    /*!
     * Writes an OCTET_STRING or DOMAIN value to the sub-object.
     *
     * \param p a pointer to the bytes to be written.
     * \param n the number of bytes to write.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * \see Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n)
     */
    void
    Set(const void* p, ::std::size_t n) {
      slave_->Set(idx_, subidx_, p, n);
    }

    /*!
     * Writes an OCTET_STRING or DOMAIN value to the sub-object.
     *
     * \param p  a pointer to the bytes to be written.
     * \param n  the number of bytes to write.
     * \param ec if the sub-object does not exist or the type does not match,
     *           the SDO abort code is stored in \a ac.
     *
     * \see Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n, ::std::error_code& ec)
     */
    void
    Set(const void* p, ::std::size_t n, ::std::error_code& ec) {
      slave_->Set(idx_, subidx_, p, n, ec);
    }

   private:
    SubObject(BasicSlave* slave, uint16_t idx, uint8_t subidx)
        : slave_(slave), idx_(idx), subidx_(subidx) {}

    BasicSlave* slave_;
    uint16_t idx_;
    uint8_t subidx_;
  };

  /*!
   * An accessor providing read-only access to a CANopen sub-object in a local
   * object dictionary.
   */
  class ConstSubObject {
    friend class Object;
    friend class ConstObject;

   public:
    /*!
     * Returns a copy of the value of the sub-object.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * \see Get()
     */
    template <class T> operator T() const { return Get<T>(); }

    /*!
     * Returns the type of the sub-object.
     *
     * \returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist.
     *
     * \see Device::Type(uint16_t idx, uint8_t subidx) const
     */
    const ::std::type_info&
    Type() const { return slave_->Type(idx_, subidx_); }

    /*!
     * Returns the type of the sub-object.
     *
     * \param ec if the sub-object does not exist, the SDO abort code is stored
     *           in \a ec.
     *
     * \returns a reference to an `std::type_info` object representing the type,
     * or `typeid(void)` if unknown.
     *
     * \see Device::Type(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    const ::std::type_info&
    Type(::std::error_code& ec) const {
      return slave_->Type(idx_, subidx_, ec);
    }

    /*!
     * Reads the value of the sub-object.
     *
     * \returns a copy of the value of the sub-object.
     *
     * \throws #lely::canopen::SdoError if the sub-object does not exist or the
     * type does not match.
     *
     * \see Device::Get(uint16_t idx, uint8_t subidx) const
     */
    template <class T>
    T Get() const { return slave_->Get<T>(idx_, subidx_); }

    /*!
     * Reads the value of the sub-object.
     *
     * \param ec if the sub-object does not exist or the type does not match,
     *           the SDO abort code is stored in \a ec.
     *
     * \returns a copy of the value of the sub-object, or an empty value on
     * error.
     *
     * \see Device::Get(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
     */
    template <class T>
    T
    Get(::std::error_code& ec) const {
      return slave_->Get<T>(idx_, subidx_, ec);
    }

   private:
    ConstSubObject(const BasicSlave* slave, uint16_t idx, uint8_t subidx)
        : slave_(slave), idx_(idx), subidx_(subidx) {}

    const BasicSlave* slave_;
    uint16_t idx_;
    uint8_t subidx_;
  };

  /*!
   * A mutator providing read/write access to a CANopen object in a local object
   * dictionary.
   */
  class Object {
    friend class BasicSlave;

   public:
    /*!
     * Returns a mutator object that provides read/write access to the specified
     * CANopen sub-object in the local object dictionary. Note that this
     * function succeeds even if the sub-object does not exist.
     *
     * \param subidx the object sub-index.
     *
     * \returns a mutator object for a CANopen sub-object in the local object
     * dictionary.
     */
    SubObject
    operator[](uint8_t subidx) { return SubObject(slave_, idx_, subidx); }

    /*!
     * Returns an accessor object that provides read-only access to the
     * specified CANopen sub-object in the local object dictionary. Note that
     * this function succeeds even if the object does not exist.
     *
     * \param subidx the object sub-index.
     *
     * \returns an accessor object for a CANopen sub-object in the local object
     * dictionary.
     */
    ConstSubObject
    operator[](uint8_t subidx) const {
      return ConstSubObject(slave_, idx_, subidx);
    }

   private:
    Object(BasicSlave* slave, uint16_t idx) : slave_(slave), idx_(idx) {}

    BasicSlave* slave_;
    uint16_t idx_;
  };

  /*!
   * An accessor providing read-only access to a CANopen object in a local
   * object dictionary.
   */
  class ConstObject {
    friend class BasicSlave;

   public:
    /*!
     * Returns an accessor object that provides read-only access to the
     * specified CANopen sub-object in the local object dictionary. Note that
     * this function succeeds even if the object does not exist.
     *
     * \param subidx the object sub-index.
     *
     * \returns an accessor object for a CANopen sub-object in the local object
     * dictionary.
     */
    ConstSubObject
    operator[](uint8_t subidx) const {
      return ConstSubObject(slave_, idx_, subidx);
    }

   private:
    ConstObject(const BasicSlave* slave, uint16_t idx)
        : slave_(slave), idx_(idx) {}

    const BasicSlave* slave_;
    uint16_t idx_;
  };

  /*!
   * Returns a mutator object that provides read/write access to the specified
   * CANopen object in the local object dictionary. Note that this function
   * succeeds even if the object does not exist.
   *
   * \param idx the object index.
   *
   * \returns a mutator object for a CANopen object in the local object
   * dictionary.
   */
  Object operator[](uint16_t idx) { return Object(this, idx); }

  /*!
   * Returns an accessor object that provides read-only access to the specified
   * CANopen object in the local object dictionary. Note that this function
   * succeeds even if the object does not exist.
   *
   * \param idx the object index.
   *
   * \returns an accessor object for a CANopen object in the local object
   * dictionary.
   */
  ConstObject
  operator[](uint16_t idx) const { return ConstObject(this, idx); }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
 private:
#endif
  /*!
   * The function invoked when a life guarding event occurs or is resolved. Note
   * that depending on the value of object 1029:01 (Error behavior object), the
   * occurrence of a life guarding event MAY trigger an NMT state transition. If
   * so, this function is called _after_ the state change completes.
   *
   * \param occurred `true` if the life guarding event occurred, `false` if it
   *                 was resolved.
   */
  virtual void OnLifeGuarding(bool occurred) noexcept { (void)occurred; };

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COCPP_SLAVE_HPP_
