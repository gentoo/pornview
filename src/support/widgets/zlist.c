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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <gdk/gdkkeysyms.h>

#include "gtk2-compat.h"
#include "zlist.h"

#define bw(widget)                  (int) GTK_CONTAINER(widget)->border_width

#define CELL_COL_FROM_X(list, x)    (SCROLLED_VX (list, (x) - (list)->x_pad - bw (list)) / \
                                     (list)->cell_width)
#define CELL_ROW_FROM_Y(list, y)    (SCROLLED_VY (list, (y) - (list)->y_pad - bw (list)) / \
                                     (list)->cell_height)
#define CELL_X_FROM_COL(list, col)  (SCROLLED_X (list, (col) * (list)->cell_width  + (list)->x_pad))
#define CELL_Y_FROM_ROW(list, row)  (SCROLLED_Y (list, (row) * (list)->cell_height + (list)->y_pad))
#define LIST_WIDTH(list)            ((list)->columns * (list)->cell_width)
#define LIST_HEIGHT(list)           ((list)->rows * (list)->cell_height)
#define HIGHLIGHT_SIZE 2

static void zlist_class_init (ZListClass * klass);
static void zlist_init (ZList * list);

static void zlist_finalize (GObject * object);

static void zlist_map (GtkWidget * widget);
static void zlist_unmap (GtkWidget * widget);
static void zlist_realize (GtkWidget * widget);
static void zlist_unrealize (GtkWidget * widget);
static void zlist_size_request (GtkWidget * widget,
				GtkRequisition * requisition);
static void zlist_size_allocate (GtkWidget * widget,
				 GtkAllocation * allocation);
static gint zlist_expose (GtkWidget * widget, GdkEventExpose * event);
static void zlist_update (ZList * list);
static void zlist_draw (GtkWidget * widget, GdkRectangle * area);

static gint zlist_button_press (GtkWidget * widget, GdkEventButton * event);
static gint zlist_button_release (GtkWidget * widget, GdkEventButton * event);
static gint zlist_motion_notify (GtkWidget * widget, GdkEventMotion * event);
static gint zlist_key_press (GtkWidget * widget, GdkEventKey * event);
static gint zlist_focus_in (GtkWidget * widget, GdkEventFocus * event);
static gint zlist_focus_out (GtkWidget * widget, GdkEventFocus * event);
static gint zlist_drag_motion (GtkWidget * widget,
			       GdkDragContext * context,
			       gint x, gint y, guint time);
static gint zlist_drag_drop (GtkWidget * widget,
			     GdkDragContext * context,
			     gint x, gint y, guint time);
static void zlist_drag_leave (GtkWidget * widget,
			      GdkDragContext * context, guint time);
static void zlist_highlight (GtkWidget * widget);
static void zlist_unhighlight (GtkWidget * widget);
static gint zlist_focus (GtkContainer * container, GtkDirectionType dir);
static void zlist_cell_draw_focus (ZList * list, int index);
static void zlist_cell_draw_default (ZList * list, int index);
static void zlist_cell_select (ZList * list, int index);
static void zlist_cell_unselect (ZList * list, int index);
static void zlist_cell_toggle (ZList * list, int index);
static void zlist_unselect_all (ZList * list);
static void zlist_extend_selection (ZList * list, int to);


static void zlist_forall (GtkContainer * container,
			  gboolean include_internals,
			  GtkCallback callback, gpointer callback_data);
static void zlist_adjust_adjustments (Scrolled * scrolled);
static int zlist_cell_index (ZList * list, gpointer cell);
static void zlist_cell_pos (ZList * list, int index, int *row, int *col);
static void zlist_cell_area (ZList * list, int index,
			     GdkRectangle * cell_area);
static void zlist_moveto (ZList * list, int index);

enum
{
    CLEAR,
    CELL_DRAW,
    CELL_SIZE_REQUEST,
    CELL_DRAW_FOCUS,
    CELL_DRAW_DEFAULT,
    CELL_SELECT,
    CELL_UNSELECT,
    LAST_SIGNAL
};

static GtkWidgetClass *parent_class = NULL;

static guint zlist_signals[LAST_SIGNAL] = { 0 };

GtkType
zlist_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
	static const GtkTypeInfo zlist_type_info = {
	    "ZList",
	    sizeof (ZList),
	    sizeof (ZListClass),
	    (GtkClassInitFunc) zlist_class_init,
	    (GtkObjectInitFunc) zlist_init,
	    /*
	     * reserved_1 
	     */
	    NULL,
	    /*
	     * reserved_2 
	     */
	    NULL,
	    (GtkClassInitFunc) NULL,
	};

	type = gtk_type_unique (SCROLLED_TYPE, &zlist_type_info);
    }

    return type;
}

