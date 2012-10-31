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
#include "string_utils.h"

#include "prefs_ui_etc.h"
#include "comment.h"

GtkWidget *slideshow_interval_label = NULL;

typedef struct PrefsWin_Tag
{
    /*
     * comment 
     */
    GtkWidget *comment_editlist;
    GtkWidget *comment_key_entry;
    GtkWidget *comment_name_entry;
    GtkWidget *comment_status_toggle;
    GtkWidget *comment_auto_toggle;
    GtkWidget *comment_disp_toggle;
}
PrefsWin;

static PrefsWin prefs_win;
extern Config *config_changed;
extern Config *config_prechanged;

/*
 *-------------------------------------------------------------------
 * comment page (private)
 *-------------------------------------------------------------------
 */

static  gboolean
check_value (const gchar * key, const gchar * name)
{
    gchar   message[BUF_SIZE];

    if (!string_is_ascii_graphable (key))
    {
	dialog_message (_("Error"),
			_("Key string includes invalid character!\n"
			  "Only ASCII (except space) is available."), NULL);
	return FALSE;
    }

    if (strchr (key, ',') || strchr (name, ','))
    {
	dialog_message (_("Error"),
			_("Sorry, cannot include \",\" character!"), NULL);
	return FALSE;
    }

    if (strchr (key, ';') || strchr (name, ';'))
    {
	dialog_message (_("Error"),
			_("Sorry, cannot include \";\" character!"), NULL);
	return FALSE;
    }

    if (!key || !*key)
    {
	g_snprintf (message, BUF_SIZE, _("\"Key name\" must be defined."));
	dialog_message (_("Error!!"), message, NULL);
	return FALSE;
    }
    if (!name || !*name)
    {
	g_snprintf (message, BUF_SIZE,
		    _("\"Display name\" must be defined."));
	dialog_message (_("Error!!"), message, NULL);
	return FALSE;
    }

    return TRUE;
}

static  gboolean
check_duplicate_comment_key (const gchar * key, gint this_row)
{
    EditableList *editlist = EDITABLE_LIST (prefs_win.comment_editlist);
    gint    row, rows = editlist->rows;

    g_return_val_if_fail (key && *key, FALSE);

    for (row = 0; row < rows; row++)
    {
	CommentDataEntry *entry;

	if (row == this_row)
	    continue;

	entry = editable_list_get_row_data (editlist, row);
	if (!entry)
	    continue;

	if (!strcmp (entry->key, key))
	{
	    /*
	     * FIXME!! add error handling 
	     */
	    return TRUE;
	}
    }

    return FALSE;
}

static void
set_default_comment_key_list (void)
{
    EditableList *editlist = EDITABLE_LIST (prefs_win.comment_editlist);
    GList  *list;
    gchar  *text[16];
    gint    row;

    for (list = comment_get_data_entry_list ();
	 list; list = g_list_next (list))
    {
	CommentDataEntry *dentry = list->data, *rowdata;

	if (!dentry)
	    continue;

	rowdata = comment_data_entry_dup (dentry);

	text[0] = dentry->key;
	text[1] = _(dentry->display_name);
	text[2] = _(boolean_to_text (dentry->enable));
	text[3] = _(boolean_to_text (dentry->auto_val));
	text[4] = _(boolean_to_text (dentry->display));
	if (dentry->userdef)
	    text[5] = _("User defined");
	else
	    text[5] = _("System defined");
	row = editable_list_append_row (editlist, text);

	if (row >= 0)
	{
	    editable_list_set_row_data_full (editlist, row, rowdata,
					     (GtkDestroyNotify)
					     comment_data_entry_delete);
	}
	else
	{
	    comment_data_entry_delete (rowdata);
	}
    }
}

static void
cb_comment_editlist_updated (EditableList * editlist)
{
    gchar  *string, *tmp;
    gint    row, rows = editable_list_get_n_rows (editlist);

    g_return_if_fail (IS_EDITABLE_LIST (editlist));

    if (config_changed->comment_key_list !=
	config_prechanged->comment_key_list)
	g_free (config_changed->comment_key_list);
    config_changed->comment_key_list = NULL;

    string = g_strdup ("");

    for (row = 0; row < rows; row++)
    {
	CommentDataEntry *entry;
	gchar  *dname_internal;

	entry = editable_list_get_row_data (editlist, row);
	if (!entry)
	    continue;
	if (!entry->key || !*entry->key)
	    continue;
	if (!entry->display_name || !*entry->display_name)
	    continue;

	tmp = string;
	if (*string)
	{
	    string = g_strconcat (string, "; ", NULL);
	    g_free (tmp);
	    tmp = string;
	}
	dname_internal = charset_internal_to_locale (entry->display_name);
	string = g_strconcat (string, entry->key, ",", dname_internal,
			      ",", boolean_to_text (entry->enable),
			      ",", boolean_to_text (entry->auto_val),
			      ",", boolean_to_text (entry->display), NULL);
	g_free (dname_internal);
	g_free (tmp);
	tmp = NULL;
    }

    config_changed->comment_key_list = string;
}

