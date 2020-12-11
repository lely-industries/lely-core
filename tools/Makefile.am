LELY_COMPAT_LIBS =
if CODE_COVERAGE_ENABLED
LELY_COMPAT_LIBS += $(CODE_COVERAGE_LIBS)
endif
LELY_COMPAT_LIBS += $(top_builddir)/lib/compat/liblely-compat.la

LELY_UTIL_LIBS = $(LELY_COMPAT_LIBS)
LELY_UTIL_LIBS += $(top_builddir)/lib/util/liblely-util.la

LELY_CAN_LIBS = $(LELY_UTIL_LIBS)
LELY_CAN_LIBS += $(top_builddir)/lib/can/liblely-can.la

LELY_IO_LIBS = $(LELY_CAN_LIBS)
LELY_IO_LIBS += $(top_builddir)/lib/io/liblely-io.la

LELY_CO_LIBS = $(LELY_CAN_LIBS)
LELY_CO_LIBS += $(top_builddir)/lib/co/liblely-co.la

AM_CPPFLAGS =

bin =
etc =

if !ECSS_COMPLIANCE

if PLATFORM_LINUX
if !NO_CO_WTM
bin += can2udp
can2udp_SOURCES = can2udp.c
can2udp_LDADD = $(LELY_IO_LIBS) $(LELY_CO_LIBS)
endif # !NO_CO_WTM
endif # PLATFORM_LINUX

if PLATFORM_LINUX
if !NO_THREADS
if !NO_CO_DCF
if !NO_CO_GW_TXT
bin += coctl
coctl_SOURCES = coctl.c
coctl_LDADD = $(LELY_IO_LIBS) $(LELY_CO_LIBS)
etc += coctl.dcf
endif # !NO_CO_GW_TXT
endif # !NO_CO_DCF
endif # !NO_THREADS
endif # PLATFORM_LINUX

if PLATFORM_LINUX
if !NO_THREADS
bin += cocat
cocat_SOURCES = cocat.c
cocat_LDADD = $(LELY_IO_LIBS) $(LELY_CO_LIBS)
endif # !NO_THREADS
endif # PLATFORM_LINUX

if PLATFORM_LINUX
if !NO_CO_DCF
if !NO_CO_TPDO
bin += cocatd
cocatd_SOURCES = cocatd.c
cocatd_LDADD = $(LELY_IO_LIBS) $(LELY_CO_LIBS)
etc += cocatd.dcf
AM_CPPFLAGS += -DCOCATD_DCF="\"$(sysconfdir)/cocatd.dcf\""
endif # !NO_CO_TPDO
endif # !NO_CO_DCF
endif # PLATFORM_LINUX

endif # !ECSS_COMPLIANCE

if !NO_CO_DCF
if !NO_CO_SDEV
bin += dcf2c
dcf2c_SOURCES = dcf2c.c
dcf2c_LDADD = $(LELY_CO_LIBS)
if PLATFORM_WIN32
nodist_dcf2c_SOURCES = version.dcf2c.rc
endif # PLATFORM_WIN32
endif # !NO_CO_SDEV
endif # !NO_CO_DCF

bin_PROGRAMS = $(bin)
dist_sysconf_DATA = $(etc)

AM_CPPFLAGS += -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif

AM_CFLAGS =
if CODE_COVERAGE_ENABLED
AM_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif

if PLATFORM_WIN32
.rc.o:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
endif
