AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([C++ CANopen application library], [0.1.0], [], [lely-coapp])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign subdir-objects])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

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
*-*-mingw*)
	platform_win32=yes
	;;
esac
AM_CONDITIONAL([PLATFORM_WIN32], [test "$platform_win32" == "yes"])

AS_IF([test "$platform_win32" == "yes"], [
	CXXFLAGS="$CXXFLAGS -Wno-error=attributes"
	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.4 liblely-libc < 2])

AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	include/Makefile
	src/coapp/Makefile
	src/coapp/version.rc
	src/Makefile
	lib${PACKAGE}.pc
	Makefile
])
AC_OUTPUT
