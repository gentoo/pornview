/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 *  These codes are mostly taken from Another X image viewer.
 *
 *  Another X image viewer Author:
 *     David Ramboz <dramboz@users.sourceforge.net>
 */

#ifndef __ZLIST_H__
#define __ZLIST_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "scrolled.h"

#define ZLIST_TYPE               (zlist_get_type ())
#define ZLIST(widget)            (GTK_CHECK_CAST ((widget), ZLIST_TYPE, ZList))
#define ZLIST_CLASS(klass)	 (GTK_CHECK_CLASS_CAST ((klass), ZLIST_TYPE, ZListClass))
#define IS_ZLIST(widget)	 (GTK_CHECK_TYPE ((widget), ZLIST_TYPE))
#define IS_ZLIST_CLASS(klass)	 (GTK_CHECK_CLASS_TYPE ((klass), ZLIST_TYPE))

enum
{
    ZLIST_HORIZONTAL = 1 << 1,
    ZLIST_1 = 1 << 2,
    ZLIST_USES_DND = 1 << 3,
    ZLIST_HIGHLIGHTED = 1 << 4
};

typedef struct _ZList ZList;
typedef struct _ZListClass ZListClass;

struct _ZList
{
    Scrolled        scrolled;

    guint           flags;

    gint            cell_width;
    gint            cell_height;

    gint            rows;
    gint            columns;

    GArray         *cells;
    gint            cell_count;

    gint            selection_mode;
    GList          *selection;
    int             focus;
    int             anchor;

    int             cell_x_pad, cell_y_pad;
    int             x_pad, y_pad;

    GtkWidget      *entered_cell;
};

struct _ZListClass
{
    ScrolledClass   parent_class;

    void            (*clear) (ZList * list);
    void            (*cell_draw) (ZList * list, gpointer cell,
				  GdkRectangle * cell_area,
				  GdkRectangle * area);
    void            (*cell_size_request) (ZList * list, gpointer cell,
					  GtkRequisition * requisition);
    void            (*cell_draw_focus) (ZList * list, gpointer cell,
					GdkRectangle * cell_area);
    void            (*cell_draw_default) (ZList * list, gpointer cell,
					  GdkRectangle * cell_area);
    void            (*cell_select) (ZList * list, gpointer cell);
    void            (*cell_unselect) (ZList * list, gpointer cell);
};

GtkType         zlist_get_type (void);

void            zlist_construct (ZList * list, int flags);
GtkWidget      *zlist_new (guint flags);
guint           zlist_add (ZList * list, gpointer cell);
void            zlist_remove (ZList * list, gpointer cell);
void            zlist_clear (ZList * list);
void            zlist_set_selection_mode (ZList * list,
					  GtkSelectionMode mode);
void            zlist_set_cell_padding (ZList * list, int x_pad, int y_pad);
void            zlist_set_cell_size (ZList * list, gint width, gint height);
void            zlist_set_1 (ZList * list, int one);
gpointer        zlist_cell_from_xy (ZList * list, gint x, gint y);

/* internal */
#define       ZLIST_CELL_FROM_INDEX(list, index)     (g_array_index (ZLIST(list)->cells, gpointer, index))

void            zlist_draw_cell (ZList * list, int index);
int             zlist_update_cell_size (ZList * list, gpointer cell);
int             zlist_cell_index_from_xy (ZList * list, gint x, gint y);

void            zlist_cell_focus (ZList * list, gint index);
void            zlist_select_all (ZList * list);

#endif /* __ZLIST_H__ */
