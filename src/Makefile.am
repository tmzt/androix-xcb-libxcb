lib_LTLIBRARIES = libXCB.la

EXTHEADERS = \
		extensions/bigreq.h \
		extensions/composite.h \
		extensions/damage.h \
		extensions/glx.h \
		extensions/shm.h \
		extensions/shape.h \
		extensions/randr.h \
		extensions/record.h \
		extensions/render.h \
		extensions/res.h \
		extensions/dpms.h \
		extensions/sync.h \
		extensions/xc_misc.h \
		extensions/xevie.h \
		extensions/xf86dri.h \
		extensions/xfixes.h \
		extensions/xprint.h \
		extensions/xv.h \
		extensions/xvmc.h
EXTSOURCES = \
		extensions/bigreq.c \
		extensions/composite.c \
		extensions/damage.c \
		extensions/glx.c \
		extensions/shm.c \
		extensions/shape.c \
		extensions/randr.c \
		extensions/record.c \
		extensions/render.c \
		extensions/res.c \
		extensions/dpms.c \
		extensions/sync.c \
		extensions/xc_misc.c \
		extensions/xevie.c \
		extensions/xf86dri.c \
		extensions/xfixes.c \
		extensions/xprint.c \
		extensions/xv.c \
		extensions/xvmc.c
EXTENSIONS = $(EXTSOURCES) $(EXTHEADERS)

COREHEADERS = xproto.h xcb_types.h
CORESOURCES = xproto.c xcb_types.c
COREPROTO   = $(CORESOURCES) $(COREHEADERS)

xcbinclude_HEADERS = xcb.h xcbext.h xcbxlib.h $(COREHEADERS) $(EXTHEADERS)

CFLAGS =
AM_CFLAGS = -include config.h $(CDEBUGFLAGS) $(XCBPROTO_CFLAGS) $(XPROTO_CFLAGS) $(XAU_CFLAGS)
libXCB_la_LIBADD = $(XCBPROTO_LIBS) $(XPROTO_LIBS) $(XAU_LIBS)
libXCB_la_SOURCES = \
		xcb_conn.c xcb_out.c xcb_in.c xcb_ext.c xcb_xid.c \
		xcb_list.c xcb_util.c xcb_xlib.c xcb_auth.c xcb_des.c \
		$(COREPROTO) $(EXTENSIONS)

xcb_des.c:
	touch xcb_des.c

BUILT_SOURCES = $(COREPROTO) $(EXTENSIONS)
CLEANFILES = $(COREPROTO) $(EXTENSIONS)
clean-local:
	rmdir extensions || true

vpath %.xml $(XCBPROTO_XCBINCLUDEDIR) $(XCBPROTO_XCBINCLUDEDIR)/extensions

%.h: %.xml c-client.xsl
	@n=`dirname $*`; test -d $$n || (echo mkdir $$n; mkdir $$n)
	$(XSLTPROC) --stringparam mode header \
	            --stringparam base-path $(XCBPROTO_XCBINCLUDEDIR)/ \
	            --stringparam extension-path \
	                          $(XCBPROTO_XCBINCLUDEDIR)/extensions/ \
	            -o $@ $(srcdir)/c-client.xsl $< 

%.c: %.xml c-client.xsl
	@n=`dirname $*`; test -d $$n || (echo mkdir $$n; mkdir $$n)
	$(XSLTPROC) --stringparam mode source \
	            --stringparam base-path $(XCBPROTO_XCBINCLUDEDIR)/ \
	            --stringparam extension-path \
	                          $(XCBPROTO_XCBINCLUDEDIR)/extensions/ \
	            -o $@ $(srcdir)/c-client.xsl $< 
