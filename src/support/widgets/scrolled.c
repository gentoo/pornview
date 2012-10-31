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

#include "gtk2-compat.h"
#include "scrolled.h"

#define SCROLL_TIMEOUT 100
/*
#define WIDGET_DRAW(widget, area)   (*((GtkWidgetClass *) GTK_OBJECT(widget)->klass)->draw) (widget, area)
*/

#define bw(widget)                  GTK_CONTAINER(widget)->border_width

#ifdef USE_GTK2
#define WIDGET_DRAW(widget, area) gdk_window_invalidate_rect (widget->window, area, TRUE)
#else
#define WIDGET_DRAW(widget, area) (gtk_signal_emit_by_name (GTK_OBJECT(widget), "draw", area))
#endif

enum
{
    ADJUST_ADJUSTMENTS,
    LAST_SIGNAL
};

static void scrolled_class_init (ScrolledClass * klass);
static void scrolled_init (Scrolled * scrolled);
static void scrolled_set_scroll_adjustments (GtkWidget * widget,
					     GtkAdjustment * hadjustment,
					     GtkAdjustment * vadjustment);
/*
static gint     scrolled_button_press (GtkWidget *widget,GdkEventButton *event);
static gint     scrolled_button_release (GtkWidget *widget,GdkEventButton *event);
static gint     scrolled_motion (GtkWidget *widget,GdkEventMotion *event);
*/

static void hadjustment_value_changed (GtkAdjustment * hadjustment,
				       gpointer data);
static void vadjustment_value_changed (GtkAdjustment * vadjustment,
				       gpointer data);
/*
static int      horizontal_timeout (Scrolled *scrolled);
static int      vertical_timeout (Scrolled *scrolled);
*/
static void check_exposures (GtkWidget * widget);

static guint scrolled_signals[LAST_SIGNAL] = { 0 };

GtkType
scrolled_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
	static const GtkTypeInfo type_info = {
	    "Scrolled",
	    sizeof (Scrolled),
	    sizeof (ScrolledClass),
	    (GtkClassInitFunc) scrolled_class_init,
	    (GtkObjectInitFunc) scrolled_init,
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

	type = gtk_type_unique (GTK_TYPE_CONTAINER, &type_info);
    }

    return type;
}

static void
scrolled_class_init (ScrolledClass * klass)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    widget_class->set_scroll_adjustments_signal =
	gtk_signal_new ("set_scroll_adjustments",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ScrolledClass,
					   set_scroll_adjustments),
			gtk_marshal_NONE__POINTER_POINTER, GTK_TYPE_NONE, 2,
			GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

    scrolled_signals[ADJUST_ADJUSTMENTS] =
	gtk_signal_new ("adjust_adjustments",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (ScrolledClass, adjust_adjustments),
			gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

    klass->set_scroll_adjustments = scrolled_set_scroll_adjustments;
    klass->adjust_adjustments = NULL;
}

static void
scrolled_init (Scrolled * scrolled)
{
    scrolled->x_offset = 0;
    scrolled->y_offset = 0;
    scrolled->h_adjustment = NULL;
    scrolled->v_adjustment = NULL;
    scrolled->freeze_count = 0;
    scrolled->copy_gc = NULL;
}

void
scrolled_realize (Scrolled * scrolled)
{
    g_return_if_fail (scrolled && GTK_WIDGET (scrolled)->window);

    scrolled->copy_gc = gdk_gc_new (GTK_WIDGET (scrolled)->window);
    gdk_gc_set_exposures (scrolled->copy_gc, TRUE);
}

void
scrolled_unrealize (Scrolled * scrolled)
{
    g_return_if_fail (scrolled);

    gdk_gc_destroy (scrolled->copy_gc);
}

static void
hadjustment_value_changed (GtkAdjustment * hadjustment, gpointer data)
{

    Scrolled *scrolled;
    GtkWidget *widget;
    GdkRectangle area;
    int     value, diff;

    widget = GTK_WIDGET (data);
    scrolled = SCROLLED (data);

    if (!GTK_WIDGET_DRAWABLE (widget)
	|| scrolled->x_offset == hadjustment->value)
	return;

    value = hadjustment->value;

    if (scrolled->freeze_count)
    {
	scrolled->x_offset = value;
	return;
    }

    if (value > scrolled->x_offset)
    {
	diff = value - scrolled->x_offset;

	if (diff >= widget->allocation.width)
	{
	    scrolled->x_offset = value;
	    WIDGET_DRAW (widget, NULL);
	    return;
	}

	if (diff > 0 && !scrolled->freeze_count)
	    gdk_window_copy_area (widget->window,
				  scrolled->copy_gc,
				  0, 0,
				  widget->window,
				  diff, 0,
				  widget->allocation.width - 2 * bw (widget) -
				  diff,
				  widget->allocation.height -
				  2 * bw (widget));

	area.x = widget->allocation.width - 2 * bw (widget) - diff;
	area.y = 0;
	area.width = diff;
	area.height = widget->allocation.height - 2 * bw (widget);

    }
    else
    {
	diff = scrolled->x_offset - value;

	if (diff >= widget->allocation.width)
	{
	    scrolled->x_offset = value;
	    WIDGET_DRAW (widget, NULL);
	    return;
	}

	if (diff > 0)
	    gdk_window_copy_area (widget->window,
				  scrolled->copy_gc,
				  diff, 0,
				  widget->window,
				  0, 0,
				  widget->allocation.width - 2 * bw (widget) -
				  diff,
				  widget->allocation.height -
				  2 * bw (widget));

	area.x = 0;
	area.y = 0;
	area.width = diff;
	area.height = widget->allocation.height - 2 * bw (widget);
    }


    if (diff > 0)
    {
	scrolled->x_offset = value;
	check_exposures (widget);
	WIDGET_DRAW (widget, &area);
    }
}

