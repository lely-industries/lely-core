AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([C++ CANopen application library], [0.1.0], [], [lely-coapp])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign subdir-objects])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AC_LANG([C])
AC_PROG_CC_STDC
CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"

AC_LANG([C++])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([11], [ext], [mandatory])
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
	;;
esac
AM_CONDITIONAL([PLATFORM_LINUX], [test "$platform_linux" == "yes"])
AM_CONDITIONAL([PLATFORM_WIN32], [test "$platform_win32" == "yes"])

AS_IF([test "$platform_win32" == "yes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"
	CXXFLAGS="$CXXFLAGS -Wno-error=attributes"
	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.4 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.8 liblely-util < 2])
PKG_CHECK_MODULES([liblely_can], [liblely-can >= 1.4 liblely-can < 2])
PKG_CHECK_MODULES([liblely_co], [liblely-co >= 1.5 liblely-co < 2])

AM_CONDITIONAL([NO_THREADS], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "$enable_threads" == "no"], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

AM_CONDITIONAL([NO_TIMEDWAIT], [false])
AC_ARG_ENABLE([timedwait],
	AS_HELP_STRING([--disable-timedwait], [disable support for timed waiting functions]))
AS_IF([test "$enable_timedwait" == "no"], [
	AM_CONDITIONAL([NO_TIMEDWAIT], [true])
	AC_DEFINE([LELY_NO_TIMEDWAIT], [1], [Define to 1 if support for timed waiting functions is disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "$enable_canfd" == "no"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AM_CONDITIONAL([NO_COAPP_MASTER], [false])
AC_ARG_ENABLE([slave],
	AS_HELP_STRING([--disable-master], [disable CANopen master support]))
AS_IF([test "$enable_master" == "no"], [
	AM_CONDITIONAL([NO_COAPP_MASTER], [true])
	AC_DEFINE([LELY_NO_COAPP_MASTER], [1], [Define to 1 if CANopen master support is disabled.])
])

AM_CONDITIONAL([NO_COAPP_SLAVE], [false])
AC_ARG_ENABLE([slave],
	AS_HELP_STRING([--disable-slave], [disable CANopen slave support]))
AS_IF([test "$enable_slave" == "no"], [
	AM_CONDITIONAL([NO_COAPP_SLAVE], [true])
	AC_DEFINE([LELY_NO_COAPP_SLAVE], [1], [Define to 1 if CANopen slave support is disabled.])
])

AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	include/Makefile
	src/aio/Makefile
	src/aio/version.rc
	src/coapp/Makefile
	src/coapp/version.rc
	src/Makefile
	liblely-aio.pc
	lib${PACKAGE}.pc
	Makefile
])
AC_OUTPUT
