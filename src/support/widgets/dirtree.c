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
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include "gtk2-compat.h"
#include "dirtree.h"
#include "file_utils.h"


/* TODO: remove globals as much as possible */
/* add a dummy subfolder, so we always have an expand icon
 * and our algorithm in cb_dirview_row_activated works */
#define ADD_DUMMY_SUBFOLDER(string) \
{ \
		gtk_tree_store_append(GTK_TREE_STORE(model), child, iter); \
		gtk_tree_store_set(GTK_TREE_STORE(model), child,\
				COL_TEXT, string, -1); \
}

enum
{
    SELECT_FILE,
    LAST_SIGNAL
};

gchar *not_readable = "(not readable)",
	  *no_subfolders = "(no subfolders)";


/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static  gboolean
tree_is_subdirs(gchar *name, gboolean show_dotfile)
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

static gboolean
tree_is_dotfile(gchar *name, gboolean show_dotfile)
{
	if (name[0] == '.' && show_dotfile == FALSE)
		return FALSE;
	else if ((strcmp(name, ".") == 0 ||
				strcmp(name, "..") == 0) &&
			show_dotfile)
		return FALSE;
	else
		return TRUE;
}

static gboolean
dirtree_append_subdirs(GtkTreeModel *model,
		GtkTreePath *treepath,
		DirTree *dt)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(model, &iter, treepath))
		return FALSE;

	return dirtree_append_subdirs_iter(model, &iter, dt);
}

/* TODO: macro type check? */
static gboolean
dirtree_append_subdirs_iter(GtkTreeModel *model,
		GtkTreeIter *iter,
		DirTree *dt)
{
	GtkTreeIter *child = g_malloc0(sizeof(*child));
	gchar *row_str,
		  *dirname,
		  *full_dirname,
		  *oldpath = g_get_current_dir();
    DIR *dir;
    struct dirent *entry;
	gboolean hasdir = FALSE;

	full_dirname = get_full_dirname_iter(model, iter);
	/* change working dir so we can scan it */
    if (chdir(full_dirname)) { /* chdir failed */
		chdir(oldpath);
		gtk_tree_model_get(model, iter, COL_TEXT, &row_str, -1);
		/* don't add dummy folders multiple times */
		if (strcmp(row_str, not_readable) != 0 &&
				strcmp(row_str, no_subfolders) != 0) {
			ADD_DUMMY_SUBFOLDER(not_readable);
		}
		g_free(full_dirname);
		g_free(oldpath);
		g_free(child);
		return FALSE;
    }
	g_free(full_dirname);

	/* get DIR object from working dir */
    if (!(dir = opendir("."))) { /* opendir failed */
		chdir(oldpath);
		g_free(oldpath);
		g_free(child);
		return FALSE;
    }

	/* for all subdirectories of the working dir */
    while ((entry = readdir(dir))) {
	    struct stat stat_buff;
		/* append 1st level subdirs to the treeview */
		if (!stat(entry->d_name, &stat_buff) &&
				S_ISDIR(stat_buff.st_mode) &&
				tree_is_dotfile(entry->d_name, dt->show_dotfile)) {
			hasdir = TRUE;
			dirname = g_build_path(G_DIR_SEPARATOR_S, entry->d_name, NULL);
			/* TODO: alphabetical order */
			gtk_tree_store_append(GTK_TREE_STORE(model), child, iter);
			gtk_tree_store_set(GTK_TREE_STORE(model), child,
					COL_ICON, folder_pixmap,
					COL_TEXT, dirname,
					-1);
			g_free(dirname);
		}
	}
	closedir(dir);

	if (!hasdir) {
		ADD_DUMMY_SUBFOLDER(no_subfolders);
	}

	chdir(oldpath);
	g_free(oldpath);
	g_free(child);
	return TRUE;
}

static void
dirtree_remove_subdirs_recursive(GtkTreeModel *model,
		GtkTreeIter *iter)
{
	GtkTreeIter *child_iter = g_malloc(sizeof(*child_iter));
	if (gtk_tree_model_iter_children(model, child_iter, iter))
		dirtree_remove_subdirs_recursive(model, child_iter);

	g_free(child_iter);

	if (gtk_tree_store_remove(GTK_TREE_STORE(model), iter))
		dirtree_remove_subdirs_recursive(model, iter);
}

static gint
sort_iter_compare_func(GtkTreeModel *model,
		GtkTreeIter *a,
		GtkTreeIter *b,
		gpointer userdata)
{
	gint sortcol = GPOINTER_TO_INT(userdata);
	gint ret = 0;

	/* TODO: sort dotfiles before all else */
	switch (sortcol) {
		case SORTID_NAME:
			{
				gchar *name1, *name2;
				gtk_tree_model_get(model, a, COL_TEXT, &name1, -1);
				gtk_tree_model_get(model, b, COL_TEXT, &name2, -1);

				if (name1 == NULL || name2 == NULL) {
					if (name1 == NULL && name2 == NULL)
						break; /* both equal */
					ret = (name1 == NULL) ? -1 : 1;
				} else {
					ret = g_utf8_collate(name1, name2);
				}
				g_free(name1);
				g_free(name2);
			}
			break;
		default:
			g_return_val_if_reached(0);
	}
	return ret;
}


