/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "dirtree.h"

#include "clist_edit.h"
#include "dialogs.h"
#include "file_utils.h"
#include "menu.h"
#include "pixbuf_utils.h"
#include "string_utils.h"

#include "dirview.h"
#include "browser.h"
#include "prefs.h"

#include "pixmaps/refresh.xpm"
#include "pixmaps/up.xpm"
#include "pixmaps/down.xpm"
#include "pixmaps/left.xpm"
#include "pixmaps/right.xpm"
#include "pixmaps/dotfile.xpm"
#include "pixmaps/home.xpm"

#define min(a, b) (((a)<(b))?(a):(b))

DirView *dirview = NULL;

static void cb_dirview_refresh_dir_tree (GtkWidget * widget, DirView * dv);
static void cb_dirview_mkdir (void);
static void cb_dirview_rename_dir (void);
static void cb_dirview_delete_dir (void);

static GtkItemFactoryEntry dirview_popup_items[] = {
    {N_("/_Refresh"), NULL, cb_dirview_refresh_dir_tree, 0, NULL},
    {"/---", NULL, NULL, 0, "<Separator>"},
    {N_("/_Make Directory"), NULL, cb_dirview_mkdir, 0, NULL},
    {N_("/Re_name Directory"), NULL, cb_dirview_rename_dir, 0, NULL},
    {N_("/_Delete Directory"), NULL, cb_dirview_delete_dir, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void dirview_update_toolbar (DirView * dv);
static void dirview_make_visible (DirView * dv, GtkCTreeNode * node);

static  gint
cb_dirview_button_press_event (GtkWidget * widget, GdkEventButton * event,
			       gpointer data)
{
    DirView *dv = data;

    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
	GtkWidget *dirview_popup, *menuitem;
	GtkItemFactory *ifactory;
	GtkCTreeNode *node;
	DirTreeNode *dirnode;
	gint    n_menu_items;
	gchar  *path, *parent, *node_text;
	gboolean expanded, is_leaf;

	node = GTK_CLIST (dv->dirtree)->selection->data;

	if (!node)
	    return FALSE;

	gtk_ctree_get_node_info (GTK_CTREE (dv->dirtree), node, &node_text,
				 NULL, NULL, NULL, NULL, NULL, &is_leaf,
				 &expanded);

	dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node);

	/*
	 * create popup menu 
	 */
	n_menu_items = sizeof (dirview_popup_items)
	    / sizeof (dirview_popup_items[0]) - 1;
	dirview_popup = menu_create_items (NULL, dirview_popup_items,
					   n_menu_items, "<DirViewPop>", dv);

	/*
	 * set sensitive 
	 */
	ifactory = gtk_item_factory_from_widget (dirview_popup);

	if (!strcmp (node_text, ".") || !strcmp (node_text, ".."))
	{
	    menuitem =
		gtk_item_factory_get_item (ifactory, "/Make Directory");
	    gtk_widget_set_sensitive (menuitem, FALSE);
	}

	if (!iswritable (dirnode->path))
	{
	    menuitem =
		gtk_item_factory_get_item (ifactory, "/Make Directory");
	    gtk_widget_set_sensitive (menuitem, FALSE);
	}

	path = g_strdup (dirnode->path);
	if (path[strlen (path) - 1] == '/')
	    path[strlen (path) - 1] = '\0';
	parent = g_path_get_dirname (path);

	if (!parent || !strcmp (parent, ".") || !iswritable (parent)
	    || !strcmp (node_text, ".") || !strcmp (node_text, ".."))
	{
	    menuitem =
		gtk_item_factory_get_item (ifactory, "/Rename Directory");
	    gtk_widget_set_sensitive (menuitem, FALSE);
	    menuitem =
		gtk_item_factory_get_item (ifactory, "/Delete Directory");
	    gtk_widget_set_sensitive (menuitem, FALSE);
	}

	g_free (path);
	g_free (parent);

	/*
	 * popup menu 
	 */
	gtk_menu_popup (GTK_MENU (dirview_popup), NULL, NULL, NULL, NULL, 0,
			0);

#ifdef USE_GTK2
	g_object_ref  (GTK_OBJECT (dirview_popup));
	g_object_ref_sink (GTK_OBJECT (dirview_popup));
#endif

	g_object_unref (dirview_popup);
    }

    return FALSE;
}

