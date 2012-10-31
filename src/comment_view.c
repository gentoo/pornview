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

#include "charset.h"
#include "file_utils.h"

#include "comment_view.h"
#include "prefs.h"
#include "prefs.h"

CommentView *commentview = NULL;

#ifdef ENABLE_TREEVIEW
typedef enum
{
    COLUMN_TERMINATOR = -1,
    COLUMN_KEY,
    COLUMN_VALUE,
    COLUMN_RAW_ENTRY,
    N_COLUMN
}
ListStoreColumn;
#endif /* ENABLE_TREEVIEW */

static void comment_view_set_sensitive (CommentView * cv);
static void comment_view_set_combo_list (CommentView * cv);
static void comment_view_data_enter (CommentView * cv);
static void comment_view_reset_data (CommentView * cv);

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_switch_page (GtkNotebook * notebook,
		GtkNotebookPage * page, gint pagenum, CommentView * cv)
{
    GtkWidget *widget;

    g_return_if_fail (page);
    g_return_if_fail (cv);

    if (!cv->button_area)
	return;

    widget = gtk_notebook_get_nth_page (notebook, pagenum);

    if (widget == cv->data_page || widget == cv->note_page)
	gtk_widget_show (cv->button_area);
    else
	gtk_widget_hide (cv->button_area);
}

static void
cb_clear_button_pressed (GtkButton * button, CommentView * cv)
{
    g_return_if_fail (cv);

    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry), "\0");
    gtk_entry_set_text (GTK_ENTRY (cv->value_entry), "\0");

    comment_view_set_sensitive (cv);
}

static void
cb_apply_button_pressed (GtkButton * button, CommentView * cv)
{
    comment_view_data_enter (cv);
}

static void
cb_del_value_button_pressed (GtkButton * button, CommentView * cv)
{
    CommentDataEntry *entry;

    g_return_if_fail (cv);

#ifdef ENABLE_TREEVIEW
    {
	GtkTreeView *treeview = GTK_TREE_VIEW (cv->comment_clist);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean found;

	found = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (!found)
	    return;

	gtk_tree_model_get (model, &iter,
			    COLUMN_RAW_ENTRY, &entry, COLUMN_TERMINATOR);
	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    }
#else /* ENABLE_TREEVIEW */
    if (cv->selected_row < 0)
	return;
    g_return_if_fail (cv->selected_row < GTK_CLIST (cv->comment_clist)->rows);

    entry = gtk_clist_get_row_data (GTK_CLIST (cv->comment_clist),
				    cv->selected_row);
    gtk_clist_remove (GTK_CLIST (cv->comment_clist), cv->selected_row);
#endif /* ENABLE_TREEVIEW */

    if (entry)
	comment_data_entry_remove (cv->comment, entry);

    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry), "\0");
    gtk_entry_set_text (GTK_ENTRY (cv->value_entry), "\0");

    comment_view_set_sensitive (cv);
}

static void
cb_save_button_pressed (GtkButton * button, CommentView * cv)
{
    gchar  *note;

    g_return_if_fail (cv);
    g_return_if_fail (cv->comment);

#if USE_GTK2
    {
	GtkTextBuffer *buffer;
	GtkTextIter start, end;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (cv->note_box));

	gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);

	note = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
    }
#else /* USE_GTK2 */
    {
	gint    len;

	len = gtk_text_get_length (GTK_TEXT (cv->note_box));
	note = gtk_editable_get_chars (GTK_EDITABLE (cv->note_box), 0, len);
    }
#endif /* USE_GTK2 */

    if (note && *note)
	comment_update_note (cv->comment, note);

    g_free (note);

    comment_save_file (cv->comment);

    if (file_exists (cv->comment->filename))
	gtk_widget_set_sensitive (cv->delete_button, TRUE);
    else
	gtk_widget_set_sensitive (cv->delete_button, FALSE);
}

static void
cb_reset_button_pressed (GtkButton * button, CommentView * cv)
{
    g_return_if_fail (cv);
    g_return_if_fail (cv->comment);

    comment_view_change_file (cv, cv->comment->info);
}

static void
cb_del_button_pressed (GtkButton * button, CommentView * cv)
{
    g_return_if_fail (cv->comment);

    comment_delete_file (cv->comment);
/*
    comment_unref (cv->comment);
    cv->comment = NULL;
    comment_view_clear_data (cv);
*/
    comment_view_change_file (cv, cv->comment->info);

    if (file_exists (cv->comment->filename))
	gtk_widget_set_sensitive (cv->delete_button, TRUE);
    else
	gtk_widget_set_sensitive (cv->delete_button, FALSE);
}

