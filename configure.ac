AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([I/O library], [1.0.1], [], [liblely-io])
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
*linux*)
	platform_linux=yes
	;;
*-*-mingw*)
	platform_win32=yes
	case "$build" in
	*linux*)
		AC_CHECK_PROGS([BINFMT_HANDLER], [wine])
		;;
	esac
	;;
esac
AM_CONDITIONAL([PLATFORM_LINUX], [test "x$platform_linux" == "xyes"])
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" == "xyes"])

AS_IF([test "$host_cpu" != "$build_cpu"], [
	AC_CHECK_PROGS([BINFMT_HANDLER], [qemu-$host_cpu qemu-$host_cpu-static])
])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"
CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Werror"
AS_IF([test "x$platform_win32" == "xyes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"
])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.2 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.2 liblely-util < 2])

AM_CONDITIONAL([NO_CAN], [false])
PKG_CHECK_MODULES([liblely_can], [liblely-can >= 1.2 liblely-can < 2], [], [
	AM_CONDITIONAL([NO_CAN], [true])
	AC_DEFINE([LELY_NO_CAN], [1], [Define to 1 if the CAN library is not available.])
])

AM_CONDITIONAL([NO_CXX], [false])
AC_ARG_ENABLE([cxx],
	AS_HELP_STRING([--disable-cxx], [disable C++ support]))
AS_IF([test "x$enable_cxx" == "xno"], [
	AM_CONDITIONAL([NO_CXX], [true])
	AC_DEFINE([LELY_NO_CXX], [1], [Define to 1 if C++ support is disabled.])
])

AM_CONDITIONAL([NO_THREADS], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "x$enable_threads" == "xno"], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "x$enable_canfd" == "xno"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AS_IF([test "x$platform_win32" == "xyes"], [
	AC_MSG_CHECKING([for WSAGetLastError in ws2_32])
	ax_winsock_ok=no
	save_LIBS=$LIBS
	LIBS="-lws2_32 $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[#include <winsock2.h>]],
			[[WSAGetLastError();]])],
		[ax_winsock_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_winsock_ok])

	AC_MSG_CHECKING([for GetAdaptersAddresses in iphlpapi])
	ax_iphlpapi_ok=no
	save_LIBS=$LIBS
	LIBS="-liphlpapi $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[
				#include <ws2tcpip.h>
				#include <iphlpapi.h>
			]], [[GetAdaptersAddresses(0, 0, NULL, NULL, NULL);]])],
		[ax_iphlpapi_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_iphlpapi_ok])
])

AS_IF([test "x$platform_linux" == "xyes"], [
	AC_CHECK_HEADERS([bluetooth/bluetooth.h bluetooth/rfcomm.h],
		[], [], [[
			#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
			#include <bluetooth/bluetooth.h>
			#endif
		]])
	AC_SEARCH_LIBS([str2ba], [bluetooth])

	AC_CHECK_HEADERS([ifaddrs.h])

	AC_CHECK_HEADERS([linux/can.h])
	AC_CHECK_HEADERS([linux/can/error.h])
	AC_CHECK_HEADERS([linux/can/netlink.h])
	AC_CHECK_HEADERS([linux/can/raw.h])
	AC_EGREP_HEADER([CAN_RAW_FD_FRAMES], [linux/can/raw.h],
		[AC_DEFINE([HAVE_CAN_RAW_FD_FRAMES], [1], [Define to 1 if CAN_RAW_FD_FRAMES is defined.])])
	AC_CHECK_HEADERS([linux/rtnetlink.h])

	AC_CHECK_HEADERS([sys/epoll.h])

	AC_CHECK_HEADERS([sys/ioctl.h])
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
	Makefile
	${PACKAGE}.pc
])
AC_OUTPUT
