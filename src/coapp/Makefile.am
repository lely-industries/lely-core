src =
src += coapp.hpp
src += io_context.cpp
src += sdo_error.cpp

lib_LTLIBRARIES = liblely-coapp.la
liblely_coapp_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_coapp_la_CPPFLAGS += $(liblely_libc_CFLAGS)
liblely_coapp_la_CPPFLAGS += $(liblely_util_CFLAGS)
liblely_coapp_la_CPPFLAGS += $(liblely_can_CFLAGS)
liblely_coapp_la_CPPFLAGS += $(liblely_co_CFLAGS)
liblely_coapp_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_coapp_la_LDFLAGS += -Wl,--output-def,lib$(PACKAGE)-$(VERSION_MAJOR).def
endif
liblely_coapp_la_LDFLAGS += -Wl,--as-needed
liblely_coapp_la_LIBADD = $(liblely_libc_LIBS)
liblely_coapp_la_LIBADD += $(liblely_util_LIBS)
liblely_coapp_la_LIBADD += $(liblely_can_LIBS)
liblely_coapp_la_LIBADD += $(top_builddir)/src/aio/liblely-aio.la
liblely_coapp_la_LIBADD += $(liblely_co_LIBS)
liblely_coapp_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = lib$(PACKAGE)-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif