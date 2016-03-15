AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([Utilities library], [1.0], [], [liblely-util])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AC_LANG(C)
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS

LT_INIT

PKG_CHECK_MODULES([liblely_libc], [liblely-libc])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"

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