static void
zlist_class_init (ZListClass * klass)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    ScrolledClass *scrolled_class;

    parent_class = gtk_type_class (SCROLLED_TYPE);

    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;
    scrolled_class = (ScrolledClass *) klass;


    zlist_signals[CLEAR] =
	gtk_signal_new ("clear",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ZListClass, clear),
			gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

    zlist_signals[CELL_DRAW] =
	gtk_signal_new ("cell_draw",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ZListClass, cell_draw),
			gtk_marshal_NONE__POINTER_POINTER_POINTER,
			GTK_TYPE_NONE, 3, GTK_TYPE_POINTER, GTK_TYPE_POINTER,
			GTK_TYPE_POINTER);

    zlist_signals[CELL_SIZE_REQUEST] =
	gtk_signal_new ("cell_size_request",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ZListClass, cell_size_request),
			gtk_marshal_NONE__POINTER_POINTER,
			GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

    zlist_signals[CELL_DRAW_FOCUS] =
	gtk_signal_new ("cell_draw_focus",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ZListClass, cell_draw_focus),
			gtk_marshal_NONE__POINTER_POINTER,
			GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

    zlist_signals[CELL_DRAW_DEFAULT] =
	gtk_signal_new ("cell_draw_default",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ZListClass, cell_draw_default),
			gtk_marshal_NONE__POINTER_POINTER,
			GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

    zlist_signals[CELL_SELECT] =
	gtk_signal_new ("cell_select",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ZListClass, cell_select),
			gtk_marshal_NONE__POINTER,
			GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);


    zlist_signals[CELL_UNSELECT] =
	gtk_signal_new ("cell_unselect",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ZListClass, cell_unselect),
			gtk_marshal_NONE__POINTER,
			GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

    gtk_object_class_add_signals (object_class, zlist_signals, LAST_SIGNAL);

    OBJECT_CLASS_SET_FINALIZE_FUNC (klass, zlist_finalize);

    widget_class->map = zlist_map;
    widget_class->unmap = zlist_unmap;
    widget_class->realize = zlist_realize;
    widget_class->unrealize = zlist_unrealize;
    widget_class->size_request = zlist_size_request;
    widget_class->size_allocate = zlist_size_allocate;
    widget_class->expose_event = zlist_expose;

    widget_class->button_press_event = zlist_button_press;
    widget_class->button_release_event = zlist_button_release;
    widget_class->motion_notify_event = zlist_motion_notify;
    widget_class->key_press_event = zlist_key_press;
    widget_class->focus_in_event = zlist_focus_in;
    widget_class->focus_out_event = zlist_focus_out;
    widget_class->drag_motion = zlist_drag_motion;
    widget_class->drag_drop = zlist_drag_drop;
    widget_class->drag_leave = zlist_drag_leave;

    container_class->forall = zlist_forall;

/*  container_class->focus               = zlist_focus; */

    scrolled_class->adjust_adjustments = zlist_adjust_adjustments;

    klass->cell_draw = NULL;
    klass->cell_size_request = NULL;
    klass->cell_draw_focus = NULL;
    klass->cell_draw_default = NULL;
    klass->cell_select = NULL;
    klass->cell_unselect = NULL;
}

static void
zlist_init (ZList * list)
{
    GTK_WIDGET_UNSET_FLAGS (list, GTK_NO_WINDOW);
    GTK_WIDGET_SET_FLAGS (list, GTK_CAN_FOCUS);

    list->flags = 0;
    list->cell_width = 1;
    list->cell_height = 1;
    list->rows = 1;
    list->columns = 1;
    list->cells = NULL;
    list->cell_count = 0;
    list->selection_mode = GTK_SELECTION_SINGLE;
    list->selection = NULL;
    list->focus = -1;
    list->anchor = -1;
    list->cell_x_pad = 4;
    list->cell_y_pad = 4;
    list->x_pad = 0;
    list->y_pad = 0;
    list->entered_cell = NULL;
}

void
zlist_construct (ZList * list, int flags)
{
    g_return_if_fail (list);

    list->flags = flags;
    list->cells = g_array_new (0, 0, sizeof (gpointer));
}

GtkWidget *
zlist_new (guint flags)
{
    ZList  *list;

    list = (ZList *) gtk_type_new (zlist_get_type ());
    g_return_val_if_fail (list, NULL);

    zlist_construct (list, flags);

    return (GtkWidget *) list;
}

static void
zlist_finalize (GObject * object)
{
    OBJECT_CLASS_FINALIZE_SUPER (parent_class, object);
}

static void
zlist_map (GtkWidget * widget)
{

    GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
    gdk_window_show (widget->window);
}

static void
zlist_unmap (GtkWidget * widget)
{

    GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
    gdk_window_hide (widget->window);
}

static void
zlist_realize (GtkWidget * widget)
{
    GdkWindowAttr attributes;
    gint    attributes_mask;

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x + bw (widget);
    attributes.y = widget->allocation.y + bw (widget);
    attributes.width = widget->allocation.width - 2 * bw (widget);
    attributes.height = widget->allocation.height - 2 * bw (widget);
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = gtk_widget_get_events (widget);
    attributes.event_mask |= (GDK_EXPOSURE_MASK |
			      GDK_BUTTON_PRESS_MASK |
			      GDK_BUTTON_RELEASE_MASK |
			      GDK_POINTER_MOTION_MASK | GDK_KEY_PRESS_MASK);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				     &attributes, attributes_mask);
    gdk_window_set_user_data (widget->window, widget);
    widget->style = gtk_style_attach (widget->style, widget->window);

    gdk_window_set_background (widget->window,
			       &widget->style->base[GTK_STATE_NORMAL]);

    scrolled_realize (SCROLLED (widget));
}

static void
zlist_unrealize (GtkWidget * widget)
{

    scrolled_unrealize (SCROLLED (widget));

    if (parent_class->unrealize)
	(*parent_class->unrealize) (widget);
}

static void
zlist_size_request (GtkWidget * widget, GtkRequisition * requisition)
{

    requisition->width = requisition->height = 50;
}

static void
zlist_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{

    if (allocation->x == widget->allocation.x
	&& allocation->y == widget->allocation.y
	&& allocation->width == widget->allocation.width
	&& allocation->height == widget->allocation.height)
	return;

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
	gdk_window_move_resize (widget->window,
				allocation->x + bw (widget),
				allocation->y + bw (widget),
				allocation->width - 2 * bw (widget),
				allocation->height - 2 * bw (widget));

    zlist_update (ZLIST (widget));
}

