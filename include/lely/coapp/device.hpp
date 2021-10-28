/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen device description declarations.
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

#ifndef LELY_COAPP_DEVICE_HPP_
#define LELY_COAPP_DEVICE_HPP_

#include <lely/coapp/sdo_error.hpp>
#include <lely/coapp/type_traits.hpp>
#include <lely/util/mutex.hpp>

#include <functional>
#include <memory>
#include <string>
#include <typeinfo>

// The CANopen device from <lely/co/dev.h>.
struct __co_dev;

// The static sCANopen device from <lely/co/sdev.h>.
struct co_sdev;

namespace lely {

namespace canopen {

/**
 * The CANopen device description. This class manages the object dictionary and
 * device setttings such as the network-ID and node-ID.
 */
class Device {
 public:
  /**
   * Creates a new CANopen device description from an existing internal device
   * description.
   *
   * @param dev   a pointer to an internal device desciption. Ownership of
   *              <b>dev</b> is transfered to the new class instance and the
   *              internal device description will be destroyed when the class
   *              instance is destroyed.
   * @param id    the node-ID (in the range [1..127, 255]). If <b>id</b> is 255
   *              (unconfigured), the node-ID is obtained from the device
   *              description.
   * @param mutex an (optional) pointer to the mutex to be locked while the
   *              internal device description is accessed. The mutex MUST be
   *              unlocked when any member function is invoked.
   */
  Device(__co_dev* dev, uint8_t id = 0xff,
         util::BasicLockable* mutex = nullptr);

