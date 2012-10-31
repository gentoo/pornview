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

#include "pornview.h"

#include "dialogs.h"
#include "file_utils.h"
#include "image.h"

#include "fileutil.h"
#include "cache.h"
#include "thumbview.h"

gboolean place_dialogs_under_mouse = FALSE;
gboolean place_dialogs_center = TRUE;
gboolean confirm_delete = TRUE;

/*
 *--------------------------------------------------------------------------
 * call these when names change, files move, deleted, etc.
 * so that any open windows are also updated
 *--------------------------------------------------------------------------
 */

void
file_maint_renamed (const gchar * source, const gchar * dest)
{
    cache_maint_moved (CACHE_THUMBS, source, dest);
    cache_maint_moved (CACHE_COMMENTS, source, dest);
}

/* under most cases ignore_list should be NULL */
void
file_maint_removed (const gchar * path, GList * ignore_list)
{
    cache_maint_removed (CACHE_THUMBS, path);
    cache_maint_removed (CACHE_COMMENTS, path);
}

/* special case for correct main window behavior */
void
file_maint_moved (const gchar * source, const gchar * dest,
		  GList * ignore_list)
{
    cache_maint_moved (CACHE_THUMBS, source, dest);
    cache_maint_moved (CACHE_COMMENTS, source, dest);
}

/*
 *--------------------------------------------------------------------------
 * The file manipulation dialogs
 *--------------------------------------------------------------------------
 */

enum
{
    DIALOG_NEW_DIR,
    DIALOG_COPY,
    DIALOG_MOVE,
    DIALOG_DELETE,
    DIALOG_RENAME
};

typedef struct _FileDataMult FileDataMult;
struct _FileDataMult
{
    gint    confirm_all;
    gint    confirmed;
    gint    skip;
    GList  *source_list;
    GList  *source_next;
    gchar  *dest_base;
    gchar  *source;
    gchar  *dest;
    gint    copy;

    gint    rename;
    gint    rename_auto;
    gint    rename_all;

    GtkWidget *rename_box;
    GtkWidget *rename_entry;
    GtkWidget *rename_auto_box;
    GtkWidget *yes_all_button;

    GtkWidget *progress;
    gboolean cancel;
    gint    count;
    gint    length;
};

typedef struct _FileDataSingle FileDataSingle;
struct _FileDataSingle
{
    gint    confirmed;
    gchar  *source;
    gchar  *dest;
    gint    copy;

    gint    rename;
    gint    rename_auto;

    GtkWidget *rename_box;
    GtkWidget *rename_entry;
    GtkWidget *rename_auto_box;
};

/*
 *--------------------------------------------------------------------------
 * Adds 1 or 2 images (if 2, side by side) to a DialogGeneric
 *--------------------------------------------------------------------------
 */

#define DIALOG_DEF_IMAGE_DIM_X 200
#define DIALOG_DEF_IMAGE_DIM_Y 150

