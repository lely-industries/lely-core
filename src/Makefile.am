src =
src += co.h
if PLATFORM_WIN32
src += co.rc
endif

lib_LTLIBRARIES = liblely-co.la
liblely_co_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS) $(liblely_util_CFLAGS) $(liblely_co_CFLAGS)
liblely_co_la_LDFLAGS = -version-number 1:0:0 -no-undefined
liblely_co_la_LIBADD = $(liblely_libc_LIBS) $(liblely_util_LIBS) $(liblely_co_LIBS)
liblely_co_la_SOURCES = $(src)

.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
