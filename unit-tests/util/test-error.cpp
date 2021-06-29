/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
 *
 * Unit Test Suite was developed under a programme of,
 * and funded by, the European Space Agency.
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <CppUTest/TestHarness.h>

#include <lely/util/error.h>

TEST_GROUP(Util_Error) {
  TEST_SETUP() {
    set_errc_get_handler(&default_set_errc_handler, &default_set_errc_data);
    get_errc_get_handler(&default_get_errc_handler, &default_get_errc_data);

    set_errc(0);
  }

  TEST_TEARDOWN() {
    set_errc_set_handler(default_set_errc_handler, default_set_errc_data);
    get_errc_set_handler(default_get_errc_handler, default_get_errc_data);

    set_errc(0);
  }

  static void CustomSetErrcHandler(int errc, void* data) {
    *static_cast<int*>(data) = errc;
  }

  static int CustomGetErrcHandler(void* data) {
    return *static_cast<int*>(data);
  }

  set_errc_handler_t* default_set_errc_handler;
  get_errc_handler_t* default_get_errc_handler;
  void* default_set_errc_data;
  void* default_get_errc_data;

  int set_errc_value;
  int get_errc_value;
};

/// @name set_errc_get_handler()
///@{

/// \Given N/A
///
/// \When set_errc_get_handler() is called with two proper pointers
///
/// \Then the default (non NULL) handler is returned
TEST(Util_Error, SetErrcGetHandler_DefaultHandler) {
  set_errc_handler_t* set_errc_handler = nullptr;
  void* set_errc_data = nullptr;

  set_errc_get_handler(&set_errc_handler, &set_errc_data);

  CHECK(set_errc_handler != nullptr);
  CHECK(set_errc_data == nullptr);
}

/// \Given N/A
///
/// \When set_errc_get_handler() is called with pointer to handler and NULL
///
/// \Then the default (non NULL) handler is returned
TEST(Util_Error, SetErrcGetHandler_NullDataPtr) {
  set_errc_handler_t* set_errc_handler = nullptr;

  set_errc_get_handler(&set_errc_handler, nullptr);

  CHECK(set_errc_handler != nullptr);
}

/// \Given N/A
///
/// \When set_errc_get_handler() is called with pointer to data and NULL
///
/// \Then the default (NULL) data is returned
TEST(Util_Error, SetErrcGetHandler_NullHandlerPtr) {
  void* set_errc_data = &set_errc_value;

  set_errc_get_handler(nullptr, &set_errc_data);

  POINTERS_EQUAL(nullptr, set_errc_data);
}

///@}

/// @name set_errc_set_handler()
///@{

/// \Given a custom set_errc() handler
///
/// \When set_errc_set_handler() is called with this handler
///
/// \Then the handler is changed
TEST(Util_Error, SetErrcSetHandler_CustomHandler) {
  set_errc_set_handler(&CustomSetErrcHandler, &set_errc_value);

  set_errc_handler_t* set_errc_handler = nullptr;
  void* set_errc_data = nullptr;
  set_errc_get_handler(&set_errc_handler, &set_errc_data);

  POINTERS_EQUAL(&CustomSetErrcHandler, set_errc_handler);
  POINTERS_EQUAL(&set_errc_value, set_errc_data);
}

///@}

/// @name get_errc_get_handler()
///@{

/// \Given N/A
///
/// \When get_errc_get_handler() is called with two proper pointers
///
/// \Then the default (non NULL) handler is returned
TEST(Util_Error, GetErrcGetHandler_DefaultHandler) {
  get_errc_handler_t* get_errc_handler = nullptr;
  void* get_errc_data = nullptr;

  get_errc_get_handler(&get_errc_handler, &get_errc_data);

  CHECK(get_errc_handler != nullptr);
  CHECK(get_errc_data == nullptr);
}

/// \Given N/A
///
/// \When get_errc_get_handler() is called with a pointer to handler and NULL
///
/// \Then the default (non NULL) handler is returned
TEST(Util_Error, GetErrcGetHandler_NullDataPtr) {
  get_errc_handler_t* get_errc_handler = nullptr;

  get_errc_get_handler(&get_errc_handler, nullptr);

  CHECK(get_errc_handler != nullptr);
}

/// \Given N/A
///
/// \When get_errc_get_handler() is called with a pointer to data and NULL
///
/// \Then the default (NULL) data is returned
TEST(Util_Error, GetErrcGetHandler_NullHandlerPtr) {
  void* get_errc_data = &get_errc_value;

  get_errc_get_handler(nullptr, &get_errc_data);

  POINTERS_EQUAL(nullptr, get_errc_data);
}

///@}

/// @name get_errc_set_handler()
///@{

/// \Given a custom get_errc() handler
///
/// \When get_errc_set_handler() is called with this handler
///
/// \Then the handler is changed
TEST(Util_Error, GetErrcSetHandler_CustomHandler) {
  get_errc_set_handler(&CustomGetErrcHandler, &get_errc_value);

  get_errc_handler_t* get_errc_handler = nullptr;
  void* get_errc_data = nullptr;
  get_errc_get_handler(&get_errc_handler, &get_errc_data);

  POINTERS_EQUAL(&CustomGetErrcHandler, get_errc_handler);
  POINTERS_EQUAL(&get_errc_value, get_errc_data);
}

///@}

/// @name get_errc()
///@{

/// \Given N/A
///
/// \When get_errc() is called
///
/// \Then zero is returned
TEST(Util_Error, GetErrc_DefaultValue) {
  const auto errc = get_errc();

  CHECK_EQUAL(0, errc);
}

