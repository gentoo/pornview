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

typedef struct _DirView DirView;

struct _DirView
{
    GtkWidget      *container;
    GtkWidget      *toolbar;
    GtkWidget      *toolbar_eventbox;
    GtkWidget      *toolbar_refresh_btn;
    GtkWidget      *toolbar_up_btn;
    GtkWidget      *toolbar_down_btn;
    GtkWidget      *toolbar_collapse_btn;
    GtkWidget      *toolbar_expand_btn;
    GtkWidget      *toolbar_show_dotfile_btn;
    GtkWidget      *toolbar_go_home;

    GtkWidget      *scroll_win;
    GtkWidget      *dirtree;

    gboolean        lock_select;
};

extern DirView *dirview;

#define DIRVIEW           dirview
#define DIRVIEW_DIRTREE   dirview->dirtree
#define DIRVIEW_CONTAINER dirview->container
#define DIRVIEW_WIDTH     dirview->scroll_win->allocation.width
#define DIRVIEW_HEIGHT    dirview->scroll_win->allocation.height

void            dirview_create (const gchar * start_path,
				GtkWidget * parent_win);
void            dirview_destroy (void);
void            dirview_scroll_center (void);
static void
dirview_select_node(DirView * dv, GtkCTreeNode * node);

#endif /* __DIRVIEW_H__ */
