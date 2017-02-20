AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([CAN library], [1.3.0], [], [liblely-can])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AX_CODE_COVERAGE
AS_IF([test "x$enable_code_coverage" == "xyes"], [
	${CFLAGS=""}
	${CXXFLAGS=""}
])

AC_LANG(C)
AC_PROG_CC_STDC

AC_LANG(C++)
AC_PROG_CXX

LT_INIT([win32-dll])
LT_PROG_RC

AM_CONDITIONAL([ENABLE_SHARED], [test "x$enable_shared" == "xyes"])

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
AM_CONDITIONAL([PLATFORM_LINUX], [test "x$platform_linux" == "xyes"])
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" == "xyes"])

AS_IF([test "$host_cpu" != "$build_cpu"], [
	AC_CHECK_PROGS([BINFMT_EXEC], [qemu-$host_cpu qemu-$host_cpu-static])
])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"
CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Werror"
AS_IF([test "x$platform_win32" == "xyes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"
])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.2 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.2 liblely-util < 2])

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
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "x$enable_canfd" == "xno"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AM_CONDITIONAL([HAVE_IXXAT_VCI3], [false])
AS_IF([test "x$platform_win32" == "xyes"], [
	AC_CHECK_HEADERS([vci3.h], [
		AM_CONDITIONAL([HAVE_IXXAT_VCI3], [true])
		AC_DEFINE([LELY_HAVE_IXXAT_VCI3], [1], [Define to 1 if you have IXXAT VCI V3.])
	])
])

AM_CONDITIONAL([HAVE_SOCKET_CAN], [false])
AS_IF([test "x$platform_linux" == "xyes"], [
	AC_CHECK_HEADERS([linux/can.h], [
		AM_CONDITIONAL([HAVE_SOCKET_CAN], [true])
		AC_DEFINE([LELY_HAVE_SOCKET_CAN], [1], [Define to 1 if you have SocketCAN.])
	])
])

# AM_PROG_PYTHON(MAJOR-VERSION[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
AC_DEFUN([AM_PROG_PYTHON], [
	AC_CHECK_PROGS([PYTHON$1], [python$1 python])
	AS_IF([test -n "${PYTHON$1}" && test -n "$1"], [
		AC_MSG_CHECKING([whether ${PYTHON$1} is version $1])
		am_python_version=`${PYTHON$1} -V 2>&1 | grep -o 'Python @<:@0-9@:>@' | grep -o '@<:@0-9@:>@'`
		AS_IF([test $am_python_version -eq $1], [
			AC_MSG_RESULT([yes])
			m4_default([$2], [:])
		], [
			AC_MSG_RESULT([no])
			PYTHON$1=
			m4_default([$3], [:])
		])
	])
])

AM_CONDITIONAL([HAVE_PYTHON2], [false])
AM_CONDITIONAL([HAVE_PYTHON3], [false])
AC_ARG_ENABLE([python],
	AS_HELP_STRING([--disable-python], [disable Python bindings]))
AS_IF([test "x$enable_shared" == "xno" -o "x$platform_win32" == "xyes"],
	[enable_python=no])
AS_IF([test "x$enable_python" != "xno"], [
	AM_PROG_PYTHON([2], [AM_CONDITIONAL([HAVE_PYTHON2], [true])])
	AM_PROG_PYTHON([3], [AM_CONDITIONAL([HAVE_PYTHON3], [true])])
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

AS_IF([test "x$platform_win32" == "xyes"], [
	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
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
	${PACKAGE}.pc
])
AC_OUTPUT
