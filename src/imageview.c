/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                        (c) 2002, 2003                        (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "zalbum.h"

#include "cursors.h"
#include "image_info.h"
#include "pixbuf_utils.h"

#include "imageview.h"
#include "browser.h"
#include "comment_view.h"
#include "navwindow.h"
#include "prefs.h"
#include "thumbview.h"
#include "viewtype.h"

#include "pixmaps/nav_button.xpm"

#if HAVE_X11_EXTENSIONS_XINERAMA_H
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <gdk/gdkx.h>
#endif

ImageView *imageview = NULL;

typedef struct
{
    gint    number;
    GdkPixbuf *image;
    gint    width;
    gint    height;
}
CacheImage;

static CacheImage *current_image = NULL;
static CacheImage *previous_image = NULL;
static CacheImage *next_image = NULL;

static gint width_backup;
static gint height_backup;

static GdkPixmap *buffer = NULL;
static gboolean move_scrollbar_by_user = TRUE;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
allocate_draw_buffer (ImageView * iv)
{
    gint    fwidth, fheight;
    gint    cwidth, cheight;

    if (!iv)
	return;

    if (buffer)
    {
	gdk_window_get_size (buffer, &cwidth, &cheight);
	imageview_get_image_frame_size (iv, &fwidth, &fheight);

	if (fwidth != cwidth || fheight != cheight)
	{
	    gdk_pixmap_unref (buffer);
	    buffer = NULL;
	    buffer =
		gdk_pixmap_new (iv->draw_area->window, fwidth, fheight, -1);
	}
    }
    else
    {
	imageview_get_image_frame_size (iv, &fwidth, &fheight);
	buffer = gdk_pixmap_new (iv->draw_area->window, fwidth, fheight, -1);
    }
}

static void
reset_scrollbar (ImageView * iv)
{
    g_return_if_fail (iv);
    g_return_if_fail (iv->draw_area);
    g_return_if_fail (iv->hadj);
    g_return_if_fail (iv->vadj);

    /*
     * horizontal 
     */
    if (iv->x_pos < 0)
	iv->hadj->value = 0 - iv->x_pos;
    else
	iv->hadj->value = 0;

    if (iv->width > iv->draw_area->allocation.width)
	iv->hadj->upper = iv->width;
    else
	iv->hadj->upper = iv->draw_area->allocation.width;

    iv->hadj->page_size = iv->draw_area->allocation.width;

    /*
     * vertical 
     */
    if (iv->y_pos < 0)
	iv->vadj->value = 0 - iv->y_pos;
    else
	iv->vadj->value = 0;

    if (iv->height > iv->draw_area->allocation.height)
	iv->vadj->upper = iv->height;
    else
	iv->vadj->upper = iv->draw_area->allocation.height;

    iv->vadj->page_size = iv->draw_area->allocation.height;

    move_scrollbar_by_user = FALSE;

    gtk_signal_emit_by_name (GTK_OBJECT (iv->hadj), "changed");
    gtk_signal_emit_by_name (GTK_OBJECT (iv->vadj), "changed");

    move_scrollbar_by_user = TRUE;
}

static void
imageview_draw_image (ImageView * iv)
{
    GdkGC  *bg_gc;

    allocate_draw_buffer (iv);

    /*
     * fill background by default bg color 
     */
    bg_gc = iv->draw_area->style->black_gc;
    gdk_draw_rectangle (buffer, bg_gc, TRUE, 0, 0, -1, -1);

    /*
     * draw image to buffer 
     */

    if (iv->pixmap)
    {
	if (iv->mask)
	{
	    gdk_gc_set_clip_mask (iv->draw_area->style->black_gc, iv->mask);
	    gdk_gc_set_clip_origin (iv->draw_area->style->black_gc,
				    iv->x_pos, iv->y_pos);
	}

	gdk_draw_pixmap (buffer,
			 iv->draw_area->style->black_gc,
			 iv->pixmap, 0, 0, iv->x_pos, iv->y_pos, -1, -1);

	if (iv->mask)
	{
	    gdk_gc_set_clip_mask (iv->draw_area->style->black_gc, NULL);
	    gdk_gc_set_clip_origin (iv->draw_area->style->black_gc, 0, 0);
	}
    }

    /*
     * draw from buffer to foreground 
     */
    gdk_draw_pixmap (iv->draw_area->window,
		     iv->draw_area->style->
		     fg_gc[GTK_WIDGET_STATE (iv->draw_area)], buffer, 0, 0, 0,
		     0, -1, -1);
}

static gboolean imgview_scroll_nolimit = FALSE;

static void
imageview_adjust_pos_in_frame (ImageView * iv, gboolean center)
{
    gint    fwidth, fheight;

    if (center)
    {
	imageview_get_image_frame_size (iv, &fwidth, &fheight);

	if (fwidth < iv->width)
	{
	    if (imgview_scroll_nolimit)
	    {
		if (iv->x_pos > fwidth)
		    iv->x_pos = 0;
		else if (iv->x_pos < 0 - iv->width)
		    iv->x_pos = 0 - iv->width + fwidth;
	    }
	    else
	    {
		if (iv->x_pos > 0)
		    iv->x_pos = 0;
		else if (iv->x_pos < 0 - iv->width + fwidth)
		    iv->x_pos = 0 - iv->width + fwidth;
	    }
	}
	else
	{
	    iv->x_pos = (fwidth - iv->width) / 2;
	}

	if (fheight < iv->height)
	{
	    if (imgview_scroll_nolimit)
	    {
		if (iv->y_pos > fheight)
		    iv->y_pos = 0;
		else if (iv->y_pos < 0 - iv->height)
		    iv->y_pos = 0 - iv->height + fheight;
	    }
	    else
	    {
		if (iv->y_pos > 0)
		    iv->y_pos = 0;
		else if (iv->y_pos < 0 - iv->height + fheight)
		    iv->y_pos = 0 - iv->height + fheight;
	    }
	}
	else
	{
	    iv->y_pos = (fheight - iv->height) / 2;
	}
    }
    else
    {
	if (!imgview_scroll_nolimit)
	{
	    imageview_get_image_frame_size (iv, &fwidth, &fheight);

	    if (iv->width <= fwidth)
	    {
		if (iv->x_pos < 0)
		    iv->x_pos = 0;
		if (iv->x_pos > fwidth - iv->width)
		    iv->x_pos = fwidth - iv->width;
	    }
	    else if (iv->x_pos < fwidth - iv->width)
	    {
		iv->x_pos = fwidth - iv->width;
	    }
	    else if (iv->x_pos > 0)
	    {
		iv->x_pos = 0;
	    }

	    if (iv->height <= fheight)
	    {
		if (iv->y_pos < 0)
		    iv->y_pos = 0;
		if (iv->y_pos > fheight - iv->height)
		    iv->y_pos = fheight - iv->height;
	    }
	    else if (iv->y_pos < fheight - iv->height)
	    {
		iv->y_pos = fheight - iv->height;
	    }
	    else if (iv->y_pos > 0)
	    {
		iv->y_pos = 0;
	    }
	}
    }
}

static void
imageview_open_navwin (ImageView * iv, gfloat x_root, gfloat y_root)
{
    GdkPixbuf *image;
    GdkPixbuf *scale;

    GdkPixmap *pixmap;
    GdkBitmap *mask;
    gint    src_width, src_height, dest_width, dest_height;
    GdkColormap *cmap;

    g_return_if_fail (iv);
    if (!iv->pixmap)
	return;

    /*
     * get pixmap for navigator 
     */
    gdk_window_get_size (iv->pixmap, &src_width, &src_height);

    cmap = gdk_colormap_get_system ();
    image = gdk_pixbuf_get_from_drawable (NULL, iv->pixmap, cmap,
					  0, 0, 0, 0, src_width, src_height);

    g_return_if_fail (image);

    if (src_width > src_height)
    {
	dest_width = NAV_WIN_SIZE;
	dest_height = src_height * NAV_WIN_SIZE / src_width;
    }
    else
    {
	dest_height = NAV_WIN_SIZE;
	dest_width = src_width * NAV_WIN_SIZE / src_height;
    }

    scale =
	gdk_pixbuf_scale_simple (image, dest_width, dest_height,
				 conf.thumbnail_quality);
    gdk_pixbuf_render_pixmap_and_mask (scale, &pixmap, &mask, 64);

    if (!pixmap)
	goto ERROR;

    /*
     * open navigate window 
     */
    navwin_create (iv, x_root, y_root, pixmap, mask);

    /*
     * free 
     */
    gdk_pixmap_unref (pixmap);

  ERROR:
    gdk_pixbuf_unref (image);
}

static void
imageview_calc_image_size (ImageView * iv)
{
    gint    width, height;
    gfloat  x_scale, y_scale;
    gboolean fit = FALSE;

    if (iv->image == NULL)
	return;

    imageview_get_image_frame_size (iv, &width, &height);

    if (iv->auto_zoom)
    {
	if (gdk_pixbuf_get_width (iv->image) > width
	    || gdk_pixbuf_get_height (iv->image) > height)
	    fit = TRUE;
	else
	{
	    iv->x_scale = iv->y_scale = 100;
	}
    }
    else if (iv->fit_to_frame)
	fit = TRUE;

    /*
     * image scale 
     */
    if (iv->rotate == 0 || iv->rotate == 2)
    {
	x_scale = iv->x_scale;
	y_scale = iv->y_scale;
    }
    else
    {
	x_scale = iv->y_scale;
	y_scale = iv->x_scale;
    }


    /*
     * calculate image size 
     */

    if (fit)
    {
	iv->x_scale = width / (gfloat) gdk_pixbuf_get_width (iv->image) * 100;
	iv->y_scale =
	    height / (gfloat) gdk_pixbuf_get_height (iv->image) * 100;

	if (iv->keep_aspect)
	{
	    if (iv->x_scale > iv->y_scale)
	    {
		iv->x_scale = iv->y_scale;
		width = gdk_pixbuf_get_width (iv->image) * iv->x_scale / 100;
	    }
	    else
	    {
		iv->y_scale = iv->x_scale;
		height =
		    gdk_pixbuf_get_height (iv->image) * iv->y_scale / 100;
	    }
	}
    }
    else
    {
	width = gdk_pixbuf_get_width (iv->image) * x_scale / 100;
	height = gdk_pixbuf_get_height (iv->image) * y_scale / 100;
    }

    if (width > 8)
	iv->width = width;
    else
	iv->width = 8;
    if (height > 8)
	iv->height = height;
    else
	iv->height = 8;
}

static void imageview_rotate_render (ImageView * iv, ImgRotateType angle);
static void imageview_print_status (ImageView * iv);

static void
imageview_calc_image (ImageView * iv, gboolean rotate)
{
    if (iv->image == NULL)
	return;

    if (rotate)
	imageview_rotate_render (iv, iv->rotate);

    iv->width = gdk_pixbuf_get_width (imageview->image);
    iv->height = gdk_pixbuf_get_height (imageview->image);

    if (imageview->pixmap)
	gdk_pixmap_unref (imageview->pixmap);

    if (imageview->mask)
	gdk_bitmap_unref (imageview->mask);

    imageview->pixmap = NULL;
    imageview->mask = NULL;

    if (iv->zoom != IMAGEVIEW_ZOOM_100)
    {
	GdkPixbuf *scale;

	imageview_calc_image_size (iv);

	scale =
	    gdk_pixbuf_scale_simple (imageview->image, iv->width, iv->height,
				     conf.image_quality);

	pixbuf_render_pixmap_and_mask (scale, &(iv->pixmap), &(iv->mask),
				       conf.image_dithering);

	gdk_pixbuf_unref (scale);
    }
    else
	pixbuf_render_pixmap_and_mask (imageview->image, &(iv->pixmap),
				       &(iv->mask), conf.image_dithering);

    imageview_adjust_pos_in_frame (iv, TRUE);

    imageview_draw_image (iv);
    reset_scrollbar (iv);

    if (!iv->fullscreen)
	imageview_print_status (iv);
}

static void
imageview_clear_image (void)
{
    GdkGC  *bg_gc;

    if (imageview->loader)
	image_loader_free (imageview->loader);
    imageview->loader = NULL;

    if (imageview->image_name)
	g_free (imageview->image_name);
    imageview->image_name = NULL;

    if (imageview->image)
	gdk_pixbuf_unref (imageview->image);
    imageview->image = NULL;

    if (imageview->pixmap)
	gdk_pixmap_unref (imageview->pixmap);

    if (imageview->mask)
	gdk_bitmap_unref (imageview->mask);

    imageview->pixmap = NULL;
    imageview->mask = NULL;

    imageview->width = 0;
    imageview->height = 0;
    imageview->x_pos = 0;
    imageview->y_pos = 0;

    bg_gc = imageview->draw_area->style->black_gc;
    gdk_draw_rectangle (imageview->draw_area->window, bg_gc, TRUE, 0, 0, -1,
			-1);

    reset_scrollbar (imageview);

    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1);
    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1, "");
    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1);
    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1, "");
}