static void
cb_destroyed (GtkWidget * widget, CommentView * cv)
{
    g_return_if_fail (cv);

    gtk_signal_disconnect_by_func (GTK_OBJECT (cv->notebook),
				   GTK_SIGNAL_FUNC (cb_switch_page), cv);

    if (cv->comment)
    {
	comment_unref (cv->comment);
	cv->comment = NULL;
    }

    g_free (cv);
}

#ifdef ENABLE_TREEVIEW
static void
cb_tree_view_cursor_changed (GtkTreeView * treeview, CommentView * cv)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean success;
    gchar  *key = NULL, *value = NULL;
    GtkEntry *entry1, *entry2;

    entry1 = GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry);
    entry2 = GTK_ENTRY (cv->value_entry);

    success = gtk_tree_selection_get_selected (selection, &model, &iter);
    if (success)
    {
	gtk_tree_model_get (model, &iter,
			    COLUMN_KEY, &key,
			    COLUMN_VALUE, &value, COLUMN_TERMINATOR);
    }

    if (key)
	gtk_entry_set_text (entry1, key);
    else
	gtk_entry_set_text (entry1, "\0");
    if (value)
	gtk_entry_set_text (entry2, value);
    else
	gtk_entry_set_text (entry2, "\0");

    g_free (key);
    g_free (value);

    comment_view_set_sensitive (cv);
}

#else /* ENABLE_TREEVIEW */

static void
cb_clist_select_row (GtkCList * clist, gint row, gint col,
		     GdkEventButton * event, CommentView * cv)
{
    gchar  *text[2] = { NULL, NULL };
    gboolean success1, success2;

    g_return_if_fail (cv);

    cv->selected_row = row;

    success1 = gtk_clist_get_text (clist, row, 0, &text[0]);
    success2 = gtk_clist_get_text (clist, row, 1, &text[1]);

    if (success1 && text[0])
    {
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry),
			    text[0]);
    }

    if (success2)
    {
	gtk_entry_set_text (GTK_ENTRY (cv->value_entry), text[1]);
    }
    else
    {
	gtk_entry_set_text (GTK_ENTRY (cv->value_entry), "\0");
    }

    comment_view_set_sensitive (cv);
}

static void
cb_clist_unselect_row (GtkCList * clist, gint row, gint col,
		       GdkEventButton * event, CommentView * cv)
{
    g_return_if_fail (cv);

    cv->selected_row = -1;

    comment_view_set_sensitive (cv);
}

void
cb_clist_row_move (GtkCList * clist, gint arg1, gint arg2, CommentView * cv)
{
    CommentDataEntry *entry1, *entry2, *tmpentry;
    GList  *node1, *node2;
    gint    pos1, pos2, tmppos;

    g_return_if_fail (clist && GTK_IS_CLIST (clist));
    g_return_if_fail (cv);
    g_return_if_fail (cv->comment);

    entry1 = gtk_clist_get_row_data (clist, arg1);
    entry2 = gtk_clist_get_row_data (clist, arg2);

    g_return_if_fail (entry1 && entry2);

    node1 = g_list_find (cv->comment->data_list, entry1);
    node2 = g_list_find (cv->comment->data_list, entry2);
    g_return_if_fail (node1 && node2);

    pos1 = g_list_position (cv->comment->data_list, node1);
    pos2 = g_list_position (cv->comment->data_list, node2);

    /*
     * swap data position in the list 
     */
    if (pos1 > pos2)
    {
	tmppos = pos1;
	pos1 = pos2;
	pos2 = tmppos;
	tmpentry = entry1;
	entry1 = entry2;
	entry2 = tmpentry;
    }
    cv->comment->data_list = g_list_remove (cv->comment->data_list, entry1);
    cv->comment->data_list = g_list_remove (cv->comment->data_list, entry2);
    cv->comment->data_list =
	g_list_insert (cv->comment->data_list, entry2, pos1);
    cv->comment->data_list =
	g_list_insert (cv->comment->data_list, entry1, pos2);
}
#endif /* ENABLE_TREEVIEW */

static void
cb_entry_changed (GtkEditable * entry, CommentView * cv)
{
    g_return_if_fail (cv);

    comment_view_set_sensitive (cv);
}