static void
cb_comment_editlist_confirm (EditableList * editlist,
			     EditableListActionType type,
			     gint selected_row,
			     EditableListConfirmFlags * flags, gpointer data)
{
    GtkWidget *t_auto = prefs_win.comment_auto_toggle;
    GtkWidget *key_entry = prefs_win.comment_key_entry;
    GtkWidget *name_entry = prefs_win.comment_name_entry;
    CommentDataEntry *entry = NULL;
    const gchar *key, *name;
    gchar   message[BUF_SIZE];
    gboolean duplicate = FALSE;

    key = gtk_entry_get_text (GTK_ENTRY (key_entry));
    name = gtk_entry_get_text (GTK_ENTRY (name_entry));

    gtk_widget_set_sensitive (key_entry, TRUE);
    gtk_widget_set_sensitive (name_entry, TRUE);
    gtk_widget_set_sensitive (t_auto, TRUE);

    if (selected_row < 0)
    {
	gtk_widget_set_sensitive (t_auto, FALSE);
    }
    else
    {
	entry = editable_list_get_row_data (editlist, selected_row);
	if (entry)
	{
	    gtk_widget_set_sensitive (key_entry, entry->userdef);
	    gtk_widget_set_sensitive (name_entry, entry->userdef);

	    if (!entry->def_val_fn)
		gtk_widget_set_sensitive (t_auto, FALSE);

	    if (!entry->userdef)
	    {
		*flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
		*flags |= EDITABLE_LIST_CONFIRM_CANNOT_DELETE;
	    }
	}
    }

    if (!key || !*key || !name || !*name)
    {
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
    }

    if ((!key || !*key) && (!name || !*name) && selected_row < 0)
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_NEW;


    if (type != EDITABLE_LIST_ACTION_ADD
	&& type != EDITABLE_LIST_ACTION_CHANGE)
	return;

    if (!check_value (key, name))
    {
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
	gtk_signal_emit_stop_by_name (GTK_OBJECT (editlist),
				      "action_confirm");
	return;
    }


#if 0
    if ((type == EDITABLE_LIST_ACTION_CHANGE) && entry && !entry->userdef)
    {
	if (strcmp (entry->key, key))
	{
	    /*
	     * FIXME!! add error handling 
	     */
	    *flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
	    return;
	}
	if (strcmp (_(entry->display_name), name))
	{
	    /*
	     * FIXME!! add error handling 
	     */
	    *flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
	    return;
	}
    }
#endif


    if (type == EDITABLE_LIST_ACTION_ADD)
	duplicate = check_duplicate_comment_key (key, -1);
    else if ((type == EDITABLE_LIST_ACTION_CHANGE) && entry && entry->userdef)
	duplicate = check_duplicate_comment_key (key, selected_row);

    if (duplicate)
    {
	g_snprintf (message, BUF_SIZE, _("\"%s\" is already defined."), key);
	dialog_message (_("Error!!"), message, NULL);
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
	gtk_signal_emit_stop_by_name (GTK_OBJECT (editlist),
				      "action_confirm");
    }
}

static gchar *
cb_editlist_deftype_get_data (EditableList * editlist,
			      EditableListActionType type,
			      GtkWidget * widget, gpointer coldata)
{
    CommentDataEntry *entry;
    gint    selected;

    g_return_val_if_fail (IS_EDITABLE_LIST (editlist), NULL);

    if (type == EDITABLE_LIST_ACTION_ADD)
	goto USER_DEF;

    selected = editable_list_get_selected_row (editlist);
    if (selected < 0)
	goto USER_DEF;

    entry = editable_list_get_row_data (editlist, selected);
    if (!entry)
	goto USER_DEF;

    if (!entry->userdef)
	return g_strdup (_("System defined"));

  USER_DEF:
    return g_strdup (_("User defined"));
}

