src =
src += libc.h
src += stdio.c
src += stdlib.c
src += string.c
src += strings.c
if !NO_THREADS
src += threads.c
endif
src += time.c
if PLATFORM_WIN32
src += version.rc
endif

lib_LTLIBRARIES = liblely-libc.la
liblely_libc_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_libc_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_libc_la_LDFLAGS += -Wl,--output-def,$(PACKAGE)-$(VERSION_MAJOR).def
endif
liblely_libc_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@

defdir = $(libdir)
def_DATA = $(PACKAGE)-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