static  gint
cb_dirview_button_release_event (GtkWidget * widget, GdkEventButton * event,
				 DirView * dv)
{
    dirview_update_toolbar (dv);

    return FALSE;
}

static  gboolean
cb_dirview_key_press (GtkWidget * widget, GdkEventKey * event, DirView * dv)
{
    guint   keyval;
    GtkCTreeNode *node;
    GList  *sel_list;

    keyval = event->keyval;

    sel_list = GTK_CLIST (dv->dirtree)->selection;

    if (!sel_list)
	return FALSE;

    node = sel_list->data;

    switch (keyval)
    {
      case GDK_space:
	  dirtree_freeze (GTK_CTREE (dv->dirtree));
	  gtk_ctree_toggle_expansion (GTK_CTREE (dv->dirtree), node);
	  dirtree_thaw (GTK_CTREE (dv->dirtree));

	  break;
      case GDK_Right:
	  dirtree_freeze (GTK_CTREE (dv->dirtree));
	  gtk_ctree_expand (GTK_CTREE (dv->dirtree), node);
	  dirtree_thaw (GTK_CTREE (dv->dirtree));

	  break;
      case GDK_Left:
	  dirtree_freeze (GTK_CTREE (dv->dirtree));
	  gtk_ctree_collapse (GTK_CTREE (dv->dirtree), node);
	  dirtree_thaw (GTK_CTREE (dv->dirtree));

	  break;
    }

    dirview_update_toolbar (dv);

    return FALSE;
}

static void
cb_dirview_select_file (GtkWidget * widget, gchar * path, DirView * dv)
{
    if (dv->lock_select)
	return;

    dirview_update_toolbar (dv);
    browser_select_dir (path);
}

static void
cb_dirview_refresh_dir_tree (GtkWidget * widget, DirView * dv)
{
    GtkCTreeNode *parent;
    gchar  *node_text;
    gboolean expanded, is_leaf;

    browser->last_path = g_string_assign (browser->last_path, "");

    parent = GTK_CLIST (dirview->dirtree)->selection->data;

    gtk_ctree_get_node_info (GTK_CTREE (dirview->dirtree), parent, &node_text,
			     NULL, NULL, NULL, NULL, NULL, &is_leaf,
			     &expanded);

    if (!strcmp (node_text, ".."))
    {
	parent = (GTK_CTREE_ROW (parent))->parent;

	if (!parent)
	    return;

	parent = (GTK_CTREE_ROW (parent))->parent;

	if (!parent)
	    return;

	dirview_make_visible (dirview, parent);
    }
    else if (!strcmp (node_text, "."))
    {
	parent = (GTK_CTREE_ROW (parent))->parent;

	if (!parent)
	    return;

	dirview_make_visible (dirview, parent);
    }

    dirtree_freeze (DIRTREE (dirview->dirtree));

    dirtree_refresh_node (DIRTREE (dirview->dirtree), parent);
    dirtree_refresh_tree ((DirTree *) dirview->dirtree, parent, TRUE);

    dirtree_thaw (DIRTREE (dirview->dirtree));

    dirview_update_toolbar (dirview);
}

static void
cb_dirview_expand (GtkWidget * widget, DirView * dv)
{
    GList  *sel_list;
    GtkCTreeNode *node;

    sel_list = GTK_CLIST (dv->dirtree)->selection;

    if (!sel_list)
	return;

    node = sel_list->data;

    dirtree_freeze (GTK_CTREE (dv->dirtree));
    gtk_ctree_expand (GTK_CTREE (dv->dirtree), node);
    dirtree_thaw (GTK_CTREE (dv->dirtree));

    dirview_update_toolbar (dirview);
}

