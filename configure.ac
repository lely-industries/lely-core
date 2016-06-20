AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([CAN library], [1.1.0], [], [liblely-can])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AC_LANG(C)
AC_PROG_CC_STDC

AC_LANG(C++)
AC_PROG_CXX

LT_INIT([win32-dll])
LT_PROG_RC

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
	;;
esac
AM_CONDITIONAL([PLATFORM_LINUX], [test "x$platform_linux" == "xyes"])
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" == "xyes"])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.1 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.1 liblely-util < 2])

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
	AC_DEFINE_UNQUOTED([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "x$enable_canfd" == "xno"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AM_CONDITIONAL([HAVE_SOCKET_CAN], [false])
AS_IF([test "x$platform_linux" == "xyes"], [
	AC_CHECK_HEADERS([linux/can.h], [
		AM_CONDITIONAL([HAVE_SOCKET_CAN], [true])
		AC_DEFINE([LELY_HAVE_SOCKET_CAN], [1], [Define to 1 if you have SocketCAN.])
	])
])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"
CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Werror"
AS_IF([test "x$platform_win32" == "xyes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"
	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

PKG_CHECK_MODULES([liblely_tap], [liblely-tap >= 1.1 liblely-tap < 2], [
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
	src/Makefile
	src/version.rc
	test/Makefile
	Makefile
	${PACKAGE}.pc
])
AC_OUTPUT