static void
zlist_update (ZList * list)
{
    GtkWidget *widget;
    GtkAdjustment *adj;

    widget = GTK_WIDGET (list);

    if (list->flags & ZLIST_HORIZONTAL)
    {
	if (list->flags & ZLIST_1)
	    list->cell_height = widget->allocation.height - 2 * bw (widget);

	list->rows =
	    MAX (1,
		 (widget->allocation.height -
		  2 * bw (widget)) / list->cell_height);
	list->columns =
	    list->cell_count % list->rows ? list->cell_count / list->rows +
	    1 : list->cell_count / list->rows;

	list->y_pad =
	    (widget->allocation.height - LIST_HEIGHT (list) -
	     2 * bw (widget)) / 2;
	if (list->y_pad < 0)
	    list->y_pad = 0;

	zlist_adjust_adjustments (SCROLLED (list));
	adj = SCROLLED (widget)->h_adjustment;
	if (adj && !SCROLLED (widget)->freeze_count
	    && adj->value > adj->upper - adj->page_size)
	{
	    adj->value = adj->upper - adj->page_size;
	    gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed", NULL);
	}

    }
    else
    {
	if (list->flags & ZLIST_1)
	    list->cell_width = widget->allocation.width - 2 * bw (widget);

	list->columns =
	    MAX (1,
		 (widget->allocation.width -
		  2 * bw (widget)) / list->cell_width);
	list->rows =
	    list->cell_count % list->columns ? list->cell_count /
	    list->columns + 1 : list->cell_count / list->columns;

	list->x_pad =
	    (widget->allocation.width - LIST_WIDTH (list) -
	     2 * bw (widget)) / 2;
	if (list->x_pad < 0)
	    list->x_pad = 0;

	zlist_adjust_adjustments (SCROLLED (widget));
	adj = SCROLLED (widget)->v_adjustment;
	if (adj && !SCROLLED (widget)->freeze_count
	    && adj->value > adj->upper - adj->page_size)
	{
	    adj->value = adj->upper - adj->page_size;
	    gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed", NULL);
	}
    }
}

static  gint
zlist_expose (GtkWidget * widget, GdkEventExpose * event)
{

    if (GTK_WIDGET_DRAWABLE (widget) && event->window == widget->window)
	zlist_draw (widget, &event->area);

    return FALSE;
}

static void
zlist_draw (GtkWidget * widget, GdkRectangle * area)
{
    ZList  *list;
    Scrolled *scr;
    gpointer *cell;
    GdkRectangle list_area, cell_area, intersect_area;
    int     first_row, last_row;
    int     first_column, last_column;
    int     i, j, idx, c;

    list = ZLIST (widget);
    scr = SCROLLED (widget);

    if (!GTK_WIDGET_DRAWABLE (widget) || scr->freeze_count)
	return;

    list_area.x = list_area.y = 0;
    /*
     * xxx 
     */
    list_area.width = widget->allocation.width;
    list_area.height = widget->allocation.height;

    if (!area)
	area = &list_area;
    /*
     * gdk_window_clear_area (widget->window, 
     * area->x, area->y, area->width, area->height); 
     */

    if (!list->cell_count)
    {
	gdk_window_clear_area (widget->window,
			       area->x, area->y, area->width, area->height);
	return;
    }

    first_column = CELL_COL_FROM_X (list, area->x);
    first_column = CLAMP (first_column, 0, list->columns);
    last_column = CELL_COL_FROM_X (list, area->x + area->width) + 1;
    last_column = CLAMP (last_column, 0, list->columns);

    first_row = CELL_ROW_FROM_Y (list, area->y);
    first_row = CLAMP (first_row, 0, list->rows);
    last_row = CELL_ROW_FROM_Y (list, area->y + area->height) + 1;
    last_row = CLAMP (last_row, 0, list->rows);

    if (list->flags & ZLIST_HORIZONTAL)
    {
	/*
	 * clear the padding area (bottom & top) 
	 */
	c = list->y_pad - area->y;
	if (c > 0)
	    gdk_window_clear_area (widget->window,
				   area->x, area->y, area->width, c);

	c = area->y + area->height - (LIST_HEIGHT (list) + list->y_pad);
	if (c > 0)
	    gdk_window_clear_area (widget->window,
				   area->x, area->y + area->height - c,
				   area->width, c);


	for (j = first_column; j < last_column; j++)
	{
	    /*
	     * clear the cell vertical padding 
	     */
	    if (list->cell_x_pad)
		gdk_window_clear_area (widget->window,
				       CELL_X_FROM_COL (list, j), area->y,
				       list->cell_x_pad, area->height);

	    idx = j * list->rows + first_row;
	    for (i = first_row; idx < list->cell_count && i < last_row;
		 i++, idx++)
	    {
		/*
		 * clear the cell horizontal padding 
		 */
		if (list->cell_y_pad)
		    gdk_window_clear_area (widget->window,
					   area->x, CELL_Y_FROM_ROW (list, i),
					   area->width, list->cell_y_pad);

		cell = ZLIST_CELL_FROM_INDEX (list, idx);

		cell_area.x = CELL_X_FROM_COL (list, j) + list->cell_x_pad;
		cell_area.y = CELL_Y_FROM_ROW (list, i) + list->cell_y_pad;
		cell_area.width = list->cell_width - list->cell_x_pad;
		cell_area.height = list->cell_height - list->cell_y_pad;

		if (gdk_rectangle_intersect
		    (area, &cell_area, &intersect_area))
		{
		    if (idx < list->cell_count)
			gtk_signal_emit (GTK_OBJECT (list),
					 zlist_signals[CELL_DRAW], cell,
					 &cell_area, &intersect_area);
		    else
			gdk_window_clear_area (widget->window,
					       intersect_area.x,
					       intersect_area.y,
					       intersect_area.width,
					       intersect_area.height);
		}
	    }			/* rows loop */
	}			/* columns loop */

	/*
	 * clear the right of the list  XXX should hasppen 
	 */
	c = area->x + area->width - CELL_X_FROM_COL (list, j);
	if (c > 0)
	    gdk_window_clear_area (widget->window,
				   CELL_X_FROM_COL (list, j), area->y,
				   c, area->height);
    }
    else
    {				/* VERTICAL ZLIST (ie zlist with a vertical scrollbar) */
	/*
	 * clear the padding area (right & left) 
	 */
	c = list->x_pad - area->x;
	if (c > 0)
	    gdk_window_clear_area (widget->window,
				   area->x, area->y, c, area->height);

	c = area->x + area->width - (LIST_WIDTH (list) + list->x_pad);
	if (c > 0)
	    gdk_window_clear_area (widget->window,
				   area->x + area->width - c, area->y,
				   c, area->height);

	for (i = first_row; i < last_row; i++)
	{
	    /*
	     * clear the cell horizontal padding 
	     */
	    if (list->cell_y_pad)
		gdk_window_clear_area (widget->window,
				       area->x, CELL_Y_FROM_ROW (list, i),
				       area->width, list->cell_y_pad);

	    idx = i * list->columns + first_column;
	    for (j = first_column;
		 /*
		  * idx < list->cell_count && 
		  */ j < last_column; j++, idx++)
	    {
		/*
		 * clear the cell vertical padding 
		 */
		if (list->cell_x_pad)
		    gdk_window_clear_area (widget->window,
					   CELL_X_FROM_COL (list, j), area->y,
					   list->cell_x_pad, area->height);

		if (idx < list->cell_count)
		    cell = ZLIST_CELL_FROM_INDEX (list, idx);

		cell_area.x = CELL_X_FROM_COL (list, j) + list->cell_x_pad;
		cell_area.y = CELL_Y_FROM_ROW (list, i) + list->cell_y_pad;
		cell_area.width = list->cell_width - list->cell_x_pad;
		cell_area.height = list->cell_height - list->cell_y_pad;

		if (gdk_rectangle_intersect
		    (area, &cell_area, &intersect_area))
		{
		    if (idx < list->cell_count)
			gtk_signal_emit (GTK_OBJECT (list),
					 zlist_signals[CELL_DRAW], cell,
					 &cell_area, &intersect_area);
		    else
			gdk_window_clear_area (widget->window,
					       intersect_area.x,
					       intersect_area.y,
					       intersect_area.width,
					       intersect_area.height);
		}
	    }			/* columns loop */
	}			/* rows loop */

	/*
	 * clear the bottom of the list 
	 */
	c = area->y + area->height - CELL_Y_FROM_ROW (list, i);
	if (c > 0)
	    gdk_window_clear_area (widget->window,
				   area->x, CELL_Y_FROM_ROW (list, i),
				   area->width, c);
    }

    if (list->focus != -1)
	zlist_cell_draw_focus (list, list->focus);

    if (list->flags & ZLIST_HIGHLIGHTED)
	zlist_highlight (widget);
}

