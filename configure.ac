AC_PREREQ([2.69])
LT_PREREQ(2.4.2)

AC_INIT([Lely core libraries], [2.4.0], [], [lely-core])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign subdir-objects])

AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

AX_CODE_COVERAGE

AX_VALGRIND_CHECK

AC_LANG([C])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS

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

platform_linux=no
platform_posix=no
platform_win32=no
case "$host" in
*linux*)
	platform_linux=yes
	platform_posix=yes
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
AM_CONDITIONAL([PLATFORM_POSIX], [test "$platform_posix" == "yes"])
AM_CONDITIONAL([PLATFORM_WIN32], [test "$platform_win32" == "yes"])

AS_IF([test "$host_cpu" != "$build_cpu"], [
	AC_CHECK_PROGS([BINFMT_EXEC], [qemu-$host_cpu qemu-$host_cpu-static])
])

RT_LIBS=
AM_CONDITIONAL([NO_RT], [false])
AC_ARG_ENABLE([rt],
	AS_HELP_STRING([--disable-rt], [disable Realtime Extensions]))
AS_IF([test "$enable_rt" != "no"], [
	AS_IF([test "$platform_win32" == "yes"], [
		AC_CHECK_LIB([pthread], [clock_gettime], [RT_LIBS="-lpthread"])
	], [
		AC_CHECK_LIB([rt], [timer_create], [RT_LIBS="-lrt"])
	])
], [
	AM_CONDITIONAL([NO_RT], [true])
	AC_DEFINE([LELY_NO_RT], [1], [Define to 1 if Realtime Extensions are disabled.])
])
AC_SUBST([RT_LIBS])

AM_CONDITIONAL([NO_THREADS], [false])
AM_CONDITIONAL([HAVE_PTHREAD], [false])
AC_ARG_ENABLE([threads],
	AS_HELP_STRING([--disable-threads], [disable multithreading support]))
AS_IF([test "$enable_threads" != "no"], [
	AX_PTHREAD([
		AM_CONDITIONAL([HAVE_PTHREAD], [true])
		CC="$PTHREAD_CC"
		CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
		LIBS="$PTHREAD_LIBS $LIBS"
	], [
		enable_threads=no
	])
])
AS_IF([test "$enable_threads" == "no"], [
	AM_CONDITIONAL([NO_THREADS], [true])
	AC_DEFINE([LELY_NO_THREADS], [1], [Define to 1 if multithreading support is disabled.])
])

CFLAGS="$CFLAGS -Wall -Wextra -pedantic -Werror"

