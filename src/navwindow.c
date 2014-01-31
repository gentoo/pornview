/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#include "pornview.h"

#include "navwindow.h"

#define PEN_WIDTH         3	/* Square border width. */
#define BORDER_WIDTH      4	/* Window border width. */

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static NavWindow *
navwin_new (ImageView * iv)
{
    NavWindow *navwin;

    GtkWidget *out_frame;
    GtkWidget *in_frame;

    navwin = g_new (NavWindow, 1);

    navwin->fix_x_pos = navwin->fix_x_pos = 1;

    navwin->iv = iv;
    navwin->popup_win = gtk_window_new (GTK_WINDOW_POPUP);

    out_frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (out_frame), GTK_SHADOW_OUT);
    gtk_container_add (GTK_CONTAINER (navwin->popup_win), out_frame);

    in_frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (in_frame), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (out_frame), in_frame);

    navwin->preview = gtk_drawing_area_new ();
    gtk_container_add (GTK_CONTAINER (in_frame), navwin->preview);

    /*
     * need gc to draw the preview sqr with
     */
    navwin->gc = gdk_gc_new (GTK_WIDGET (iv->draw_area)->window);
    gdk_gc_set_function (navwin->gc, GDK_INVERT);
    gdk_gc_set_line_attributes (navwin->gc,
				PEN_WIDTH,
				GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

    return navwin;
}

static void
navwin_draw_sqr (NavWindow * navwin, gboolean undraw, gint x, gint y)
{
    if ((navwin->sqr_x == x) && (navwin->sqr_y == y) && undraw)
	return;

    if ((navwin->sqr_x == 0)
	&& (navwin->sqr_y == 0)
	&& (navwin->sqr_width == navwin->popup_width)
	&& (navwin->sqr_height == navwin->popup_height))
	return;

    if (undraw)
    {
	gdk_draw_rectangle (navwin->preview->window,
			    navwin->gc, FALSE,
			    navwin->sqr_x + 1,
			    navwin->sqr_y + 1,
			    navwin->sqr_width - PEN_WIDTH,
			    navwin->sqr_height - PEN_WIDTH);
    }

    gdk_draw_rectangle (navwin->preview->window,
			navwin->gc, FALSE,
			x + 1,
			y + 1,
			navwin->sqr_width - PEN_WIDTH,
			navwin->sqr_height - PEN_WIDTH);

    navwin->sqr_x = x;
    navwin->sqr_y = y;
}

static void
get_sqr_origin_as_double (NavWindow * navwin,
			  gint mx, gint my, gdouble * x, gdouble * y)
{
    *x = MIN (mx - BORDER_WIDTH, NAV_WIN_SIZE);
    *y = MIN (my - BORDER_WIDTH, NAV_WIN_SIZE);

    if (*x - navwin->sqr_width / 2.0 < 0.0)
	*x = navwin->sqr_width / 2.0;

    if (*y - navwin->sqr_height / 2.0 < 0.0)
	*y = navwin->sqr_height / 2.0;

    if (*x + navwin->sqr_width / 2.0 > navwin->popup_width - 0)
	*x = navwin->popup_width - 0 - navwin->sqr_width / 2.0;

    if (*y + navwin->sqr_height / 2.0 > navwin->popup_height - 0)
	*y = navwin->popup_height - 0 - navwin->sqr_height / 2.0;

    *x = *x - navwin->sqr_width / 2.0;
    *y = *y - navwin->sqr_height / 2.0;
}

