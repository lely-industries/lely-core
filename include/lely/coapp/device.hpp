/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen device description declarations.
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

#ifndef LELY_COAPP_DEVICE_HPP_
#define LELY_COAPP_DEVICE_HPP_

#include <lely/coapp/detail/type_traits.hpp>
#include <lely/coapp/sdo_error.hpp>
#include <lely/util/mutex.hpp>

#include <memory>
#include <string>
#include <typeinfo>

namespace lely {

// The CANopen device from <lely/co/dev.hpp>.
class CODev;

namespace canopen {

/**
 * The CANopen device description. This class manages the object dictionary and
 * device setttings such as the network-ID and node-ID.
 */
class Device {
 public:
  /**
   * Creates a new CANopen device description.
   *
   * @param dcf_txt the path of the text EDS or DCF containing the device
   *                description.
   * @param dcf_bin the path of the (binary) concise DCF containing the values
   *                of (some of) the objets in the object dictionary. If
   *                <b>dcf_bin</b> is empty, no concise DCF is loaded.
   * @param id      the node-ID (in the range [1..127, 255]). If <b>id</b> is
   *                255 (unconfigured), the node-ID is obtained from the DCF.
   * @param mutex   an (optional) pointer to the mutex to be locked while the
   *                internal device description is accessed. The mutex MUST be
   *                unlocked when any member function is invoked.
   */
  Device(const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
         uint8_t id = 0xff, util::BasicLockable* mutex = nullptr);

  Device(const Device&) = delete;
  Device(Device&&) = default;

  Device& operator=(const Device&) = delete;
  Device& operator=(Device&&);

  /// Returns the network-ID.
  uint8_t netid() const noexcept;

  /// Returns the node-ID.
  uint8_t id() const noexcept;

