/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __DIRTREE_H__
#define __DIRTREE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#include "pixmaps/folder.xpm"
#include "pixmaps/folder_open.xpm"
#include "pixmaps/folder_link.xpm"
#include "pixmaps/folder_link_open.xpm"
#include "pixmaps/folder_lock.xpm"
#include "pixmaps/folder_go.xpm"
#include "pixmaps/folder_up.xpm"

#define DIRTREE(obj) ((DirTree *)(obj))

#define MAX_PATH_LENGTH 4096


GdkPixbuf *folder_pixmap,
		  *ofolder_pixmap,
		  *lfolder_pixmap,
		  *lofolder_pixmap,
		  *lckfolder_pixmap,
		  *gofolder_pixmap,
		  *upfolder_pixmap;

typedef struct _DirTree DirTree;
typedef struct _DirTreeNode DirTreeNode;
typedef struct _DirTreeMode DirTreeMode;

struct _DirTree
{
    GtkWidget *treeview; /* GtkTreeView */

    gboolean collapse;

    gboolean check_dir;
    gboolean check_hlinks;
    gboolean show_dotfile;

    gint line_style;
    gint expander_style;

    gboolean check_events;
};

struct _DirTreeNode
{
    gboolean scanned;
    gchar *path;
};

enum
{
	COL_ICON = 0,
	COL_TEXT,
	NUM_COLS
};

enum
{
	SORTID_NAME = 0,
};



static  gboolean
tree_is_subdirs(gchar *name, gboolean show_dotfile);

static gboolean
tree_is_dotfile(gchar *name, gboolean show_dotfile);

/**
 * Append all subdirs of the row described by treepath
 * to the tree model.
 *
 * @param model the interface which the treepath is associated with
 * @param treepath describes the row we scan the subdirs of
 * @return TRUE/FALSE for success/failure
 */
static gboolean
dirtree_append_subdirs(GtkTreeModel *model,
		GtkTreePath *treepath,
		DirTree *dt);

static gboolean
dirtree_append_subdirs_iter(GtkTreeModel *model,
		GtkTreeIter *iter,
		DirTree *dt);

static void
dirtree_remove_subdirs_recursive(GtkTreeModel *model,
		GtkTreeIter *iter);

static void
dirtree_remove_subdirs_recursive(GtkTreeModel *model,
		GtkTreeIter *iter);

GtkWidget *
dirtree_new(GtkWidget * win, const gchar * start_path, gboolean check_dir,
	     gboolean check_hlinks, gboolean show_dotfile, gint line_style,
	     gint expander_style);

void
dirtree_destroy_tree(gpointer *data);

/**
 * Find a file/dir at the same level, result stored
 * in iter.
 *
 * @param model the interface which the treepath is associated with
 * @param iter describes the level [out]
 * @param str_search_file describes the file/dir
 *        (single name, without separator or period)
 * @return TRUE/FALSE for success/failure
 */
gboolean
dirtree_find_file_at_level(GtkTreeModel *model,
		GtkTreeIter *iter,
		gchar *str_search_file);

/**
 * Get the treepath that is corresponding to the full_dirname
 * of the model.
 *
 * @param model the interface which the treepath is associated with
 * @param full_dirname the full directory name
 * @return a newly allocated GtkTreePath, must be freed, NULL on failure
 */
GtkTreePath *
get_treepath(GtkTreeModel *model,
		gchar *full_dirname);

/**
 * Get the iter that is corresponding to the full_dirname
 * of the model.
 *
 * @param model the interface which the treepath is associated with
 * @param full_dirname the full directory name
 * @return a newly allocated GtkTreeIter, must be freed, NULL on failure
 */
GtkTreeIter *
get_iter(GtkTreeModel *model,
		gchar *full_dirname);

/**
 * Get the full filsystem path of the row which is
 * described by treepath.
 *
 * @param model the interface which the treepath is associated with
 * @param treepath describes the row
 * @return a newly allocated string
 */
gchar *
get_full_dirname(GtkTreeModel *model,
		GtkTreePath *treepath);

gchar *
get_full_dirname_iter(GtkTreeModel *model,
		GtkTreeIter *myiter);

/**
 * Basically calls dirtree_append_subdirs on
 * all child rows of the parent row described by path,
 * so we always have scanned one depth level ahead.
 *
 * @param model the interface which the treepath is associated with
 * @param treepath the GtkTreePath for the activated row
 */
void
dirtree_append_sub_subdirs(GtkTreeModel *model,
		GtkTreePath *treepath,
		DirTree *dt);

void
dirtree_append_sub_subdirs_iter(GtkTreeModel *model,
		GtkTreeIter *iter,
		DirTree *dt);

void
dirtree_remove_subdirs(GtkTreeModel *model,
		GtkTreeIter *iter);

void
dirtree_refresh_node(GtkTreeModel *model,
		GtkTreeIter *iter,
		DirTree *dt);

void
dirtree_set_mode(DirTree *dt, gboolean check_dir, gboolean check_hlinks,
		gboolean show_dotfile, gint line_style, gint expander_style);


#endif /* __DIRTREE_H__ */
