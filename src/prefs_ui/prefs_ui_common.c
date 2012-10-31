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
#include "file_type.h"
#include "menu.h"
#include "string_utils.h"

#include "prefs_ui_common.h"
#include "browser.h"

typedef struct PrefsWin_Tag
{
    /*
     * common 
     */
    GtkWidget *startup_dir_entry;
    GtkWidget *startup_dir_table;

    /*
     * filtering 
     */
    GtkWidget *filter_editlist;
    GtkWidget *filter_ext_entry;
    GtkWidget *filter_type_entry;
    GtkWidget *filter_disable_toggle;
    GtkWidget *filter_clist;
    GtkWidget *filter_new;
    GtkWidget *filter_add;
    GtkWidget *filter_change;
    GtkWidget *filter_del;
    gint    filter_selected;
}
PrefsWin;

static PrefsWin prefs_win;
extern GtkWidget *prefs_window;
extern Config *config_changed;
extern Config *config_prechanged;

typedef struct _FilterRowData
{
    gboolean enable;
    gboolean user_def;
}
FilterRowData;

static const gchar *charset_to_internal_items[] = {
    N_("Never convert"),
    N_("Locale"),
    N_("Auto detect"),
    N_("Any"),
    NULL
};

/*
 *-------------------------------------------------------------------
 * callback functions (common)
 *-------------------------------------------------------------------
 */

static void
cb_startup_dir_mode (GtkWidget * radiobutton, gpointer data)
{
    gint    idx = GPOINTER_TO_INT (data);

    if (!GTK_TOGGLE_BUTTON (radiobutton)->active)
	return;

    switch (idx)
    {
      case 0:
	  config_changed->startup_dir_mode = 0;
	  break;
      case 1:
	  config_changed->startup_dir_mode = 1;
	  break;
      case 3:
	  config_changed->startup_dir_mode = 2;
	  break;
      default:
	  break;
    }

    gtk_widget_set_sensitive (GTK_WIDGET (prefs_win.startup_dir_table),
			      config_changed->startup_dir_mode == 2);
}

static void
cb_save_win_state (GtkWidget * checkbutton, gpointer data)
{
    config_changed->save_win_state =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data));
}

static void
cb_enable_dock (GtkWidget * checkbutton, gpointer data)
{
    config_changed->enable_dock =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data));
}

static void
cb_startup_dir_browse_ok (GtkWidget * widget, gpointer data)
{
    gtk_entry_set_text (GTK_ENTRY (prefs_win.startup_dir_entry),
			gtk_file_selection_get_filename (GTK_FILE_SELECTION
							 (data)));

    gtk_widget_destroy (data);
}

static void
cb_startup_dir_browse_cancel (GtkWidget * widget, gpointer data)
{
    gtk_widget_destroy (data);
}

static void
cb_startup_dir_browse (GtkWidget * button, gpointer data)
{
    GtkWidget *file_dialog;

    file_dialog = dialog_choose_dir (_("Startup Directory"));
    gtk_window_set_transient_for (GTK_WINDOW (file_dialog),
				  GTK_WINDOW (prefs_window));

    gtk_signal_connect (GTK_OBJECT
			(GTK_FILE_SELECTION (file_dialog)->ok_button),
			"clicked",
			GTK_SIGNAL_FUNC (cb_startup_dir_browse_ok),
			file_dialog);

    gtk_signal_connect (GTK_OBJECT
			(GTK_FILE_SELECTION (file_dialog)->cancel_button),
			"clicked",
			GTK_SIGNAL_FUNC (cb_startup_dir_browse_cancel),
			file_dialog);

    gtk_widget_show (file_dialog);
}

static void
cb_startup_dir_current (GtkWidget * button, gpointer data)
{
    gtk_entry_set_text (GTK_ENTRY (prefs_win.startup_dir_entry),
			browser->current_path->str);
}

