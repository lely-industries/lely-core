bin =
bin += test-fbuf
test_fbuf_SOURCES = fbuf.c

CLEANFILES =
CLEANFILES += fbuf.dat

check_PROGRAMS = $(bin)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += $(liblely_libc_CFLAGS)
AM_CPPFLAGS += $(liblely_tap_CFLAGS)

LDADD = $(top_srcdir)/src/liblely-util.la
LDADD += $(liblely_libc_LIBS)
LDADD += $(liblely_tap_LIBS)

TESTS = $(check_PROGRAMS)

LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TEST_LOG_DRIVER = $(LOG_DRIVER)
