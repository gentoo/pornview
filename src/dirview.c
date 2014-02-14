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
#include "pixmaps/collapse.xpm"
#include "pixmaps/expand.xpm"

#define min(a, b) (((a)<(b))?(a):(b))
#define TOOLBAR_APPEND -1
#define TOOLBAR_PREPEND 0

DirView *dirview = NULL;

static gint timer_refresh;

static gchar *refresh_dir = NULL;

static const gchar *dirview_toolbar_xml =
"<ui>"
	"<toolbar name=\"DirviewToolbar\" action=\"DirviewToolBarAction\">"
		"<placeholder name=\"ToolItems\">"
			"<separator/>"
			"<toolitem name=\"Home\" action=\"HomeAction\"/>"
			"<toolitem name=\"Refresh\" action=\"RefreshAction\"/>"
			"<toolitem name=\"ShowHideDotfile\""
				"action=\"ShowHideDotfileAction\"/>"
			"<separator/>"
		"</placeholder>"
	"</toolbar>"
"</ui>";

/* FIXME: accelerator does not work */
static GtkActionEntry dirview_toolbar_entries[] = {
	{ "DirviewToolBarAction", NULL, NULL,		/* name, stock id, label */
		NULL, NULL,								/* accelerator, tooltip */
		NULL },									/* callback */
	{ "HomeAction", "user-home", N_("_Home"),
		NULL, "Go home",
		G_CALLBACK(cb_dirview_go_home) },
	{ "RefreshAction", "view-refresh", N_("_Refresh"),
		NULL, "Refresh",
		G_CALLBACK(cb_dirview_refresh) },
	/* TODO: appropriate icon */
	{ "ShowHideDotfileAction", "folder-visiting", N_("_Show/Hide dotfile"),
		"<Control>h", "Show/Hide dotfile",
		G_CALLBACK(cb_dirview_show_dotfile) }
};

static guint n_dirview_toolbar_entries = G_N_ELEMENTS(dirview_toolbar_entries);

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static
void cb_dirview_selection_changed(GtkTreeSelection *treeselection,
		gpointer user_data)
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *path;

	treeview = gtk_tree_selection_get_tree_view(treeselection);
	model = gtk_tree_view_get_model(treeview);

	if (gtk_tree_selection_get_selected(treeselection,
			&model,
			&iter)) {
		path = get_full_dirname_iter(model, &iter);
		browser_select_dir(path);
		g_free(path);
	}
}

static void
cb_dirview_row_activated(GtkTreeView *treeview,
		GtkTreePath *treepath,
		GtkTreeViewColumn *treeview_column,
		gpointer userdata)
{
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);
	gchar *path;

	if (gtk_tree_view_row_expanded(treeview, treepath)) {
		gtk_tree_view_collapse_row(treeview, treepath);
		return;
	} else {
		gtk_tree_view_expand_row(treeview, treepath, FALSE);
	}

	path = get_full_dirname(model, treepath);
	browser_select_dir(path);
	g_free(path);
}

static void
cb_dirview_row_expanded(GtkTreeView *treeview,
		GtkTreeIter *iter,
		GtkTreePath *treepath,
		gpointer userdata)
{
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);

	gtk_tree_store_set(GTK_TREE_STORE(model),
			iter,
			COL_ICON, ofolder_pixmap,
			-1);

	dirtree_append_sub_subdirs(model, treepath, (DirTree *)DIRVIEW_DIRTREE);
}

static void
cb_dirview_row_test_collapsed(GtkTreeView *treeview,
		GtkTreeIter *iter,
		GtkTreePath *treepath,
		gpointer userdata)
{
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);

	gtk_tree_store_set(GTK_TREE_STORE(model),
			iter,
			COL_ICON, folder_pixmap,
			-1);

	/* FIXME: simpler please? Child rows of the collapsed row
	 * don't seem to get the "row-collapsed" signal */
	gtk_tree_path_down(treepath);
	/* collapse all subdirs recursively */
	while (gtk_tree_model_get_iter(
				model,
				iter, treepath)) {
		if (gtk_tree_view_row_expanded(treeview, treepath)) {
			GtkTreePath *tmp_path = gtk_tree_path_copy(treepath);
			cb_dirview_row_test_collapsed(treeview, iter, tmp_path, userdata);
			gtk_tree_path_free(tmp_path);
		}

		gtk_tree_path_next(treepath);
	}
}

static gboolean
cb_dirview_button_press_event(GtkWidget *widget,
		GdkEventButton *event,
		gpointer data)
{
	/* right click */
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		gchar *path;
		GtkTreePath *tree_path;
		GtkWidget *rightclick_menu = gtk_menu_new();
		GtkTreeModel *model = gtk_tree_view_get_model(TREEVIEW);

		dirview_create_rightclick_menu(rightclick_menu);

		gtk_menu_popup(GTK_MENU(rightclick_menu),
				NULL,
				NULL,
				NULL,
				data,
				event->button,
				event->time);
	}

	/* also call the default action */
	return FALSE;
}

/* static  gint */
/* cb_dirview_button_press_event (GtkWidget * widget, GdkEventButton * event, */
				   /* gpointer data) */