static void
cb_entry_enter (GtkEditable * entry, CommentView * cv)
{
    g_return_if_fail (cv);

    comment_view_data_enter (cv);
}

static void
cb_combo_select (GtkWidget * label, CommentView * cv)
{
    GtkWidget *clist;
    gchar  *key = gtk_object_get_data (GTK_OBJECT (label), "key");

    g_return_if_fail (label && GTK_IS_LIST_ITEM (label));
    g_return_if_fail (cv);
    g_return_if_fail (key);

    cv->selected_item = label;
    clist = cv->comment_clist;

#ifdef ENABLE_TREEVIEW
    {
	GtkTreeView *treeview = GTK_TREE_VIEW (clist);
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreeIter iter;
	gboolean go_next;

	go_next = gtk_tree_model_get_iter_first (model, &iter);

	for (; go_next; go_next = gtk_tree_model_iter_next (model, &iter))
	{
	    CommentDataEntry *entry;

	    gtk_tree_model_get (model, &iter,
				COLUMN_RAW_ENTRY, &entry, COLUMN_TERMINATOR);
	    if (!entry)
		continue;

	    if (entry->key && !strcmp (key, entry->key))
	    {
		GtkTreePath *treepath =
		    gtk_tree_model_get_path (model, &iter);

		if (!treepath)
		    continue;
		gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);
		gtk_tree_path_free (treepath);
		break;
	    }
	}
    }
#else /* ENABLE_TREEVIEW */
    {
	gint    row;

	for (row = 0; row < GTK_CLIST (clist)->rows; row++)
	{
	    CommentDataEntry *entry;

	    entry = gtk_clist_get_row_data (GTK_CLIST (clist), row);
	    if (!entry)
		continue;

	    if (entry->key && !strcmp (key, entry->key))
	    {
		gtk_clist_select_row (GTK_CLIST (clist), row, 0);
		/*
		 * gtk_clist_moveto (GTK_CLIST (clist), row, 0, 0.0, 0.0); 
		 */
		break;
	    }
	}
    }
#endif /* ENABLE_TREEVIEW */
}

static void
cb_combo_deselect (GtkWidget * label, CommentView * cv)
{
    g_return_if_fail (label && GTK_IS_LIST_ITEM (label));
    g_return_if_fail (cv);

    cv->selected_item = NULL;
}

static void
cb_file_saved (Comment * comment, CommentView * cv)
{
    g_return_if_fail (cv);
    if (!cv->comment)
	return;

    comment_view_reset_data (cv);
}

/*
 *-------------------------------------------------------------------
 * other private functions
 *-------------------------------------------------------------------
 */

static CommentView *
comment_view_new ()
{
    CommentView *cv;

    cv = g_new0 (CommentView, 1);
    g_return_val_if_fail (cv, NULL);

    cv->comment = NULL;

    cv->window = NULL;
    cv->main_vbox = NULL;
    cv->notebook = NULL;
    cv->button_area = NULL;

    cv->data_page = NULL;
    cv->note_page = NULL;

    cv->comment_clist = NULL;
    cv->selected_row = -1;
    cv->key_combo = NULL;
    cv->value_entry = NULL;
    cv->note_box = NULL;
    cv->selected_item = NULL;

    cv->clear_button = NULL;
    cv->apply_button = NULL;
    cv->del_value_button = NULL;

    cv->save_button = NULL;
    cv->reset_button = NULL;
    cv->delete_button = NULL;

    return cv;
}

static GtkWidget *
create_data_page (CommentView * cv)
{
    GtkWidget *vbox, *vbox1, *hbox, *hbox1;
    GtkWidget *scrolledwin, *clist, *combo, *entry, *button;
    GtkWidget *label;
    gchar  *titles[] = { _("Key"), _("Value") };

    label = gtk_label_new (_(" Data "));
    gtk_widget_set_name (label, "TabLabel");
    gtk_widget_show (label);
    cv->data_page = vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

    gtk_notebook_append_page (GTK_NOTEBOOK (cv->notebook), vbox, label);

    /*
     * scrolled window & clist 
     */
    scrolledwin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
				    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (vbox), scrolledwin, TRUE, TRUE, 0);

