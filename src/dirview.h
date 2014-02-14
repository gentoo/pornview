/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __DIRVIEW_H__
#define __DIRVIEW_H__

#include <gtk/gtk.h>

#define DIRVIEW           dirview
#define DIRVIEW_DIRTREE   (DIRTREE(dirview->dirtree))
#define DIRVIEW_DIRTREE_WIDGET   dirview->dirtree
#define DIRVIEW_CONTAINER dirview->container
#define DIRVIEW_WIDTH     dirview->scroll_win->allocation.width
#define DIRVIEW_HEIGHT    dirview->scroll_win->allocation.height
#define TREEVIEW (GTK_TREE_VIEW(DIRTREE(DIRVIEW_DIRTREE)->treeview))
#define TREEVIEW_WIDGET (DIRTREE(DIRVIEW_DIRTREE)->treeview)

typedef struct _DirView DirView;

struct _DirView
{
    GtkWidget *container;
    GtkUIManager *toolbar;
    GtkWidget *toolbar_eventbox;
    GtkToolButton *toolbar_refresh_btn;
    GtkToolButton *toolbar_up_btn;
    GtkToolButton *toolbar_down_btn;
    GtkToolButton *toolbar_collapse_btn;
    GtkToolButton *toolbar_expand_btn;
    GtkToolButton *toolbar_show_dotfile_btn;
    GtkToolButton *toolbar_go_home;
    GtkToolButton *toolbar_back;

    GtkWidget *scroll_win;
    GtkWidget *dirtree;

    gboolean lock_select;
};

/* not used anywhere else? */
extern DirView *dirview;

static
void cb_dirview_selection_changed(GtkTreeSelection *treeselection,
		gpointer user_data);

/**
 * Callback function for the "row-activated" signal
 * of treeview.
 *
 * @param treeview the object on which the signal is emitted
 * @param treepath the GtkTreePath for the activated row
 * @param treeview_column the GtkTreeViewColumn in which the
 *        activation occurred
 * @param userdata user data set when the signal handler was connected
 */
static void
cb_dirview_row_activated(GtkTreeView *treeview,
		GtkTreePath *treepath,
		GtkTreeViewColumn *treeview_column,
		gpointer userdata);

/**
 * Callback function for the "row-expanded" signal
 * of treeview.
 *
 * @param treeview the object on which the signal is emitted
 * @param iter the tree iter of the expanded row
 * @param path a tree path that points to the row
 * @param userdata user data set when the signal handler was connected
 */
static void
cb_dirview_row_expanded(GtkTreeView *treeview,
		GtkTreeIter *iter,
		GtkTreePath *treepath,
		gpointer userdata);

/**
 * Callback function for the "test-row-collapsed" signal
 * of treeview; basically just adjusts the folder icon.
 *
 * recursive
 *
 * @param treeview the object on which the signal is emitted
 * @param iter the tree iter of the expanded row
 * @param path a tree path that points to the row
 * @param userdata user data set when the signal handler was connected
 */
static void
cb_dirview_row_test_collapsed(GtkTreeView *treeview,
		GtkTreeIter *iter,
		GtkTreePath *treepath,
		gpointer userdata);

static gboolean
cb_dirview_button_press_event(GtkWidget *widget,
		GdkEventButton *event,
		gpointer data);

static void
cb_dirview_refresh();

static void
cb_dirview_show_dotfile();

static void
cb_dirview_go_home();

static void
dirview_create_rightclick_menu(GtkWidget *rightclick_menu);

static void
cb_dirview_entry_enter (GtkEntry *entry,
		gpointer path_dialog);

static void
dirview_mkdir(GtkMenuItem *menuitem,
		gpointer data);

static void
dirview_rename_dir(GtkMenuItem *menuitem,
		gpointer data);
void
dirview_create (const gchar *start_path, GtkWidget *parent_win);

void
dirview_goto_dir(GtkTreeView *treeview, const gchar *dest_dir);

/**
 * Get's the currently selected dir in the dirview
 * agnostic of the current working directory.
 *
 * @param model the interface which the dirview is associated with
 * @return a newly allocated string representing the currently selectet dir,
 *         or NULL on failure
 */
gchar *
dirview_get_current_dir(GtkTreeModel *model);

void
dirview_scroll_center (void);

void
dirview_destroy(void);


#endif /* __DIRVIEW_H__ */
