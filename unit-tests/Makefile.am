bin =

LELY_LIBC_LIBS =
if CODE_COVERAGE_ENABLED
LELY_LIBC_LIBS += $(CODE_COVERAGE_LIBS)
endif

LELY_LIBC_LIBS += $(top_builddir)/src/libc/liblely-libc.la

if PLATFORM_WIN32
CPPUTEST_LIBS += -lpthread
endif

# CppUTest library self-test

bin += test-cpputest
test_cpputest_SOURCES = tests-shared-main.cpp test-cpputest.cpp
test_cpputest_LDADD = $(CPPUTEST_LIBS)

# Utilities library tests

LELY_UTIL_LIBS = $(top_builddir)/src/util/liblely-util.la

src_util_tests =
src_util_tests += util/test-bits.cpp
src_util_tests += util/test-cmp.cpp
src_util_tests += util/test-endian.cpp
src_util_tests += util/test-dllist.cpp
src_util_tests += util/test-pheap.cpp
src_util_tests += util/test-rbtree.cpp
src_util_tests += util/test-util.cpp
src_util_tests += util/test-diag.cpp
src_util_tests += util/test-time.cpp
if !ECSS_COMPLIANCE
src_util_tests += util/test-bitset.cpp
src_util_tests += util/test-mutex.cpp
src_util_tests += util/test-sllist.cpp
src_util_tests += util/test-spscring.cpp
endif

bin += unit-test-util
unit_test_util_SOURCES = tests-shared-main.cpp $(src_util_tests)
unit_test_util_LDADD = $(LELY_LIBC_LIBS) $(LELY_UTIL_LIBS) $(CPPUTEST_LIBS)

# CAN library tests

LELY_CAN_LIBS = $(top_builddir)/src/can/liblely-can.la

bin += unit-test-can

src_can_tests =
src_can_tests += can/test-buf.cpp
src_can_tests += can/test-msg.cpp
src_can_tests += can/test-net.cpp

unit_test_can_SOURCES = tests-shared-main.cpp $(src_can_tests)
unit_test_can_LDADD = $(LELY_LIBC_LIBS) $(LELY_UTIL_LIBS) $(LELY_CAN_LIBS) $(CPPUTEST_LIBS)

# CANopen library tests

LELY_CO_LIBS = $(top_builddir)/src/co/liblely-co.la

bin += unit-test-co

src_co_tests =
src_co_tests += co/test-crc.cpp
src_co_tests += co/test-type.cpp

unit_test_co_SOURCES = tests-shared-main.cpp $(src_co_tests)
unit_test_co_LDADD = $(LELY_LIBC_LIBS) $(LELY_UTIL_LIBS) $(LELY_CAN_LIBS) $(LELY_CO_LIBS) $(CPPUTEST_LIBS)

# Compilation flags

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}\" -DLELY_NO_CXX=1
AM_CPPFLAGS += $(CPPUTEST_CFLAGS)

if NO_UNIT_TESTS_MEMLEAK
AM_CPPFLAGS += -DCPPUTEST_MEM_LEAK_DETECTION_DISABLED
endif

if CODE_COVERAGE_ENABLED
AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif

AM_CFLAGS =
if CODE_COVERAGE_ENABLED
AM_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif

AM_CXXFLAGS =
if CODE_COVERAGE_ENABLED
AM_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif

AM_LDFLAGS =
if PLATFORM_WIN32
AM_LDFLAGS += -all-static
endif

check_PROGRAMS = $(bin)

TESTS = $(check_PROGRAMS)
EXEC = $(SHELL) $(top_builddir)/exec-wrapper.sh

if CODE_COVERAGE_ENABLED
LOG_COMPILER = $(SHELL) $(top_builddir)/lcov-wrapper.sh
AM_TESTS_ENVIRONMENT = EXEC_WRAPPER="$(EXEC)"
AM_TESTS_ENVIRONMENT += ;
@CODE_COVERAGE_RULES@
else
LOG_COMPILER = $(EXEC)
endif

@VALGRIND_CHECK_RULES@