AM_CONDITIONAL([ECSS_COMPLIANCE], [false])
AC_ARG_ENABLE([ecss-compliance],
	AS_HELP_STRING([--enable-ecss-compliance], [enable ECSS compliant build]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [
	AM_CONDITIONAL([ECSS_COMPLIANCE], [true])
])

AC_ARG_ENABLE([errno],
	AS_HELP_STRING([--disable-errno], [disable errno]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_errno=no])
AS_IF([test "$enable_errno" == "no"], [
	AC_DEFINE([LELY_NO_ERRNO], [1], [Define to 1 if errno is disabled.])
])

AM_CONDITIONAL([NO_MALLOC], [false])
AC_ARG_ENABLE([malloc],
	AS_HELP_STRING([--disable-malloc], [disable dynamic memory allocation]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_malloc=no])
AS_IF([test "$enable_malloc" == "no"], [
	AM_CONDITIONAL([NO_MALLOC], [true])
	AC_DEFINE([LELY_NO_MALLOC], [1], [Define to 1 if dynamic memory allocation is disabled.])
])

AM_CONDITIONAL([NO_STDIO], [false])
AC_ARG_ENABLE([stdio],
	AS_HELP_STRING([--disable-stdio], [disable standard I/O]))
AS_IF([test "$enable_errno" == "no" -o "$enable_malloc" == "no"], [enable_stdio=no])
AS_IF([test "$enable_stdio" == "no"], [
	AM_CONDITIONAL([NO_STDIO], [true])
	AC_DEFINE([LELY_NO_STDIO], [1], [Define to 1 if standard I/O is disabled.])
])

AM_CONDITIONAL([NO_CXX], [false])
AC_ARG_ENABLE([cxx],
	AS_HELP_STRING([--disable-cxx], [disable C++ support]))
AS_IF([test "$enable_errno" == "no" -o "$enable_malloc" == "no"], [enable_cxx=no])
AS_IF([test "$enable_cxx" == "no"], [
	AM_CONDITIONAL([NO_CXX], [true])
	AC_DEFINE([LELY_NO_CXX], [1], [Define to 1 if C++ support is disabled.])
])

AM_CONDITIONAL([NO_DAEMON], [false])
AC_ARG_ENABLE([daemon],
	AS_HELP_STRING([--disable-daemon], [disable daemon support]))
AS_IF([test "$enable_stdio" == "no"], [enable_daemon=no])
AS_IF([test "$enable_daemon" == "no"], [
	AM_CONDITIONAL([NO_DAEMON], [true])
	AC_DEFINE([LELY_NO_DAEMON], [1], [Define to 1 if daemon support is disabled.])
])

AM_CONDITIONAL([NO_DIAG], [false])
AC_ARG_ENABLE([diag],
	AS_HELP_STRING([--disable-diag], [disable diagnostic functions]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_diag=no])
AS_IF([test "$enable_diag" == "no"], [
	AM_CONDITIONAL([NO_DIAG], [true])
	AC_DEFINE([LELY_NO_DIAG], [1], [Define to 1 if diagnostic functions are disabled.])
])

AM_CONDITIONAL([NO_CANFD], [false])
AC_ARG_ENABLE([canfd],
	AS_HELP_STRING([--disable-canfd], [disable CAN FD support]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_canfd=no])
AS_IF([test "$enable_canfd" == "no"], [
	AM_CONDITIONAL([NO_CANFD], [true])
	AC_DEFINE([LELY_NO_CANFD], [1], [Define to 1 if CAN FD support is disabled.])
])

AM_CONDITIONAL([NO_CO_DCF], [false])
AC_ARG_ENABLE([dcf],
	AS_HELP_STRING([--disable-dcf], [disable EDS/DCF support]))
AS_IF([test "$enable_malloc" == "no"], [enable_dcf=no])
AS_IF([test "$enable_stdio" == "no"], [enable_dcf=no])
AS_IF([test "$enable_dcf" == "no"], [
	AM_CONDITIONAL([NO_CO_DCF], [true])
	AC_DEFINE([LELY_NO_CO_DCF], [1], [Define to 1 if EDS/DCF support is disabled.])
])

AM_CONDITIONAL([NO_CO_DCF_RESTORE], [false])
AC_ARG_ENABLE([dcf-restore],
	AS_HELP_STRING([--disable-dcf-restore], [do not (re)store concise DCF of the application parameters]))
AS_IF([test "$enable_dcf_restore" == "no"], [
	AM_CONDITIONAL([NO_CO_DCF_RESTORE], [true])
	AC_DEFINE([LELY_NO_CO_DCF_RESTORE], [1], [Define to 1 if concise DCF of the application parameters is not (re)stored.])
])

AM_CONDITIONAL([NO_CO_OBJ_DEFAULT], [false])
AC_ARG_ENABLE([obj-default],
	AS_HELP_STRING([--disable-obj-default], [disable default values in the object dictionary]))
AS_IF([test "$enable_obj_default" == "no"], [
	AM_CONDITIONAL([NO_CO_OBJ_DEFAULT], [true])
	AC_DEFINE([LELY_NO_CO_OBJ_DEFAULT], [1], [Define to 1 if default values are disabled in the object dictionary.])
])

AM_CONDITIONAL([NO_CO_OBJ_FILE], [false])
AC_ARG_ENABLE([obj-file],
	AS_HELP_STRING([--disable-obj-file], [disable UploadFile/DownloadFile support for the object dictionary]))
AS_IF([test "$enable_stdio" == "no"], [enable_obj_file=no])
AS_IF([test "$enable_obj_file" == "no"], [
	AM_CONDITIONAL([NO_CO_OBJ_FILE], [true])
	AC_DEFINE([LELY_NO_CO_OBJ_FILE], [1], [Define to 1 if UploadFile/DownloadFile support is disabled for the object dictionary.])
])

AM_CONDITIONAL([NO_CO_OBJ_LIMITS], [false])
AC_ARG_ENABLE([obj-limits],
	AS_HELP_STRING([--disable-obj-limits], [disable minimum/maximum values in the object dictionary]))
AS_IF([test "$enable_obj_limits" == "no"], [
	AM_CONDITIONAL([NO_CO_OBJ_LIMITS], [true])
	AC_DEFINE([LELY_NO_CO_OBJ_LIMITS], [1], [Define to 1 if minimum/maximum values are disabled in the object dictionary.])
])

AM_CONDITIONAL([NO_CO_OBJ_NAME], [false])
AC_ARG_ENABLE([obj-name],
	AS_HELP_STRING([--disable-obj-name], [disable names in the object dictionary]))
AS_IF([test "$enable_malloc" == "no"], [enable_obj_name=no])
AS_IF([test "$enable_obj_name" == "no"], [
	AM_CONDITIONAL([NO_CO_OBJ_NAME], [true])
	AC_DEFINE([LELY_NO_CO_OBJ_NAME], [1], [Define to 1 if names are disabled in the object dictionary.])
])

AM_CONDITIONAL([NO_CO_OBJ_UPLOAD], [false])
AC_ARG_ENABLE([obj-upload],
	AS_HELP_STRING([--disable-obj-upload], [disable custom upload indication functions in the object dictionary]))
AS_IF([test "$enable_obj_upload" == "no"], [
	AM_CONDITIONAL([NO_CO_OBJ_UPLOAD], [true])
	AC_DEFINE([LELY_NO_CO_OBJ_UPLOAD], [1], [Define to 1 if custom upload indication functions are disabled in the object dictionary.])
])

AM_CONDITIONAL([NO_CO_SDEV], [false])
AC_ARG_ENABLE([sdev],
	AS_HELP_STRING([--disable-sdev], [disable static device description support]))
AS_IF([test "$enable_malloc" == "no"], [enable_sdev=no])
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

AM_CONDITIONAL([NO_CO_MPDO], [false])
AC_ARG_ENABLE([mpdo],
	AS_HELP_STRING([--disable-mpdo], [disable Multiplex PDO support]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_mpdo=no])
AS_IF([test "$enable_mpdo" == "no"], [
	AM_CONDITIONAL([NO_CO_MPDO], [true])
	AC_DEFINE([LELY_NO_CO_MPDO], [1], [Define to 1 if Multiplex PDO support is disabled.])
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
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_time=no])
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
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_lss=no])
AS_IF([test "$enable_lss" == "no"], [
	AM_CONDITIONAL([NO_CO_LSS], [true])
	AC_DEFINE([LELY_NO_CO_LSS], [1], [Define to 1 if Layer Setting Services (LSS) and protocols support is disabled.])
])

