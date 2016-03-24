AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([CANopen library], [1.0], [], [liblely-co])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AC_LANG(C)
AC_PROG_CC_STDC

LT_INIT([win32-dll])
LT_PROG_RC

case "$host" in
*-*-mingw*)
	platform_win32=yes
	;;
esac
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" == "xyes"])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc])
PKG_CHECK_MODULES([liblely_util], [liblely-util])
PKG_CHECK_MODULES([liblely_can], [liblely-can])

AM_CONDITIONAL([NO_CO_DCF], [false])
AC_ARG_ENABLE([dcf],
	AS_HELP_STRING([--disable-dcf], [disable EDS/DCF support]))
AS_IF([test "x$enable_dcf" == "xno"], [
	AM_CONDITIONAL([NO_CO_DCF], [true])
	AC_DEFINE([LELY_NO_CO_DCF], [1], [Define to 1 if EDS/DCF support is disabled.])
])

AM_CONDITIONAL([NO_CO_SDEV], [false])
AC_ARG_ENABLE([sdev],
	AS_HELP_STRING([--disable-sdev], [disable static device description support]))
AS_IF([test "x$enable_sdev" == "xno"], [
	AM_CONDITIONAL([NO_CO_SDEV], [true])
	AC_DEFINE([LELY_NO_CO_SDEV], [1], [Define to 1 if static device description support is disabled.])
])

AM_CONDITIONAL([NO_CO_CSDO], [false])
AC_ARG_ENABLE([csdo],
	AS_HELP_STRING([--disable-csdo], [disable Client-SDO support]))
AS_IF([test "x$enable_csdo" == "xno"], [
	AM_CONDITIONAL([NO_CO_CSDO], [true])
	AC_DEFINE([LELY_NO_CO_CSDO], [1], [Define to 1 if Client-SDO support is disabled.])
])

AM_CONDITIONAL([NO_CO_RPDO], [false])
AC_ARG_ENABLE([rpdo],
	AS_HELP_STRING([--disable-rpdo], [disable Receive-PDO support]))
AS_IF([test "x$enable_rpdo" == "xno"], [
	AM_CONDITIONAL([NO_CO_RPDO], [true])
	AC_DEFINE([LELY_NO_CO_RPDO], [1], [Define to 1 if Receive-PDO support is disabled.])
])

AM_CONDITIONAL([NO_CO_TPDO], [false])
AC_ARG_ENABLE([tpdo],
	AS_HELP_STRING([--disable-tpdo], [disable Transmit-PDO support]))
AS_IF([test "x$enable_tpdo" == "xno"], [
	AM_CONDITIONAL([NO_CO_TPDO], [true])
	AC_DEFINE([LELY_NO_CO_TPDO], [1], [Define to 1 if Transmit-PDO support is disabled.])
])

AM_CONDITIONAL([NO_CO_SYNC], [false])
AC_ARG_ENABLE([sync],
	AS_HELP_STRING([--disable-sync], [disable synchronization (SYNC) object support]))
AS_IF([test "x$enable_sync" == "xno"], [
	AM_CONDITIONAL([NO_CO_SYNC], [true])
	AC_DEFINE([LELY_NO_CO_SYNC], [1], [Define to 1 if synchronization (SYNC) object support is disabled.])
])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"
AS_IF([test "x$platform_win32" == "xyes"], [
	LDFLAGS="$LDFLAGS -Wc,-static-libgcc"
])

AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	include/Makefile
	src/Makefile
	tools/Makefile
	Makefile
	${PACKAGE}.pc
])
AC_OUTPUT
