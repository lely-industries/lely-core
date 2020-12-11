src =
if !ECSS_COMPLIANCE
src += bimap.c
src += mkjmp.c
src += sllist.c
src += spscring.c
endif # !ECSS_COMPLIANCE

src += bits.c
if !NO_MALLOC
src += bitset.c
endif
src += cmp.c
if !NO_MALLOC
src += config.c
if !NO_STDIO
src += config_ini.c
endif
endif # !NO_MALLOC
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
if !NO_MALLOC
src += fiber.c
endif
if !NO_STDIO
src += frbuf.c
src += fwbuf.c
endif
if !NO_STDIO
src += lex.c
endif
src += membuf.c
src += pheap.c
if !NO_STDIO
src += print.c
endif
src += rbtree.c
if !NO_MALLOC
src += stop.c
endif
src += time.c
src += ustring.c
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
if !ECSS_COMPLIANCE
liblely_util_la_LIBADD += -lm
endif
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