static void
cb_startup_dir_entry (GtkEditable * editable, gpointer data)
{
    if (config_changed->startup_dir != config_prechanged->startup_dir)
	g_free (config_changed->startup_dir);

    config_changed->startup_dir =
	g_strdup (gtk_entry_get_text
		  (GTK_ENTRY (prefs_win.startup_dir_entry)));
}

/*
 *-------------------------------------------------------------------
 * public functions (common)
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_common_page (void)
{
    GtkWidget *main_vbox;
    GtkWidget *vbox1;
    GtkWidget *frame1;
    GtkWidget *button1;
    GtkWidget *button2;
    GSList *vbox1_group = NULL;
    GtkWidget *frame2;
    GtkWidget *vbox2;
    GtkWidget *radiobutton1;
    GtkWidget *radiobutton2;
    GtkWidget *radiobutton3;
    GtkWidget *checkbutton1;
    GtkWidget *checkbutton2;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    frame1 = gtk_frame_new (_(" Startup Directory "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame1, FALSE, FALSE, 0);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), 10);
    gtk_container_add (GTK_CONTAINER (frame1), vbox1);

    radiobutton1 =
	gtk_radio_button_new_with_label (vbox1_group, _("Home directory"));
    vbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
    gtk_box_pack_start (GTK_BOX (vbox1), radiobutton1, FALSE, FALSE, 0);

    radiobutton2 =
	gtk_radio_button_new_with_label (vbox1_group,
					 _("Go to last visited location"));
    vbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
    gtk_box_pack_start (GTK_BOX (vbox1), radiobutton2, FALSE, FALSE, 0);

    radiobutton3 =
	gtk_radio_button_new_with_label (vbox1_group,
					 _("Go to this directory:"));
    vbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
    gtk_box_pack_start (GTK_BOX (vbox1), radiobutton3, FALSE, FALSE, 0);

    prefs_win.startup_dir_table = gtk_table_new (2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox1), prefs_win.startup_dir_table, TRUE,
			TRUE, 0);

    prefs_win.startup_dir_entry = gtk_entry_new ();
    gtk_table_attach (GTK_TABLE (prefs_win.startup_dir_table),
		      prefs_win.startup_dir_entry, 0, 1, 0, 1,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);

    button1 = gtk_button_new_with_label (_("Browse..."));
    gtk_table_attach (GTK_TABLE (prefs_win.startup_dir_table), button1, 1, 2,
		      0, 1, (GtkAttachOptions) (GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);

    button2 = gtk_button_new_with_label (_("Current"));
    gtk_table_attach (GTK_TABLE (prefs_win.startup_dir_table), button2, 1, 2,
		      1, 2, (GtkAttachOptions) (GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);
    gtk_widget_set_usize (button2, 90, -2);
    gtk_widget_show_all (frame1);

    frame2 = gtk_frame_new (_(" Main Window "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame2, FALSE, FALSE, 10);

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
    gtk_container_add (GTK_CONTAINER (frame2), vbox2);

    checkbutton1 =
	gtk_check_button_new_with_label (_("Remember window size"));
    gtk_box_pack_start (GTK_BOX (vbox2), checkbutton1, FALSE, FALSE, 0);

    checkbutton2 = gtk_check_button_new_with_label (_("Enable/Disable dock"));
    gtk_box_pack_start (GTK_BOX (vbox2), checkbutton2, FALSE, FALSE, 0);
    gtk_widget_show_all (frame2);

    gtk_widget_set_sensitive (GTK_WIDGET (prefs_win.startup_dir_table),
			      FALSE);

    if (conf.startup_dir_mode == 0)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton1), TRUE);
    else if (conf.startup_dir_mode == 1)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton2), TRUE);
    else
    {
	gtk_entry_set_text (GTK_ENTRY (prefs_win.startup_dir_entry),
			    conf.startup_dir);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton3), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (prefs_win.startup_dir_table),
				  TRUE);
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1),
				  conf.save_win_state);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton2),
				  conf.enable_dock);

    gtk_signal_connect (GTK_OBJECT (radiobutton1), "toggled",
			GTK_SIGNAL_FUNC
			(cb_startup_dir_mode), GINT_TO_POINTER (0));

    gtk_signal_connect (GTK_OBJECT (radiobutton2), "toggled",
			GTK_SIGNAL_FUNC
			(cb_startup_dir_mode), GINT_TO_POINTER (1));

    gtk_signal_connect (GTK_OBJECT (radiobutton3), "toggled",
			GTK_SIGNAL_FUNC
			(cb_startup_dir_mode), GINT_TO_POINTER (3));

    gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			GTK_SIGNAL_FUNC (cb_startup_dir_browse), NULL);

    gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			GTK_SIGNAL_FUNC (cb_startup_dir_current), NULL);

    gtk_signal_connect (GTK_OBJECT (prefs_win.startup_dir_entry), "changed",
			GTK_SIGNAL_FUNC (cb_startup_dir_entry), NULL);

    gtk_signal_connect (GTK_OBJECT (checkbutton1), "clicked",
			GTK_SIGNAL_FUNC (cb_save_win_state), checkbutton1);

    gtk_signal_connect (GTK_OBJECT (checkbutton2), "clicked",
			GTK_SIGNAL_FUNC (cb_enable_dock), checkbutton2);

    return main_vbox;
}

/*
 *-------------------------------------------------------------------
 * callback functions (filter)
 *-------------------------------------------------------------------
 */