static void
generic_dialog_add_images (GenericDialog * gd, const gchar * path1,
			   const gchar * path2)
{
    ImageWindow *iw;
    GtkWidget *hbox = NULL;
    GtkWidget *vbox;
    GtkWidget *sep;
    GtkWidget *label;

    if (!path1)
	return;

    sep = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (gd->vbox), sep, FALSE, FALSE, 5);
    gtk_widget_show (sep);

    if (path2)
    {
	hbox = gtk_hbox_new (TRUE, 5);
	gtk_box_pack_start (GTK_BOX (gd->vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
    }

    /*
     * image 1 
     */

    vbox = gtk_vbox_new (FALSE, 0);
    if (hbox)
    {
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    }
    else
    {
	gtk_box_pack_start (GTK_BOX (gd->vbox), vbox, TRUE, TRUE, 0);
    }
    gtk_widget_show (vbox);

    iw = image_new (TRUE);
    gtk_widget_set_usize (iw->widget, DIALOG_DEF_IMAGE_DIM_X,
			  DIALOG_DEF_IMAGE_DIM_Y);
    gtk_box_pack_start (GTK_BOX (vbox), iw->widget, TRUE, TRUE, 0);
    image_set_path (iw, path1);
    gtk_widget_show (iw->widget);

    label = gtk_label_new (filename_from_path (path1));
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    /*
     * image 2 
     */

    if (hbox && path2)
    {
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show (vbox);

	iw = image_new (TRUE);
	gtk_widget_set_usize (iw->widget, DIALOG_DEF_IMAGE_DIM_X,
			      DIALOG_DEF_IMAGE_DIM_Y);
	gtk_box_pack_start (GTK_BOX (vbox), iw->widget, TRUE, TRUE, 0);
	image_set_path (iw, path2);
	gtk_widget_show (iw->widget);

	label = gtk_label_new (filename_from_path (path2));
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
    }
}

/*
 *--------------------------------------------------------------------------
 * Wrappers to aid in setting additional dialog properties (unde mouse, etc.)
 *--------------------------------------------------------------------------
 */

GenericDialog *
file_util_gen_dlg (const gchar * title, const gchar * message,
		   const gchar * wmclass, const gchar * wmsubclass,
		   gint auto_close, void (*cb_cancel) (GenericDialog *,
						       gpointer),
		   gpointer data)
{
    GenericDialog *gd;

    gd = generic_dialog_new (title, message, wmclass, wmsubclass, auto_close,
			     cb_cancel, data);
    if (place_dialogs_under_mouse)
	gtk_window_position (GTK_WINDOW (gd->dialog), GTK_WIN_POS_MOUSE);
    else if (place_dialogs_center)
	gtk_window_position (GTK_WINDOW (gd->dialog), GTK_WIN_POS_CENTER);

    return gd;
}

FileDialog *
file_util_file_dlg (const gchar * title, const gchar * message,
		    const gchar * wmclass, const gchar * wmsubclass,
		    void (*cb_cancel) (FileDialog *, gpointer), gpointer data)
{
    FileDialog *fd;

    fd = file_dialog_new (title, message, wmclass, wmsubclass, cb_cancel,
			  data);
    if (place_dialogs_under_mouse)
	gtk_window_position (GTK_WINDOW (GENERIC_DIALOG (fd)->dialog),
			     GTK_WIN_POS_MOUSE);
    else if (place_dialogs_center)
	gtk_window_position (GTK_WINDOW (GENERIC_DIALOG (fd)->dialog),
			     GTK_WIN_POS_CENTER);

    return fd;
}

static void
cb_file_util_warning_dialog_ok (GenericDialog * gd, gpointer data)
{
    /*
     * no op 
     */
}

GenericDialog *
file_util_warning_dialog (const gchar * title, const gchar * message)
{
    GenericDialog *gd;

    gd = file_util_gen_dlg (title, message, "PornView", "warning", TRUE, NULL,
			    NULL);
    generic_dialog_add (gd, _("Ok"), cb_file_util_warning_dialog_ok, TRUE);

    if (place_dialogs_under_mouse)
	gtk_window_position (GTK_WINDOW (gd->dialog), GTK_WIN_POS_MOUSE);
    else if (place_dialogs_center)
	gtk_window_position (GTK_WINDOW (gd->dialog), GTK_WIN_POS_CENTER);

    gtk_widget_show (gd->dialog);

    return gd;
}

/*
 *--------------------------------------------------------------------------
 * Move and Copy routines
 *--------------------------------------------------------------------------
 */

/*
 * Multi file move
 */

static FileDataMult *
file_data_multiple_new (GList * source_list, const gchar * dest, gint copy)
{
    FileDataMult *fdm = g_new0 (FileDataMult, 1);
    fdm->confirm_all = FALSE;
    fdm->confirmed = FALSE;
    fdm->skip = FALSE;
    fdm->source_list = source_list;
    fdm->source_next = fdm->source_list;
    fdm->dest_base = g_strdup (dest);
    fdm->source = NULL;
    fdm->dest = NULL;
    fdm->copy = copy;
    fdm->progress = NULL;
    fdm->cancel = FALSE;
    fdm->count = 0;
    fdm->length = g_list_length (source_list);
    return fdm;
}

static void
file_data_multiple_free (FileDataMult * fdm)
{
    if (fdm->progress)
    {
	gtk_grab_remove (fdm->progress);
	gtk_widget_destroy (fdm->progress);
    }

    if (fdm->count > 0 && !fdm->copy)
	thumbview_refresh ();

    path_list_free (fdm->source_list);
    g_free (fdm->dest_base);
    g_free (fdm->dest);
    g_free (fdm);
}

static void file_util_move_multiple (FileDataMult * fdm);

static void
cb_file_util_move_multiple_ok (GenericDialog * gd, gpointer data)
{
    FileDataMult *fdm = data;

    fdm->confirmed = TRUE;

    if (fdm->rename_auto)
    {
	gchar  *buf;

	buf = unique_filename_simple (fdm->dest);
	if (buf)
	{
	    g_free (fdm->dest);
	    fdm->dest = buf;
	}
	else
	{
	    /*
	     * unique failed? well, return to the overwrite prompt :( 
	     */
	    fdm->confirmed = FALSE;
	}
    }
    else if (fdm->rename)
    {
	const gchar *name;

	name = gtk_entry_get_text (GTK_ENTRY (fdm->rename_entry));

	if (strlen (name) == 0 ||
	    strcmp (name, filename_from_path (fdm->source)) == 0)
	{
	    fdm->confirmed = FALSE;
	}
	else
	{
	    g_free (fdm->dest);
	    fdm->dest = concat_dir_and_file (fdm->dest_base, name);
	    fdm->confirmed = !isname (fdm->dest);
	}
    }

    gtk_widget_hide (gd->dialog);

    file_util_move_multiple (fdm);
}

static void
cb_file_util_move_multiple_all (GenericDialog * gd, gpointer data)
{
    FileDataMult *fdm = data;

    gtk_widget_hide (gd->dialog);

    fdm->confirm_all = TRUE;

    if (fdm->rename_auto)
	fdm->rename_all = TRUE;

    file_util_move_multiple (fdm);
}

static void
cb_file_util_move_multiple_skip (GenericDialog * gd, gpointer data)
{
    FileDataMult *fdm = data;

    gtk_widget_hide (gd->dialog);

    fdm->skip = TRUE;
    file_util_move_multiple (fdm);
}

static void
cb_file_util_move_multiple_skip_all (GenericDialog * gd, gpointer data)
{
    FileDataMult *fdm = data;

    gtk_widget_hide (gd->dialog);

    fdm->skip = TRUE;
    fdm->confirm_all = TRUE;

    file_util_move_multiple (fdm);
}

static void
cb_file_util_move_multiple_cancel (GenericDialog * gd, gpointer data)
{
    FileDataMult *fdm = data;

    gtk_widget_hide (gd->dialog);

    file_data_multiple_free (fdm);
}

/* rename option */

static void
cb_file_util_move_multiple_rename_auto (GtkWidget * widget, gpointer data)
{
    GenericDialog *gd = data;
    FileDataMult *fdm;

    fdm = gd->data;

    fdm->rename_auto =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    gtk_widget_set_sensitive (fdm->rename_box, !fdm->rename_auto);
    gtk_widget_set_sensitive (fdm->rename_entry,
			      (!fdm->rename_auto && fdm->rename));

    gtk_widget_set_sensitive (fdm->yes_all_button,
			      (fdm->rename_auto || !fdm->rename));
}

static void
cb_file_util_move_multiple_rename (GtkWidget * widget, gpointer data)
{
    GenericDialog *gd = data;
    FileDataMult *fdm;

    fdm = gd->data;

    fdm->rename = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    gtk_widget_set_sensitive (fdm->rename_entry, fdm->rename);
    gtk_widget_set_sensitive (fdm->yes_all_button, !fdm->rename);

    if (fdm->rename)
	gtk_widget_grab_focus (fdm->rename_entry);
}

static void
file_util_move_multiple (FileDataMult * fdm)
{
    while (fdm->dest || fdm->source_next)
    {
	if (!fdm->dest)
	{
	    GList  *work = fdm->source_next;
	    fdm->source = work->data;
	    fdm->dest =
		concat_dir_and_file (fdm->dest_base,
				     filename_from_path (fdm->source));
	    fdm->source_next = work->next;
	}

	if (fdm->dest && fdm->source && strcmp (fdm->dest, fdm->source) == 0)
	{
	    GenericDialog *gd;
	    const gchar *title;
	    gchar  *text;

	    if (fdm->progress)
	    {
		gtk_grab_remove (fdm->progress);
		gtk_widget_destroy (fdm->progress);
		fdm->progress = NULL;
		fdm->cancel = FALSE;
	    }

	    if (fdm->copy)
	    {
		title = _("Source to copy matches destination");
		text =
		    g_strdup_printf (_
				     ("Unable to copy file:\n%s\nto itself."),
				     fdm->dest);
	    }
	    else
	    {
		title = _("Source to move matches destination");
		text =
		    g_strdup_printf (_
				     ("Unable to move file:\n%s\nto itself."),
				     fdm->dest);
	    }

	    gd = file_util_gen_dlg (title, text, "PornView", "dlg_confirm",
				    TRUE, cb_file_util_move_multiple_cancel,
				    fdm);
	    g_free (text);
	    generic_dialog_add (gd, _("Continue"),
				cb_file_util_move_multiple_skip, TRUE);

	    gtk_widget_show (gd->dialog);
	    return;
	}
	else if (isfile (fdm->dest) && !fdm->confirmed && !fdm->confirm_all
		 && !fdm->skip)
	{
	    GenericDialog *gd;
	    gchar  *text;

	    GtkWidget *hbox;

	    if (fdm->progress)
	    {
		gtk_grab_remove (fdm->progress);
		gtk_widget_destroy (fdm->progress);
		fdm->progress = NULL;
		fdm->cancel = FALSE;
	    }

	    text =
		g_strdup_printf (_("Overwrite file:\n %s\n with:\n %s"),
				 fdm->dest, fdm->source);
	    gd = file_util_gen_dlg (_("Overwrite file"), text, "PornView",
				    "dlg_confirm", TRUE,
				    cb_file_util_move_multiple_cancel, fdm);
	    g_free (text);

	    generic_dialog_add (gd, _("Yes"), cb_file_util_move_multiple_ok,
				TRUE);
	    fdm->yes_all_button =
		generic_dialog_add (gd, _("Yes to all"),
				    cb_file_util_move_multiple_all, FALSE);
	    generic_dialog_add (gd, _("Skip"),
				cb_file_util_move_multiple_skip, FALSE);
	    generic_dialog_add (gd, _("Skip all"),
				cb_file_util_move_multiple_skip_all, FALSE);
	    generic_dialog_add_images (gd, fdm->dest, fdm->source);

	    /*
	     * rename option 
	     */

	    fdm->rename = FALSE;
	    fdm->rename_all = FALSE;
	    fdm->rename_auto = FALSE;

	    hbox = gtk_hbox_new (FALSE, 5);
	    gtk_box_pack_start (GTK_BOX (gd->vbox), hbox, FALSE, FALSE, 0);
	    gtk_widget_show (hbox);

	    fdm->rename_auto_box =
		gtk_check_button_new_with_label (_("Auto rename"));
	    gtk_signal_connect (GTK_OBJECT (fdm->rename_auto_box), "clicked",
				GTK_SIGNAL_FUNC
				(cb_file_util_move_multiple_rename_auto), gd);
	    gtk_box_pack_start (GTK_BOX (hbox), fdm->rename_auto_box, FALSE,
				FALSE, 0);
	    gtk_widget_show (fdm->rename_auto_box);

	    hbox = gtk_hbox_new (FALSE, 5);
	    gtk_box_pack_start (GTK_BOX (gd->vbox), hbox, FALSE, FALSE, 0);
	    gtk_widget_show (hbox);

	    fdm->rename_box = gtk_check_button_new_with_label (_("Rename"));
	    gtk_signal_connect (GTK_OBJECT (fdm->rename_box), "clicked",
				GTK_SIGNAL_FUNC
				(cb_file_util_move_multiple_rename), gd);
	    gtk_box_pack_start (GTK_BOX (hbox), fdm->rename_box, FALSE, FALSE,
				0);
	    gtk_widget_show (fdm->rename_box);

	    fdm->rename_entry = gtk_entry_new ();
	    gtk_entry_set_text (GTK_ENTRY (fdm->rename_entry),
				filename_from_path (fdm->dest));
	    gtk_widget_set_sensitive (fdm->rename_entry, FALSE);
	    gtk_box_pack_start (GTK_BOX (hbox), fdm->rename_entry, TRUE, TRUE,
				0);
	    gtk_widget_show (fdm->rename_entry);

	    gtk_widget_show (gd->dialog);
	    return;
	}
	else
	{
	    gint    success = FALSE;
	    if (fdm->skip)
	    {
		success = TRUE;
		if (!fdm->confirm_all)
		    fdm->skip = FALSE;
	    }
	    else
	    {
		gint    try = TRUE;

		if (fdm->confirm_all && fdm->rename_all && isfile (fdm->dest))
		{
		    gchar  *buf;
		    buf = unique_filename_simple (fdm->dest);
		    if (buf)
		    {
			g_free (fdm->dest);
			fdm->dest = buf;
		    }
		    else
		    {
			try = FALSE;
		    }
		}

		if (try)
		{
		    if (!fdm->progress)
			fdm->progress =
			    dialog_progress_create (_("Pornview - copy/move"),
						    _(" "), &fdm->cancel, 300,
						    -1);
		    gtk_grab_add (fdm->progress);


		    if (fdm->copy)
		    {
			gfloat  c;
			c = (gfloat) fdm->count / (gfloat) fdm->length;
			dialog_progress_update (fdm->progress,
						_("PornView - copy"),
						g_basename (fdm->source),
						_(" "), c);

			if (fdm->cancel)
			    break;
			success = copy_file (fdm->source, fdm->dest);
			fdm->count++;
		    }
		    else
		    {
			gfloat  c;

			c = (gfloat) fdm->count / (gfloat) fdm->length;
			dialog_progress_update (fdm->progress,
						_("PornView - move"),
						g_basename (fdm->source),
						_(" "), c);

			if (fdm->cancel)
			    break;

			while (gtk_events_pending ())
			    gtk_main_iteration ();

			fdm->count++;

			if (move_file (fdm->source, fdm->dest))
			{
			    success = TRUE;
			    file_maint_moved (fdm->source, fdm->dest,
					      fdm->source_list);

			}
		    }
		}
	    }
	    if (!success)
	    {
		GenericDialog *gd;
		const gchar *title;
		gchar  *text;

		if (fdm->progress)
		{
		    gtk_grab_remove (fdm->progress);
		    gtk_widget_destroy (fdm->progress);
		    fdm->progress = NULL;
		    fdm->cancel = FALSE;
		}

		if (fdm->copy)
		{
		    title = _("Error copying file");
		    text =
			g_strdup_printf (_
					 ("Unable to copy file:\n%sto:\n%s\n during multiple file copy."),
					 fdm->source, fdm->dest);
		}
		else
		{
		    title = _("Error moving file");
		    text =
			g_strdup_printf (_
					 ("Unable to move file:\n%sto:\n%s\n during multiple file move."),
					 fdm->source, fdm->dest);
		}
		gd = file_util_gen_dlg (title, text, "PornView",
					"dlg_confirm", TRUE,
					cb_file_util_move_multiple_cancel,
					fdm);
		g_free (text);

		generic_dialog_add (gd, _("Continue"),
				    cb_file_util_move_multiple_skip, TRUE);

		gtk_widget_show (gd->dialog);
		return;
	    }
	    fdm->confirmed = FALSE;
	    g_free (fdm->dest);
	    fdm->dest = NULL;
	}
    }

    file_data_multiple_free (fdm);
}

/*
 * Single file move
 */

static FileDataSingle *
file_data_single_new (const gchar * source, const gchar * dest, gint copy)
{
    FileDataSingle *fds = g_new0 (FileDataSingle, 1);
    fds->confirmed = FALSE;
    fds->source = g_strdup (source);
    fds->dest = g_strdup (dest);
    fds->copy = copy;
    return fds;
}

static void
file_data_single_free (FileDataSingle * fds)
{
    g_free (fds->source);
    g_free (fds->dest);
    g_free (fds);
}

static void file_util_move_single (FileDataSingle * fds);

static void
cb_file_util_move_single_ok (GenericDialog * gd, gpointer data)
{
    FileDataSingle *fds = data;

    fds->confirmed = TRUE;

    if (fds->rename_auto)
    {
	gchar  *buf;

	buf = unique_filename_simple (fds->dest);
	if (buf)
	{
	    g_free (fds->dest);
	    fds->dest = buf;
	}
	else
	{
	    /*
	     * unique failed? well, return to the overwrite prompt :( 
	     */
	    fds->confirmed = FALSE;
	}
    }
    else if (fds->rename)
    {
	const gchar *name;

	name = gtk_entry_get_text (GTK_ENTRY (fds->rename_entry));
	if (strlen (name) == 0 ||
	    strcmp (name, filename_from_path (fds->source)) == 0)
	{
	    fds->confirmed = FALSE;
	}
	else
	{
	    gchar  *base;

	    base = remove_level_from_path (fds->dest);
	    g_free (fds->dest);
	    fds->dest = concat_dir_and_file (base, name);
	    fds->confirmed = !isname (fds->dest);

	    g_free (base);
	}
    }

    file_util_move_single (fds);
}

static void
cb_file_util_move_single_cancel (GenericDialog * gd, gpointer data)
{
    FileDataSingle *fds = data;

    file_data_single_free (fds);
}

static void
cb_file_util_move_single_rename_auto (GtkWidget * widget, gpointer data)
{
    GenericDialog *gd = data;
    FileDataSingle *fds;

    fds = gd->data;

    fds->rename_auto =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    gtk_widget_set_sensitive (fds->rename_box, !fds->rename_auto);
    gtk_widget_set_sensitive (fds->rename_entry,
			      (!fds->rename_auto && fds->rename));
}

static void
cb_file_util_move_single_rename (GtkWidget * widget, gpointer data)
{
    GenericDialog *gd = data;
    FileDataSingle *fds;

    fds = gd->data;

    fds->rename = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    gtk_widget_set_sensitive (fds->rename_entry, fds->rename);

    if (fds->rename)
	gtk_widget_grab_focus (fds->rename_entry);
}

static void
file_util_move_single (FileDataSingle * fds)
{
    if (fds->dest && fds->source && strcmp (fds->dest, fds->source) == 0)
    {
	file_util_warning_dialog (_("Source matches destination"),
				  _
				  ("Source and destination are the same, operation cancelled."));
    }
    else if (isfile (fds->dest) && !fds->confirmed)
    {
	GenericDialog *gd;
	gchar  *text;

	GtkWidget *hbox;

	text =
	    g_strdup_printf (_("Overwrite file:\n%s\n with:\n%s"), fds->dest,
			     fds->source);
	gd = file_util_gen_dlg (_("Overwrite file"), text, "PornView",
				"dlg_confirm", TRUE,
				cb_file_util_move_single_cancel, fds);
	g_free (text);

	generic_dialog_add (gd, _("Overwrite"), cb_file_util_move_single_ok,
			    TRUE);
	generic_dialog_add_images (gd, fds->dest, fds->source);

	/*
	 * rename option 
	 */

	fds->rename = FALSE;
	fds->rename_auto = FALSE;

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (gd->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	fds->rename_auto_box =
	    gtk_check_button_new_with_label (_("Auto rename"));
	gtk_signal_connect (GTK_OBJECT (fds->rename_auto_box), "clicked",
			    GTK_SIGNAL_FUNC
			    (cb_file_util_move_single_rename_auto), gd);
	gtk_box_pack_start (GTK_BOX (hbox), fds->rename_auto_box, FALSE,
			    FALSE, 0);
	gtk_widget_show (fds->rename_auto_box);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (gd->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	fds->rename_box = gtk_check_button_new_with_label (_("Rename"));
	gtk_signal_connect (GTK_OBJECT (fds->rename_box), "clicked",
			    GTK_SIGNAL_FUNC (cb_file_util_move_single_rename),
			    gd);
	gtk_box_pack_start (GTK_BOX (hbox), fds->rename_box, FALSE, FALSE, 0);
	gtk_widget_show (fds->rename_box);

	fds->rename_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (fds->rename_entry),
			    filename_from_path (fds->dest));
	gtk_widget_set_sensitive (fds->rename_entry, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), fds->rename_entry, TRUE, TRUE, 0);
	gtk_widget_show (fds->rename_entry);

	gtk_widget_show (gd->dialog);
	return;
    }
    else
    {
	gint    success = FALSE;
	if (fds->copy)
	{
	    success = copy_file (fds->source, fds->dest);
	}
	else
	{
	    if (move_file (fds->source, fds->dest))
	    {
		success = TRUE;
		file_maint_moved (fds->source, fds->dest, NULL);
		thumbview_refresh ();
	    }
	}
	if (!success)
	{
	    gchar  *title;
	    gchar  *text;
	    if (fds->copy)
	    {
		title = _("Error copying file");
		text =
		    g_strdup_printf (_("Unable to copy file:\n%s\nto:\n%s"),
				     fds->source, fds->dest);
	    }
	    else
	    {
		title = _("Error moving file");
		text =
		    g_strdup_printf (_("Unable to move file:\n%s\nto:\n%s"),
				     fds->source, fds->dest);
	    }
	    file_util_warning_dialog (title, text);
	    g_free (text);
	}
    }

    file_data_single_free (fds);
}

/*
 * file move dialog
 */

static void
file_util_move_do (FileDialog * fd)
{

    gtk_widget_hide (fd->gd.dialog);

    if (fd->multiple_files)
    {
	file_util_move_multiple (file_data_multiple_new
				 (fd->source_list, fd->dest_path, fd->type));
	fd->source_list = NULL;
    }
    else
    {
	if (isdir (fd->dest_path))
	{
	    gchar  *buf = concat_dir_and_file (fd->dest_path,
					       filename_from_path (fd->
								   source_path));
	    gtk_entry_set_text (GTK_ENTRY (fd->entry), buf);
	    g_free (buf);
	}
	file_util_move_single (file_data_single_new
			       (fd->source_path, fd->dest_path, fd->type));
    }

    file_dialog_close (fd);
}

static void
file_util_move_check (FileDialog * fd)
{
    if (fd->multiple_files && !isdir (fd->dest_path))
    {
	if (isfile (fd->dest_path))
	{
	    file_util_warning_dialog (_("Invalid destination"),
				      _
				      ("When operating with multiple files, please select\n a directory, not file."));
	}
	else
	    file_util_warning_dialog (_("Invalid directory"),
				      _
				      ("Please select an existing directory"));
	return;
    }

    file_util_move_do (fd);
}

static void
cb_file_util_move (FileDialog * fd, gpointer data)
{
    file_util_move_check (fd);
}

static void
cb_file_util_move_cancel (FileDialog * fd, gpointer data)
{
    file_dialog_close (fd);
}

static void
real_file_util_move (const gchar * source_path, GList * source_list,
		     const gchar * dest_path, gint copy)
{
    FileDialog *fd;
    gchar  *path = NULL;
    gint    multiple;
    gchar  *text;
    const gchar *title;
    const gchar *op_text;

    if (!source_path && !source_list)
	return;

    if (source_path)
    {
	path = g_strdup (source_path);
	multiple = FALSE;
    }
    else if (source_list->next)
    {
	multiple = TRUE;
    }
    else
    {
	path = g_strdup (source_list->data);
	path_list_free (source_list);
	source_list = NULL;
	multiple = FALSE;
    }

    if (copy)
    {
	title = _("PornView - copy");
	op_text = _("Copy");
	if (path)
	    text = g_strdup_printf (_("Copy file:\n%s\nto:"), path);
	else
	    text = g_strdup (_("Copy multiple files to:"));
    }
    else
    {
	title = _("PornView - move");
	op_text = _("Move");
	if (path)
	    text = g_strdup_printf (_("Move file:\n%s\nto:"), path);
	else
	    text = g_strdup (_("Move multiple files to:"));
    }

    fd = file_util_file_dlg (title, text, "PornView", "dlg_copymove",
			     cb_file_util_move_cancel, NULL);
    g_free (text);

    file_dialog_add_button (fd, op_text, cb_file_util_move, TRUE);

    file_dialog_add_path_widgets (fd, NULL, dest_path, NULL, NULL);

    fd->type = copy;
    fd->source_path = path;
    fd->source_list = source_list;
    fd->multiple_files = multiple;

    gtk_widget_show (GENERIC_DIALOG (fd)->dialog);
}

void
file_util_move (const gchar * source_path, GList * source_list,
		const gchar * dest_path)
{
    real_file_util_move (source_path, source_list, dest_path, FALSE);
}

void
file_util_copy (const gchar * source_path, GList * source_list,
		const gchar * dest_path)
{
    real_file_util_move (source_path, source_list, dest_path, TRUE);
}

void
file_util_move_simple (GList * list, const gchar * dest_path)
{
    if (!list)
	return;
    if (!dest_path)
    {
	path_list_free (list);
	return;
    }

    if (!list->next)
    {
	const gchar *source;
	gchar  *dest;

	source = list->data;
	dest = concat_dir_and_file (dest_path, filename_from_path (source));

	file_util_move_single (file_data_single_new (source, dest, FALSE));
	g_free (dest);
	path_list_free (list);
	return;
    }

    file_util_move_multiple (file_data_multiple_new (list, dest_path, FALSE));
}

void
file_util_copy_simple (GList * list, const gchar * dest_path)
{
    if (!list)
	return;
    if (!dest_path)
    {
	path_list_free (list);
	return;
    }

    if (!list->next)
    {
	const gchar *source;
	gchar  *dest;

	source = list->data;
	dest = concat_dir_and_file (dest_path, filename_from_path (source));

	file_util_move_single (file_data_single_new (source, dest, TRUE));
	g_free (dest);
	path_list_free (list);
	return;
    }

    file_util_move_multiple (file_data_multiple_new (list, dest_path, TRUE));
}

/*
 *--------------------------------------------------------------------------
 * Delete routines
 *--------------------------------------------------------------------------
 */

/*
 * delete multiple files
 */

static void cb_file_util_delete_multiple_ok (GenericDialog * gd,
					     gpointer data);
static void cb_file_util_delete_multiple_cancel (GenericDialog * gd,
						 gpointer data);

static void
cb_file_util_delete_multiple_ok (GenericDialog * gd, gpointer data)
{
    GList  *source_list = data;
    GtkWidget *progress = NULL;
    gboolean cancel = FALSE;
    gint    count = 0;
    gint    length = g_list_length (source_list);

    gtk_widget_hide (gd->dialog);


    while (source_list)
    {
	gfloat  c;
	gchar  *path = source_list->data;

	source_list = g_list_remove (source_list, path);

	if (!progress)
	    progress =
		dialog_progress_create (_("Pornview - delete"), _(" "),
					&cancel, 300, -1);
	gtk_grab_add (progress);


	c = (gfloat) count / (gfloat) length;
	dialog_progress_update (progress, _("PornView - delete"),
				g_basename (path), _(" "), c);

	if (cancel)
	{
	    g_free (path);
	    break;
	}

	while (gtk_events_pending ())
	    gtk_main_iteration ();

	if (unlink (path) < 0)
	{
	    if (progress)
	    {
		gtk_grab_remove (progress);
		gtk_widget_destroy (progress);
		progress = NULL;
		cancel = FALSE;
	    }

	    if (source_list)
	    {
		GenericDialog *d;
		gchar  *text;

		text =
		    g_strdup_printf (_
				     ("Unable to delete file:\n %s\n Continue multiple delete operation?"),
				     path);
		d = file_util_gen_dlg (_("Delete failed"), text, "PornView",
				       "dlg_confirm", TRUE,
				       cb_file_util_delete_multiple_cancel,
				       source_list);
		g_free (text);

		generic_dialog_add (d, _("Continue"),
				    cb_file_util_delete_multiple_ok, TRUE);
		gtk_widget_show (gd->dialog);
	    }
	    else
	    {
		gchar  *text;

		text =
		    g_strdup_printf (_("Unable to delete file:\n%s"), path);
		file_util_warning_dialog (_("Delete failed"), text);
		g_free (text);
	    }
	    g_free (path);
	    return;
	}
	else
	{
	    count++;
	    file_maint_removed (path, source_list);
	}
	g_free (path);
    }

    if (progress)
    {
	gtk_grab_remove (progress);
	gtk_widget_destroy (progress);
	progress = NULL;
	cancel = FALSE;
    }

    if (count > 0)
	thumbview_refresh ();
}

static void
cb_file_util_delete_multiple_cancel (GenericDialog * gd, gpointer data)
{
    GList  *source_list = data;

    path_list_free (source_list);
}

static void
file_util_delete_multiple (GList * source_list)
{
    if (!confirm_delete)
    {
	cb_file_util_delete_multiple_ok (NULL, source_list);
    }
    else
    {
	GenericDialog *gd;

	gd = file_util_gen_dlg (_("Delete files"),
				_("About to delete multiple files..."),
				"PornView", "dlg_confirm", TRUE,
				cb_file_util_delete_multiple_cancel,
				source_list);

	generic_dialog_add (gd, _("Delete"), cb_file_util_delete_multiple_ok,
			    TRUE);

	gtk_widget_show (gd->dialog);
    }
}

/*
 * delete single file
 */

static void
cb_file_util_delete_ok (GenericDialog * gd, gpointer data)
{
    gchar  *path = data;

    if (unlink (path) < 0)
    {
	gchar  *text =
	    g_strdup_printf (_("Unable to delete file:\n%s"), path);
	file_util_warning_dialog (_("File deletion failed"), text);
	g_free (text);
    }
    else
    {
	file_maint_removed (path, NULL);
	thumbview_refresh ();
    }

    g_free (path);
}

static void
cb_file_util_delete_cancel (GenericDialog * gd, gpointer data)
{
    gchar  *path = data;

    g_free (path);
}

static void
file_util_delete_single (const gchar * path)
{
    gchar  *buf = g_strdup (path);

    if (!confirm_delete)
    {
	cb_file_util_delete_ok (NULL, buf);
    }
    else
    {
	GenericDialog *gd;
	gchar  *text;

	text = g_strdup_printf (_("About to delete the file:\n %s"), buf);
	gd = file_util_gen_dlg (_("Delete file"), text, "PornView",
				"dlg_confirm", TRUE,
				cb_file_util_delete_cancel, buf);
	g_free (text);

	generic_dialog_add (gd, _("Delete"), cb_file_util_delete_ok, TRUE);

	gtk_widget_show (gd->dialog);
    }
}

void
file_util_delete (const gchar * source_path, GList * source_list)
{
    if (!source_path && !source_list)
	return;

    if (source_path)
    {
	file_util_delete_single (source_path);
    }
    else if (!source_list->next)
    {
	file_util_delete_single (source_list->data);
	path_list_free (source_list);
    }
    else
    {
	file_util_delete_multiple (source_list);
    }
}

/*
 *--------------------------------------------------------------------------
 * Rename routines
 *--------------------------------------------------------------------------
 */

/*
 * rename multiple files
 */

typedef struct _RenameDataMult RenameDataMult;
struct _RenameDataMult
{
    FileDialog *fd;

    const gchar *source;
    gint    rename_auto;

    GtkWidget *clist;
    GtkWidget *button_auto;

    GtkWidget *rename_box;
    GtkWidget *rename_label;
    GtkWidget *rename_entry;

    GtkWidget *auto_box;
    GtkWidget *auto_entry_front;
    GtkWidget *auto_spin_start;
    GtkWidget *auto_spin_pad;
    GtkWidget *auto_entry_end;
};

static void file_util_rename_multiple (RenameDataMult * rd);

static void
cb_file_util_rename_multiple_ok (GenericDialog * gd, gpointer data)
{
    RenameDataMult *rd = data;
    GtkWidget *dialog;

    dialog = GENERIC_DIALOG (rd->fd)->dialog;
    if (!GTK_WIDGET_VISIBLE (dialog))
	gtk_widget_show (dialog);

    rd->fd->type = TRUE;
    file_util_rename_multiple (rd);
}

static void
cb_file_util_rename_multiple_cancel (GenericDialog * gd, gpointer data)
{
    RenameDataMult *rd = data;
    GtkWidget *dialog;

    dialog = GENERIC_DIALOG (rd->fd)->dialog;
    if (!GTK_WIDGET_VISIBLE (dialog))
	gtk_widget_show (dialog);
}

static void
file_util_rename_multiple (RenameDataMult * rd)
{
    FileDialog *fd;

    fd = rd->fd;

    if (isfile (fd->dest_path) && !fd->type)
    {
	GenericDialog *gd;
	gchar  *text;

	text =
	    g_strdup_printf (_("Overwrite file:\n%s\nby renaming:\n%s"),
			     fd->dest_path, fd->source_path);
	gd = file_util_gen_dlg (_("Overwrite file"), text, "PornView",
				"dlg_confirm", TRUE,
				cb_file_util_rename_multiple_cancel, rd);
	g_free (text);

	generic_dialog_add (gd, _("Overwrite"),
			    cb_file_util_rename_multiple_ok, TRUE);
	generic_dialog_add_images (gd, fd->dest_path, fd->source_path);

	gtk_widget_hide (GENERIC_DIALOG (fd)->dialog);

	gtk_widget_show (gd->dialog);
	return;
    }
    else
    {
	if (rename (fd->source_path, fd->dest_path) < 0)
	{
	    gchar  *text =
		g_strdup_printf (_("Unable to rename file:\n%s\n to:\n%s"),
				 filename_from_path (fd->source_path),
				 filename_from_path (fd->dest_path));
	    file_util_warning_dialog (_("Error renaming file"), text);
	    g_free (text);
	}
	else
	{
	    gint    row;
	    gint    n;

	    file_maint_renamed (fd->source_path, fd->dest_path);
	    thumbview_refresh ();

	    row =
		gtk_clist_find_row_from_data (GTK_CLIST (rd->clist),
					      (gchar *) rd->source);

	    n = GTK_CLIST (rd->clist)->rows;
	    if (n - 1 > row)
		n = row;
	    else if (n > 1)
		n = row - 1;
	    else
		n = -1;

	    if (n >= 0)
	    {
		rd->source = NULL;
		gtk_clist_remove (GTK_CLIST (rd->clist), row);
		gtk_clist_select_row (GTK_CLIST (rd->clist), n, -1);
	    }
	    else
	    {
		file_dialog_close (rd->fd);
		g_free (rd);
	    }
	}
    }
}

static void
file_util_rename_multiple_auto (RenameDataMult * rd)
{
    const gchar *front;
    const gchar *end;
    gint    start_n;
    gint    padding;
    gint    n;
    gint    row;
    gint    success;
    gint    count = 0;

    front = gtk_entry_get_text (GTK_ENTRY (rd->auto_entry_front));
    end = gtk_entry_get_text (GTK_ENTRY (rd->auto_entry_end));
    start_n =
	gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
					  (rd->auto_spin_start));
    padding =
	gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
					  (rd->auto_spin_pad));

    /*
     * first check for name conflicts 
     */
    success = TRUE;
    n = start_n;
    row = 0;
    while (row < GTK_CLIST (rd->clist)->rows && success)
    {
	gchar  *dest;
	gchar  *base;
	const gchar *path;

	path = gtk_clist_get_row_data (GTK_CLIST (rd->clist), row);

	base = remove_level_from_path (path);
	dest = g_strdup_printf ("%s/%s%0*d%s", base, front, padding, n, end);
	if (isname (dest))
	    success = FALSE;
	g_free (dest);
	g_free (base);

	n++;
	row++;
    }

    if (!success)
    {
	dialog_message (_("Auto rename"),
			_
			("Can not auto rename with the selected\nnumber set, one or more files exist that\nmatch the resulting name list.\n"),
			NULL);
	return;
    }

    /*
     * now do it for real 
     */
    success = TRUE;
    n = start_n;
    while (GTK_CLIST (rd->clist)->rows > 0 && success)
    {
	gchar  *dest;
	gchar  *base;

	base = remove_level_from_path (rd->source);
	dest = g_strdup_printf ("%s/%s%0*d%s", base, front, padding, n, end);

	while (gtk_events_pending ())
	    gtk_main_iteration ();

	if (rename (rd->source, dest) < 0)
	{
	    success = FALSE;
	}
	else
	{
	    count++;
	    file_maint_renamed (rd->source, dest);
	}

	g_free (dest);
	g_free (base);

	if (success)
	{
	    rd->source = NULL;
	    gtk_clist_remove (GTK_CLIST (rd->clist), 0);
	    if (GTK_CLIST (rd->clist)->rows > 0)
	    {
		gtk_clist_select_row (GTK_CLIST (rd->clist), 0, -1);
	    }
	}

	n++;
    }

    if (!success)
    {
	gchar  *buf;

	n--;
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (rd->auto_spin_start),
				   (float) n);

	buf =
	    g_strdup_printf (_("Failed to rename\n%s\nThe number was %d."),
			     filename_from_path (rd->source), n);
	dialog_message (_("Auto rename"), buf, NULL);
	g_free (buf);

	return;
    }

    file_dialog_close (rd->fd);
    g_free (rd);

    if (count > 0)
	thumbview_refresh ();
}

