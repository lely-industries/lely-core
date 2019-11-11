AM_CPPFLAGS =

bin =
etc =
if !NO_CO_WTM
if PLATFORM_LINUX
bin += can2udp
can2udp_SOURCES = can2udp.c
endif
endif
if !NO_CO_GW_TXT
if !NO_CO_DCF
if !NO_THREADS
if PLATFORM_LINUX
bin += coctl
coctl_SOURCES = coctl.c
etc += coctl.dcf
endif
endif
endif
endif
if PLATFORM_LINUX
if !NO_THREADS
bin += cocat
cocat_SOURCES = cocat.c
endif
if !NO_CO_TPDO
if !NO_CO_DCF
bin += cocatd
cocatd_SOURCES = cocatd.c
etc += cocatd.dcf
AM_CPPFLAGS += -DCOCATD_DCF="\"$(sysconfdir)/cocatd.dcf\""
endif
endif
endif
if !NO_CO_DCF
if !NO_CO_SDEV
bin += dcf2c
dcf2c_SOURCES = dcf2c.c
if PLATFORM_WIN32
nodist_dcf2c_SOURCES = version.dcf2c.rc
endif
endif
endif

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

if PLATFORM_WIN32
.rc.o:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
endif
