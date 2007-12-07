#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([libxcb],
        1.1,
        [xcb@lists.freedesktop.org])
AC_CONFIG_SRCDIR([xcb.pc.in])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

PKG_CHECK_MODULES(CHECK, [check >= 0.9.4], [HAVE_CHECK=yes], [HAVE_CHECK=no])
AM_CONDITIONAL(HAVE_CHECK, test x$HAVE_CHECK = xyes)

AC_CONFIG_HEADERS([src/config.h])

AC_PROG_LIBTOOL
AC_PROG_CC

AC_PATH_PROG(XSLTPROC, xsltproc, no)
if test "$XSLTPROC" = "no"; then
   AC_MSG_ERROR([XCB requires xsltproc.])
fi

HTML_CHECK_RESULT=false
if test x"$HAVE_CHECK" = xyes; then
	if test x"$XSLTPROC" != xno; then
		HTML_CHECK_RESULT=true
	fi
fi
AC_SUBST(HTML_CHECK_RESULT)

# Checks for pkg-config packages
PKG_CHECK_MODULES(XCBPROTO, xcb-proto >= 1.1)
NEEDED="pthread-stubs xau >= 0.99.2"
PKG_CHECK_MODULES(NEEDED, $NEEDED)

have_xdmcp="no"
PKG_CHECK_MODULES(XDMCP, xdmcp,
	AC_CHECK_LIB(Xdmcp, XdmcpWrap,
		[
		AC_DEFINE(HASXDMAUTH,1,[Has Wraphelp.c needed for XDM AUTH protocols])
		NEEDED="$NEEDED xdmcp"
		have_xdmcp="yes"
		],
		[
		XDMCP_CFLAGS=
		XDMCP_LIBS=
		], [$XDMCP_LIBS]),
	[AC_MSG_RESULT(no)])

AC_CHECK_HEADER([execinfo.h], [AC_DEFINE(HAVE_BACKTRACE,1,[Has backtrace*() needed for retrieving stack traces])])

AC_SUBST(NEEDED)

# Find the xcb-proto protocol descriptions
AC_MSG_CHECKING(XCBPROTO_XCBINCLUDEDIR)
XCBPROTO_XCBINCLUDEDIR=`$PKG_CONFIG --variable=xcbincludedir xcb-proto`
AC_MSG_RESULT($XCBPROTO_XCBINCLUDEDIR)
AC_SUBST(XCBPROTO_XCBINCLUDEDIR)

AC_HEADER_STDC
AC_SEARCH_LIBS(getaddrinfo, socket)
AC_SEARCH_LIBS(connect, socket)

xcbincludedir='${includedir}/xcb'
AC_SUBST(xcbincludedir)

if  test "x$GCC" = xyes ; then
    CWARNFLAGS="-Wall -pedantic -Wpointer-arith \
		-Wstrict-prototypes -Wmissing-declarations -Wnested-externs"
else
    AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
    if test "x$SUNCC" = "xyes"; then
	CWARNFLAGS="-v"
    fi
fi
AC_SUBST(CWARNFLAGS)

GCC_CHECK_VISIBILITY()

# htmldir is not defined prior to autoconf 2.59c, so on earlier versions
# set an equivalent value.
AC_PREREQ([2.59c], [], [AC_SUBST([htmldir], [m4_ifset([AC_PACKAGE_TARNAME],
	  ['${datadir}/doc/${PACKAGE_TARNAME}'],
	  ['${datadir}/doc/${PACKAGE}'])
])])
AM_CHECK_DOXYGEN()

