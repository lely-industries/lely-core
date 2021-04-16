bin =

LELY_COMPAT_LIBS =
if CODE_COVERAGE_ENABLED
LELY_COMPAT_LIBS += $(CODE_COVERAGE_LIBS)
endif

LELY_COMPAT_LIBS += $(top_builddir)/lib/compat/liblely-compat.la

if PLATFORM_WIN32
CPPUTEST_LIBS += -lpthread
endif

LELY_UNIT_TEST_UTIL_COMMON_LIBS =
LELY_UNIT_TEST_UTIL_COMMON_LIBS += $(LELY_COMPAT_LIBS)
LELY_UNIT_TEST_UTIL_COMMON_LIBS += $(LELY_UTIL_LIBS)
LELY_UNIT_TEST_UTIL_COMMON_LIBS += $(CPPUTEST_LIBS)

src_common =
src_common += tests-main.cpp

# Utilities library tests

LELY_UTIL_LIBS = $(top_builddir)/lib/util/liblely-util.la

bin += unit-test-util-bits
unit_test_util_bits_SOURCES = $(src_common) test-bits.cpp
unit_test_util_bits_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-cmp
unit_test_util_cmp_SOURCES = $(src_common) test-cmp.cpp
unit_test_util_cmp_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-dllist
unit_test_util_dllist_SOURCES = $(src_common) test-dllist.cpp
unit_test_util_dllist_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-endian
unit_test_util_endian_SOURCES = $(src_common) test-endian.cpp
unit_test_util_endian_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-error
unit_test_util_error_SOURCES = $(src_common) test-error.cpp
unit_test_util_error_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-membuf
unit_test_util_membuf_SOURCES = $(src_common) test-membuf.cpp
unit_test_util_membuf_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-memory
unit_test_util_memory_SOURCES = $(src_common) test-memory.cpp
unit_test_util_memory_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-mempool
unit_test_util_mempool_SOURCES = $(src_common) test-mempool.cpp
unit_test_util_mempool_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-pheap
unit_test_util_pheap_SOURCES = $(src_common) test-pheap.cpp
unit_test_util_pheap_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-rbtree
unit_test_util_rbtree_SOURCES = $(src_common) test-rbtree.cpp
unit_test_util_rbtree_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-time
unit_test_util_time_SOURCES = $(src_common) test-time.cpp
unit_test_util_time_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-ustring
unit_test_util_ustring_SOURCES = $(src_common) test-ustring.cpp
unit_test_util_ustring_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-util
unit_test_util_util_SOURCES = $(src_common) test-util.cpp
unit_test_util_util_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

if !ECSS_COMPLIANCE
if !NO_MALLOC
bin += unit-test-util-bitset
unit_test_util_bitset_SOURCES = $(src_common) test-bitset.cpp
unit_test_util_bitset_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)
endif

if !NO_CXX
bin += unit-test-util-mutex
unit_test_util_mutex_SOURCES = $(src_common) test-mutex.cpp
unit_test_util_mutex_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)
endif

bin += unit-test-util-sllist
unit_test_util_sllist_SOURCES = $(src_common) test-sllist.cpp
unit_test_util_sllist_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)

bin += unit-test-util-spscring
unit_test_util_spscring_SOURCES = $(src_common) test-spscring.cpp
unit_test_util_spscring_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)
endif

if !NO_DIAG
bin += unit-test-util-diag
unit_test_util_diag_SOURCES = $(src_common) test-diag.cpp
unit_test_util_diag_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)
else # !NO_DIAG
if !NO_CO_DCF
bin += unit-test-util-diag
unit_test_util_diag_SOURCES = $(src_common) test-diag.cpp
unit_test_util_diag_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)
else # !NO_CO_DCF
if !NO_CO_GW_TXT
bin += unit-test-util-diag
unit_test_util_diag_SOURCES = $(src_common) test-diag.cpp
unit_test_util_diag_LDADD = $(LELY_UNIT_TEST_UTIL_COMMON_LIBS)
endif # !NO_CO_GW_TXT
endif # !NO_CO_DCF
endif # !NO_DIAG

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
