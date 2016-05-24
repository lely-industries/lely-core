src =
src += buf.c
src += can.h
if PLATFORM_WIN32
src += can.win32.rc
endif
src += msg.c
src += net.c
if HAVE_SOCKET_CAN
src += socket.c
endif

lib_LTLIBRARIES = liblely-can.la
liblely_can_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS) $(liblely_util_CFLAGS)
liblely_can_la_LDFLAGS = -version-number 1:0:0 -no-undefined
liblely_can_la_LIBADD = $(liblely_libc_LIBS) $(liblely_util_LIBS)
liblely_can_la_SOURCES = $(src)

.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
