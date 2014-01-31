/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include "gtk2-compat.h"
#include "dirtree.h"
#include "file_utils.h"

#define CTREE_SPACING 5
#define NODE_TEXT(tree, node) (GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.cell[(tree)->tree_column])->text)

static void dirtree_class_init (DirTreeClass * kclass);
static void dirtree_init (DirTree * dt);
static void dirtree_destroy_tree (gpointer data);
static void dirtree_select_row (GtkCTree * tree, GtkCTreeNode * node,
				gint column);
static void dirtree_expand (GtkCTree * tree, GtkCTreeNode * node);
static void dirtree_collapse (GtkCTree * tree, GtkCTreeNode * node);

static void tree_expand_node (DirTree * dt, GtkCTreeNode * node,
			      gboolean expand);
static void tree_collapse (GtkCTree * tree, GtkCTreeNode * node);

static gboolean tree_is_subdirs (gchar * name, gboolean show_dotfile);
static gboolean tree_is_dotfile (gchar * name, gboolean show_dotfile);
static void dirtree_set_cursor (GtkWidget * widget,
				GdkCursorType cursor_type);

enum
{
    SELECT_FILE,
    LAST_SIGNAL
};

static GtkCTreeClass *parent_class = NULL;
static guint dirtree_signals[LAST_SIGNAL] = { 0 };

#include "pixmaps/folder.xpm"
#include "pixmaps/folder_open.xpm"
#include "pixmaps/folder_link.xpm"
#include "pixmaps/folder_link_open.xpm"
#include "pixmaps/folder_lock.xpm"
#include "pixmaps/folder_go.xpm"
#include "pixmaps/folder_up.xpm"

static GdkPixmap *folder_pixmap, *ofolder_pixmap,
    *lfolder_pixmap, *lofolder_pixmap, *lckfolder_pixmap,
    *gofolder_pixmap, *upfolder_pixmap;
static GdkBitmap *folder_mask, *ofolder_mask,
    *lfolder_mask, *lofolder_mask, *lckfolder_mask,
    *gofolder_mask, *upfolder_mask;

static gboolean lock = FALSE;

GtkType
dirtree_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
	GtkTypeInfo dirtree_info = {
	    "DirTree",
	    sizeof (DirTree),
	    sizeof (DirTreeClass),
	    (GtkClassInitFunc) dirtree_class_init,
	    (GtkObjectInitFunc) dirtree_init,
	    NULL,
	    NULL,
	    (GtkClassInitFunc) NULL,
	};

	type = gtk_type_unique (GTK_TYPE_CTREE, &dirtree_info);
    }

    return type;
}

