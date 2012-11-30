
CLEANFILES = *~ .deps/* .*.swp .*.swo *.a

AM_CPPFLAGS = -I../support -I../ -I../support/widgets \
	$(GTK_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(PNG_FLAGS) $(MOVIELIB_CFLAGS)

noinst_LTLIBRARIES = libprefs_ui.la

libprefs_ui_la_SOURCES =	\
	prefs_ui_common.c	\
	prefs_ui_common.h	\
	prefs_ui_dirview.c	\
	prefs_ui_dirview.h	\
	prefs_ui_etc.c		\
	prefs_ui_etc.h		\
	prefs_ui_imageview.c	\
	prefs_ui_imageview.h	\
	prefs_ui_plugin.c	\
	prefs_ui_plugin.h	\
	prefs_ui_progs.c	\
	prefs_ui_progs.h	\
	prefs_ui_thumbview.c	\
	prefs_ui_thumbview.h	\
	prefs_ui_utils.c	\
	prefs_ui_utils.h	\
	prefs_win.c		\
	prefs_win.h
