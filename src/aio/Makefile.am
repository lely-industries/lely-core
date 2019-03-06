src =
if !NO_CXX
src += cxx/aio.hpp
src += cxx/can_bus.cpp
src += cxx/context.cpp
src += cxx/exec.cpp
src += cxx/future.cpp
src += cxx/loop.cpp
src += cxx/poll.cpp
src += cxx/reactor.cpp
src += cxx/strand.cpp
src += cxx/timer.cpp
endif
if PLATFORM_LINUX
src += linux/aio.c
src += linux/can_bus.c
src += linux/reactor.c
src += linux/self_pipe.c
src += linux/timer.c
endif
src += aio.h
src += can_bus.c
src += context.c
src += exec.c
src += loop.c
src += poll.c
src += queue.c
src += reactor.c
src += strand.c
src += timer.c

lib_LTLIBRARIES = liblely-aio.la
liblely_aio_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_aio_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_aio_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_aio_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_aio_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_aio_la_LDFLAGS += -Wl,--output-def,liblely-aio-$(VERSION_MAJOR).def
endif
liblely_aio_la_LDFLAGS += -Wl,--as-needed
liblely_aio_la_LIBADD =
liblely_aio_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
liblely_aio_la_LIBADD += $(top_builddir)/src/util/liblely-util.la
liblely_aio_la_LIBADD += $(top_builddir)/src/can/liblely-can.la
if CODE_COVERAGE_ENABLED
liblely_aio_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_aio_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_aio_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-aio-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
