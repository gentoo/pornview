
CLEANFILES = *~ .deps/* .*.swp .*.swo *.a *.la

SUBDIRS = pixmaps
DIST_SUBDIRS = pixmaps

AM_CPPFLAGS = $(GTK_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(PNG_FLAGS) $(MOVIELIB_CFLAGS) -I../

noinst_LTLIBRARIES = libsupport_widgets.la

libsupport_widgets_la_SOURCES =	\
	dirtree.c		\
	dirtree.h		\
	editable_list.c		\
	editable_list.h		\
	gedo-hpaned.c		\
	gedo-hpaned.h		\
	gedo-paned.c		\
	gedo-paned.h		\
	gedo-vpaned.c		\
	gedo-vpaned.h		\
	gtk2-compat.h		\
	gtkcellrendererpixmap.c	\
	gtkcellrendererpixmap.h	\
	gtkmplayer.c		\
	gtkmplayer.h		\
	gtkxine.c		\
	gtkxine.h		\
	gtkxine_old.c		\
	gtkxine_old.h		\
	marshal.c		\
	marshal.h		\
	scrolled.c		\
	scrolled.h		\
	zalbum.c		\
	zalbum.h		\
	zlist.c			\
	zlist.h

#EXTRA_DIST = gimv_marshal.list
#
#if USE_GTK2
#gimv_marshal.h : gimv_marshal.list $(GLIB_GENMARSHAL)
#	$(GLIB_GENMARSHAL) $< --header --prefix=gimv_marshal > $@
#
#gimv_marshal.c : gimv_marshal.list $(GLIB_GENMARSHAL)
#	$(GLIB_GENMARSHAL) $< --body --prefix=gimv_marshal > $@
#endif
