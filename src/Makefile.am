src =
src += co.h

lib_LTLIBRARIES = liblely-co.la
liblely_co_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS) $(liblely_util_CFLAGS) $(liblely_co_CFLAGS)
liblely_co_la_LDFLAGS = -version-number 1:0:0
liblely_co_la_LIBADD = $(liblely_libc_LIBS) $(liblely_util_LIBS) $(liblely_co_LIBS)
liblely_co_la_SOURCES = $(src)