/* { */
    /* DirView *dv = data; */
	/* GtkCTreeNode *node = GTK_CLIST(dv->dirtree)->selection->data; */

	/* if (event->type == GDK_BUTTON_PRESS && event->button == 1) { */
		/* prev_node = node; */
		/* return FALSE; */
	/* } */

    /* if (event->type == GDK_BUTTON_PRESS && event->button == 3) */
    /* { */
	/* GtkWidget *dirview_popup, *menuitem; */
	/* GtkItemFactory *ifactory; */
	/* DirTreeNode *dirnode; */
	/* gint    n_menu_items; */
	/* gchar  *path, *parent, *node_text = ""; */
	/* gboolean expanded, is_leaf; */

	/* if (!node) */
		/* return FALSE; */

	/* gtk_ctree_get_node_info (GTK_CTREE (dv->dirtree), node, &node_text, */
				 /* NULL, NULL, NULL, NULL, NULL, &is_leaf, */
				 /* &expanded); */

	/* dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node); */

	/*
	 * create popup menu
	 */
	/* n_menu_items = sizeof (dirview_popup_items) */
		/* / sizeof (dirview_popup_items[0]) - 1; */
	/* dirview_popup = menu_create_items (NULL, dirview_popup_items, */
					   /* n_menu_items, "<DirViewPop>", dv); */

	/*
	 * set sensitive
	 */
	/* ifactory = gtk_item_factory_from_widget (dirview_popup); */

	/* if (!strcmp (node_text, ".") || !strcmp (node_text, "..")) */
	/* { */
		/* menuitem = */
		/* gtk_item_factory_get_item (ifactory, "/Make Directory"); */
		/* gtk_widget_set_sensitive (menuitem, FALSE); */
	/* } */

	/* if (!iswritable (dirnode->path)) */
	/* { */
		/* menuitem = */
		/* gtk_item_factory_get_item (ifactory, "/Make Directory"); */
		/* gtk_widget_set_sensitive (menuitem, FALSE); */
	/* } */

	/* path = g_strdup (dirnode->path); */
	/* if (path[strlen (path) - 1] == '/') */
		/* path[strlen (path) - 1] = '\0'; */
	/* parent = g_path_get_dirname (path); */

	/* if (!parent || !strcmp (parent, ".") || !iswritable (parent) */
		/* || !strcmp (node_text, ".") || !strcmp (node_text, "..")) */
	/* { */
		/* menuitem = */
		/* gtk_item_factory_get_item (ifactory, "/Rename Directory"); */
		/* gtk_widget_set_sensitive (menuitem, FALSE); */
		/* menuitem = */
		/* gtk_item_factory_get_item (ifactory, "/Delete Directory"); */
		/* gtk_widget_set_sensitive (menuitem, FALSE); */
	/* } */

	/* g_free (path); */
	/* g_free (parent); */

	/*
	 * popup menu
	 */
	/* gtk_menu_popup (GTK_MENU (dirview_popup), NULL, NULL, NULL, NULL, */
			/* 3, gtk_get_current_event_time()); */

	/* g_object_ref  (GTK_OBJECT (dirview_popup)); */
	/* g_object_ref_sink (GTK_OBJECT (dirview_popup)); */

	/* g_object_unref (dirview_popup); */
    /* } */

    /* return FALSE; */
/* } */

/* static  gboolean */
/* cb_dirview_key_press (GtkWidget * widget, GdkEventKey * event, DirView * dv) */
/* { */
    /* guint   keyval; */
    /* GtkCTreeNode *node; */
    /* GList  *sel_list; */

    /* keyval = event->keyval; */

    /* sel_list = GTK_CLIST (dv->dirtree)->selection; */

    /* if (!sel_list) */
	/* return FALSE; */

    /* node = sel_list->data; */
	/* prev_node = node; */

    /* switch (keyval) */
    /* { */
      /* case GDK_space: */
	  /* dirtree_freeze (GTK_CTREE (dv->dirtree)); */
	  /* gtk_ctree_toggle_expansion (GTK_CTREE (dv->dirtree), node); */
	  /* dirtree_thaw (GTK_CTREE (dv->dirtree)); */

	  /* break; */
      /* case GDK_Right: */
	  /* dirtree_freeze (GTK_CTREE (dv->dirtree)); */
	  /* gtk_ctree_expand (GTK_CTREE (dv->dirtree), node); */
	  /* dirtree_thaw (GTK_CTREE (dv->dirtree)); */

	  /* break; */
      /* case GDK_Left: */
	  /* dirtree_freeze (GTK_CTREE (dv->dirtree)); */
	  /* gtk_ctree_collapse (GTK_CTREE (dv->dirtree), node); */
	  /* dirtree_thaw (GTK_CTREE (dv->dirtree)); */

	  /* break; */
    /* } */

    /* dirview_update_toolbar (dv); */

    /* return FALSE; */
/* } */

/* static void */
/* cb_dirview_select_file (GtkWidget * widget, gchar * path, DirView * dv) */
/* { */
    /* if (dv->lock_select) */
	/* return; */

    /* dirview_update_toolbar (dv); */
    /* browser_select_dir (path); */
/* } */

static void
cb_dirview_refresh()
{
	GtkTreeModel *model = gtk_tree_view_get_model(
			TREEVIEW);
	GtkTreeIter iter;
	GtkTreeSelection *treeselection;
	gchar *old_path = dirview_get_current_dir(model);


	treeselection = gtk_tree_view_get_selection(
			TREEVIEW);
	model = gtk_tree_view_get_model(TREEVIEW);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		dirtree_refresh_node(model, &iter, DIRVIEW_DIRTREE);
		dirview_goto_dir(TREEVIEW, old_path);
	}
	g_free(old_path);
}