  /**
   * Submits an SDO upload request to the local object dictionary. This function
   * reads the value of a sub-object while honoring all access checks and
   * executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   *
   * @returns the result of the SDO request.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type Read(
      uint16_t idx, uint8_t subidx) const;

  /**
   * Submits an SDO upload request to the local object dictionary. This function
   * reads the value of a sub-object while honoring all access checks and
   * executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   *
   * @returns the result of the SDO request, or an empty value on error.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type Read(
      uint16_t idx, uint8_t subidx, ::std::error_code& ec) const;

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a CANopen basic value to a sub-object while honoring all
   * access and range checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type Write(
      uint16_t idx, uint8_t subidx, T value);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a CANopen basic value to a sub-object while honoring all
   * access and range checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type Write(
      uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a CANopen array value to a sub-object while honoring all
   * access checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_array<T>::value>::type Write(
      uint16_t idx, uint8_t subidx, const T& value);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a CANopen array value to a sub-object while honoring all
   * access checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_array<T>::value>::type Write(
      uint16_t idx, uint8_t subidx, const T& value, ::std::error_code& ec);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a VISIBLE_STRING to a sub-object while honoring all access
   * checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) string to be written.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  void Write(uint16_t idx, uint8_t subidx, const char* value);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a VISIBLE_STRING to a sub-object while honoring all access
   * checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) string to be written.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  void Write(uint16_t idx, uint8_t subidx, const char* value,
             ::std::error_code& ec);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a UNICODE_STRING to a sub-object while honoring all access
   * checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) UCS-2 string to be
   *               written.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  void Write(uint16_t idx, uint8_t subidx, const char16_t* value);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a UNICODE_STRING to a sub-object while honoring all access
   * checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) UCS-2 string to be
   *               written.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  void Write(uint16_t idx, uint8_t subidx, const char16_t* value,
             ::std::error_code& ec);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes an OCTET_STRING or DOMAIN value to a sub-object while
   * honoring all access checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param p      a pointer to the bytes to be written.
   * @param n      the number of bytes to write.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  void Write(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes an OCTET_STRING or DOMAIN value to a sub-object while
   * honoring all access checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param p      a pointer to the bytes to be written.
   * @param n      the number of bytes to write.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  void Write(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
             ::std::error_code& ec);

  /**
   * Reads the value of a sub-object in a remote object dictionary by submitting
   * an SDO upload request to the corresponding PDO-mapped sub-object in the
   * local object dictionary. This function honors all access checks of the
   * local object dictionary and executes any registered callback function.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   *
   * @returns the result of the SDO request.
   *
   * @throws #lely::canopen::SdoError on error.
   *
   * @pre a valid mapping from remote TPDO-mapped sub-objects to local
   * RPDO-mapped sub-objects has been generated with UpdateRpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  RpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const;

  /**
   * Reads the value of a sub-object in a remote object dictionary by submitting
   * an SDO upload request to the corresponding PDO-mapped sub-object in the
   * local object dictionary. This function honors all access checks of the
   * local object dictionary and executes any registered callback function.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   *
   * @returns the result of the SDO request, or an empty value on error.
   *
   * @pre a valid mapping from remote TPDO-mapped sub-objects to local
   * RPDO-mapped sub-objects has been generated with UpdateRpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  RpdoRead(uint8_t id, uint16_t idx, uint8_t subidx,
           ::std::error_code& ec) const;

  /**
   * Submits an SDO upload request to a TPDO-mapped sub-object in the local
   * object dictionary, which reads the value that will be written to an
   * RPDO-mapped sub-object in a remote object dictionary by a Transmit-PDO.
   * This function honors all access checks of the local object dictionary and
   * executes any registered callback function.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   *
   * @returns the result of the SDO request.
   *
   * @throws #lely::canopen::SdoError on error.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const;

  /**
   * Submits an SDO upload request to a TPDO-mapped sub-object in the local
   * object dictionary, which reads the value that will be written to an
   * RPDO-mapped sub-object in a remote object dictionary by a Transmit-PDO.
   * This function honors all access checks of the local object dictionary and
   * executes any registered callback function.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   *
   * @returns the result of the SDO request, or an empty value on error.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx,
           ::std::error_code& ec) const;

  /**
   * Writes a value to a sub-object in a remote object dictionary by submitting
   * an SDO download request to the corresponding PDO-mapped sub-object in the
   * local object dictionary. This function honors all access checks of the
   * local object dictionary and executes any registered callback function.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param value  the value to be written.
   *
   * @throws #lely::canopen::SdoError on error.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type TpdoWrite(
      uint8_t id, uint16_t idx, uint8_t subidx, T value);

  /**
   * Writes a value to a sub-object in a remote object dictionary by submitting
   * an SDO download request to the corresponding PDO-mapped sub-object in the
   * local object dictionary. This function honors all access checks of the
   * local object dictionary and executes any registered callback function.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param value  the value to be written.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type TpdoWrite(
      uint8_t id, uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec);

 protected:
  ~Device();

  /// Returns a pointer to the internal CANopen device from <lely/co/dev.hpp>.
  CODev* dev() const noexcept;

  /**
   * Returns the type of a sub-object.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   *
   * @returns a reference to an `std::type_info` object representing the type,
   * or `typeid(void)` if unknown.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist.
   */
  const ::std::type_info& Type(uint16_t idx, uint8_t subidx) const;

  /**
   * Returns the type of a sub-object.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     if the sub-object does not exist, the SDO abort code is
   *               stored in <b>ec</b>.
   *
   * @returns a reference to an `std::type_info` object representing the type,
   * or `typeid(void)` if unknown.
   */
  const ::std::type_info& Type(uint16_t idx, uint8_t subidx,
                               ::std::error_code& ec) const;

