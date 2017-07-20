src =
src += lely_can/__init__.pxd
src += lely_can/__init__.py
src += lely_can/can.pxd
src += lely_can/can.pyx
src += lely_can/msg.pxd
src += lely_can/msg.pyx

EXTRA_DIST = $(src)
EXTRA_DIST += setup.py

build_base = $(realpath $(builddir))/build

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
	LDFLAGS="$(LDFLAGS) $(AM_LDFLAGS)"

CLEANFILES = $(patsubst %.pyx,$(srcdir)/%.c,$(filter %.pyx,$(src)))

all-local:
if HAVE_PYTHON2
	@cd $(srcdir); \
	$(CYTHON_ENV) $(PYTHON2) setup.py build --build-base $(build_base)
endif
if HAVE_PYTHON3
	@cd $(srcdir); \
	$(CYTHON_ENV) $(PYTHON3) setup.py build --build-base $(build_base)
endif

clean-local:
	rm -rf $(build_base)

install-exec-local:
if HAVE_PYTHON2
	@$(PYTHON2) $(srcdir)/setup.py install --prefix $(DESTDIR)$(prefix) \
		--root /
endif
if HAVE_PYTHON3
	@$(PYTHON3) $(srcdir)/setup.py install --prefix $(DESTDIR)$(prefix) \
		--root /
endif
