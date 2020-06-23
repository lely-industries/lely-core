/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
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

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>

#include <CppUTest/TestHarness.h>

#include <lely/util/diag.h>

#include "unit-tests/libc-overrides.hpp"

TEST_GROUP(Util_Diag_Cmdname){};

TEST(Util_Diag_Cmdname, OnlyCmd) {
  const std::string path = "fourtytwo";

  const std::string cmd = cmdname(path.data());

  STRCMP_EQUAL("fourtytwo", cmd.data());
}

TEST(Util_Diag_Cmdname, Delimeters) {
#ifdef _WIN32
  const std::string path = "test\\string\\testing\\fourtytwo";
#else
  const std::string path = "test/string/testing/fourtytwo";
#endif  // _WIN32

  const std::string cmd = cmdname(path.data());

  STRCMP_EQUAL("fourtytwo", cmd.data());
}

TEST_GROUP(Util_Diag_FlocLex) {
  floc location;

  TEST_SETUP() { location = {"something.txt", 0, 0}; }
};

TEST(Util_Diag_FlocLex, FlocLex_FlocNull) {
  const std::string buf = "";

  const auto ret = floc_lex(nullptr, buf.data(), buf.data() + 1UL);

  CHECK_EQUAL(0UL, ret);
}

TEST(Util_Diag_FlocLex, FlocLex_FlocNullBufferWithTab) {
  const std::string buf = "\tsome text\n";

  const auto ret = floc_lex(nullptr, buf.data(), buf.data() + buf.size());

  CHECK_EQUAL(buf.size(), ret);
}

TEST(Util_Diag_FlocLex, FlocLex_FlocNullBufferWithTabEndIsNull) {
  const std::string buf = "\tsome text\n";

  const auto ret = floc_lex(nullptr, buf.data(), nullptr);

  CHECK_EQUAL(buf.size(), ret);
}

TEST(Util_Diag_FlocLex, FlocLex_Nonempty) {
  const std::string buf = "lorem ipsum \r\n";

  const auto ret = floc_lex(&location, buf.data(), buf.data() + buf.size());

  CHECK_EQUAL(buf.size(), ret);
  CHECK_EQUAL(1, location.line);
  CHECK_EQUAL(1, location.column);
}

TEST(Util_Diag_FlocLex, FlocLex_NonemptyManyLines) {
  const std::string buf = "lorem ipsum \r\nlorem ipsum \r\n";

  const auto ret = floc_lex(&location, buf.data(), buf.data() + buf.size());

  CHECK_EQUAL(buf.size(), ret);
  CHECK_EQUAL(2, location.line);
  CHECK_EQUAL(1, location.column);
}

TEST(Util_Diag_FlocLex, FlocLex_NonemptyTab) {
  const std::string buf = "lorem ipsum \r\n\tlorem ipsum";

  const auto ret = floc_lex(&location, buf.data(), buf.data() + buf.size());

  CHECK_EQUAL(buf.size(), ret);
  CHECK_EQUAL(1, location.line);
  CHECK_EQUAL(20, location.column);
}

TEST(Util_Diag_FlocLex, FlocLex_FirstIsREndIsNull) {
  const std::string buf = " \r\n\tlorem ipsum";

  const auto ret = floc_lex(&location, buf.data(), nullptr);

  CHECK_EQUAL(buf.size(), ret);
  CHECK_EQUAL(1, location.line);
  CHECK_EQUAL(20, location.column);
}

TEST(Util_Diag_FlocLex, FlocLex_CpBiggerThanTheEnd) {
  const std::string buf = "\r\r\rlorem ipsum";

  const auto ret = floc_lex(&location, buf.data(), buf.data() + 2UL);

  CHECK_EQUAL(2UL, ret);
  CHECK_EQUAL(2, location.line);
  CHECK_EQUAL(1, location.column);
}

#if !LELY_NO_DIAG

TEST_GROUP(Util_Diag) {
  static const size_t BUF_SIZE = 30;
  char buffer[BUF_SIZE] = {};

  TEST_TEARDOWN() {
#if HAVE_SNPRINTF_OVERRIDE
    valid_calls_snprintf = -1;
#endif  // HAVE_SNPRINTF_OVERRIDE
  }
};

TEST(Util_Diag, SnprintfFloc_NoFilename) {
  const floc at = {"", 0, 0};

  const auto ret = snprintf_floc(buffer, BUF_SIZE, &at);

  CHECK_EQUAL(1, ret);
  STRCMP_EQUAL(":", buffer);
}