static void
cb_dirview_show_dotfile()
{
	GtkTreeModel *model = gtk_tree_view_get_model(
			TREEVIEW);
	GtkTreeIter iter;
	GtkTreeSelection *treeselection;
	gchar *old_path = dirview_get_current_dir(model);

	if (DIRVIEW_DIRTREE->show_dotfile == TRUE) {
		conf.show_dotfile = FALSE;
		DIRVIEW_DIRTREE->show_dotfile = FALSE;
	} else {
		conf.show_dotfile = TRUE;
		DIRVIEW_DIRTREE->show_dotfile = TRUE;
	}


	treeselection = gtk_tree_view_get_selection(
			TREEVIEW);
	model = gtk_tree_view_get_model(TREEVIEW);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		dirtree_refresh_node(model, &iter, DIRVIEW_DIRTREE);
		dirview_goto_dir(TREEVIEW, old_path);
	}
	g_free(old_path);
}

static void
cb_dirview_go_home()
{
	dirview_goto_dir(TREEVIEW, g_get_home_dir());
}

 /*
  * make directory
  */

/* static gint cb_dirview_rename_node (ClistEditData * ced, const gchar * old, */
					/* const gchar * new, gpointer data); */

/* static void */
/* cb_dirview_mkdir (void) */
/* { */
    /* GtkCTreeNode *node; */
    /* DirTreeNode *dirnode; */
    /* gboolean success; */
    /* gint    row; */
    /* gchar  *path, *basename; */
    /* gchar  *new_path; */

    /* node = GTK_CLIST (dirview->dirtree)->selection->data; */

    /* if (!node) */
	/* return; */

    /* dirnode = */
	/* gtk_ctree_node_get_row_data (GTK_CTREE (dirview->dirtree), node); */

    /* if (!dirnode) */
	/* return; */

    /* if (!iswritable (dirnode->path)) */
    /* { */
	/* gchar  *message = NULL; */

	/* basename = g_path_get_basename(dirnode->path); */
	/* message = */
		/* g_strdup_printf ("%s \"%s\".", _("Permission denied"), */
				 /* basename); */
	/* dialog_message (_("Error"), message, browser->window); */
	/* g_free(basename); */
	/* g_free (message); */

	/* return; */
    /* } */

    /* path = g_strconcat (dirnode->path, "/", _("new folder"), NULL); */

    /* new_path = unique_filename (path, NULL, NULL, FALSE); */

    /* success = makedir (new_path); */

    /* dirtree_freeze (DIRTREE (dirview->dirtree)); */

    /* dirtree_refresh_node (DIRTREE (dirview->dirtree), node); */

    /* dirtree_refresh_tree (DIRTREE (dirview->dirtree), node, FALSE); */

	/* basename = g_path_get_basename(new_path); */
    /* node = */
	/* dirtree_find_file (DIRTREE (dirview->dirtree), node, */
			   /* basename); */
	/* g_free(basename); */
    /* dirtree_refresh_tree (DIRTREE (dirview->dirtree), node, TRUE); */

    /* dirview_scroll_center (); */

    /* dirtree_thaw (DIRTREE (dirview->dirtree)); */

    /* row = */
	/* g_list_position (GTK_CLIST (dirview->dirtree)->row_list, */
			 /* (GList *) node); */

    /* if (row < 0) */
	/* return; */

    /* dirview_make_visible (dirview, node); */

    /* gtk_ctree_node_moveto (GTK_CTREE (dirview->dirtree), node, 0, 0.0, 0.0); */

    /* clist_edit_by_row (GTK_CLIST (dirview->dirtree), row, 0, */
			   /* cb_dirview_rename_node, dirview); */

    /* g_free (path); */
    /* g_free (new_path); */
/* } */

 /*
  * rename directory
  */

/* static int */
/* cb_dirview_refresh_timer_proc (gpointer data) */
/* { */
    /* GtkCTreeNode *parent; */
    /* GtkCTreeNode *node = data; */

    /* parent = (GTK_CTREE_ROW (node))->parent; */

    /* dirtree_freeze (DIRTREE (dirview->dirtree)); */

    /* if (parent) */
	/* dirtree_refresh_tree (DIRTREE (dirview->dirtree), parent, FALSE); */

    /* if (refresh_dir) */
    /* { */
	/* node = */
		/* dirtree_find_file (DIRTREE (dirview->dirtree), parent, */
				   /* refresh_dir); */
	/* dirtree_refresh_tree (DIRTREE (dirview->dirtree), node, TRUE); */

	/* g_free (refresh_dir); */
	/* refresh_dir = NULL; */
    /* } */

    /* dirview_scroll_center (); */

    /* dirtree_thaw (DIRTREE (dirview->dirtree)); */

    /* return FALSE; */
/* } */

/* static  gint */
/* cb_dirview_rename_node (ClistEditData * ced, const gchar * old, */
			/* const gchar * new, gpointer data) */