AM_CONDITIONAL([NO_CO_WTM], [false])
AC_ARG_ENABLE([wtm],
	AS_HELP_STRING([--disable-wtm], [disable Wireless Transmission Media (WTM) support]))
AS_IF([test "$enable_malloc" == "no"], [enable_wtm=no])
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

AM_CONDITIONAL([NO_CO_NG], [false])
AC_ARG_ENABLE([ng],
	AS_HELP_STRING([--disable-ng], [disable node guarding support]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_ng=no])
AS_IF([test "$enable_ng" == "no"], [
	AM_CONDITIONAL([NO_CO_NG], [true])
	AC_DEFINE([LELY_NO_CO_NG], [1], [Define to 1 if node guarding support is disabled.])
])

AM_CONDITIONAL([NO_CO_NMT_BOOT], [false])
AC_ARG_ENABLE([nmt-boot],
	AS_HELP_STRING([--disable-nmt-boot], [disable NMT boot slave support]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_nmt_boot=no])
AS_IF([test "$enable_master" == "no"], [enable_nmt_boot=no])
AS_IF([test "$enable_nmt_boot" == "no"], [
	AM_CONDITIONAL([NO_CO_NMT_BOOT], [true])
	AC_DEFINE([LELY_NO_CO_NMT_BOOT], [1], [Define to 1 if NMT boot slave support is disabled.])
])

AM_CONDITIONAL([NO_CO_NMT_CFG], [false])
AC_ARG_ENABLE([nmt-cfg],
	AS_HELP_STRING([--disable-nmt-cfg], [disable NMT configuration request support]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_nmt_cfg=no])
AS_IF([test "$enable_master" == "no"], [enable_nmt_cfg=no])
AS_IF([test "$enable_nmt_cfg" == "no"], [
	AM_CONDITIONAL([NO_CO_NMT_CFG], [true])
	AC_DEFINE([LELY_NO_CO_NMT_CFG], [1], [Define to 1 if NMT configuration request support is disabled.])
])

AM_CONDITIONAL([NO_CO_GW], [false])
AC_ARG_ENABLE([gw],
	AS_HELP_STRING([--disable-gw], [disable gateway support]))
