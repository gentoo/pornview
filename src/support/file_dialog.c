/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GQview.
 * GQview author: John Ellis
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h>	/* for keyboard values */

#include "generic_dialog.h"
#include "file_dialog.h"
#include "path_selection.h"
#include "file_utils.h"
#include "string_utils.h"

void
file_dialog_close (FileDialog * fd)
{
    g_free (fd->source_path);
    g_free (fd->dest_path);
    if (fd->source_list)
	path_list_free (fd->source_list);

    generic_dialog_close (GENERIC_DIALOG (fd));
}

FileDialog *
file_dialog_new (const gchar * title, const gchar * message,
		 const gchar * wmclass, const gchar * wmsubclass,
		 void (*cb_cancel) (FileDialog *, gpointer), gpointer data)
{
    FileDialog *fd = NULL;

    fd = g_new0 (FileDialog, 1);

    generic_dialog_setup (GENERIC_DIALOG (fd), title, message,
			  wmclass, wmsubclass, FALSE,
			  (void *) cb_cancel, data);

    return fd;
}


GtkWidget *
file_dialog_add_button (FileDialog * fd, const gchar * text,
			void (*cb_func) (FileDialog *, gpointer),
			gint is_default)
{
    return generic_dialog_add (GENERIC_DIALOG (fd), text,
			       (void *) cb_func, is_default);
}

static void
cb_file_dialog_entry (GtkWidget * widget, gpointer data)
{
    FileDialog *fd = data;
    g_free (fd->dest_path);
    fd->dest_path =
	remove_trailing_slash (gtk_entry_get_text (GTK_ENTRY (fd->entry)));
}

static void
cb_file_dialog_entry_enter (const gchar * path, gpointer data)
{
    GenericDialog *gd = data;

    cb_file_dialog_entry (NULL, data);

    if (gd->cb_default)
	gd->cb_default (gd, gd->data);
}

void
file_dialog_add_path_widgets (FileDialog * fd, const gchar * default_path,
			      const gchar * path, const gchar * filter,
			      const gchar * filter_desc)
{
    GtkWidget *list;
    GtkWidget *sep;

    if (fd->entry)
	return;

    fd->entry = gtk_entry_new ();
    gtk_box_pack_end (GTK_BOX (GENERIC_DIALOG (fd)->vbox), fd->entry, FALSE,
		      FALSE, 0);

    generic_dialog_attach_default (GENERIC_DIALOG (fd), fd->entry);
    gtk_widget_show (fd->entry);

    if (path && path[0] == '/')
    {
	fd->dest_path = g_strdup (path);
    }
    else
    {
	const gchar *base;

	base = default_path;

	if (!base)
	    base = home_dir ();

	if (path)
	{
	    fd->dest_path = concat_dir_and_file (base, path);
	}
	else
	{
	    fd->dest_path = g_strdup (base);
	}
    }

    list =
	path_selection_new_with_files (fd->entry, fd->dest_path, filter,
				       filter_desc);
    path_selection_add_select_func (fd->entry, cb_file_dialog_entry_enter,
				    fd);
    gtk_box_pack_end (GTK_BOX (GENERIC_DIALOG (fd)->vbox), list, TRUE, TRUE,
		      5);
    gtk_widget_show (list);

    sep = gtk_hseparator_new ();
    gtk_box_pack_end (GTK_BOX (GENERIC_DIALOG (fd)->vbox), sep, FALSE, FALSE,
		      0);
    gtk_widget_show (sep);

    gtk_widget_grab_focus (fd->entry);
    if (fd->dest_path)
    {
	gtk_entry_set_text (GTK_ENTRY (fd->entry), fd->dest_path);
	gtk_entry_set_position (GTK_ENTRY (fd->entry),
				strlen (fd->dest_path));
    }

    gtk_signal_connect (GTK_OBJECT (fd->entry), "changed",
			GTK_SIGNAL_FUNC (cb_file_dialog_entry), fd);
}
