src =
src += can.h

lib_LTLIBRARIES = liblely-can.la
liblely_can_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS) $(liblely_util_CFLAGS)
liblely_can_la_LDFLAGS = -version-number 1:0:0
liblely_can_la_LIBADD = $(liblely_libc_LIBS) $(liblely_util_LIBS)
liblely_can_la_SOURCES = $(src)
