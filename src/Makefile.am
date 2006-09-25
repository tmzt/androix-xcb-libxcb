lib_LTLIBRARIES = libxcb.la \
                  libxcb-composite.la \
                  libxcb-damage.la \
                  libxcb-dpms.la \
                  libxcb-glx.la \
                  libxcb-randr.la \
                  libxcb-record.la \
                  libxcb-render.la \
                  libxcb-res.la \
                  libxcb-screensaver.la \
                  libxcb-shape.la \
                  libxcb-shm.la \
                  libxcb-sync.la \
                  libxcb-xevie.la \
                  libxcb-xf86dri.la \
                  libxcb-xfixes.la \
                  libxcb-xprint.la \
                  libxcb-xtest.la \
                  libxcb-xv.la \
                  libxcb-xvmc.la

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

EXTENSION_XML = \
		extensions/bigreq.xml \
		extensions/composite.xml \
		extensions/damage.xml \
		extensions/dpms.xml \
		extensions/glx.xml \
		extensions/randr.xml \
		extensions/record.xml \
		extensions/render.xml \
		extensions/res.xml \
		extensions/screensaver.xml \
		extensions/shape.xml \
		extensions/shm.xml \
		extensions/sync.xml \
		extensions/xc_misc.xml \
		extensions/xevie.xml \
		extensions/xf86dri.xml \
		extensions/xfixes.xml \
		extensions/xprint.xml \
		extensions/xtest.xml \
		extensions/xvmc.xml \
		extensions/xv.xml

COREHEADERS = xproto.h
CORESOURCES = xproto.c
COREPROTO   = $(CORESOURCES) $(COREHEADERS)

xcbinclude_HEADERS = xcb.h xcbext.h xcbxlib.h $(COREHEADERS) $(EXTHEADERS)
noinst_HEADERS = xcbint.h

AM_CFLAGS = $(COPTFLAGS) $(CWARNFLAGS) $(CDEBUGFLAGS) $(XCBPROTO_CFLAGS) $(XPROTO_CFLAGS) $(XAU_CFLAGS) $(XDMCP_CFLAGS)
libxcb_la_LIBADD = $(XCBPROTO_LIBS) $(XPROTO_LIBS) $(XAU_LIBS) $(XDMCP_LIBS)
libxcb_la_SOURCES = \
		xcb_conn.c xcb_out.c xcb_in.c xcb_ext.c xcb_xid.c \
		xcb_list.c xcb_util.c xcb_xlib.c xcb_auth.c \
		$(COREPROTO) $(ESSENTIAL_EXTENSIONS) c-client.xsl

# Explanation for -version-info:
# -version-info current:revision:age
# The library supports interface (current-age) to interface current.
# * If you changed the source code at all, increment revision.
# * If you add an interface, increment current and age and set revision to 0.
# * If you change or remove an interface, increment current and set revision
#   and age to 0.
libxcb_la_LDFLAGS = -version-info 0:0:0

BUILT_SOURCES = $(COREPROTO) $(EXTENSIONS)
CLEANFILES = $(COREPROTO) $(EXTENSIONS)

XCB_LIBS = libxcb.la

# FIXME: find a way to autogenerate this from the XML files.

libxcb_composite_la_LDFLAGS = -version-info 0:0:0
libxcb_composite_la_LIBADD = $(XCB_LIBS)
libxcb_composite_la_SOURCES = extensions/composite.c extensions/composite.h

libxcb_damage_la_LDFLAGS = -version-info 0:0:0
libxcb_damage_la_LIBADD = $(XCB_LIBS)
libxcb_damage_la_SOURCES = extensions/damage.c extensions/damage.h

libxcb_dpms_la_LDFLAGS = -version-info 0:0:0
libxcb_dpms_la_LIBADD = $(XCB_LIBS)
libxcb_dpms_la_SOURCES = extensions/dpms.c extensions/dpms.h

libxcb_glx_la_LDFLAGS = -version-info 0:0:0
libxcb_glx_la_LIBADD = $(XCB_LIBS)
libxcb_glx_la_SOURCES = extensions/glx.c extensions/glx.h

