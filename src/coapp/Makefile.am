src =
src += coapp.hpp
src += device.cpp
if !NO_COAPP_MASTER
src += driver.cpp
src += fiber_driver.cpp
src += logical_driver.cpp
src += loop_driver.cpp
if !NO_CO_LSS
src += lss_master.cpp
endif
src += master.cpp
endif
src += node.cpp
src += sdo.cpp
src += sdo_error.cpp
if !NO_COAPP_SLAVE
src += slave.cpp
endif
src += type_traits.cpp

lib_LTLIBRARIES = liblely-coapp.la
liblely_coapp_la_CPPFLAGS = -I$(top_srcdir)/include
if CODE_COVERAGE_ENABLED
liblely_coapp_la_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif
liblely_coapp_la_CXXFLAGS =
if CODE_COVERAGE_ENABLED
liblely_coapp_la_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif
liblely_coapp_la_LDFLAGS = -no-undefined -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_PATCH)
if PLATFORM_WIN32
liblely_coapp_la_LDFLAGS += -Wl,--output-def,liblely-coapp-$(VERSION_MAJOR).def
endif
liblely_coapp_la_LDFLAGS += -Wl,--as-needed
liblely_coapp_la_LIBADD =
liblely_coapp_la_LIBADD += $(top_builddir)/src/libc/liblely-libc.la
liblely_coapp_la_LIBADD += $(top_builddir)/src/util/liblely-util.la
liblely_coapp_la_LIBADD += $(top_builddir)/src/can/liblely-can.la
liblely_coapp_la_LIBADD += $(top_builddir)/src/ev/liblely-ev.la
liblely_coapp_la_LIBADD += $(top_builddir)/src/io2/liblely-io2.la
liblely_coapp_la_LIBADD += $(top_builddir)/src/co/liblely-co.la
if CODE_COVERAGE_ENABLED
liblely_coapp_la_LIBADD += $(CODE_COVERAGE_LIBS)
endif
liblely_coapp_la_SOURCES = $(src)
if PLATFORM_WIN32
nodist_liblely_coapp_la_SOURCES = version.rc
endif

if PLATFORM_WIN32
.rc.lo:
	$(AM_V_GEN) $(LIBTOOL) --silent --tag=RC --mode=compile $(RC) $< -o $@
if ENABLE_SHARED
defdir = $(libdir)
def_DATA = liblely-coapp-$(VERSION_MAJOR).def
MOSTLYCLEANFILES = $(def_DATA)
endif
endif
