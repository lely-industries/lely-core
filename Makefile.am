ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc include src test
if HAVE_TAP
SUBDIRS += test
endif

dist =
dist += LICENSE
dist += NOTICE
dist += README.md

EXTRA_DIST = $(dist)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE).pc