static void
navwin_update_view (NavWindow * navwin)
{
    gint    popup_x, popup_y;
    gint    popup_width, popup_height;
    gint    w, h, fwidth, fheight;
    gdouble factor;
    ImageView *iv = navwin->iv;

    w = navwin->image_width = iv->width;
    h = navwin->image_height = iv->height;

    /*
     * w = navwin->image_width;
     * h = navwin->image_height;
     */

    factor = MIN ((gdouble) (NAV_WIN_SIZE) / w, (gdouble) (NAV_WIN_SIZE) / h);
    navwin->factor = factor;

    /*
     * Popup window size.
     */
    popup_width = MAX ((gint) floor (factor * w + 0.5), 1);
    popup_height = MAX ((gint) floor (factor * h + 0.5), 1);

    gtk_drawing_area_size (GTK_DRAWING_AREA (navwin->preview),
			   popup_width, popup_height);

    /*
     * The square.
     */
    imageview_get_image_frame_size (iv, &fwidth, &fheight);

    navwin->sqr_width = fwidth * factor;
    navwin->sqr_width = MAX (navwin->sqr_width, BORDER_WIDTH);
    navwin->sqr_width = MIN (navwin->sqr_width, popup_width);

    navwin->sqr_height = fheight * factor;
    navwin->sqr_height = MAX (navwin->sqr_height, BORDER_WIDTH);
    navwin->sqr_height = MIN (navwin->sqr_height, popup_height);

    navwin->sqr_x = (0 - iv->x_pos) * factor;
    if (navwin->sqr_x < 0)
	navwin->sqr_x = 0;
    navwin->sqr_y = (0 - iv->y_pos) * factor;
    if (navwin->sqr_y < 0)
	navwin->sqr_y = 0;

    /*
     * fix x (or y) if image is smaller than frame
     */
    if (fwidth > navwin->image_width)
	navwin->fix_x_pos = (0 - iv->x_pos);
    else
	navwin->fix_x_pos = 1;
    if (fheight > navwin->image_height)
	navwin->fix_y_pos = (0 - iv->y_pos);
    else
	navwin->fix_y_pos = 1;

    /*
     * Popup window position.
     */
    popup_x = MIN ((gint) navwin->x_root - navwin->sqr_x
		   - BORDER_WIDTH
		   - navwin->sqr_width / 2,
		   gdk_screen_width () - popup_width - BORDER_WIDTH * 2);
    popup_y = MIN ((gint) navwin->y_root - navwin->sqr_y
		   - BORDER_WIDTH
		   - navwin->sqr_height / 2,
		   gdk_screen_height () - popup_height - BORDER_WIDTH * 2);

    navwin->popup_x = popup_x;
    navwin->popup_y = popup_y;
    navwin->popup_width = popup_width;
    navwin->popup_height = popup_height;

    gtk_widget_draw (navwin->preview, NULL);
}

static void
navwin_grab_pointer (NavWindow * navwin)
{
    GdkCursor *cursor;

    gtk_grab_add (navwin->popup_win);

    cursor = gdk_cursor_new (GDK_CROSSHAIR);

    gdk_pointer_grab (navwin->popup_win->window, TRUE,
		      GDK_BUTTON_RELEASE_MASK |
		      GDK_POINTER_MOTION_HINT_MASK |
		      GDK_BUTTON_MOTION_MASK |
		      GDK_EXTENSION_EVENTS_ALL,
		      navwin->preview->window, cursor, 0);

    gdk_cursor_destroy (cursor);

    /*
     * Capture keyboard events.
     */
    gdk_keyboard_grab (navwin->popup_win->window, TRUE, GDK_CURRENT_TIME);
    gtk_widget_grab_focus (navwin->popup_win);
}

static void
navwin_draw (NavWindow * navwin)
{
    ImageView *iv;

    g_return_if_fail (navwin);

    iv = navwin->iv;
    g_return_if_fail (iv);

    gdk_draw_pixmap (navwin->preview->window,
		     iv->draw_area->style->white_gc,
		     navwin->pixmap, 0, 0, 0, 0, -1, -1);

    navwin_draw_sqr (navwin, FALSE, navwin->sqr_x, navwin->sqr_y);
}

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static  gint
cb_navwin_expose (GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
    NavWindow *navwin = data;

    navwin_draw (navwin);

    if (gtk_grab_get_current () != navwin->popup_win)
	navwin_grab_pointer (navwin);

    return FALSE;
}

static  gint
cb_navwin_button_press (GtkWidget * widget,
			GdkEventButton * event, gpointer data)
{
    NavWindow *navwin = data;

    switch (event->button)
    {
      case 1:
	  navwin_grab_pointer (navwin);
	  break;
      default:
	  break;
    }

    return FALSE;
}

static  gint
cb_navwin_button_release (GtkWidget * widget,
			  GdkEventButton * event, gpointer data)
{
    NavWindow *navwin = data;

    switch (event->button)
    {
      case 1:
	  gdk_keyboard_ungrab (GDK_CURRENT_TIME);
	  gtk_grab_remove (navwin->popup_win);

	  gdk_gc_destroy (navwin->gc);
	  gtk_widget_destroy (navwin->popup_win);
	  gdk_pixmap_unref (navwin->pixmap);
	  g_free (navwin);
	  break;
      case 4:
	  gtk_signal_handler_block (GTK_OBJECT (widget),
				    navwin->motion_hand_id);
	  imageview_zoom_image (navwin->iv, IMAGEVIEW_ZOOM_OUT, 0, 0);
	  navwin_update_view (navwin);
	  navwin_draw (navwin);
	  gtk_signal_handler_unblock (GTK_OBJECT (widget),
				      navwin->motion_hand_id);
	  break;
      case 5:
	  gtk_signal_handler_block (GTK_OBJECT (widget),
				    navwin->motion_hand_id);
	  imageview_zoom_image (navwin->iv, IMAGEVIEW_ZOOM_IN, 0, 0);
	  navwin_update_view (navwin);
	  navwin_draw (navwin);
	  gtk_signal_handler_unblock (GTK_OBJECT (widget),
				      navwin->motion_hand_id);
	  break;
      default:
	  break;
    }

    return FALSE;
}

