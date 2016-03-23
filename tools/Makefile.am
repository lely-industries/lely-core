bin =
if !NO_DCF
if !NO_SDEV
bin += dcf2c
dcf2c_SOURCES = dcf2c.c
endif
endif

bin_PROGRAMS = $(bin)

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += $(liblely_libc_CFLAGS)
AM_CPPFLAGS += $(liblely_util_CFLAGS)
AM_CPPFLAGS += $(liblely_can_CFLAGS)

LDADD = $(top_srcdir)/src/liblely-co.la
LDADD += $(liblely_libc_LIBS)
LDADD += $(liblely_util_LIBS)
LDADD += $(liblely_can_LIBS)

