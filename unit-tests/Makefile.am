bin =

LELY_LIBC_LIBS =
if CODE_COVERAGE_ENABLED
LELY_LIBC_LIBS += $(CODE_COVERAGE_LIBS)
endif

LELY_LIBC_LIBS += $(top_builddir)/src/libc/liblely-libc.la

if PLATFORM_WIN32
CPPUTEST_LIBS += -lpthread
endif

src_common =
src_common += override/call-wrapper.hpp
src_common += override/defs.hpp
src_common += override/lely-defs.hpp
src_common += override/libc-defs.hpp
src_common += override/libc-stdio.hpp
src_common += override/override-test-plugin.hpp
src_common += override/override-test-plugin.cpp
src_common += lely-cpputest-ext.hpp
src_common += lely-unit-test.hpp
src_common += tests-shared-main.cpp

# CppUTest library self-test

bin += test-cpputest
test_cpputest_SOURCES = $(src_common) test-cpputest.cpp
test_cpputest_LDADD = $(CPPUTEST_LIBS)

# Override library (lely-core)

LELY_OVERRIDE_CO_VAL_LIB =

if !PLATFORM_WIN32
if ENABLE_SHARED
liblelyco_val_override_src =
liblelyco_val_override_src += override/lelyco-val.hpp
liblelyco_val_override_src += override/lelyco-val.cpp

lib_LTLIBRARIES = liblelyco_val_override.la

liblelyco_val_override_la_LDFLAGS =
liblelyco_val_override_la_LDFLAGS += -Wl,--as-needed
liblelyco_val_override_la_LIBADD =
liblelyco_val_override_la_LIBADD += -ldl

liblelyco_val_override_la_CPPFLAGS = -I$(top_srcdir)/include -DLELY_ENABLE_SHARED=1
liblelyco_val_override_la_SOURCES = $(liblelyco_val_override_src)

LELY_OVERRIDE_CO_VAL_LIB += liblelyco_val_override.la
endif
endif

# Utilities library tests

LELY_UTIL_LIBS = $(top_builddir)/src/util/liblely-util.la

src_util_tests =
src_util_tests += util/test-bits.cpp
src_util_tests += util/test-cmp.cpp
src_util_tests += util/test-dllist.cpp
src_util_tests += util/test-endian.cpp
src_util_tests += util/test-membuf.cpp
src_util_tests += util/test-pheap.cpp
src_util_tests += util/test-rbtree.cpp
src_util_tests += util/test-time.cpp
src_util_tests += util/test-ustring.cpp
src_util_tests += util/test-util.cpp
if !ECSS_COMPLIANCE
if !NO_MALLOC
src_util_tests += util/test-bitset.cpp
endif
if !NO_CXX
src_util_tests += util/test-mutex.cpp
endif
src_util_tests += util/test-sllist.cpp
src_util_tests += util/test-spscring.cpp
endif

if !NO_DIAG
src_util_tests += util/test-diag.cpp
else # !NO_DIAG
if !NO_CO_DCF
src_util_tests += util/test-diag.cpp
else # !NO_CO_DCF
if !NO_CO_GW_TXT
src_util_tests += util/test-diag.cpp
endif # !NO_CO_GW_TXT
endif # !NO_CO_DCF
endif # !NO_DIAG

bin += unit-test-util
unit_test_util_SOURCES = $(src_common) $(src_util_tests)
unit_test_util_LDADD = $(LELY_LIBC_LIBS) $(LELY_UTIL_LIBS) $(CPPUTEST_LIBS)

# CAN library tests

LELY_CAN_LIBS = $(top_builddir)/src/can/liblely-can.la

bin += unit-test-can

src_can_tests =
src_can_tests += can/test-buf.cpp
src_can_tests += can/test-msg.cpp
src_can_tests += can/test-net.cpp

unit_test_can_SOURCES = $(src_common) $(src_can_tests)
unit_test_can_LDADD = $(LELY_LIBC_LIBS) $(LELY_UTIL_LIBS) $(LELY_CAN_LIBS) $(CPPUTEST_LIBS)

# CANopen library tests

LELY_CO_LIBS = $(top_builddir)/src/co/liblely-co.la

bin += unit-test-co

src_co_tests =
src_co_tests += co/holder/array-init.hpp
src_co_tests += co/holder/dev.hpp
src_co_tests += co/holder/holder.hpp
src_co_tests += co/holder/obj.hpp
src_co_tests += co/holder/sub.hpp
src_co_tests += co/test-crc.cpp
src_co_tests += co/test-dev.cpp
src_co_tests += co/test-obj.cpp
src_co_tests += co/test-rpdo.cpp
src_co_tests += co/test-type.cpp
src_co_tests += co/test-val.cpp
if !PLATFORM_WIN32
if !ENABLE_SHARED
src_co_tests += override/lelyco-val.cpp
endif
endif

unit_test_co_SOURCES = $(src_common) $(src_co_tests)
unit_test_co_LDADD = $(LELY_LIBC_LIBS) $(LELY_UTIL_LIBS) $(LELY_CAN_LIBS) $(LELY_OVERRIDE_CO_VAL_LIB) $(LELY_CO_LIBS) $(CPPUTEST_LIBS)
if !PLATFORM_WIN32
if !ENABLE_SHARED
unit_test_co_LDFLAGS =
unit_test_co_LDFLAGS += -Wl,--wrap=co_val_read
unit_test_co_LDFLAGS += -Wl,--wrap=co_val_write
unit_test_co_LDFLAGS += -Wl,--wrap=co_val_make
unit_test_co_LDFLAGS += -Wl,--wrap=co_val_init_min
unit_test_co_LDFLAGS += -Wl,--wrap=co_val_init_max
unit_test_co_LDFLAGS += -Wl,--wrap=co_val_init
endif
else
unit_test_co_LDFLAGS = -all-static
endif

# Compilation flags

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}\" -DLELY_NO_CXX=1
if ENABLE_SHARED
AM_CPPFLAGS += -DLELY_ENABLE_SHARED=1
endif
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