TEST(Util_Diag, SnprintfFloc_NonemptyFilename) {
  const floc at = {"nonempty.txt", 3, 0};

  const auto ret = snprintf_floc(buffer, BUF_SIZE, &at);

  CHECK_EQUAL(15, ret);
  STRCMP_EQUAL("nonempty.txt:3:", buffer);
}

#if HAVE_SNPRINTF_OVERRIDE
TEST(Util_Diag, SnprintfFloc_NonemptyFilenameErrorInFilenameEncoding) {
  const floc at = {"nonempty.txt", 3, 14};

  valid_calls_snprintf = 0;
  const auto ret = snprintf_floc(buffer, BUF_SIZE, &at);

  CHECK_EQUAL(-1, ret);
  STRCMP_EQUAL("", buffer);
}

TEST(Util_Diag, SnprintfFloc_NonemptyFilenameErrorInLineEncoding) {
  const floc at = {"nonempty.txt", 3, 14};

  valid_calls_snprintf = 1;
  const auto ret = snprintf_floc(buffer, BUF_SIZE, &at);

  CHECK_EQUAL(-1, ret);
  STRCMP_EQUAL("nonempty.txt:", buffer);
}

TEST(Util_Diag, SnprintfFloc_NonemptyFilenameErrorInColumnEncoding) {
  const floc at = {"nonempty.txt", 3, 14};

  valid_calls_snprintf = 2;
  const auto ret = snprintf_floc(buffer, BUF_SIZE, &at);

  CHECK_EQUAL(-1, ret);
  STRCMP_EQUAL("nonempty.txt:3:", buffer);
}
#endif  // HAVE_SNPRINTF_OVERRIDE

TEST(Util_Diag, SnprintfFloc_FilenameNull) {
  const floc at = {nullptr, 3, 14};

  const auto ret = snprintf_floc(buffer, BUF_SIZE, &at);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL("", buffer);
}

TEST_GROUP(Util_Diag_Handler){
    static void ref_phandler(void*, diag_severity, int, const char*,
                             va_list)  // enforce new line for cpplint
    {}

    static void ref_phandler_at(void*, diag_severity, int, const floc*,
                                const char*,
                                va_list)  // enforce new line for cpplint
    {}};

TEST(Util_Diag_Handler, DiagGetHandler_BothNull) {
  diag_get_handler(nullptr, nullptr);
}

TEST(Util_Diag_Handler, DiagAtGetHandler_BothNull) {
  diag_at_get_handler(nullptr, nullptr);
}

TEST(Util_Diag_Handler, DiagSetHandler_ArgNotNull) {
  diag_handler_t* phandler = nullptr;
  void* phandle = nullptr;
  int argument = 0;

  diag_set_handler(ref_phandler, &argument);

  diag_get_handler(&phandler, &phandle);
  POINTERS_EQUAL(ref_phandler, phandler);
  POINTERS_EQUAL(&argument, phandle);
}

TEST(Util_Diag_Handler, DiagSetHandler_BothNull) {
  diag_handler_t* phandler = nullptr;
  void* phandle = nullptr;

  diag_set_handler(nullptr, nullptr);

  diag_get_handler(&phandler, &phandle);
  POINTERS_EQUAL(nullptr, phandler);
  POINTERS_EQUAL(nullptr, phandle);
}

TEST(Util_Diag_Handler, DiagSetHandler_ArgIsNull) {
  diag_handler_t* phandler = nullptr;
  void* phandle = nullptr;

  diag_set_handler(ref_phandler, nullptr);

  diag_get_handler(&phandler, &phandle);
  POINTERS_EQUAL(ref_phandler, phandler);
  POINTERS_EQUAL(nullptr, phandle);
}

TEST(Util_Diag_Handler, DiagAtSetHandler) {
  diag_at_handler_t* phandler_at = nullptr;
  void* phandle = nullptr;
  int argument = 0;

  diag_at_set_handler(ref_phandler_at, &argument);

  diag_at_get_handler(&phandler_at, &phandle);
  POINTERS_EQUAL(ref_phandler_at, phandler_at);
  POINTERS_EQUAL(&argument, phandle);
}

