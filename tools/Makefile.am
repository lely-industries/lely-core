bin =
if !NO_CO_DCF
if !NO_CO_SDEV
bin += dcf2c
dcf2c_SOURCES = dcf2c.c
if PLATFORM_WIN32
dcf2c_SOURCES += version.dcf2c.rc
endif
endif
endif
if !NO_CO_WTM
if !NO_IO
if PLATFORM_LINUX
bin += can2udp
can2udp_SOURCES = can2udp.c
endif
endif
endif

bin_PROGRAMS = $(bin)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += $(liblely_libc_CFLAGS)
AM_CPPFLAGS += $(liblely_util_CFLAGS)
AM_CPPFLAGS += $(liblely_can_CFLAGS)
if !NO_IO
AM_CPPFLAGS += $(liblely_io_CFLAGS)
endif

LDADD = $(top_srcdir)/src/liblely-co.la
LDADD += $(liblely_libc_LIBS)
LDADD += $(liblely_util_LIBS)
LDADD += $(liblely_can_LIBS)
if !NO_IO
LDADD += $(liblely_io_LIBS)
endif

if PLATFORM_WIN32
.rc.o:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
endif
