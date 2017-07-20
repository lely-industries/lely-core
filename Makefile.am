ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc include src python
if HAVE_TAP
SUBDIRS += test
endif

dist =
dist += LICENSE
dist += NOTICE
dist += README.md

EXTRA_DIST = $(dist)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lib$(PACKAGE).pc

if CODE_COVERAGE_ENABLED
@CODE_COVERAGE_RULES@
CODE_COVERAGE_OUTPUT_FILE = coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = coverage
CODE_COVERAGE_BRANCH_COVERAGE = 1
CODE_COVERAGE_GENHTML_OPTIONS = $(CODE_COVERAGE_GENHTML_OPTIONS_DEFAULT)
CODE_COVERAGE_GENHTML_OPTIONS += -p $(realpath $(abs_top_srcdir))
CODE_COVERAGE_IGNORE_PATTERN = "/usr/*" "$(realpath $(abs_top_srcdir))/test/*"
endif