AS_IF([test "$enable_malloc" == "no"], [enable_gw=no])
AS_IF([test "$enable_gw" == "no"], [
	AM_CONDITIONAL([NO_CO_GW], [true])
	AC_DEFINE([LELY_NO_CO_GW], [1], [Define to 1 if gateway support is disabled.])
])

AM_CONDITIONAL([NO_CO_GW_TXT], [false])
AC_ARG_ENABLE([gw-txt],
	AS_HELP_STRING([--disable-gw-txt], [disable ASCII gateway support]))
AS_IF([test "$enable_stdio" == "no"], [enable_gw_txt=no])
AS_IF([test "$enable_gw" == "no"], [enable_gw_txt=no])
AS_IF([test "$enable_gw_txt" == "no"], [
	AM_CONDITIONAL([NO_CO_GW_TXT], [true])
	AC_DEFINE([LELY_NO_CO_GW_TXT], [1], [Define to 1 if ASCII gateway support is disabled.])
])

AM_CONDITIONAL([NO_COAPP_MASTER], [false])
AC_ARG_ENABLE([slave],
	AS_HELP_STRING([--disable-coapp-master], [disable C++ CANopen application master support]))
AS_IF([test "$enable_master" == "no"], [enable_coapp_master=no])
AS_IF([test "$enable_cxx" == "no"], [enable_coapp_master=no])
AS_IF([test "$enable_coapp_master" == "no"], [
	AM_CONDITIONAL([NO_COAPP_MASTER], [true])
	AC_DEFINE([LELY_NO_COAPP_MASTER], [1], [Define to 1 if C++ CANopen application master support is disabled.])
])

AM_CONDITIONAL([NO_COAPP_SLAVE], [false])
AC_ARG_ENABLE([slave],
	AS_HELP_STRING([--disable-coapp-slave], [disable C++ CANopen application slave support]))
AS_IF([test "$enable_cxx" == "no"], [enable_coapp_slave=no])
AS_IF([test "$enable_coapp_slave" == "no"], [
	AM_CONDITIONAL([NO_COAPP_SLAVE], [true])
	AC_DEFINE([LELY_NO_COAPP_SLAVE], [1], [Define to 1 if C++ CANopen application slave support is disabled.])
])

AM_CONDITIONAL([NO_TOOLS], [false])
AC_ARG_ENABLE([tools],
	AS_HELP_STRING([--disable-tools], [disable tools]))
AS_IF([test "$enable_diag" == "no"], [enable_tools=no])
AS_IF([test "$enable_tools" == "no"], [
	AM_CONDITIONAL([NO_TOOLS], [true])
])

AM_CONDITIONAL([NO_TESTS], [false])
AC_ARG_ENABLE([tests],
	AS_HELP_STRING([--disable-tests], [disable tests]))
AS_IF([test "$enable_ecss_compliance" == "yes"], [enable_tests=no])
AS_IF([test "$enable_tests" == "no"], [
	AM_CONDITIONAL([NO_TESTS], [true])
])

m4_ifdef([PKG_CHECK_MODULES], [
	PKG_CHECK_MODULES([CPPUTEST], [cpputest >= 4], [with_cpputest=yes], [with_cpputest=no])
], [
	AC_MSG_WARN(pkg-config not installed - unit tests will not be available!)
	with_cpputest=no
])
AC_SUBST([CPPUTEST_CFLAGS])
AC_SUBST([CPPUTEST_LIBS])

AM_CONDITIONAL([NO_UNIT_TESTS], [false])
AC_ARG_ENABLE([unit-tests],
	AS_HELP_STRING([--disable-unit-tests], [disable unit tests]))
AS_IF([test "$with_cpputest" == "no"], [enable_unit_tests=no])
AS_IF([test "$enable_unit_tests" == "no"], [
	AM_CONDITIONAL([NO_UNIT_TESTS], [true])
])

AM_CONDITIONAL([NO_UNIT_TESTS_MEMLEAK], [false])
AC_ARG_ENABLE([unit-tests-memleak],
	AS_HELP_STRING([--disable-unit-tests-memleak], [disable unit tests memory leak detection]))
AS_IF([test "$enable_unit_tests" == "no"], [enable_unit_tests_memleak=no])
AS_IF([test "$enable_unit_tests_memleak" == "no"], [
	AM_CONDITIONAL([NO_UNIT_TESTS_MEMLEAK], [true])
])

AC_CHECK_HEADER([valgrind/valgrind.h], [
	AC_DEFINE([LELY_HAVE_VALGRIND], [1], [Define to 1 if you have the <valgrind/valgrind.h> header file.])
])

