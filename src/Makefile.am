lib_LTLIBRARIES = libXCB.la \
                  libXCBcomposite.la \
                  libXCBdamage.la \
                  libXCBdpms.la \
                  libXCBglx.la \
                  libXCBrandr.la \
                  libXCBrecord.la \
                  libXCBrender.la \
                  libXCBres.la \
                  libXCBscreensaver.la \
                  libXCBshape.la \
                  libXCBshm.la \
                  libXCBsync.la \
                  libXCBxevie.la \
                  libXCBxf86dri.la \
                  libXCBxfixes.la \
                  libXCBxprint.la \
                  libXCBxtest.la \
                  libXCBxv.la \
                  libXCBxvmc.la

EXTHEADERS = \
		extensions/bigreq.h \
		extensions/composite.h \
		extensions/damage.h \
		extensions/dpms.h \
		extensions/glx.h \
		extensions/randr.h \
		extensions/record.h \
		extensions/render.h \
		extensions/res.h \
		extensions/screensaver.h \
		extensions/shape.h \
		extensions/shm.h \
		extensions/sync.h \
		extensions/xc_misc.h \
		extensions/xevie.h \
		extensions/xf86dri.h \
		extensions/xfixes.h \
		extensions/xprint.h \
		extensions/xtest.h \
		extensions/xv.h \
		extensions/xvmc.h
EXTSOURCES = \
		extensions/bigreq.c \
		extensions/composite.c \
		extensions/damage.c \
		extensions/dpms.c \
		extensions/glx.c \
		extensions/randr.c \
		extensions/record.c \
		extensions/render.c \
		extensions/res.c \
		extensions/screensaver.c \
		extensions/shape.c \
		extensions/shm.c \
		extensions/sync.c \
		extensions/xc_misc.c \
		extensions/xevie.c \
		extensions/xf86dri.c \
		extensions/xfixes.c \
		extensions/xprint.c \
		extensions/xtest.c \
		extensions/xv.c \
		extensions/xvmc.c
EXTENSIONS = $(EXTSOURCES) $(EXTHEADERS)

ESSENTIAL_EXTENSIONS = \
		extensions/bigreq.h \
		extensions/bigreq.c \
		extensions/xc_misc.h \
		extensions/xc_misc.c

COREHEADERS = xproto.h
CORESOURCES = xproto.c
COREPROTO   = $(CORESOURCES) $(COREHEADERS)

xcbinclude_HEADERS = xcb.h xcbext.h xcbxlib.h $(COREHEADERS) $(EXTHEADERS)
noinst_HEADERS = xcbint.h

AM_CFLAGS = $(COPTFLAGS) $(CWARNFLAGS) $(CDEBUGFLAGS) $(XCBPROTO_CFLAGS) $(XPROTO_CFLAGS) $(XAU_CFLAGS) $(XDMCP_CFLAGS)
libXCB_la_LIBADD = $(XCBPROTO_LIBS) $(XPROTO_LIBS) $(XAU_LIBS) $(XDMCP_LIBS)
libXCB_la_SOURCES = \
		xcb_conn.c xcb_out.c xcb_in.c xcb_ext.c xcb_xid.c \
		xcb_list.c xcb_util.c xcb_xlib.c xcb_auth.c \
		$(COREPROTO) $(ESSENTIAL_EXTENSIONS)

# Explanation for -version-info:
# -version-info current:revision:age
# The library supports interface (current-age) to interface current.
# * If you changed the source code at all, increment revision.
# * If you add an interface, increment current and age and set revision to 0.
# * If you change or remove an interface, increment current and set revision
#   and age to 0.
libXCB_la_LDFLAGS = -version-info 0:0:0

BUILT_SOURCES = $(COREPROTO) $(EXTENSIONS)
CLEANFILES = $(COREPROTO) $(EXTENSIONS)

XCB_LIBS = libXCB.la

# FIXME: find a way to autogenerate this from the XML files.

libXCBcomposite_la_LDFLAGS = -version-info 0:0:0
libXCBcomposite_la_LIBADD = $(XCB_LIBS)
libXCBcomposite_la_SOURCES = extensions/composite.c extensions/composite.h

libXCBdamage_la_LDFLAGS = -version-info 0:0:0
libXCBdamage_la_LIBADD = $(XCB_LIBS)
libXCBdamage_la_SOURCES = extensions/damage.c extensions/damage.h

libXCBdpms_la_LDFLAGS = -version-info 0:0:0
libXCBdpms_la_LIBADD = $(XCB_LIBS)
libXCBdpms_la_SOURCES = extensions/dpms.c extensions/dpms.h

libXCBglx_la_LDFLAGS = -version-info 0:0:0
libXCBglx_la_LIBADD = $(XCB_LIBS)
libXCBglx_la_SOURCES = extensions/glx.c extensions/glx.h

libXCBrandr_la_LDFLAGS = -version-info 0:0:0
libXCBrandr_la_LIBADD = $(XCB_LIBS)
libXCBrandr_la_SOURCES = extensions/randr.c extensions/randr.h

libXCBrecord_la_LDFLAGS = -version-info 0:0:0
libXCBrecord_la_LIBADD = $(XCB_LIBS)
libXCBrecord_la_SOURCES = extensions/record.c extensions/record.h

libXCBrender_la_LDFLAGS = -version-info 0:0:0
libXCBrender_la_LIBADD = $(XCB_LIBS)
libXCBrender_la_SOURCES = extensions/render.c extensions/render.h