static void
dirtree_class_init (DirTreeClass * klass)
{
    GtkObjectClass *object_class;
    GtkCTreeClass *ctree_class;

    object_class = (GtkObjectClass *) klass;
    ctree_class = (GtkCTreeClass *) klass;

    parent_class = gtk_type_class (gtk_ctree_get_type ());

    dirtree_signals[SELECT_FILE] =
	gtk_signal_new ("select_file",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (DirTreeClass, select_file),
			gtk_marshal_NONE__POINTER,
			GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

    gtk_object_class_add_signals (object_class, dirtree_signals, LAST_SIGNAL);

    ctree_class->tree_expand = dirtree_expand;
    ctree_class->tree_collapse = dirtree_collapse;
    ctree_class->tree_select_row = dirtree_select_row;

    klass->select_file = NULL;
}

static void
dirtree_init (DirTree * dt)
{
}

static void
dirtree_destroy_tree (gpointer data)
{
    DirTreeNode *node;

    node = data;
    g_free (node->path);
    g_free (node);
}

GtkWidget *
dirtree_new (GtkWidget * win, const gchar * start_path, gboolean check_dir,
	     gboolean check_hlinks, gboolean show_dotfile, gint line_style,
	     gint expander_style)
{
    GtkWidget *widget;
    DirTree *dt;
    DirTreeNode *dt_node;
    GtkCTreeNode *node = NULL;
    char   *root = "/", *rp, *tmp;

    widget = gtk_type_new (dirtree_get_type ());

    dt = DIRTREE (widget);
    dt->collapse = FALSE;
    dt->check_dir = check_dir;
    dt->check_hlinks = check_hlinks;
    dt->show_dotfile = show_dotfile;
    dt->expander_style = expander_style;
    dt->line_style = line_style;
    dt->check_events = TRUE;

    lock = FALSE;

    gtk_clist_set_column_auto_resize (GTK_CLIST (dt), 0, TRUE);
    gtk_clist_set_selection_mode (GTK_CLIST (dt), GTK_SELECTION_BROWSE);
    gtk_clist_set_row_height (GTK_CLIST (dt), 18);
    gtk_ctree_set_expander_style (GTK_CTREE (dt), expander_style);
    gtk_ctree_set_line_style (GTK_CTREE (dt), line_style);

    folder_pixmap =
	gdk_pixmap_create_from_xpm_d (win->window, &folder_mask, NULL,
				      folder_xpm);
    ofolder_pixmap =
	gdk_pixmap_create_from_xpm_d (win->window, &ofolder_mask, NULL,
				      folder_open_xpm);
    lfolder_pixmap =
	gdk_pixmap_create_from_xpm_d (win->window, &lfolder_mask, NULL,
				      folder_link_xpm);
    lofolder_pixmap =
	gdk_pixmap_create_from_xpm_d (win->window, &lofolder_mask, NULL,
				      folder_link_open_xpm);
    lckfolder_pixmap =
	gdk_pixmap_create_from_xpm_d (win->window, &lckfolder_mask, NULL,
				      folder_lock_xpm);
    gofolder_pixmap =
	gdk_pixmap_create_from_xpm_d (win->window, &gofolder_mask, NULL,
				      folder_go_xpm);
    upfolder_pixmap =
	gdk_pixmap_create_from_xpm_d (win->window, &upfolder_mask, NULL,
				      folder_up_xpm);

    node = gtk_ctree_insert_node (GTK_CTREE (dt),
				  NULL, NULL,
				  &root, CTREE_SPACING,
				  folder_pixmap, folder_mask, ofolder_pixmap,
				  ofolder_mask, FALSE, TRUE);

    dt_node = g_malloc0 (sizeof (DirTreeNode));
    dt_node->path = g_strdup ("/");
    dt_node->scanned = TRUE;
    gtk_ctree_node_set_row_data_full (GTK_CTREE (dt), node, dt_node,
				      dirtree_destroy_tree);

    tree_expand_node (dt, node, FALSE);

    rp = g_strdup (start_path);
    tmp = strtok (rp, "/");

    while (tmp)
    {
	node = dirtree_find_file (dt, node, tmp);

	if (!node)
	    break;

	gtk_ctree_expand (GTK_CTREE (dt), node);
	gtk_ctree_select (GTK_CTREE (dt), node);
	GTK_CLIST (dt)->focus_row =
	    GTK_CLIST (dt)->rows - g_list_length ((GList *) node);

	tmp = strtok (NULL, "/");
    }

    g_free (rp);

    return widget;
}

void
dirtree_set_mode (DirTree * dt, gboolean check_dir, gboolean check_hlinks,
		  gboolean show_dotfile, gint line_style, gint expander_style)
{
    dt->check_dir = check_dir;
    dt->check_hlinks = check_hlinks;
    dt->show_dotfile = show_dotfile;

    if (dt->expander_style != expander_style)
    {
	gtk_ctree_set_expander_style (GTK_CTREE (dt), expander_style);
	dt->expander_style = expander_style;
    }

    if (dt->line_style != line_style)
    {
	gtk_ctree_set_line_style (GTK_CTREE (dt), line_style);
	dt->line_style = line_style;
    }
}

void
dirtree_refresh_tree (DirTree * dt, GtkCTreeNode * parent, gboolean selected)
{
    lock = TRUE;

    if (selected)
    {
	gtk_ctree_select (GTK_CTREE (dt), parent);
	GTK_CLIST (dt)->focus_row =
	    GTK_CLIST (dt)->rows - g_list_length ((GList *) parent);
    }

    tree_expand_node (dt, parent, TRUE);

    lock = FALSE;

    if (selected)
    {
	gtk_ctree_select (GTK_CTREE (dt), parent);
	GTK_CLIST (dt)->focus_row =
	    GTK_CLIST (dt)->rows - g_list_length ((GList *) parent);
    }
}

void
dirtree_refresh_node (DirTree * dt, GtkCTreeNode * node)
{
    DirTreeNode *dirnode = NULL;
    gint    has_subdirs;
    gboolean expanded;

    dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dt), node);

    has_subdirs = tree_is_subdirs (dirnode->path, dt->show_dotfile);

    if (has_subdirs && GTK_CTREE_ROW (node)->expanded == 1)
	expanded = TRUE;
    else
	expanded = FALSE;

    if (has_subdirs)
	gtk_ctree_set_node_info (GTK_CTREE (dt), node,
				 g_basename (dirnode->path), CTREE_SPACING,
				 folder_pixmap, folder_mask, ofolder_pixmap,
				 ofolder_mask, FALSE, expanded);
    else
	gtk_ctree_set_node_info (GTK_CTREE (dt), node,
				 g_basename (dirnode->path), CTREE_SPACING,
				 folder_pixmap, folder_mask, ofolder_pixmap,
				 ofolder_mask, TRUE, expanded);
}