static void
cb_dirview_collapse (GtkWidget * widget, DirView * dv)
{
    GList  *sel_list;
    GtkCTreeNode *node;

    sel_list = GTK_CLIST (dv->dirtree)->selection;

    if (!sel_list)
	return;

    node = sel_list->data;

    dirtree_freeze (GTK_CTREE (dv->dirtree));
    gtk_ctree_collapse (GTK_CTREE (dv->dirtree), node);
    dirtree_thaw (GTK_CTREE (dv->dirtree));

    dirview_update_toolbar (dirview);
}

static void
cb_dirview_home(GtkWidget * widget, DirView * dv)
{
    GList  *sel_list;
    GtkCTreeNode *node;
    GtkCTreeNode *child;
    char   *rp, *tmp;

    sel_list = GTK_CLIST (dv->dirtree)->selection;
    node = sel_list->data;

	while (GTK_CTREE_NODE_PREV (node)) {
		node = GTK_CTREE_NODE_PREV (node);
	}

    rp = g_strdup (getenv("HOME"));
    tmp = strtok (rp, "/");
	node = dirtree_find_file(dv->dirtree, node, tmp);
	tmp = strtok(NULL, "/");
	node = dirtree_find_file(dv->dirtree, node, tmp);
	dirview_select_node(dv, node);

	g_free (rp);
}

static void
cb_dirview_down (GtkWidget * widget, DirView * dv)
{
    GtkCTreeNode *node;

    GList  *sel_list;

    sel_list = GTK_CLIST (dv->dirtree)->selection;

    if (!sel_list)
	return;

    node = sel_list->data;
    node = GTK_CTREE_NODE_NEXT (node);

	dirview_select_node(dv, node);
}

static void
cb_dirview_up (GtkWidget * widget, DirView * dv)
{
    GtkCTreeNode *node;

    GList  *sel_list;

    sel_list = GTK_CLIST (dv->dirtree)->selection;

    if (!sel_list)
	return;

    node = sel_list->data;

    node = GTK_CTREE_NODE_PREV (node);

	dirview_select_node(dv, node);
}

static void
cb_dirview_show_dotfile (GtkWidget * widget, DirView * dv)
{
    dv->lock_select = TRUE;

    if (DIRTREE (dv->dirtree)->show_dotfile)
	DIRTREE (dv->dirtree)->show_dotfile = FALSE;
    else
	DIRTREE (dv->dirtree)->show_dotfile = TRUE;

    cb_dirview_refresh_dir_tree (widget, dv);

    dv->lock_select = FALSE;
}

 /*
  * make directory 
  */

static gint cb_dirview_rename_node (ClistEditData * ced, const gchar * old,
				    const gchar * new, gpointer data);

static void
cb_dirview_mkdir (void)
{
    GtkCTreeNode *node;
    DirTreeNode *dirnode;
    gboolean success;
    gint    row;
    gchar  *path, *basename;
    gchar  *new_path;

    node = GTK_CLIST (dirview->dirtree)->selection->data;

    if (!node)
	return;

    dirnode =
	gtk_ctree_node_get_row_data (GTK_CTREE (dirview->dirtree), node);

    if (!dirnode)
	return;

    if (!iswritable (dirnode->path))
    {
	gchar  *message = NULL;

	basename = g_path_get_basename(dirnode->path);
	message =
	    g_strdup_printf ("%s \"%s\".", _("Permission denied"),
			     basename);
	dialog_message (_("Error"), message, browser->window);
	g_free(basename);
	g_free (message);

	return;
    }

    path = g_strconcat (dirnode->path, "/", _("new folder"), NULL);

    new_path = unique_filename (path, NULL, NULL, FALSE);

    success = makedir (new_path);

    dirtree_freeze (DIRTREE (dirview->dirtree));

    dirtree_refresh_node (DIRTREE (dirview->dirtree), node);

    dirtree_refresh_tree (DIRTREE (dirview->dirtree), node, FALSE);

	basename = g_path_get_basename(new_path);
    node =
	dirtree_find_file (DIRTREE (dirview->dirtree), node,
			   basename);
	g_free(basename);
    dirtree_refresh_tree (DIRTREE (dirview->dirtree), node, TRUE);

    dirview_scroll_center ();

    dirtree_thaw (DIRTREE (dirview->dirtree));

    row =
	g_list_position (GTK_CLIST (dirview->dirtree)->row_list,
			 (GList *) node);

    if (row < 0)
	return;

    dirview_make_visible (dirview, node);

    gtk_ctree_node_moveto (GTK_CTREE (dirview->dirtree), node, 0, 0.0, 0.0);

    clist_edit_by_row (GTK_CLIST (dirview->dirtree), row, 0,
		       cb_dirview_rename_node, dirview);

    g_free (path);
    g_free (new_path);
}

 /*
  * rename directory 
  */

