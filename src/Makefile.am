src =
src += libc.h
if PLATFORM_WIN32
src += libc.rc
endif
src += time.c

lib_LTLIBRARIES = liblely-libc.la
liblely_libc_la_CPPFLAGS = -I$(top_srcdir)/include
liblely_libc_la_LDFLAGS = -version-number 1:0:0 -no-undefined
liblely_libc_la_SOURCES = $(src)

.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
