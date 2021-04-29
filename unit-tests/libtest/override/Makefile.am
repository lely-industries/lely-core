
lib_LTLIBRARIES =

# Override test plugin

if !PLATFORM_WIN32
liblely_override_plugin_src =
liblely_override_plugin_src += override-test-plugin.cpp

lib_LTLIBRARIES += liblely-override-plugin.la

liblely_override_plugin_la_CPPFLAGS = -I$(top_srcdir)/include
if ENABLE_SHARED
liblely_override_plugin_la_CPPFLAGS += -DLELY_ENABLE_SHARED=1
endif
liblely_override_plugin_la_SOURCES = $(liblely_override_plugin_src)
endif

# liblelyco-val override

if !PLATFORM_WIN32
liblelyco_val_override_src =
liblelyco_val_override_src += lelyco-val.cpp

lib_LTLIBRARIES += liblelyco-val-override.la

liblelyco_val_override_la_LDFLAGS =
liblelyco_val_override_la_LDFLAGS += -Wl,--as-needed
liblelyco_val_override_la_LIBADD =
liblelyco_val_override_la_LIBADD += -ldl

liblelyco_val_override_la_CPPFLAGS = -I$(top_srcdir)/include
if ENABLE_SHARED
liblelyco_val_override_la_CPPFLAGS += -DLELY_ENABLE_SHARED=1
endif
liblelyco_val_override_la_SOURCES = $(liblelyco_val_override_src)
endif

# liblelyutil-membuf override

if !PLATFORM_WIN32
liblelyutil_membuf_override_src =
liblelyutil_membuf_override_src += lelyutil-membuf.cpp

lib_LTLIBRARIES += liblelyutil-membuf-override.la

liblelyutil_membuf_override_la_LDFLAGS =
liblelyutil_membuf_override_la_LDFLAGS += -Wl,--as-needed
liblelyutil_membuf_override_la_LIBADD =
liblelyutil_membuf_override_la_LIBADD += -ldl

liblelyutil_membuf_override_la_CPPFLAGS = -I$(top_srcdir)/include
if ENABLE_SHARED
liblelyutil_membuf_override_la_CPPFLAGS += -DLELY_ENABLE_SHARED=1
endif
liblelyutil_membuf_override_la_SOURCES = $(liblelyutil_membuf_override_src)
endif

# Compilation flags

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CPPFLAGS += -DTEST_SRCDIR=\"${srcdir}\"
if ENABLE_SHARED
AM_CPPFLAGS += -DLELY_ENABLE_SHARED=1
endif
AM_CPPFLAGS += $(CPPUTEST_CFLAGS)

if NO_UNIT_TESTS_MEMLEAK
AM_CPPFLAGS += -DCPPUTEST_MEM_LEAK_DETECTION_DISABLED
endif

if CODE_COVERAGE_ENABLED
AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
endif

AM_CFLAGS =
if CODE_COVERAGE_ENABLED
AM_CFLAGS += $(CODE_COVERAGE_CFLAGS)
endif

AM_CXXFLAGS =
if CODE_COVERAGE_ENABLED
AM_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)
endif

AM_LDFLAGS =
if PLATFORM_WIN32
AM_LDFLAGS += -all-static
endif

if CODE_COVERAGE_ENABLED
LOG_COMPILER = $(SHELL) $(top_builddir)/lcov-wrapper.sh
AM_TESTS_ENVIRONMENT = EXEC_WRAPPER="$(EXEC)"
AM_TESTS_ENVIRONMENT += ;
@CODE_COVERAGE_RULES@
else
LOG_COMPILER = $(EXEC)
endif

@VALGRIND_CHECK_RULES@