static  gboolean
imageview_get_image_position (ImageView * iv, gint * x, gint * y)
{
    g_return_val_if_fail (iv, FALSE);
    g_return_val_if_fail (x && y, FALSE);

    *x = 0 - iv->x_pos;
    *y = 0 - iv->y_pos;

    return TRUE;
}

static void
show_cursor (ImageView * iv)
{
    GdkCursor *cursor;

    g_return_if_fail (iv);
    g_return_if_fail (iv->draw_area);
    g_return_if_fail (GTK_WIDGET_MAPPED (iv->draw_area));

    cursor = cursor_get (iv->draw_area->window, CURSOR_HAND_OPEN);
    gdk_window_set_cursor (iv->draw_area->window, cursor);
    gdk_cursor_destroy (cursor);
}

static void
hide_cursor (ImageView * iv)
{
    GdkCursor *cursor;

    g_return_if_fail (iv);
    g_return_if_fail (iv->draw_area);
    g_return_if_fail (GTK_WIDGET_MAPPED (iv->draw_area));

    cursor = cursor_get (iv->draw_area->window, CURSOR_VOID);
    gdk_window_set_cursor (iv->draw_area->window, cursor);
    gdk_cursor_destroy (cursor);
}

static  gint
timeout_hide_cursor (gpointer data)
{
    ImageView *iv = data;

    hide_cursor (iv);
    iv->hide_cursor_timer_id = 0;

    return FALSE;
}

