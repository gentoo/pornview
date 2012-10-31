/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#include "pornview.h"

#include "editable_list.h"

#include "charset.h"
#include "dialogs.h"
#include "prefs.h"
#include "prefs_ui_utils.h"

extern GtkWidget *prefs_window;

/*******************************************************************************
 *
 *  Directory list preference
 *
 *******************************************************************************/
typedef struct DirListPrefs_Tag
{
    GtkWidget *editlist, *entry, *select_button;
    const gchar *defval;
    gchar **prefsval;
    gchar   sepchar;
    const gchar *dialog_title;
}
DirListPrefs;

static void
set_default_dir_list (DirListPrefs * dirprefs)
{
    gchar **strs;
    gint    i;

    strs = g_strsplit (dirprefs->defval, &dirprefs->sepchar, -1);
    for (i = 0; strs && strs[i]; i++)
    {
	gchar  *text[1];

	text[0] = charset_to_internal (strs[i],
				       conf.charset_filename,
				       conf.charset_auto_detect_fn,
				       conf.charset_filename_mode);

	editable_list_append_row (EDITABLE_LIST (dirprefs->editlist), text);

	g_free (text[0]);
    }
    g_strfreev (strs);
}

static void
cb_dirprefs_editlist_updated (EditableList * editlist, gpointer data)
{
    DirListPrefs *dirprefs = data;
    gchar  *string = NULL;
    gint    i;

    for (i = 0; i < editlist->rows; i++)
    {
	gchar **text, *text_locale, *tmpstr;

	text = editable_list_get_row_text (editlist, i);
	if (!text)
	    continue;

	if (string)
	{
	    tmpstr = string;
	    string = g_strconcat (string, &dirprefs->sepchar, NULL);
	    g_free (tmpstr);
	}
	else
	{
	    string = g_strdup ("");
	}

	tmpstr = string;
	text_locale = charset_internal_to_locale (text[0]);
	string = g_strconcat (tmpstr, text_locale, NULL);
	g_free (text_locale);
	g_free (tmpstr);
    }

    if (dirprefs->defval != *dirprefs->prefsval)
    {
	g_free (*dirprefs->prefsval);
    }
    *dirprefs->prefsval = string;
}

static void
cb_dirprefs_dirsel_pressed (GtkButton * button, gpointer data)
{
    DirListPrefs *dirprefs = data;
    gchar  *path;
    const gchar *default_path, *dialog_title = _("Select directory");

    if (dirprefs->dialog_title)
	dialog_title = dirprefs->dialog_title;

    default_path = gtk_entry_get_text (GTK_ENTRY (dirprefs->entry));
    path = dialog_choose_dir_modal (dialog_title, default_path, GTK_WINDOW(prefs_window));
    
    if (path)
	gtk_entry_set_text (GTK_ENTRY (dirprefs->entry), path);
    g_free (path);
}

static void
cb_dirprefs_destroy (GtkWidget * widget, gpointer data)
{
    g_free (data);
}

GtkWidget *
prefs_ui_dir_list_prefs (const gchar * title,
			 const gchar * dialog_title,
			 const gchar * defval,
			 gchar ** prefsval, gchar sepchar)
{
    DirListPrefs *dirprefs = g_new0 (DirListPrefs, 1);
    GtkWidget *frame, *frame_vbox, *hbox;
    GtkWidget *editlist, *label, *entry, *button;
    gchar  *titles[] = {
	N_("Directory"),
    };
    gint    titles_num = sizeof (titles) / sizeof (gchar *);

    g_return_val_if_fail (prefsval, NULL);

    dirprefs->defval = defval;
    dirprefs->prefsval = prefsval;
    dirprefs->sepchar = sepchar;
    dirprefs->dialog_title = dialog_title;

    /*
     * frame 
     */
    frame = gtk_frame_new (title);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
    frame_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

    gtk_signal_connect (GTK_OBJECT (frame), "destroy",
			GTK_SIGNAL_FUNC (cb_dirprefs_destroy), dirprefs);

    /*
     * clist 
     */
    dirprefs->editlist = editlist
	= editable_list_new_with_titles (titles_num, titles);
    editable_list_set_column_title_visible (EDITABLE_LIST (editlist), FALSE);
    gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
    set_default_dir_list (dirprefs);
    gtk_signal_connect (GTK_OBJECT (editlist), "list_updated",
			GTK_SIGNAL_FUNC (cb_dirprefs_editlist_updated),
			dirprefs);

    /*
     * entry area 
     */
    hbox = EDITABLE_LIST (editlist)->edit_area;

    label = gtk_label_new (_("Directory to add : "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    entry = dirprefs->entry
	= editable_list_create_entry (EDITABLE_LIST (editlist), 0,
				      NULL, FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

    button = dirprefs->select_button =
	gtk_button_new_with_label (_("Select"));
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dirprefs_dirsel_pressed),
			dirprefs);

    return frame;
}
