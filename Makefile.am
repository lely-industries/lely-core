ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc include src

EXTRA_DIST =
EXTRA_DIST += LICENSE
EXTRA_DIST += NOTICE
EXTRA_DIST += README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE).pc
if !NO_RT
pkgconfig_DATA += $(PACKAGE)_rt.pc
endif
