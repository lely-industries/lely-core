AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([Utilities library], [1.4.0], [], [liblely-util])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AX_CODE_COVERAGE
AS_IF([test "x$enable_code_coverage" == "xyes"], [
	${CFLAGS=""}
	${CXXFLAGS=""}
])

AC_LANG(C)
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS

AC_LANG(C++)
AC_PROG_CXX

LT_INIT([win32-dll])
LT_PROG_RC

AM_CONDITIONAL([ENABLE_SHARED], [test "x$enable_shared" == "xyes"])

m4_define(version_split, m4_split(AC_PACKAGE_VERSION, [\.]))
AC_SUBST(VERSION_MAJOR, m4_argn(1, version_split))
AC_SUBST(VERSION_MINOR, m4_argn(2, version_split))
AC_SUBST(VERSION_PATCH, m4_argn(3, version_split))

case "$host" in
*-*-mingw*)
	platform_win32=yes
	case "$build" in
	*linux*)
		AC_CHECK_PROGS([BINFMT_EXEC], [wine])
		AC_CHECK_PROGS([SCRIPT_EXEC], [wine])
		;;
	esac
	;;
esac
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" == "xyes"])

AS_IF([test "$host_cpu" != "$build_cpu"], [
	AC_CHECK_PROGS([BINFMT_EXEC], [qemu-$host_cpu qemu-$host_cpu-static])
])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"
CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Werror"
AS_IF([test "x$platform_win32" == "xyes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"
])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.2 liblely-libc < 2])

AM_CONDITIONAL([NO_CXX], [false])
AC_ARG_ENABLE([cxx],
	AS_HELP_STRING([--disable-cxx], [disable C++ support]))
AS_IF([test "x$enable_cxx" == "xno"], [
	AM_CONDITIONAL([NO_CXX], [true])
	AC_DEFINE([LELY_NO_CXX], [1], [Define to 1 if C++ support is disabled.])
])

AM_CONDITIONAL([NO_DAEMON], [false])
AC_ARG_ENABLE([daemon],
	AS_HELP_STRING([--disable-daemon], [disable daemon support]))
AS_IF([test "x$enable_daemon" == "xno"], [
	AM_CONDITIONAL([NO_DAEMON], [true])
	AC_DEFINE([LELY_NO_DAEMON], [1], [Define to 1 if daemon support is disabled.])
])

AM_CONDITIONAL([NO_THREADS], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "x$enable_threads" == "xno"], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

AS_IF([test "x$platform_win32" == "xyes"], [
	AC_MSG_CHECKING([for PathRemoveFileSpecA in shlwapi])
	ax_shlwapi_ok=no
	save_LIBS=$LIBS
	LIBS="-lshlwapi $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[
				#include <windows.h>
				#include <shlwapi.h>
			]],
			[[PathRemoveFileSpecA(NULL);]])],
		[ax_shlwapi_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_shlwapi_ok])

	AC_MSG_CHECKING([for WTSSendMessage in wtsapi32])
	ax_wtsapi32_ok=no
	save_LIBS=$LIBS
	LIBS="-lwtsapi32 $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[
				#include <windows.h>
				#include <wtsapi32.h>
			]],
			[[WTSSendMessage(WTS_CURRENT_SERVER_HANDLE, 0, NULL, 0, NULL, 0, 0, 0, NULL, FALSE);]])],
		[ax_wtsapi32_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_wtsapi32_ok])
])

AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

PKG_CHECK_MODULES([liblely_tap], [liblely-tap >= 1.2 liblely-tap < 2], [
	AM_CONDITIONAL([HAVE_TAP], [true])
	AC_PROG_AWK
	AC_REQUIRE_AUX_FILE([tap-driver.sh])
], [
	AM_CONDITIONAL([HAVE_TAP], [false])
])

AS_IF([test "x$platform_win32" == "xyes"], [
	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	include/Makefile
	src/Makefile
	src/version.rc
	test/Makefile
	exec-wrapper.sh
	Makefile
	${PACKAGE}.pc
])
AC_OUTPUT