/* { */
    /* DirView *dv = data; */
    /* DirTreeNode *dirnode; */
    /* GtkCTreeNode *node; */
    /* gchar  *src_path; */
    /* gchar  *dest_path; */

    /* node = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), (guint) ced->row); */

    /* if (!node) */
	/* return FALSE; */

    /* dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node); */

    /* if (!dirnode) */
	/* return FALSE; */

    /* src_path = g_strdup (dirnode->path); */

    /* if (src_path[strlen (src_path) - 1] == '/') */
	/* src_path[strlen (src_path) - 1] = '\0'; */

    /* dest_path = g_strconcat (g_path_get_dirname(src_path), "/", new, NULL); */

    /* if (rename (src_path, dest_path) < 0) */
    /* { */
	/* gchar  *message = NULL; */
	/* gchar  *basename; */

	/* basename = g_path_get_basename(src_path); */
	/* message = */
		/* g_strdup_printf ("%s \"%s\".\n%s", _("Faild to rename directory"), */
				 /* basename, g_strerror (errno)); */
	/* dialog_message (_("Error"), message, browser->window); */
	/* g_free(basename); */
	/* g_free (message); */
    /* } */
    /* else */
    /* { */
	/* refresh_dir = g_strdup (new); */

	/* timer_refresh = */
		/* g_timeout_add(100, cb_dirview_refresh_timer_proc, node); */
    /* } */

    /* g_free (src_path); */
    /* g_free (dest_path); */

    /* return FALSE; */
/* } */

/* static void */
/* cb_dirview_rename_dir (void) */
/* { */
    /* GtkCTreeNode *node; */
    /* DirTreeNode *dirnode; */
    /* gboolean exist; */
    /* struct stat st; */
    /* gint    row; */

    /* node = GTK_CLIST (dirview->dirtree)->selection->data; */

    /* if (!node) */
	/* return; */

    /* dirnode = */
	/* gtk_ctree_node_get_row_data (GTK_CTREE (dirview->dirtree), node); */

    /* if (!dirnode) */
	/* return; */

    /*
     * check for direcotry exist
     */

    /* exist = !lstat (dirnode->path, &st); */

    /* if (!exist) */
    /* { */
	/* gchar  *message = NULL; */
	/* gchar  *basename; */

	/* basename = g_path_get_basename(dirnode->path); */
	/* message = */
		/* g_strdup_printf ("%s \"%s\".", _("Directory not exist"), */
				 /* basename); */
	/* dialog_message (_("Error"), message, browser->window); */
	/* g_free(basename); */
	/* g_free (message); */

	/* return; */
    /* } */

    /* row = */
	/* g_list_position (GTK_CLIST (dirview->dirtree)->row_list, */
			 /* (GList *) node); */

    /* if (row < 0) */
	/* return; */

    /* dirview_make_visible (dirview, node); */

    /* gtk_ctree_node_moveto (GTK_CTREE (dirview->dirtree), node, 0, 0.0, 0.0); */

    /* clist_edit_by_row (GTK_CLIST (dirview->dirtree), row, 0, */
			   /* cb_dirview_rename_node, dirview); */

    /* return; */
/* } */

 /*
  * delete directory
  */

/* static void */
/* cb_dirview_delete_dir (void) */
/* { */
    /* GtkCTreeNode *node, *parent; */
    /* DirTreeNode *dirnode; */
    /* gboolean exist; */
    /* struct stat st; */
    /* ConfirmType action; */
    /* gchar  *path = NULL; */
    /* gchar  *message = NULL; */
	/* gchar  *basename; */


    /* node = GTK_CLIST (dirview->dirtree)->selection->data; */

    /* if (!node) */
	/* return; */

    /* dirnode = */
	/* gtk_ctree_node_get_row_data (GTK_CTREE (dirview->dirtree), node); */

    /* if (!dirnode) */
	/* return; */

    /*
     * check direcotry exist or not
     */
    /* exist = !lstat (dirnode->path, &st); */

    /* if (!exist) */
    /* { */

	/* basename = g_path_get_basename(dirnode->path); */
	/* message = */
		/* g_strdup_printf ("%s \"%s\".", _("Directory not exist"), */
				 /* basename); */
	/* dialog_message (_("Error"), message, browser->window); */
	/* g_free(basename); */
	/* g_free (message); */

	/* return; */
    /* } */

    /*
     * confirm
     */
	/* basename = g_path_get_basename(dirnode->path); */
    /* message = */
	/* g_strdup_printf ("%s \"%s\" ?", _("Delete directory"), */
			 /* basename); */
    /* action = */
	/* dialog_confirm (_("Confirm Deleting Directory"), message, */
			/* browser->window); */
	/* g_free(basename); */
    /* g_free (message); */

    /* if (action != CONFIRM_YES) */
	/* return; */

    /* path = g_strdup (dirnode->path); */

    /* if (path[strlen (path) - 1] == '/') */
	/* path[strlen (path) - 1] = '\0'; */

    /* if (rmdir (path) < 0) */
    /* { */
	/* basename = g_path_get_basename(path); */
	/* message = */
		/* g_strdup_printf ("%s \"%s\".\n%s", _("Faild to delete directory"), */
				 /* basename, g_strerror (errno)); */
	/* dialog_message (_("Error"), message, browser->window); */
	/* g_free(basename); */
	/* g_free (message); */

	/* g_free (path); */

	/* return; */
    /* } */

    /* g_free (path); */

    /*
     * refresh dir tree
     */
    /* parent = (GTK_CTREE_ROW (node))->parent; */

    /* if (!parent) */
	/* return; */

    /* dirtree_freeze (DIRTREE (dirview->dirtree)); */
    /* dirtree_refresh_node (DIRTREE (dirview->dirtree), parent); */
    /* dirtree_refresh_tree (DIRTREE (dirview->dirtree), parent, TRUE); */
    /* dirview_make_visible (dirview, parent); */
    /* dirtree_thaw (DIRTREE (dirview->dirtree)); */