  /**
   * Creates a new CANopen device description from file.
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

  /**
   * Creates a new CANopen device description from a static device description.
   *
   * @param sdev  a pointer to a static device desciption.
   * @param id    the node-ID (in the range [1..127, 255]). If <b>id</b> is 255
   *              (unconfigured), the node-ID is obtained from the device
   *              description.
   * @param mutex an (optional) pointer to the mutex to be locked while the
   *              internal device description is accessed. The mutex MUST be
   *              unlocked when any member function is invoked.
   */
  Device(const co_sdev* sdev, uint8_t id = 0xff,
         util::BasicLockable* mutex = nullptr);

  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

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
  typename ::std::enable_if<is_canopen<T>::value, T>::type Read(
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
  typename ::std::enable_if<is_canopen<T>::value, T>::type Read(
      uint16_t idx, uint8_t subidx, ::std::error_code& ec) const;

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a CANopen value to a sub-object while honoring all access
   * and range checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type Write(uint16_t idx,
                                                              uint8_t subidx,
                                                              const T& value);

  /**
   * Submits an SDO download request to the local object dictionary. This
   * function writes a CANopen value to a sub-object while honoring all access
   * and range checks and executing any registered callback function.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   * @param ec     on error, the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type Write(
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
   * Submits a series of SDO download requests to the local object dictionary.
   * This functions writes each entry in the specified concise DCF to a
   * sub-object while honoring all access checks and executing any registered
   * callback function.
   *
   * @param begin a pointer the the first byte in a concise DCF (see object 1F22
   *              in CiA 302-3 version 4.1.0).
   * @param end   a pointer to one past the last byte in the concise DCF. At
   *              most `end - begin` bytes are read.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  void WriteDcf(const uint8_t* begin, const uint8_t* end);

  /**
   * Submits a series of SDO download requests to the local object dictionary.
   * This functions writes each entry in the specified concise DCF to a
   * sub-object while honoring all access checks and executing any registered
   * callback function.
   *
   * @param begin a pointer the the first byte in a concise DCF (see object 1F22
   *              in CiA 302-3 version 4.1.0).
   * @param end   a pointer to one past the last byte in the concise DCF. At
   *              most `end - begin` bytes are read.
   * @param ec    on error, the SDO abort code is stored in <b>ec</b>.
   */
  void WriteDcf(const uint8_t* begin, const uint8_t* end,
                ::std::error_code& ec);

  /**
   * Submits a series of SDO download requests to the local object dictionary.
   * This functions writes each entry in the specified concise DCF to a
   * sub-object while honoring all access checks and executing any registered
   * callback function.
   *
   * @param path the path of the concise DCF.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  void WriteDcf(const char* path);

  /**
   * Submits a series of SDO download requests to the local object dictionary.
   * This functions writes each entry in the specified concise DCF to a
   * sub-object while honoring all access checks and executing any registered
   * callback function.
   *
   * @param path the path of the concise DCF.
   * @param ec   on error, the SDO abort code is stored in <b>ec</b>.
   */
  void WriteDcf(const char* path, ::std::error_code& ec);

  /**
   * Checks if the specified sub-object in the local object dictionary can be
   * mapped into a PDO and, if so, triggers the transmission of every
   * event-driven, asynchronous Transmit-PDO into which the sub-object is
   * mapped.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist.
   */
  void WriteEvent(uint16_t idx, uint8_t subidx);

  /**
   * Checks if the specified sub-object in the local object dictionary can be
   * mapped into a PDO and, if so, triggers the transmission of every
   * event-driven, asynchronous Transmit-PDO into which the sub-object is
   * mapped.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     if the sub-object does not exist, the SDO abort code is
   *               stored in <b>ec</b>.
   */
  void WriteEvent(uint16_t idx, uint8_t subidx, ::std::error_code& ec) noexcept;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type RpdoRead(
      uint8_t id, uint16_t idx, uint8_t subidx) const;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type RpdoRead(
      uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type TpdoRead(
      uint8_t id, uint16_t idx, uint8_t subidx) const;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type TpdoRead(
      uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code& ec) const;

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
  typename ::std::enable_if<is_canopen_basic<T>::value>::type TpdoWrite(
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
  typename ::std::enable_if<is_canopen_basic<T>::value>::type TpdoWrite(
      uint8_t id, uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec);

  /**
   * Triggers the transmission of every event-driven, asynchronous Transmit-PDO
   * which is mapped into the specified sub-object in a remote object
   * dictionary.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  void TpdoWriteEvent(uint8_t id, uint16_t idx, uint8_t subidx);

  /**
   * Triggers the transmission of every event-driven, asynchronous Transmit-PDO
   * which is mapped into the specified sub-object in a remote object
   * dictionary.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param ec     if the sub-object does not exist, the SDO abort code is
   *               stored in <b>ec</b>.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  void TpdoWriteEvent(uint8_t id, uint16_t idx, uint8_t subidx,
                      ::std::error_code& ec) noexcept;

  /*
   * Registers the function to be invoked when a value is successfully written
   * to the local object dictionary by an SDO download (or Receive-PDO) request.
   * Only a single function can be registered at any one time. If
   * <b>on_write</b> contains a callable function target, a copy of the target
   * is invoked _after_ OnWrite(uint16_t, uint8_t) completes.
   */
  void OnWrite(::std::function<void(uint16_t, uint8_t)> on_write);

  /*
   * Registers the function to be invoked when a value is successfully written
   * to an RPDO-mapped object in the local object dictionary by a Receive-PDO
   * (or SDO download). Only a single function can be registered at any one
   * time. If <b>on_tpdo_write</b> contains a callable function target, a copy
   * of the target is invoked _after_ OnRpdoWrite(uint8_t, uint16_t, uint8_t)
   * completes.
   */
  void OnRpdoWrite(
      ::std::function<void(uint8_t, uint16_t, uint8_t)> on_rpdo_write);

 protected:
  ~Device();

  /// Returns a pointer to the internal CANopen device from <lely/co/dev.hpp>.
  __co_dev* dev() const noexcept;

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
                               ::std::error_code& ec) const noexcept;

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
  typename ::std::enable_if<is_canopen<T>::value, T>::type Get(
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
  typename ::std::enable_if<is_canopen<T>::value, T>::type Get(
      uint16_t idx, uint8_t subidx, ::std::error_code& ec) const noexcept;

  /**
   * Writes a CANopen value to a sub-object. This function writes the value
   * directly to the object dictionary and bypasses any access checks or
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
  typename ::std::enable_if<is_canopen<T>::value>::type Set(uint16_t idx,
                                                            uint8_t subidx,
                                                            const T& value);

  /**
   * Writes a CANopen value to a sub-object. This function writes the value
   * directly to the object dictionary and bypasses any access checks or
   * registered callback functions.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param value  the value to be written.
   * @param ec     if the sub-object does not exist or the type does not match,
   *               the SDO abort code is stored in <b>ec</b>.
   */
  template <class T>
  typename ::std::enable_if<is_canopen<T>::value>::type Set(
      uint16_t idx, uint8_t subidx, const T& value,
      ::std::error_code& ec) noexcept;

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
           ::std::error_code& ec) noexcept;

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
           ::std::error_code& ec) noexcept;

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
           ::std::error_code& ec) noexcept;