static gint timer_refresh;
static gchar *refresh_dir = NULL;

static int
cb_dirview_refresh_timer_proc (gpointer data)
{
    GtkCTreeNode *parent;
    GtkCTreeNode *node = data;

    parent = (GTK_CTREE_ROW (node))->parent;

    dirtree_freeze (DIRTREE (dirview->dirtree));

    if (parent)
	dirtree_refresh_tree (DIRTREE (dirview->dirtree), parent, FALSE);

    if (refresh_dir)
    {
	node =
	    dirtree_find_file (DIRTREE (dirview->dirtree), parent,
			       refresh_dir);
	dirtree_refresh_tree (DIRTREE (dirview->dirtree), node, TRUE);

	g_free (refresh_dir);
	refresh_dir = NULL;
    }

    dirview_scroll_center ();

    dirtree_thaw (DIRTREE (dirview->dirtree));

    return FALSE;
}

static  gint
cb_dirview_rename_node (ClistEditData * ced, const gchar * old,
			const gchar * new, gpointer data)
{
    DirView *dv = data;
    DirTreeNode *dirnode;
    GtkCTreeNode *node;
    gchar  *src_path;
    gchar  *dest_path;

    node = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), (guint) ced->row);

    if (!node)
	return FALSE;

    dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node);

    if (!dirnode)
	return FALSE;

    src_path = g_strdup (dirnode->path);

    if (src_path[strlen (src_path) - 1] == '/')
	src_path[strlen (src_path) - 1] = '\0';

    dest_path = g_strconcat (g_dirname (src_path), "/", new, NULL);

    if (rename (src_path, dest_path) < 0)
    {
	gchar  *message = NULL;
	gchar  *basename;

	basename = g_path_get_basename(src_path);
	message =
	    g_strdup_printf ("%s \"%s\".\n%s", _("Faild to rename directory"),
			     basename, g_strerror (errno));
	dialog_message (_("Error"), message, browser->window);
	g_free(basename);
	g_free (message);
    }
    else
    {
	refresh_dir = g_strdup (new);

	timer_refresh =
	    gtk_timeout_add (100, cb_dirview_refresh_timer_proc, node);
    }

    g_free (src_path);
    g_free (dest_path);

    return FALSE;
}

static void
cb_dirview_rename_dir (void)
{
    GtkCTreeNode *node;
    DirTreeNode *dirnode;
    gboolean exist;
    struct stat st;
    gint    row;

    node = GTK_CLIST (dirview->dirtree)->selection->data;

    if (!node)
	return;

    dirnode =
	gtk_ctree_node_get_row_data (GTK_CTREE (dirview->dirtree), node);

    if (!dirnode)
	return;

    /*
     * check for direcotry exist 
     */

    exist = !lstat (dirnode->path, &st);

    if (!exist)
    {
	gchar  *message = NULL;
	gchar  *basename;

	basename = g_path_get_basename(dirnode->path);
	message =
	    g_strdup_printf ("%s \"%s\".", _("Directory not exist"),
			     basename);
	dialog_message (_("Error"), message, browser->window);
	g_free(basename);
	g_free (message);

	return;
    }

    row =
	g_list_position (GTK_CLIST (dirview->dirtree)->row_list,
			 (GList *) node);

    if (row < 0)
	return;

    dirview_make_visible (dirview, node);

    gtk_ctree_node_moveto (GTK_CTREE (dirview->dirtree), node, 0, 0.0, 0.0);

    clist_edit_by_row (GTK_CLIST (dirview->dirtree), row, 0,
		       cb_dirview_rename_node, dirview);

    return;
}

 /*
  * delete directory 
  */

