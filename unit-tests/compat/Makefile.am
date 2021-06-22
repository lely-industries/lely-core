bin =

LELY_COMPAT_LIBS =
if CODE_COVERAGE_ENABLED
LELY_COMPAT_LIBS += $(CODE_COVERAGE_LIBS)
endif

LELY_COMPAT_LIBS += $(top_builddir)/lib/compat/liblely-compat.la

LELY_UNIT_TEST_COMPAT_LDFLGS =

if PLATFORM_WIN32
LELY_UNIT_TEST_COMPAT_LDFLGS += -all-static
CPPUTEST_LIBS += -lpthread
endif

LELY_UNIT_TEST_COMPAT_COMMON_LIBS =
LELY_UNIT_TEST_COMPAT_COMMON_LIBS += $(LELY_COMPAT_LIBS)
LELY_UNIT_TEST_COMPAT_COMMON_LIBS += $(CPPUTEST_LIBS)

src_common =
src_common += tests-main.cpp

# CANopen library tests

# Compatibility library tests

if NO_HOSTED
bin += unit-test-compat-string
unit_test_compat_string_SOURCES = $(src_common) test-string.cpp
unit_test_compat_string_LDADD = $(LELY_UNIT_TEST_COMPAT_COMMON_LIBS)
unit_test_compat_string_LDFLAGS = $(LELY_UNIT_TEST_COMPAT_LDFLGS)
endif NO_HOSTED

bin += unit-test-compat-strings
unit_test_compat_strings_SOURCES = $(src_common) test-strings.cpp
unit_test_compat_strings_LDADD = $(LELY_UNIT_TEST_COMPAT_COMMON_LIBS)
unit_test_compat_strings_LDFLAGS = $(LELY_UNIT_TEST_COMPAT_LDFLGS)

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

AM_TESTS_ENVIRONMENT = TEST_ARGS="-ojunit -v"; export TEST_ARGS;

if CODE_COVERAGE_ENABLED
LOG_COMPILER = $(SHELL) $(top_builddir)/lcov-wrapper.sh
AM_TESTS_ENVIRONMENT += EXEC_WRAPPER="$(EXEC)"; export EXEC_WRAPPER;
@CODE_COVERAGE_RULES@
else
LOG_COMPILER = $(EXEC)
endif

@VALGRIND_CHECK_RULES@