  /**
   * Reads the value of a sub-object. This function reads the value directly
   * from the object dictionary and bypasses any access checks or registered
   * callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   *
   * @returns a copy of the value of the sub-object.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or the
   * type does not match.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type Get(
      uint16_t idx, uint8_t subidx) const;

  /**
   * Reads the value of a sub-object. This function reads the value directly
   * from the object dictionary and bypasses any access checks or registered
   * callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     if the sub-object does not exist or the type does not match,
   *               the SDO abort code is stored in <b>ec</b>.
   *
   * @returns a copy of the value of the sub-object, or an empty value on error.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type Get(
      uint16_t idx, uint8_t subidx, ::std::error_code& ec) const;

  /**
   * Writes a CANopen basic value to a sub-object. This function writes the
   * value directly to the object dictionary and bypasses any access and range
   * checks or registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or the
   * type does not match.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type Set(
      uint16_t idx, uint8_t subidx, T value);

  /**
   * Writes a CANopen basic value to a sub-object. This function writes the
   * value directly to the object dictionary and bypasses any access and range
   * checks or registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   * @param ec     if the sub-object does not exist or the type does not match,
   *               the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type Set(
      uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec);

  /**
   * Writes a CANopen array value to a sub-object. This function writes the
   * array directly to the object dictionary and bypasses any access checks or
   * registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or the
   * type does not match.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_array<T>::value>::type Set(
      uint16_t idx, uint8_t subidx, const T& value);

  /**
   * Writes a CANopen array value to a sub-object. This function writes the
   * array directly to the object dictionary and bypasses any access checks or
   * registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   * @param ec     if the sub-object does not exist or the type does not match,
   *               the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_array<T>::value>::type Set(
      uint16_t idx, uint8_t subidx, const T& value, ::std::error_code& ec);

  /**
   * Writes a VISIBLE_STRING to a sub-object. This function writes the string
   * directly to the object dictionary and bypasses any access checks or
   * registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) string to be written.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or the
   * type does not match.
   */
  void Set(uint16_t idx, uint8_t subidx, const char* value);

  /**
   * Writes a VISIBLE_STRING to a sub-object. This function writes the string
   * directly to the object dictionary and bypasses any access checks or
   * registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) string to be written.
   * @param ec     if the sub-object does not exist or the type does not match,
   *               the SDO abort code is stored in <b>ec</b>.
   */
  void Set(uint16_t idx, uint8_t subidx, const char* value,
           ::std::error_code& ec);

  /**
   * Writes a UNICODE_STRING to a sub-object. This function writes the string
   * directly to the object dictionary and bypasses any access checks or
   * registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) UCS-2 string to be
   *               written.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or the
   * type does not match.
   */
  void Set(uint16_t idx, uint8_t subidx, const char16_t* value);

  /**
   * Writes a UNICODE_STRING to a sub-object. This function writes the string
   * directly to the object dictionary and bypasses any access checks or
   * registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  a pointer to the (null-terminated) UCS-2 string to be
   *               written.
   * @param ec     if the sub-object does not exist or the type does not match,
   *               the SDO abort code is stored in <b>ec</b>.
   */
  void Set(uint16_t idx, uint8_t subidx, const char16_t* value,
           ::std::error_code& ec);

  /**
   * Writes an OCTET_STRING or DOMAIN value to a sub-object. This function
   * writes the bytes directly to the object dictionary and bypasses any access
   * checks or registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param p      a pointer to the bytes to be written.
   * @param n      the number of bytes to write.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or the
   * type does not match.
   */
  void Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n);

  /**
   * Writes an OCTET_STRING or DOMAIN value to a sub-object. This function
   * writes the bytes directly to the object dictionary and bypasses any access
   * checks or registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param p      a pointer to the bytes to be written.
   * @param n      the number of bytes to write.
   * @param ec     if the sub-object does not exist or the type does not match,
   *               the SDO abort code is stored in <b>ec</b>.
   */
  void Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
           ::std::error_code& ec);