/* 
 * Selection semantics used in this widget:
 * 
 * note : me is the cell pointed by the cursor
 *
 *--------------------------------------------------------------------------------------
 *             |     BUTTON_PRESS           |   MOTION             | BUTTON_RELEASE    |
 * ------------------------------------------------------------------------------------|
 * SINGLE      | focus = me                 | focus = me           | unselect all      |
 *             | anchor = me                | anchor = me          | if (anchor == me) |
 *             |                            |                      |   cell_toggle (me)|
 * ------------------------------------------------------------------------------------|
 * MULTIPLE    |        * idem *            |       * idem *       | if (anchor == me) |
 *             |                            |                      |   cell_toggle (me)|
 *             |                            |                      |                   |
 * ------------------------------------------------------------------------------------|
 * BROWSE      | unselect all               | unselect all         |                   |
 *             | cell_select (me)           | cell_select (me)     |                   |
 *             |                            |                      |                   |
 * ------------------------------------------------------------------------------------|
 * EXTENDED    | if (<ctl> pressed)         | if (<ctl> pressed)    | anchor = focus   |
 *             |   focus = anchor = me      |   focus = anchor = me | extend_selection |
 *             |   cell_toggle (me)         |   cell_toggle (me)    |                  |
 *             | else if (<shift> pressed)  | else                  |                  |
 *             |   extend_selection         |   extend_selection    |                  |
 *             | else                       |                       |                  |
 *             |   unselect all             |                       |                  |
 *             |   focus = anchor = me      |                       |                  |
 *             |   cell_select (me)         |                       |                  |
 *             |                            |                       |                  |
 * ------------------------------------------------------------------------------------|
 * 
 * XXX needs to be changed because of dnd 
 */

static  gint
zlist_button_press (GtkWidget * widget, GdkEventButton * event)
{
    ZList  *list;
    gpointer *cell;
    int     idx;

    if (event->type != GDK_BUTTON_PRESS ||
	event->button != 1 || event->window != widget->window)
	return FALSE;

    list = ZLIST (widget);

    idx = zlist_cell_index_from_xy (list, event->x, event->y);
    if (idx < 0)
    {
/* select */
/*    zlist_unselect_all (list); */
	return FALSE;
    }

    cell = ZLIST_CELL_FROM_INDEX (list, idx);

    if (list->focus != idx)
    {
	if (list->focus != -1)
	    zlist_cell_draw_default (list, list->focus);
	list->focus = idx;
    }

    if (list->flags & ZLIST_USES_DND &&
	g_list_find (list->selection, GUINT_TO_POINTER (idx)))
    {
	list->anchor = idx;
	zlist_cell_draw_focus (list, idx);
	return FALSE;
    }

    switch (list->selection_mode)
    {
      case GTK_SELECTION_SINGLE:

	  list->anchor = idx;
	  zlist_cell_draw_focus (list, idx);
	  break;

      case GTK_SELECTION_BROWSE:
	  zlist_unselect_all (list);
	  zlist_cell_select (list, idx);
	  break;

      case GTK_SELECTION_EXTENDED:
	  if (event->state & GDK_CONTROL_MASK)
	  {
	      list->anchor = idx;
	      zlist_cell_toggle (list, idx);
	  }
	  else if (event->state & GDK_SHIFT_MASK)
	  {
	      zlist_extend_selection (list, idx);
	  }
	  else
	  {
	      list->anchor = idx;

	      zlist_unselect_all (list);
	      zlist_cell_select (list, idx);
	  }
	  break;

      default:
	  break;
    }

    if (!GTK_WIDGET_HAS_FOCUS (list))
	gtk_widget_grab_focus (widget);

    if (gdk_pointer_grab (widget->window, TRUE,
			  GDK_POINTER_MOTION_HINT_MASK |
			  GDK_BUTTON1_MOTION_MASK |
			  GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time))
	return FALSE;

    gtk_grab_add (widget);

    return FALSE;
}