TEST(Util_Diag_Handler, DiagAtSetHandler_ArgIsNull) {
  diag_at_handler_t* phandler_at = nullptr;
  void* phandle = nullptr;

  diag_at_set_handler(ref_phandler_at, nullptr);

  diag_at_get_handler(&phandler_at, &phandle);
  POINTERS_EQUAL(ref_phandler_at, phandler_at);
  POINTERS_EQUAL(nullptr, phandle);
}

TEST(Util_Diag_Handler, DiagAtSetHandler_BothNull) {
  diag_at_handler_t* phandler_at = nullptr;
  void* phandle = nullptr;

  diag_at_set_handler(nullptr, nullptr);

  diag_at_get_handler(&phandler_at, &phandle);
  POINTERS_EQUAL(nullptr, phandler_at);
  POINTERS_EQUAL(nullptr, phandle);
}

TEST_BASE(Util_Diag_Stderrhandler) {
  const std::string stderr_filename = "stderr.txt";

  std::string GetStderrLine() {
    std::ifstream stderr_stream(stderr_filename.data());
    std::string line;

    if (stderr_stream.good() && std::getline(stderr_stream, line)) {
      remove(stderr_filename.data());
      freopen(stderr_filename.data(), "w", stderr);
      return line;
    }
    return std::string("");
  }

  void setup() { freopen(stderr_filename.data(), "w", stderr); }
  void teardown() { remove(stderr_filename.data()); }
};

TEST_GROUP_BASE(Util_Diag_Diag, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() { Util_Diag_Stderrhandler::teardown(); }
};

TEST(Util_Diag_Diag, Diag_InfoHandlerIsNull) {
  diag_set_handler(nullptr, nullptr);
  diag_severity ds = DIAG_INFO;
  int errc = 0;
  const std::string format = "%s (errc %i)";
  const std::string message = "some Info";

  diag(ds, errc, format.data(), message.data(), errc);

  STRCMP_EQUAL("", GetStderrLine().data());

  diag_set_handler(default_diag_handler, nullptr);
}

TEST(Util_Diag_Diag, Diag_Info) {
  diag_severity ds = DIAG_INFO;
  int errc = 0;
  const std::string format = "%s (errc %i)";
  const std::string message = "some Info";

  diag(ds, errc, format.data(), message.data(), errc);

  STRCMP_EQUAL("some Info (errc 0)", GetStderrLine().data());
}

TEST(Util_Diag_Diag, Diag_DebugErrc1) {
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2no(ERRNUM_ACCES);
  const std::string format = "%s (errc %i)";
  const std::string message = "some debug message";

  diag(ds, errc, format.data(), message.data(), errc);

  const std::string expected = "debug: some debug message (errc";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_Diag, Diag_DebugErrc2) {
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2no(ERRNUM_ADDRINUSE);
  const std::string format = "%s (errc %i)";
  const std::string message = "some debug message";

  diag(ds, errc, format.data(), message.data(), errc);

  const std::string expected = "debug: some debug message (errc";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_Diag, Diag_Warning) {
  diag_severity ds = DIAG_WARNING;
  int errc = errc2num(ERRNUM_ADDRNOTAVAIL);
  const std::string format = "%s (errc %i)";
  const std::string message = "some warning message";

  diag(ds, errc, format.data(), message.data(), errc);

  const std::string expected = "warning: some warning message (errc";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_Diag, Diag_Error) {
  diag_severity ds = DIAG_ERROR;
  int errc = errc2no(ERRNUM_AFNOSUPPORT);
  const std::string format = "%s (errc %i)";
  const std::string message = "some error message";

  diag(ds, errc, format.data(), message.data(), errc);

  const std::string expected = "error: some error message (errc";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_Diag, DiagAt_DebugFlocNotNullAtHandlerNull) {
  diag_at_set_handler(nullptr, nullptr);
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2no(ERRNUM_AGAIN);
  const std::string format = "%s (errc %i)";
  const std::string message = "some debug message";
  const floc at = {"diagAtFilename.txt", 4, 2};

  diag_at(ds, errc, &at, format.data(), message.data(), errc);

  STRCMP_EQUAL("", GetStderrLine().data());

  diag_at_set_handler(default_diag_at_handler, nullptr);
}

