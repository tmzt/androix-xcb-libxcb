SUBDIRS=src tests doc

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = \
xcb.pc \
xcb-xlib.pc

if BUILD_COMPOSITE
pkgconfig_DATA += xcb-composite.pc
endif
if BUILD_DAMAGE
pkgconfig_DATA += xcb-damage.pc
endif
if BUILD_DPMS
pkgconfig_DATA += xcb-dpms.pc
endif
if BUILD_GLX
pkgconfig_DATA += xcb-glx.pc
endif
if BUILD_RANDR
pkgconfig_DATA += xcb-randr.pc
endif
if BUILD_RECORD
pkgconfig_DATA += xcb-record.pc
endif
if BUILD_RENDER
pkgconfig_DATA += xcb-render.pc
endif
if BUILD_RES
pkgconfig_DATA += xcb-res.pc
endif
if BUILD_SCREENSAVER
pkgconfig_DATA += xcb-screensaver.pc
endif
if BUILD_SHAPE
pkgconfig_DATA += xcb-shape.pc
endif
if BUILD_SHM
pkgconfig_DATA += xcb-shm.pc
endif
if BUILD_SYNC
pkgconfig_DATA += xcb-sync.pc
endif
if BUILD_XEVIE
pkgconfig_DATA += xcb-xevie.pc
endif
if BUILD_XF86DRI
pkgconfig_DATA += xcb-xf86dri.pc
endif
if BUILD_XFIXES
pkgconfig_DATA += xcb-xfixes.pc
endif
if BUILD_XINERAMA
pkgconfig_DATA += xcb-xinerama.pc
endif
if BUILD_XPRINT
pkgconfig_DATA += xcb-xprint.pc
endif
if BUILD_XTEST
pkgconfig_DATA += xcb-xtest.pc
endif
if BUILD_XV
pkgconfig_DATA += xcb-xv.pc
endif
if BUILD_XVMC
pkgconfig_DATA += xcb-xvmc.pc
endif


EXTRA_DIST = \
xcb.pc.in \
xcb-xlib.pc.in \
tools/README \
tools/api_conv.pl \
tools/constants \
autogen.sh

if BUILD_COMPOSITE
EXTRA_DIST += xcb-composite.pc.in
endif
if BUILD_DAMAGE
EXTRA_DIST += xcb-damage.pc.in
endif
if BUILD_DPMS
EXTRA_DIST += xcb-dpms.pc.in
endif
if BUILD_GLX
EXTRA_DIST += xcb-glx.pc.in
endif
if BUILD_RANDR
EXTRA_DIST += xcb-randr.pc.in
endif
if BUILD_RECORD
EXTRA_DIST += xcb-record.pc.in
endif
if BUILD_RENDER
EXTRA_DIST += xcb-render.pc.in
endif
if BUILD_RES
EXTRA_DIST += xcb-res.pc.in
endif
if BUILD_SCREENSAVER
EXTRA_DIST += xcb-screensaver.pc.in
endif
if BUILD_SHAPE
EXTRA_DIST += xcb-shape.pc.in
endif
if BUILD_SHM
EXTRA_DIST += xcb-shm.pc.in
endif
if BUILD_SYNC
EXTRA_DIST += xcb-sync.pc.in
endif
if BUILD_XEVIE
EXTRA_DIST += xcb-xevie.pc.in
endif
if BUILD_XF86DRI
EXTRA_DIST += xcb-xf86dri.pc.in
endif
if BUILD_XFIXES
EXTRA_DIST += xcb-xfixes.pc.in
endif
if BUILD_XINERAMA
EXTRA_DIST += xcb-xinerama.pc.in
endif
if BUILD_XPRINT
EXTRA_DIST += xcb-xprint.pc.in
endif
if BUILD_XTEST
EXTRA_DIST += xcb-xtest.pc.in
endif
if BUILD_XV
EXTRA_DIST += xcb-xv.pc.in
endif
if BUILD_XVMC
EXTRA_DIST += xcb-xvmc.pc.in
endif