static void
cb_file_util_rename_multiple (FileDialog * fd, gpointer data)
{
    RenameDataMult *rd = data;
    gchar  *base;
    const gchar *name;

    if (rd->rename_auto)
    {
	file_util_rename_multiple_auto (rd);
	return;
    }

    name = gtk_entry_get_text (GTK_ENTRY (rd->rename_entry));
    base = remove_level_from_path (fd->source_path);

    g_free (fd->dest_path);
    fd->dest_path = concat_dir_and_file (base, name);
    g_free (base);

    if (strlen (name) == 0 || strcmp (fd->source_path, fd->dest_path) == 0)
    {
	return;
    }

    fd->type = FALSE;
    file_util_rename_multiple (rd);
}

static void
cb_file_util_rename_multiple_close (FileDialog * fd, gpointer data)
{
    RenameDataMult *rd = data;

    file_dialog_close (rd->fd);
    g_free (rd);
}

static void
cb_file_util_rename_multiple_select (GtkWidget * clist, gint row, gint column,
				     GdkEventButton * bevent, gpointer data)
{
    RenameDataMult *rd = data;
    const gchar *name;
    const gchar *path;

    path = gtk_clist_get_row_data (GTK_CLIST (clist), row);

    g_free (rd->fd->source_path);
    rd->fd->source_path = g_strdup (path);

    rd->source = path;

    name = filename_from_path (rd->fd->source_path);
    gtk_label_set (GTK_LABEL (rd->rename_label), name);
    gtk_entry_set_text (GTK_ENTRY (rd->rename_entry), name);

    if (GTK_WIDGET_VISIBLE (rd->rename_box))
    {
	gtk_widget_grab_focus (rd->rename_entry);
    }
}