#ifdef ENABLE_TREEVIEW
    {
	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *render;

	store = gtk_list_store_new (N_COLUMN,
				    G_TYPE_STRING,
				    G_TYPE_STRING, G_TYPE_POINTER);
	clist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	cv->comment_clist = clist;

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (clist), TRUE);

	/*
	 * set column for key 
	 */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, titles[0]);
	render = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, render, TRUE);
	gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_KEY);
	gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);

	/*
	 * set column for value 
	 */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, titles[1]);
	render = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, render, TRUE);
	gtk_tree_view_column_add_attribute (col, render, "text",
					    COLUMN_VALUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);

	gtk_signal_connect (GTK_OBJECT (clist), "cursor-changed",
			    GTK_SIGNAL_FUNC (cb_tree_view_cursor_changed),
			    cv);

	gtk_container_add (GTK_CONTAINER (scrolledwin), clist);
    }
#else /* ENABLE_TREEVIEW */
    clist = cv->comment_clist = gtk_clist_new_with_titles (2, titles);
    gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_SINGLE);
    /*
     * gtk_clist_set_column_width (GTK_CLIST(clist), 0, 80); 
     */
    gtk_clist_set_column_auto_resize (GTK_CLIST (clist), 0, TRUE);
    gtk_clist_set_column_auto_resize (GTK_CLIST (clist), 1, TRUE);
    gtk_clist_set_reorderable (GTK_CLIST (clist), TRUE);
    gtk_clist_set_use_drag_icons (GTK_CLIST (clist), FALSE);
    gtk_container_add (GTK_CONTAINER (scrolledwin), clist);

    gtk_signal_connect (GTK_OBJECT (clist), "select_row",
			GTK_SIGNAL_FUNC (cb_clist_select_row), cv);
    gtk_signal_connect (GTK_OBJECT (clist), "unselect_row",
			GTK_SIGNAL_FUNC (cb_clist_unselect_row), cv);
    gtk_signal_connect (GTK_OBJECT (clist), "row-move",
			GTK_SIGNAL_FUNC (cb_clist_row_move), cv);
#endif /* ENABLE_TREEVIEW */

    /*
     * entry area 
     */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);
    label = gtk_label_new (_("Key: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

    cv->key_combo = combo = gtk_combo_new ();
    gtk_box_pack_start (GTK_BOX (vbox1), combo, TRUE, TRUE, 0);
    gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry),
			    FALSE);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);
    label = gtk_label_new (_("Value: "));
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    cv->value_entry = entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (vbox1), entry, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			GTK_SIGNAL_FUNC (cb_entry_changed), cv);
    gtk_signal_connect (GTK_OBJECT (entry), "activate",
			GTK_SIGNAL_FUNC (cb_entry_enter), cv);

    /*
     * buttons 
     */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_end (GTK_BOX (hbox), hbox1, FALSE, TRUE, 0);
    gtk_box_set_homogeneous (GTK_BOX (hbox1), TRUE);

    button = gtk_button_new_with_label (_("Apply"));
    cv->apply_button = button;
    gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_apply_button_pressed), cv);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    button = gtk_button_new_with_label (_("Clear"));
    cv->clear_button = button;
    gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_clear_button_pressed), cv);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    button = gtk_button_new_with_label (_("Delete"));
    cv->del_value_button = button;
    gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_del_value_button_pressed), cv);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    gtk_widget_show_all (cv->data_page);

    return vbox;
}

static GtkWidget *
create_note_page (CommentView * cv)
{
    GtkWidget *scrolledwin;
    GtkWidget *label;

    /*
     * "Note" page 
     */
    label = gtk_label_new (_(" Note "));
    gtk_widget_set_name (label, "TabLabel");
    gtk_widget_show (label);

    cv->note_page = scrolledwin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
				    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_show (scrolledwin);

#ifdef USE_GTK2
    cv->note_box = gtk_text_view_new ();
#else /* USE_GTK2 */
    cv->note_box = gtk_text_new (gtk_scrolled_window_get_hadjustment
				 (GTK_SCROLLED_WINDOW (scrolledwin)),
				 gtk_scrolled_window_get_vadjustment
				 (GTK_SCROLLED_WINDOW (scrolledwin)));
    gtk_text_set_editable (GTK_TEXT (cv->note_box), TRUE);
#endif /* USE_GTK2 */
    gtk_container_add (GTK_CONTAINER (scrolledwin), cv->note_box);
    gtk_widget_show (cv->note_box);

    gtk_notebook_append_page (GTK_NOTEBOOK (cv->notebook),
			      scrolledwin, label);

    return cv->note_page;
}

