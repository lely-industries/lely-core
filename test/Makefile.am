bin =

check_PROGRAMS = $(bin)

extra =

EXTRA_DIST = $(extra)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += $(liblely_libc_CFLAGS)
AM_CPPFLAGS += $(liblely_tap_CFLAGS)
AM_CPPFLAGS += $(liblely_util_CFLAGS)
AM_CPPFLAGS += $(liblely_can_CFLAGS)

LDADD = $(top_srcdir)/src/liblely-co.la
LDADD += $(liblely_libc_LIBS)
LDADD += $(liblely_tap_LIBS)
LDADD += $(liblely_util_LIBS)
LDADD += $(liblely_can_LIBS)

TESTS = $(check_PROGRAMS)

LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TEST_LOG_DRIVER = $(LOG_DRIVER)