static void
file_util_rename_mulitple_auto_toggle (GtkWidget * widget, gpointer data)
{
    RenameDataMult *rd = data;

    rd->rename_auto =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (rd->button_auto));

    if (rd->rename_auto)
    {
	if (GTK_WIDGET_VISIBLE (rd->rename_box))
	    gtk_widget_hide (rd->rename_box);
	if (!GTK_WIDGET_VISIBLE (rd->auto_box))
	    gtk_widget_show (rd->auto_box);
    }
    else
    {
	if (GTK_WIDGET_VISIBLE (rd->auto_box))
	    gtk_widget_hide (rd->auto_box);
	if (!GTK_WIDGET_VISIBLE (rd->rename_box))
	    gtk_widget_show (rd->rename_box);
    }
}

static GtkWidget *
furm_simple_label (GtkWidget * box, const gchar * text)
{
    GtkWidget *hbox;
    GtkWidget *label;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new (text);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    return label;
}

static GtkWidget *
furm_simple_vlabel (GtkWidget * box, const gchar * text)
{
    GtkWidget *vbox;
    GtkWidget *label;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    label = gtk_label_new (text);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    return vbox;
}

static void
file_util_rename_multiple_do (GList * source_list)
{
    RenameDataMult *rd;
    GtkWidget *scrolled;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *box2;
    GtkWidget *label;
    GtkObject *adj;
    GList  *work;

    rd = g_new0 (RenameDataMult, 1);

    rd->fd =
	file_util_file_dlg (_("PornView - rename"),
			    _("Rename multiple files:"), "PornView",
			    "dlg_rename", cb_file_util_rename_multiple_close,
			    rd);
    file_dialog_add_button (rd->fd, _("Rename"), cb_file_util_rename_multiple,
			    TRUE);

    rd->fd->source_path = g_strdup (source_list->data);
    rd->fd->dest_path = NULL;

    vbox = GENERIC_DIALOG (rd->fd)->vbox;

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
    gtk_widget_show (scrolled);

    rd->clist = gtk_clist_new (1);
    gtk_clist_set_column_auto_resize (GTK_CLIST (rd->clist), 0, TRUE);
    gtk_clist_set_reorderable (GTK_CLIST (rd->clist), TRUE);
    gtk_signal_connect (GTK_OBJECT (rd->clist), "select_row",
			GTK_SIGNAL_FUNC (cb_file_util_rename_multiple_select),
			rd);
    gtk_widget_set_usize (rd->clist, 250, 150);
    gtk_container_add (GTK_CONTAINER (scrolled), rd->clist);
    gtk_widget_show (rd->clist);

    rd->source = source_list->data;

    work = source_list;
    while (work)
    {
	gint    row;
	gchar  *buf[2];
	buf[0] = (gchar *) filename_from_path (work->data);
	buf[1] = NULL;
	row = gtk_clist_append (GTK_CLIST (rd->clist), buf);
	gtk_clist_set_row_data_full (GTK_CLIST (rd->clist), row,
				     work->data, (GtkDestroyNotify) g_free);
	work = work->next;
    }

    g_list_free (source_list);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    rd->button_auto = gtk_check_button_new_with_label (_("Auto rename"));
    gtk_signal_connect (GTK_OBJECT (rd->button_auto), "clicked",
			GTK_SIGNAL_FUNC
			(file_util_rename_mulitple_auto_toggle), rd);
    gtk_box_pack_end (GTK_BOX (hbox), rd->button_auto, FALSE, FALSE, 0);
    gtk_widget_show (rd->button_auto);

    rd->rename_box = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), rd->rename_box, FALSE, FALSE, 0);
    gtk_widget_show (rd->rename_box);

    furm_simple_label (rd->rename_box, _("Rename:"));
    rd->rename_label =
	furm_simple_label (rd->rename_box,
			   filename_from_path (rd->fd->source_path));
    furm_simple_label (rd->rename_box, _("to:"));

    rd->rename_entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (rd->rename_entry),
			filename_from_path (rd->fd->source_path));
    gtk_box_pack_start (GTK_BOX (rd->rename_box), rd->rename_entry, FALSE,
			FALSE, 0);
    generic_dialog_attach_default (GENERIC_DIALOG (rd->fd), rd->rename_entry);
    gtk_widget_grab_focus (rd->rename_entry);
    gtk_widget_show (rd->rename_entry);

    rd->auto_box = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), rd->auto_box, FALSE, FALSE, 0);
    /*
     * do not show it here 
     */

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (rd->auto_box), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    box2 = furm_simple_vlabel (hbox, _("Begin text"));

    rd->auto_entry_front = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (rd->auto_entry_front), "");
    gtk_box_pack_start (GTK_BOX (box2), rd->auto_entry_front, TRUE, TRUE, 0);
    gtk_widget_show (rd->auto_entry_front);

    box2 = furm_simple_vlabel (hbox, _("Start #"));

    adj = gtk_adjustment_new (0.0, 1.0, 1000000.0, 1.0, 1.0, 10.0);
    rd->auto_spin_start = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1.0, 0);
    gtk_widget_set_usize (rd->auto_spin_start, 70, -1);
    gtk_box_pack_start (GTK_BOX (box2), rd->auto_spin_start, FALSE, FALSE, 0);
    gtk_widget_show (rd->auto_spin_start);

    box2 = furm_simple_vlabel (hbox, _("End text"));

    rd->auto_entry_end = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (rd->auto_entry_end), "");
    gtk_box_pack_start (GTK_BOX (box2), rd->auto_entry_end, TRUE, TRUE, 0);
    gtk_widget_show (rd->auto_entry_end);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (rd->auto_box), hbox, FALSE, FALSE, 5);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Padding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
    gtk_widget_show (label);

    adj = gtk_adjustment_new (1.0, 1.0, 8.0, 1.0, 1.0, 1.0);
    rd->auto_spin_pad = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1.0, 0);
    gtk_box_pack_start (GTK_BOX (hbox), rd->auto_spin_pad, FALSE, FALSE, 0);
    gtk_widget_show (rd->auto_spin_pad);

    gtk_widget_show (GENERIC_DIALOG (rd->fd)->dialog);
}

