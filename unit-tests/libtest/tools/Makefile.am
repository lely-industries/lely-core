LELY_CAN_LIBS = $(top_builddir)/lib/can/liblely-can.la
LELY_CO_LIBS = $(top_builddir)/lib/co/liblely-co.la
LELY_UTIL_LIBS = $(top_builddir)/lib/util/liblely-util.la

# lely-unit-test library

lib_LTLIBRARIES =

liblely_unit_test_src =
liblely_unit_test_src += lely-unit-test.cpp
liblely_unit_test_src += can-send.cpp
liblely_unit_test_src += co-csdo-dn-con.cpp
liblely_unit_test_src += co-csdo-up-con.cpp
liblely_unit_test_src += co-nmt-hb-ind.cpp
liblely_unit_test_src += co-nmt-st-ind.cpp
liblely_unit_test_src += co-nmt-rdn-ind.cpp
liblely_unit_test_src += co-sub-dn-ind.cpp
liblely_unit_test_src += co-sub-up-ind.cpp
liblely_unit_test_src += sdo-init-expected-data.cpp
liblely_unit_test_src += sdo-create-message.cpp

lib_LTLIBRARIES += liblely-unit-test.la

liblely_unit_test_la_CPPFLAGS = -I$(top_srcdir)/include $(CPPUTEST_CFLAGS)
# even in no-hosted unit-tests are compiled with libc
if NO_HOSTED
liblely_unit_test_la_CPPFLAGS += -DLELY_HAVE_TIMESPEC -DLELY_HAVE_ITIMERSPEC
endif
liblely_unit_test_la_SOURCES = $(liblely_unit_test_src)
liblely_unit_test_la_LIBADD = $(LELY_UTIL_LIBS) $(LELY_CAN_LIBS) $(LELY_CO_LIBS)

# Compilation flags

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}\"
AM_CPPFLAGS += $(CPPUTEST_CFLAGS)

if CODE_COVERAGE_ENABLED
AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif

AM_CFLAGS =
if CODE_COVERAGE_ENABLED
AM_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif

AM_CXXFLAGS =
AM_CXXFLAGS += -fno-exceptions
if CODE_COVERAGE_ENABLED
AM_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif

AM_LDFLAGS =
if PLATFORM_WIN32
AM_LDFLAGS += -all-static
endif

if CODE_COVERAGE_ENABLED
LOG_COMPILER = $(SHELL) $(top_builddir)/lcov-wrapper.sh
AM_TESTS_ENVIRONMENT = EXEC_WRAPPER="$(EXEC)"
AM_TESTS_ENVIRONMENT += ;
@CODE_COVERAGE_RULES@
else
LOG_COMPILER = $(EXEC)
endif

@VALGRIND_CHECK_RULES@
