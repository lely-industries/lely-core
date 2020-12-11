src =
src += buf.c
src += can.h
src += msg.c
src += net.c
if !NO_STDIO
if HAVE_SOCKET_CAN
src += socket.c
endif
if HAVE_VCI
src += vci.c
endif
endif # !NO_STDIO
if PLATFORM_WIN32
src += version.rc
endif

lib_LTLIBRARIES = liblely-can.la
liblely_can_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_can_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_can_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_can_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_can_la_LDFLAGS = -no-undefined -version-number 1:9:2
if PLATFORM_WIN32
liblely_can_la_LDFLAGS += -Wl,--output-def,liblely-can-1.def
endif
liblely_can_la_LDFLAGS += -Wl,--as-needed
liblely_can_la_LIBADD =
liblely_can_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
liblely_can_la_LIBADD += $(top_builddir)/src/util/liblely-util.la
if CODE_COVERAGE_ENABLED
liblely_can_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_can_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-can-1.def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