libXCBres_la_LDFLAGS = -version-info 0:0:0
libXCBres_la_LIBADD = $(XCB_LIBS)
libXCBres_la_SOURCES = extensions/res.c extensions/res.h

libXCBscreensaver_la_LDFLAGS = -version-info 0:0:0
libXCBscreensaver_la_LIBADD = $(XCB_LIBS)
libXCBscreensaver_la_SOURCES = extensions/screensaver.c extensions/screensaver.h

libXCBshape_la_LDFLAGS = -version-info 0:0:0
libXCBshape_la_LIBADD = $(XCB_LIBS)
libXCBshape_la_SOURCES = extensions/shape.c extensions/shape.h

libXCBshm_la_LDFLAGS = -version-info 0:0:0
libXCBshm_la_LIBADD = $(XCB_LIBS)
libXCBshm_la_SOURCES = extensions/shm.c extensions/shm.h

libXCBsync_la_LDFLAGS = -version-info 0:0:0
libXCBsync_la_LIBADD = $(XCB_LIBS)
libXCBsync_la_SOURCES = extensions/sync.c extensions/sync.h

libXCBxevie_la_LDFLAGS = -version-info 0:0:0
libXCBxevie_la_LIBADD = $(XCB_LIBS)
libXCBxevie_la_SOURCES = extensions/xevie.c extensions/xevie.h

libXCBxf86dri_la_LDFLAGS = -version-info 0:0:0
libXCBxf86dri_la_LIBADD = $(XCB_LIBS)
libXCBxf86dri_la_SOURCES = extensions/xf86dri.c extensions/xf86dri.h

libXCBxfixes_la_LDFLAGS = -version-info 0:0:0
libXCBxfixes_la_LIBADD = $(XCB_LIBS)
libXCBxfixes_la_SOURCES = extensions/xfixes.c extensions/xfixes.h

libXCBxprint_la_LDFLAGS = -version-info 0:0:0
libXCBxprint_la_LIBADD = $(XCB_LIBS)
libXCBxprint_la_SOURCES = extensions/xprint.c extensions/xprint.h

libXCBxtest_la_LDFLAGS = -version-info 0:0:0
libXCBxtest_la_LIBADD = $(XCB_LIBS)
libXCBxtest_la_SOURCES = extensions/xtest.c extensions/xtest.h

libXCBxv_la_LDFLAGS = -version-info 0:0:0
libXCBxv_la_LIBADD = $(XCB_LIBS)
libXCBxv_la_SOURCES = extensions/xv.c extensions/xv.h

libXCBxvmc_la_LDFLAGS = -version-info 0:0:0
libXCBxvmc_la_LIBADD = $(XCB_LIBS)
libXCBxvmc_la_SOURCES = extensions/xvmc.c extensions/xvmc.h

$(EXTHEADERS) $(EXTSOURCES): c-client.xsl

SUFFIXES = .xml

.xml.h:
	@n=`dirname $*`; test -d $$n || (echo mkdir $$n; mkdir $$n)
	$(XSLTPROC) --stringparam mode header \
	            --stringparam base-path $(XCBPROTO_XCBINCLUDEDIR)/ \
	            --stringparam extension-path \
	                          $(XCBPROTO_XCBINCLUDEDIR)/extensions/ \
	            -o $@ $(srcdir)/c-client.xsl $< 

.xml.c:
	@n=`dirname $*`; test -d $$n || (echo mkdir $$n; mkdir $$n)
	$(XSLTPROC) --stringparam mode source \
	            --stringparam base-path $(XCBPROTO_XCBINCLUDEDIR)/ \
	            --stringparam extension-path \
	                          $(XCBPROTO_XCBINCLUDEDIR)/extensions/ \
	            -o $@ $(srcdir)/c-client.xsl $< 

xproto.xml: $(XCBPROTO_XCBINCLUDEDIR)/xproto.xml
	ln -s $< $@

extensions/bigreq.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/bigreq.xml
	ln -s $< $@

extensions/composite.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/composite.xml
	ln -s $< $@

extensions/damage.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/damage.xml
	ln -s $< $@

extensions/dpms.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/dpms.xml
	ln -s $< $@

extensions/glx.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/glx.xml
	ln -s $< $@

extensions/randr.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/randr.xml
	ln -s $< $@

extensions/record.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/record.xml
	ln -s $< $@

extensions/render.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/render.xml
	ln -s $< $@

extensions/res.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/res.xml
	ln -s $< $@

extensions/screensaver.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/screensaver.xml
	ln -s $< $@

extensions/shape.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/shape.xml
	ln -s $< $@

extensions/shm.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/shm.xml
	ln -s $< $@

extensions/sync.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/sync.xml
	ln -s $< $@

extensions/xc_misc.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xc_misc.xml
	ln -s $< $@

extensions/xevie.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xevie.xml
	ln -s $< $@

extensions/xf86dri.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xf86dri.xml
	ln -s $< $@

extensions/xfixes.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xfixes.xml
	ln -s $< $@

extensions/xprint.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xprint.xml
	ln -s $< $@

extensions/xtest.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xtest.xml
	ln -s $< $@

extensions/xvmc.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xvmc.xml
	ln -s $< $@

extensions/xv.xml: $(XCBPROTO_XCBINCLUDEDIR)/extensions/xv.xml
	ln -s $< $@
