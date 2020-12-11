src =
src += ev.h
src += exec.c
if !NO_MALLOC
src += fiber_exec.c
src += future.c
src += loop.c
endif
src += poll.c
if !NO_MALLOC
src += std_exec.c
src += strand.c
endif
src += task.c
src += thrd_loop.c

lib_LTLIBRARIES = liblely-ev.la
liblely_ev_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_ev_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_ev_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_ev_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_ev_la_CXXFLAGS =
if CODE_COVERAGE_ENABLED
liblely_ev_la_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif
liblely_ev_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_ev_la_LDFLAGS += -Wl,--output-def,liblely-ev-$(VERSION_MAJOR).def
endif
liblely_ev_la_LDFLAGS += -Wl,--as-needed
liblely_ev_la_LIBADD =
liblely_ev_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
liblely_ev_la_LIBADD += $(top_builddir)/src/util/liblely-util.la
if CODE_COVERAGE_ENABLED
liblely_ev_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_ev_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_ev_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-ev-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