AM_CONDITIONAL([HAVE_IXXAT], [false])
AM_CONDITIONAL([HAVE_VCI], [false])
AS_IF([test "$platform_win32" == "yes"], [
	CFLAGS="$CFLAGS -std=c99 -Wno-error=attributes"

	AC_ARG_WITH([ixxat],
		[AS_HELP_STRING(
			[--with-ixxat@<:@=DIR@:>@],
			[search for the IXXAT VCI SDK (in DIR if specified)]
		)],,
		[with_ixxat=no]
	)
	AS_IF([test "$with_ixxat" != "no"], [
		save_CPPFLAGS="$CPPFLAGS"
		AS_IF([test "$with_ixxat" != "yes"], [
			CPPFLAGS="$CPPFLAGS -I$with_ixxat/inc"
		])
		AM_COND_IF([NO_CANFD], [
			ixxat_header="vcinpl.h"
		], [
			ixxat_header="vcinpl2.h"
		])
		AC_CHECK_HEADERS($ixxat_header, [
			AM_CONDITIONAL([HAVE_IXXAT], [true])
			AC_DEFINE([LELY_HAVE_IXXAT], [1], [Define to 1 if you have the IXXAT VCI SDK.])
		], [
			CPPFLAGS="$save_CPPFLAGS"
		], [
			#include <windows.h>
			#define _MSC_VER 1900
		])
	])

	AC_MSG_CHECKING([for PathRemoveFileSpecA in shlwapi])
	ax_shlwapi_ok=no
	save_LIBS=$LIBS
	LIBS="-lshlwapi $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[
				#include <windows.h>
				#include <shlwapi.h>
			]],
			[[PathRemoveFileSpecA(NULL);]])],
		[ax_shlwapi_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_shlwapi_ok])

	AC_MSG_CHECKING([for WTSSendMessage in wtsapi32])
	ax_wtsapi32_ok=no
	save_LIBS=$LIBS
	LIBS="-lwtsapi32 $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[
				#include <windows.h>
				#include <wtsapi32.h>
			]],
			[[WTSSendMessage(WTS_CURRENT_SERVER_HANDLE, 0, NULL, 0, NULL, 0, 0, 0, NULL, FALSE);]])],
		[ax_wtsapi32_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_wtsapi32_ok])

	AC_ARG_WITH([vci],
		[AS_HELP_STRING(
			[--with-vci@<:@=DIR@:>@],
			[search for the IXXAT VCI SDK (in DIR if specified)]
		)],,
		[with_vci=no]
	)
	AS_IF([test "$with_vci" != "no"], [
		save_CPPFLAGS="$CPPFLAGS"
		AS_IF([test "$with_vci" != "yes"], [
			CPPFLAGS="$CPPFLAGS -I$with_vci/inc"
		])
		AC_CHECK_HEADERS([cantype.h], [
			AM_CONDITIONAL([HAVE_VCI], [true])
			AC_DEFINE([LELY_HAVE_VCI], [1], [Define to 1 if you have the IXXAT VCI SDK.])
		], [
			CPPFLAGS=$save_CPPFLAGS
		], [
			#include <windows.h>
			#define _MSC_VER 1900
		])
	])

	AC_MSG_CHECKING([for WSAGetLastError in ws2_32])
	ax_winsock_ok=no
	save_LIBS=$LIBS
	LIBS="-lws2_32 $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[#include <winsock2.h>]],
			[[WSAGetLastError();]])],
		[ax_winsock_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_winsock_ok])

	AC_MSG_CHECKING([for GetAdaptersAddresses in iphlpapi])
	ax_iphlpapi_ok=no
	save_LIBS=$LIBS
	LIBS="-liphlpapi $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[[
				#include <ws2tcpip.h>
				#include <iphlpapi.h>
			]], [[GetAdaptersAddresses(0, 0, NULL, NULL, NULL);]])],
		[ax_iphlpapi_ok=yes], [LIBS=$save_LIBS])
	AC_MSG_RESULT([$ax_iphlpapi_ok])

	LDFLAGS="$LDFLAGS -Wc,-static-libgcc,-static-libstdc++"
])

