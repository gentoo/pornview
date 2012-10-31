/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "image.h"
#include "file_type.h"
#include "pixbuf_utils.h"

#include "pixmaps/image.xpm"
#include "pixmaps/video.xpm"
#include "pixmaps/video2.xpm"

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
image_size_sync (ImageWindow * iw, gint new_width, gint new_height)
{
    if (iw->window_width == new_width && iw->window_height == new_height)
	return;

    iw->window_width = new_width;
    iw->window_height = new_height;

    gtk_widget_set_usize (iw->image, iw->window_width, iw->window_height);
}

static void
image_draw (ImageWindow * iw)
{
    gint    x, y;

    GdkGC  *gc;

    gc = iw->image->style->black_gc;

    if (!iw->image->window)
	return;

    gdk_draw_rectangle (iw->image->window, gc, TRUE, 0, 0, -1, -1);

    if (iw->pixmap)
    {
	x = (iw->window_width - iw->width) / 2;
	y = (iw->window_height - iw->height) / 2;

	if (iw->mask)
	{
	    gdk_gc_set_clip_mask (gc, iw->mask);
	    gdk_gc_set_clip_origin (gc, x, y);
	}

	gdk_draw_pixmap (iw->image->window, gc, iw->pixmap, 0, 0, x, y, -1,
			 -1);

	if (iw->mask)
	{
	    gdk_gc_set_clip_mask (gc, NULL);
	    gdk_gc_set_clip_origin (gc, 0, 0);
	}

	return;
    }

    if (!iw->pixbuf)
	return;

    x = (iw->window_width - iw->width) / 2;
    y = (iw->window_height - iw->height) / 2;

    gdk_pixbuf_render_to_drawable (iw->pixbuf, iw->image->window,
				   iw->image->style->fg_gc[GTK_STATE_NORMAL],
				   0, 0, x, y, iw->width, iw->height, 0, 0,
				   0);
}

static void
image_pixmap_free (ImageWindow * iw)
{
    if (iw->pixmap)
	gdk_pixmap_unref (iw->pixmap);

    if (iw->mask)
	gdk_bitmap_unref (iw->mask);

    iw->pixmap = NULL;
    iw->mask = NULL;
}

static void
image_pixbuf_free (ImageWindow * iw)
{
    if (iw->pixbuf)
	gdk_pixbuf_unref (iw->pixbuf);

    iw->pixbuf = NULL;
}

static void
image_free (ImageWindow * iw)
{
    image_loader_free (iw->il);
    iw->il = NULL;

    image_pixmap_free (iw);
    image_pixbuf_free (iw);

    g_free (iw);
}

static  gint
image_check_type (ImageWindow * iw, const gchar * path)
{
    gint    file_type;

    if (file_type_is_movie ((gchar *) path))
    {
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data (video_xpm);
	gdk_pixbuf_render_pixmap_and_mask (pixbuf, &(iw->pixmap),
					   &(iw->mask), 127);

	iw->width = gdk_pixbuf_get_width (pixbuf);
	iw->height = gdk_pixbuf_get_height (pixbuf);

	gdk_pixbuf_unref (pixbuf);

	file_type = FILETYPE_VIDEO;
    }
    else
    {
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data (image_xpm);
	gdk_pixbuf_render_pixmap_and_mask (pixbuf, &(iw->pixmap),
					   &(iw->mask), 127);

	iw->width = gdk_pixbuf_get_width (pixbuf);
	iw->height = gdk_pixbuf_get_height (pixbuf);

	gdk_pixbuf_unref (pixbuf);

	file_type = FILETYPE_IMAGE;
    }

    image_draw (iw);
    return file_type;
}

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static  gint
cb_image_size (GtkWidget * widget, GtkAllocation * allocation, gpointer data)
{
    ImageWindow *iw = data;

    image_size_sync (iw, allocation->width, allocation->height);

    return FALSE;
}

static  gint
cb_image_expose (GtkWidget * widget, GdkEventExpose * event, gpointer data)
{
    ImageWindow *iw = data;

    image_draw (iw);

    return TRUE;
}

static void
cb_image_destroy (GtkObject * widget, gpointer data)
{
    ImageWindow *iw = data;

    image_free (iw);
}

static void
cb_image_load_done (ImageLoader * il, gpointer data)
{
    ImageWindow *iw = data;

    image_pixmap_free (iw);
    image_pixbuf_free (iw);

    iw->pixbuf = image_loader_get_pixbuf (iw->il);

    iw->image_width = gdk_pixbuf_get_width (iw->pixbuf);
    iw->image_height = gdk_pixbuf_get_height (iw->pixbuf);

    if (iw->image_width > iw->window_height
	|| iw->image_height > iw->window_height)
    {
	if (((float) iw->window_width / iw->image_width) <
	    ((float) iw->window_height / iw->image_height))
	{
	    iw->width = iw->window_width;
	    iw->height =
		(float) iw->width / iw->image_width * iw->image_height;
	    if (iw->height < 1)
		iw->height = 1;
	}
	else
	{
	    iw->height = iw->window_height;
	    iw->width =
		(float) iw->height / iw->image_height * iw->image_width;
	}
    }
    else
    {
	iw->width = iw->image_width;
	iw->height = iw->image_height;
    }

    iw->pixbuf =
	gdk_pixbuf_scale_simple (iw->pixbuf, iw->width, iw->height, 1);

    image_draw (iw);
}

static void
cb_image_load_error (ImageLoader * il, gpointer data)
{
/* ??? */
    cb_image_load_done (il, data);
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

ImageWindow *
image_new (gint frame)
{
    ImageWindow *iw;

    iw = g_new0 (ImageWindow, 1);
    iw->pixbuf = NULL;
    iw->pixmap = NULL;
    iw->mask = NULL;
    iw->il = NULL;
    iw->has_frame = frame;

    iw->eventbox = gtk_event_box_new ();

    gtk_signal_connect_after (GTK_OBJECT (iw->eventbox), "size_allocate",
			      GTK_SIGNAL_FUNC (cb_image_size), iw);

    if (iw->has_frame)
    {
	iw->widget = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (iw->widget), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (iw->widget), iw->eventbox);
	gtk_widget_show (iw->eventbox);
    }
    else
    {
	iw->widget = iw->eventbox;
    }

    iw->image = gtk_drawing_area_new ();

    gtk_signal_connect (GTK_OBJECT (iw->image), "expose_event",
			GTK_SIGNAL_FUNC (cb_image_expose), iw);

    gtk_container_add (GTK_CONTAINER (iw->eventbox), iw->image);
    gtk_widget_show (iw->image);

    gtk_signal_connect (GTK_OBJECT (iw->widget), "destroy",
			GTK_SIGNAL_FUNC (cb_image_destroy), iw);

    return iw;
}

void
image_set_path (ImageWindow * iw, const gchar * path)
{
    image_loader_free (iw->il);
    iw->il = NULL;

    image_pixmap_free (iw);
    image_pixbuf_free (iw);

    if (image_check_type (iw, path) == FILETYPE_IMAGE)
    {
	iw->il = image_loader_new (path);

	image_loader_set_error_func (iw->il, cb_image_load_error, iw);
	image_loader_set_buffer_size (iw->il, 0);

	if (!image_loader_start (iw->il, cb_image_load_done, iw))
	    cb_image_load_error (iw->il, iw);
    }
}

GdkPixbuf *
image_get_video_pixbuf (void)
{
    return gdk_pixbuf_new_from_xpm_data (video2_xpm);
}
