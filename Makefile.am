ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc include src python pkgconfig
if !NO_TOOLS
SUBDIRS += tools
endif
if !NO_TESTS
SUBDIRS += test
endif
if !NO_UNIT_TESTS
SUBDIRS += unit-tests
endif

EXTRA_DIST =
EXTRA_DIST += CPPLINT.cfg
EXTRA_DIST += LICENSE
EXTRA_DIST += NOTICE
EXTRA_DIST += README.md
EXTRA_DIST += suppressions.txt
EXTRA_DIST += .clang-format.c
EXTRA_DIST += .clang-format.cpp
EXTRA_DIST += .cquery
EXTRA_DIST += .gcc-flags.json

if CODE_COVERAGE_ENABLED
@CODE_COVERAGE_RULES@
CODE_COVERAGE_OUTPUT_FILE = coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = coverage
CODE_COVERAGE_BRANCH_COVERAGE = 1
CODE_COVERAGE_GENHTML_OPTIONS = $(CODE_COVERAGE_GENHTML_OPTIONS_DEFAULT)
CODE_COVERAGE_GENHTML_OPTIONS += -p $(realpath $(abs_top_srcdir))
CODE_COVERAGE_GENHTML_OPTIONS += --show-details
CODE_COVERAGE_GENHTML_OPTIONS += --rc genhtml_hi_limit=100
CODE_COVERAGE_GENHTML_OPTIONS += --rc genhtml_med_limit=80
CODE_COVERAGE_IGNORE_PATTERN = "/usr/*" "$(realpath $(abs_top_srcdir))/test/*" "$(realpath $(abs_top_srcdir))/unit-tests/*"
# some options are duplicated in lcov-wrapper.sh.in
endif
