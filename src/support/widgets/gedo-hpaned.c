/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef USE_NORMAL_PANED

#include "gedo-hpaned.h"
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

static void gedo_hpaned_class_init (GedoHPanedClass * klass);
static void gedo_hpaned_init (GedoHPaned * hpaned);
static void gedo_hpaned_size_request (GtkWidget * widget,
				      GtkRequisition * requisition);
static void gedo_hpaned_size_allocate (GtkWidget * widget,
				       GtkAllocation * allocation);
static void gedo_hpaned_draw (GtkWidget * widget, GdkRectangle * area);
static void gedo_hpaned_xor_line (GedoPaned * paned);
static gint gedo_hpaned_button_press (GtkWidget * widget,
				      GdkEventButton * event);
static gint gedo_hpaned_button_release (GtkWidget * widget,
					GdkEventButton * event);

guint
gedo_hpaned_get_type (void)
{
    static guint hpaned_type = 0;

    if (!hpaned_type)
    {
	static const GtkTypeInfo hpaned_info = {
	    "GedoHPaned",
	    sizeof (GedoHPaned),
	    sizeof (GedoHPanedClass),
	    (GtkClassInitFunc) gedo_hpaned_class_init,
	    (GtkObjectInitFunc) gedo_hpaned_init,
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
	hpaned_type = gtk_type_unique (gedo_paned_get_type (), &hpaned_info);
    }
    return hpaned_type;
}

static void
gedo_hpaned_class_init (GedoHPanedClass * class)
{
    GtkWidgetClass *widget_class;
    GedoPanedClass *paned_class;

    widget_class = (GtkWidgetClass *) class;
    paned_class = (GedoPanedClass *) class;

    widget_class->size_request = gedo_hpaned_size_request;
    widget_class->size_allocate = gedo_hpaned_size_allocate;

#ifndef USE_GTK2
    widget_class->draw = gedo_hpaned_draw;
#endif

    widget_class->button_press_event = gedo_hpaned_button_press;
    widget_class->button_release_event = gedo_hpaned_button_release;

    paned_class->xor_line = gedo_hpaned_xor_line;
}

static void
gedo_hpaned_init (GedoHPaned * hpaned)
{
    GEDO_PANED (hpaned)->horizontal = TRUE;
}

GtkWidget *
gedo_hpaned_new (void)
{
    GedoHPaned *hpaned;

    hpaned = gtk_type_new (gedo_hpaned_get_type ());

    return GTK_WIDGET (hpaned);
}

static void
gedo_hpaned_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
    GedoPaned *paned;
    GtkRequisition child_requisition;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GEDO_IS_HPANED (widget));
    g_return_if_fail (requisition != NULL);

    paned = GEDO_PANED (widget);
    requisition->width = 0;
    requisition->height = 0;

    if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1))
    {
	gtk_widget_size_request (paned->child1, &child_requisition);

	requisition->height = child_requisition.height;
	requisition->width = child_requisition.width;
    }

    if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
    {
	gtk_widget_size_request (paned->child2, &child_requisition);

	requisition->height = MAX (requisition->height,
				   child_requisition.height);
	requisition->width += child_requisition.width;
    }

    requisition->width += (GTK_CONTAINER (paned)->border_width * 2
			   + paned->gutter_size);
    requisition->height += GTK_CONTAINER (paned)->border_width * 2;
}

static void
gedo_hpaned_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
    GedoPaned *paned;
    GtkRequisition child1_requisition;
    GtkRequisition child2_requisition;
    GtkAllocation child1_allocation;
    GtkAllocation child2_allocation;
    gint    border_width, gutter_size;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GEDO_IS_HPANED (widget));
    g_return_if_fail (allocation != NULL);

    widget->allocation = *allocation;
    paned = GEDO_PANED (widget);
    border_width = GTK_CONTAINER (paned)->border_width;
    gutter_size = paned->gutter_size;

    if (paned->child1)
	gtk_widget_get_child_requisition (paned->child1, &child1_requisition);
    else
	child1_requisition.width = 0;

    if (paned->child2)
	gtk_widget_get_child_requisition (paned->child2, &child2_requisition);
    else
	child2_requisition.width = 0;

    gedo_paned_compute_position (paned,
				 MAX (1, (gint) widget->allocation.width
				      - gutter_size
				      - 2 * border_width),
				 child1_requisition.width,
				 child2_requisition.width);

    if (paned->child_hidden != 0)
    {
	gutter_size = 0;
	if ((paned->child_hidden == 1) && paned->child1)
	{
	    /*
	     * hide child1 and show child2 if it exists. 
	     */
	    gtk_widget_hide (paned->child1);
	    if (paned->child2 && !GTK_WIDGET_VISIBLE (paned->child2))
		gtk_widget_show (paned->child2);
	}
	if ((paned->child_hidden == 2) && paned->child2)
	{
	    /*
	     * hide child2 and show child1 if it exists. 
	     */
	    gtk_widget_hide (paned->child2);
	    if (paned->child1 && !GTK_WIDGET_VISIBLE (paned->child1))
		gtk_widget_show (paned->child1);
	}
    }
    else
    {
	/*
	 * Show both children. 
	 */
	if (paned->child1 && !GTK_WIDGET_VISIBLE (paned->child1))
	    gtk_widget_show (paned->child1);
	if (paned->child2 && !GTK_WIDGET_VISIBLE (paned->child2))
	    gtk_widget_show (paned->child2);
    }

    /*
     * Move the handle before the children so we don't get extra expose 
     * events 
     */

    paned->handle_ypos = border_width;
    paned->handle_xpos = paned->child1_size + border_width;

    if (GTK_WIDGET_REALIZED (widget))
    {
	gdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);

	if (paned->child_hidden == 0)
	{
	    gdk_window_move_resize (paned->handle,
				    paned->handle_xpos,
				    paned->handle_ypos,
				    paned->gutter_size, allocation->height);
	    gdk_window_show (paned->handle);
	}
	else
	    gdk_window_hide (paned->handle);
    }

    child1_allocation.height = child2_allocation.height
	= MAX (1, (gint) allocation->height - border_width * 2);
    child1_allocation.width = paned->child1_size;
    child1_allocation.y = child2_allocation.y = border_width;
    child1_allocation.x = border_width;

    child2_allocation.x
	= child1_allocation.x + child1_allocation.width + gutter_size;
    child2_allocation.width
	=
	MAX (1,
	     (gint) allocation->width - child2_allocation.x - border_width);

    /*
     * Now allocate the childen, making sure, when resizing not to
     * overlap the windows 
     */
    if (GTK_WIDGET_MAPPED (widget) &&
	paned->child1 && GTK_WIDGET_VISIBLE (paned->child1) &&
	paned->child1->allocation.width < child1_allocation.width)
    {
	if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
	    gtk_widget_size_allocate (paned->child2, &child2_allocation);
	gtk_widget_size_allocate (paned->child1, &child1_allocation);
    }
    else
    {
	if (paned->child1 && GTK_WIDGET_VISIBLE (paned->child1))
	    gtk_widget_size_allocate (paned->child1, &child1_allocation);
	if (paned->child2 && GTK_WIDGET_VISIBLE (paned->child2))
	    gtk_widget_size_allocate (paned->child2, &child2_allocation);
    }
}

