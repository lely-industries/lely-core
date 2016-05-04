src =
src += tap.h

lib_LTLIBRARIES = liblely-tap.la
liblely_tap_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS)
liblely_tap_la_LDFLAGS = -version-number 1:0:0
liblely_tap_la_LIBADD = $(liblely_libc_LIBS)
liblely_tap_la_SOURCES = $(src)
