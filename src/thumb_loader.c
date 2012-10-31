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

#include "pornview.h"

#include "file_utils.h"
#include "pixbuf_utils.h"

#include "thumb_loader.h"
#include "cache.h"
#include "prefs.h"

static void cb_thumb_loader_error (ImageLoader * il, gpointer data);
static void thumb_loader_setup (ThumbLoader * tl, gchar * path);

/*
 *-----------------------------------------------------------------------------
 * thumbnail routines: creation, caching, and maintenance (private)
 *-----------------------------------------------------------------------------
 */

static  gint
thumb_loader_save_to_cache (ThumbLoader * tl)
{
    gchar  *cache_dir;
    gint    success = FALSE;
    mode_t  mode = 0755;

    if (!tl || !tl->pixbuf)
	return FALSE;

    cache_dir =
	cache_get_location (CACHE_THUMBS, tl->path, FALSE, NULL, &mode);

    if (cache_ensure_dir_exists (cache_dir, mode))
    {
	gchar  *cache_path;

	cache_path =
	    g_strconcat (cache_dir, "/", filename_from_path (tl->path),
			 PORNVIEW_CACHE_THUMB_EXT, NULL);

	success = pixbuf_to_file_as_png (tl->pixbuf, cache_path);
	if (success)
	{
	    struct utimbuf ut;
	    /*
	     * set thumb time to that of source file 
	     */

	    ut.actime = ut.modtime = filetime (tl->path);
	    if (ut.modtime > 0)
	    {
		utime (cache_path, &ut);
	    }
	}

	g_free (cache_path);
    }

    g_free (cache_dir);

    return success;
}

static  gint
thumb_loader_mark_failure (ThumbLoader * tl)
{
    gchar  *cache_dir;
    gint    success = FALSE;
    mode_t  mode = 0755;

    if (!tl)
	return FALSE;

    cache_dir =
	cache_get_location (CACHE_THUMBS, tl->path, FALSE, NULL, &mode);

    if (cache_ensure_dir_exists (cache_dir, mode))
    {
	gchar  *cache_path;
	FILE   *f;

	cache_path =
	    g_strconcat (cache_dir, "/", filename_from_path (tl->path),
			 PORNVIEW_CACHE_THUMB_EXT, NULL);

	f = fopen (cache_path, "w");
	if (f)
	{
	    struct utimbuf ut;

	    fclose (f);

	    ut.actime = ut.modtime = filetime (tl->path);
	    if (ut.modtime > 0)
	    {
		utime (cache_path, &ut);
	    }

	    success = TRUE;
	}

	g_free (cache_path);
    }

    g_free (cache_dir);
    return success;
}

static void
cb_thumb_loader_percent (ImageLoader * il, gfloat percent, gpointer data)
{
    ThumbLoader *tl = data;

    if (tl->func_percent)
	tl->func_percent (tl, percent, tl->data_percent);
}

static void
cb_thumb_loader_done (ImageLoader * il, gpointer data)
{
    ThumbLoader *tl = data;
    GdkPixbuf *pixbuf;
    gint    pw, ph;
    gint    save;

    pixbuf = image_loader_get_pixbuf (tl->il);
    if (!pixbuf)
    {
	cb_thumb_loader_error (tl->il, tl);
	return;
    }

    pw = gdk_pixbuf_get_width (pixbuf);
    ph = gdk_pixbuf_get_height (pixbuf);

    if (tl->from_cache && pw != tl->max_w && ph != tl->max_h)
    {
	/*
	 * requested thumbnail size may have changed, load original 
	 */
	tl->from_cache = FALSE;

	image_loader_free (tl->il);
	thumb_loader_setup (tl, tl->path);

	if (!image_loader_start (tl->il, cb_thumb_loader_done, tl))
	    cb_thumb_loader_error (tl->il, tl);

	return;
    }

    gdk_pixbuf_ref (pixbuf);

    /*
     * scale ?? 
     */

    if (pw > tl->max_w || ph > tl->max_h)
    {
	gint    w, h;

	if (((float) tl->max_w / pw) < ((float) tl->max_h / ph))
	{
	    w = tl->max_w;
	    h = (float) w / pw * ph;
	    if (h < 1)
		h = 1;
	}
	else
	{
	    h = tl->max_h;
	    w = (float) h / ph * pw;
	    if (w < 1)
		w = 1;
	}

	tl->pixbuf =
	    gdk_pixbuf_scale_simple (pixbuf, w, h,
				     (GdkInterpType) conf.thumbnail_quality);
	save = TRUE;
    }
    else
    {
	tl->pixbuf = pixbuf;
	save = FALSE;
    }
    gdk_pixbuf_ref (tl->pixbuf);

    gdk_pixbuf_unref (pixbuf);

    /*
     * save it ? 
     */
    if (conf.enable_thumb_caching && save)
    {
	thumb_loader_save_to_cache (tl);
    }

    if (tl->func_done)
	tl->func_done (tl, tl->data_done);
}