  /**
   * Reads the value of a sub-object in a remote object dictionary by reading
   * the corresponding PDO-mapped sub-object in the local object dictionary.
   * This function reads the value directly from the local object dictionary and
   * bypasses any access checks or registered callback functions.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   *
   * @returns a copy of the value of the PDO-mapped sub-object.
   *
   * @throws #lely::canopen::SdoError if the PDO-mapped sub-object does not
   * exist or the type does not match.
   *
   * @pre a valid mapping from remote TPDO-mapped sub-objects to local
   * RPDO-mapped sub-objects has been generated with UpdateRpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const;

  /**
   * Reads the value of a sub-object in a remote object dictionary by reading
   * the corresponding PDO-mapped sub-object in the local object dictionary.
   * This function reads the value directly from the local object dictionary and
   * bypasses any access checks or registered callback functions.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param ec     if the PDO-mapped sub-object does not exist or the type does
   *               not match, the SDO abort code is stored in <b>ec</b>.
   *
   * @returns a copy of the value of the PDO-mapped sub-object, or an empty
   * value on error.
   *
   * @pre a valid mapping from remote TPDO-mapped sub-objects to local
   * RPDO-mapped sub-objects has been generated with UpdateRpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx,
          ::std::error_code& ec) const;

  /**
   * Reads the value of a TPDO-mapped sub-object in the local object dictionary
   * that will be written to an RPDO-mapped sub-object in a remote object
   * dictionary by a Transmit-PDO. This function reads the value directly from
   * the local object dictionary and bypasses any access checks or registered
   * callback functions.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   *
   * @returns a copy of the value of the PDO-mapped sub-object.
   *
   * @throws #lely::canopen::SdoError if the PDO-mapped sub-object does not
   * exist or the type does not match.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const;

  /**
   * Reads the value of a TPDO-mapped sub-object in the local object dictionary
   * that will be written to an RPDO-mapped sub-object in a remote object
   * dictionary by a Transmit-PDO. This function reads the value directly from
   * the local object dictionary and bypasses any access checks or registered
   * callback functions.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param ec     if the PDO-mapped sub-object does not exist or the type does
   *               not match, the SDO abort code is stored in <b>ec</b>.
   *
   * @returns a copy of the value of the PDO-mapped sub-object, or an empty
   * value on error.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
  TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx,
          ::std::error_code& ec) const;

  /**
   * Writes a value to a sub-object in a remote object dictionary by writing to
   * the corresponding PDO-mapped sub-object in the local object dictionary.
   * This function writes the value directly to the local object dictionary and
   * bypasses any access and range checks or registered callback functions.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param value  the value to be written.
   *
   * @throws #lely::canopen::SdoError if the PDO-mapped sub-object does not
   * exist or the type does not match.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type TpdoSet(
      uint8_t id, uint16_t idx, uint8_t subidx, T value);

  /**
   * Writes a value to a sub-object in a remote object dictionary by writing to
   * the corresponding PDO-mapped sub-object in the local object dictionary.
   * This function writes the value directly to the local object dictionary and
   * bypasses any access and range checks or registered callback functions.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param value  the value to be written.
   * @param ec     if the PDO-mapped sub-object does not exist or the type does
   *               not match, the SDO abort code is stored in <b>ec</b>.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  template <class T>
  typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type TpdoSet(
      uint8_t id, uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec);

  /**
   * Updates the mapping from remote TPDO-mapped sub-objects to local
   * RPDO-mapped sub-objects. The mapping is constructed from the RPDO
   * communication and mapping parameters, and the Lely-specific objects
   * 5800..59FF (Remote TPDO number and node-ID) and 5A00..5BFF (Remote TPDO
   * mapping).
   */
  void UpdateRpdoMapping();

  /**
   * Updates the mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects. The mapping is constructed from the TPDO
   * communication and mapping parameters, and the Lely-specific objects
   * 5C00..5DFF (Remote RPDO number and node-ID) and 5E00..5FFF (Remote RPDO
   * mapping).
   */
  void UpdateTpdoMapping();

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_DEVICE_HPP_
