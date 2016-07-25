ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc include src tools
if HAVE_TAP
SUBDIRS += test
endif

EXTRA_DIST =
EXTRA_DIST += LICENSE
EXTRA_DIST += NOTICE
EXTRA_DIST += README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE).pc