/*
 * rename single file
 */

static void file_util_rename_single (FileDataSingle * fds);

static void
cb_file_util_rename_single_ok (GenericDialog * gd, gpointer data)
{
    FileDataSingle *fds = data;
    fds->confirmed = TRUE;
    file_util_rename_single (fds);
}

static void
cb_file_util_rename_single_cancel (GenericDialog * gd, gpointer data)
{
    FileDataSingle *fds = data;
    file_data_single_free (fds);
}

static void
file_util_rename_single (FileDataSingle * fds)
{
    if (isfile (fds->dest) && !fds->confirmed)
    {
	GenericDialog *gd;
	gchar  *text;

	text =
	    g_strdup_printf (_("Overwrite file:\n%s\nby renaming:\n%s"),
			     fds->dest, fds->source);
	gd = file_util_gen_dlg (_("Overwrite file"), text, "PornView",
				"dlg_confirm", TRUE,
				cb_file_util_rename_single_cancel, fds);
	g_free (text);

	generic_dialog_add (gd, _("Overwrite"), cb_file_util_rename_single_ok,
			    TRUE);
	generic_dialog_add_images (gd, fds->dest, fds->source);

	gtk_widget_show (gd->dialog);

	return;
    }
    else
    {
	if (rename (fds->source, fds->dest) < 0)
	{
	    gchar  *text =
		g_strdup_printf (_("Unable to rename file:\n%s\nto:\n%s"),
				 filename_from_path (fds->source),
				 filename_from_path (fds->dest));
	    file_util_warning_dialog (_("Error renaming file"), text);
	    g_free (text);
	}
	else
	{
	    file_maint_renamed (fds->source, fds->dest);
	    thumbview_refresh ();
	}
    }
    file_data_single_free (fds);
}