libxcb_randr_la_LDFLAGS = -version-info 0:0:0
libxcb_randr_la_LIBADD = $(XCB_LIBS)
libxcb_randr_la_SOURCES = extensions/randr.c extensions/randr.h

libxcb_record_la_LDFLAGS = -version-info 0:0:0
libxcb_record_la_LIBADD = $(XCB_LIBS)
libxcb_record_la_SOURCES = extensions/record.c extensions/record.h

libxcb_render_la_LDFLAGS = -version-info 0:0:0
libxcb_render_la_LIBADD = $(XCB_LIBS)
libxcb_render_la_SOURCES = extensions/render.c extensions/render.h

libxcb_res_la_LDFLAGS = -version-info 0:0:0
libxcb_res_la_LIBADD = $(XCB_LIBS)
libxcb_res_la_SOURCES = extensions/res.c extensions/res.h

libxcb_screensaver_la_LDFLAGS = -version-info 0:0:0
libxcb_screensaver_la_LIBADD = $(XCB_LIBS)
libxcb_screensaver_la_SOURCES = extensions/screensaver.c extensions/screensaver.h

libxcb_shape_la_LDFLAGS = -version-info 0:0:0
libxcb_shape_la_LIBADD = $(XCB_LIBS)
libxcb_shape_la_SOURCES = extensions/shape.c extensions/shape.h

libxcb_shm_la_LDFLAGS = -version-info 0:0:0
libxcb_shm_la_LIBADD = $(XCB_LIBS)
libxcb_shm_la_SOURCES = extensions/shm.c extensions/shm.h

libxcb_sync_la_LDFLAGS = -version-info 0:0:0
libxcb_sync_la_LIBADD = $(XCB_LIBS)
libxcb_sync_la_SOURCES = extensions/sync.c extensions/sync.h

libxcb_xevie_la_LDFLAGS = -version-info 0:0:0
libxcb_xevie_la_LIBADD = $(XCB_LIBS)
libxcb_xevie_la_SOURCES = extensions/xevie.c extensions/xevie.h

libxcb_xf86dri_la_LDFLAGS = -version-info 0:0:0
libxcb_xf86dri_la_LIBADD = $(XCB_LIBS)
libxcb_xf86dri_la_SOURCES = extensions/xf86dri.c extensions/xf86dri.h

libxcb_xfixes_la_LDFLAGS = -version-info 0:0:0
libxcb_xfixes_la_LIBADD = $(XCB_LIBS)
libxcb_xfixes_la_SOURCES = extensions/xfixes.c extensions/xfixes.h

libxcb_xprint_la_LDFLAGS = -version-info 0:0:0
libxcb_xprint_la_LIBADD = $(XCB_LIBS)
libxcb_xprint_la_SOURCES = extensions/xprint.c extensions/xprint.h

libxcb_xtest_la_LDFLAGS = -version-info 0:0:0
libxcb_xtest_la_LIBADD = $(XCB_LIBS)
libxcb_xtest_la_SOURCES = extensions/xtest.c extensions/xtest.h

libxcb_xv_la_LDFLAGS = -version-info 0:0:0
libxcb_xv_la_LIBADD = $(XCB_LIBS)
libxcb_xv_la_SOURCES = extensions/xv.c extensions/xv.h

libxcb_xvmc_la_LDFLAGS = -version-info 0:0:0
libxcb_xvmc_la_LIBADD = $(XCB_LIBS)
libxcb_xvmc_la_SOURCES = extensions/xvmc.c extensions/xvmc.h

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
	$(LN_S) $(XCBPROTO_XCBINCLUDEDIR)/xproto.xml $@

$(EXTENSION_XML):
	[ -d extensions ] || mkdir extensions
	for i in $(EXTENSION_XML) ; do \
           rm -f $$i ; \
           $(LN_S) $(XCBPROTO_XCBINCLUDEDIR)/$$i $$i ; \
        done
