/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GQview.
 * GQview author: John Ellis
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "image_loader.h"

/* the number of bytes to read per idle call (define x 512 bytes) */
#define IMAGE_LOADER_DEFAULT_BUFFER_SIZE 1

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
image_loader_sync_pixbuf (ImageLoader * il)
{
    GdkPixbuf *pb;

    if (!il->loader)
	return;

    pb = gdk_pixbuf_loader_get_pixbuf (il->loader);

    if (pb == il->pixbuf)
	return;

    if (il->pixbuf)
	gdk_pixbuf_unref (il->pixbuf);
    il->pixbuf = pb;
    if (il->pixbuf)
	gdk_pixbuf_ref (il->pixbuf);
}

static void
cb_image_loader_area (GdkPixbufLoader * loader,
		      guint x, guint y, guint w, guint h, gpointer data)
{
    ImageLoader *il = data;

    if (il->func_area_ready)
    {
	if (!il->pixbuf)
	{
	    image_loader_sync_pixbuf (il);
	    if (!il->pixbuf)
	    {
		printf
		    ("critical: area_ready signal with NULL pixbuf (out of mem?)\n");
	    }
	}
	il->func_area_ready (il, x, y, w, h, il->data_area_ready);
    }
}

static void
image_loader_stop (ImageLoader * il)
{
#ifdef USE_GTK2
    GError *err = NULL;
#endif

    if (!il)
	return;

    if (il->idle_id != -1)
    {
	gtk_idle_remove (il->idle_id);
	il->idle_id = -1;
    }

    if (il->loader)
    {
	/*
	 * some loaders do not have a pixbuf till close, order is important here 
	 */

#ifdef USE_GTK2
	gdk_pixbuf_loader_close (il->loader, &err);
#else
	gdk_pixbuf_loader_close (il->loader);
#endif

	image_loader_sync_pixbuf (il);

#ifdef USE_GTK2
	g_object_unref (il->loader);
#else
	gtk_object_unref (GTK_OBJECT (il->loader));
#endif

	il->loader = NULL;
    }

    if (il->load_fd != -1)
    {
	close (il->load_fd);
	il->load_fd = -1;
    }

    il->done = TRUE;
}

static void
image_loader_done (ImageLoader * il)
{
    image_loader_stop (il);

    if (il->func_done)
	il->func_done (il, il->data_done);
}

static  gint
cb_image_loader_done_delay (gpointer data)
{
    ImageLoader *il = data;

    il->idle_done_id = -1;
    image_loader_done (il);
    return FALSE;
}

static void
image_loader_done_delay (ImageLoader * il)
{
    if (il->idle_done_id == -1)
	il->idle_done_id =
	    gtk_idle_add_priority (il->idle_priority,
				   cb_image_loader_done_delay, il);
}

static void
image_loader_error (ImageLoader * il)
{
    image_loader_stop (il);

    if (il->func_error)
	il->func_error (il, il->data_error);
}

static  gint
cb_image_loader_idle (gpointer data)
{
    ImageLoader *il = data;
    guchar  buf1[512];
    guchar  buf2[4096];
    guchar *buf;
    gint    buf_size;
    gint    b;
    gint    c;

#ifdef USE_GTK2
    GError *err = NULL;
#endif

    if (!il)
	return FALSE;

    if (il->idle_id == -1)
	return FALSE;

    if (il->buffer_size == 0)
    {
	buf = buf2;
	buf_size = sizeof (buf2);
	c = 1;
    }
    else
    {
	buf = buf1;
	buf_size = sizeof (buf1);
	c = il->buffer_size ? il->buffer_size : 1;
    }

    while (c > 0)
    {
	b = read (il->load_fd, buf, buf_size);

	if (b == 0)
	{
	    image_loader_done (il);
	    return FALSE;
	}

#ifdef USE_GTK2
	if (b < 0
	    || (b > 0 && !gdk_pixbuf_loader_write (il->loader, buf, b, &err)))
#else
	if (b < 0 || (b > 0 && !gdk_pixbuf_loader_write (il->loader, buf, b)))
#endif
	{
	    image_loader_error (il);
	    return FALSE;
	}

	il->bytes_read += b;

	c--;
    }

    if (il->func_percent && il->bytes_total > 0)
    {
	il->func_percent (il, (gfloat) il->bytes_read / il->bytes_total,
			  il->data_percent);
    }

    return TRUE;
}

static  gint
image_loader_begin (ImageLoader * il)
{
    guchar  buf1[512];
    guchar  buf2[4096];
    guchar *buf;
    gint    buf_size;
    int     b;

#ifdef USE_GTK2
    GError *err = NULL;
#endif

    if (!il->loader || il->pixbuf)
	return FALSE;

    if (il->buffer_size == 0)
    {
	buf = buf2;
	buf_size = sizeof (buf2);
    }
    else
    {
	buf = buf1;
	buf_size = sizeof (buf1);
    }

    b = read (il->load_fd, buf, buf_size);

    if (b < 1)
    {
	image_loader_stop (il);
	return FALSE;
    }

#ifdef USE_GTK2
    if (gdk_pixbuf_loader_write (il->loader, buf, b, &err))
#else
    if (gdk_pixbuf_loader_write (il->loader, buf, b))
#endif
    {
	il->bytes_read += b;

	if (b < sizeof (buf))
	{
	    /*
	     * end of file already 
	     */
	    image_loader_stop (il);

	    if (!il->pixbuf)
		return FALSE;

	    image_loader_done_delay (il);
	    return TRUE;
	}
	else
	{
	    /*
	     * larger file 
	     */

	    /*
	     * read until size is known 
	     */
	    while (il->loader && !gdk_pixbuf_loader_get_pixbuf (il->loader)
		   && b > 0)
	    {
		b = read (il->load_fd, buf, buf_size);
		if (b < 0 || (b > 0
#ifdef USE_GTK2
			      && !gdk_pixbuf_loader_write (il->loader, buf, b,
							   &err)))
#else
			      && !gdk_pixbuf_loader_write (il->loader, buf,
							   b)))
#endif
		{
		    image_loader_stop (il);
		    return FALSE;
		}
		il->bytes_read += b;
	    }
	    if (!il->pixbuf)
		image_loader_sync_pixbuf (il);

	    if (il->bytes_read == il->bytes_total || b < sizeof (buf))
	    {
		/*
		 * done, handle (broken) loaders that do not have pixbuf till close 
		 */
		image_loader_stop (il);

		if (!il->pixbuf)
		    return FALSE;

		image_loader_done_delay (il);
		return TRUE;
	    }

	    if (!il->pixbuf)
	    {
		image_loader_stop (il);
		return FALSE;
	    }

	    /*
	     * finally, progressive loading :) 
	     */
	    il->idle_id =
		gtk_idle_add_priority (il->idle_priority,
				       cb_image_loader_idle, il);
	    return TRUE;
	}
    }
    else
    {
	image_loader_stop (il);
	return FALSE;
    }

    return TRUE;
}

