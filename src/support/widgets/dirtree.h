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

#define TYPE_DIRTREE            (dirtree_get_type ())
#define DIRTREE(obj)            (GTK_CHECK_CAST((obj), TYPE_DIRTREE, DirTree))
#define DIRTREE_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), TYPE_DIRTREE, DirTreeClass))
#define IS_DIRTREE(obj)         (GTK_CHECK_TYPE((obj), TYPE_DIRTREE))
#define IS_DIRTREE_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), TYPE_DIRTREE))

#define MAX_PATH_LENGTH 4096

typedef struct _DirTree DirTree;
typedef struct _DirTreeNode DirTreeNode;
typedef struct _DirTreeClass DirTreeClass;
typedef struct _DirTreeMode DirTreeMode;

struct _DirTree
{
    GtkCTree        tree;

    gboolean        collapse;

    gboolean        check_dir;
    gboolean        check_hlinks;
    gboolean        show_dotfile;

    gint            line_style;
    gint            expander_style;

    gboolean        check_events;
};

struct _DirTreeNode
{
    gboolean        scanned;
    gchar          *path;
};

struct _DirTreeClass
{
    GtkCTreeClass   parent_class;

    void            (*select_file) (DirTree * dir_tree, char *file_path);
};

GtkType         dirtree_get_type (void);
GtkWidget      *dirtree_new (GtkWidget * win, const gchar * root_path,
			     gboolean check_dir, gboolean follow_link,
			     gboolean show_dotfile, gint line_style,
			     gint expander_style);
void            dirtree_set_mode (DirTree * dt, gboolean check_dir,
				  gboolean show_dotfile, gboolean follow_link,
				  gint line_style, gint expander_style);
void            dirtree_refresh_tree (DirTree * dt, GtkCTreeNode * parent,
				      gboolean selected);
void            dirtree_refresh_node (DirTree * dt, GtkCTreeNode * node);
GtkCTreeNode   *dirtree_find_file (DirTree * dt, GtkCTreeNode * parent,
				   char *file_name);
#define dirtree_freeze(dirtree) (gtk_clist_freeze(GTK_CLIST(dirtree)))
#define dirtree_thaw(dirtree)   (gtk_clist_thaw(GTK_CLIST(dirtree)))

#endif /* __DIRTREE_H__ */