  /**
   * Returns the value of the UploadFile attribute of a sub-object, if present.
   * The returned value is valid until the next call to SetUploadFile() for the
   * same sub-object.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   *
   * @returns a pointer to the UploadFile string, or `nullptr` if the
   * UploadFile attribute does not exist.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist.
   */
  const char* GetUploadFile(uint16_t idx, uint8_t subidx) const;

  /**
   * Returns the value of the UploadFile attribute of a sub-object, if present.
   * The returned value is valid until the next call to SetUploadFile() for the
   * same sub-object.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     if the sub-object does not exist, the SDO abort code is
   *               stored in <b>ec</b>.
   *
   * @returns a pointer to the UploadFile string, or `nullptr` on error or if
   * the UploadFile attribute does not exist.
   */
  const char* GetUploadFile(uint16_t idx, uint8_t subidx,
                            ::std::error_code& ec) const noexcept;

  /**
   * Sets the value of the UploadFile attribute of a sub-object, if present.
   * This operation invalidates any value returned by a previous call to
   * GetUploadFile() for the same sub-object.
   *
   * @param idx      the object index.
   * @param subidx   the object sub-index.
   * @param filename a pointer to the string to be copied to the UploadFile
   *                 attribute.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or does
   * not have the UploadFile attribute.
   */
  void SetUploadFile(uint16_t idx, uint8_t subidx, const char* filename);

  /**
   * Sets the value of the UploadFile attribute of a sub-object, if present.
   * This operation invalidates any value returned by a previous call to
   * GetUploadFile() for the same sub-object.
   *
   * @param idx      the object index.
   * @param subidx   the object sub-index.
   * @param filename a pointer to the string to be copied to the UploadFile
   *                 attribute.
   * @param ec       if the sub-object does not exist or does not have the
   *                 UploadFile attribute, the SDO abort code is stored in
   *                 <b>ec</b>.
   */
  void SetUploadFile(uint16_t idx, uint8_t subidx, const char* filename,
                     ::std::error_code& ec) noexcept;

  /**
   * Returns the value of the DownloadFile attribute of a sub-object, if
   * present. The returned value is valid until the next call to
   * SetDownloadFile() for the same sub-object.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   *
   * @returns a pointer to the DownloadFile string, or `nullptr` if the
   * DownloadFile attribute does not exist.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist.
   */
  const char* GetDownloadFile(uint16_t idx, uint8_t subidx) const;

  /**
   * Returns the value of the DownloadFile attribute of a sub-object, if
   * present. The returned value is valid until the next call to
   * SetDownloadFile() for the same sub-object.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     if the sub-object does not exist, the SDO abort code is
   *               stored in <b>ec</b>.
   *
   * @returns a pointer to the DownloadFile string, or `nullptr` on error or if
   * the DownloadFile attribute does not exist.
   */
  const char* GetDownloadFile(uint16_t idx, uint8_t subidx,
                              ::std::error_code& ec) const noexcept;

  /**
   * Sets the value of the DownloadFile attribute of a sub-object, if present.
   * This operation invalidates any value returned by a previous call to
   * GetDownloadFile() for the same sub-object.
   *
   * @param idx      the object index.
   * @param subidx   the object sub-index.
   * @param filename a pointer to the string to be copied to the DownloadFile
   *                 attribute.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist or does
   * not have the DownloadFile attribute.
   */
  void SetDownloadFile(uint16_t idx, uint8_t subidx, const char* filename);