static void
cb_file_util_rename_single (FileDialog * fd, gpointer data)
{
    const gchar *name;
    gchar  *path;

    name = gtk_entry_get_text (GTK_ENTRY (fd->entry));
    path = concat_dir_and_file (fd->dest_path, name);

    if (strlen (name) == 0 || strcmp (fd->source_path, path) == 0)
    {
	g_free (path);
	return;
    }

    file_util_rename_single (file_data_single_new
			     (fd->source_path, path, fd->type));

    g_free (path);
    file_dialog_close (fd);
}

static void
cb_file_util_rename_single_close (FileDialog * fd, gpointer data)
{
    file_dialog_close (fd);
}

static void
file_util_rename_single_do (const gchar * source_path)
{
    FileDialog *fd;
    gchar  *text;
    const gchar *name = filename_from_path (source_path);

    text = g_strdup_printf (_("Rename file:\n%s\nto:"), name);
    fd = file_util_file_dlg (_("PornView - rename"), text, "PornView",
			     "dlg_rename", cb_file_util_rename_single_close,
			     NULL);
    g_free (text);

    file_dialog_add_button (fd, _("Rename"), cb_file_util_rename_single,
			    TRUE);

    fd->source_path = g_strdup (source_path);
    fd->dest_path = remove_level_from_path (source_path);

    fd->entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (GENERIC_DIALOG (fd)->vbox), fd->entry, FALSE,
			FALSE, 0);
    gtk_entry_set_text (GTK_ENTRY (fd->entry),
			filename_from_path (fd->source_path));
    gtk_entry_select_region (GTK_ENTRY (fd->entry), 0,
			     strlen (gtk_entry_get_text
				     (GTK_ENTRY (fd->entry))));
    generic_dialog_attach_default (GENERIC_DIALOG (fd), fd->entry);
    gtk_widget_grab_focus (fd->entry);
    gtk_widget_show (fd->entry);

    gtk_widget_show (GENERIC_DIALOG (fd)->dialog);
}

