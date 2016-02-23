AC_INIT([oppdf], [0.16])
AC_CONFIG_SRCDIR([npshell.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIRS([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AM_PROG_AR
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# don't want static libraries
LT_INIT([shared disable-static])

# For the plugin part
PKG_CHECK_MODULES(NSPR, nspr)

# For the external helper part
PKG_CHECK_MODULES(GTK2, gtk+-2.0)
PKG_CHECK_MODULES(GTK2PRINT, gtk+-unix-print-2.0)
PKG_CHECK_MODULES(CAIRO_PS, cairo-ps)
PKG_CHECK_MODULES(POPPLER_GLIB, poppler-glib)
PKG_CHECK_MODULES(POPPLER_CAIRO, poppler-cairo)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