static  gint
zlist_button_release (GtkWidget * widget, GdkEventButton * event)
{
    ZList  *list;
    gpointer *cell;
    int     index;

    list = ZLIST (widget);

    if (GTK_WIDGET_HAS_GRAB (widget))
	gtk_grab_remove (widget);

    if (gdk_pointer_is_grabbed ())
	gdk_pointer_ungrab (event->time);

    index = zlist_cell_index_from_xy (list, event->x, event->y);
    if (index < 0)
	return FALSE;

    cell = ZLIST_CELL_FROM_INDEX (list, index);

    switch (list->selection_mode)
    {
      case GTK_SELECTION_SINGLE:
	  if (list->anchor == index)
	  {
	      list->focus = index;
	      zlist_unselect_all (list);
	      zlist_cell_toggle (list, index);
	  }
	  list->anchor = index;
	  break;

      case GTK_SELECTION_EXTENDED:
	  if ((list->flags & ZLIST_USES_DND))
	  {
	      int     x, y;

	      /*
	       * on a drag-drop event, the event->x & event->y always seem to be 0 
	       */
	      gdk_window_get_pointer (widget->window, &x, &y, NULL);
	      index = zlist_cell_index_from_xy (list, x, y);
	      if (index < 0)
		  return FALSE;
	      /*
	       * if (list->anchor == index)
	       * zlist_extend_selection (list, list->focus);
	       */
	  }

	  list->anchor = list->focus;
	  break;

      default:
	  break;
    }

    return FALSE;
}

static  gint
zlist_motion_notify (GtkWidget * widget, GdkEventMotion * event)
{
    ZList  *list;
    gpointer *cell;
    int     index, x, y;

    list = ZLIST (widget);
    if (list->flags & ZLIST_USES_DND)
	return FALSE;

    gdk_window_get_pointer (widget->window, &x, &y, NULL);

    index = zlist_cell_index_from_xy (list, x, y);
    if (index < 0)
	return FALSE;


    cell = ZLIST_CELL_FROM_INDEX (list, index);

    if (gdk_pointer_is_grabbed () && GTK_WIDGET_HAS_GRAB (widget))
    {
	if (index == list->focus)
	    return FALSE;

	zlist_cell_draw_default (list, list->focus);
	list->focus = index;

	switch (list->selection_mode)
	{
	  case GTK_SELECTION_SINGLE:
	      zlist_cell_draw_focus (list, index);
	      break;

	  case GTK_SELECTION_BROWSE:
	      zlist_unselect_all (list);
	      zlist_cell_select (list, index);
	      break;

	  case GTK_SELECTION_EXTENDED:
	      if (event->state & GDK_CONTROL_MASK)
	      {
		  list->anchor = index;
		  zlist_cell_toggle (list, index);
	      }
	      else
	      {
		  zlist_extend_selection (list, index);
	      }
	      break;

	  default:
	      break;
	}
    }
    else
    {

    }

    return FALSE;
}

/* 
 * GtkContainers doesn't seem to forward the focus movement to
 * containers which don't have child widgets
 */

static  gint
zlist_key_press (GtkWidget * widget, GdkEventKey * event)
{
    int     direction = -1;

    g_return_val_if_fail (widget && event, FALSE);

    if (GTK_WIDGET_HAS_FOCUS (widget))
    {
	switch (event->keyval)
	{
	  case GDK_Up:
	      direction = GTK_DIR_UP;
	      break;
	  case GDK_Down:
	      direction = GTK_DIR_DOWN;
	      break;
	  case GDK_Left:
	      direction = GTK_DIR_LEFT;
	      break;
	  case GDK_Right:
	      direction = GTK_DIR_RIGHT;
	      break;
	      /*
	       * case GDK_Tab:
	       * case GDK_ISO_Left_Tab:
	       * if (event->state & GDK_SHIFT_MASK)
	       * direction = GTK_DIR_TAB_BACKWARD;
	       * else
	       * direction = GTK_DIR_TAB_FORWARD;
	       * break;
	       */
	  default:
	      break;
	}
    }

    if (direction != -1)
    {
	zlist_focus (GTK_CONTAINER (widget), direction);
	return TRUE;
    }

    if (parent_class->key_press_event &&
	(*parent_class->key_press_event) (widget, event))
	return TRUE;

    return FALSE;
}

static  gint
zlist_focus_in (GtkWidget * widget, GdkEventFocus * event)
{
    GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

    return FALSE;
}

static  gint
zlist_focus_out (GtkWidget * widget, GdkEventFocus * event)
{
    GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

    return FALSE;
}

static  gint
zlist_drag_motion (GtkWidget * widget,
		   GdkDragContext * context, gint x, gint y, guint time)
{
    ZList  *list;
    int     index;

    g_return_val_if_fail (widget, FALSE);

    list = ZLIST (widget);

    if (!(list->flags & ZLIST_USES_DND))
	return FALSE;

    if (!(list->flags & ZLIST_HIGHLIGHTED))
    {
	list->flags |= ZLIST_HIGHLIGHTED;
	zlist_highlight (widget);
    }

    index = zlist_cell_index_from_xy (ZLIST (widget), x, y);
    if (index < 0)
	return FALSE;
    /*
     * if (g_list_find (ZLIST(widget)->selection, GUINT_TO_POINTER(index)))
     * return TRUE;
     */
    return FALSE;
}

static  gint
zlist_drag_drop (GtkWidget * widget,
		 GdkDragContext * context, gint x, gint y, guint time)
{
    ZList  *list;

    list = ZLIST (widget);

    if (!(list->flags & ZLIST_USES_DND))
	return FALSE;

    return FALSE;
}

static void
zlist_drag_leave (GtkWidget * widget, GdkDragContext * context, guint time)
{
    ZList  *list;

    list = ZLIST (widget);
    if (list->flags & ZLIST_HIGHLIGHTED)
    {
	list->flags &= ~ZLIST_HIGHLIGHTED;
	zlist_unhighlight (widget);
    }
}

