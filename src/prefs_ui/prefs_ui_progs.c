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
#include "check_button.h"

#include "prefs.h"
#include "prefs_ui_progs.h"
#include "prefs_win.h"

typedef struct PrefsWin_Tag
{
    GtkWidget *text_viewer_command;
}
PrefsWin;

static PrefsWin prefs_win;
extern Config *config_changed;
extern Config *config_prechanged;

static void
set_default_progs_list (EditableList * editlist)
{
    gint    i, j, num;
    gchar **pair, *data[16];

    num = sizeof (conf.progs) / sizeof (gchar *);
    j = 0;

    editable_list_set_max_row (editlist, num);

    for (i = 0; i < num; i++)
    {
	if (!config_changed->progs[i])
	    continue;

	pair = g_strsplit (conf.progs[i], ",", 3);
	if (pair[0] && pair[1])
	{
	    data[0] = charset_locale_to_internal (pair[0]);
	    data[1] = charset_locale_to_internal (pair[1]);
	    if (pair[2] && !strcasecmp ("TRUE", pair[2]))
		data[2] = _("TRUE");
	    else
		data[2] = _("FALSE");

	    editable_list_append_row (EDITABLE_LIST (editlist), data);

	    if (config_changed->progs[i] == config_prechanged->progs[i])
	    {
		config_changed->progs[j] =
		    g_strdup (config_changed->progs[i]);
	    }
	    else
	    {
		config_changed->progs[j] = config_changed->progs[i];
	    }
	    if (j != i)
		config_changed->progs[i] = NULL;

	    g_free (data[0]);
	    g_free (data[1]);

	    j++;
	}
	g_strfreev (pair);
    }
}

static void
cb_editlist_updated (EditableList * editlist,
		     EditableListActionType type,
		     gint selected_row, gpointer data)
{
    gint    i, maxnum = sizeof (conf.progs) / sizeof (gchar *);

    for (i = 0; i < editlist->rows && i < maxnum; i++)
    {
	gchar **text, *confstring, *booltext, *locale[2];

	text = editable_list_get_row_text (editlist, i);
	if (!text)
	    continue;

	if (text[2] && *text[2] && !strcmp (text[2], _("TRUE")))
	    booltext = "TRUE";
	else
	    booltext = "FALSE";

	locale[0] = charset_internal_to_locale (text[0]);
	locale[1] = charset_internal_to_locale (text[1]);

	confstring = g_strconcat (locale[0], ",", locale[1], ",",
				  booltext, NULL);

	g_free (locale[0]);
	g_free (locale[1]);

	g_strfreev (text);

	if (config_changed->progs[i] != config_prechanged->progs[i])
	    g_free (config_changed->progs[i]);
	config_changed->progs[i] = confstring;
    }

    for (i = editlist->rows; i < maxnum; i++)
    {
	if (config_changed->progs[i] != config_prechanged->progs[i])
	    g_free (config_changed->progs[i]);
	config_changed->progs[i] = NULL;
    }
}

/*******************************************************************************
 *  prefs_progs_page:
 *     @ Create external programs preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_progs_page (void)
{
    gchar  *titles[] = {
	_("Program Name"),
	_("Command"),
	_("Dialog"),
    };
    gint    titles_num = sizeof (titles) / sizeof (gchar *);

    GtkWidget *main_vbox, *frame, *frame_vbox, *hbox, *hbox1, *hbox2, *vbox;
    GtkWidget *editlist, *label, *entry, *toggle;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);


   /**********************************************
    *  Viewer/Editor frame
    **********************************************/
    frame = gtk_frame_new (_(" Graphic Viewer/Editor "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, TRUE, 0);

    frame_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame_vbox), 10);
    gtk_container_add (GTK_CONTAINER (frame), frame_vbox);
    gtk_widget_show (frame_vbox);
    gtk_widget_show (frame);

    editlist = editable_list_new_with_titles (titles_num, titles);
    set_default_progs_list (EDITABLE_LIST (editlist));
    gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (editlist), "list_updated",
			GTK_SIGNAL_FUNC (cb_editlist_updated), NULL);
    gtk_widget_set_usize (editlist, -2, 320);
	
    /*
     *  create edit area
     */
    hbox = EDITABLE_LIST (editlist)->edit_area;

    /*
     * program name entry 
     */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);

    label = gtk_label_new (_("Program Name: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

    entry = editable_list_create_entry (EDITABLE_LIST (editlist), 0,
					NULL, FALSE);
    gtk_widget_set_usize (entry, 100, -1);
    gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);

    /*
     * command entry 
     */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);

    label = gtk_label_new (_("Command: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);

    entry = editable_list_create_entry (EDITABLE_LIST (editlist), 1,
					NULL, FALSE);
    gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);

    /*
     * check box 
     */
    toggle = editable_list_create_check_button (EDITABLE_LIST (editlist), 2,
						_("Dialog"), FALSE,
						_("TRUE"), _("FALSE"));
    gtk_box_pack_start (GTK_BOX (hbox2), toggle, FALSE, FALSE, 0);

    gtk_widget_show_all (main_vbox);

    return main_vbox;
}

/*******************************************************************************
 *  prefs_scripts_page:
 *     @ Create List View preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_scripts_page (void)
{
    GtkWidget *main_vbox, *frame, *toggle;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    frame = prefs_ui_dir_list_prefs (_(" Directories list to search scripts "),
				     _("Select scripts directory"),
				     config_prechanged->
				     scripts_search_dir_list,
				     &config_changed->scripts_search_dir_list,
				     ',');
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

    /*
     * Show dialog 
     */
    toggle =
	check_button_create (_("Show dialog befor execute script"),
			     conf.scripts_show_dialog,
			     check_button_get_data_from_toggle_cb,
			     &config_changed->scripts_show_dialog);
    gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);

    gtk_widget_show_all (main_vbox);

    return main_vbox;
}