AC_ARG_ENABLE(composite, AS_HELP_STRING([--enable-composite], [Build XCB Composite Extension (default: yes)]), [BUILD_COMPOSITE=$enableval], [BUILD_COMPOSITE=yes])
AM_CONDITIONAL(BUILD_COMPOSITE, [test "x$BUILD_COMPOSITE" = xyes])
AC_ARG_ENABLE(damage, AS_HELP_STRING([--enable-damage], [Build XCB Damage Extension (default: yes)]), [BUILD_DAMAGE=$enableval], [BUILD_DAMAGE=yes])
AM_CONDITIONAL(BUILD_DAMAGE, [test "x$BUILD_DAMAGE" = xyes])
AC_ARG_ENABLE(dpms, AS_HELP_STRING([--enable-dpms], [Build XCB DPMS Extension (default: yes)]), [BUILD_DPMS=$enableval], [BUILD_DPMS=yes])
AM_CONDITIONAL(BUILD_DPMS, [test "x$BUILD_DPMS" = xyes])
AC_ARG_ENABLE(glx, AS_HELP_STRING([--enable-glx], [Build XCB GLX Extension (default: yes)]), [BUILD_GLX=$enableval], [BUILD_GLX=yes])
AM_CONDITIONAL(BUILD_GLX, [test "x$BUILD_GLX" = xyes])
AC_ARG_ENABLE(randr, AS_HELP_STRING([--enable-randr], [Build XCB RandR Extension (default: yes)]), [BUILD_RANDR=$enableval], [BUILD_RANDR=yes])
AM_CONDITIONAL(BUILD_RANDR, [test "x$BUILD_RANDR" = xyes])
AC_ARG_ENABLE(record, AS_HELP_STRING([--enable-record], [Build XCB Record Extension (default: yes)]), [BUILD_RECORD=$enableval], [BUILD_RECORD=yes])
AM_CONDITIONAL(BUILD_RECORD, [test "x$BUILD_RECORD" = xyes])
AC_ARG_ENABLE(render, AS_HELP_STRING([--enable-render], [Build XCB Render Extension (default: yes)]), [BUILD_RENDER=$enableval], [BUILD_RENDER=yes])
AM_CONDITIONAL(BUILD_RENDER, [test "x$BUILD_RENDER" = xyes])
AC_ARG_ENABLE(res, AS_HELP_STRING([--enable-res], [Build XCB X-Resource Extension (default: yes)]), [BUILD_RES=$enableval], [BUILD_RES=yes])
AM_CONDITIONAL(BUILD_RES, [test "x$BUILD_RES" = xyes])
AC_ARG_ENABLE(screensaver, AS_HELP_STRING([--enable-screensaver], [Build XCB Screensaver Extension (default: yes)]), [BUILD_SCREENSAVER=$enableval], [BUILD_SCREENSAVER=yes])
AM_CONDITIONAL(BUILD_SCREENSAVER, [test "x$BUILD_SCREENSAVER" = xyes])
AC_ARG_ENABLE(shape, AS_HELP_STRING([--enable-shape], [Build XCB Shape Extension (default: yes)]), [BUILD_SHAPE=$enableval], [BUILD_SHAPE=yes])
AM_CONDITIONAL(BUILD_SHAPE, [test "x$BUILD_SHAPE" = xyes])
AC_ARG_ENABLE(shm, AS_HELP_STRING([--enable-shm], [Build XCB Shm Extension (default: yes)]), [BUILD_SHM=$enableval], [BUILD_SHM=yes])
AM_CONDITIONAL(BUILD_SHM, [test "x$BUILD_SHM" = xyes])
AC_ARG_ENABLE(sync, AS_HELP_STRING([--enable-sync], [Build XCB Sync Extension (default: yes)]), [BUILD_SYNC=$enableval], [BUILD_SYNC=yes])
AM_CONDITIONAL(BUILD_SYNC, [test "x$BUILD_SYNC" = xyes])
AC_ARG_ENABLE(xevie, AS_HELP_STRING([--enable-xevie], [Build XCB Xevie Extension (default: yes)]), [BUILD_XEVIE=$enableval], [BUILD_XEVIE=yes])
AM_CONDITIONAL(BUILD_XEVIE, [test "x$BUILD_XEVIE" = xyes])
AC_ARG_ENABLE(xf86dri, AS_HELP_STRING([--enable-xf86dri], [Build XCB XFree86-DRI Extension (default: yes)]), [BUILD_XF86DRI=$enableval], [BUILD_XF86DRI=yes])
AM_CONDITIONAL(BUILD_XF86DRI, [test "x$BUILD_XF86DRI" = xyes])
AC_ARG_ENABLE(xfixes, AS_HELP_STRING([--enable-xfixes], [Build XCB XFixes Extension (default: yes)]), [BUILD_XFIXES=$enableval], [BUILD_XFIXES=yes])
AM_CONDITIONAL(BUILD_XFIXES, [test "x$BUILD_XFIXES" = xyes])
AC_ARG_ENABLE(xinerama, AS_HELP_STRING([--enable-xinerama], [Build XCB Xinerama Extension (default: yes)]), [BUILD_XINERAMA=$enableval], [BUILD_XINERAMA=yes])
AM_CONDITIONAL(BUILD_XINERAMA, [test "x$BUILD_XINERAMA" = xyes])
AC_ARG_ENABLE(xprint, AS_HELP_STRING([--enable-xprint], [Build XCB Xprint Extension (default: yes)]), [BUILD_XPRINT=$enableval], [BUILD_XPRINT=yes])
AM_CONDITIONAL(BUILD_XPRINT, [test "x$BUILD_XPRINT" = xyes])
AC_ARG_ENABLE(xtest, AS_HELP_STRING([--enable-xtest], [Build XCB XTEST Extension (default: yes)]), [BUILD_XTEST=$enableval], [BUILD_XTEST=yes])
AM_CONDITIONAL(BUILD_XTEST, [test "x$BUILD_XTEST" = xyes])
AC_ARG_ENABLE(xv, AS_HELP_STRING([--enable-xv], [Build XCB Xv Extension (default: yes)]), [BUILD_XV=$enableval], [BUILD_XV=yes])
AM_CONDITIONAL(BUILD_XV, [test "x$BUILD_XV" = xyes])
AC_ARG_ENABLE(xvmc, AS_HELP_STRING([--enable-xvmc], [Build XCB XvMC Extension (default: yes)]), [BUILD_XVMC=$enableval], [BUILD_XVMC=yes])
AM_CONDITIONAL(BUILD_XVMC, [test "x$BUILD_XVMC" = xyes])


AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile doc/Makefile])
AC_CONFIG_FILES([xcb.pc xcb-xlib.pc xcb-composite.pc xcb-damage.pc xcb-dpms.pc xcb-glx.pc xcb-randr.pc xcb-record.pc xcb-render.pc xcb-res.pc xcb-screensaver.pc xcb-shape.pc xcb-shm.pc xcb-sync.pc xcb-xevie.pc xcb-xf86dri.pc xcb-xfixes.pc xcb-xinerama.pc xcb-xprint.pc xcb-xtest.pc xcb-xv.pc xcb-xvmc.pc])
AC_CONFIG_FILES([doc/xcb.doxygen])

AC_OUTPUT

dnl Configuration output

echo ""
echo "  Package: ${PACKAGE_NAME} ${PACKAGE_VERSION}"
echo ""
echo "  Configuration"
echo "    XDM support.........: ${have_xdmcp}"
echo "    Build unit tests....: ${HAVE_CHECK}"
echo ""
echo "  Used CFLAGS:"
echo "    CPPFLAGS............: ${CPPFLAGS}"
echo "    CFLAGS..............: ${CFLAGS}"
echo "    Warning CFLAGS......: ${CWARNFLAGS}"
echo ""
echo "  Installation:"
echo "    Prefix..............: ${prefix}"
echo ""