static void
zlist_highlight (GtkWidget * widget)
{
    gtk_draw_shadow (widget->style,
		     widget->window,
		     GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		     0, 0,
		     widget->allocation.width - 2 * bw (widget),
		     widget->allocation.height - 2 * bw (widget));

    gdk_draw_rectangle (widget->window,
			widget->style->black_gc,
			FALSE,
			0, 0,
			widget->allocation.width - 2 * bw (widget) - 1,
			widget->allocation.height - 2 * bw (widget) - 1);
}

static void
zlist_unhighlight (GtkWidget * widget)
{
    GdkRectangle area;

    area.x = 0;
    area.y = 0;
    area.width = HIGHLIGHT_SIZE;
    area.height = widget->allocation.height - 2 * bw (widget);
    gtk_widget_draw (widget, &area);

    area.width = widget->allocation.width - 2 * bw (widget);
    area.height = HIGHLIGHT_SIZE;
    gtk_widget_draw (widget, &area);

    area.y = widget->allocation.height - 2 * bw (widget) - HIGHLIGHT_SIZE;
    gtk_widget_draw (widget, &area);

    area.x = widget->allocation.width - 2 * bw (widget) - HIGHLIGHT_SIZE;
    area.y = 0;
    area.width = HIGHLIGHT_SIZE;
    area.height = widget->allocation.height - 2 * bw (widget);
    gtk_widget_draw (widget, &area);
}

static void
zlist_forall (GtkContainer * container,
	      gboolean include_internals,
	      GtkCallback callback, gpointer callback_data)
{

    /*
     * ZList *list;
     * int i;
     * 
     * list = ZLIST(container);
     * 
     * if (include_internals)
     * for (i = 0; i < list->cell_count; i++)
     * (* callback) (ZLIST_CELL_FROM_INDEX (list, i), callback_data);
     */
}

static  gint
zlist_focus (GtkContainer * container, GtkDirectionType dir)
{
    ZList  *list;
    int     focus;

    list = ZLIST (container);

    if (list->focus < 0)
	return FALSE;

    focus = list->focus;
    switch (dir)
    {
      case GTK_DIR_LEFT:
	  focus -= list->flags & ZLIST_HORIZONTAL ? list->rows : 1;
	  break;
      case GTK_DIR_RIGHT:
	  focus += list->flags & ZLIST_HORIZONTAL ? list->rows : 1;
	  break;
      case GTK_DIR_UP:
      case GTK_DIR_TAB_BACKWARD:
	  focus -= list->flags & ZLIST_HORIZONTAL ? 1 : list->columns;
	  break;
      case GTK_DIR_DOWN:
      case GTK_DIR_TAB_FORWARD:
	  focus += list->flags & ZLIST_HORIZONTAL ? 1 : list->columns;
	  break;
      default:
	  return FALSE;
	  break;
    }

    if (focus < 0 || focus >= list->cell_count)
	return FALSE;

    zlist_cell_draw_default (list, list->focus);
    list->focus = focus;
    zlist_cell_draw_focus (list, focus);
    zlist_moveto (list, focus);
/* select */
    zlist_unselect_all (list);
    zlist_cell_select (list, focus);


    return TRUE;
}

guint
zlist_add (ZList * list, gpointer cell)
{
    GtkRequisition requisition = { 0 };
    int     adjust = FALSE;
    guint   retval;

    list->cells = g_array_append_val (list->cells, cell);
    retval = list->cells->len - 1;

    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_SIZE_REQUEST],
		     cell, &requisition);

    if (list->flags & ZLIST_HORIZONTAL)
    {
	if (list->cell_count && list->cell_count % list->rows == 0)
	{
	    list->columns++;
	    adjust = TRUE;
	}
    }
    else
    {
	if (list->cell_count && list->cell_count % list->columns == 0)
	{
	    list->rows++;
	    adjust = TRUE;
	}
    }

    list->cell_count++;

    if (requisition.width + list->cell_x_pad > list->cell_width ||
	requisition.height + list->cell_y_pad > list->cell_height)
    {
	list->cell_width =
	    MAX (list->cell_width, requisition.width + list->cell_x_pad);
	list->cell_height =
	    MAX (list->cell_height, requisition.height + list->cell_y_pad);

	zlist_update (list);
	gtk_widget_draw (GTK_WIDGET (list), NULL);

    }
    else
    {

	if (adjust)
	    zlist_adjust_adjustments (SCROLLED (list));

	zlist_draw_cell (list, list->cell_count - 1);
    }

/* select */
/*
    if (list->cell_count == 1)
    {
	zlist_cell_toggle (list, 0);
	list->focus = 0;
	list->anchor = 0;
	zlist_cell_draw_focus (list, 0);
    }
*/

    return retval;
}

void
zlist_remove (ZList * list, gpointer cell)
{
    GdkRectangle area;
    GList  *item;
    int     index, *i;

    index = zlist_cell_index (list, cell);
    if (index == -1)
	return;

    list->cells = g_array_remove_index (list->cells, index);
    list->cell_count--;

    list->selection =
	g_list_remove (list->selection, GUINT_TO_POINTER (index));
    item = list->selection;
    while (item)
    {
	i = (guint *) & item->data;
	if (*i > index)
	    *i -= 1;
	item = item->next;
    }

    if (list->focus == index)
	list->focus = -1;
    else if (list->focus > index)
	list->focus--;

    if (list->anchor == index)
	list->anchor = -1;
    else if (list->anchor > index)
	list->anchor--;

    if ((list->flags & ZLIST_HORIZONTAL)
	&& list->cell_count % list->rows == 0)
    {
	list->columns--;
	zlist_adjust_adjustments (SCROLLED (list));
    }
    else if (!(list->flags & ZLIST_HORIZONTAL)
	     && list->cell_count % list->columns == 0)
    {
	list->rows--;
	zlist_adjust_adjustments (SCROLLED (list));
    }

    zlist_cell_area (list, index, &area);

    /*
     * using gdk_window_copy_area will be faster but harder too 
     */
    area.width = GTK_WIDGET (list)->allocation.width - area.x;
    area.height = GTK_WIDGET (list)->allocation.height - area.y;
    zlist_draw (GTK_WIDGET (list), &area);

    if (list->flags & ZLIST_HORIZONTAL)
    {
	area.width -= list->cell_width;
	area.height = area.y;
	area.x += list->cell_width;
	area.y = 0;
    }
    else
    {
	area.width = area.x;
	area.height -= list->cell_height;
	area.x = 0;
	area.y += list->cell_height;
    }

    gtk_widget_draw (GTK_WIDGET (list), &area);
}