static  gboolean
filter_check_value (const gchar * ext, const gchar * name)
{
    if (!string_is_ascii_printable (ext) || !string_is_ascii_printable (name))
    {
	dialog_message (_("Error"), _("String includes invalid character!\n"
				      "Only ASCII is available."), NULL);
	return FALSE;
    }

    if (strchr (ext, ',') || strchr (name, ','))
    {
	dialog_message (_("Error"),
			_("Sorry, cannot include \",\" character!"), NULL);
	return FALSE;
    }

    if (strchr (ext, ';') || strchr (name, ';'))
    {
	dialog_message (_("Error"),
			_("Sorry, cannot include \";\" character!"), NULL);
	return FALSE;
    }

    return TRUE;
}

static  gboolean
filter_check_duplicate (const gchar * ext, gint row, gboolean dialog)
{
    EditableList *editlist = EDITABLE_LIST (prefs_win.filter_editlist);
    gchar  *text, message[BUF_SIZE];
    gint    i, rows = editable_list_get_n_rows (editlist);

    if (!ext || !*ext)
	return FALSE;

    for (i = 0; i < rows; i++)
    {
	if (i == row)
	    continue;

	text = editable_list_get_cell_text (editlist, i, 0);
	if (!text)
	    continue;

	if (text && *text && !strcasecmp (ext, text))
	{
	    if (dialog)
	    {
		g_snprintf (message, BUF_SIZE,
			    _("\"%s\" is already defined."), ext);
		dialog_message (_("Error!!"), message, NULL);
	    }
	    g_free (text);
	    return TRUE;
	}

	g_free (text);
    }

    return FALSE;
}

static  gboolean
filter_sysdef_check_enable (gchar * ext, gchar ** list)
{
    gint    i;

    if (!list)
	return TRUE;

    for (i = 0; list[i]; i++)
    {
	if (list[i] && *list[i] && !strcmp (ext, list[i]))
	    return FALSE;
    }

    return TRUE;
}