static void
imageview_set_bg_color (ImageView * iv, gint red, gint green, gint brue)
{
    GdkColor *bg_color;

    g_return_if_fail (iv);
    g_return_if_fail (iv->draw_area);

    bg_color = g_new0 (GdkColor, 1);
    bg_color->pixel = 0;

    bg_color->red = red;
    bg_color->green = green;
    bg_color->blue = brue;

    if (GTK_WIDGET_MAPPED (iv->draw_area))
    {
	GdkColormap *colormap;
	GtkStyle *style;
	colormap = gdk_window_get_colormap (iv->draw_area->window);
	gdk_colormap_alloc_color (colormap, bg_color, FALSE, TRUE);
	style = gtk_style_copy (gtk_widget_get_style (iv->draw_area));
	style->bg[GTK_STATE_NORMAL] = *bg_color;
	gtk_widget_set_style (iv->draw_area, style);
	gtk_style_unref (style);
    }
}

/* fullscreen functions */

static void imageview_slideshow_delete (ImageView * iv);
static gboolean cb_fullscreen_motion_notify (GtkWidget * widget,
					     GdkEventButton * bevent,
					     gpointer data);

static CacheImage *load_in_direction (ImageView * iv, gint dir);

static void
destroy_image (CacheImage * im)
{
    if (!im)
	return;

    if (im->image)
	gdk_pixbuf_unref (im->image);

    g_free (im);
}

static  gint
load_rest (gpointer data)
{
    ImageView *iv = data;

    if (iv->f_current == (iv->f_max - 1))
    {
	iv->f_direction = -1;
	next_image = NULL;
	previous_image = load_in_direction (iv, -1);
    }
    else
    {
	iv->f_direction = +1;

	if ((iv->f_current - 1) >= 0)
	    previous_image = load_in_direction (iv, -1);
	else
	    previous_image = NULL;

	next_image = load_in_direction (iv, 1);
    }

    return FALSE;
}

static void
render (ImageView * iv)
{
    ZAlbumCell *cell;

    if (!current_image)
	return;

    iv->f_current = current_image->number;

    if (iv->image_name)
	g_free (iv->image_name);
    iv->image_name = NULL;

    if (iv->image)
	gdk_pixbuf_unref (iv->image);
    iv->image = NULL;

    iv->image = gdk_pixbuf_ref (current_image->image);
    cell = ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album), iv->f_current);

    iv->image_name = g_strdup (cell->name);

    if (iv->image)
	imageview_calc_image (iv, TRUE);
}

static CacheImage *
load_file (int number)
{
    ZAlbumCell *cell;
    CacheImage *im = g_malloc (sizeof (CacheImage));

    cell = ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album), number);

#ifdef USE_GTK2
    if ((im->image = gdk_pixbuf_new_from_file (cell->name, NULL)) == NULL)
#else
    if ((im->image = gdk_pixbuf_new_from_file (cell->name)) == NULL)
#endif
    {
	g_free (im);
	return NULL;
    }

    im->number = number;
    im->width = gdk_pixbuf_get_width (im->image);
    im->height = gdk_pixbuf_get_height (im->image);

    return im;
}

static CacheImage *
load_in_direction (ImageView * iv, gint dir)
{
    CacheImage *im = NULL;
    guint   id;

    if (current_image != NULL)
	id = current_image->number + dir;
    else
	id = iv->f_current;

    while (id < iv->f_max && !(im = load_file (id)))
	id += dir;

    if (im != NULL)
	im->number = id;

    return im;
}

static void
load_direction (ImageView * iv, gint dir)
{
    CacheImage **backup;
    CacheImage **next;
    static gboolean next_is_requested = FALSE;
    static gboolean load_finished = TRUE;
    gint    count_break = 0;

    iv->f_load_lock = 1;

    if (load_finished == FALSE)
    {
	next_is_requested = TRUE;
	return;
    }

    if (ABS (dir) != 1)
	return;

    if ((dir == 1 && next_image == NULL)
	|| (dir == -1 && previous_image == NULL))
    {
	iv->f_load_lock = 0;
	return;
    }

    load_finished = FALSE;

    if (dir == -1)
    {
	backup = &next_image;
	next = &previous_image;
    }
    else
    {
	backup = &previous_image;
	next = &next_image;
    }

    destroy_image (*backup);
    *backup = current_image;
    current_image = *next;
    render (iv);

    while (gtk_events_pending () != 0 && next_is_requested == FALSE)
    {
	if (thumbview->thumbs_running)
	{
	    count_break++;

	    if (count_break == 100)
		break;
	}

	gtk_main_iteration_do (FALSE);
    }

    *next = load_in_direction (iv, dir);
    next_is_requested = FALSE;
    load_finished = TRUE;

    iv->f_load_lock = 0;
}

static void
load_first_image (ImageView * iv)
{
    gint    id;

    destroy_image (previous_image);
    destroy_image (current_image);
    destroy_image (next_image);

    id = 0;
    
    while (id < iv->f_max && !(current_image = load_file (id)))
	id++;

    render (iv);
    iv->f_direction = +1;
    previous_image = NULL;

    id++;

    while (id < iv->f_max && !(next_image = load_file (id)))
	id++;
}

