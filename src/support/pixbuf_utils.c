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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <png.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "pixbuf_utils.h"

gboolean
pixbuf_to_file_as_png (GdkPixbuf * pixbuf, gchar * filename)
{
    FILE   *handle;
    char   *buffer;
    gboolean has_alpha;
    int     width, height, depth, rowstride;
    guchar *pixels;
    png_structp png_ptr;
    png_infop info_ptr;
    png_text text[2];
    int     i;

    g_return_val_if_fail (pixbuf != NULL, FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (filename[0] != '\0', FALSE);

    handle = fopen (filename, "wb");
    if (handle == NULL)
	return FALSE;

    png_ptr =
	png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
	fclose (handle);
	return FALSE;
    }

    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL)
    {
	png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
	fclose (handle);
	return FALSE;
    }

    if (setjmp (png_jmpbuf(png_ptr)))
    {
	png_destroy_write_struct (&png_ptr, &info_ptr);
	fclose (handle);
	return FALSE;
    }

    png_init_io (png_ptr, handle);

    has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    depth = gdk_pixbuf_get_bits_per_sample (pixbuf);
    pixels = gdk_pixbuf_get_pixels (pixbuf);
    rowstride = gdk_pixbuf_get_rowstride (pixbuf);

    png_set_IHDR (png_ptr, info_ptr, width, height,
		  depth, PNG_COLOR_TYPE_RGB_ALPHA,
		  PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    /*
     * Some text to go with the png image
     */
    text[0].key = "Title";
    text[0].text = filename;
    text[0].compression = PNG_TEXT_COMPRESSION_NONE;
    text[1].key = "Software";
    text[1].text = "Pornview";
    text[1].compression = PNG_TEXT_COMPRESSION_NONE;
    png_set_text (png_ptr, info_ptr, text, 2);

    /*
     * Write header data
     */
    png_write_info (png_ptr, info_ptr);

    /*
     * if there is no alpha in the data, allocate buffer to expand into
     */
    if (has_alpha)
	buffer = NULL;
    else
	buffer = g_malloc (4 * width);

    /*
     * pump the raster data into libpng, one scan line at a time
     */
    for (i = 0; i < height; i++)
    {
	if (has_alpha)
	{
	    png_bytep row_pointer = pixels;
	    png_write_row (png_ptr, row_pointer);
	}
	else
	{
	    /*
	     * expand RGB to RGBA using an opaque alpha value
	     */
	    int     x;
	    char   *buffer_ptr = buffer;
	    char   *source_ptr = pixels;
	    for (x = 0; x < width; x++)
	    {
		*buffer_ptr++ = *source_ptr++;
		*buffer_ptr++ = *source_ptr++;
		*buffer_ptr++ = *source_ptr++;
		*buffer_ptr++ = 255;
	    }
	    png_write_row (png_ptr, (png_bytep) buffer);
	}
	pixels += rowstride;
    }

    png_write_end (png_ptr, info_ptr);
    png_destroy_write_struct (&png_ptr, &info_ptr);

    g_free (buffer);

    fclose (handle);
    return TRUE;
}

void
pixbuf_render_pixmap_and_mask (GdkPixbuf * pixbuf, GdkPixmap ** pixmap_return,
			       GdkBitmap ** mask_return, GdkInterpType dither)
{
    gint    width;
    gint    height;

    g_return_if_fail (pixbuf != NULL);

    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);

    if (pixmap_return)
    {
	GdkGC  *gc;

	*pixmap_return = gdk_pixmap_new (NULL, width, height,
					 gdk_rgb_get_visual ()->depth);
	gc = gdk_gc_new (*pixmap_return);
	gdk_pixbuf_render_to_drawable (pixbuf, *pixmap_return, gc,
				       0, 0, 0, 0,
				       width, height, dither, 0, 0);
	gdk_gc_unref (gc);
    }

    if (mask_return)
    {
	if (gdk_pixbuf_get_has_alpha (pixbuf))
	{
	    *mask_return = gdk_pixmap_new (NULL, width, height, 1);
	    gdk_pixbuf_render_threshold_alpha (pixbuf, *mask_return,
					       0, 0, 0, 0,
					       width, height, 127);
	}
	else

	    *mask_return = NULL;
    }
}

