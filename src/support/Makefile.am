
CLEANFILES = *~ .deps/* .*.swp .*.swo *.a *.la

SUBDIRS = cursors libexif pixmaps widgets
DIST_SUBDIRS = cursors libexif pixmaps widgets

INCLUDES = $(GTK_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(PNG_FLAGS) $(MOVIELIB_CFLAGS) -Ilibexif -I../

noinst_LTLIBRARIES = libsupport_utils.la

libsupport_utils_la_SOURCES =	\
	button.c		\
	button.h		\
	charset.c		\
	charset.h		\
	check_button.c		\
	check_button.h		\
	clist_edit.c		\
	clist_edit.h		\
	cursors.c		\
	cursors.h		\
	dialogs.c		\
	dialogs.h		\
	exif_view.c		\
	exif_view.h		\
	file_dialog.c		\
	file_dialog.h		\
	file_list.c		\
	file_list.h		\
	file_type.c		\
	file_type.h		\
	file_utils.c		\
	file_utils.h		\
	generic_dialog.c	\
	generic_dialog.h	\
	image.c			\
	image.h			\
	image_info.c		\
	image_info.h		\
	image_loader.c		\
	image_loader.h		\
	intl.h			\
	japanese.c		\
	menu.c			\
	menu.h			\
	path_selection.c	\
	path_selection.h	\
	pixbuf_utils.c		\
	pixbuf_utils.h		\
	string_utils.c		\
	string_utils.h