static void
load_last_image (ImageView * iv)
{
    gint    id;

    destroy_image (previous_image);
    destroy_image (current_image);
    destroy_image (next_image);

    id = iv->f_max - 1;
    
    while (id >= 0 && !(current_image = load_file (id)))
	id--;

    render (iv);
    iv->f_direction = -1;
    next_image = NULL;

    id--;
    
    while (id >= 0 && !(previous_image = load_file (id)))
	id--;
}

void
imageview_fullscreen_new (ImageView * iv)
{
    int     width, height;
    int     x, y;
#if HAVE_X11_EXTENSIONS_XINERAMA_H
    Display *dpy;
#endif

    g_return_if_fail (iv);

    imageview_get_image_frame_size (iv, &width_backup, &height_backup);
    iv->is_fullscreen = TRUE;

    /*
     * create window
     */
    iv->fullscreen = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_modal (GTK_WINDOW (iv->fullscreen), TRUE);
    gtk_window_set_wmclass (GTK_WINDOW (iv->fullscreen), "",
			    "pornview_fullscreen");

    x = 0;
    y = 0;
    width = gdk_screen_width ();
    height = gdk_screen_height ();
#if HAVE_X11_EXTENSIONS_XINERAMA_H
    dpy = GDK_DISPLAY ();
    if (XineramaIsActive (dpy))
    {
	XineramaScreenInfo *xinerama;
	int     i, count;

	xinerama = XineramaQueryScreens (dpy, &count);
	for (i = 0; i < count; i++)
	{
	    if (x >= xinerama[i].x_org && y >= xinerama[i].y_org &&
		x < xinerama[i].x_org + xinerama[i].width &&
		y < xinerama[i].y_org + xinerama[i].height)
	    {
		x = xinerama[i].x_org;
		y = xinerama[i].y_org;
		width = xinerama[i].width;
		height = xinerama[i].height;
		break;
	    }
	}
	if (count > 0 && i == count)
	{
	    i = 0;
	    x = xinerama[i].x_org;
	    y = xinerama[i].y_org;
	    width = xinerama[i].width;
	    height = xinerama[i].height;
	}
    }
#endif
    gtk_widget_set_uposition (iv->fullscreen, 0, 0);
    gtk_widget_set_usize (iv->fullscreen, width, height);

    gtk_signal_connect (GTK_OBJECT (iv->fullscreen),
			"motion_notify_event",
			(GtkSignalFunc) cb_fullscreen_motion_notify, iv);

    /*
     * set draw widget
     */
    imageview_set_bg_color (iv, 0, 0, 0);

    if (iv->x_pos < 0 || iv->y_pos)
	imageview_moveto (iv, 0, 0);

    gtk_widget_reparent (GTK_WIDGET (iv->draw_area), iv->fullscreen);

    gtk_widget_show (iv->fullscreen);

    gdk_keyboard_grab (iv->fullscreen->window, TRUE, GDK_CURRENT_TIME);
    gtk_grab_add (iv->fullscreen);
    gtk_widget_grab_focus (GTK_WIDGET (iv->draw_area));

    iv->f_current = thumbview->current;
    iv->f_max = ZALBUM (thumbview->album)->len;
    iv->f_load_lock = 0;
    iv->f_direction = +1;

    previous_image = NULL;
    next_image = NULL;
    current_image = g_malloc (sizeof (CacheImage));
    current_image->image = gdk_pixbuf_ref (iv->image);
    current_image->width = gdk_pixbuf_get_width (iv->image);
    current_image->height = gdk_pixbuf_get_height (iv->image);
    current_image->number = iv->f_current;

    iv->f_load_timer_id = gtk_timeout_add (100, load_rest, iv);

    /*
     * enable auto hide cursor stuff
     */
    iv->hide_cursor_timer_id
	= gtk_timeout_add (IMGWIN_FULLSCREEN_HIDE_CURSOR_DELAY,
			   timeout_hide_cursor, iv);
}

void
imageview_fullscreen_delete (ImageView * iv)
{
    gint    view_type;
    ImageInfo *ii;

    g_return_if_fail (iv);

    if (iv->f_load_lock == 1)
	return;

    if (iv->is_slideshow)
	imageview_slideshow_delete (iv);

    /*
     * restore draw widget 
     */

    view_type = viewtype_get ();


    if (view_type != VIEWTYPE_IMAGEVIEW)
	viewtype_set_force (VIEWTYPE_IMAGEVIEW);

    gtk_widget_reparent (GTK_WIDGET (iv->draw_area), iv->table);

    destroy_image (current_image);
    destroy_image (next_image);
    destroy_image (previous_image);

    current_image = NULL;
    previous_image = NULL;
    next_image = NULL;

    imageview_print_status (iv);
    ii = image_info_get_from_pixbuf (iv->image_name, iv->image);
    comment_view_change_file (commentview, ii);
    thumbview_iupdate ();

    zlist_cell_focus (ZLIST (thumbview->album), iv->f_current);

    gdk_keyboard_ungrab (GDK_CURRENT_TIME);
    gtk_widget_grab_focus (GTK_WIDGET (iv->draw_area));

    /*
     * disable auto hide cursor stuff 
     */
    if (iv->hide_cursor_timer_id != 0)
	gtk_timeout_remove (iv->hide_cursor_timer_id);
    show_cursor (iv);

    if (view_type != VIEWTYPE_IMAGEVIEW)
	viewtype_set_force (view_type);

    gtk_widget_destroy (iv->fullscreen);

    iv->fullscreen = NULL;
    iv->is_fullscreen = FALSE;
}

static gint cb_slideshow (gpointer data);

static void
imageview_slideshow_new (ImageView * iv)
{
    if (iv->is_fullscreen == FALSE)
	imageview_fullscreen_new (iv);

    iv->slideshow_timer_id = gtk_timeout_add (conf.slideshow_interval,
					      (GtkFunction) cb_slideshow, iv);
    iv->is_slideshow = TRUE;
}

static void
imageview_slideshow_delete (ImageView * iv)
{
    if (iv->slideshow_timer_id)
    {
	gtk_timeout_remove (iv->slideshow_timer_id);
	iv->slideshow_timer_id = 0;
    }

    iv->is_slideshow = FALSE;
}

typedef struct _RotateData_Tag
{
    ImageView *iv;
    ImgRotateType angle;
}
RotateData;

static  gint
cb_rotate (gpointer user_data)
{
    RotateData *data = user_data;

    imageview_rotate_render (data->iv, data->angle);
    imageview_calc_image (data->iv, FALSE);

    g_free (data);

    return FALSE;
}

