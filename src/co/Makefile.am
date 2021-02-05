src =
src += co.h
src += crc.c
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
if !NO_CO_GW
src += gw.c
endif
if !NO_CO_GW_TXT
src += gw_txt.c
endif
if !NO_CO_LSS
src += lss.c
endif
src += nmt.c
if !NO_CO_NMT_BOOT
src += nmt_boot.c
src += nmt_boot.h
endif
if !NO_CO_NMT_CFG
src += nmt_cfg.c
src += nmt_cfg.h
endif
src += nmt_hb.c
src += nmt_hb.h
src += nmt_srv.c
src += nmt_srv.h
src += obj.c
src += pdo.c
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
if !NO_CO_WTM
src += wtm.c
endif

lib_LTLIBRARIES = liblely-co.la
liblely_co_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_co_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_co_la_CFLAGS =
if CODE_COVERAGE_ENABLED
liblely_co_la_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif
liblely_co_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_co_la_LDFLAGS += -Wl,--output-def,liblely-co-$(VERSION_MAJOR).def
endif
liblely_co_la_LDFLAGS += -Wl,--as-needed
liblely_co_la_LIBADD =
liblely_co_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
liblely_co_la_LIBADD += $(top_builddir)/src/util/liblely-util.la
liblely_co_la_LIBADD += $(top_builddir)/src/can/liblely-can.la
if CODE_COVERAGE_ENABLED
liblely_co_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_co_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_co_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-co-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
