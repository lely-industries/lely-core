src =
src += buf.c
src += can.h
if HAVE_IXXAT_VCI3
src += ixxat.c
endif
src += msg.c
src += net.c
if HAVE_SOCKET_CAN
src += socket.c
endif
if PLATFORM_WIN32
src += version.rc
endif

lib_LTLIBRARIES = liblely-can.la
liblely_can_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_can_la_CPPFLAGS += $(liblely_libc_CFLAGS)
liblely_can_la_CPPFLAGS += $(liblely_util_CFLAGS)
liblely_can_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_can_la_LDFLAGS += -Wl,--output-def,$(PACKAGE)-$(VERSION_MAJOR).def
endif
liblely_can_la_LIBADD = $(liblely_libc_LIBS)
liblely_can_la_LIBADD += $(liblely_util_LIBS)
liblely_can_la_SOURCES = $(src)

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = $(PACKAGE)-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
