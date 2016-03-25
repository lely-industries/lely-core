src =
src += co.h
if PLATFORM_WIN32
src += co.rc
endif
if !NO_CO_CSDO
src += csdo.c
endif
if !NO_CO_DCF
src += dcf.c
endif
src += dev.c
if !NO_CO_EMCY
src += emcy.c
endif
src += nmt.c
if !NO_CO_NMT_MASTER
src += nmt_boot.c
src += nmt_boot.h
endif
src += nmt_ec.c
src += nmt_ec.h
src += nmt_srv.c
src += nmt_srv.h
src += obj.c
src += obj.h
if !NO_CO_RPDO
src += rpdo.c
endif
if !NO_CO_SDEV
src += sdev.c
endif
src += sdo.c
src += sdo.h
src += ssdo.c
if !NO_CO_SYNC
src += sync.c
endif
if !NO_CO_TIME
src += time.c
endif
if !NO_CO_TPDO
src += tpdo.c
endif
src += type.c
src += val.c

lib_LTLIBRARIES = liblely-co.la
liblely_co_la_CPPFLAGS = -I$(top_srcdir)/include $(liblely_libc_CFLAGS) $(liblely_util_CFLAGS) $(liblely_can_CFLAGS)
liblely_co_la_LDFLAGS = -version-number 1:0:0 -no-undefined
liblely_co_la_LIBADD = $(liblely_libc_LIBS) $(liblely_util_LIBS) $(liblely_can_LIBS)
liblely_co_la_SOURCES = $(src)

.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
