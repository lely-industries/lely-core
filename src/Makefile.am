src =
src += tap.c
src += tap.h
if PLATFORM_WIN32
src += version.rc
endif

lib_LTLIBRARIES = liblely-tap.la
liblely_tap_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_tap_la_CPPFLAGS += $(liblely_libc_CFLAGS)
if CODE_COVERAGE_ENABLED
liblely_tap_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_tap_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_tap_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_tap_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_tap_la_LDFLAGS += -Wl,--output-def,$(PACKAGE)-$(VERSION_MAJOR).def
endif
liblely_tap_la_LIBADD = $(liblely_libc_LIBS)
if CODE_COVERAGE_ENABLED
liblely_tap_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_tap_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = $(PACKAGE)-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