GtkCTreeNode *
dirtree_find_file (DirTree * dt, GtkCTreeNode * parent, char *file_name)
{
    GtkCTreeNode *child;

    child = GTK_CTREE_ROW (parent)->children;

    while (child)
    {
	if (!strcmp (NODE_TEXT (GTK_CTREE (dt), child), file_name))
	    return child;

	child = GTK_CTREE_ROW (child)->sibling;
    }

    return NULL;
}

static void
dirtree_select_row (GtkCTree * tree, GtkCTreeNode * node, gint column)
{
    DirTree *dt;
    DirTreeNode *dt_node = NULL;

    if (parent_class->tree_select_row)
	(*parent_class->tree_select_row) (tree, node, column);

    if (lock)
	return;

    dt = DIRTREE (tree);

    dt_node = gtk_ctree_node_get_row_data (tree, node);

    if (dt_node != NULL)
	chdir (dt_node->path);

    gtk_signal_emit_by_name (GTK_OBJECT (dt), "select_file",
			     g_get_current_dir ());
}

static void
dirtree_expand (GtkCTree * tree, GtkCTreeNode * node)
{
    GtkAdjustment *h_adjustment;
    gfloat  row_align;

    DirTreeNode *dt_node;

    dt_node = gtk_ctree_node_get_row_data (tree, node);

    if (dt_node->scanned == FALSE)
    {
	tree_expand_node (DIRTREE (tree), node, FALSE);
	dt_node->scanned = TRUE;
    }

    if (parent_class->tree_expand)
	(*parent_class->tree_expand) (tree, node);

    h_adjustment = gtk_clist_get_hadjustment (GTK_CLIST (tree));

    if (h_adjustment && h_adjustment->upper != 0.0)
    {
	row_align =
	    (float) (tree->tree_indent * GTK_CTREE_ROW (node)->level) /
	    h_adjustment->upper;
	gtk_ctree_node_moveto (tree, node, tree->tree_column, 0, row_align);
    }
}

static void
dirtree_collapse (GtkCTree * tree, GtkCTreeNode * node)
{
    GtkCTreeNode *node_sel;

    DirTreeNode *parent_node;
    DirTreeNode *sel_node;
    gchar  *ret = NULL;

    if (DIRTREE (tree)->collapse == FALSE)
    {
	node_sel = GTK_CLIST (tree)->selection->data;

	sel_node = gtk_ctree_node_get_row_data (tree, node_sel);
	parent_node = gtk_ctree_node_get_row_data (tree, node);

	ret = g_strdup (strstr (sel_node->path, parent_node->path));

	if (ret != NULL)
	{
	    GTK_CLIST (tree)->focus_row =
		GTK_CLIST (tree)->rows - g_list_length ((GList *) node);
	    gtk_ctree_select (tree, node);
	}
    }

    if (parent_class->tree_collapse)
	(*parent_class->tree_collapse) (tree, node);
}

static void
tree_collapse (GtkCTree * tree, GtkCTreeNode * node)
{
    GtkCTreeNode *child;

    DIRTREE (tree)->collapse = TRUE;

    gtk_clist_freeze (GTK_CLIST (tree));

    child = GTK_CTREE_ROW (node)->children;

    while (child)
    {
	gtk_ctree_remove_node (GTK_CTREE (tree), child);
	child = GTK_CTREE_ROW (node)->children;
    }

    gtk_clist_thaw (GTK_CLIST (tree));

    DIRTREE (tree)->collapse = FALSE;
}

