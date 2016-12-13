pkg = lely_io

src = 
src += $(pkg)/__init__.pxd
src += $(pkg)/__init__.py
src += $(pkg)/addr.pxd
src += $(pkg)/addr.pyx
src += $(pkg)/attr.pxd
src += $(pkg)/attr.pyx
src += $(pkg)/can.pxd
src += $(pkg)/can.pyx
src += $(pkg)/file.pxd
src += $(pkg)/file.pyx
src += $(pkg)/io.pxd
src += $(pkg)/io.pyx
src += $(pkg)/poll.pxd
src += $(pkg)/poll.pyx
src += $(pkg)/serial.pxd
src += $(pkg)/serial.pyx
src += $(pkg)/sock.pxd
src += $(pkg)/sock.pyx

EXTRA_DIST = $(src)
EXTRA_DIST += setup.py

AM_CPPFLAGS = -I$(abs_top_srcdir)/include
AM_CPPFLAGS += $(liblely_util_CFLAGS)
AM_CPPFLAGS += $(liblely_libc_CFLAGS)

AM_CFLAGS = -shared -Wno-pedantic -Wno-strict-prototypes

AM_LDFLAGS = -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-z,relro
AM_LDFLAGS += -L$(abs_top_builddir)/src/.libs

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
