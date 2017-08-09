AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([CAN library], [1.3.3], [], [lely-can])
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

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.2 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.2 liblely-util < 2])

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

AM_CONDITIONAL([HAVE_VCI], [false])
AS_IF([test "$platform_win32" == "yes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"

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

	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

AM_CONDITIONAL([HAVE_SOCKET_CAN], [false])
AS_IF([test "$platform_linux" == "yes"], [
	AC_CHECK_HEADERS([linux/can.h], [
		AM_CONDITIONAL([HAVE_SOCKET_CAN], [true])
		AC_DEFINE([LELY_HAVE_SOCKET_CAN], [1], [Define to 1 if you have SocketCAN.])
	])
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