/* } */

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */


/* static void */
/* dirview_make_visible (DirView * dv, GtkCTreeNode * node) */
/* { */
    /* GtkCTreeNode *parent; */

    /* parent = GTK_CTREE_ROW (node)->parent; */

    /* gtk_clist_freeze (GTK_CLIST (dv->dirtree)); */

    /* while (parent) */
    /* { */
	/* if (!GTK_CTREE_ROW (parent)->expanded) */
	/* { */
		/* gtk_ctree_expand (GTK_CTREE (dv->dirtree), parent); */
	/* } */
	/* parent = GTK_CTREE_ROW (parent)->parent; */
    /* } */

    /*
     * the realized test is a hack, otherwise the scrollbar is incorrect at start up
     */
    /* if (GTK_WIDGET_REALIZED (dv->dirtree) */
	/* && gtk_ctree_node_is_visible (GTK_CTREE (dv->dirtree), */
					  /* node) != GTK_VISIBILITY_FULL) */
    /* { */
	/* gtk_ctree_node_moveto (GTK_CTREE (dv->dirtree), node, 0, 0, 0.0); */
    /* } */

    /* gtk_clist_thaw (GTK_CLIST (dv->dirtree)); */
/* } */

/* static void */
/* dirview_update_toolbar(DirView *dv) */
/* { */
    /* GtkCTreeNode *node; */
    /* GList  *sel_list; */
    /* gboolean is_leaf, expanded; */
    /* gint    current, len; */

    /* sel_list = GTK_CLIST (dv->dirtree)->selection; */

    /* if (!sel_list) */
	/* return; */

    /* node = sel_list->data; */

    /* len = GTK_CLIST (GTK_CTREE (dv->dirtree))->rows; */

    /* current = len - g_list_length ((GList *) node); */

	/* if (current == 0) { */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
						/* dv->toolbar_up_btn), FALSE); */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_down_btn), TRUE); */
	/* } else if (current == (len - 1)) { */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_up_btn), TRUE); */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_down_btn), FALSE); */
	/* } else { */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_up_btn), TRUE); */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_down_btn), TRUE); */
	/* } */

    /* gtk_ctree_get_node_info (GTK_CTREE (dv->dirtree), node, NULL, NULL, */
				 /* NULL, NULL, NULL, NULL, &is_leaf, &expanded); */

	/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
				/* dv->toolbar_expand_btn), !is_leaf); */
	/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
				/* dv->toolbar_collapse_btn), !is_leaf); */

    /* if (is_leaf) */
		/* return; */

	/* if (expanded) { */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_expand_btn), FALSE); */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_collapse_btn), TRUE); */
	/* } else { */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_expand_btn), TRUE); */
		/* gtk_widget_set_sensitive(gtk_tool_button_get_icon_widget( */
					/* dv->toolbar_collapse_btn), FALSE); */
	/* } */
/* } */


/**
 * Creates the right click menu.
 *
 * @param rightclick_menu the GtkMenu [out]
 */
static void
dirview_create_rightclick_menu(GtkWidget *rightclick_menu)
{
	/* GtkMenuItem */
	GtkWidget *create_dir = gtk_menu_item_new_with_label("Create dir"),
			  *rename_dir = gtk_menu_item_new_with_label("Rename dir"),
			  *delete_dir = gtk_menu_item_new_with_label("Delete dir");

	g_signal_connect(GTK_MENU_ITEM(create_dir),
			"activate",
			G_CALLBACK(dirview_mkdir),
			NULL);
	g_signal_connect(GTK_MENU_ITEM(rename_dir),
			"activate",
			G_CALLBACK(dirview_rename_dir),
			NULL);

	g_signal_connect(GTK_MENU_ITEM(delete_dir),
			"activate",
			G_CALLBACK(dirview_remove_dir),
			NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(rightclick_menu), create_dir);
	gtk_menu_shell_append(GTK_MENU_SHELL(rightclick_menu), rename_dir);
	gtk_menu_shell_append(GTK_MENU_SHELL(rightclick_menu), delete_dir);

	gtk_widget_show(create_dir);
	gtk_widget_show(rename_dir);
	gtk_widget_show(delete_dir);
}

static void
cb_dirview_entry_enter (GtkEntry *entry,
		gpointer path_dialog)
{
	gtk_dialog_response(GTK_DIALOG(path_dialog),
			GTK_RESPONSE_OK);
}