/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

GtkWidget *
dirtree_new(GtkWidget * win, const gchar * start_path, gboolean check_dir,
	     gboolean check_hlinks, gboolean show_dotfile, gint line_style,
	     gint expander_style)
{
    DirTree *dt;
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkCellRenderer *text_renderer, *xpm_renderer;
	GtkTreeModel *model;
	GtkWidget *tree;

	/* globals */
	folder_pixmap = gdk_pixbuf_new_from_xpm_data(folder_xpm);
	ofolder_pixmap = gdk_pixbuf_new_from_xpm_data(folder_open_xpm);

	/* TODO: free/unref everything */

	tree = gtk_tree_view_new();

	text_renderer = gtk_cell_renderer_text_new();
	xpm_renderer = gtk_cell_renderer_pixbuf_new();

	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree),
			-1,
			"Icon",
			xpm_renderer,
			"pixbuf",
			COL_ICON,
			NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree),
			-1,
			"Name",
			text_renderer,
			"text",
			COL_TEXT,
			NULL);

	gtk_tree_view_column_set_sizing(
			gtk_tree_view_get_column(GTK_TREE_VIEW(tree), COL_ICON),
			GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sizing(
			gtk_tree_view_get_column(GTK_TREE_VIEW(tree), COL_TEXT),
			GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	gtk_tree_selection_set_mode(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
			GTK_SELECTION_BROWSE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

	store = gtk_tree_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);

	path = gtk_tree_path_new_first();
	gtk_tree_model_get_iter(model, &iter, path);
	/* TODO: check if iter is non-NULL */
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter,
			COL_ICON, folder_pixmap,
			COL_TEXT, "/",
			-1);

	/* sortable = GTK_TREE_SORTABLE(model); */
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), SORTID_NAME,
			sort_iter_compare_func, GINT_TO_POINTER(SORTID_NAME), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
			SORTID_NAME, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);
	g_object_unref(model);

	dt = g_malloc(sizeof(*dt));

	dt->treeview = GTK_WIDGET(tree);
	dt->collapse = FALSE;
	dt->check_dir = check_dir;
	dt->check_hlinks = check_hlinks;
	dt->show_dotfile = show_dotfile;
	dt->expander_style = expander_style;
	dt->line_style = line_style;
	dt->check_events = TRUE;

	dirtree_append_subdirs(model, path, dt);

	gtk_tree_path_free(path);
	return GTK_WIDGET(dt);
}

void
dirtree_destroy_tree(gpointer *data)
{
    DirTreeNode *node;

    node = *data;
    g_free (node->path);
    g_free (node);
}

gboolean
dirtree_find_file_at_level(GtkTreeModel *model,
		GtkTreeIter *iter,
		gchar *str_search_file)
{
	gboolean found = FALSE;
	gchar *str_cur_file;

	do {
		gtk_tree_model_get(model, iter,
					COL_TEXT, &str_cur_file,
					-1);

		if (!strcmp(str_search_file, str_cur_file)) {
			found = TRUE;
			break;
		}
	} while(gtk_tree_model_iter_next(model, iter));

	if (found)
		return TRUE;
	else
		return FALSE;
}

GtkTreePath *
get_treepath(GtkTreeModel *model,
		gchar *full_dirname)
{
	GtkTreeIter *iter;
	GtkTreePath *treepath = NULL;

	iter = get_iter(model, full_dirname);
	treepath = gtk_tree_model_get_path(model, iter);

	gtk_tree_iter_free(iter);

	return treepath;
}

GtkTreeIter *
get_iter(GtkTreeModel *model,
		gchar *full_dirname)
{
	GtkTreeIter iter, parent_iter;
	gchar *token, *rp;
	GtkTreePath *treepath = gtk_tree_path_new_first();

	if (!gtk_tree_model_get_iter(model, &parent_iter, treepath)) {
		gtk_tree_path_free(treepath);
		return NULL;
	}
	iter = parent_iter; /* initialize, in case we already are at "/" */

	rp = g_strdup(full_dirname);
	token = strtok(rp, "/");
	while (token) {
		if (!gtk_tree_model_iter_has_child(model, &parent_iter))
			/* not scanned yet, break and return the path we have until now */
			break;

		if (!gtk_tree_model_iter_children(model, &iter, &parent_iter)) {
			/* assigning to iter failed */
			g_free(rp);
			gtk_tree_path_free(treepath);
			return NULL;
		} else if (!dirtree_find_file_at_level(model, &iter, token)) {
			/* invalid path, give up */
			g_free(rp);
			gtk_tree_path_free(treepath);
			return NULL;
		} else {
			parent_iter = iter;
			token = strtok(NULL, "/");
		}
	}

	g_free(rp);
	gtk_tree_path_free(treepath);

	return gtk_tree_iter_copy(&iter);
}

