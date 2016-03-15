AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([CAN library], [1.0], [], [liblely-can])
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

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "x$enable_canfd" == "xno"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE_UNQUOTED([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
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
	Makefile
	${PACKAGE}.pc
])
AC_OUTPUT
