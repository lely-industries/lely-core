bin =

bin += test-tap
test_tap_SOURCES = tap.c

bin += test-util-config
test_util_config_SOURCES = util-config.c
bin += test-util-endian
test_util_endian_SOURCES = util-endian.c
bin += test-util-c_factory
test_util_c_factory_SOURCES = util-c_factory.cpp
bin += test-util-fbuf
test_util_fbuf_SOURCES = util-fbuf.c
bin += test-util-xtime
test_util_xtime_SOURCES = util-xtime.c

bin += test-can-net
test_can_net_SOURCES = can-net.c

bin += test-io-cxx
test_io_cxx_SOURCES = io-cxx.cpp
bin += test-io-poll
test_io_poll_SOURCES = io-poll.c

EXTRA_DIST =
EXTRA_DIST += util-config.ini

CLEANFILES =
CLEANFILES += util-fbuf.dat

check_PROGRAMS = $(bin)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}\"
if CODE_COVERAGE_ENABLED
AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif

AM_CFLAGS =
if CODE_COVERAGE_ENABLED
AM_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif

LDADD =
LDADD += $(top_builddir)/src/libc/liblely-libc.la
LDADD += $(top_builddir)/src/tap/liblely-tap.la
LDADD += $(top_builddir)/src/util/liblely-util.la
LDADD += $(top_builddir)/src/can/liblely-can.la
LDADD += $(top_builddir)/src/io/liblely-io.la
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
