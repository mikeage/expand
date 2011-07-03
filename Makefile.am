EXTRA_DIST = \
	Makefile.mingw

pkgdir = $(PIDGIN_PLUGINDIR)

EXPAND_SOURCES = \
				   expand.c \
				   expand.h \
				   local_util.c \
				   local_util.h 

AM_CFLAGS = \
	$(st)

expand_la_LDFLAGS = -module -avoid-version $(PURPLE_LIBS)

st = 

pkg_LTLIBRARIES = \
	expand.la 

expand_la_SOURCES = $(EXPAND_SOURCES)
expand_la_LIBADD = $(GLIB_LIBS)

AM_CPPFLAGS = \
	$(DEBUG_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(PIDGIN_PLUGINS) \
	-DLOCALEDIR=\"$(LIBPURPLE_DATADIR)/locale\"