static void
zlist_adjust_adjustments (Scrolled * scrolled)
{
    ZList  *list;
    GtkWidget *widget;
    GtkAdjustment *adj;

    list = ZLIST (scrolled);
    widget = GTK_WIDGET (scrolled);

    if (!GTK_WIDGET_DRAWABLE (widget) || scrolled->freeze_count)
	return;

    if (scrolled->h_adjustment)
    {
	adj = scrolled->h_adjustment;

	adj->page_size = widget->allocation.width - 2 * bw (widget);
	adj->page_increment = adj->page_size;
	adj->step_increment = list->cell_width;
	adj->lower = 0;
	adj->upper = LIST_WIDTH (list) + 2 * list->x_pad;

	gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");
    }

    if (scrolled->v_adjustment)
    {
	adj = scrolled->v_adjustment;

	adj->page_size = widget->allocation.height - 2 * bw (widget);
	adj->page_increment = adj->page_size;
	adj->step_increment = list->cell_height;
	adj->lower = 0;
	adj->upper = LIST_HEIGHT (list) + 2 * list->y_pad;

	gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");
    }
}

void
zlist_clear (ZList * list)
{
    Scrolled *scrolled;

    g_return_if_fail (list);

    zlist_unselect_all (list);

    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CLEAR]);

    g_array_set_size (list->cells, 0);
    list->cell_count = 0;
    list->focus = -1;
    list->anchor = -1;
    list->entered_cell = NULL;

    if (list->flags & ZLIST_HORIZONTAL)
	list->columns = 1;
    else
	list->rows = 1;

    scrolled = SCROLLED (list);
    zlist_adjust_adjustments (SCROLLED (list));
    scrolled->h_adjustment->value = 0;
    scrolled->v_adjustment->value = 0;
    gtk_signal_emit_by_name (GTK_OBJECT (scrolled->h_adjustment),
			     "value_changed");
    gtk_signal_emit_by_name (GTK_OBJECT (scrolled->v_adjustment),
			     "value_changed");

    zlist_draw (GTK_WIDGET (list), NULL);
}

void
zlist_set_cell_padding (ZList * list, int x_pad, int y_pad)
{
    g_return_if_fail (list);
    g_return_if_fail (x_pad >= 0 && y_pad >= 0);

    list->cell_width += x_pad - list->cell_x_pad;
    list->cell_height += y_pad - list->cell_y_pad;

    list->cell_x_pad = x_pad;
    list->cell_y_pad = y_pad;

    zlist_update (list);
    gtk_widget_draw (GTK_WIDGET (list), NULL);
}

void
zlist_set_cell_size (ZList * list, gint width, gint height)
{
    g_return_if_fail (list);

    if (width > 0)
	list->cell_width = width + list->cell_x_pad;

    if (height > 0)
	list->cell_height = height + list->cell_y_pad;

    zlist_update (list);
    zlist_draw (GTK_WIDGET (list), NULL);
}

void
zlist_set_selection_mode (ZList * list, GtkSelectionMode mode)
{
    g_return_if_fail (list);

    list->selection_mode = mode;
    zlist_unselect_all (list);
    gtk_widget_draw (GTK_WIDGET (list), NULL);
}

int
zlist_cell_index_from_xy (ZList * list, gint x, gint y)
{
    Scrolled *scr;
    gint    row, column, index;

    scr = SCROLLED (list);

    if (!list->cell_count || x < list->x_pad || y < list->y_pad)
	return -1;

    row = CELL_ROW_FROM_Y (list, y);
    column = CELL_COL_FROM_X (list, x);

    if (row >= list->rows || column >= list->columns)
	return -1;

    index =
	list->flags & ZLIST_HORIZONTAL ? column * list->rows +
	row : row * list->columns + column;

    return index < list->cell_count ? index : -1;
}

void
zlist_set_1 (ZList * list, int one)
{
    g_return_if_fail (list);

    if (one)
	list->flags |= ZLIST_1;
    else
	list->flags &= ~ZLIST_1;

    zlist_update (list);
    gtk_widget_draw (GTK_WIDGET (list), NULL);
}

gpointer
zlist_cell_from_xy (ZList * list, int x, int y)
{
    int     index;

    index = zlist_cell_index_from_xy (list, x, y);

    return index < 0 ? NULL : ZLIST_CELL_FROM_INDEX (list, index);
}

static void
zlist_cell_pos (ZList * list, int index, int *row, int *col)
{
    g_return_if_fail (list && index != -1 && row && col);

    if (list->flags & ZLIST_HORIZONTAL)
    {
	*row = index % list->rows;
	*col = index / list->rows;
    }
    else
    {
	*row = index / list->columns;
	*col = index % list->columns;
    }
}

static void
zlist_cell_area (ZList * list, int index, GdkRectangle * cell_area)
{
    int     row, col;
    g_return_if_fail (list && index != -1 && cell_area);

    zlist_cell_pos (list, index, &row, &col);

    cell_area->x = CELL_X_FROM_COL (list, col) + list->cell_x_pad;
    cell_area->y = CELL_Y_FROM_ROW (list, row) + list->cell_y_pad;
    cell_area->width = list->cell_width - list->cell_x_pad;
    cell_area->height = list->cell_height - list->cell_y_pad;
}

void
zlist_draw_cell (ZList * list, int index)
{
    GtkWidget *cell;
    GdkRectangle cell_area;

    g_return_if_fail (list && index != -1);

    if (!GTK_WIDGET_DRAWABLE (list))
	return;

    zlist_cell_area (list, index, &cell_area);
    cell = ZLIST_CELL_FROM_INDEX (list, index);

    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_DRAW], cell,
		     &cell_area, &cell_area);

    if (index == list->focus)
	zlist_cell_draw_focus (list, index);
}

