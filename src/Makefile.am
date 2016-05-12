src =
src += tap.c
src += tap.h
if PLATFORM_WIN32
src += tap.rc
endif

lib_LTLIBRARIES = liblely-tap.la
liblely_tap_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS)
liblely_tap_la_LDFLAGS = -version-number 1:0:0 -no-undefined
liblely_tap_la_LIBADD = $(liblely_libc_LIBS)
liblely_tap_la_SOURCES = $(src)

.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