static void
imageview_rotate (ImageView * iv, ImgRotateType angle)
{
    ImgRotateType a, rotate_angle;
    RotateData *data;

    a = (iv->rotate + angle) % 4;

    rotate_angle = (a - iv->rotate) % 4;
    if (rotate_angle < 0)
	rotate_angle = rotate_angle + 4;

    iv->rotate = a;

    data = g_new0 (RotateData, 1);
    data->iv = iv;
    data->angle = rotate_angle;

    iv->rotate_timer_id =
	gtk_timeout_add (100, (GtkFunction) cb_rotate, data);
}

static GdkPixbuf *
imageview_rotate_image_90 (GdkPixbuf * source)
{
    GdkPixbuf *destination;

    destination = pixbuf_copy_rotate_90 (source, TRUE);
    gdk_pixbuf_unref (source);

    return destination;
}

static GdkPixbuf *
imageview_rotate_image_180 (GdkPixbuf * source)
{
    GdkPixbuf *destination;

    destination = pixbuf_copy_mirror (source, TRUE, TRUE);
    gdk_pixbuf_unref (source);

    return destination;
}

static GdkPixbuf *
imageview_rotate_image_270 (GdkPixbuf * source)
{
    GdkPixbuf *destination;

    destination = pixbuf_copy_rotate_90 (source, FALSE);
    gdk_pixbuf_unref (source);

    return destination;
}

static  gint
cb_zoom (gpointer user_data)
{
    ImageView *iv = user_data;

    imageview_zoom_image (iv, iv->zoom, 0, 0);

    return FALSE;
}

static void
imageview_zoom (ImageView * iv, ImgZoomType zoom)
{
    iv->zoom = zoom;

    iv->zoom_timer_id = gtk_timeout_add (100, (GtkFunction) cb_zoom, iv);
}

static void
imageview_rotate_render (ImageView * iv, ImgRotateType angle)
{
    switch (angle)
    {
      case IMAGEVIEW_ROTATE_90:
	  iv->image = imageview_rotate_image_90 (iv->image);
	  break;
      case IMAGEVIEW_ROTATE_180:
	  iv->image = imageview_rotate_image_180 (iv->image);
	  break;
      case IMAGEVIEW_ROTATE_270:
	  iv->image = imageview_rotate_image_270 (iv->image);
	  break;
      default:
	  break;
    }
}

static void
imageview_print_status (ImageView * iv)
{
    gchar  *buf = NULL;
    gchar  *rotate = NULL;
    gchar  *zoom = NULL;

    switch (iv->zoom)
    {
      case IMAGEVIEW_ZOOM_100:
	  zoom = g_strdup (_("original size"));
	  break;
      case IMAGEVIEW_ZOOM_IN:
	  zoom = g_strdup (_("zoom in"));
	  break;
      case IMAGEVIEW_ZOOM_OUT:
	  zoom = g_strdup (_("zoom out"));
	  break;
      case IMAGEVIEW_ZOOM_FIT:
	  zoom = g_strdup (_("fit to window"));
	  break;
      case IMAGEVIEW_ZOOM_AUTO:
	  zoom = g_strdup (_("auto zoom"));
	  break;
    }

    switch (iv->rotate)
    {
      case IMAGEVIEW_ROTATE_0:
	  rotate = g_strdup (_("0 deg"));
	  break;
      case IMAGEVIEW_ROTATE_90:
	  rotate = g_strdup (_("90 deg"));
	  break;
      case IMAGEVIEW_ROTATE_180:
	  rotate = g_strdup (_("180 deg"));
	  break;
      case IMAGEVIEW_ROTATE_270:
	  rotate = g_strdup (_("-90 deg"));
	  break;
    }

    buf =
	g_strdup_printf (_("  %s (%dx%d)"), g_basename (iv->image_name),
			 iv->image_width, iv->image_height);

    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1);
    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1, buf);

    g_free (buf);

    buf = g_strdup_printf (_("  [ zoom: %s ] [ rotate: %s ]"), zoom, rotate);

    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1);
    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1, buf);

    g_free (buf);
    g_free (zoom);
    g_free (rotate);
}

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_scrollbar_value_changed (GtkAdjustment * adj, ImageView * iv)
{
    g_return_if_fail (iv);

    if (!move_scrollbar_by_user)
	return;

    if (iv->width > iv->draw_area->allocation.width)
	iv->x_pos = 0 - iv->hadj->value;
    if (iv->height > iv->draw_area->allocation.height)
	iv->y_pos = 0 - iv->vadj->value;

    imageview_draw_image (iv);
}

static  gboolean
cb_nav_button (GtkWidget * widget, GdkEventButton * event, ImageView * iv)
{
    g_return_val_if_fail (iv, FALSE);

    imageview_open_navwin (iv, event->x_root, event->y_root);

    return FALSE;
}

static void
cb_imageview_load_done (ImageLoader * il, gpointer data)
{
    ImageView *iv = data;
    ImageInfo *ii;

    imageview->image = image_loader_get_pixbuf (il);
    gdk_pixbuf_ref (iv->image);

    if (iv->image)
	imageview_calc_image (iv, TRUE);

    iv->image_width = gdk_pixbuf_get_width (iv->image);
    iv->image_height = gdk_pixbuf_get_height (iv->image);

    imageview_print_status (iv);

    ii = image_info_get_from_pixbuf (iv->image_name, iv->image);
    comment_view_change_file (commentview, ii);
}

static void
cb_imageview_load_error (ImageLoader * il, gpointer data)
{
/* ??? */
    cb_imageview_load_done (il, data);
}

static  gboolean
cb_image_configure (GtkWidget * widget, GdkEventConfigure * event,
		    ImageView * iv)
{
    gint    fwidth, fheight;

    imageview_get_image_frame_size (iv, &fwidth, &fheight);

    if (fwidth < iv->width)
    {
	if (iv->x_pos > 0 || iv->x_pos > fwidth || iv->x_pos < 0 - iv->width)
	    iv->x_pos = 0;
    }
    else
    {
	iv->x_pos = (fwidth - iv->width) / 2;
    }

    if (fheight < iv->height)
    {
	if (iv->y_pos > 0 || iv->y_pos > fheight
	    || iv->y_pos < 0 - iv->height)
	    iv->y_pos = 0;
    }
    else
    {
	iv->y_pos = (fheight - iv->height) / 2;
    }

    if (iv->x_pos < 0 || iv->y_pos < 0)
	imageview_moveto (iv, 0, 0);

    if ((iv->zoom == IMAGEVIEW_ZOOM_FIT || iv->zoom == IMAGEVIEW_ZOOM_AUTO)
	&& iv->pixmap)
    {
	gint    width, height;

	gdk_window_get_size (iv->pixmap, &width, &height);

	if (fwidth != width || fheight != height)
	{
	    imageview_calc_image (iv, FALSE);
	    reset_scrollbar (iv);

	    return TRUE;
	}
    }

    imageview_draw_image (iv);
    reset_scrollbar (iv);

    return TRUE;
}

