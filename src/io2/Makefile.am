src =
if PLATFORM_LINUX
src += linux/io.h
src += linux/poll.c
src += linux/timer.c
endif
if PLATFORM_POSIX
src += posix/io.c
src += posix/io.h
if !PLATFORM_LINUX
src += posix/poll.c
src += posix/timer.c
endif
endif
src += sys/clock.c
if PLATFORM_WIN32
src += win32/io.c
src += win32/io.h
src += win32/poll.c
src += win32/timer.c
endif
src += clock.c
src += ctx.c
src += dev.c
src += io2.h
src += timer.c
src += timer.h

lib_LTLIBRARIES = liblely-io2.la
liblely_io2_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_io2_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_io2_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_io2_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_io2_la_CXXFLAGS =
if CODE_COVERAGE_ENABLED
liblely_io2_la_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif
liblely_io2_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_io2_la_LDFLAGS += -Wl,--output-def,liblely-io2-$(VERSION_MAJOR).def
endif
liblely_io2_la_LDFLAGS += -Wl,--as-needed
liblely_io2_la_LIBADD =
liblely_io2_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
liblely_io2_la_LIBADD += $(top_builddir)/src/util/liblely-util.la
liblely_io2_la_LIBADD += $(top_builddir)/src/ev/liblely-ev.la
liblely_io2_la_LIBADD += $(RT_LIBS)
if CODE_COVERAGE_ENABLED
liblely_io2_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_io2_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_io2_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-io2-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
