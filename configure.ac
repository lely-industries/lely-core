AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([I/O library], [1.2.2], [], [lely-io])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AX_CODE_COVERAGE
AS_IF([test "$enable_code_coverage" == "yes"], [
	${CFLAGS=""}
	${CXXFLAGS=""}
])

AC_LANG([C])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"

AC_LANG([C++])
AC_PROG_CXX
CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Werror"

LT_INIT([win32-dll])
LT_PROG_RC

AM_CONDITIONAL([ENABLE_SHARED], [test "$enable_shared" == "yes"])

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
		AC_CHECK_PROGS([BINFMT_EXEC], [wine])
		AC_CHECK_PROGS([SCRIPT_EXEC], [wine])
		;;
	esac
	;;
esac
AM_CONDITIONAL([PLATFORM_LINUX], [test "$platform_linux" == "yes"])
AM_CONDITIONAL([PLATFORM_WIN32], [test "$platform_win32" == "yes"])

AS_IF([test "$host_cpu" != "$build_cpu"], [
	AC_CHECK_PROGS([BINFMT_EXEC], [qemu-$host_cpu qemu-$host_cpu-static])
])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.4 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.2 liblely-util < 2])

AM_CONDITIONAL([NO_CAN], [false])
PKG_CHECK_MODULES([liblely_can], [liblely-can >= 1.3 liblely-can < 2], [], [
	AM_CONDITIONAL([NO_CAN], [true])
	AC_DEFINE([LELY_NO_CAN], [1], [Define to 1 if the CAN library is not available.])
])

AM_CONDITIONAL([NO_CXX], [false])
AC_ARG_ENABLE([cxx],
	AS_HELP_STRING([--disable-cxx], [disable C++ support]))
AS_IF([test "$enable_cxx" == "no"], [
	AM_CONDITIONAL([NO_CXX], [true])
	AC_DEFINE([LELY_NO_CXX], [1], [Define to 1 if C++ support is disabled.])
])

AM_CONDITIONAL([NO_THREADS], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "$enable_threads" == "no"], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "$enable_canfd" == "no"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AS_IF([test "$platform_win32" == "yes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"

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

	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

AS_IF([test "$platform_linux" == "yes"], [
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

AM_CONDITIONAL([HAVE_PYTHON2], [false])
AM_CONDITIONAL([HAVE_PYTHON3], [false])
AC_ARG_ENABLE([python],
	AS_HELP_STRING([--disable-python], [disable Python bindings]))
AS_IF([test "x$enable_shared" == "xno" -o "x$platform_win32" == "xyes"],
	[enable_python=no])
AS_IF([test "x$enable_python" != "xno"], [
	AX_PYTHON([2], [AM_CONDITIONAL([HAVE_PYTHON2], [true])])
	AX_PYTHON([3], [AM_CONDITIONAL([HAVE_PYTHON3], [true])])
])

AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

PKG_CHECK_MODULES([liblely_tap], [liblely-tap >= 1.2 liblely-tap < 2], [
	AM_CONDITIONAL([HAVE_TAP], [true])
	AC_PROG_AWK
	AC_REQUIRE_AUX_FILE([tap-driver.sh])
], [
	AM_CONDITIONAL([HAVE_TAP], [false])
])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	include/Makefile
	python/Makefile
	src/Makefile
	src/version.rc
	test/Makefile
	exec-wrapper.sh
	Makefile
	lib${PACKAGE}.pc
])
AC_OUTPUT
