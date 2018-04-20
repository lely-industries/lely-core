AC_PREREQ([2.69])

AC_INIT([C++ CANopen application library], [0.1.0], [], [lely-coapp])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign subdir-objects])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	Makefile
])
AC_OUTPUT
