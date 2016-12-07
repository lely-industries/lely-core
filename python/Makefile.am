pkg = lely_can

src = 
src += $(pkg)/__init__.pxd
src += $(pkg)/__init__.py
src += $(pkg)/can.pxd
src += $(pkg)/can.pyx
src += $(pkg)/msg.pxd
src += $(pkg)/msg.pyx

EXTRA_DIST = $(src)
EXTRA_DIST += setup.py

AM_CPPFLAGS = -I$(abs_top_srcdir)/include
AM_CPPFLAGS += $(liblely_util_CFLAGS)
AM_CPPFLAGS += $(liblely_libc_CFLAGS)

AM_CFLAGS = -shared -Wno-pedantic

AM_LDFLAGS = -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-z,relro
AM_LDFLAGS += -L$(abs_top_builddir)/src/.libs

LIBS += $(liblely_util_LIBS)
LIBS += $(liblely_libc_LIBS)

CYTHON_ENV = \
	CC="$(CC)" \
	CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" \
	CFLAGS="$(CFLAGS) $(AM_CFLAGS)" \
	LDSHARED="$(CC)" \
	LDFLAGS="$(CFLAGS) $(AM_CFLAGS) $(LDFLAGS) $(AM_LDFLAGS) $(LIBS)"

build_base = $(realpath $(builddir))/build

all-local:
if HAVE_PYTHON2
	cd $(srcdir); \
	$(CYTHON_ENV) $(PYTHON2) setup.py build --build-base $(build_base)
endif
if HAVE_PYTHON3
	cd $(srcdir); \
	$(CYTHON_ENV) $(PYTHON3) setup.py build --build-base $(build_base)
endif

clean-local:
	rm -rf $(builddir)/build

distclean-local:
	rm -rf $(srcdir)/$(pkg)/*.c

install-exec-local:
if HAVE_PYTHON2
	$(PYTHON2) $(srcdir)/setup.py install --prefix $(DESTDIR)$(prefix)
endif
if HAVE_PYTHON3
	$(PYTHON3) $(srcdir)/setup.py install --prefix $(DESTDIR)$(prefix)
endif