static  gint
image_loader_setup (ImageLoader * il)
{
    struct stat st;

    if (!il)
	return FALSE;

    il->load_fd = open (il->path, O_RDONLY | O_NONBLOCK);
    if (il->load_fd == -1)
	return FALSE;

    if (fstat (il->load_fd, &st) == 0)
    {
	il->bytes_total = st.st_size;
    }

    il->loader = gdk_pixbuf_loader_new ();

#ifdef USE_GTK2
    g_signal_connect (G_OBJECT (il->loader), "area_updated",
		      G_CALLBACK (cb_image_loader_area), il);
#else
    gtk_signal_connect (GTK_OBJECT (il->loader), "area_updated",
			GTK_SIGNAL_FUNC (cb_image_loader_area), il);
#endif

    return image_loader_begin (il);
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

ImageLoader *
image_loader_new (const gchar * path)
{
    ImageLoader *il;

    if (!path)
	return NULL;

    il = g_new0 (ImageLoader, 1);
    if (path)
	il->path = g_strdup (path);
    il->pixbuf = NULL;
    il->idle_id = -1;
    il->idle_priority = G_PRIORITY_DEFAULT_IDLE;
    il->done = FALSE;
    il->loader = NULL;

    il->bytes_read = 0;
    il->bytes_total = 0;

    il->idle_done_id = -1;

    il->buffer_size = IMAGE_LOADER_DEFAULT_BUFFER_SIZE;

    return il;
}

void
image_loader_free (ImageLoader * il)
{
    if (!il)
	return;

    image_loader_stop (il);
    if (il->idle_done_id != -1)
	gtk_idle_remove (il->idle_done_id);
    if (il->pixbuf)
	gdk_pixbuf_unref (il->pixbuf);
    g_free (il->path);
    g_free (il);
}

/* don't forget to gdk_pixbuf_ref() it if you want to use it after image_loader_free() */
GdkPixbuf *
image_loader_get_pixbuf (ImageLoader * il)
{
    if (!il)
	return NULL;

    return il->pixbuf;
}

void
image_loader_set_area_ready_func (ImageLoader * il,
				  void (*func_area_ready) (ImageLoader *,
							   guint, guint,
							   guint, guint,
							   gpointer),
				  gpointer data_area_ready)
{
    if (!il)
	return;

    il->func_area_ready = func_area_ready;
    il->data_area_ready = data_area_ready;
}

void
image_loader_set_error_func (ImageLoader * il,
			     void (*func_error) (ImageLoader *, gpointer),
			     gpointer data_error)
{
    if (!il)
	return;

    il->func_error = func_error;
    il->data_error = data_error;
}

void
image_loader_set_percent_func (ImageLoader * il,
			       void (*func_percent) (ImageLoader *, gfloat,
						     gpointer),
			       gpointer data_percent)
{
    if (!il)
	return;

    il->func_percent = func_percent;
    il->data_percent = data_percent;
}

void
image_loader_set_buffer_size (ImageLoader * il, guint size)
{
    if (!il)
	return;

    il->buffer_size = size ? size : 1;
}

void
image_loader_set_priority (ImageLoader * il, gint priority)
{
    if (!il)
	return;

    il->idle_priority = priority;
}

gint
image_loader_start (ImageLoader * il,
		    void (*func_done) (ImageLoader *, gpointer),
		    gpointer data_done)
{
    if (!il)
	return FALSE;

    if (!il->path)
	return FALSE;

    il->func_done = func_done;
    il->data_done = data_done;

    return image_loader_setup (il);
}

gfloat
image_loader_get_percent (ImageLoader * il)
{
    if (!il || il->bytes_total == 0)
	return 0.0;

    return (gfloat) il->bytes_read / il->bytes_total;
}

gint
image_loader_get_is_done (ImageLoader * il)
{
    if (!il)
	return FALSE;

    return il->done;
}

gint
image_load_dimensions (const gchar * path, gint * width, gint * height)
{
    ImageLoader *il;
    gint    success;

    il = image_loader_new (path);

    success = image_loader_start (il, NULL, NULL);

    if (success && il->pixbuf)
    {
	if (width)
	    *width = gdk_pixbuf_get_width (il->pixbuf);
	if (height)
	    *height = gdk_pixbuf_get_height (il->pixbuf);;
    }
    else
    {
	if (width)
	    *width = -1;
	if (height)
	    *height = -1;
    }

    image_loader_free (il);

    return success;
}
