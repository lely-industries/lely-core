src =
src += io.h

lib_LTLIBRARIES = liblely-io.la
liblely_io_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_io_la_CPPFLAGS += $(liblely_libc_CFLAGS)
liblely_io_la_CPPFLAGS += $(liblely_util_CFLAGS)
liblely_io_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
liblely_io_la_LIBADD = $(liblely_libc_LIBS)
liblely_io_la_LIBADD += $(liblely_util_LIBS)
liblely_io_la_SOURCES = $(src)
