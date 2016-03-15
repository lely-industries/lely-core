src =
src += util.h

lib_LTLIBRARIES = liblely-util.la
liblely_util_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS)
liblely_util_la_LDFLAGS = -version-number 1:0:0
liblely_util_la_LIBADD = $(liblely_libc_LIBS)
liblely_util_la_SOURCES = $(src)
