src =
src += bimap.c
src += bits.c
src += bitset.c
src += cmp.c
src += config.c
src += config_ini.c
if !NO_DAEMON
src += daemon.c
endif
src += diag.c
src += dllist.c
src += endian.c
src += errnum.c
if !NO_CXX
src += exception.cpp
endif
if PLATFORM_WIN32
src += fiber-win32.c
else
src += fiber-sjlj.c
endif
src += fiber.h
src += frbuf.c
src += fwbuf.c
src += lex.c
src += membuf.c
src += mkjmp.c
src += pheap.c
src += print.c
src += rbtree.c
src += sllist.c
src += spscring.c
src += time.c
src += util.h

lib_LTLIBRARIES = liblely-util.la
liblely_util_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_util_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_util_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_util_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_util_la_CXXFLAGS =
if CODE_COVERAGE_ENABLED
liblely_util_la_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif
liblely_util_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_util_la_LDFLAGS += -Wl,--output-def,liblely-util-$(VERSION_MAJOR).def
endif
liblely_util_la_LDFLAGS += -Wl,--as-needed
liblely_util_la_LIBADD =
liblely_util_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
if CODE_COVERAGE_ENABLED
liblely_util_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_util_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_util_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-util-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