static void
filter_set_value (void)
{
    EditableList *editlist = EDITABLE_LIST (prefs_win.filter_editlist);
    GList  *list;
    FilterRowData *rowdata;
    gchar  *text[16];
    gchar **sysdef_disables = NULL, **user_defs = NULL, **sections = NULL;
    gint    i, row;

    /*
     * system defined image types 
     */

    if (conf.imgtype_disables && *conf.imgtype_disables)
    {
	sysdef_disables = g_strsplit (conf.imgtype_disables, ",", -1);
	if (sysdef_disables)
	    for (i = 0; sysdef_disables[i]; i++)
		if (*sysdef_disables[i])
		    g_strstrip (sysdef_disables[i]);
    }

    list = file_type_get_sysdef_ext_list_all ();

    for (i = 0; list; list = g_list_next (list), i++)
    {
	rowdata = g_new0 (FilterRowData, 1);
	rowdata->enable =
	    filter_sysdef_check_enable (list->data, sysdef_disables);
	rowdata->user_def = FALSE;

	text[0] = list->data;
	text[1] = file_type_get_type_from_ext (text[0]);
	text[2] = rowdata->enable ? _("Enable") : _("Disable");
	text[3] = _("System defined");

	row = editable_list_append_row (editlist, text);

	editable_list_set_row_data_full (editlist, row, rowdata,
					 (GtkDestroyNotify) g_free);
    }

    if (sysdef_disables)
	g_strfreev (sysdef_disables);


    /*
     * user defined image types 
     */

    if (conf.imgtype_user_defs && *conf.imgtype_user_defs)
	user_defs = g_strsplit (conf.imgtype_user_defs, ";", -1);
    if (!user_defs)
	return;

    for (i = 0; user_defs[i]; i++)
    {
	if (!*user_defs[i])
	    continue;

	sections = g_strsplit (user_defs[i], ",", 3);
	if (!sections)
	    continue;
	g_strstrip (sections[0]);
	g_strstrip (sections[1]);
	g_strstrip (sections[2]);
	if (!*sections[0])
	    goto ERROR;
	if (filter_check_duplicate (sections[0], -1, FALSE))
	    goto ERROR;

	text[0] = sections[0];

	rowdata = g_new0 (FilterRowData, 1);
	rowdata->user_def = TRUE;

	text[1] = *sections[1] ? sections[1] : "UNKNOWN";

	if (*sections[2] && !strcasecmp (sections[2], "ENABLE"))
	{
	    text[2] = _("Enable");
	    rowdata->enable = TRUE;
	}
	else
	{
	    text[2] = _("Disable");
	    rowdata->enable = FALSE;
	}

	text[3] = _("User defined");

	row = editable_list_append_row (editlist, text);

	editable_list_set_row_data_full (editlist, row, rowdata,
					 (GtkDestroyNotify) g_free);

      ERROR:
	g_strfreev (sections);
    }

    g_strfreev (user_defs);
}

static void
cb_filter_editlist_confirm (EditableList * editlist,
			    EditableListActionType type,
			    gint selected_row,
			    EditableListConfirmFlags * flags, gpointer data)
{
    const gchar *ext, *name;
    gboolean duplicate = FALSE;
    FilterRowData *rowdata;

    ext = gtk_entry_get_text (GTK_ENTRY (prefs_win.filter_ext_entry));
    name = gtk_entry_get_text (GTK_ENTRY (prefs_win.filter_type_entry));

    gtk_widget_set_sensitive (prefs_win.filter_ext_entry, TRUE);
    gtk_widget_set_sensitive (prefs_win.filter_type_entry, TRUE);

    if (selected_row >= 0)
    {
	rowdata = editable_list_get_row_data (editlist, selected_row);
	if (rowdata && !rowdata->user_def)
	{
	    *flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
	    *flags |= EDITABLE_LIST_CONFIRM_CANNOT_DELETE;
	    gtk_widget_set_sensitive (prefs_win.filter_ext_entry, FALSE);
	    gtk_widget_set_sensitive (prefs_win.filter_type_entry, FALSE);
	}
    }

    if (!ext || !*ext || !name || !*name)
    {
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
    }

    if (type != EDITABLE_LIST_ACTION_ADD
	&& type != EDITABLE_LIST_ACTION_CHANGE)
	return;

    /*
     * check intput value 
     */
    if (!filter_check_value (ext, name))
    {
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
	return;
    }

    /*
     * check duplicate 
     */
    if (type == EDITABLE_LIST_ACTION_ADD)
	duplicate = filter_check_duplicate (ext, -1, TRUE);
    else if (type == EDITABLE_LIST_ACTION_CHANGE)
	duplicate = filter_check_duplicate (ext, selected_row, TRUE);

    if (duplicate)
    {
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_ADD;
	*flags |= EDITABLE_LIST_CONFIRM_CANNOT_CHANGE;
    }
}

