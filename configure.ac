AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([Lely core libraries], [1.9.0], [], [lely-core])
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

platform_linux=no
platform_win32=no
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

RT_LIBS=
AM_CONDITIONAL([NO_RT], [false])
AC_ARG_ENABLE([rt],
	AS_HELP_STRING([--disable-rt], [disable Realtime Extensions]))
AS_IF([test "$enable_rt" != "no"], [
	AS_IF([test "$platform_win32" == "yes"], [
		AC_CHECK_LIB([pthread], [clock_gettime], [RT_LIBS="-lpthread"])
	], [
		AC_CHECK_LIB([rt], [timer_create], [RT_LIBS="-lrt"])
	])
], [
	AM_CONDITIONAL([NO_RT], [true])
	AC_DEFINE([LELY_NO_RT], [1], [Define to 1 if Realtime Extensions are disabled.])
])
AC_SUBST([RT_LIBS])

with_pthread=no
AX_PTHREAD([
	with_pthread=yes
	AC_DEFINE([LELY_HAVE_PTHREAD], [1], [Define to 1 if you have pthread.])
])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"

AM_CONDITIONAL([NO_THREADS], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "$platform_win32" == "no" -a "$with_pthread" == "no"], [
	enable_threads=no
])
AS_IF([test "$enable_threads" == "no"], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
	AS_IF([test "$platform_win32" == "yes"], [LIBS="$RT_LIBS $LIBS"])
], [test "$with_pthread" == "yes"], [
	CC="$PTHREAD_CC"
	CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
	LIBS="$PTHREAD_LIBS $LIBS"
])

AM_CONDITIONAL([NO_CXX], [false])
AC_ARG_ENABLE([cxx],
	AS_HELP_STRING([--disable-cxx], [disable C++ support]))
AS_IF([test "$enable_cxx" == "no"], [
	AM_CONDITIONAL([NO_CXX], [true])
	AC_DEFINE([LELY_NO_CXX], [1], [Define to 1 if C++ support is disabled.])
])

AM_CONDITIONAL([NO_DAEMON], [false])
AC_ARG_ENABLE([daemon],
	AS_HELP_STRING([--disable-daemon], [disable daemon support]))
AS_IF([test "$enable_daemon" == "no"], [
	AM_CONDITIONAL([NO_DAEMON], [true])
	AC_DEFINE([LELY_NO_DAEMON], [1], [Define to 1 if daemon support is disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "$enable_canfd" == "no"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AM_CONDITIONAL([HAVE_VCI], [false])
AS_IF([test "$platform_win32" == "yes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"

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

	AC_ARG_WITH([vci],
		[AS_HELP_STRING(
			[--with-vci@<:@=DIR@:>@],
			[search for the IXXAT VCI SDK (in DIR if specified)]
		)],,
		[with_vci=no]
	)
	AS_IF([test "$with_vci" != "no"], [
		save_CPPFLAGS="$CPPFLAGS"
		AS_IF([test "$with_vci" != "yes"], [
			CPPFLAGS="$CPPFLAGS -I$with_vci/inc"
		])
		AC_CHECK_HEADERS([cantype.h], [
			AM_CONDITIONAL([HAVE_VCI], [true])
			AC_DEFINE([LELY_HAVE_VCI], [1], [Define to 1 if you have the IXXAT VCI SDK.])
		], [
			CPPFLAGS=$save_CPPFLAGS
		], [
			#include <windows.h>
			#define _MSC_VER 1900
		])
	])

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
], [
	AC_SEARCH_LIBS([dlopen], [dl])
])

AM_CONDITIONAL([HAVE_SOCKET_CAN], [false])
AS_IF([test "$platform_linux" == "yes"], [
	AC_CHECK_HEADERS([linux/can.h], [
		AM_CONDITIONAL([HAVE_SOCKET_CAN], [true])
		AC_DEFINE([LELY_HAVE_SOCKET_CAN], [1], [Define to 1 if you have SocketCAN.])
	])
	AC_CHECK_HEADERS([linux/can/error.h])

	AC_CHECK_HEADERS([bluetooth/bluetooth.h bluetooth/rfcomm.h],
		[], [], [[
			#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
			#include <bluetooth/bluetooth.h>
			#endif
		]])
	AC_SEARCH_LIBS([str2ba], [bluetooth])

	AC_CHECK_HEADERS([ifaddrs.h])

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
	AS_HELP_STRING([--disable-python], [do no generate Python bindings]))
AM_CONDITIONAL([WITH_PYTHON], [false])
AS_IF([test "$enable_shared" == "no" -o "$platform_win32" == "yes"], [
	enable_python=no
])
AS_IF([test "$enable_python" != "no"], [
	AX_CHECK_PYTHON([2], [AM_CONDITIONAL([HAVE_PYTHON2], [true])])
	AX_CHECK_PYTHON([3], [AM_CONDITIONAL([HAVE_PYTHON3], [true])])
])

AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_PROG_AWK
AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	include/Makefile
	python/Makefile
	src/can/Makefile
	src/can/version.rc
	src/io/Makefile
	src/io/version.rc
	src/libc/Makefile
	src/libc/version.rc
	src/tap/Makefile
	src/tap/version.rc
	src/util/Makefile
	src/util/version.rc
	src/Makefile
	test/Makefile
	exec-wrapper.sh
	Makefile
	liblely-can.pc
	liblely-io.pc
	liblely-libc.pc
	liblely-libc_rt.pc
	liblely-tap.pc
	liblely-util.pc
])
AC_OUTPUT