static void
vadjustment_value_changed (GtkAdjustment * vadjustment, gpointer data)
{

    Scrolled *scrolled;
    GtkWidget *widget;
    GdkRectangle area;
    int     value, diff;

    widget = GTK_WIDGET (data);
    scrolled = SCROLLED (data);

    if (!GTK_WIDGET_DRAWABLE (widget)
	|| scrolled->y_offset == vadjustment->value)
	return;

    value = vadjustment->value;

    if (scrolled->freeze_count)
    {
	scrolled->y_offset = value;
	return;
    }

    if (value > scrolled->y_offset)
    {
	diff = value - scrolled->y_offset;

	if (diff >= widget->allocation.height)
	{
	    scrolled->y_offset = value;
	    WIDGET_DRAW (widget, NULL);
	    return;
	}

	if (diff > 0)
	    gdk_window_copy_area (widget->window,
				  scrolled->copy_gc,
				  0, 0,
				  widget->window,
				  0, diff,
				  widget->allocation.width - 2 * bw (widget),
				  widget->allocation.height -
				  2 * bw (widget) - diff);

	area.x = 0;
	area.y = widget->allocation.height - 2 * bw (widget) - diff;
	area.width = widget->allocation.width - 2 * bw (widget);
	area.height = diff;

    }
    else
    {
	diff = scrolled->y_offset - value;

	if (diff >= widget->allocation.height)
	{
	    scrolled->y_offset = value;
	    WIDGET_DRAW (widget, NULL);
	    return;
	}

	if (diff > 0)
	    gdk_window_copy_area (widget->window,
				  scrolled->copy_gc,
				  0, diff,
				  widget->window,
				  0, 0,
				  widget->allocation.width - 2 * bw (widget),
				  widget->allocation.height -
				  2 * bw (widget) - diff);

	area.x = 0;
	area.y = 0;
	area.width = widget->allocation.width - 2 * bw (widget);
	area.height = diff;
    }


    if (diff > 0)
    {
	scrolled->y_offset = value;
	check_exposures (widget);

	WIDGET_DRAW (widget, &area);
    }
}

static void
scrolled_set_scroll_adjustments (GtkWidget * widget,
				 GtkAdjustment * hadjustment,
				 GtkAdjustment * vadjustment)
{

    Scrolled *scrolled;
    scrolled = SCROLLED (widget);

    if (scrolled->h_adjustment != hadjustment)
    {
	if (scrolled->h_adjustment)
	{
	    gtk_signal_disconnect_by_data (GTK_OBJECT
					   (scrolled->h_adjustment),
					   scrolled);
	    gtk_object_unref (GTK_OBJECT (scrolled->h_adjustment));
	}

	scrolled->h_adjustment = hadjustment;

	if (hadjustment)
	{
	    gtk_object_ref (GTK_OBJECT (hadjustment));
	    gtk_signal_connect (GTK_OBJECT (hadjustment), "value_changed",
				(GtkSignalFunc) hadjustment_value_changed,
				scrolled);
	}
    }


    if (scrolled->v_adjustment != vadjustment)
    {
	if (scrolled->v_adjustment)
	{
	    gtk_signal_disconnect_by_data (GTK_OBJECT
					   (scrolled->v_adjustment),
					   scrolled);
	    gtk_object_unref (GTK_OBJECT (scrolled->v_adjustment));
	}

	scrolled->v_adjustment = vadjustment;

	if (vadjustment)
	{
	    gtk_object_ref (GTK_OBJECT (vadjustment));
	    gtk_signal_connect (GTK_OBJECT (vadjustment), "value_changed",
				(GtkSignalFunc) vadjustment_value_changed,
				scrolled);
	}
    }

    gtk_signal_emit (GTK_OBJECT (scrolled),
		     scrolled_signals[ADJUST_ADJUSTMENTS]);
}

void
scrolled_freeze (Scrolled * scrolled)
{
    g_return_if_fail (scrolled);
    g_return_if_fail (scrolled->freeze_count != (guint) - 1);

    scrolled->freeze_count++;
}

void
scrolled_thawn (Scrolled * scrolled)
{
    g_return_if_fail (scrolled);
    g_return_if_fail (scrolled->freeze_count);

    scrolled->freeze_count--;
    if (!scrolled->freeze_count)
    {
	gtk_signal_emit (GTK_OBJECT (scrolled),
			 scrolled_signals[ADJUST_ADJUSTMENTS]);
	gtk_widget_draw (GTK_WIDGET (scrolled), NULL);
    }
}

/* from gtkclist.c */
static void
check_exposures (GtkWidget * widget)
{
    GdkEvent *event;

    if (!GTK_WIDGET_REALIZED (widget))
	return;

    /*
     * Make sure graphics expose events are processed before scrolling
     * again 
     */
    while ((event = gdk_event_get_graphics_expose (widget->window)) != NULL)
    {
	gtk_widget_event (widget, event);
	if (event->expose.count == 0)
	{
	    gdk_event_free (event);
	    break;
	}
	gdk_event_free (event);
    }
}