static void
cb_filter_editlist_updated (EditableList * editlist)
{
    FilterRowData *rowdata;
    gint    i, rows = editable_list_get_n_rows (editlist);
    gchar  *ext, *type, *disable_list = NULL, *user_defs =
	NULL, *tmpstr, *status;

    for (i = 0; i < rows; i++)
    {
	rowdata = editable_list_get_row_data (editlist, i);
	if (!rowdata)
	    continue;

	ext = editable_list_get_cell_text (editlist, i, 0);
	if (!ext)
	    continue;
	type = editable_list_get_cell_text (editlist, i, 1);
	if (!type)
	    type = "UNKNOWN";

	if (!rowdata->user_def)
	{			/* system define */
	    if (rowdata->enable)
		continue;
	    if (!disable_list)
	    {
		disable_list = g_strdup (ext);
	    }
	    else
	    {
		tmpstr = disable_list;
		disable_list = g_strconcat (disable_list, ",", ext, NULL);
		g_free (tmpstr);
	    }
	}
	else
	{			/* user define */
	    status = rowdata->enable ? "ENABLE" : "DISABLE";

	    if (!user_defs)
	    {
		user_defs = g_strconcat (ext, ",", type, ",", status, NULL);
	    }
	    else
	    {
		tmpstr = user_defs;
		user_defs = g_strconcat (user_defs, "; ",
					 ext, ",", type, ",", status, NULL);
		g_free (tmpstr);
	    }
	}
    }

    if (config_changed->imgtype_disables !=
	config_prechanged->imgtype_disables)
	g_free (config_changed->imgtype_disables);
    config_changed->imgtype_disables = disable_list;

    if (config_changed->imgtype_user_defs !=
	config_prechanged->imgtype_user_defs)
	g_free (config_changed->imgtype_user_defs);
    config_changed->imgtype_user_defs = user_defs;
}

static gchar *
cb_filter_deftype_get_data (EditableList * editlist,
			    EditableListActionType type,
			    GtkWidget * widget, gpointer coldata)
{
    FilterRowData *rowdata;
    gint    selected;

    g_return_val_if_fail (IS_EDITABLE_LIST (editlist), NULL);

    if (type == EDITABLE_LIST_ACTION_ADD)
	goto USER_DEF;

    selected = editable_list_get_selected_row (editlist);
    if (selected < 0)
	goto USER_DEF;

    rowdata = editable_list_get_row_data (editlist, selected);
    if (!rowdata)
	goto USER_DEF;

    if (!rowdata->user_def)
	return g_strdup (_("System defined"));

  USER_DEF:
    return g_strdup (_("User defined"));
}

static  gboolean
cb_filter_get_row_data (EditableList * editlist,
			EditableListActionType type,
			gpointer * rowdata_ret,
			GtkDestroyNotify * destroy_fn_ret)
{
    GtkToggleButton *toggle =
	GTK_TOGGLE_BUTTON (prefs_win.filter_disable_toggle);
    FilterRowData *rowdata = NULL;
    gint    selected = editable_list_get_selected_row (editlist);
    gboolean retval = FALSE;

    g_return_val_if_fail (rowdata_ret && destroy_fn_ret, FALSE);

    if (type == EDITABLE_LIST_ACTION_ADD)
    {
	rowdata = g_new0 (FilterRowData, 1);
	rowdata->user_def = TRUE;
	*rowdata_ret = rowdata;
	*destroy_fn_ret = (GtkDestroyNotify) g_free;
	retval = TRUE;
    }
    else if (type == EDITABLE_LIST_ACTION_CHANGE)
    {
	rowdata = editable_list_get_row_data (editlist, selected);
	g_return_val_if_fail (rowdata, FALSE);
    }
    else
    {
	return FALSE;
    }

    rowdata->enable = !toggle->active;

    return retval;
}