static void
tree_expand_node (DirTree * dt, GtkCTreeNode * node, gboolean expand)
{
    GtkCTreeNode *child;
    DirTreeNode *parent_node;
    DirTreeNode *child_node;
    DIR    *dir;
    struct stat stat_buff;
    struct dirent *entry;
    gboolean has_subdirs, has_link, has_access;
    char   *old_path;
    char   *subdir = "?";
    char   *file_name;
    GtkWidget *top = gtk_widget_get_toplevel (GTK_WIDGET (dt));
    old_path = g_get_current_dir ();

    if (!old_path)
	return;

    parent_node = gtk_ctree_node_get_row_data (GTK_CTREE (dt), node);

    if (chdir (parent_node->path))
    {
	g_free (old_path);
	return;
    }

    dir = opendir (".");

    if (!dir)
    {
	chdir (old_path);
	g_free (old_path);
	return;
    }

    dirtree_set_cursor (top, GDK_WATCH);

    gtk_clist_freeze (GTK_CLIST (dt));

    tree_collapse (GTK_CTREE (dt), node);

    child = NULL;

    while ((entry = readdir (dir)))
    {
	if (!stat (entry->d_name, &stat_buff) && S_ISDIR (stat_buff.st_mode)
	    && tree_is_dotfile (entry->d_name, dt->show_dotfile))
	{
	    child_node = g_malloc0 (sizeof (DirTreeNode));

	    child_node->path =
		g_strconcat (parent_node->path, "/", entry->d_name, NULL);

	    has_link = islink (entry->d_name);
	    has_access = access (entry->d_name, X_OK);

	    file_name = entry->d_name;

	    if (has_access)
	    {
		child = gtk_ctree_insert_node (GTK_CTREE (dt),
					       node, child,
					       &file_name,
					       CTREE_SPACING,
					       lckfolder_pixmap,
					       lckfolder_mask, NULL,
					       NULL, 1, 0);
		has_subdirs = FALSE;
	    }
	    else if (!strcmp (entry->d_name, "."))
	    {
		child = gtk_ctree_insert_node (GTK_CTREE (dt),
					       node, child,
					       &file_name,
					       CTREE_SPACING,
					       gofolder_pixmap,
					       gofolder_mask, NULL,
					       NULL, 1, 0);

		has_subdirs = FALSE;
	    }
	    else if (!strcmp (entry->d_name, ".."))
	    {
		child = gtk_ctree_insert_node (GTK_CTREE (dt),
					       node, child,
					       &file_name,
					       CTREE_SPACING,
					       upfolder_pixmap,
					       upfolder_mask, NULL,
					       NULL, 1, 0);

		has_subdirs = FALSE;
	    }
	    else
	    {
		if (dt->check_dir)
		{
		    if (dt->check_hlinks)
		    {
			if (stat_buff.st_nlink == 2 && dt->show_dotfile)
			    has_subdirs = TRUE;
			else if (stat_buff.st_nlink > 2)
			    has_subdirs = TRUE;
			else if (stat_buff.st_nlink == 1)
			    has_subdirs =
				tree_is_subdirs (entry->d_name,
						 dt->show_dotfile);
			else
			    has_subdirs = FALSE;
		    }
		    else
			has_subdirs =
			    tree_is_subdirs (entry->d_name, dt->show_dotfile);

		}
		else
		    has_subdirs = TRUE;

		if (access (entry->d_name, X_OK) != 0)
		{
		    has_subdirs = FALSE;
		}

		if (has_link)
		    child = gtk_ctree_insert_node (GTK_CTREE (dt),
						   node, child,
						   &file_name,
						   CTREE_SPACING,
						   lfolder_pixmap,
						   lfolder_mask,
						   lofolder_pixmap,
						   lofolder_mask,
						   !has_subdirs, 0);
		else
		    child = gtk_ctree_insert_node (GTK_CTREE (dt),
						   node, child,
						   &file_name,
						   CTREE_SPACING,
						   folder_pixmap,
						   folder_mask,
						   ofolder_pixmap,
						   ofolder_mask,
						   !has_subdirs, 0);
	    }

	    if (child)
	    {
		gtk_ctree_node_set_row_data_full (GTK_CTREE (dt), child,
						  child_node,
						  dirtree_destroy_tree);

		if (has_subdirs)
		    gtk_ctree_insert_node (GTK_CTREE (dt),
					   child, NULL,
					   &subdir,
					   CTREE_SPACING,
					   NULL, NULL, NULL, NULL, 0, 0);
	    }
	}

	if (dt->check_events)
	    while (gtk_events_pending ())
		gtk_main_iteration ();
    }

    closedir (dir);

    chdir (old_path);
    g_free (old_path);

    gtk_ctree_sort_node (GTK_CTREE (dt), node);

    if (expand == TRUE)
	gtk_ctree_expand (GTK_CTREE (dt), node);

    gtk_clist_thaw (GTK_CLIST (dt));

    dirtree_set_cursor (top, -1);
}

static  gboolean
tree_is_subdirs (gchar * name, gboolean show_dotfile)
{
    struct dirent *entry;
    DIR    *dir;
    gchar  *tmp;

    dir = opendir (name);

    if (dir == 0)
	return FALSE;

    while ((entry = readdir (dir)))
    {
	if (!tree_is_dotfile (entry->d_name, show_dotfile));
	else
	{
	    tmp = g_strconcat (name, "/", entry->d_name, NULL);

	    if (isdir (tmp) == 1)
	    {
		closedir (dir);
		return TRUE;
	    }
	}
    }

    closedir (dir);

    return FALSE;
}

static  gboolean
tree_is_dotfile (gchar * name, gboolean show_dotfile)
{
    if (name[0] == '.' && show_dotfile == FALSE)
	return FALSE;

    return TRUE;
}

static void
dirtree_set_cursor (GtkWidget * widget, GdkCursorType cursor_type)
{
    GdkCursor *cursor = NULL;

    if (!widget || !widget->window)
	return;

    if (cursor_type > -1)
	cursor = gdk_cursor_new (cursor_type);

    gdk_window_set_cursor (widget->window, cursor);

    if (cursor)
	gdk_cursor_destroy (cursor);

    gdk_flush ();
}