static void
cb_dirview_delete_dir (void)
{
    GtkCTreeNode *node, *parent;
    DirTreeNode *dirnode;
    gboolean exist;
    struct stat st;
    ConfirmType action;
    gchar  *path = NULL;
    gchar  *message = NULL;
	gchar  *basename;


    node = GTK_CLIST (dirview->dirtree)->selection->data;

    if (!node)
	return;

    dirnode =
	gtk_ctree_node_get_row_data (GTK_CTREE (dirview->dirtree), node);

    if (!dirnode)
	return;

    /*
     * check direcotry exist or not 
     */
    exist = !lstat (dirnode->path, &st);

    if (!exist)
    {

	basename = g_path_get_basename(dirnode->path);
	message =
	    g_strdup_printf ("%s \"%s\".", _("Directory not exist"),
			     basename);
	dialog_message (_("Error"), message, browser->window);
	g_free(basename);
	g_free (message);

	return;
    }

    /*
     * confirm 
     */
	basename = g_path_get_basename(dirnode->path);
    message =
	g_strdup_printf ("%s \"%s\" ?", _("Delete directory"),
			 basename);
    action =
	dialog_confirm (_("Confirm Deleting Directory"), message,
			browser->window);
	g_free(basename);
    g_free (message);

    if (action != CONFIRM_YES)
	return;

    path = g_strdup (dirnode->path);

    if (path[strlen (path) - 1] == '/')
	path[strlen (path) - 1] = '\0';

    if (rmdir (path) < 0)
    {
	basename = g_path_get_basename(path);
	message =
	    g_strdup_printf ("%s \"%s\".\n%s", _("Faild to delete directory"),
			     basename, g_strerror (errno));
	dialog_message (_("Error"), message, browser->window);
	g_free(basename);
	g_free (message);

	g_free (path);

	return;
    }

    g_free (path);

    /*
     * refresh dir tree 
     */
    parent = (GTK_CTREE_ROW (node))->parent;

    if (!parent)
	return;

    dirtree_freeze (DIRTREE (dirview->dirtree));
    dirtree_refresh_node (DIRTREE (dirview->dirtree), parent);
    dirtree_refresh_tree (DIRTREE (dirview->dirtree), parent, TRUE);
    dirview_make_visible (dirview, parent);
    dirtree_thaw (DIRTREE (dirview->dirtree));
}

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
dirview_select_node(DirView * dv, GtkCTreeNode * node)
{
    if (node != NULL) {
		dirtree_freeze (GTK_CTREE (dv->dirtree));

		gtk_ctree_select (GTK_CTREE (dv->dirtree), node);
		GTK_CLIST (dv->dirtree)->focus_row =
			GTK_CLIST (dv->dirtree)->rows - g_list_length ((GList *) node);

		dirtree_thaw (GTK_CTREE (dv->dirtree));

		dirview_scroll_center ();
		dirview_update_toolbar (dirview);
    }
}

static void
dirview_make_visible (DirView * dv, GtkCTreeNode * node)
{
    GtkCTreeNode *parent;

    parent = GTK_CTREE_ROW (node)->parent;

    gtk_clist_freeze (GTK_CLIST (dv->dirtree));

    while (parent)
    {
	if (!GTK_CTREE_ROW (parent)->expanded)
	{
	    gtk_ctree_expand (GTK_CTREE (dv->dirtree), parent);
	}
	parent = GTK_CTREE_ROW (parent)->parent;
    }

    /*
     * the realized test is a hack, otherwise the scrollbar is incorrect at start up 
     */
    if (GTK_WIDGET_REALIZED (dv->dirtree)
	&& gtk_ctree_node_is_visible (GTK_CTREE (dv->dirtree),
				      node) != GTK_VISIBILITY_FULL)
    {
	gtk_ctree_node_moveto (GTK_CTREE (dv->dirtree), node, 0, 0, 0.0);
    }

    gtk_clist_thaw (GTK_CLIST (dv->dirtree));
}

