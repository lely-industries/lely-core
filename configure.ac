AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([I/O library], [1.0.0], [], [liblely-io])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AC_LANG(C)
AC_PROG_CC_STDC

LT_INIT

m4_define(version_split, m4_split(AC_PACKAGE_VERSION, [\.]))
AC_SUBST(VERSION_MAJOR, m4_argn(1, version_split))
AC_SUBST(VERSION_MINOR, m4_argn(2, version_split))
AC_SUBST(VERSION_PATCH, m4_argn(3, version_split))

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.1 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.1 liblely-util < 2])

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
