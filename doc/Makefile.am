
EXTRA_DIST = \
tutorial/index.html \
tutorial/xcb.css

if BUILD_DOCS

# install documentation

all-local: doc-build.stamp

# rule to remove all old created files
doc-prepare.stamp:
	@if test -d manual/ ; then \
	  rm -rf manual/; \
	fi

# rule to build documentation and copy necessary files
doc-build.stamp: doc-prepare.stamp
	@cd .. && doxygen xcb.doxygen

# rules to clean
clean-local:
	@rm -rf manual/

install-data-local: install-html-local

# rule to install the html documentation and tutorial in $(htmldir)
install-html-local:
	@if ! test -d "$(DESTDIR)$(htmldir)"; then \
	  $(mkinstalldirs) "$(DESTDIR)$(htmldir)"; \
	fi
	@cp -pr manual/ "$(DESTDIR)$(htmldir)"
	@cp -pr tutorial/ "$(DESTDIR)$(htmldir)"

# rule to uninstall the documentation
uninstall-local:
	@rm -rf $(DESTDIR)$(htmldir)

endif