/* TODO: check (no subfolders) and (not readable) cases */
static void
dirview_mkdir(GtkMenuItem *menuitem,
		gpointer data)
{
	GtkTreeIter iter;
	GtkTreePath *treepath;
	GtkTreeModel *model = gtk_tree_view_get_model(TREEVIEW);
	GtkWidget *path_dialog, /* GtkDialog */
			  *entry_field = gtk_entry_new(); /* GtkEntry */

	gint ret_val;
	gchar *path = dirview_get_current_dir(model),
		  *new_path;
	gchar *new_dir;

	/* open entry dialog for the dir name */
	path_dialog = gtk_dialog_new_with_buttons("Create dir",
			GTK_WINDOW(browser->window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(path_dialog),
			GTK_RESPONSE_OK);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(path_dialog)->vbox),
			entry_field, TRUE, TRUE, 0);
	g_signal_connect(GTK_ENTRY(entry_field),
			"activate",
			G_CALLBACK(cb_dirview_entry_enter),
			path_dialog);
	gtk_widget_show(entry_field);
	ret_val = gtk_dialog_run(GTK_DIALOG(path_dialog));

	/* check response and get the dir name */
	switch (ret_val) {
		case GTK_RESPONSE_OK:
			new_dir = g_malloc0(gtk_entry_get_text_length(
						GTK_ENTRY(entry_field)) + 1);
			strcpy(new_dir, gtk_entry_get_text(
						GTK_ENTRY(entry_field)));
			gtk_widget_destroy(path_dialog);
			break;
		default:
			gtk_widget_destroy(path_dialog);
			g_free(path);
			return;
	}

	new_path = g_build_path(G_DIR_SEPARATOR_S, path, new_dir, NULL);
	g_free(new_dir);

	/* do the actual mkdir in dirtree */
	ret_val = dirtree_mkdir(DIRVIEW_DIRTREE, new_path);

	/* check for errors */
	if (!ret_val) { /* success, refresh the node */
		treepath = get_treepath(model, path);
		if (gtk_tree_model_get_iter(model, &iter, treepath)) {
			dirtree_refresh_node(model, &iter, DIRVIEW_DIRTREE);
			gtk_tree_view_expand_to_path(TREEVIEW, treepath);
		} else {
			/* TODO: debug message, should not happen :o */
		}
		gtk_tree_path_free(treepath);
	} else {
		GtkWidget *error_dialog; /* GtkDialog */
		gchar *error_msg = g_malloc0(100);

		switch (ret_val) {
			case 1: /* makedir failed */
				sprintf(error_msg, "Error creating dir \"%s\",\n"
						"makedir failed.",
						new_path);
				break;
			case 2:
				sprintf(error_msg, "Error creating dir \"%s\",\n"
						"no write permission.",
						new_path);
				break;
			case 3:
				sprintf(error_msg, "Error creating already existing dir"
						"\"%s\".",
						new_path);
				break;
			case 4:
				sprintf(error_msg, "Error creating dir \"%s\",\n"
						"not a valid parent dir.",
						new_path);
				break;
			default:
				sprintf(error_msg, "Unknown error while creating dir"
						"\"%s\".",
						new_path);
		}

		/* show error dialog */
		error_dialog = gtk_message_dialog_new(
				GTK_WINDOW(browser->window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s\n",
				error_msg);

		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
		g_free(error_msg);
	}

	g_free(new_path);
	g_free(path);
}

static void
dirview_rename_dir(GtkMenuItem *menuitem,
		gpointer data)
{
	GtkTreeIter iter;
	GtkTreePath *treepath;
	GtkTreeModel *model = gtk_tree_view_get_model(TREEVIEW);
	GtkWidget *path_dialog, /* GtkDialog */
			  *entry_field = gtk_entry_new(); /* GtkEntry */

	gint ret_val;
	gchar *path = dirview_get_current_dir(model),
		  *base_path,
		  *new_path;
	gchar *new_dir;

	gint len;

	/* open entry dialog for the dir name */
	path_dialog = gtk_dialog_new_with_buttons("Create dir",
			GTK_WINDOW(browser->window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(path_dialog),
			GTK_RESPONSE_OK);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(path_dialog)->vbox),
			entry_field, TRUE, TRUE, 0);
	g_signal_connect(GTK_ENTRY(entry_field),
			"activate",
			G_CALLBACK(cb_dirview_entry_enter),
			path_dialog);
	gtk_widget_show(entry_field);
	ret_val = gtk_dialog_run(GTK_DIALOG(path_dialog));

	/* check response and get the dir name */
	switch (ret_val) {
		case GTK_RESPONSE_OK:
			new_dir = g_malloc0(gtk_entry_get_text_length(
						GTK_ENTRY(entry_field)) + 1);
			strcpy(new_dir, gtk_entry_get_text(
						GTK_ENTRY(entry_field)));
			gtk_widget_destroy(path_dialog);
			break;
		default:
			gtk_widget_destroy(path_dialog);
			g_free(path);
			return;
	}

	base_path = g_path_get_dirname(path);
	new_path = g_build_path(G_DIR_SEPARATOR_S, base_path, new_dir, NULL);
	g_free(new_dir);

	/* do the actual move */
	ret_val = dirtree_rename_dir(DIRVIEW_DIRTREE, path, new_path);

	/* check for errors */
	if (!ret_val) { /* success, refresh the node */
		treepath = get_treepath(model, base_path);
		if (gtk_tree_model_get_iter(model, &iter, treepath)) {
			dirtree_refresh_node(model, &iter, DIRVIEW_DIRTREE);
			dirview_goto_dir(TREEVIEW, base_path);
		} else {
			/* TODO: debug message, should not happen :o */
		}
		gtk_tree_path_free(treepath);
	} else {
		GtkWidget *error_dialog; /* GtkDialog */
		gchar *error_msg = g_malloc0(100);

		switch (ret_val) {
			case 1: /* makedir failed */
				sprintf(error_msg, "Error moving dir \"%s\" to \"%s\",\n"
						"mv failed.",
						path, new_path);
				break;
			case 2:
				sprintf(error_msg, "Error moving dir \"%s\" to \"%s\",\n"
						"no write permission.",
						path, new_path);
				break;
			case 3:
				sprintf(error_msg, "Error moving dir \"%s\",\n"
						"not a directory.",
						path);
				break;
			case 4:
				sprintf(error_msg, "Error moving dir \"%s\" to \"%s\",\n"
						"not a valid destination.",
						path, new_path);
				break;
			case 5:
				sprintf(error_msg, "Error moving dir \"%s\" to \"%s\",\n"
						"not a valid source.",
						path, new_path);
				break;
			default:
				sprintf(error_msg, "Unknown error while moving dir"
						"\"%s\" to \"%s\".",
						path, new_path);
		}

		/* show error dialog */
		error_dialog = gtk_message_dialog_new(
				GTK_WINDOW(browser->window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s\n",
				error_msg);

		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
		g_free(error_msg);
	}
	g_free(new_path);
	g_free(base_path);
	g_free(path);
}