static  gboolean
cb_editlist_get_row_data (EditableList * editlist,
			  EditableListActionType type,
			  gpointer * rowdata, GtkDestroyNotify * destroy_fn)
{
    GtkEntry *entry1 = GTK_ENTRY (prefs_win.comment_key_entry);
    GtkEntry *entry2 = GTK_ENTRY (prefs_win.comment_name_entry);
    GtkToggleButton *t_ins =
	GTK_TOGGLE_BUTTON (prefs_win.comment_status_toggle);
    GtkToggleButton *t_auto =
	GTK_TOGGLE_BUTTON (prefs_win.comment_auto_toggle);
    GtkToggleButton *t_disp =
	GTK_TOGGLE_BUTTON (prefs_win.comment_disp_toggle);
    CommentDataEntry *entry;
    gint    row = editable_list_get_selected_row (editlist);
    const gchar *key, *name;

    g_return_val_if_fail (rowdata && destroy_fn, FALSE);

    key = gtk_entry_get_text (entry1);
    name = gtk_entry_get_text (entry2);

    if (type == EDITABLE_LIST_ACTION_ADD)
    {
	entry = g_new0 (CommentDataEntry, 1);
	entry->key = NULL;
	entry->display_name = NULL;
	entry->value = NULL;
	entry->userdef = TRUE;
    }
    else if (type == EDITABLE_LIST_ACTION_CHANGE)
    {
	entry = editable_list_get_row_data (editlist, row);
	g_return_val_if_fail (entry, FALSE);
    }
    else
    {
	return FALSE;
    }

    if (!entry->userdef)
    {
	if (!entry->def_val_fn)
	    entry->auto_val = FALSE;
	else
	    entry->auto_val = t_auto->active;
    }
    else
    {
	g_free (entry->key);
	g_free (entry->display_name);

	entry->key = g_strdup (key);
	entry->display_name = g_strdup (name);
	entry->auto_val = FALSE;
    }

    entry->enable = t_ins->active;
    entry->display = t_disp->active;

    if (type == EDITABLE_LIST_ACTION_ADD)
    {
	g_return_val_if_fail (rowdata && destroy_fn, FALSE);
	*rowdata = entry;
	*destroy_fn = (GtkDestroyNotify) comment_data_entry_delete;
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

static void
cb_comment_charset_changed (GtkEditable * entry, gpointer data)
{
    if (config_changed->comment_charset != config_prechanged->comment_charset)
	g_free (config_changed->comment_charset);

    config_changed->comment_charset
	= g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

/*
 *-------------------------------------------------------------------
 * comment page (public)
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_comment_page (void)
{
    GtkWidget *main_vbox, *frame, *frame_vbox, *hbox, *hbox1, *vbox;
    GtkWidget *editlist, *entry, *toggle, *label, *combo;
    gchar  *titles[] = {
	_("Key Name"),
	_("Displayed Name"),
	_("Status"),
	_("Auto"),
	_("Display"),
	NULL
    };
    gint    titles_num = sizeof (titles) / sizeof (gchar *);

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    /*
     * Key Name definition frame 
     */
    frame = gtk_frame_new (_(" Key Name Definition "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

    frame_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

    gtk_widget_show (frame_vbox);
    gtk_widget_show (frame);

    editlist = editable_list_new_with_titles (titles_num, titles);
    prefs_win.comment_editlist = editlist;
    editable_list_set_reorderable (EDITABLE_LIST (editlist), FALSE);
    gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
    gtk_widget_show (editlist);

    /*
     *  create edit area
     */
    hbox = EDITABLE_LIST (editlist)->edit_area;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
    gtk_widget_show (hbox1);

    label = gtk_label_new (_("Key Name: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    entry =
	editable_list_create_entry (EDITABLE_LIST (editlist), 0, NULL, FALSE);
    prefs_win.comment_key_entry = entry;
    gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);
    gtk_widget_show (entry);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
    gtk_widget_show (hbox1);

    label = gtk_label_new (_("Displayed Name: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
    gtk_widget_show (hbox1);

    entry =
	editable_list_create_entry (EDITABLE_LIST (editlist), 1, NULL, FALSE);
    prefs_win.comment_name_entry = entry;
    gtk_box_pack_start (GTK_BOX (hbox1), entry, TRUE, TRUE, 0);
    gtk_widget_show (entry);

    /*
     * "insert" check box 
     */
    toggle = editable_list_create_check_button (EDITABLE_LIST (editlist), 2,
						_("Enable"), TRUE,
						_("TRUE"), _("FALSE"));
    prefs_win.comment_status_toggle = toggle;
    gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
    gtk_widget_show (toggle);

    /*
     * "Auto" check box 
     */
    toggle = editable_list_create_check_button (EDITABLE_LIST (editlist), 3,
						_("Auto"), FALSE,
						_("TRUE"), _("FALSE"));
    prefs_win.comment_auto_toggle = toggle;
    gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
    gtk_widget_show (toggle);

    /*
     * "display" check box 
     */
    toggle = editable_list_create_check_button (EDITABLE_LIST (editlist), 4,
						_("Display"), TRUE,
						_("TRUE"), _("FALSE"));
    prefs_win.comment_disp_toggle = toggle;
    gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
    gtk_widget_show (toggle);

    /*
     * definition type column 
     */
    editable_list_set_column_funcs (EDITABLE_LIST (editlist),
				    NULL, 5, NULL,
				    cb_editlist_deftype_get_data,
				    NULL, NULL, NULL);

    /*
     * for row data 
     */
    editable_list_set_get_row_data_func (EDITABLE_LIST (editlist),
					 cb_editlist_get_row_data);

    set_default_comment_key_list ();
    gtk_signal_connect (GTK_OBJECT (editlist), "action_confirm",
			GTK_SIGNAL_FUNC (cb_comment_editlist_confirm), NULL);
    gtk_signal_connect (GTK_OBJECT (editlist), "list_updated",
			GTK_SIGNAL_FUNC (cb_comment_editlist_updated), NULL);


    /*
     * Charset Frame 
     */
    frame = gtk_frame_new (_("Character set"));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 10);

    frame_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

/* locale charset */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, TRUE, 5);
    label = gtk_label_new (_("Character set: "));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 5);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    combo = gtk_combo_new ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 5);
    gtk_combo_set_popdown_strings (GTK_COMBO (combo),
				   charset_get_known_list (NULL));
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
			conf.comment_charset);
    gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
			GTK_SIGNAL_FUNC (cb_comment_charset_changed), NULL);

    gtk_widget_show_all (frame);

    return main_vbox;
}

void
prefs_comment_apply (Config * src, Config * dest, PrefsWinButton state)
{
    comment_update_data_entry_list ();
}

/*
 *-------------------------------------------------------------------
 * slideshow page (private)
 *-------------------------------------------------------------------
 */

static const gint intervals[] = {
    1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000,
    15000, 20000, 30000, 45000, 60000, 2 * 60000, 3 * 60000, 4 * 60000,
    5 * 60000, 6 * 60000, 7 * 60000, 8 * 60000, 9 * 60000, 10 * 60000,
    20 * 60000,
    30 * 60000, 40 * 60000, 60 * 60000, 2 * 60 * 60000, 4 * 60 * 60000,
    8 * 60 * 60000,
    24 * 60 * 60000
};

static const gint nintervals = 32;
static guint s_interval;

static  gint
slideshow_interval_get_index (gint interval)
{
    gint    i;

    for (i = 0; i < nintervals; i++)
    {
	if (interval <= intervals[i])
	    return i;
    }

    return nintervals - 1;
}

static void
slideshow_interval_update (gint interval)
{
    static const guchar *unit_name[] = {
	N_("second(s)"),
	N_("minute(s)"),
	N_("hour(s)"),
	N_("day(s)")
    };
    static const gint unit_count[] = {
	1000, 60, 60, 24
    };
    gint    i;
    guchar  buf[80];

    s_interval = interval;
    if (interval == 0)
    {
	gtk_label_set (GTK_LABEL (slideshow_interval_label),
		       _("milliseconds, no delay"));
    }
    else
    {
	i = 0;
	interval = (interval + 999) / 1000;

	while (i < 3 && interval >= unit_count[i + 1])
	{
	    i++;
	    interval /= unit_count[i];
	}
	sprintf (buf, "%u %s, %i %s", s_interval, _("milliseconds"), interval,
		 _(unit_name[i]));

	gtk_label_set (GTK_LABEL (slideshow_interval_label), buf);
    }
}

static void
cb_slideshow_interval_scale_changed (GtkAdjustment * adjustment,
				     gpointer data)
{
    config_changed->slideshow_interval = intervals[(gint) adjustment->value];
    slideshow_interval_update (config_changed->slideshow_interval);
}

/*
 *-------------------------------------------------------------------
 * slideshow page (public)
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_slideshow_page (void)
{
    GtkWidget *main_vbox;
    GtkWidget *frame1;
    GtkWidget *vbox;
    GtkWidget *frame2;
    GtkObject *adjustment;
    GtkWidget *hscale;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    frame1 = gtk_frame_new (_(" Delay before image change: "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame1, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame1), vbox);

    frame2 = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame2, FALSE, TRUE, 1);
    gtk_widget_set_usize (frame2, -2, 30);

    adjustment = gtk_adjustment_new (5.0, 0.0, (gfloat) nintervals,
				     1.0, 1.0, 1.0);

    gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (cb_slideshow_interval_scale_changed),
			NULL);

    GTK_ADJUSTMENT (adjustment)->value =
	(gfloat) slideshow_interval_get_index (conf.slideshow_interval);

    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
    gtk_range_set_update_policy (GTK_RANGE (hscale), GTK_UPDATE_CONTINUOUS);
    gtk_scale_set_digits (GTK_SCALE (hscale), 1);
    gtk_container_add (GTK_CONTAINER (frame2), hscale);
    gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);

    slideshow_interval_label = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (vbox), slideshow_interval_label, FALSE,
			FALSE, 3);

    slideshow_interval_update (conf.slideshow_interval);

    gtk_widget_show_all (frame1);

    return main_vbox;
}
