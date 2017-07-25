bin =
bin += test-config
test_config_SOURCES = config.c
bin += test-endian
test_endian_SOURCES = endian.c
bin += test-c_factory
test_c_factory_SOURCES = c_factory.cpp
bin += test-fbuf
test_fbuf_SOURCES = fbuf.c
bin += test-xtime
test_xtime_SOURCES = xtime.c

EXTRA_DIST =
EXTRA_DIST += config.ini

CLEANFILES =
CLEANFILES += fbuf.dat

check_PROGRAMS = $(bin)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += $(liblely_libc_CFLAGS)
AM_CPPFLAGS += $(liblely_tap_CFLAGS)
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}\"
if CODE_COVERAGE_ENABLED
AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif

AM_CFLAGS =
if CODE_COVERAGE_ENABLED
AM_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif

LDADD = $(top_builddir)/src/liblely-util.la
LDADD += $(liblely_libc_LIBS)
LDADD += $(liblely_tap_LIBS)
if CODE_COVERAGE_ENABLED
LDADD += $(CODE_COVERAGE_LIBS)
endif

TESTS = $(check_PROGRAMS)

EXEC = $(SHELL) $(top_srcdir)/exec-wrapper.sh
LOG_COMPILER = $(EXEC)
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TEST_LOG_DRIVER = $(LOG_DRIVER)

if CODE_COVERAGE_ENABLED
@CODE_COVERAGE_RULES@
endif