static  gboolean
cb_image_expose (GtkWidget * widget, GdkEventExpose * event, ImageView * iv)
{
    if (iv->pixmap)
	imageview_draw_image (iv);

    return TRUE;
}

static  gboolean
cb_image_motion_notify (GtkWidget * widget, GdkEventMotion * event,
			ImageView * iv)
{
    GdkModifierType mods;
    gint    x, y, x_pos, y_pos, dx, dy;

    if (!iv->pressed)
	return FALSE;

    if (event->is_hint)
	gdk_window_get_pointer (widget->window, &x, &y, &mods);

    x_pos = x - iv->drag_startx;
    y_pos = y - iv->drag_starty;
    dx = x_pos - iv->x_pos_drag_start;
    dy = y_pos - iv->y_pos_drag_start;

    if (!iv->dragging && (abs (dx) > 2 || abs (dy) > 2))
	iv->dragging = TRUE;

    /*
     * scroll image 
     */
    if (iv->button == 1)
    {
	if (event->is_hint)
	    gdk_window_get_pointer (widget->window, &x, &y, &mods);

	iv->x_pos = x_pos;
	iv->y_pos = y_pos;

	imageview_adjust_pos_in_frame (iv, FALSE);
    }

    imageview_draw_image (iv);
    reset_scrollbar (iv);

    return TRUE;
}

static  gboolean
cb_image_button_press (GtkWidget * widget, GdkEventButton * event,
		       ImageView * iv)
{
    GdkCursor *cursor;
    gint    retval;

    g_return_val_if_fail (iv, FALSE);

    iv->pressed = TRUE;
    iv->button = event->button;

    gtk_widget_grab_focus (widget);

    if (iv->dragging)
	return FALSE;

    if (event->button == 1)
    {				/* scroll image */
	if (!iv->pixmap)
	    return FALSE;

	cursor = cursor_get (widget->window, CURSOR_HAND_CLOSED);

	retval = gdk_pointer_grab (widget->window, FALSE,
				   (GDK_POINTER_MOTION_MASK
				    | GDK_POINTER_MOTION_HINT_MASK
				    | GDK_BUTTON_RELEASE_MASK),
				   NULL, cursor, event->time);

	gdk_cursor_destroy (cursor);

	if (retval != 0)
	    return FALSE;

	iv->drag_startx = event->x - iv->x_pos;
	iv->drag_starty = event->y - iv->y_pos;
	iv->x_pos_drag_start = iv->x_pos;
	iv->y_pos_drag_start = iv->y_pos;

	return TRUE;

    }
    else if (event->button == 2 && iv->is_fullscreen)
    {
	imageview_fullscreen_delete (iv);
	return TRUE;
    }

    return FALSE;
}

static  gboolean
cb_image_button_release (GtkWidget * widget, GdkEventButton * event,
			 ImageView * iv)
{
    gint    retval = TRUE;

    if (iv->pressed && !iv->dragging)
    {
	switch (event->button)
	{
	  case 1:
	  case 5:
	      if (iv->fullscreen)
	      {
		  gtk_main_iteration_do (FALSE);
		  load_direction (iv, 1);
	      }
	      else
		  thumbview_select_next ();
	      break;

	  case 3:
	  case 4:
	      if (iv->fullscreen)
	      {
		  gtk_main_iteration_do (FALSE);
		  load_direction (iv, -1);
	      }
	      else
		  thumbview_select_prev ();
	      break;
	}
    }

    iv->button = 0;
    iv->pressed = FALSE;
    iv->dragging = FALSE;

    gdk_pointer_ungrab (event->time);

    return retval;
}

static void
cb_draw_area_map (GtkWidget * widget, ImageView * iv)
{
    GdkCursor *cursor;

    imageview_set_bg_color (iv, 0, 0, 0);

    cursor = cursor_get (iv->draw_area->window, CURSOR_HAND_OPEN);
    gdk_window_set_cursor (iv->draw_area->window, cursor);
}

static  gboolean
cb_image_key_press (GtkWidget * widget, GdkEventKey * event, ImageView * iv)
{
    guint   keyval;
    GdkModifierType modval;
    gboolean move = FALSE;
    gint    mx, my;

    g_return_val_if_fail (iv, FALSE);

    if (iv->is_hide)
	return FALSE;

    imageview_get_image_position (iv, &mx, &my);

    keyval = event->keyval;
    modval = event->state;

    switch (keyval)
    {
      case GDK_F:
      case GDK_f:
	  if (iv->zoom != IMAGEVIEW_ZOOM_FIT)
	      imageview_zoom_image (iv, IMAGEVIEW_ZOOM_FIT, 0, 0);
	  return TRUE;

      case GDK_O:
      case GDK_o:
	  if (iv->zoom != IMAGEVIEW_ZOOM_100)
	      imageview_zoom_image (iv, IMAGEVIEW_ZOOM_100, 0, 0);
	  return TRUE;

      case GDK_plus:
      case GDK_KP_Add:
	  imageview_zoom_image (iv, IMAGEVIEW_ZOOM_IN, 0, 0);
	  return TRUE;

      case GDK_minus:
      case GDK_KP_Subtract:
	  imageview_zoom_image (iv, IMAGEVIEW_ZOOM_OUT, 0, 0);
	  return TRUE;

      case GDK_Left:
	  mx -= 20;
	  move = TRUE;
	  break;

      case GDK_Right:
	  mx += 20;
	  move = TRUE;
	  break;

      case GDK_Up:
	  my -= 20;
	  move = TRUE;
	  break;

      case GDK_Down:
	  my += 20;
	  move = TRUE;
	  break;

      case GDK_Page_Up:
      case GDK_P:
      case GDK_p:
	  if (iv->fullscreen)
	  {
	      if (iv->f_load_lock == 0)
		  load_direction (iv, -1);
	  }
	  else
	      thumbview_select_prev ();
	  return TRUE;

      case GDK_Page_Down:
      case GDK_N:
      case GDK_n:
	  if (iv->fullscreen)
	  {
	      if (iv->f_load_lock == 0)
		  load_direction (iv, 1);
	  }
	  else
	      thumbview_select_next ();
	  return TRUE;

      case GDK_Home:
	  if (iv->fullscreen)
	      load_first_image (iv);
	  else
	      thumbview_select_first ();
	  return TRUE;

      case GDK_End:
	  if (iv->fullscreen)
	      load_last_image (iv);
	  else
	      thumbview_select_last ();
	  return TRUE;

      case GDK_Return:
	  if (iv->is_fullscreen == FALSE)
	      imageview_fullscreen_new (iv);
	  return TRUE;

      case GDK_S:
      case GDK_s:
	  if (iv->is_fullscreen)
	  {
	      if (iv->is_slideshow)
		  imageview_slideshow_delete (iv);
	      else
		  imageview_slideshow_new (iv);
	  }
	  return TRUE;

      case GDK_Q:
      case GDK_q:
      case GDK_Escape:
	  if (iv->is_fullscreen)
	      imageview_fullscreen_delete (iv);
	  return TRUE;
    }

    if (move)
    {
	imageview_moveto (iv, mx, my);
	return TRUE;
    }

    return FALSE;
}