static void
comment_view_set_sensitive_all (CommentView * cv, gboolean sensitive)
{
    g_return_if_fail (cv);

    gtk_widget_set_sensitive (cv->comment_clist, sensitive);
    gtk_widget_set_sensitive (cv->note_box, sensitive);

    gtk_widget_set_sensitive (cv->key_combo, sensitive);
    gtk_widget_set_sensitive (cv->value_entry, sensitive);

    gtk_widget_set_sensitive (cv->clear_button, sensitive);
    gtk_widget_set_sensitive (cv->apply_button, sensitive);
    gtk_widget_set_sensitive (cv->del_value_button, sensitive);

    gtk_widget_set_sensitive (cv->save_button, sensitive);
    gtk_widget_set_sensitive (cv->reset_button, sensitive);

    if (cv->comment)
    {
	if (file_exists (cv->comment->filename))
	    gtk_widget_set_sensitive (cv->delete_button, TRUE);
	else
	    gtk_widget_set_sensitive (cv->delete_button, FALSE);
    }
    else
	gtk_widget_set_sensitive (cv->delete_button, sensitive);
}

static void
comment_view_set_sensitive (CommentView * cv)
{
    const gchar *key_str, *value_str;
    gboolean selected = FALSE;

    g_return_if_fail (cv);

    key_str =
	gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry));
    value_str = gtk_entry_get_text (GTK_ENTRY (cv->value_entry));

    if (!cv->comment)
    {
	comment_view_set_sensitive_all (cv, FALSE);
	return;
    }
    else
    {
	comment_view_set_sensitive_all (cv, TRUE);
    }

#ifdef ENABLE_TREEVIEW
    {
	GtkTreeView *treeview = GTK_TREE_VIEW (cv->comment_clist);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
	GtkTreeModel *model;
	GtkTreeIter iter;

	selected = gtk_tree_selection_get_selected (selection, &model, &iter);
    }
#else /* ENABLE_TREEVIEW */
    if (cv->selected_row >= 0)
	selected = TRUE;
#endif /* ENABLE_TREEVIEW */

    if (selected)
    {
	gtk_widget_set_sensitive (cv->del_value_button, TRUE);
    }
    else
    {
	gtk_widget_set_sensitive (cv->del_value_button, FALSE);
    }

    if (!key_str || !*key_str)
    {
	gtk_widget_set_sensitive (cv->clear_button, FALSE);
	gtk_widget_set_sensitive (cv->apply_button, FALSE);
    }
    else
    {
	gtk_widget_set_sensitive (cv->clear_button, TRUE);
	gtk_widget_set_sensitive (cv->apply_button, TRUE);
    }
}

static void
entry_list_remove_item (GtkWidget * widget, GtkContainer * container)
{
    g_return_if_fail (widget && GTK_IS_WIDGET (widget));
    g_return_if_fail (container && GTK_IS_CONTAINER (container));

    gtk_container_remove (container, widget);
}

static void
comment_view_set_combo_list (CommentView * cv)
{
    GList  *list;

    g_return_if_fail (cv);
    g_return_if_fail (cv->key_combo);

    gtk_container_foreach (GTK_CONTAINER (GTK_COMBO (cv->key_combo)->list),
			   (GtkCallback) (entry_list_remove_item),
			   GTK_COMBO (cv->key_combo)->list);

    list = comment_get_data_entry_list ();
    while (list)
    {
	CommentDataEntry *data_entry = list->data;
	ImageInfo *info;
	GtkWidget *label;

	list = g_list_next (list);

	if (!data_entry)
	    continue;
	if (!data_entry->display)
	    continue;
	if (!data_entry->key || !*data_entry->key)
	    continue;

	if (cv->comment)
	    info = cv->comment->info;
	else
	    info = NULL;

	label = gtk_list_item_new_with_label (_(data_entry->display_name));

	gtk_object_set_data_full (GTK_OBJECT (label), "key",
				  g_strdup (data_entry->key),
				  (GtkDestroyNotify) g_free);
	gtk_container_add (GTK_CONTAINER (GTK_COMBO (cv->key_combo)->list),
			   label);
	gtk_widget_show (label);

	gtk_signal_connect (GTK_OBJECT (label), "select",
			    GTK_SIGNAL_FUNC (cb_combo_select), cv);
	gtk_signal_connect (GTK_OBJECT (label), "deselect",
			    GTK_SIGNAL_FUNC (cb_combo_deselect), cv);
    }
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry), "\0");
}