/* TODO: rename to treepath_get_full_dirname */
gchar *
get_full_dirname(GtkTreeModel *model,
		GtkTreePath *treepath)
{
	GtkTreeIter iter;

	gtk_tree_model_get_iter(model, &iter, treepath);

	return get_full_dirname_iter(model, &iter);
}

gchar *
get_full_dirname_iter(GtkTreeModel *model,
		GtkTreeIter *myiter)
{
	GtkTreeIter iter = *myiter, parent_iter;
	gchar *row_str,
		  *tmp_str = g_malloc0(256),
		  *full_dirname = g_malloc0(256);

	/*
	 * walk up to "/" and build the full path of the directory
	 */
	while (gtk_tree_model_iter_parent(model, &parent_iter, &iter)) {
		gtk_tree_model_get(model, &iter, COL_TEXT, &row_str, -1);
		if (!strcmp(row_str, not_readable) ||
				!strcmp(row_str, no_subfolders))
			return NULL;
		iter = parent_iter;
		sprintf(full_dirname, "%s%s%s",
				row_str, G_DIR_SEPARATOR_S, tmp_str);
		sprintf(tmp_str, "%s", full_dirname);
		g_free(row_str);
	}
	/* add leading separator */
	sprintf(full_dirname, "%s%s", G_DIR_SEPARATOR_S, tmp_str);

	g_free(tmp_str);

	return full_dirname;
}

void
dirtree_append_sub_subdirs(GtkTreeModel *model,
		GtkTreePath *treepath,
		DirTree *dt)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(model, &iter, treepath))
		return;

	dirtree_append_sub_subdirs_iter(model, &iter, dt);
}

void
dirtree_append_sub_subdirs_iter(GtkTreeModel *model,
		GtkTreeIter *iter,
		DirTree *dt)
{
	GtkTreeIter *child_iter = g_malloc0(sizeof(*child_iter));

	/* If we have children of depth 2, then
	 * the children of depth 1 have already been
	 * scanned. We can rely on this, because we
	 * added dummy subfolders for empty folders.*/
	if (gtk_tree_model_iter_children(model, child_iter, iter) &&
		gtk_tree_model_iter_children(model, iter, child_iter)) {
			g_free(child_iter);
			return;
	}

	/* append all 2nd level subdirs */
	do {
		dirtree_append_subdirs_iter(model, child_iter, dt);
	} while (gtk_tree_model_iter_next(model, child_iter));

	g_free(child_iter);
}

void
dirtree_remove_subdirs(GtkTreeModel *model,
		GtkTreeIter *iter)
{
	GtkTreeIter *child_iter = g_malloc(sizeof(*child_iter));


	if (gtk_tree_model_iter_children(model, child_iter, iter)) {
		dirtree_remove_subdirs_recursive(model, child_iter);
	}

	g_free(child_iter);
}


void
dirtree_refresh_node(GtkTreeModel *model,
		GtkTreeIter *iter,
		DirTree *dt)
{
	dirtree_remove_subdirs(model, iter);
	dirtree_append_subdirs_iter(model, iter, dt);
	dirtree_append_sub_subdirs_iter(model, iter, dt);
}

/* FIXME: not yet ported properly */
void
dirtree_set_mode(DirTree *dt, gboolean check_dir, gboolean check_hlinks,
		gboolean show_dotfile, gint line_style, gint expander_style)
{
    dt->check_dir = check_dir;
    dt->check_hlinks = check_hlinks;
    dt->show_dotfile = show_dotfile;

    if (dt->expander_style != expander_style) {
		/* gtk_ctree_set_expander_style (GTK_CTREE (dt), expander_style); */
		dt->expander_style = expander_style;
    }

    if (dt->line_style != line_style) {
		/* gtk_ctree_set_line_style (GTK_CTREE (dt), line_style); */
		dt->line_style = line_style;
    }
}

gint
dirtree_mkdir(DirTree *dt, gchar *path)
{
	gchar *parent_dir;

	if (!path || !strcmp(path, "")) /* could be (no subfolder) as well */
		return 3;

	if (isdir(path)) /* exists already */
		return 4;

	parent_dir = g_path_get_dirname(path);
	if (!iswritable(parent_dir)) { /* not writable */
		g_free(parent_dir);
		return 1;
	}
	g_free(parent_dir);

	if (makedir(path))
		return 0;
	else
		return 2; /* makedir failed */
}

}