static  gboolean
cb_fullscreen_motion_notify (GtkWidget * widget,
			     GdkEventButton * bevent, gpointer data)
{
    ImageView *iv = data;

    gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
    show_cursor (iv);

    if (iv->hide_cursor_timer_id != 0)
	gtk_timeout_remove (iv->hide_cursor_timer_id);
    iv->hide_cursor_timer_id
	= gtk_timeout_add (IMGWIN_FULLSCREEN_HIDE_CURSOR_DELAY,
			   timeout_hide_cursor, iv);

    return FALSE;
}

static  gint
cb_slideshow (gpointer data)
{
    ImageView *iv = data;

    gdk_threads_enter ();

    load_direction (iv, iv->f_direction);

    gdk_threads_leave ();

    return TRUE;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
imageview_create (void)
{
    GdkPixmap *pixmap;
    GdkBitmap *mask;
    GdkPixbuf *pixbuf;

    imageview = g_new0 (ImageView, 1);
    imageview->image = NULL;
    imageview->loader = NULL;
    imageview->pixmap = NULL;
    imageview->mask = NULL;
    imageview->image_name = NULL;
    imageview->x_scale = imageview->y_scale = 100;
    imageview->auto_zoom = conf.image_fit;
    imageview->is_fullscreen = FALSE;
    imageview->is_slideshow = FALSE;
    imageview->is_hide = FALSE;

    if (imageview->auto_zoom)
    {
	imageview->fit_to_frame = TRUE;
	imageview->zoom = IMAGEVIEW_ZOOM_AUTO;
    }
    else
	imageview->fit_to_frame = FALSE;

    /*
     * container 
     */
    imageview->container = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (imageview->container),
			       GTK_SHADOW_IN);
    gtk_widget_show (imageview->container);

    /*
     * create widgets 
     */
    imageview->table = gtk_table_new (2, 2, FALSE);
    gtk_widget_show (imageview->table);
    gtk_container_add (GTK_CONTAINER (imageview->container),
		       imageview->table);

    imageview->draw_area = gtk_drawing_area_new ();
    gtk_widget_show (imageview->draw_area);

    imageview->hadj =
	GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 10.0, 10.0, 0.0));
    imageview->hscrollbar = gtk_hscrollbar_new (imageview->hadj);
    gtk_widget_show (imageview->hscrollbar);

    imageview->vadj =
	GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 10.0, 10.0, 0.0));
    imageview->vscrollbar = gtk_vscrollbar_new (imageview->vadj);
    gtk_widget_show (imageview->vscrollbar);

    imageview->event_box = gtk_event_box_new ();
    gtk_widget_set_name (imageview->event_box, "/imageview/NavWinButton");
    gtk_widget_show (imageview->event_box);

    pixbuf = gdk_pixbuf_new_from_xpm_data (nav_button_xpm);

    gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &mask, 127);

    imageview->nav_button = gtk_pixmap_new (pixmap, mask);

    gdk_pixbuf_unref (pixbuf);

    gtk_container_add (GTK_CONTAINER (imageview->event_box),
		       imageview->nav_button);
    gtk_widget_show (imageview->nav_button);

    gtk_table_attach (GTK_TABLE (imageview->table),
		      GTK_WIDGET (imageview->draw_area), 0, 1, 0, 1,
		      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
    gtk_table_attach (GTK_TABLE (imageview->table), imageview->vscrollbar, 1,
		      2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (imageview->table), imageview->hscrollbar, 0,
		      1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE (imageview->table), imageview->event_box, 1,
		      2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

    gtk_signal_connect_after (GTK_OBJECT (imageview->draw_area), "map",
			      GTK_SIGNAL_FUNC (cb_draw_area_map), imageview);
    gtk_signal_connect_after (GTK_OBJECT (imageview->draw_area),
			      "key-press-event",
			      GTK_SIGNAL_FUNC (cb_image_key_press),
			      imageview);

    gtk_signal_connect (GTK_OBJECT (imageview->draw_area), "configure_event",
			GTK_SIGNAL_FUNC (cb_image_configure), imageview);
    gtk_signal_connect (GTK_OBJECT (imageview->draw_area), "expose_event",
			GTK_SIGNAL_FUNC (cb_image_expose), imageview);
    gtk_signal_connect (GTK_OBJECT (imageview->draw_area),
			"motion_notify_event",
			GTK_SIGNAL_FUNC (cb_image_motion_notify), imageview);

    gtk_signal_connect (GTK_OBJECT (imageview->draw_area),
			"button_press_event",
			GTK_SIGNAL_FUNC (cb_image_button_press), imageview);

    gtk_signal_connect (GTK_OBJECT (imageview->draw_area),
			"button_release_event",
			GTK_SIGNAL_FUNC (cb_image_button_release), imageview);

    /*
     * set flags 
     */
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (imageview->draw_area), GTK_CAN_FOCUS);

    gtk_widget_add_events (GTK_WIDGET (imageview->draw_area),
			   GDK_FOCUS_CHANGE
			   | GDK_BUTTON_PRESS_MASK | GDK_2BUTTON_PRESS
			   | GDK_KEY_PRESS | GDK_KEY_RELEASE
			   | GDK_BUTTON_RELEASE_MASK
			   | GDK_POINTER_MOTION_MASK
			   | GDK_POINTER_MOTION_HINT_MASK);

    /*
     * set signals 
     */
    gtk_signal_connect (GTK_OBJECT (imageview->hadj), "value_changed",
			GTK_SIGNAL_FUNC (cb_scrollbar_value_changed),
			imageview);

    gtk_signal_connect (GTK_OBJECT (imageview->vadj), "value_changed",
			GTK_SIGNAL_FUNC (cb_scrollbar_value_changed),
			imageview);

    gtk_signal_connect (GTK_OBJECT (imageview->event_box),
			"button_press_event", GTK_SIGNAL_FUNC (cb_nav_button),
			imageview);
}