static void
dirview_remove_dir(GtkMenuItem *menuitem,
		gpointer data)
{
	GtkTreeIter iter;
	GtkTreePath *treepath;
	GtkTreeModel *model = gtk_tree_view_get_model(TREEVIEW);
	GtkWidget *confirm_dialog, /* GtkDialog */
			  *label, /* GtkLabel */
			  *icon, /* GtkImage */
			  *hbox;

	gint ret_val;
	gchar *path = dirview_get_current_dir(model),
		  *base_path,
		  *message = g_malloc0(256);

	/* open confirmation dialog */
	confirm_dialog = gtk_dialog_new_with_buttons("Create dir",
			GTK_WINDOW(browser->window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(confirm_dialog),
			GTK_RESPONSE_CANCEL);

	hbox = gtk_hbox_new(FALSE, 2);

	/* add icon to hbox */
	icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
			GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(hbox,
			icon, TRUE, TRUE, 0);
	gtk_widget_show(icon);

	/* add label hbox */
	sprintf(message, "Do you really want to delete\n\"%s\"?\n", path);
	label = gtk_label_new(message);
	gtk_box_pack_start(hbox,
			label, TRUE, TRUE, 0);
	gtk_widget_show(label);

	/* add hbox to dialog */
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(confirm_dialog)->vbox),
			hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	ret_val = gtk_dialog_run(GTK_DIALOG(confirm_dialog));

	/* check response and get the dir name */
	switch (ret_val) {
		case GTK_RESPONSE_OK:
			gtk_widget_destroy(confirm_dialog);
			g_free(message);
			break;
		default:
			gtk_widget_destroy(confirm_dialog);
			g_free(message);
			g_free(path);
			return;
	}

	base_path = g_path_get_dirname(path);

	ret_val = dirtree_remove_dir(DIRVIEW_DIRTREE, path);

	/* check for errors */
	if (!ret_val) { /* success, refresh the node */
		treepath = get_treepath(model, base_path);
		if (gtk_tree_model_get_iter(model, &iter, treepath)) {
			dirtree_refresh_node(model, &iter, DIRVIEW_DIRTREE);
			dirview_goto_dir(TREEVIEW, base_path);
		} else {
			/* TODO: debug message, should not happen :o */
		}
		gtk_tree_path_free(treepath);
	} else {
		GtkWidget *error_dialog; /* GtkDialog */
		gchar *error_msg = g_malloc0(100);

		switch (ret_val) {
			case 1: /* rmdir failed */
				sprintf(error_msg, "Error removing dir \"%s\",\n"
						"rmdir failed. Only removing empty dirs\n"
						"is supported.",
						path);
				break;
			case 2:
				sprintf(error_msg, "Error removing dir \"%s\",\n"
						"got a relative path.",
						path);
				break;
			case 3:
				sprintf(error_msg, "Error removing dir \"%s\",\n"
						"not a directory",
						path);
				break;
			case 4:
				sprintf(error_msg, "Error removing dir \"%s\",\n"
						"not a valid path.",
						path);
				break;
			default:
				sprintf(error_msg, "Unknown error while removing dir"
						"\"%s\".",
						path);
		}

		/* show error dialog */
		error_dialog = gtk_message_dialog_new(
				GTK_WINDOW(browser->window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s\n",
				error_msg);

		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
		g_free(error_msg);
	}

	g_free(path);
	g_free(base_path);
}


/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
dirview_create (const gchar *start_path, GtkWidget *parent_win)
{
	GtkActionGroup *action_group;
    GtkWidget *toolbar;
	GtkTreeSelection *tree_selection;
	guint ui_id;

	const gchar *homepath = g_get_home_dir();
    dirview = g_new0(DirView, 1);

    /*
     * main vbox
     */
    dirview->container = gtk_vbox_new(FALSE, 0);
    g_object_ref (dirview->container);
    g_object_set_data_full (G_OBJECT (parent_win), "dirview_container",
			      dirview->container,
			      (GDestroyNotify) g_object_unref);
    gtk_widget_show (dirview->container);

	/*
	 * toolbar
	 */
	dirview->toolbar = gtk_ui_manager_new();

	action_group = gtk_action_group_new("DirviewAction");
	gtk_action_group_add_actions(action_group, dirview_toolbar_entries,
			n_dirview_toolbar_entries, NULL);
	gtk_ui_manager_insert_action_group(dirview->toolbar, action_group, 0);

	GError *error;
	error = NULL;
	ui_id = gtk_ui_manager_add_ui_from_string(dirview->toolbar,
			dirview_toolbar_xml,
			-1,
			&error);
	if (error) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
	}

	/* use gtk_ui_manager_remove_ui() later */
    toolbar = gtk_ui_manager_get_widget(dirview->toolbar, "/DirviewToolbar");

	gtk_box_pack_start(GTK_BOX(dirview->container), toolbar, FALSE, FALSE, 0);

	gtk_window_add_accel_group(GTK_WINDOW(parent_win),
			gtk_ui_manager_get_accel_group(dirview->toolbar));


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
    dirview->dirtree = dirtree_new(parent_win, start_path, conf.scan_dir,
			conf.check_hlinks, conf.show_dotfile, conf.dirtree_line_style,
			conf.dirtree_expander_style);

    gtk_container_add (GTK_CONTAINER (dirview->scroll_win),
			TREEVIEW_WIDGET);

	/* TODO: use macro/global or different func? */
	gtk_widget_set_size_request(GTK_WIDGET (dirview->scroll_win),
			  50, 200);

    gtk_widget_show (TREEVIEW_WIDGET);

    /*
     * callback signals
     */
	g_signal_connect(TREEVIEW,
			"row-activated",
			G_CALLBACK(cb_dirview_row_activated),
			NULL);
	g_signal_connect(TREEVIEW,
			"row-expanded",
			G_CALLBACK(cb_dirview_row_expanded),
			NULL);
	g_signal_connect(TREEVIEW,
			"test-collapse-row",
			G_CALLBACK(cb_dirview_row_test_collapsed),
			NULL);
	g_signal_connect(TREEVIEW,
			"button-press-event",
			G_CALLBACK(cb_dirview_button_press_event),
			NULL);

	/* set initial directory */
	if (start_path && isdir(start_path)) {
		/* tree_selection not yet registered, chdir manually */
		chdir(start_path);
		dirview_goto_dir(TREEVIEW, start_path);
	} else if (isdir(homepath)) {
		chdir(homepath);
		dirview_goto_dir(TREEVIEW, homepath);
	}

	/* FIXME: moving it before the above block causes segfault */
	tree_selection = gtk_tree_view_get_selection(TREEVIEW);
	g_signal_connect(tree_selection,
			"changed",
			G_CALLBACK(cb_dirview_selection_changed),
			NULL);

	gtk_widget_show (dirview->scroll_win);
    /* dirview_update_toolbar(dirview); */
}