TEST(Util_Diag_Diag, DiagAt_DebugFlocNull) {
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2no(ERRNUM_AGAIN);
  const std::string format = "%s (errc %i)";
  const std::string message = "some debug message";
  const floc* at = nullptr;

  diag_at(ds, errc, at, format.data(), message.data(), errc);

  const std::string expected = "debug: some debug message (errc";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_Diag, DiagAt_DebugFlocNotNull) {
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2num(ERRNUM_AGAIN);
  const std::string format = "%s (errc %i)";
  const std::string message = "some debug message";
  const floc at = {"diagAtFilename.txt", 4, 2};

  diag_at(ds, errc, &at, format.data(), message.data(), errc);

  const std::string expected =
      "diagAtFilename.txt:4:2: debug: some debug message (errc";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_Diag, DiagIf_DebugFlocNull) {
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2num(ERRNUM_AGAIN);
  const std::string format = "%s (errc %i)";
  const std::string message = "some debug message";
  const floc* at = nullptr;

  diag_if(ds, errc, at, format.data(), message.data(), errc);

  STRCMP_EQUAL("", GetStderrLine().data());
}

TEST(Util_Diag_Diag, DiagIf_DebugFlocNotNull) {
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2num(ERRNUM_AGAIN);
  const std::string format = "%s (errc %i)";
  const std::string message = "some debug message";
  const floc at = {"diagAtFilename.txt", 4, 2};

  diag_if(ds, errc, &at, format.data(), message.data(), errc);

  const std::string expected =
      "diagAtFilename.txt:4:2: debug: some debug message (errc";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST_GROUP_BASE(Util_Diag_VsnprintfDiagAtWrapper, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];

  diag_severity ds = DIAG_DEBUG;
  int errc = errc2num(ERRNUM_ACCES);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some msg";
  floc location = {"file.txt", 4, 2};

  void vsnprintf_diag_at_wrapper(char* buf, size_t n, diag_severity ds,
                                 int errc, const floc* location,
                                 const char* format, ...) {
    va_list va;
    va_start(va, format);
    vsnprintf_diag_at(buf, n, ds, errc, location, format, va);
    va_end(va);
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() {
    Util_Diag_Stderrhandler::teardown();
    snprintf(buffer, 1UL, "%s", "");
#if HAVE_SNPRINTF_OVERRIDE
    valid_calls_snprintf = -1;
#endif
  }
};

TEST(Util_Diag_VsnprintfDiagAtWrapper, NormalExecution) {
  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  const std::string expected = "file.txt:4:2: debug: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.size());
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, Errc0) {
  errc = 0;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  const std::string expected = "file.txt:4:2: debug: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.length());
}

#if HAVE_SNPRINTF_OVERRIDE
TEST(Util_Diag_VsnprintfDiagAtWrapper, SnprintfFail) {
  valid_calls_snprintf = 0;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  STRCMP_EQUAL("", buffer);
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, SnprintfFailAfter1) {
  valid_calls_snprintf = 1;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  STRCMP_EQUAL("file.txt:", buffer);
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, SnprintfFailAfter2) {
  valid_calls_snprintf = 2;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  STRCMP_EQUAL("file.txt:4:", buffer);
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, SnprintfFailAfter3) {
  valid_calls_snprintf = 3;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  STRCMP_EQUAL("file.txt:4:2:", buffer);
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, SnprintfFailAfter4) {
  valid_calls_snprintf = 4;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  STRCMP_EQUAL("file.txt:4:2: ", buffer);
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, SnprintfFailAfter5) {
  valid_calls_snprintf = 5;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  const std::string expected = "file.txt:4:2: debug: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.size());
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, SnprintfFailAfter6) {
  valid_calls_snprintf = 6;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  const std::string expected = "file.txt:4:2: debug: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.length());
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, FilenameInLocationIsNull) {
  location = {nullptr, 4, 2};

  vsnprintf_diag_at_wrapper(buffer, 0, ds, errc, &location, format.data(),
                            message.data(), errc);

  STRCMP_EQUAL("", buffer);
}
#endif  // HAVE_SNPRINTF_OVERRIDE

TEST(Util_Diag_VsnprintfDiagAtWrapper, DiagFatal) {
  location = {"file.txt", 4, 2};
  ds = DIAG_FATAL;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  const std::string expected = "file.txt:4:2: fatal: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.size());
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, UnknownDS) {
  ds = static_cast<diag_severity>(342345);

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format.data(),
                            message.data(), errc);

  const std::string expected = "file.txt:4:2: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.size());
}