void
imageview_destroy (void)
{
    if (buffer)
	gdk_pixmap_unref (buffer);

    if (imageview->loader)
	image_loader_free (imageview->loader);

    if (imageview->image_name)
	g_free (imageview->image_name);
    imageview->image_name = NULL;

    if (imageview->image)
	gdk_pixbuf_unref (imageview->image);

    if (imageview->pixmap)
	gdk_pixmap_unref (imageview->pixmap);

    if (imageview->mask)
	gdk_bitmap_unref (imageview->mask);

    g_free (imageview);
    imageview = NULL;
}

void
imageview_stop (void)
{
    if (imageview->is_fullscreen)
	imageview_fullscreen_delete (imageview);

    if (imageview->loader)
	imageview_set_image (NULL);
}

void
imageview_set_image (gchar * path)
{
    if (imageview->image_name && path)
	if (strcmp (imageview->image_name, path) == 0)
	    return;

    imageview->keep_aspect = TRUE;

    image_loader_free (imageview->loader);
    imageview->loader = NULL;

    if (imageview->image_name)
	g_free (imageview->image_name);
    imageview->image_name = NULL;

    if (imageview->image)
	gdk_pixbuf_unref (imageview->image);
    imageview->image = NULL;

    if (path == NULL)
    {
	imageview_clear_image ();
	return;
    }

    imageview->image_name = g_strdup (path);
    imageview->loader = image_loader_new (path);
    image_loader_set_error_func (imageview->loader, cb_imageview_load_error,
				 imageview);
    image_loader_set_buffer_size (imageview->loader, 0);
    image_loader_set_priority (imageview->loader, G_PRIORITY_HIGH_IDLE);

    if (!image_loader_start
	(imageview->loader, cb_imageview_load_done, imageview))
    {
	image_loader_free (imageview->loader);
	imageview->loader = NULL;

	if (imageview->image_name)
	    g_free (imageview->image_name);
	imageview->image_name = NULL;

	if (imageview->image)
	    gdk_pixbuf_unref (imageview->image);

	imageview->image = NULL;
	imageview_clear_image ();
    }
}

void
imageview_no_zoom (void)
{
    imageview_zoom (imageview, IMAGEVIEW_ZOOM_100);
    thumbview_iupdate ();
}

void
imageview_zoom_fit (void)
{
    imageview_zoom (imageview, IMAGEVIEW_ZOOM_FIT);
    thumbview_iupdate ();
}

void
imageview_zoom_auto (void)
{
    imageview_zoom (imageview, IMAGEVIEW_ZOOM_AUTO);
    thumbview_iupdate ();
}

void
imageview_zoom_in (void)
{
    imageview_zoom (imageview, IMAGEVIEW_ZOOM_IN);
    thumbview_iupdate ();
}

void
imageview_zoom_out (void)
{
    imageview_zoom (imageview, IMAGEVIEW_ZOOM_OUT);
    thumbview_iupdate ();
}

void
imageview_rotate_90 (void)
{
    imageview_rotate (imageview, IMAGEVIEW_ROTATE_90);
}

void
imageview_rotate_180 (void)
{
    imageview_rotate (imageview, IMAGEVIEW_ROTATE_180);
}

void
imageview_rotate_270 (void)
{
    imageview_rotate (imageview, IMAGEVIEW_ROTATE_270);
}

void
imageview_set_fullscreen (void)
{
    imageview_fullscreen_new (imageview);
}

void
imageview_start_slideshow (void)
{
    imageview_slideshow_new (imageview);
}

void
imageview_get_image_frame_size (ImageView * iv, gint * width, gint * height)
{
    g_return_if_fail (width && height);

    *width = *height = 0;

    g_return_if_fail (iv);

/* FIXME !!!!! */
    if (iv->draw_area->allocation.width == 1
	&& iv->draw_area->allocation.height == 1 && iv->is_fullscreen)
    {
	*width = width_backup;
	*height = height_backup;
    }
    else
    {
	*width = iv->draw_area->allocation.width;
	*height = iv->draw_area->allocation.height;
    }
}

void
imageview_zoom_image (ImageView * iv, ImgZoomType zoom,
		      gfloat x_scale, gfloat y_scale)
{
    gint    cx_pos, cy_pos, fwidth, fheight;
    gfloat  src_x_scale, src_y_scale;

    g_return_if_fail (iv);

    src_x_scale = iv->x_scale;
    src_y_scale = iv->y_scale;

    switch (zoom)
    {
      case IMAGEVIEW_ZOOM_IN:
	  if (iv->x_scale < IMG_MAX_SCALE && iv->y_scale < IMG_MAX_SCALE)
	  {
	      iv->x_scale = iv->x_scale + IMG_MIN_SCALE;
	      iv->y_scale = iv->y_scale + IMG_MIN_SCALE;
	  }
	  break;

      case IMAGEVIEW_ZOOM_OUT:
	  if (iv->x_scale > IMG_MIN_SCALE && iv->y_scale > IMG_MIN_SCALE)
	  {
	      iv->x_scale = iv->x_scale - IMG_MIN_SCALE;
	      iv->y_scale = iv->y_scale - IMG_MIN_SCALE;
	  }
	  break;

      case IMAGEVIEW_ZOOM_AUTO:
	  iv->fit_to_frame = TRUE;
	  iv->auto_zoom = TRUE;
	  break;

      case IMAGEVIEW_ZOOM_FIT:
	  iv->fit_to_frame = TRUE;
	  break;

      case IMAGEVIEW_ZOOM_100:
	  iv->x_scale = iv->y_scale = 100;
	  break;
      default:
	  break;
    }

    imageview_get_image_frame_size (iv, &fwidth, &fheight);

    cx_pos = (iv->x_pos - fwidth / 2) * iv->x_scale / src_x_scale;
    cy_pos = (iv->y_pos - fheight / 2) * iv->y_scale / src_y_scale;

    iv->x_pos = cx_pos + fwidth / 2;
    iv->y_pos = cy_pos + fheight / 2;

    if (zoom != IMAGEVIEW_ZOOM_FIT && zoom != IMAGEVIEW_ZOOM_AUTO)
	iv->fit_to_frame = FALSE;

    if (zoom != IMAGEVIEW_ZOOM_AUTO)
	iv->auto_zoom = FALSE;

    iv->zoom = zoom;

    imageview_calc_image (iv, FALSE);
}

void
imageview_moveto (ImageView * iv, gint x, gint y)
{
    g_return_if_fail (iv);

    iv->x_pos = 0 - x;
    iv->y_pos = 0 - y;

    imageview_adjust_pos_in_frame (iv, TRUE);

    imageview_draw_image (iv);
    reset_scrollbar (iv);
}