void
dirview_goto_dir(GtkTreeView *treeview, const gchar *dest_dir)
{
	GtkTreePath *dest_path;
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);

	gchar *dest_string, *ret_string;
	guint i = 0;

	dest_string = g_strdup(dest_dir);

	dest_path = get_treepath(
			model, dest_string);

	ret_string = get_full_dirname(model, dest_path);

	/* strip trailing '/' so we can compare the strings later */
	while (dest_string[strlen(dest_string) - ++i] == '/')
		dest_string[strlen(dest_string) - i] = '\0';
	i = 0;
	while (ret_string[strlen(ret_string) - ++i] == '/')
		ret_string[strlen(ret_string) - i] = '\0';

	if (!strcmp(ret_string, dest_string)) {
		gtk_tree_view_expand_to_path(treeview, dest_path);
		/* select and activate, so we don't lose focus */
		gtk_tree_view_set_cursor(treeview, dest_path, NULL, FALSE);
	} else if (isdir(dest_string)){ /* need to expand subdirs */
		gtk_tree_view_expand_to_path(treeview, dest_path);
		dirview_goto_dir(treeview, dest_string);
	}

	g_free(dest_string);
	g_free(ret_string);
	gtk_tree_path_free(dest_path);
}

gchar *
dirview_get_current_dir(GtkTreeModel *model)
{
	GtkTreeIter iter;
	GtkTreeSelection *treeselection;
	gchar *full_dirname;

	treeselection = gtk_tree_view_get_selection(
			TREEVIEW);

	if (gtk_tree_selection_get_selected(treeselection,
			&model,
			&iter)) {
		full_dirname = get_full_dirname_iter(model, &iter);
		return full_dirname;
	}

	return NULL;
}

void
dirview_scroll_center (void)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeSelection *treeselection;
	gchar *dirname;

	treeselection = gtk_tree_view_get_selection(
			TREEVIEW);
	model = gtk_tree_view_get_model(TREEVIEW);
	gtk_tree_selection_get_selected(treeselection,
			&model,
			&iter);

	/* FIXME: this is an ugly way to convert iter to path */
	dirname = get_full_dirname_iter(model, &iter);
	path = get_treepath(model, dirname);
	g_free(dirname);

	gtk_tree_view_scroll_to_cell(TREEVIEW,
			path, NULL, FALSE, 0.5, 0.5);
	gtk_tree_path_free(path);
}

void
dirview_destroy(void)
{
    gtk_widget_destroy(TREEVIEW_WIDGET);

    g_free(dirview);
    dirview = NULL;
}
