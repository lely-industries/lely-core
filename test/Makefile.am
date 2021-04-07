bin =

# C11 and POSIX compatibility library tests

LELY_LIBC_LIBS =
if CODE_COVERAGE_ENABLED
LELY_LIBC_LIBS += $(CODE_COVERAGE_LIBS)
endif
LELY_LIBC_LIBS += $(top_builddir)/src/libc/liblely-libc.la
LELY_LIBC_LIBS += $(top_builddir)/src/tap/liblely-tap.la

# Test Anything Protocol (TAP) library tests

LELY_TAP_LIBS = $(LELY_LIBC_LIBS)

bin += test-tap
test_tap_SOURCES = test.h tap.c
test_tap_LDADD = $(LELY_TAP_LIBS)

# Utilities library tests

LELY_UTIL_LIBS = $(LELY_TAP_LIBS)
LELY_UTIL_LIBS += $(top_builddir)/src/util/liblely-util.la

if !NO_STDIO
bin += test-util-config
test_util_config_SOURCES = test.h util-config.c
test_util_config_LDADD = $(LELY_UTIL_LIBS)
endif

bin += test-util-endian
test_util_endian_SOURCES = test.h util-endian.c
test_util_endian_LDADD = $(LELY_UTIL_LIBS)

if !NO_STDIO
bin += test-util-fbuf
test_util_fbuf_SOURCES = test.h util-fbuf.c
test_util_fbuf_LDADD = $(LELY_UTIL_LIBS)
endif

if !NO_CXX
bin += test-util-fiber
test_util_fiber_SOURCES = test.h util-fiber.cpp
test_util_fiber_LDADD = $(LELY_UTIL_LIBS)
endif

if !ECSS_COMPLIANCE
if !NO_THREADS
bin += test-util-spscring
test_util_spscring_SOURCES = test.h util-spscring.c
test_util_spscring_LDADD = $(LELY_UTIL_LIBS)
endif
endif

if !ECSS_COMPLIANCE
if !NO_THREADS
if !NO_CXX
bin += test-util-stop
test_util_stop_SOURCES = test.h util-stop.cpp
test_util_stop_LDADD = $(LELY_UTIL_LIBS)
endif
endif
endif

# CAN library tests

LELY_CAN_LIBS = $(LELY_UTIL_LIBS)
LELY_CAN_LIBS += $(top_builddir)/src/can/liblely-can.la

if !NO_MALLOC
bin += test-can-net
test_can_net_SOURCES = test.h can-net.c
test_can_net_LDADD = $(LELY_CAN_LIBS)
endif

# I/O library tests

LELY_IO_LIBS = $(LELY_CAN_LIBS)
LELY_IO_LIBS += $(top_builddir)/src/io/liblely-io.la

if !NO_STDIO

if !NO_CXX
bin += test-io-cxx
test_io_cxx_SOURCES = test.h io-cxx.cpp
test_io_cxx_LDADD = $(LELY_IO_LIBS)
endif

bin += test-io-poll
test_io_poll_SOURCES = test.h io-poll.c
test_io_poll_LDADD = $(LELY_IO_LIBS)

endif # !NO_STDIO

# Event library tests

LELY_EV_LIBS = $(LELY_UTIL_LIBS)
LELY_EV_LIBS += $(top_builddir)/src/ev/liblely-ev.la

if !NO_CXX
bin += test-ev-fiber
test_ev_fiber_SOURCES = test.h ev-fiber.cpp
test_ev_fiber_LDADD = $(LELY_EV_LIBS)
endif

if !NO_CXX
bin += test-ev-future
test_ev_future_SOURCES = test.h ev-future.cpp
test_ev_future_LDADD = $(LELY_EV_LIBS)
endif

if !NO_CXX
bin += test-ev-loop
test_ev_loop_SOURCES = test.h ev-loop.cpp
test_ev_loop_LDADD = $(LELY_EV_LIBS)
endif

# I/O library tests

LELY_IO2_LIBS = $(LELY_EV_LIBS)
LELY_IO2_LIBS += $(top_builddir)/src/io2/liblely-io2.la

if !NO_STDIO

if !NO_CXX
bin += test-io2-can_rt
test_io2_can_rt_SOURCES = test.h io2-can_rt.cpp
test_io2_can_rt_LDADD = $(LELY_IO2_LIBS)
endif

if PLATFORM_POSIX
if !NO_CXX
bin += test-io2-sigset
test_io2_sigset_SOURCES = test.h io2-sigset.cpp
test_io2_sigset_LDADD = $(LELY_IO2_LIBS)
endif
endif

if !NO_CXX
bin += test-io2-timer
test_io2_timer_SOURCES = test.h io2-timer.cpp
test_io2_timer_LDADD = $(LELY_IO2_LIBS)
endif

if !NO_CXX
bin += test-io2-tqueue
test_io2_tqueue_SOURCES = test.h io2-tqueue.cpp
test_io2_tqueue_LDADD = $(LELY_IO2_LIBS)
endif

endif # !NO_STDIO

if !NO_CXX
bin += test-io2-vcan
test_io2_vcan_SOURCES = test.h io2-vcan.cpp
test_io2_vcan_LDADD = $(LELY_IO2_LIBS)
endif

# CANopen library tests

LELY_CO_LIBS = $(LELY_CAN_LIBS)
LELY_CO_LIBS += $(top_builddir)/src/co/liblely-co.la