void
file_util_rename (const gchar * source_path, GList * source_list)
{
    if (!source_path && !source_list)
	return;

    if (source_path)
    {
	file_util_rename_single_do (source_path);
    }
    else if (!source_list->next)
    {
	file_util_rename_single_do (source_list->data);
	path_list_free (source_list);
    }
    else
    {
	file_util_rename_multiple_do (source_list);
    }
}

/*
 *--------------------------------------------------------------------------
 * Create directory routines
 *--------------------------------------------------------------------------
 */

static void
file_util_create_dir_do (const gchar * base, const gchar * name)
{
    gchar  *path;

    path = concat_dir_and_file (base, name);

    if (isdir (path))
    {
	gchar  *text =
	    g_strdup_printf (_("The directory:\n%s\nalready exists."), name);
	file_util_warning_dialog (_("Directory exists"), text);
	g_free (text);
    }
    else if (isname (path))
    {
	gchar  *text =
	    g_strdup_printf (_("The path:\n%s\nalready exists as a file."),
			     name);
	file_util_warning_dialog (_("Could not create directory"), text);
	g_free (text);
    }
    else
    {
	if (mkdir (path, 0755) < 0)
	{
	    gchar  *text =
		g_strdup_printf (_("Unable to create directory:\n%s"), name);
	    file_util_warning_dialog (_("Error creating directory"), text);
	    g_free (text);
	}
	else
	{
#if 0
	    if (strcmp (base, current_path) == 0)
	    {
		gchar  *buf = g_strdup (current_path);
		filelist_change_to (buf);
		g_free (buf);
	    }
#endif
	}
    }

    g_free (path);
}