static GtkWidget *
dirview_create_toolbar (DirView * dv)
{
    GtkWidget *toolbar;
    GtkWidget *iconw;

    g_return_val_if_fail (dv, NULL);

#ifdef USE_GTK2
    toolbar = gtk_toolbar_new ();
#else
    toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
    gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar),
				 GTK_TOOLBAR_SPACE_LINE);
    gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar), 16);
#endif

    /*
     * refresh 
     */
    iconw = pixbuf_create_pixmap_from_xpm_data (refresh_xpm);
    dv->toolbar_refresh_btn = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
						       NULL,
						       _("Refresh"),
						       NULL,
						       iconw,
						       GTK_SIGNAL_FUNC
						       (cb_dirview_refresh_dir_tree),
						       dv);
    /*
     * up 
     */
    iconw = pixbuf_create_pixmap_from_xpm_data (up_xpm);
    dv->toolbar_up_btn = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
						  NULL,
						  _("Up"),
						  NULL,
						  iconw,
						  GTK_SIGNAL_FUNC
						  (cb_dirview_up), dv);
    /*
     * down 
     */
    iconw = pixbuf_create_pixmap_from_xpm_data (down_xpm);
    dv->toolbar_down_btn = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
						    NULL,
						    _("Down"),
						    NULL,
						    iconw,
						    GTK_SIGNAL_FUNC
						    (cb_dirview_down), dv);
    /*
     * collapse 
     */
    iconw = pixbuf_create_pixmap_from_xpm_data (left_xpm);
    dv->toolbar_collapse_btn = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
							NULL,
							_("Collapse"),
							NULL,
							iconw,
							GTK_SIGNAL_FUNC
							(cb_dirview_collapse),
							dv);
    /*
     * expand 
     */
    iconw = pixbuf_create_pixmap_from_xpm_data (right_xpm);
    dv->toolbar_expand_btn = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
						      NULL,
						      _("Expand"),
						      NULL,
						      iconw,
						      GTK_SIGNAL_FUNC
						      (cb_dirview_expand),
						      dv);

    /*
     * show/hide dotfile
     */
    iconw = pixbuf_create_pixmap_from_xpm_data (dotfile_xpm);
    dv->toolbar_show_dotfile_btn =
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL,
				 _("Show/Hide dotfile"),
				 NULL, iconw,
				 GTK_SIGNAL_FUNC (cb_dirview_show_dotfile),
				 dv);

	/*
	 * home button
	 */
    iconw = pixbuf_create_pixmap_from_xpm_data (home_xpm);
    dv->toolbar_go_home = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
						      NULL,
						      _("Home"),
						      NULL,
						      iconw,
						      GTK_SIGNAL_FUNC
						      (cb_dirview_home),
						      dv);

    gtk_widget_show_all (toolbar);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

    return toolbar;
}

