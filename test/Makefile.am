bin =
bin += test-config
test_config_SOURCES = config.c
bin += test-fbuf
test_fbuf_SOURCES = fbuf.c

EXTRA_DIST =
EXTRA_DIST += config.ini

CLEANFILES =
CLEANFILES += fbuf.dat

check_PROGRAMS = $(bin)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += $(liblely_libc_CFLAGS)
AM_CPPFLAGS += $(liblely_tap_CFLAGS)
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}/\"

LDADD = $(top_builddir)/src/liblely-util.la
LDADD += $(liblely_libc_LIBS)
LDADD += $(liblely_tap_LIBS)

TESTS = $(check_PROGRAMS)

LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TEST_LOG_DRIVER = $(LOG_DRIVER)
