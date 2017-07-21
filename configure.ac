AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([CANopen library], [1.4.0], [], [lely-co])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AX_CODE_COVERAGE
AS_IF([test "$enable_code_coverage" == "yes"], [
	${CFLAGS=""}
	${CXXFLAGS=""}
])

AC_LANG([C])
AC_PROG_CC_STDC
CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"

AC_LANG([C++])
AC_PROG_CXX
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
	case "$build" in
	*linux*)
		AC_CHECK_PROGS([BINFMT_EXEC], [wine])
		AC_CHECK_PROGS([SCRIPT_EXEC], [wine])
		;;
	esac
	;;
esac
AM_CONDITIONAL([PLATFORM_LINUX], [test "$platform_linux" == "yes"])
AM_CONDITIONAL([PLATFORM_WIN32], [test "$platform_win32" == "yes"])

AS_IF([test "$host_cpu" != "$build_cpu"], [
	AC_CHECK_PROGS([BINFMT_EXEC], [qemu-$host_cpu qemu-$host_cpu-static])
])

PKG_CHECK_MODULES([liblely_libc], [liblely-libc >= 1.3 liblely-libc < 2])
PKG_CHECK_MODULES([liblely_util], [liblely-util >= 1.5 liblely-util < 2])
PKG_CHECK_MODULES([liblely_can], [liblely-can >= 1.2 liblely-can < 2])

AM_CONDITIONAL([NO_IO], [false])
PKG_CHECK_MODULES([liblely_io], [liblely-io >= 1.2 liblely-io < 2], [], [
	AM_CONDITIONAL([NO_IO], [true])
	AC_DEFINE([LELY_NO_IO], [1], [Define to 1 if the I/O library is not available.])
])

AM_CONDITIONAL([NO_CXX], [false])
AC_ARG_ENABLE([cxx],
	AS_HELP_STRING([--disable-cxx], [disable C++ support]))
AS_IF([test "$enable_cxx" == "no"], [
	AM_CONDITIONAL([NO_CXX], [true])
	AC_DEFINE([LELY_NO_CXX], [1], [Define to 1 if C++ support is disabled.])
])

AM_CONDITIONAL([NO_THREADS], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "$enable_threads" == "no"], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "$enable_canfd" == "no"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AM_CONDITIONAL([NO_CO_DCF], [false])
AC_ARG_ENABLE([dcf],
	AS_HELP_STRING([--disable-dcf], [disable EDS/DCF support]))
AS_IF([test "$enable_dcf" == "no"], [
	AM_CONDITIONAL([NO_CO_DCF], [true])
	AC_DEFINE([LELY_NO_CO_DCF], [1], [Define to 1 if EDS/DCF support is disabled.])
])

AM_CONDITIONAL([NO_CO_OBJ_FILE], [false])
AC_ARG_ENABLE([obj-file],
	AS_HELP_STRING([--disable-obj-file], [disable UploadFile/DownloadFile support for the object dictionary]))
AS_IF([test "$enable_obj_file" == "no"], [
	AM_CONDITIONAL([NO_CO_OBJ_FILE], [true])
	AC_DEFINE([LELY_NO_CO_OBJ_FILE], [1], [Define to 1 if UploadFile/DownloadFile support is disabled for the object dictionary.])
])

AM_CONDITIONAL([NO_CO_SDEV], [false])
AC_ARG_ENABLE([sdev],
	AS_HELP_STRING([--disable-sdev], [disable static device description support]))
AS_IF([test "$enable_sdev" == "no"], [
	AM_CONDITIONAL([NO_CO_SDEV], [true])
	AC_DEFINE([LELY_NO_CO_SDEV], [1], [Define to 1 if static device description support is disabled.])
])

AM_CONDITIONAL([NO_CO_CSDO], [false])
AC_ARG_ENABLE([csdo],
	AS_HELP_STRING([--disable-csdo], [disable Client-SDO support]))
AS_IF([test "$enable_csdo" == "no"], [
	AM_CONDITIONAL([NO_CO_CSDO], [true])
	AC_DEFINE([LELY_NO_CO_CSDO], [1], [Define to 1 if Client-SDO support is disabled.])
])

AM_CONDITIONAL([NO_CO_RPDO], [false])
AC_ARG_ENABLE([rpdo],
	AS_HELP_STRING([--disable-rpdo], [disable Receive-PDO support]))
AS_IF([test "$enable_rpdo" == "no"], [
	AM_CONDITIONAL([NO_CO_RPDO], [true])
	AC_DEFINE([LELY_NO_CO_RPDO], [1], [Define to 1 if Receive-PDO support is disabled.])
])

