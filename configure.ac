AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([C11 and POSIX compatibility library], [1.2.0], [], [liblely-libc])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AC_LANG(C)
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS

LT_INIT([win32-dll])
LT_PROG_RC

AM_CONDITIONAL([ENABLE_SHARED], [test "x$enable_shared" == "xyes"])

m4_define(version_split, m4_split(AC_PACKAGE_VERSION, [\.]))
AC_SUBST(VERSION_MAJOR, m4_argn(1, version_split))
AC_SUBST(VERSION_MINOR, m4_argn(2, version_split))
AC_SUBST(VERSION_PATCH, m4_argn(3, version_split))

case "$host" in
*-*-mingw*)
	platform_win32=yes
	;;
esac
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" == "xyes"])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"
AS_IF([test "x$platform_win32" == "xyes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"
])

AX_PTHREAD
AS_IF([test "x$ax_pthread_ok" == "xyes"], [
	AC_DEFINE([LELY_HAVE_PTHREAD], [1], [Define to 1 if you have pthread.])
])

AM_CONDITIONAL([NO_THREADS], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "x$ax_pthread_ok" == "xyes" -a "x$enable_threads" != "xno"], [
	CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
	LIBS="$PTHREAD_LIBS $LIBS"
], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

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
	src/version.rc
	Makefile
	${PACKAGE}.pc
])
AC_OUTPUT