static void
cb_thumb_loader_error (ImageLoader * il, gpointer data)
{
    /*
     * at least some of the image must be available, go to cb_done
     * * the error handling should be settable to this or the #if 0 below, I guess
     */

    cb_thumb_loader_done (il, data);

#if 0
    ThumbLoader *tl = data;

    image_loader_free (tl->il);
    tl->il = NULL;

    if (tl->func_error)
	tl->func_error (tl, tl->data_error);
#endif
}

static void
thumb_loader_setup (ThumbLoader * tl, gchar * path)
{
    tl->il = image_loader_new (path);

    image_loader_set_error_func (tl->il, cb_thumb_loader_error, tl);

    if (tl->func_percent)
	image_loader_set_percent_func (tl->il, cb_thumb_loader_percent, tl);
}

/*
 *-----------------------------------------------------------------------------
 * thumbnail routines: creation, caching, and maintenance (private)
 *-----------------------------------------------------------------------------
 */

void
thumb_loader_set_error_func (ThumbLoader * tl,
			     void (*func_error) (ThumbLoader *, gpointer),
			     gpointer data_error)
{
    if (!tl)
	return;

    tl->func_error = func_error;
    tl->data_error = data_error;
}

void
thumb_loader_set_percent_func (ThumbLoader * tl,
			       void (*func_percent) (ThumbLoader *, gfloat,
						     gpointer),
			       gpointer data_percent)
{
    if (!tl)
	return;

    tl->func_percent = func_percent;
    tl->data_percent = data_percent;
}

gint
thumb_loader_start (ThumbLoader * tl,
		    void (*func_done) (ThumbLoader *, gpointer),
		    gpointer data_done)
{
    gchar  *cache_path = NULL;

    if (!tl || !tl->path)
	return FALSE;

    tl->func_done = func_done;
    tl->data_done = data_done;

    if (conf.enable_thumb_caching)
    {
	cache_path =
	    cache_find_location (CACHE_THUMBS, tl->path,
				 PORNVIEW_CACHE_THUMB_EXT);

	if (cache_path)
	{
	    if (filetime (cache_path) == filetime (tl->path))
	    {
		if (filesize (cache_path) == 0)
		{
		    g_free (cache_path);
		    return FALSE;
		}

	    }
	    else
	    {
		g_free (cache_path);
		cache_path = NULL;
	    }
	}
    }

    if (cache_path)
    {
	thumb_loader_setup (tl, cache_path);
	g_free (cache_path);
	tl->from_cache = TRUE;
    }
    else
    {
	thumb_loader_setup (tl, tl->path);
    }

    if (!image_loader_start (tl->il, cb_thumb_loader_done, tl))
    {
	/*
	 * try from original if cache attempt 
	 */
	if (tl->from_cache)
	{
	    tl->from_cache = FALSE;
	    image_loader_free (tl->il);
	    thumb_loader_setup (tl, tl->path);
	    if (image_loader_start (tl->il, cb_thumb_loader_done, tl))
		return TRUE;
	}
	/*
	 * mark failed thumbnail in cache with 0 byte file 
	 */
	if (conf.enable_thumb_caching)
	{
	    thumb_loader_mark_failure (tl);
	}

	image_loader_free (tl->il);
	tl->il = NULL;
	return FALSE;
    }

    return TRUE;
}

gint
thumb_loader_to_pixmap (ThumbLoader * tl, GdkPixmap ** pixmap,
			GdkBitmap ** mask)
{
    if (!tl || !tl->pixbuf)
	return -1;

    gdk_pixbuf_render_pixmap_and_mask (tl->pixbuf, pixmap, mask, 128);

    return thumb_loader_get_space (tl);
}

gint
thumb_loader_get_space (ThumbLoader * tl)
{
    if (!tl)
	return 0;

    if (tl->pixbuf)
	return (tl->max_w - gdk_pixbuf_get_width (tl->pixbuf));

    return tl->max_w;
}

ThumbLoader *
thumb_loader_new (gchar * path, gint width, gint height)
{
    ThumbLoader *tl;

    if (!path)
	return NULL;

    tl = g_new0 (ThumbLoader, 1);
    tl->path = g_strdup (path);
    tl->from_cache = FALSE;
    tl->percent_done = 0.0;
    tl->max_w = width;
    tl->max_h = height;

    tl->il = NULL;

    tl->idle_done_id = -1;

    return tl;
}

void
thumb_loader_free (ThumbLoader * tl)
{
    if (!tl)
	return;

    if (tl->pixbuf)
	gdk_pixbuf_unref (tl->pixbuf);
    image_loader_free (tl->il);
    g_free (tl->path);

    if (tl->idle_done_id != -1)
	gtk_idle_remove (tl->idle_done_id);

    g_free (tl);
}
