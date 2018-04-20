src =
src += coapp.hpp

lib_LTLIBRARIES = liblely-coapp.la
liblely_coapp_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_coapp_la_CPPFLAGS += $(liblely_libc_CFLAGS)
liblely_coapp_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
liblely_coapp_la_LDFLAGS += -Wl,--as-needed
liblely_coapp_la_LIBADD = $(liblely_libc_LIBS)
liblely_coapp_la_SOURCES = $(src)
