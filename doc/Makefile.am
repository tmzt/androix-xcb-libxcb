
EXTRA_DIST = \
tutorial/index.html \
tutorial/xcb.css \
xcb.doxygen.in

htmlDATA_INSTALL = cp -pR

html_DATA = tutorial/

uninstall-htmlDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(html_DATA)'; for p in $$list; do \
	  f=$(am__strip_dir) \
	  echo " rm -rf '$(DESTDIR)$(htmldir)/$$f'"; \
	  rm -rf "$(DESTDIR)$(htmldir)/$$f"; \
	done
	rmdir "$(DESTDIR)$(htmldir)/" || true

if BUILD_DOCS

# rule to build documentation and copy necessary files
manual: clean-local
	doxygen xcb.doxygen

# rules to clean
clean-local:
	rm -rf manual/

html_DATA += manual/

endif