GtkWidget *
pixbuf_create_pixmap_from_xpm_data (const char **data)
{
    GdkPixbuf *pixbuf = NULL;
    GdkPixmap *pixmap = NULL;
    GdkBitmap *mask = NULL;
    GtkWidget *iconw = NULL;

    pixbuf = gdk_pixbuf_new_from_xpm_data (data);
    gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &mask, 127);
    gdk_pixbuf_unref (pixbuf);
    iconw = gtk_pixmap_new (pixmap, mask);

    return iconw;
}

/*
 * Returns a copy of pixbuf src rotated 90 degrees clockwise or 90
 * counterclockwise.
 */
GdkPixbuf *
pixbuf_copy_rotate_90 (GdkPixbuf * src, gboolean counter_clockwise)
{
    GdkPixbuf *dest;
    gint    has_alpha;
    gint    src_w, src_h, src_rs;
    gint    dest_w, dest_h, dest_rs;
    guchar *src_pix, *src_p;
    guchar *dest_pix, *dest_p;
    gint    i, j;
    gint    a;

    if (!src)
	return NULL;

    src_w = gdk_pixbuf_get_width (src);
    src_h = gdk_pixbuf_get_height (src);
    has_alpha = gdk_pixbuf_get_has_alpha (src);
    src_rs = gdk_pixbuf_get_rowstride (src);
    src_pix = gdk_pixbuf_get_pixels (src);

    dest_w = src_h;
    dest_h = src_w;
    dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, dest_w, dest_h);
    dest_rs = gdk_pixbuf_get_rowstride (dest);
    dest_pix = gdk_pixbuf_get_pixels (dest);

    a = (has_alpha ? 4 : 3);

    for (i = 0; i < src_h; i++)
    {
	src_p = src_pix + (i * src_rs);
	for (j = 0; j < src_w; j++)
	{
	    if (counter_clockwise)
		dest_p = dest_pix + ((dest_h - j - 1) * dest_rs) + (i * a);
	    else
		dest_p = dest_pix + (j * dest_rs) + ((dest_w - i - 1) * a);

	    *(dest_p++) = *(src_p++);	/* r */
	    *(dest_p++) = *(src_p++);	/* g */
	    *(dest_p++) = *(src_p++);	/* b */
	    if (has_alpha)
		*(dest_p) = *(src_p++);	/* a */
	}
    }

    return dest;
}

/*
 * Returns a copy of pixbuf mirrored and or flipped.
 * TO do a 180 degree rotations set both mirror and flipped TRUE
 * if mirror and flip are FALSE, result is a simple copy.
 */
GdkPixbuf *
pixbuf_copy_mirror (GdkPixbuf * src, gboolean mirror, gboolean flip)
{
    GdkPixbuf *dest;
    gint    has_alpha;
    gint    w, h;
    gint    src_rs, dest_rs;
    guchar *src_pix, *dest_pix;
    guchar *src_p, *dest_p;
    gint    i, j;
    gint    a;

    if (!src)
	return NULL;

    w = gdk_pixbuf_get_width (src);
    h = gdk_pixbuf_get_height (src);
    has_alpha = gdk_pixbuf_get_has_alpha (src);
    src_rs = gdk_pixbuf_get_rowstride (src);
    src_pix = gdk_pixbuf_get_pixels (src);

    dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, w, h);
    dest_rs = gdk_pixbuf_get_rowstride (dest);
    dest_pix = gdk_pixbuf_get_pixels (dest);

    a = has_alpha ? 4 : 3;

    for (i = 0; i < h; i++)
    {
	src_p = src_pix + (i * src_rs);
	if (flip)
	    dest_p = dest_pix + ((h - i - 1) * dest_rs);
	else
	    dest_p = dest_pix + (i * dest_rs);

	if (mirror)
	{
	    dest_p += (w - 1) * a;
	    for (j = 0; j < w; j++)
	    {
		*(dest_p++) = *(src_p++);	/* r */
		*(dest_p++) = *(src_p++);	/* g */
		*(dest_p++) = *(src_p++);	/* b */
		if (has_alpha)
		    *(dest_p) = *(src_p++);	/* a */
		dest_p -= (a + 3);
	    }
	}
	else
	{
	    for (j = 0; j < w; j++)
	    {
		*(dest_p++) = *(src_p++);	/* r */
		*(dest_p++) = *(src_p++);	/* g */
		*(dest_p++) = *(src_p++);	/* b */
		if (has_alpha)
		    *(dest_p++) = *(src_p++);	/* a */
	    }
	}
    }

    return dest;
}
