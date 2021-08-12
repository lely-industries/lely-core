if HAVE_DOXYGEN

if ECSS_COMPLIANCE
html-local: Doxyfile Doxyfile.ecss
	$(DOXYGEN) Doxyfile.ecss
else
html-local: Doxyfile
	$(DOXYGEN)
endif

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
