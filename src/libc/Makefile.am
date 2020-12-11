src =
if !NO_RT
src += clock.c
endif
src += getopt.c
src += libc.h
src += stdatomic.c
if !NO_STDIO
src += stdio.c
endif
if !NO_MALLOC
if !NO_STDIO
src += stdlib.c
endif
endif
src += string.c
src += strings.c
if !NO_RT
src += sleep.c
endif
if !NO_THREADS
if HAVE_PTHREAD
src += threads-pthread.c
endif
if PLATFORM_WIN32
src += threads-win32.c
endif
endif
if !NO_RT
src += time.c
endif

lib_LTLIBRARIES = liblely-libc.la
liblely_libc_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_libc_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_libc_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_libc_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_libc_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_libc_la_LDFLAGS += -Wl,--output-def,liblely-libc-$(VERSION_MAJOR).def
endif
liblely_libc_la_LDFLAGS += -Wl,--as-needed
liblely_libc_la_LIBADD = $(RT_LIBS)
if CODE_COVERAGE_ENABLED
liblely_libc_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_libc_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_libc_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-libc-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