if !NO_CXX
bin += test-co-cxx
test_co_cxx_SOURCES = test.h co-cxx.cpp
test_co_cxx_LDADD = $(LELY_CO_LIBS)
endif

if !NO_MALLOC
if !NO_CO_DCF

if !NO_CO_EMCY
bin += test-co-emcy
test_co_emcy_SOURCES = co-test.h co-emcy.c
test_co_emcy_LDADD = $(LELY_CO_LIBS)
endif

if !NO_CO_GW_TXT
bin += test-co-gw_txt
test_co_gw_txt_SOURCES = co-test.h co-gw_txt.c
test_co_gw_txt_LDADD = $(LELY_CO_LIBS)
endif

if !NO_CO_MASTER
bin += test-co-nmt
test_co_nmt_SOURCES = co-test.h co-nmt.c
test_co_nmt_LDADD = $(LELY_CO_LIBS)
endif

if !NO_CO_RPDO
if !NO_CO_TPDO
bin += test-co-pdo
test_co_pdo_SOURCES = co-test.h co-pdo.c
test_co_pdo_LDADD = $(LELY_CO_LIBS)
endif
endif

if !NO_TOOLS
if !NO_CO_SDEV
bin += test-co-sdev
test_co_sdev_SOURCES = co-test.h co-sdev.c
nodist_test_co_sdev_SOURCES = test-co-sdev.h
test_co_sdev_LDADD = $(LELY_CO_LIBS)
test-co-sdev.h: co-sdev.dcf $(top_builddir)/tools/dcf2c$(EXEEXT)
if CODE_COVERAGE_ENABLED
	EXEC_WRAPPER="$(EXEC)" TEST_NAME="test-co-sdev-dcf2c" $(LOG_COMPILER) $(top_builddir)/tools/dcf2c$(EXEEXT) $< test_co_sdev -o $@
else
	$(EXEC) $(top_builddir)/tools/dcf2c$(EXEEXT) $< test_co_sdev -o $@
endif
endif
endif

if !NO_CO_CSDO
bin += test-co-sdo
test_co_sdo_SOURCES = co-test.h co-sdo.c
test_co_sdo_LDADD = $(LELY_CO_LIBS)
endif

if !NO_CO_SYNC
bin += test-co-sync
test_co_sync_SOURCES = co-test.h co-sync.c
test_co_sync_LDADD = $(LELY_CO_LIBS)
endif

if !NO_CO_TIME
bin += test-co-time
test_co_time_SOURCES = co-test.h co-time.c
test_co_time_LDADD = $(LELY_CO_LIBS)
endif

endif # !NO_CO_DCF
endif # !NO_MALLOC

# C++ CANopen application library tests

LELY_COAPP_LIBS = $(LELY_IO2_LIBS) $(LELY_CO_LIBS)
LELY_COAPP_LIBS += $(top_builddir)/src/coapp/liblely-coapp.la

if !NO_CXX
if !NO_STDIO
if !NO_CO_DCF

if !NO_COAPP_MASTER
bin += test-coapp-fiber
test_coapp_fiber_SOURCES = test.h coapp-fiber.cpp
test_coapp_fiber_LDADD = $(LELY_COAPP_LIBS)
endif

if !NO_COAPP_MASTER
if !NO_CO_LSS
bin += test-coapp-lss
test_coapp_lss_SOURCES = test.h coapp-lss.cpp
test_coapp_lss_LDADD = $(LELY_COAPP_LIBS)
endif
endif

endif # !NO_CO_DCF
endif # !NO_STDIO
endif # !NO_CXX

EXTRA_DIST =
EXTRA_DIST += util-config.ini
EXTRA_DIST += co-emcy.dcf
if !NO_CO_GW_TXT
EXTRA_DIST += co-gw_txt-master.dcf
EXTRA_DIST += co-gw_txt-slave.dcf
endif
EXTRA_DIST += co-nmt-master.dat
EXTRA_DIST += co-nmt-slave.dcf
EXTRA_DIST += co-pdo-receive.dcf
EXTRA_DIST += co-pdo-transmit.dcf
EXTRA_DIST += co-sdev.dcf
EXTRA_DIST += co-sdo-client.dcf
EXTRA_DIST += co-sdo-server.dcf
EXTRA_DIST += co-sync.dcf
EXTRA_DIST += co-time.dcf
if !NO_CXX
if !NO_COAPP_MASTER
EXTRA_DIST += coapp-fiber-master.dcf
EXTRA_DIST += coapp-fiber-slave.dcf
EXTRA_DIST += coapp-lss-master.dcf
EXTRA_DIST += coapp-lss-slave.dcf
endif
endif

BUILT_SOURCES =
if !NO_TOOLS
if !NO_CO_DCF
if !NO_CO_SDEV
BUILT_SOURCES += test-co-sdev.h
endif
endif
endif

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

AM_CXXFLAGS =
if CODE_COVERAGE_ENABLED
AM_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif

TESTS = $(check_PROGRAMS)

EXEC = $(SHELL) $(top_builddir)/exec-wrapper.sh
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TEST_LOG_DRIVER = $(LOG_DRIVER)

if CODE_COVERAGE_ENABLED
LOG_COMPILER = $(SHELL) $(top_builddir)/lcov-wrapper.sh
AM_TESTS_ENVIRONMENT = EXEC_WRAPPER="$(EXEC)"
AM_TESTS_ENVIRONMENT += ;
@CODE_COVERAGE_RULES@
else
LOG_COMPILER = $(EXEC)
endif

@VALGRIND_CHECK_RULES@