/*
 *-------------------------------------------------------------------
 * public functions (filter)
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_filter_page (void)
{
    GtkWidget *main_vbox, *frame, *frame_vbox, *hbox, *hbox1, *vbox;
    GtkWidget *editlist, *entry;
    GtkWidget *toggle, *label;
    gchar  *titles[] = { _("Extension"), _("Type"), _("Status"), NULL };
    gint    titles_num = sizeof (titles) / sizeof (gchar *);

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

   /********************************************** 
    * Image Types Frame
    **********************************************/
    frame = gtk_frame_new (_("Image Types"));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, TRUE, 0);

    frame_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame), frame_vbox);
    gtk_widget_show (frame_vbox);
    gtk_widget_show (frame);

    editlist = editable_list_new_with_titles (titles_num, titles);
    prefs_win.filter_editlist = editlist;
    editable_list_set_reorderable (EDITABLE_LIST (editlist), FALSE);
    editable_list_set_auto_sort (EDITABLE_LIST (editlist), 0);
    gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
    gtk_widget_show (editlist);
    gtk_widget_set_usize (editlist, -2, 320);

    /*
     * entry area 
     */
    hbox = EDITABLE_LIST (editlist)->edit_area;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
    gtk_widget_show (hbox1);

    label = gtk_label_new (_("Extension: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    entry =
	editable_list_create_entry (EDITABLE_LIST (editlist), 0, NULL, FALSE);
    prefs_win.filter_ext_entry = entry;
    gtk_widget_set_usize (entry, 100, -1);
    gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);
    gtk_widget_show (entry);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
    gtk_widget_show (hbox1);

    label = gtk_label_new (_("Image Type: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
    gtk_widget_show (hbox1);

    entry = editable_list_create_entry (EDITABLE_LIST (editlist), 1,
					"UNKNOWN", FALSE);
    prefs_win.filter_type_entry = entry;
    gtk_box_pack_start (GTK_BOX (hbox1), entry, TRUE, TRUE, 0);
    gtk_widget_show (entry);

    /*
     * disable check box 
     */
    toggle = editable_list_create_check_button (EDITABLE_LIST (editlist), 2,
						_("Disable"), FALSE,
						_("Disable"), _("Enable"));
    prefs_win.filter_disable_toggle = toggle;
    gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
    gtk_widget_show (toggle);

    /*
     * definition type column 
     */
    editable_list_set_column_funcs (EDITABLE_LIST (editlist),
				    NULL, 3, NULL,
				    cb_filter_deftype_get_data,
				    NULL, NULL, NULL);

    /*
     * for row data 
     */
    editable_list_set_get_row_data_func (EDITABLE_LIST (editlist),
					 cb_filter_get_row_data);

    filter_set_value ();

    gtk_signal_connect (GTK_OBJECT (editlist), "action_confirm",
			GTK_SIGNAL_FUNC (cb_filter_editlist_confirm), NULL);
    gtk_signal_connect (GTK_OBJECT (editlist), "list_updated",
			GTK_SIGNAL_FUNC (cb_filter_editlist_updated), NULL);

    return main_vbox;
}

void
prefs_filter_apply (Config * src, Config * dest, PrefsWinButton state)
{
    file_type_update_image_types (dest->imgtype_disables,
				  dest->imgtype_user_defs);
}

/*
 *-------------------------------------------------------------------
 * callback functions (charset)
 *-------------------------------------------------------------------
 */

static void
cb_locale_charset_changed (GtkEditable * entry, gpointer data)
{
    if (config_changed->charset_locale != config_prechanged->charset_locale)
	g_free (config_changed->charset_locale);

    config_changed->charset_locale
	= g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
cb_internal_charset_changed (GtkEditable * entry, gpointer data)
{
    if (config_changed->charset_internal !=
	config_prechanged->charset_internal)
	g_free (config_changed->charset_internal);

    config_changed->charset_internal
	= g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
cb_filename_charset_changed (GtkEditable * entry, gpointer data)
{
    if (config_changed->charset_filename !=
	config_prechanged->charset_filename)
	g_free (config_changed->charset_filename);

    config_changed->charset_filename
	= g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
set_sensitive_filename_charset_mode (gint num, GtkWidget * combo)
{
    g_return_if_fail (combo);

    if (num == CHARSET_TO_LOCALE_ANY)
    {
	gtk_widget_set_sensitive (combo, TRUE);
    }
    else
    {
	gtk_widget_set_sensitive (combo, FALSE);
    }
}

static void
cb_filename_charset_conv (GtkWidget * widget, gpointer data)
{
    GtkWidget *combo = data;

    config_changed->charset_filename_mode
	= GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), "num"));

    set_sensitive_filename_charset_mode (config_changed->
					 charset_filename_mode, combo);
}

/*
 *-------------------------------------------------------------------
 * public functions (charset)
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_charset_page (void)
{
    GtkWidget *main_vbox, *frame, *frame_vbox, *table, *hbox;
    GtkWidget *label, *combo, *option_menu;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    /*
     * common frame 
     */
    frame = gtk_frame_new (_(" Common "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, TRUE, 0);

    frame_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, TRUE, TRUE, 0);
    table = gtk_table_new (2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, TRUE, 5);

    /*
     * locale charset 
     */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
		      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    label = gtk_label_new (_("Locale character set: "));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    combo = gtk_combo_new ();
    gtk_combo_set_popdown_strings (GTK_COMBO (combo),
				   charset_get_known_list (NULL));
    gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 0, 1,
		      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
			conf.charset_locale);
    gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
			GTK_SIGNAL_FUNC (cb_locale_charset_changed), NULL);

    /*
     * internal charset 
     */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 1, 2,
		      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    label = gtk_label_new (_("Internal character set: "));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    combo = gtk_combo_new ();
    gtk_combo_set_popdown_strings (GTK_COMBO (combo),
				   charset_get_known_list (NULL));
    gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 1, 2,
		      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
			conf.charset_internal);
    gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
			GTK_SIGNAL_FUNC (cb_internal_charset_changed), NULL);

    /*
     * Language to detect 
     */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, TRUE, 0);
    label = gtk_label_new (_("Language for auto detecting"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 5);
    option_menu = menu_option_simple (charset_auto_detect_labels,
				      conf.charset_auto_detect_lang,
				      &config_changed->
				      charset_auto_detect_lang);
    gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, TRUE, 5);
    gtk_widget_show_all (frame);

    /*
     * filename frame 
     */
    frame = gtk_frame_new (_(" File name "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, TRUE, 10);

    frame_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

    /*
     * charset for filename 
     */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, TRUE, 0);
    label = gtk_label_new (_("Character set of file name"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 5);

    /*
     * filename charset 
     */
    combo = gtk_combo_new ();
    gtk_widget_set_usize (combo, 120, -1);
    gtk_combo_set_popdown_strings (GTK_COMBO (combo),
				   charset_get_known_list (NULL));
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
			conf.charset_filename);
    gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
			GTK_SIGNAL_FUNC (cb_filename_charset_changed), NULL);

    option_menu = menu_option (charset_to_internal_items,
			       conf.charset_filename_mode,
			       cb_filename_charset_conv, combo);

    gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, TRUE, 5);
    gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
    gtk_widget_show_all (frame);

    set_sensitive_filename_charset_mode (conf.charset_filename_mode, combo);

    return main_vbox;
}
