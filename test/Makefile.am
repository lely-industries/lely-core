bin =
if !NO_CO_RPDO
if !NO_CO_TPDO
bin += test-pdo
test_pdo_SOURCES = test.h pdo.c
endif
endif
if !NO_CO_CSDO
bin += test-sdo
test_sdo_SOURCES = test.h sdo.c
endif
if !NO_CO_SYNC
bin += test-sync
test_sync_SOURCES = test.h sync.c
endif

check_PROGRAMS = $(bin)

extra =
extra += pdo-receive.dcf
extra += pdo-transmit.dcf
extra += sdo-client.dcf
extra += sdo-server.dcf
extra += sync.dcf

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
