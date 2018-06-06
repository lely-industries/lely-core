EXTRA_DIST =
EXTRA_DIST += can/overview.md
EXTRA_DIST += co/can2udp.md
EXTRA_DIST += co/cocat.md
EXTRA_DIST += co/coctl.md
EXTRA_DIST += co/dcf2c.md
EXTRA_DIST += co/fsm.md
EXTRA_DIST += co/overview.md
EXTRA_DIST += co/standards.md
EXTRA_DIST += io/overview.md
EXTRA_DIST += libc/overview.md
EXTRA_DIST += util/callback.md
EXTRA_DIST += util/object.md
EXTRA_DIST += util/overview.md
EXTRA_DIST += logo.png

if HAVE_DOXYGEN

html-local: Doxyfile
	$(DOXYGEN)

install-data-local: install-html-local

install-html-local: html-local
	if test -e html/index.html; then \
		$(INSTALL) -d $(DESTDIR)$(htmldir)/html; \
		$(INSTALL_DATA) -t $(DESTDIR)$(htmldir)/html html/*; \
	fi

clean-local:
	rm -rf doxygen_sqlite3.db html

uninstall-local:
	if test -d $(DESTDIR)$(htmldir)/html; then \
		rm -rf $(DESTDIR)$(htmldir)/html; \
	fi

endif
