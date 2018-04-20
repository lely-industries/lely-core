src =
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
liblely_aio_la_CPPFLAGS += $(liblely_libc_CFLAGS)
liblely_aio_la_CPPFLAGS += $(liblely_util_CFLAGS)
liblely_aio_la_CPPFLAGS += $(liblely_can_CFLAGS)
liblely_aio_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_aio_la_LDFLAGS += -Wl,--output-def,liblely-aio-$(VERSION_MAJOR).def
endif
liblely_aio_la_LDFLAGS += -Wl,--as-needed
liblely_aio_la_LIBADD = $(liblely_libc_LIBS)
liblely_aio_la_LIBADD += $(liblely_util_LIBS)
liblely_aio_la_LIBADD += $(liblely_can_LIBS)
liblely_aio_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-aio-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