AM_CONDITIONAL([NO_CO_TPDO], [false])
AC_ARG_ENABLE([tpdo],
	AS_HELP_STRING([--disable-tpdo], [disable Transmit-PDO support]))
AS_IF([test "$enable_tpdo" == "no"], [
	AM_CONDITIONAL([NO_CO_TPDO], [true])
	AC_DEFINE([LELY_NO_CO_TPDO], [1], [Define to 1 if Transmit-PDO support is disabled.])
])

AM_CONDITIONAL([NO_CO_SYNC], [false])
AC_ARG_ENABLE([sync],
	AS_HELP_STRING([--disable-sync], [disable synchronization (SYNC) object support]))
AS_IF([test "$enable_sync" == "no"], [
	AM_CONDITIONAL([NO_CO_SYNC], [true])
	AC_DEFINE([LELY_NO_CO_SYNC], [1], [Define to 1 if synchronization (SYNC) object support is disabled.])
])

AM_CONDITIONAL([NO_CO_TIME], [false])
AC_ARG_ENABLE([time],
	AS_HELP_STRING([--disable-time], [disable time stamp (TIME) object support]))
AS_IF([test "$enable_time" == "no"], [
	AM_CONDITIONAL([NO_CO_TIME], [true])
	AC_DEFINE([LELY_NO_CO_TIME], [1], [Define to 1 if time stamp (TIME) object support is disabled.])
])

AM_CONDITIONAL([NO_CO_EMCY], [false])
AC_ARG_ENABLE([emcy],
	AS_HELP_STRING([--disable-emcy], [disable emergency (EMCY) object support]))
AS_IF([test "$enable_emcy" == "no"], [
	AM_CONDITIONAL([NO_CO_EMCY], [true])
	AC_DEFINE([LELY_NO_CO_EMCY], [1], [Define to 1 if emergency (EMCY) object support is disabled.])
])

AM_CONDITIONAL([NO_CO_LSS], [false])
AC_ARG_ENABLE([lss],
	AS_HELP_STRING([--disable-lss], [disable Layer Setting Services (LSS) and protocols support]))
AS_IF([test "$enable_lss" == "no"], [
	AM_CONDITIONAL([NO_CO_LSS], [true])
	AC_DEFINE([LELY_NO_CO_LSS], [1], [Define to 1 if Layer Setting Services (LSS) and protocols support is disabled.])
])

AM_CONDITIONAL([NO_CO_WTM], [false])
AC_ARG_ENABLE([lss],
	AS_HELP_STRING([--disable-wtm], [disable Wireless Transmission Media (WTM) support]))
AS_IF([test "$enable_wtm" == "no"], [
	AM_CONDITIONAL([NO_CO_WTM], [true])
	AC_DEFINE([LELY_NO_CO_WTM], [1], [Define to 1 if Wireless Transmission Media (WTM) support is disabled.])
])

AM_CONDITIONAL([NO_CO_MASTER], [false])
AC_ARG_ENABLE([master],
	AS_HELP_STRING([--disable-master], [disable master support]))
AS_IF([test "$enable_csdo" == "no"], [enable_master=no])
AS_IF([test "$enable_master" == "no"], [
	AM_CONDITIONAL([NO_CO_MASTER], [true])
	AC_DEFINE([LELY_NO_CO_MASTER], [1], [Define to 1 if master support is disabled.])
])

AM_CONDITIONAL([NO_CO_GW], [false])
AC_ARG_ENABLE([gw],
	AS_HELP_STRING([--disable-gw], [disable gateway support]))
AS_IF([test "$enable_gw" == "no"], [
	AM_CONDITIONAL([NO_CO_GW], [true])
	AC_DEFINE([LELY_NO_CO_GW], [1], [Define to 1 if gateway support is disabled.])
])

AM_CONDITIONAL([NO_CO_GW_TXT], [false])
AC_ARG_ENABLE([gw-txt],
	AS_HELP_STRING([--disable-gw-txt], [disable ASCII gateway support]))
AS_IF([test "$enable_gw" == "no"], [enable_gw_txt=no])
AS_IF([test "$enable_gw_txt" == "no"], [
	AM_CONDITIONAL([NO_CO_GW_TXT], [true])
	AC_DEFINE([LELY_NO_CO_GW_TXT], [1], [Define to 1 if ASCII gateway support is disabled.])
])

AS_IF([test "$platform_win32" == "yes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"
	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

PKG_CHECK_MODULES([liblely_tap], [liblely-tap >= 1.3 liblely-tap < 2], [
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
	tools/Makefile
	tools/version.dcf2c.rc
	test/Makefile
	test/nmt-master.dcf
	exec-wrapper.sh
	Makefile
	lib${PACKAGE}.pc
])
AC_OUTPUT