static void
dirview_update_toolbar (DirView * dv)
{
    GtkCTreeNode *node;
    GList  *sel_list;
    gboolean is_leaf, expanded;
    gint    current, len;

    sel_list = GTK_CLIST (dv->dirtree)->selection;

    if (!sel_list)
	return;

    node = sel_list->data;

    len = GTK_CLIST (GTK_CTREE (dv->dirtree))->rows;

    current = len - g_list_length ((GList *) node);

    if (current == 0)
    {
	gtk_widget_set_sensitive (dv->toolbar_up_btn, FALSE);
	gtk_widget_set_sensitive (dv->toolbar_down_btn, TRUE);
    }
    else if (current == (len - 1))
    {
	gtk_widget_set_sensitive (dv->toolbar_up_btn, TRUE);
	gtk_widget_set_sensitive (dv->toolbar_down_btn, FALSE);
    }
    else
    {
	gtk_widget_set_sensitive (dv->toolbar_up_btn, TRUE);
	gtk_widget_set_sensitive (dv->toolbar_down_btn, TRUE);
    }

    gtk_ctree_get_node_info (GTK_CTREE (dv->dirtree), node, NULL, NULL,
			     NULL, NULL, NULL, NULL, &is_leaf, &expanded);

    gtk_widget_set_sensitive (dv->toolbar_expand_btn, !is_leaf);
    gtk_widget_set_sensitive (dv->toolbar_collapse_btn, !is_leaf);

    if (is_leaf)
	return;

    if (expanded)
    {
	gtk_widget_set_sensitive (dv->toolbar_expand_btn, FALSE);
	gtk_widget_set_sensitive (dv->toolbar_collapse_btn, TRUE);
    }
    else
    {
	gtk_widget_set_sensitive (dv->toolbar_expand_btn, TRUE);
	gtk_widget_set_sensitive (dv->toolbar_collapse_btn, FALSE);
    }
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
dirview_create (const gchar * start_path, GtkWidget * parent_win)
{
    dirview = g_new0 (DirView, 1);

    /*
     * main vbox 
     */
    dirview->container = gtk_vbox_new (FALSE, 0);
    g_object_ref (dirview->container);
    gtk_object_set_data_full (GTK_OBJECT (parent_win), "dirview_container",
			      dirview->container,
			      (GtkDestroyNotify) g_object_unref);
    gtk_widget_show (dirview->container);

    /*
     * toolbar 
     */
    dirview->toolbar_eventbox = gtk_event_box_new ();
    gtk_container_set_border_width (GTK_CONTAINER (dirview->toolbar_eventbox),
				    1);
    gtk_box_pack_start (GTK_BOX (dirview->container),
			dirview->toolbar_eventbox, FALSE, FALSE, 0);
    gtk_widget_show (dirview->toolbar_eventbox);

    dirview->toolbar = dirview_create_toolbar (dirview);
    gtk_container_add (GTK_CONTAINER (dirview->toolbar_eventbox),
		       dirview->toolbar);

    /*
     * scrolled window 
     */
    dirview->scroll_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (dirview->scroll_win), 1);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (dirview->scroll_win),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (dirview->container), dirview->scroll_win,
			TRUE, TRUE, 0);
    gtk_widget_show (dirview->scroll_win);

    /*
     * dirtree 
     */
    dirview->dirtree =
	dirtree_new (parent_win, start_path, conf.scan_dir, conf.check_hlinks,
		     conf.show_dotfile, conf.dirtree_line_style,
		     conf.dirtree_expander_style);

    gtk_container_add (GTK_CONTAINER (dirview->scroll_win), dirview->dirtree);
    gtk_widget_set_usize (GTK_WIDGET (dirview->scroll_win),
			  conf.dirtree_width, conf.dirtree_height);
    gtk_widget_show (dirview->dirtree);

    /*
     * callback singnals 
     */
    gtk_signal_connect (GTK_OBJECT (dirview->dirtree), "button_press_event",
			GTK_SIGNAL_FUNC (cb_dirview_button_press_event),
			dirview);

    gtk_signal_connect (GTK_OBJECT (dirview->dirtree), "button_release_event",
			GTK_SIGNAL_FUNC (cb_dirview_button_release_event),
			dirview);

    gtk_signal_connect (GTK_OBJECT (dirview->dirtree), "key_press_event",
			GTK_SIGNAL_FUNC (cb_dirview_key_press), dirview);

    gtk_signal_connect (GTK_OBJECT (dirview->dirtree), "select_file",
			GTK_SIGNAL_FUNC (cb_dirview_select_file), dirview);

    gtk_widget_show (dirview->scroll_win);
    dirview_update_toolbar (dirview);
}

void
dirview_scroll_center (void)
{
    GtkAdjustment *vadj;
    gfloat  pos;

    vadj =
	gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
					     (dirview->scroll_win));

    pos = (vadj->upper * GTK_CLIST (DIRTREE (dirview->dirtree))->focus_row)
	/ (gfloat) GTK_CLIST (dirview->dirtree)->rows;

    gtk_adjustment_set_value (vadj, min (vadj->upper - vadj->page_size, pos));
}

void
dirview_destroy (void)
{
    gtk_widget_destroy (dirview->dirtree);

    g_free (dirview);
    dirview = NULL;
}
