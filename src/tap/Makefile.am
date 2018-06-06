src =
src += tap.c
src += tap.h

lib_LTLIBRARIES = liblely-tap.la
liblely_tap_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_tap_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_tap_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_tap_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_tap_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_tap_la_LDFLAGS += -Wl,--output-def,liblely-tap-$(VERSION_MAJOR).def
endif
liblely_tap_la_LDFLAGS += -Wl,--as-needed
liblely_tap_la_LIBADD =
liblely_tap_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
if CODE_COVERAGE_ENABLED
liblely_tap_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_tap_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_tap_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-tap-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
