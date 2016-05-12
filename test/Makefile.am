bin =
bin += test-tap
test_tap_SOURCES = tap.c

check_PROGRAMS = $(bin)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += $(liblely_libc_CFLAGS)

LDADD = $(top_srcdir)/src/liblely-tap.la
LDADD += $(liblely_libc_LIBS)

TESTS = $(check_PROGRAMS)

LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TEST_LOG_DRIVER = $(LOG_DRIVER)

