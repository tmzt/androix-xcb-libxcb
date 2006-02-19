#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([libXCB],
        0.9,
        [xcb@lists.freedesktop.org])
AC_CONFIG_SRCDIR([xcb.pc.in])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

dnl This ifdef has no useful effect prior to automake 1.9, but in 1.9
dnl it allows the user to not have check.m4 installed.
m4_ifdef([AM_PATH_CHECK],[
AM_PATH_CHECK(0.8.2, [HAVE_CHECK=true], [HAVE_CHECK=false])
])
AM_CONDITIONAL(HAVE_CHECK, test x$HAVE_CHECK = xtrue)

AC_CONFIG_HEADERS([src/config.h])

AC_PROG_LIBTOOL
AC_PROG_CC

AC_PATH_PROG(XSLTPROC, xsltproc, no)
if test "$XSLTPROC" = "no"; then
   AC_MSG_ERROR([XCB requires xsltproc.])
fi

HTML_CHECK_RESULT=false
if test x"$HAVE_CHECK" = xtrue; then
	if test x"$XSLTPROC" != xno; then
		HTML_CHECK_RESULT=true
	fi
fi
AC_SUBST(HTML_CHECK_RESULT)

# Checks for pkg-config packages
PKG_CHECK_MODULES(XCBPROTO, xcb-proto >= 0.9)
PKG_CHECK_MODULES(XPROTO, xproto)
PKG_CHECK_MODULES(XAU, xau)

# Find the xcb-proto protocol descriptions
AC_MSG_CHECKING(XCBPROTO_XCBINCLUDEDIR)
XCBPROTO_XCBINCLUDEDIR=`$PKG_CONFIG --variable=xcbincludedir xcb-proto`
AC_MSG_RESULT($XCBPROTO_XCBINCLUDEDIR)
AC_SUBST(XCBPROTO_XCBINCLUDEDIR)

AC_HEADER_STDC
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(connect, socket)

xcbincludedir='${includedir}/X11/XCB'
AC_SUBST(xcbincludedir)

AC_ARG_WITH(opt,
AC_HELP_STRING([--with-opt],       [compile with reasonable optimizations])
AC_HELP_STRING([--with-opt=FLAGS], [compile with specified FLAGS])
AC_HELP_STRING([--with-opt=small], [compile for smallest code])
AC_HELP_STRING([--with-opt=debug], [compile for debugging (default)]),
[
	case "$withval" in
	yes)
		optflags="-O3"
		;;
	small)
		optflags="-Os -fomit-frame-pointer -DNDEBUG"
		;;
	no|debug)
		optflags="-g"
		;;
	*)
		optflags="$withval"
		;;
	esac
])
AC_CACHE_CHECK([what compiler optimizations to apply], [optflags], [optflags="-g"])
AC_DEFINE(_XOPEN_SOURCE, 500, [Conform to Single Unix Specification.])
CDEBUGFLAGS="$optflags -Wall -pedantic -Wpointer-arith \
	-Wstrict-prototypes -Wmissing-declarations -Wnested-externs"
AC_SUBST(CDEBUGFLAGS)

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_CONFIG_FILES([xcb.pc])

AC_OUTPUT