/// \Given a custom get_errc() handler
///
/// \When get_errc() is called
///
/// \Then value provided by the handler is returned
TEST(Util_Error, GetErrc_HandlerValue) {
  get_errc_set_handler(&CustomGetErrcHandler, &get_errc_value);
  const int expectedErrc = 1410;
  get_errc_value = expectedErrc;

  const auto errc = get_errc();

  CHECK_EQUAL(expectedErrc, errc);
}

/// \Given NULL get_errc() handler
///
/// \When get_errc() is called
///
/// \Then zero is returned
TEST(Util_Error, GetErrc_NullHandler) {
  get_errc_set_handler(NULL, NULL);

  set_errc(42);
  const auto errc = get_errc();

  CHECK_EQUAL(0, errc);
}

///@}

/// @name set_errc()
///@{

/// \Given N/A
///
/// \When set_errc() is called with some error code
///
/// \Then the passed error code is set
TEST(Util_Error, SetErrc_DefaultHandler) {
  const int expectedErrc = 1410;

  set_errc(expectedErrc);

  CHECK_EQUAL(expectedErrc, get_errc());
}

/// \Given a custom set_errc() handler
///
/// \When set_errc() is called with some error code
///
/// \Then the code is passed to the handler
TEST(Util_Error, SetErrc_Handler) {
  set_errc_set_handler(&CustomSetErrcHandler, &set_errc_value);
  const int expectedErrc = 1410;
  set_errc_value = 0;

  set_errc(expectedErrc);

  CHECK_EQUAL(expectedErrc, set_errc_value);
}

/// \Given NULL set_errc() handler
///
/// \When set_errc() is called with some error code
///
/// \Then the error code is ignored
TEST(Util_Error, SetErrc_NullHandler) {
  set_errc_set_handler(NULL, NULL);

  set_errc(42);

  CHECK_EQUAL(0, get_errc());
}

///@}

/// @name get_errnum()
///@{

/// \Given N/A
///
/// \When get_errnum() is called
///
/// \Then ERRNUM_SUCCESS is returned
TEST(Util_Error, GetErrnum_DefaultValue) {
  const auto errc = get_errnum();

  CHECK_EQUAL(ERRNUM_SUCCESS, errc);
}

/// \Given a custom get_errc() handler
///
/// \When get_errnum() is called
///
/// \Then the value from the handler is returned
TEST(Util_Error, GetErrnum_HandlerValue) {
  get_errc_set_handler(&CustomGetErrcHandler, &get_errc_value);
  get_errc_value = errnum2c(ERRNUM_INVAL);

  const auto errnum = get_errnum();

  CHECK_EQUAL(ERRNUM_INVAL, errnum);
}

/// \Given NULL get_errc() handler
///
/// \When get_errnum() is called
///
/// \Then ERRNUM_SUCCESS is returned
TEST(Util_Error, GetErrnum_NullHandler) {
  get_errc_set_handler(NULL, NULL);

  set_errnum(ERRNUM_INVAL);
  const auto errnum = get_errnum();

  CHECK_EQUAL(ERRNUM_SUCCESS, errnum);
}

///@}

/// @name set_errnum()
///@{

/// \Given N/A
///
/// \When set_errnum() is called with some error number
///
/// \Then passed error number is set
TEST(Util_Error, SetErrnum_DefaultHandler) {
  set_errnum(ERRNUM_INVAL);

  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a custom set_errc() handler
///
/// \When set_errnum() is called with some error number
///
/// \Then the number is passed to the handler
TEST(Util_Error, SetErrnum_Handler) {
  set_errc_set_handler(&CustomSetErrcHandler, &set_errc_value);
  set_errc_value = 0;

  set_errnum(ERRNUM_INVAL);

  CHECK_EQUAL(errnum2c(ERRNUM_INVAL), set_errc_value);
}

/// \Given NULL set_errc() handler
///
/// \When set_errnum() is called with some error number
///
/// \Then the error number is ignored
TEST(Util_Error, SetErrnum_NullHandler) {
  set_errc_set_handler(NULL, NULL);

  set_errnum(ERRNUM_INVAL);

  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());
}

///@}

#if LELY_NO_ERRNO

/// @name errnum2c()
///@{

/// \Given the library compiled with LELY_NO_ERRNO switch
///
/// \When errnum2c() is called with any value
///
/// \Then the same value is returned
TEST(Util_Error, Errnum2c) { CHECK_EQUAL(1789, errnum2c(1789)); }

///@}

/// @name errc2num()
///@{

/// \Given the library compiled with LELY_NO_ERRNO switch
///
/// \When errc2num() is called with any value
///
/// \Then the same value is returned
TEST(Util_Error, Errc2num) {
  CHECK_EQUAL(ERRNUM_INVAL, errc2num(ERRNUM_INVAL));
}

/// @}

/// @name set_errc_from_errno()
///@{

/// \Given the library compiled with LELY_NO_ERRNO switch
///
/// \When set_errc_from_errno() is called
///
/// \Then the error code is set to 0
TEST(Util_Error, SetErrcFromErrno) {
  set_errc(1410);

  set_errc_from_errno();

  CHECK_EQUAL(0, get_errc());
}

///@}

/// @name get_errc_from_errno()
///@{

/// \Given the library compiled with LELY_NO_ERRNO switch
///
/// \When get_errc_from_errno() is called
///
/// \Then zero is returned
TEST(Util_Error, GetErrcFromErrno) {
  const auto result = get_errc_from_errno();

  CHECK_EQUAL(0, result);
}

///@}

#endif  // LELY_NO_ERRNO
