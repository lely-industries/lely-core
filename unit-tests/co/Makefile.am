bin =

LELY_COMPAT_LIBS =
if CODE_COVERAGE_ENABLED
LELY_COMPAT_LIBS += $(CODE_COVERAGE_LIBS)
endif

LELY_COMPAT_LIBS += $(top_builddir)/lib/compat/liblely-compat.la

LELY_UNIT_TEST_CO_LDFLGS =

if PLATFORM_WIN32
LELY_UNIT_TEST_CO_LDFLGS += -all-static
CPPUTEST_LIBS += -lpthread
endif

LELY_UNIT_TEST_LIBS = $(top_builddir)/unit-tests/libtest/tools/liblely-unit-test.la

LELY_OVERRIDE_LIBS =
LELY_OVERRIDE_CO_VAL_LIB =
LELY_OVERRIDE_CO_VAL_LDFLGS =

if !PLATFORM_WIN32
LELY_OVERRIDE_LIBS += $(top_srcdir)/unit-tests/libtest/override/liblely-override-plugin.la
LELY_OVERRIDE_CO_VAL_LIB += $(top_srcdir)/unit-tests/libtest/override/liblelyco-val-override.la
if !ENABLE_SHARED
LELY_OVERRIDE_CO_VAL_LDFLGS += -Wl,--wrap=co_val_read
LELY_OVERRIDE_CO_VAL_LDFLGS += -Wl,--wrap=co_val_write
LELY_OVERRIDE_CO_VAL_LDFLGS += -Wl,--wrap=co_val_make
LELY_OVERRIDE_CO_VAL_LDFLGS += -Wl,--wrap=co_val_init_min
LELY_OVERRIDE_CO_VAL_LDFLGS += -Wl,--wrap=co_val_init_max
LELY_OVERRIDE_CO_VAL_LDFLGS += -Wl,--wrap=co_val_init
endif
endif

LELY_UNIT_TEST_CO_COMMON_LIBS =
LELY_UNIT_TEST_CO_COMMON_LIBS += $(LELY_CO_LIBS)
LELY_UNIT_TEST_CO_COMMON_LIBS += $(LELY_CAN_LIBS)
LELY_UNIT_TEST_CO_COMMON_LIBS += $(LELY_UTIL_LIBS)
LELY_UNIT_TEST_CO_COMMON_LIBS += $(LELY_COMPAT_LIBS)
LELY_UNIT_TEST_CO_COMMON_LIBS += $(LELY_UNIT_TEST_LIBS)
LELY_UNIT_TEST_CO_COMMON_LIBS += $(LELY_OVERRIDE_LIBS)
LELY_UNIT_TEST_CO_COMMON_LIBS += $(CPPUTEST_LIBS)

src_common =
src_common += tests-main.cpp

# CANopen library tests

LELY_UTIL_LIBS = $(top_builddir)/lib/util/liblely-util.la
LELY_CAN_LIBS = $(top_builddir)/lib/can/liblely-can.la
LELY_CO_LIBS = $(top_builddir)/lib/co/liblely-co.la

bin += unit-test-co-crc
unit_test_co_crc_SOURCES = $(src_common) test-crc.cpp
unit_test_co_crc_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_crc_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-csdo
unit_test_co_csdo_SOURCES = $(src_common) test-csdo.cpp
unit_test_co_csdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_csdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-dev
unit_test_co_dev_SOURCES = $(src_common) test-dev.cpp
unit_test_co_dev_LDADD = $(LELY_OVERRIDE_CO_VAL_LIB) $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_dev_LDFLAGS = $(LELY_OVERRIDE_CO_VAL_LDFLGS) $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-emcy
unit_test_co_emcy_SOURCES = $(src_common) test-emcy.cpp
unit_test_co_emcy_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_emcy_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-nmt
unit_test_co_nmt_SOURCES = $(src_common) test-nmt.cpp
unit_test_co_nmt_LDADD = $(LELY_OVERRIDE_CO_VAL_LIB) $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_nmt_LDFLAGS = $(LELY_OVERRIDE_CO_VAL_LDFLGS) $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-obj
unit_test_co_obj_SOURCES = $(src_common) test-obj.cpp
unit_test_co_obj_LDADD = $(LELY_OVERRIDE_CO_VAL_LIB) $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_obj_LDFLAGS = $(LELY_OVERRIDE_CO_VAL_LDFLGS) $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-pdo
unit_test_co_pdo_SOURCES = $(src_common) test-pdo.cpp
unit_test_co_pdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_pdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-rpdo
unit_test_co_rpdo_SOURCES = $(src_common) test-rpdo.cpp
unit_test_co_rpdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_rpdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-rpdo-sdo
unit_test_co_rpdo_sdo_SOURCES = $(src_common) test-rpdo-sdo.cpp
unit_test_co_rpdo_sdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_rpdo_sdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-sdo
unit_test_co_sdo_SOURCES = $(src_common) test-sdo.cpp
unit_test_co_sdo_LDADD = $(LELY_OVERRIDE_CO_VAL_LIB) $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_sdo_LDFLAGS = $(LELY_OVERRIDE_CO_VAL_LDFLGS) $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-ssdo
unit_test_co_ssdo_SOURCES = $(src_common) test-ssdo.cpp
unit_test_co_ssdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_ssdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-ssdo-sdo
unit_test_co_ssdo_sdo_SOURCES = $(src_common) test-ssdo-sdo.cpp
unit_test_co_ssdo_sdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_ssdo_sdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-sync
unit_test_co_sync_SOURCES = $(src_common) test-sync.cpp
unit_test_co_sync_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_sync_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-sync-sdo
unit_test_co_sync_sdo_SOURCES = $(src_common) test-sync-sdo.cpp
unit_test_co_sync_sdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_sync_sdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-tpdo
unit_test_co_tpdo_SOURCES = $(src_common) test-tpdo.cpp
unit_test_co_tpdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_tpdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-tpdo-sdo
unit_test_co_tpdo_sdo_SOURCES = $(src_common) test-tpdo-sdo.cpp
unit_test_co_tpdo_sdo_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_tpdo_sdo_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-type
unit_test_co_type_SOURCES = $(src_common) test-type.cpp
unit_test_co_type_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_type_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)

bin += unit-test-co-val
unit_test_co_val_SOURCES = $(src_common) test-val.cpp
unit_test_co_val_LDADD = $(LELY_UNIT_TEST_CO_COMMON_LIBS)
unit_test_co_val_LDFLAGS = $(LELY_UNIT_TEST_CO_LDFLGS)


# Compilation flags

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/unit-tests
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}\"
if ENABLE_SHARED
AM_CPPFLAGS += -DLELY_ENABLE_SHARED=1
endif
AM_CPPFLAGS += $(CPPUTEST_CFLAGS)

if NO_UNIT_TESTS_MEMLEAK
AM_CPPFLAGS += -DCPPUTEST_MEM_LEAK_DETECTION_DISABLED
endif

if NO_HOSTED
# even in no-hosted unit-tests are compiled with libc
AM_CPPFLAGS += -DLELY_HAVE_TIMESPEC -DLELY_HAVE_ITIMERSPEC
endif

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
