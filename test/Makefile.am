bin =
if !NO_CO_DCF
if !NO_CO_EMCY
bin += test-emcy
test_emcy_SOURCES = test.h emcy.c
endif
if !NO_CO_MASTER
bin += test-nmt
test_nmt_SOURCES = test.h nmt.c
endif
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
if !NO_CO_TIME
bin += test-time
test_time_SOURCES = test.h time.c
endif
endif

EXTRA_DIST =
EXTRA_DIST += emcy.dcf
EXTRA_DIST += nmt-master.dat
EXTRA_DIST += nmt-master.dcf
EXTRA_DIST += nmt-slave.dcf
EXTRA_DIST += pdo-receive.dcf
EXTRA_DIST += pdo-transmit.dcf
EXTRA_DIST += sdo-client.dcf
EXTRA_DIST += sdo-server.dcf
EXTRA_DIST += sync.dcf
EXTRA_DIST += time.dcf

CLEANFILES =
CLEANFILES += nmt-slave.dat

check_PROGRAMS = $(bin)

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