static  gint
cb_navwin_motion_notify (GtkWidget * widget,
			 GdkEventMotion * event, gpointer data)
{
    NavWindow *navwin = data;
    GdkModifierType mask;
    gint    mx, my;
    gdouble x, y;
    ImageView *iv = navwin->iv;

    g_return_val_if_fail (navwin, FALSE);
    g_return_val_if_fail (iv, FALSE);

    gdk_window_get_pointer (widget->window, &mx, &my, &mask);
    get_sqr_origin_as_double (navwin, mx, my, &x, &y);

    mx = (gint) x;
    my = (gint) y;
    navwin_draw_sqr (navwin, TRUE, mx, my);

    mx = (gint) (x / navwin->factor);
    my = (gint) (y / navwin->factor);
    if (navwin->fix_x_pos < 0)
	mx = navwin->fix_x_pos;
    if (navwin->fix_y_pos < 0)
	my = navwin->fix_y_pos;

    imageview_moveto (iv, mx, my);

    return FALSE;
}

static  gint
cb_navwin_key_press (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    NavWindow *navwin = data;
    ImageView *iv = navwin->iv;
    gboolean move = FALSE;
    GdkModifierType modval;
    guint   keyval;
    gint    mx = 0 - iv->x_pos, my = 0 - iv->y_pos;

    keyval = event->keyval;
    modval = event->state;

    if (keyval == GDK_Left)
    {
	mx -= 10;
	move = TRUE;
    }
    else if (keyval == GDK_Right)
    {
	mx += 10;
	move = TRUE;
    }
    else if (keyval == GDK_Up)
    {
	my -= 10;
	move = TRUE;
    }
    else if (keyval == GDK_Down)
    {
	my += 10;
	move = TRUE;
    }

    if (move)
    {
	gtk_signal_handler_block (GTK_OBJECT (widget),
				  navwin->keypress_hand_id);
	if (navwin->fix_x_pos < 0)
	    mx = navwin->fix_x_pos;
	if (navwin->fix_y_pos < 0)
	    my = navwin->fix_y_pos;
	imageview_moveto (iv, mx, my);
	mx *= navwin->factor;
	my *= navwin->factor;
	navwin_draw_sqr (navwin, TRUE, mx, my);
	gtk_signal_handler_unblock (GTK_OBJECT (widget),
				    navwin->keypress_hand_id);
    }

    return FALSE;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

NavWindow *
navwin_create (ImageView * iv, gfloat x_root, gfloat y_root,
	       GdkPixmap * pixmap, GdkBitmap * mask)
{
    NavWindow *navwin;

    g_return_val_if_fail (iv, NULL);
    g_return_val_if_fail (pixmap, NULL);

    navwin = navwin_new (iv);
    navwin->pixmap = gdk_pixmap_ref (pixmap);
    if (mask)
	navwin->mask = gdk_bitmap_ref (mask);

    navwin->x_root = x_root;
    navwin->y_root = y_root;

    navwin->image_width = iv->width;
    navwin->image_height = iv->height;

    navwin_update_view (navwin);

    gtk_signal_connect (GTK_OBJECT (navwin->popup_win),
			"expose_event",
			GTK_SIGNAL_FUNC (cb_navwin_expose), navwin);
    gtk_signal_connect (GTK_OBJECT (navwin->popup_win),
			"button_press_event",
			GTK_SIGNAL_FUNC (cb_navwin_button_press), navwin);
    gtk_signal_connect (GTK_OBJECT (navwin->popup_win),
			"motion_notify_event",
			GTK_SIGNAL_FUNC (cb_navwin_motion_notify), navwin);
    navwin->motion_hand_id
	= gtk_signal_connect (GTK_OBJECT (navwin->popup_win),
			      "button_release_event",
			      GTK_SIGNAL_FUNC (cb_navwin_button_release),
			      navwin);
    navwin->keypress_hand_id =
	gtk_signal_connect (GTK_OBJECT (navwin->popup_win), "key-press-event",
			    GTK_SIGNAL_FUNC (cb_navwin_key_press), navwin);

    gtk_widget_set_uposition (navwin->popup_win,
			      navwin->popup_x, navwin->popup_y);
    gtk_widget_set_usize (navwin->popup_win,
			  navwin->popup_width + BORDER_WIDTH * 2,
			  navwin->popup_height + BORDER_WIDTH * 2);

    gtk_widget_show_all (navwin->popup_win);

    navwin_grab_pointer (navwin);

    return NULL;
}