TEST(Util_Diag_VsnprintfDiagAtWrapper, DiagIsFatalFormatIsEmpty) {
  ds = DIAG_FATAL;
  const char format_char = '\0';
  const char* const format_ptr = &format_char;

  vsnprintf_diag_at_wrapper(buffer, BUFSIZ, ds, errc, &location, format_ptr,
                            message.data(), errc);

  const std::string expected = "file.txt:4:2: fatal:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.size());
}

TEST_GROUP_BASE(Util_Diag_DefaultDiagHandler, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];
  diag_severity ds = DIAG_DEBUG;
  void* handle = static_cast<void*>(&ds);
  int errc = errc2num(ERRNUM_ACCES);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some dmesg";
  floc location = {"file.txt", 4, 2};

  void default_diag_handler_wrapper(void* handle, diag_severity ds, int errc,
                                    floc* location, const char* format, ...) {
    va_list va;
    va_start(va, format);
    default_diag_at_handler(handle, ds, errc, location, format, va);
    va_end(va);
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() { Util_Diag_Stderrhandler::teardown(); }
};

TEST(Util_Diag_DefaultDiagHandler, NormalExecution) {
  default_diag_handler_wrapper(handle, ds, errc, &location, format.data(),
                               message.data(), errc);

  const std::string expected = "file.txt:4:2: debug: some dmesg (errc:";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST_GROUP_BASE(Util_Diag_CmdDiagHandler, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];
  void* handle = nullptr;
  diag_severity ds = DIAG_WARNING;
  int errc = errc2num(ERRNUM_BADMSG);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some cmd msg";

  void cmd_diag_handler_wrapper(void* handle, enum diag_severity ds, int errc,
                                const char* format, ...) {
    va_list va;
    va_start(va, format);
    cmd_diag_handler(handle, ds, errc, format, va);
    va_end(va);
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() { Util_Diag_Stderrhandler::teardown(); }
};

TEST(Util_Diag_CmdDiagHandler, NormalExecution) {
  cmd_diag_handler_wrapper(handle, ds, errc, format.data(), message.data(),
                           errc);

  const std::string expected = "warning: some cmd msg (errc:";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_CmdDiagHandler, CmdHandle0) {
  int cmd_handle = 0;
  handle = static_cast<void*>(&cmd_handle);

  cmd_diag_handler_wrapper(handle, ds, errc, format.data(), message.data(),
                           errc);

  const std::string expected = "warning: some cmd msg (errc:";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST(Util_Diag_CmdDiagHandler, CmdNonempty) {
  char cmd[] = "EXAMPLE COMMAND";
  handle = static_cast<void*>(&cmd);

  cmd_diag_handler_wrapper(handle, ds, errc, format.data(), message.data(),
                           errc);

  const std::string expected = "EXAMPLE COMMAND: warning: some cmd msg (errc:";
  STRNCMP_EQUAL(expected.data(), GetStderrLine().data(), expected.size());
}

TEST_GROUP_BASE(Util_Diag_DaemonDiagHandler, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];
  void* handle = nullptr;
  diag_severity ds = DIAG_WARNING;
  int errc = errc2num(ERRNUM_ALREADY);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some msg";

  void daemon_diag_handler_wrapper(void* handle, diag_severity ds, int errc,
                                   const char* format, ...) {
    va_list va;
    va_start(va, format);
    daemon_diag_handler(handle, ds, errc, format, va);
    va_end(va);
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() { Util_Diag_Stderrhandler::teardown(); }
};

TEST(Util_Diag_DaemonDiagHandler, DaemonDiagHandler) {
  daemon_diag_handler_wrapper(handle, ds, errc, format.data(), message.data(),
                              errc);
#if _WIN32
  CHECK_EQUAL(13, errno);
#else
  CHECK_EQUAL(0, errno);
#endif
}

TEST_GROUP_BASE(Util_Diag_DaemonDiagAtHandler, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];
  void* handle = nullptr;
  diag_severity ds = DIAG_WARNING;
  int errc = errc2num(ERRNUM_ALREADY);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some msg";
  floc location = {"file.txt", 4, 3};

  void daemon_diag_at_handler_wrapper(void* handle, diag_severity ds, int errc,
                                      floc* location, const char* format, ...) {
    va_list va;
    va_start(va, format);
    daemon_diag_at_handler(handle, ds, errc, location, format, va);
    va_end(va);
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() { Util_Diag_Stderrhandler::teardown(); }
};

TEST(Util_Diag_DaemonDiagAtHandler, DaemonDiagAtHandler) {
  daemon_diag_at_handler_wrapper(handle, ds, errc, &location, format.data(),
                                 message.data(), errc);
#if _WIN32
  CHECK_EQUAL(13, errno);
#else
  CHECK_EQUAL(0, errno);
#endif
}

TEST_GROUP_BASE(Util_Diag_VsnprintfDiag, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];

  void vsnprintf_diag_wrapper(char* s, size_t n, diag_severity ds, int errc,
                              const char* format, ...) {
    va_list va;
    va_start(va, format);
    vsnprintf_diag(s, n, ds, errc, format, va);
    va_end(va);
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() { Util_Diag_Stderrhandler::teardown(); }
};

TEST(Util_Diag_VsnprintfDiag, VsnprintfDiag) {
  diag_severity ds = DIAG_DEBUG;
  int errc = errc2num(ERRNUM_ADDRINUSE);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some msg";

  vsnprintf_diag_wrapper(buffer, BUFSIZ, ds, errc, format.data(),
                         message.data(), errc);

  const std::string expected = "debug: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buffer, expected.size());
}

TEST_GROUP_BASE(Util_Diag_VasprintfDiag, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];

  void vasprintf_diag_wrapper(char** ps, diag_severity ds, int errc,
                              const char* format, ...) {
    va_list va;
    va_start(va, format);
    vasprintf_diag(ps, ds, errc, format, va);
    va_end(va);
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() { Util_Diag_Stderrhandler::teardown(); }
};

TEST(Util_Diag_VasprintfDiag, VasprintfDiag) {
  diag_severity ds = DIAG_INFO;
  int errc = errc2num(ERRNUM_ADDRINUSE);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some msg";
  char* buf_ptr = nullptr;

  vasprintf_diag_wrapper(&buf_ptr, ds, errc, format.data(), message.data(),
                         errc);
  const std::string expected = "some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buf_ptr, expected.size());
  free(buf_ptr);
}

TEST_GROUP_BASE(Util_Diag_VasprintfDiagAt, Util_Diag_Stderrhandler) {
  char buffer[BUFSIZ];
  diag_severity ds = DIAG_INFO;
  int errc = errc2num(ERRNUM_ADDRINUSE);
  const std::string format = "%s (errc: %i)";
  const std::string message = "some msg";
  floc location = {"file.txt", 7, 10};

  int vasprintf_diag_at_wrapper(char** ps, diag_severity ds, int errc,
                                floc* location, const char* format, ...) {
    va_list va;
    va_start(va, format);
    int chars_written = vasprintf_diag_at(ps, ds, errc, location, format, va);
    va_end(va);
    return chars_written;
  }

  TEST_SETUP() { Util_Diag_Stderrhandler::setup(); }
  TEST_TEARDOWN() {
    Util_Diag_Stderrhandler::teardown();
#if HAVE_SNPRINTF_OVERRIDE
    valid_calls_snprintf = -1;
#endif
  }
};

TEST(Util_Diag_VasprintfDiagAt, VasprintfDiagAt) {
  int chars_written = 0;
  char* buf_ptr = nullptr;

  chars_written = vasprintf_diag_at_wrapper(
      &buf_ptr, ds, errc, &location, format.data(), message.data(), errc);

  const std::string expected = "file.txt:7:10: some msg (errc:";
  STRNCMP_EQUAL(expected.data(), buf_ptr, expected.size());
  if (chars_written >= 0) free(buf_ptr);
}

#if HAVE_SNPRINTF_OVERRIDE
TEST(Util_Diag_VasprintfDiagAt, SnprintfFailAfter1) {
  char* buf_ptr = nullptr;
  valid_calls_snprintf = 1;

  int chars_written = vasprintf_diag_at_wrapper(
      &buf_ptr, ds, errc, &location, format.data(), message.data(), errc);

  POINTERS_EQUAL(nullptr, buf_ptr);
  CHECK_EQUAL(-1, chars_written);
}

TEST(Util_Diag_VasprintfDiagAt, SnprintfFailAfter6) {
  char* buf_ptr = buffer;
  valid_calls_snprintf = 6;

  int chars_written = vasprintf_diag_at_wrapper(
      &buf_ptr, ds, errc, &location, format.data(), message.data(), errc);

  CHECK_EQUAL(-1, chars_written);
}
#endif  // HAVE_SNPRINTF_OVERRIDE

#endif  // !LELY_NO_DIAG
