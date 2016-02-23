ACLOCAL_AMFLAGS = -I m4

SUBDIRS = 

myplugindir = $(libdir)/mozilla/plugins

myplugin_LTLIBRARIES = liboppdf.la

bin_PROGRAMS = oppdf-handler

noinst_PROGRAMS = oppdf-test

# Note that the include files from moz-sdk were taken from the 
# mozilla/dist/sdk/include directory of the Firefox 2.0.0.2 release.

# need better solution than hardcoded X11 include path, but
# 'pkg-config --cflags' does not work correctly for x11 or xproto packages

liboppdf_la_CFLAGS = -Wall -I$(top_srcdir)/moz-sdk $(NSPR_CFLAGS) -I/usr/include/X11

liboppdf_la_LDFLAGS = $(NSPR_LIBS) -avoid-version

liboppdf_la_SOURCES = \
	npshell.c \
	npunix.c \
	oppdf.h \
	moz-sdk/jni.h \
	moz-sdk/jni_md.h \
	moz-sdk/jri.h \
	moz-sdk/jri_md.h \
	moz-sdk/jritypes.h \
	moz-sdk/npapi.h \
	moz-sdk/npruntime.h \
	moz-sdk/nptypes.h \
	moz-sdk/npupp.h

oppdf_handler_SOURCES = \
	support.h main.c \
	interface.c interface.h \
	callbacks.c callbacks.h

oppdf_handler_CFLAGS= \
	-Wall -g -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(GTK2_CFLAGS) $(CAIRO_PS_CFLAGS) $(POPPLER_GLIB_CFLAGS) $(POPPLER_CAIRO_CFLAGS) $(GTK2PRINT_CFLAGS)

oppdf_handler_LDADD = \
	$(GTK2_LIBS) $(CAIRO_PS_LIBS) \
	$(POPPLER_GLIB_LIBS) $(POPPLER_CAIRO_LIBS) \
	$(GTK2PRINT_LIBS) $(INTLLIBS)

oppdf_test_SOURCES = \
	oppdf-test.c

oppdf_test_CFLAGS= \
	-Wall -g -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(GTK2_CFLAGS)

oppdf_test_LDADD = \
	$(GTK2_LIBS)

# custom install rule
install-plugin:
	mkdir -p ~/.mozilla/plugins
	cp .libs/liboppdf.so ~/.mozilla/plugins