static void
comment_view_data_enter (CommentView * cv)
{
    CommentDataEntry *entry;
    const gchar *key, *dname, *value;
    gchar  *text[16];

    g_return_if_fail (cv);
    g_return_if_fail (cv->selected_item);

    key = gtk_object_get_data (GTK_OBJECT (cv->selected_item), "key");
    g_return_if_fail (key);

    dname = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry));
    value = gtk_entry_get_text (GTK_ENTRY (cv->value_entry));
    g_return_if_fail (dname && *dname);

    entry = comment_append_data (cv->comment, key, value);

    g_return_if_fail (entry);

#ifdef ENABLE_TREEVIEW
    {
	GtkTreeView *treeview = GTK_TREE_VIEW (cv->comment_clist);
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreeIter iter;
	gboolean go_next;
	CommentDataEntry *src_entry;

	go_next = gtk_tree_model_get_iter_first (model, &iter);
	for (; go_next; go_next = gtk_tree_model_iter_next (model, &iter))
	{
	    gtk_tree_model_get (model, &iter,
				COLUMN_RAW_ENTRY, &src_entry,
				COLUMN_TERMINATOR);
	    if (src_entry == entry)
		break;
	}

	text[0] = entry->display_name;
	text[1] = entry->value;
	if (!entry->userdef)
	    text[0] = _(text[0]);

	if (!go_next)
	    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    COLUMN_KEY, text[0],
			    COLUMN_VALUE, text[1],
			    COLUMN_RAW_ENTRY, entry, COLUMN_TERMINATOR);
    }
#else /* ENABLE_TREEVIEW */
    {
	GtkCList *clist = GTK_CLIST (cv->comment_clist);
	gint    row = gtk_clist_find_row_from_data (clist, entry);

	text[0] = entry->display_name;
	text[1] = entry->value;
	if (!entry->userdef)
	    text[0] = _(text[0]);

	if (row < 0)
	{
	    row = gtk_clist_append (clist, text);
	    gtk_clist_set_row_data (clist, row, entry);
	}
	else
	{
	    gtk_clist_set_text (clist, row, 1, text[1]);
	}
    }
#endif /* ENABLE_TREEVIEW */

    comment_view_set_sensitive (cv);
}

static void
comment_view_reset_data (CommentView * cv)
{
    ImageInfo *info;
    GList  *node;
    gchar  *text[2];

    comment_view_clear_data (cv);
    if (cv->comment)
    {
	info = cv->comment->info;
	node = cv->comment->data_list;
	while (node)
	{
	    CommentDataEntry *entry = node->data;

	    node = g_list_next (node);

	    if (!entry)
		continue;
	    if (!entry->display)
		continue;

	    text[0] = _(entry->display_name);
	    text[1] = entry->value;

#ifdef ENABLE_TREEVIEW
	    {
		GtkTreeModel *model;
		GtkTreeIter iter;
		model =
		    gtk_tree_view_get_model (GTK_TREE_VIEW
					     (cv->comment_clist));
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COLUMN_KEY, text[0],
				    COLUMN_VALUE, text[1],
				    COLUMN_RAW_ENTRY, entry,
				    COLUMN_TERMINATOR);
	    }
#else /* ENABLE_TREEVIEW */
	    {
		gint    row;
		row = gtk_clist_append (GTK_CLIST (cv->comment_clist), text);
		gtk_clist_set_row_data (GTK_CLIST (cv->comment_clist),
					row, entry);
	    }
#endif /* ENABLE_TREEVIEW */
	}

	if (cv->comment->note && *cv->comment->note)
	{
#ifdef USE_GTK2
	    GtkTextBuffer *buffer;

	    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (cv->note_box));
	    gtk_text_buffer_set_text (buffer, cv->comment->note, -1);
#else /* USE_GTK2 */

	    gtk_text_insert (GTK_TEXT (cv->note_box), NULL, NULL, NULL,
			     cv->comment->note, -1);

#endif /* USE_GTK2 */
	}
    }

    comment_view_set_sensitive (cv);
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
comment_view_clear_data (CommentView * cv)
{
    g_return_if_fail (cv);

#ifdef ENABLE_TREEVIEW
    {
	GtkTreeModel *model
	    = gtk_tree_view_get_model (GTK_TREE_VIEW (cv->comment_clist));
	gtk_list_store_clear (GTK_LIST_STORE (model));
    }
#else /* ENABLE_TREEVIEW */
    {
	gint    i;
	for (i = GTK_CLIST (cv->comment_clist)->rows - 1; i >= 0; i--)
	{
	    gtk_clist_remove (GTK_CLIST (cv->comment_clist), i);
	}
    }
#endif /* ENABLE_TREEVIEW */

    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry), "\0");
    gtk_entry_set_text (GTK_ENTRY (cv->value_entry), "\0");

