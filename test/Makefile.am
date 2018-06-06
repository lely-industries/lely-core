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

if !NO_CXX
bin += test-co-cxx
test_co_cxx_SOURCES = co-cxx.cpp
endif
if !NO_CO_DCF
if !NO_CO_EMCY
bin += test-co-emcy
test_co_emcy_SOURCES = co-test.h co-emcy.c
endif
if !NO_CO_GW_TXT
bin += test-co-gw_txt
test_co_gw_txt_SOURCES = co-test.h co-gw_txt.c
endif
if !NO_CO_MASTER
bin += test-co-nmt
test_co_nmt_SOURCES = co-test.h co-nmt.c
endif
if !NO_CO_RPDO
if !NO_CO_TPDO
bin += test-co-pdo
test_co_pdo_SOURCES = co-test.h co-pdo.c
endif
endif
if !NO_CO_DCF
if !NO_CO_SDEV
bin += test-co-sdev
test_co_sdev_SOURCES = co-test.h co-sdev.c
nodist_test_co_sdev_SOURCES = test-co-sdev.h
test-co-sdev.h: co-sdev.dcf $(top_builddir)/tools/dcf2c$(EXEEXT)
	$(EXEC) $(top_builddir)/tools/dcf2c$(EXEEXT) $< test_co_sdev -o $@
endif
endif
if !NO_CO_CSDO
bin += test-co-sdo
test_co_sdo_SOURCES = co-test.h co-sdo.c
endif
if !NO_CO_SYNC
bin += test-co-sync
test_co_sync_SOURCES = co-test.h co-sync.c
endif
if !NO_CO_TIME
bin += test-co-time
test_co_time_SOURCES = co-test.h co-time.c
endif
endif

EXTRA_DIST =
EXTRA_DIST += util-config.ini
EXTRA_DIST += co-emcy.dcf
EXTRA_DIST += co-gw_txt-master.dcf
EXTRA_DIST += co-gw_txt-slave.dcf
EXTRA_DIST += co-nmt-master.dat
EXTRA_DIST += co-nmt-slave.dcf
EXTRA_DIST += co-pdo-receive.dcf
EXTRA_DIST += co-pdo-transmit.dcf
EXTRA_DIST += co-sdev.dcf
EXTRA_DIST += co-sdo-client.dcf
EXTRA_DIST += co-sdo-server.dcf
EXTRA_DIST += co-sync.dcf
EXTRA_DIST += co-time.dcf

BUILT_SOURCES =
BUILT_SOURCES += test-co-sdev.h

CLEANFILES =
CLEANFILES += util-fbuf.dat
CLEANFILES += co-nmt-slave.dat
CLEANFILES += test-co-sdev.h

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
LDADD += $(top_builddir)/src/co/liblely-co.la
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