  /**
   * Sets the value of the DownloadFile attribute of a sub-object, if present.
   * This operation invalidates any value returned by a previous call to
   * GetDownloadFile() for the same sub-object.
   *
   * @param idx      the object index.
   * @param subidx   the object sub-index.
   * @param filename a pointer to the string to be copied to the DownloadFile
   *                 attribute.
   * @param ec       if the sub-object does not exist or does not have the
   *                 DownloadFile attribute, the SDO abort code is stored in
   *                 <b>ec</b>.
   */
  void SetDownloadFile(uint16_t idx, uint8_t subidx, const char* filename,
                       ::std::error_code& ec) noexcept;
  /**
   * Checks if the specified sub-object in the local object dictionary can be
   * mapped into a PDO and, if so, triggers the transmission of every
   * event-driven, asynchronous Transmit-PDO into which the sub-object is
   * mapped.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   *
   * @throws #lely::canopen::SdoError if the sub-object does not exist.
   */
  void SetEvent(uint16_t idx, uint8_t subidx);

  /**
   * Checks if the specified sub-object in the local object dictionary can be
   * mapped into a PDO and, if so, triggers the transmission of every
   * event-driven, asynchronous Transmit-PDO into which the sub-object is
   * mapped.
   *
   * @param idx    the object index.
   * @param subidx the object sub-index.
   * @param ec     if the sub-object does not exist, the SDO abort code is
   *               stored in <b>ec</b>.
   *
   * @throws #lely::canopen::SdoError on error.
   */
  void SetEvent(uint16_t idx, uint8_t subidx, ::std::error_code& ec) noexcept;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type RpdoGet(
      uint8_t id, uint16_t idx, uint8_t subidx) const;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type RpdoGet(
      uint8_t id, uint16_t idx, uint8_t subidx,
      ::std::error_code& ec) const noexcept;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type TpdoGet(
      uint8_t id, uint16_t idx, uint8_t subidx) const;

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
  typename ::std::enable_if<is_canopen_basic<T>::value, T>::type TpdoGet(
      uint8_t id, uint16_t idx, uint8_t subidx,
      ::std::error_code& ec) const noexcept;

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
  typename ::std::enable_if<is_canopen_basic<T>::value>::type TpdoSet(
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
  typename ::std::enable_if<is_canopen_basic<T>::value>::type TpdoSet(
      uint8_t id, uint16_t idx, uint8_t subidx, T value,
      ::std::error_code& ec) noexcept;

  /**
   * Triggers the transmission of every event-driven, asynchronous Transmit-PDO
   * which is mapped into the specified sub-object in a remote object
   * dictionary.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   *
   * @throws #lely::canopen::SdoError if the PDO-mapped sub-object does not
   * exist.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  void TpdoSetEvent(uint8_t id, uint16_t idx, uint8_t subidx);

  /**
   * Triggers the transmission of every event-driven, asynchronous Transmit-PDO
   * which is mapped into the specified sub-object in a remote object
   * dictionary.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param ec     if the PDO-mapped sub-object does not exist, the SDO abort
   *               code is stored in <b>ec</b>.
   *
   * @pre a valid mapping from remote RPDO-mapped sub-objects to local
   * TPDO-mapped sub-objects has been generated with UpdateTpdoMapping().
   */
  void TpdoSetEvent(uint8_t id, uint16_t idx, uint8_t subidx,
                    ::std::error_code& ec) noexcept;

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

  /*
   * The function invoked when a value is successfully written to the local
   * object dictionary by an SDO download (or Receive-PDO) request.
   *
   * @param idx    the object index (in the range [0x2000..0xBFFF]).
   * @param subidx the object sub-index.
   */
  virtual void
  OnWrite(uint16_t /*idx*/, uint8_t /*subidx*/) noexcept {}

  /*
   * The function invoked when a value is successfully written to an RPDO-mapped
   * object in the local object dictionary by a Receive-PDO (or SDO download)
   * request. In the case of a PDO, this function is invoked for each sub-object
   * in the order of the RPDO mapping.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   *
   * @pre a valid mapping from remote TPDO-mapped sub-objects to local
   * RPDO-mapped sub-objects has been generated with UpdateRpdoMapping().
   */
  virtual void
  OnRpdoWrite(uint8_t /*id*/, uint16_t /*idx*/, uint8_t /*subidx*/) noexcept {}

  /**
   * Invokes OnRpdoWrite() _as if_ a value was written to an RPDO-mapped object
   * in the local object dictionary.
   *
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   */
  void RpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx);

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_DEVICE_HPP_
