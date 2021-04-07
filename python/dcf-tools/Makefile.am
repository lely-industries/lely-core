src =

src += dcf/__init__.py
src += dcf/cli.py
src += dcf/device.py
src += dcf/lint.py
src += dcf/parse.py
src += dcf/print.py

src += dcf2dev/data/dev.c.em
src += dcf2dev/data/dev.h.em
src += dcf2dev/__init__.py
src += dcf2dev/cdevice.py
src += dcf2dev/cli.py

src += dcfgen/data/master.dcf.em
src += dcfgen/__init__.py
src += dcfgen/cli.py

EXTRA_DIST = $(src)
EXTRA_DIST += setup.py

build_base = $(realpath $(builddir))/build
dist_dir = $(realpath $(builddir))/dist

all-local: python-sdist python-bdist_wheel

clean-local:
	rm -rf $(build_base) $(dist_dir) $(srcdir)/*.egg-info $(builddir)/*.egg-info

install-exec-local: python-install

.PHONY: python-bdist_wheel
python-bdist_wheel:
if HAVE_PYTHON3
	@$(PYTHON3_ENV) $(PYTHON3) $(srcdir)/setup.py \
		bdist_wheel --dist-dir $(dist_dir)
endif

.PHONY: python-install
python-install:
if HAVE_PYTHON3
	@$(PYTHON3_ENV) $(PYTHON3) $(srcdir)/setup.py \
		install --prefix $(DESTDIR)$(prefix) --root /
endif

.PHONY: python-sdist
python-sdist:
if HAVE_PYTHON3
	@cd $(srcdir); $(PYTHON3_ENV) $(PYTHON3) setup.py \
		sdist --dist-dir $(dist_dir)
endif