AM_CONDITIONAL([HAVE_SOCKET_CAN], [false])
AS_IF([test "$platform_linux" == "yes"], [
	AC_CHECK_HEADERS([linux/can.h], [
		AM_CONDITIONAL([HAVE_SOCKET_CAN], [true])
		AC_DEFINE([LELY_HAVE_SOCKET_CAN], [1], [Define to 1 if you have SocketCAN.])
	])
	AC_CHECK_HEADERS([linux/can/error.h])

	AC_CHECK_HEADERS([bluetooth/bluetooth.h bluetooth/rfcomm.h],
		[], [], [[
			#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
			#include <bluetooth/bluetooth.h>
			#endif
		]])
	AC_SEARCH_LIBS([str2ba], [bluetooth])

	AC_CHECK_HEADERS([ifaddrs.h])

	AC_CHECK_HEADERS([linux/can/netlink.h])
	AC_CHECK_HEADERS([linux/can/raw.h])
	AC_EGREP_HEADER([CAN_RAW_FD_FRAMES], [linux/can/raw.h],
		[AC_DEFINE([HAVE_CAN_RAW_FD_FRAMES], [1], [Define to 1 if CAN_RAW_FD_FRAMES is defined.])])
	AC_CHECK_HEADERS([linux/rtnetlink.h])

	AC_CHECK_HEADERS([sys/epoll.h])

	AC_CHECK_HEADERS([sys/ioctl.h])
])

AC_ARG_ENABLE([python],
	AS_HELP_STRING([--disable-python], [disable Python tools and bindings]))

AM_CONDITIONAL([HAVE_PYTHON2], [false])
AC_ARG_ENABLE([python2],
	AS_HELP_STRING([--disable-python2], [disable Python 2 tools and bindings]))
AS_IF([test "$enable_python" == "no"], [enable_python2=no])
AS_IF([test "$enable_python2" != "no"], [
	AX_CHECK_PYTHON([2], [AM_CONDITIONAL([HAVE_PYTHON2], [true])])
])

AM_CONDITIONAL([HAVE_PYTHON3], [false])
AC_ARG_ENABLE([python3],
	AS_HELP_STRING([--disable-python3], [disable Python 3 tools and bindings]))
AS_IF([test "$enable_python" == "no"], [enable_python3=no])
AS_IF([test "$enable_python3" != "no"], [
	AX_CHECK_PYTHON([3], [AM_CONDITIONAL([HAVE_PYTHON3], [true])])
])

AM_CONDITIONAL([NO_CYTHON], [false])
AC_ARG_ENABLE([cython],
	AS_HELP_STRING([--disable-cython], [do not generate Python bindings with Cython]))
AS_IF([test "$enable_shared" == "no" -o "$platform_win32" == "yes" -o "$enable_malloc" == "no" -o "$enable_stdio" == "no" -o "enable_python" == "no"], [
	enable_cython=no
])
AS_IF([test "$enable_cython" == "no"], [
	AM_CONDITIONAL([NO_CYTHON], [true])
])

AM_CONDITIONAL([HAVE_DOXYGEN], [false])
AC_ARG_ENABLE([doc],
	AS_HELP_STRING([--disable-doc], [disable Doxygen documentation]))
AS_IF([test "$enable_doc" != "no"], [
	AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
	AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
])

AC_PROG_AWK
AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	doc/Doxyfile
	doc/Makefile
	include/Makefile
	pkgconfig/liblely-can.pc
	pkgconfig/liblely-co.pc
	pkgconfig/liblely-coapp.pc
	pkgconfig/liblely-ev.pc
	pkgconfig/liblely-io.pc
	pkgconfig/liblely-io2.pc
	pkgconfig/liblely-libc.pc
	pkgconfig/liblely-libc_rt.pc
	pkgconfig/liblely-tap.pc
	pkgconfig/liblely-util.pc
	pkgconfig/Makefile
	python/can/Makefile
	python/dcf-tools/Makefile
	python/io/Makefile
	python/Makefile
	src/can/Makefile
	src/co/Makefile
	src/co/version.rc
	src/coapp/Makefile
	src/coapp/version.rc
	src/ev/Makefile
	src/ev/version.rc
	src/io/Makefile
	src/io2/Makefile
	src/io2/version.rc
	src/libc/Makefile
	src/libc/version.rc
	src/tap/Makefile
	src/tap/version.rc
	src/util/Makefile
	src/util/version.rc
	src/Makefile
	tools/Makefile
	tools/version.dcf2c.rc
	test/co-nmt-master.dcf
	test/Makefile
	unit-tests/Makefile
	exec-wrapper.sh
	lcov-wrapper.sh
	Makefile
])
AC_OUTPUT