static void
cb_file_util_create_dir (FileDialog * fd, gpointer data)
{
    const gchar *name;

    name = gtk_entry_get_text (GTK_ENTRY (fd->entry));

    if (strlen (name) == 0)
	return;

    if (name[0] == '/')
    {
	gchar  *buf;
	buf = remove_level_from_path (name);
	file_util_create_dir_do (buf, filename_from_path (name));
	g_free (buf);
    }
    else
    {
	file_util_create_dir_do (fd->dest_path, name);
    }

    file_dialog_close (fd);
}

static void
cb_file_util_create_dir_close (FileDialog * fd, gpointer data)
{
    file_dialog_close (fd);
}

void
file_util_create_dir (const gchar * path)
{
    FileDialog *fd;
    gchar  *text;

    if (!isdir (path))
	return;

    text = g_strdup_printf (_("Create directory in:\n%s\nnamed:"), path);
    fd = file_util_file_dlg (_("PornView - new directory"), text, "PornView",
			     "dlg_newdir", cb_file_util_create_dir_close,
			     NULL);
    g_free (text);

    file_dialog_add_button (fd, _("Create"), cb_file_util_create_dir, TRUE);

    fd->dest_path = g_strdup (path);

    fd->entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (GENERIC_DIALOG (fd)->vbox), fd->entry, FALSE,
			FALSE, 0);
    generic_dialog_attach_default (GENERIC_DIALOG (fd), fd->entry);
    gtk_widget_grab_focus (fd->entry);
    gtk_widget_show (fd->entry);

    gtk_widget_show (GENERIC_DIALOG (fd)->dialog);
}
