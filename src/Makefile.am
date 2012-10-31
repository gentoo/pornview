
CLEANFILES = *~ .deps/* .*.swp .*.swo

SUBDIRS = pixmaps support prefs_ui scripts 
DIST_SUBDIRS = pixmaps support prefs_ui scripts 

INCLUDES = -DLOCALEDIR=\""$(localedir)"\" -Iprefs_ui -Isupport \
	$(GTK_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(PNG_CFLAGS) $(MOVIELIB_CFLAGS) \
	-Isupport/libexif -Isupport/widgets 

bin_PROGRAMS = pornview

pornview_SOURCES = 		\
	about.c			\
	about.h			\
	browser.c		\
	browser.h		\
	cache.c			\
	cache.h			\
	comment.c		\
	comment.h		\
	comment_view.c		\
	comment_view.h		\
	dirview.c		\
	dirview.h		\
	dock.c			\
	dock.h			\
	fileutil.c		\
	fileutil.h		\
	imageinfo.c		\
	imageinfo.h		\
	imageview.c		\
	imageview.h		\
	main.c			\
	navwindow.c		\
	navwindow.h		\
	plugin.c		\
	plugin.h		\
	pornview.h		\
	prefs.c			\
	prefs.h			\
	thumb_loader.c		\
	thumb_loader.h		\
	thumbview.c		\
	thumbview.h		\
	videoplay.h		\
	videoplay_mplayer.c	\
	videoplay_xine.c	\
	viewtype.c		\
	viewtype.h

pornview_LDFLAGS = \
	-export-dynamic

pornview_LDADD = prefs_ui/libprefs_ui.la  support/widgets/libsupport_widgets.la \
	support/libsupport_utils.la $(LIBEXIF) \
	$(GTK_LIBS) $(GDK_PIXBUF_LIBS) $(PNG_LIBS) $(MOVIELIB_LIBS) \
	$(INTLLIBS) $(LIBICONV) $(LIBCHARSET) $(X_LIBS) $(XINERAMA_LIBS)
