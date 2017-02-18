src =
src += addr.c
src += attr.c
src += attr.h
src += can.c
src += default.h
src += file.c
src += handle.c
src += handle.h
src += if.c
src += io.c
src += io.h
src += poll.c
if PLATFORM_LINUX
src += rtnl.c
src += rtnl.h
endif
src += serial.c
src += sock.c
if PLATFORM_WIN32
src += version.rc
endif

lib_LTLIBRARIES = liblely-io.la
liblely_io_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_io_la_CPPFLAGS += $(liblely_libc_CFLAGS)
liblely_io_la_CPPFLAGS += $(liblely_util_CFLAGS)
if !NO_CAN
liblely_io_la_CPPFLAGS += $(liblely_can_CFLAGS)
endif
if CODE_COVERAGE_ENABLED
liblely_io_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_io_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_io_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_io_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_io_la_LDFLAGS += -Wl,--output-def,$(PACKAGE)-$(VERSION_MAJOR).def
endif
liblely_io_la_LIBADD = $(liblely_libc_LIBS)
liblely_io_la_LIBADD += $(liblely_util_LIBS)
if !NO_CAN
liblely_io_la_LIBADD += $(liblely_can_LIBS)
endif
if CODE_COVERAGE_ENABLED
liblely_io_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_io_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = $(PACKAGE)-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