static void
gedo_hpaned_draw (GtkWidget * widget, GdkRectangle * area)
{
    GedoPaned *paned;
    GdkRectangle handle_area, child_area;
    guint16 border_width;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GEDO_IS_PANED (widget));

    if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
	gint    width, height;

	paned = GEDO_PANED (widget);
	border_width = GTK_CONTAINER (paned)->border_width;

	gdk_window_clear_area (widget->window,
			       area->x, area->y, area->width, area->height);

	gdk_window_get_size (paned->handle, &width, &height);

	handle_area.x = paned->handle_xpos;
	handle_area.y = paned->handle_ypos;
	handle_area.width = width;
	handle_area.height = height;

	if (gdk_rectangle_intersect (&handle_area, area, &child_area))
	{
	    child_area.x -= handle_area.x;
	    child_area.y -= handle_area.y;

	    gtk_paint_flat_box (widget->style, paned->handle,
				GTK_WIDGET_STATE (widget),
				GTK_SHADOW_NONE,
				&child_area, widget, "paned",
				0, 0, width, height);
	}

	/*
	 * Redraw the children
	 */
	if (paned->child1 &&
	    gtk_widget_intersect (paned->child1, area, &child_area))
	    gtk_widget_draw (paned->child1, &child_area);
	if (paned->child2 &&
	    gtk_widget_intersect (paned->child2, area, &child_area))
	    gtk_widget_draw (paned->child2, &child_area);
    }
}

static void
gedo_hpaned_xor_line (GedoPaned * paned)
{
    GtkWidget *widget;
    GdkGCValues values;
    guint16 xpos;

    widget = GTK_WIDGET (paned);

    if (!paned->xor_gc)
    {
	GdkBitmap *stipple;

	stipple = gdk_bitmap_create_from_data (NULL, gray50_bits,
					       gray50_width, gray50_height);

	values.function = GDK_INVERT;
	values.subwindow_mode = GDK_INCLUDE_INFERIORS;
	values.fill = GDK_STIPPLED;
	values.stipple = stipple;
	paned->xor_gc = gdk_gc_new_with_values (widget->window,
						&values,
						GDK_GC_FUNCTION |
						GDK_GC_SUBWINDOW |
						GDK_GC_FILL | GDK_GC_STIPPLE);
	gdk_bitmap_unref (stipple);
    }

    xpos = paned->child1_size + GTK_CONTAINER (paned)->border_width;

    gdk_draw_rectangle (widget->window, paned->xor_gc,
			TRUE,
			xpos,
			0, paned->gutter_size, widget->allocation.height - 1);
}

static  gint
gedo_hpaned_button_press (GtkWidget * widget, GdkEventButton * event)
{
    GedoPaned *paned;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GEDO_IS_PANED (widget), FALSE);

    paned = GEDO_PANED (widget);

    if (!paned->in_drag &&
	(event->window == paned->handle) && (event->button == 1))
    {
	paned->in_drag = TRUE;
	/*
	 * We need a server grab here, not gtk_grab_add(), since
	 * we don't want to pass events on to the widget's children 
	 */
	gdk_pointer_grab (paned->handle, FALSE,
			  GDK_POINTER_MOTION_HINT_MASK
			  | GDK_BUTTON1_MOTION_MASK
			  | GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);
	paned->child1_size += event->x - paned->gutter_size / 2;
	paned->child1_size = CLAMP (paned->child1_size, 0,
				    widget->allocation.width
				    - paned->gutter_size
				    -
				    2 * GTK_CONTAINER (paned)->border_width);
	gedo_hpaned_xor_line (paned);
    }

    return TRUE;
}

static  gint
gedo_hpaned_button_release (GtkWidget * widget, GdkEventButton * event)
{
    GedoPaned *paned;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GEDO_IS_PANED (widget), FALSE);

    paned = GEDO_PANED (widget);

    if (paned->in_drag && (event->button == 1))
    {
	gedo_hpaned_xor_line (paned);
	paned->in_drag = FALSE;
	paned->position_set = TRUE;
	gdk_pointer_ungrab (event->time);
	gtk_widget_queue_resize (GTK_WIDGET (paned));
    }

    return TRUE;
}

#endif /* USE_NORMAL_PANED */
