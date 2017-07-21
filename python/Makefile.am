src =
src += lely_io/__init__.pxd
src += lely_io/__init__.py
src += lely_io/addr.pxd
src += lely_io/addr.pyx
src += lely_io/attr.pxd
src += lely_io/attr.pyx
src += lely_io/can.pxd
src += lely_io/can.pyx
src += lely_io/file.pxd
src += lely_io/file.pyx
src += lely_io/io.pxd
src += lely_io/io.pyx
src += lely_io/pipe.pxd
src += lely_io/pipe.pyx
src += lely_io/poll.pxd
src += lely_io/poll.pyx
src += lely_io/serial.pxd
src += lely_io/serial.pyx
src += lely_io/sock.pxd
src += lely_io/sock.pyx

EXTRA_DIST = $(src)
EXTRA_DIST += setup.py

build_base = $(realpath $(builddir))/build

AM_CPPFLAGS = -I$(abs_top_srcdir)/include
AM_CPPFLAGS += $(liblely_can_CFLAGS)
AM_CPPFLAGS += $(liblely_util_CFLAGS)
AM_CPPFLAGS += $(liblely_libc_CFLAGS)

AM_CFLAGS = -shared -Wno-pedantic

AM_LDFLAGS = -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-z,relro
AM_LDFLAGS += -L$(abs_top_builddir)/src/.libs

LIBS += $(liblely_can_LIBS)
LIBS += $(liblely_util_LIBS)
LIBS += $(liblely_libc_LIBS)

CYTHON_ENV = \
	CC="$(CC)" \
	CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" \
	CFLAGS="$(CFLAGS) $(AM_CFLAGS) -Wno-strict-prototypes" \
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
	rm -rf *.egg-info

install-exec-local:
if HAVE_PYTHON2
	@$(PYTHON2) $(srcdir)/setup.py install --prefix $(DESTDIR)$(prefix) \
		--root /
endif
if HAVE_PYTHON3
	@$(PYTHON3) $(srcdir)/setup.py install --prefix $(DESTDIR)$(prefix) \
		--root /
endif