#ifdef USE_GTK2
    {
	GtkTextBuffer *buffer;
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (cv->note_box));
	gtk_text_buffer_set_text (buffer, "\0", -1);
    }
#else /* USE_GTK2 */
    {
	GtkText *text;
	text = GTK_TEXT (cv->note_box);
	gtk_text_set_point (text, 0);
	gtk_text_forward_delete (text, gtk_text_get_length (text));
    }
#endif /* USE_GTK2 */

    comment_view_set_sensitive (cv);
}

void
comment_view_clear (CommentView * cv)
{
    comment_view_clear_data (cv);
    comment_view_set_sensitive_all (cv, FALSE);
}

gboolean
comment_view_change_file (CommentView * cv, ImageInfo * info)
{
    g_return_val_if_fail (cv, FALSE);

    comment_view_clear (cv);

    if (cv->comment)
    {
	comment_unref (cv->comment);
	cv->comment = NULL;
    }

    cv->comment = comment_get_from_image_info (info);
    gtk_signal_connect (GTK_OBJECT (cv->comment), "file_saved",
			GTK_SIGNAL_FUNC (cb_file_saved), cv);

    comment_view_reset_data (cv);

    comment_view_set_combo_list (cv);

    return TRUE;
}

CommentView *
comment_view_create ()
{
    CommentView *cv;
    GtkWidget *hbox, *hbox1;
    GtkWidget *button;

    cv = comment_view_new ();

    cv->main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_set_name (cv->main_vbox, "CommentView");
    gtk_signal_connect (GTK_OBJECT (cv->main_vbox), "destroy",
			GTK_SIGNAL_FUNC (cb_destroyed), cv);

    cv->notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (cv->notebook), 1);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (cv->notebook), TRUE);
    gtk_box_pack_start (GTK_BOX (cv->main_vbox), cv->notebook, TRUE, TRUE, 0);
    gtk_widget_show (cv->notebook);

    gtk_signal_connect (GTK_OBJECT (cv->notebook), "switch-page",
			GTK_SIGNAL_FUNC (cb_switch_page), cv);

    create_data_page (cv);
    create_note_page (cv);

    /*
     * button area 
     */
    hbox = cv->button_area = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (cv->main_vbox), cv->button_area, FALSE,
			FALSE, 5);
    gtk_widget_show (cv->main_vbox);

    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_end (GTK_BOX (hbox), hbox1, FALSE, TRUE, 0);
    gtk_box_set_homogeneous (GTK_BOX (hbox1), TRUE);

    button = gtk_button_new_with_label (_("Save"));
    cv->save_button = button;
    gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_save_button_pressed), cv);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    button = gtk_button_new_with_label (_("Reset"));
    cv->reset_button = button;
    gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_reset_button_pressed), cv);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    button = gtk_button_new_with_label (_("Delete"));
    cv->delete_button = button;
    gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_del_button_pressed), cv);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    gtk_widget_show_all (cv->main_vbox);

    comment_view_set_sensitive (cv);

    return cv;
}

CommentView *
comment_view_create_window (ImageInfo * info)
{
    CommentView *cv;
    gchar   buf[BUF_SIZE];

    g_return_val_if_fail (info, NULL);

    cv = comment_view_create ();
    if (!cv)
	return NULL;

    cv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_snprintf (buf, BUF_SIZE, _("Edit Comment (%s)"), info->filename);
    gtk_window_set_title (GTK_WINDOW (cv->window), buf);
    gtk_window_set_default_size (GTK_WINDOW (cv->window), 400, 350);
    gtk_window_set_position (GTK_WINDOW (cv->window), GTK_WIN_POS_MOUSE);

    gtk_container_add (GTK_CONTAINER (cv->window), cv->main_vbox);

    gtk_widget_show_all (cv->window);

    comment_view_change_file (cv, info);

    return cv;
}
