src =
src += bitset.c
src += config.c
src += config_ini.c
src += diag.c
src += errnum.c
src += fbuf.c
src += idlist.c
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
src += util.win32.rc
endif

lib_LTLIBRARIES = liblely-util.la
liblely_util_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS)
liblely_util_la_LDFLAGS = -version-number 1:0:0 -no-undefined
liblely_util_la_LIBADD = $(liblely_libc_LIBS)
liblely_util_la_SOURCES = $(src)

.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