static int
zlist_cell_index (ZList * list, gpointer cell)
{
    int     i;
    g_return_val_if_fail (list && cell, -1);

    for (i = 0; i < list->cell_count; i++)
	if (ZLIST_CELL_FROM_INDEX (list, i) == cell)
	    return i;

    return -1;
}

int
zlist_update_cell_size (ZList * list, gpointer cell)
{
    GtkRequisition requisition;

    g_return_val_if_fail (list && cell, FALSE);

    if (list->flags & ZLIST_1)
	return FALSE;

    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_SIZE_REQUEST],
		     cell, &requisition);
    if (requisition.width + list->cell_x_pad > list->cell_width
	|| requisition.height + list->cell_y_pad > list->cell_height)
    {
	list->cell_width =
	    MAX (list->cell_width, requisition.width + list->cell_x_pad);
	list->cell_height =
	    MAX (list->cell_height, requisition.height + list->cell_y_pad);

	zlist_update (list);
	gtk_widget_draw (GTK_WIDGET (list), NULL);

	return TRUE;
    }

    return FALSE;
}

/*
 * Focus & Selection handling
 * 
 */

static void
zlist_cell_draw_focus (ZList * list, int index)
{
    GdkRectangle cell_area;
    g_return_if_fail (list && index != -1);

    zlist_cell_area (list, index, &cell_area);
    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_DRAW_FOCUS],
		     ZLIST_CELL_FROM_INDEX (list, index), &cell_area);
}


static void
zlist_cell_draw_default (ZList * list, int index)
{
    GdkRectangle cell_area;
    g_return_if_fail (list && index != -1);

    zlist_cell_area (list, index, &cell_area);
    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_DRAW_DEFAULT],
		     ZLIST_CELL_FROM_INDEX (list, index), &cell_area);
}

static void
zlist_cell_select (ZList * list, int index)
{
    g_return_if_fail (list && index != -1);

    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_SELECT],
		     ZLIST_CELL_FROM_INDEX (list, index));
    list->selection =
	g_list_prepend (list->selection, GUINT_TO_POINTER (index));
    zlist_draw_cell (list, index);
}

static void
zlist_cell_unselect (ZList * list, int index)
{
    g_return_if_fail (list && index != -1);

    gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_UNSELECT],
		     ZLIST_CELL_FROM_INDEX (list, index));
    list->selection =
	g_list_remove (list->selection, GUINT_TO_POINTER (index));
    zlist_draw_cell (list, index);
}

static void
zlist_cell_toggle (ZList * list, int index)
{
    g_return_if_fail (list && index != -1);

    if (g_list_find (list->selection, GUINT_TO_POINTER (index)))
	zlist_cell_unselect (list, index);
    else
	zlist_cell_select (list, index);
}

static void
zlist_unselect_all (ZList * list)
{
    GList  *item;

    item = list->selection;
    while (item)
    {
	gtk_signal_emit (GTK_OBJECT (list), zlist_signals[CELL_UNSELECT],
			 ZLIST_CELL_FROM_INDEX (list,
						GPOINTER_TO_UINT (item->
								  data)));
	zlist_draw_cell (list, GPOINTER_TO_UINT (item->data));

	item = item->next;
    }

    g_list_free (list->selection);
    list->selection = NULL;
}

static void
zlist_extend_selection (ZList * list, int to)
{
    GList  *item;
    int     i, s, e;
    g_return_if_fail (list && to != -1);

    if (list->anchor < to)
    {
	s = list->anchor;
	e = to;
    }
    else
    {
	s = to;
	e = list->anchor;
    }

    for (i = s; i <= e; i++)
	/*
	 * XXX the state of the cell should be cached in the cells array 
	 */
	if (!g_list_find (list->selection, GUINT_TO_POINTER (i)))
	    zlist_cell_select (list, i);
	else if (i == list->focus)
	    zlist_cell_draw_focus (list, i);

    item = list->selection;
    while (item)
    {
	i = GPOINTER_TO_UINT (item->data);
	item = item->next;

	if (i < s || i > e)
	    zlist_cell_unselect (list, i);
    }
}

static void
zlist_moveto (ZList * list, int index)
{
    GdkRectangle cell_area;
    GtkAdjustment *adj;

    g_return_if_fail (list && index != -1);

    if (!GTK_WIDGET_DRAWABLE (list) || SCROLLED (list)->freeze_count)
	return;

    zlist_cell_area (list, index, &cell_area);

    if (list->flags & ZLIST_HORIZONTAL)
    {

	adj = SCROLLED (list)->h_adjustment;

	if (cell_area.x < 0)
	{
	    adj->value += cell_area.x;
	    gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
	}

	if (cell_area.x + cell_area.width >
	    GTK_WIDGET (list)->allocation.width)
	{
	    adj->value += (cell_area.x - adj->page_size) + cell_area.width;
	    gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
	}

    }
    else
    {				/* !ZLIST_HORIZONTAL */

	adj = SCROLLED (list)->v_adjustment;

	if (cell_area.y < 0)
	{
	    adj->value += cell_area.y;
	    gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
	}

	if (cell_area.y + cell_area.height >
	    GTK_WIDGET (list)->allocation.height)
	{
	    adj->value += (cell_area.y - adj->page_size) + cell_area.height;
	    gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
	}
    }
}

void
zlist_cell_focus (ZList * list, gint index)
{
    if ((g_list_length (list->selection) == 1) && list->focus == index)
	return;

    list->focus = index;
    list->anchor = index;

    zlist_unselect_all (list);
    zlist_cell_select (list, index);
    zlist_cell_draw_focus (list, index);
    zlist_moveto (list, index);
}

void
zlist_select_all (ZList * list)
{
    gint    i;

    g_list_free (list->selection);
    list->selection = NULL;

    for (i = 0; i < list->cell_count; i++)
	zlist_cell_select (list, i);

    list->anchor = list->focus = list->cell_count - 1;

    zlist_cell_draw_focus (list, list->focus);
}
