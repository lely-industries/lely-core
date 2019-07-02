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
src += $(pkg)/pipe.pxd
src += $(pkg)/pipe.pyx
src += $(pkg)/poll.pxd
src += $(pkg)/poll.pyx
src += $(pkg)/serial.pxd
src += $(pkg)/serial.pyx
src += $(pkg)/sock.pxd
src += $(pkg)/sock.pyx

EXTRA_DIST = $(src)
EXTRA_DIST += setup.py

CLEANFILES = $(patsubst %.pyx,$(srcdir)/%.c,$(filter %.pyx,$(src)))

build_base = $(realpath $(builddir))/build
dist_dir = $(realpath $(builddir))/dist

AM_CPPFLAGS = -I$(abs_top_srcdir)/include

AM_CFLAGS = -shared -Wno-pedantic

AM_LDFLAGS = -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-z,relro
AM_LDFLAGS += -L$(abs_top_builddir)/src/io/.libs

CYTHON_ENV = \
	CC="$(CC)" \
	CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" \
	CFLAGS="$(CFLAGS) $(AM_CFLAGS) -Wno-strict-prototypes -Wno-implicit-fallthrough -Wno-cast-function-type" \
	LDSHARED="$(CC)" \
	LDFLAGS="$(LDFLAGS) $(AM_LDFLAGS)"
CYTHON2_ENV = $(PYTHON2_ENV) $(CYTHON_ENV)
CYTHON3_ENV = $(PYTHON3_ENV) $(CYTHON_ENV)

all-local: python-build python-sdist python-bdist_wheel

clean-local:
	rm -rf $(build_base) $(dist_dir) $(srcdir)/*.egg-info $(builddir)/*.egg-info

install-exec-local: python-install

.PHONY: python-bdist_wheel
python-bdist_wheel: python-build
if HAVE_PYTHON2
	@$(PYTHON2_ENV) $(PYTHON2) $(srcdir)/setup.py \
		bdist_wheel --dist-dir $(dist_dir)
endif
if HAVE_PYTHON3
	@$(PYTHON3_ENV) $(PYTHON3) $(srcdir)/setup.py \
		bdist_wheel --dist-dir $(dist_dir)
endif

.PHONY: python-build
python-build:
if HAVE_PYTHON2
	@cd $(srcdir); $(CYTHON2_ENV) $(PYTHON2) setup.py \
		build --build-base $(build_base)
endif
if HAVE_PYTHON3
	@cd $(srcdir); $(CYTHON3_ENV) $(PYTHON3) setup.py \
		build --build-base $(build_base)
endif

.PHONY: python-install
python-install: python-build
if HAVE_PYTHON2
	@$(PYTHON2_ENV) $(PYTHON2) $(srcdir)/setup.py \
		install --prefix $(DESTDIR)$(prefix) --root /
endif
if HAVE_PYTHON3
	@$(PYTHON3_ENV) $(PYTHON3) $(srcdir)/setup.py \
		install --prefix $(DESTDIR)$(prefix) --root /
endif

.PHONY: python-sdist
python-sdist:
if HAVE_PYTHON2
	@cd $(srcdir); $(PYTHON2_ENV) $(PYTHON2) setup.py \
		sdist --dist-dir $(dist_dir)
endif
if HAVE_PYTHON3
	@cd $(srcdir); $(PYTHON3_ENV) $(PYTHON3) setup.py \
		sdist --dist-dir $(dist_dir)
endif
