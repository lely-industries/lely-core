ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc include src python test

EXTRA_DIST =
EXTRA_DIST += LICENSE
EXTRA_DIST += NOTICE
EXTRA_DIST += README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =
pkgconfig_DATA += liblely-can.pc
pkgconfig_DATA += liblely-io.pc
pkgconfig_DATA += liblely-libc.pc
if !NO_RT
pkgconfig_DATA += liblely-libc_rt.pc
endif
pkgconfig_DATA += liblely-tap.pc
pkgconfig_DATA += liblely-util.pc

if CODE_COVERAGE_ENABLED
@CODE_COVERAGE_RULES@
CODE_COVERAGE_OUTPUT_FILE = coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = coverage
CODE_COVERAGE_BRANCH_COVERAGE = 1
CODE_COVERAGE_GENHTML_OPTIONS = $(CODE_COVERAGE_GENHTML_OPTIONS_DEFAULT)
CODE_COVERAGE_GENHTML_OPTIONS += -p $(realpath $(abs_top_srcdir))
CODE_COVERAGE_IGNORE_PATTERN = "$(realpath $(abs_top_srcdir))/test/*"
endif
