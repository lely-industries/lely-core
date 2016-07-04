src =
if !NO_CXX
src += c_type.cpp
endif
src += config.c
src += config_ini.c
src += diag.c
src += errnum.c
if !NO_CXX
src += exception.cpp
endif
src += fbuf.c
src += lex.c
src += membuf.c
src += page.h
src += pheap.c
src += pool.c
src += print.c
src += rbtree.c
src += unicode.h
src += util.h
if PLATFORM_WIN32
src += version.rc
endif

lib_LTLIBRARIES = liblely-util.la
liblely_util_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_util_la_CPPFLAGS += $(liblely_libc_CFLAGS)
liblely_util_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_util_la_LDFLAGS += -Wl,--output-def,$(PACKAGE)-$(VERSION_MAJOR).def
endif
liblely_util_la_LIBADD = $(liblely_libc_LIBS)
liblely_util_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = $(PACKAGE)-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
