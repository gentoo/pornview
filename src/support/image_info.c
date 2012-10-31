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

#include "image_info.h"

static void
image_info_finalize (ImageInfo * info)
{
    g_return_if_fail (info);

    if (info->filename)
    {
	g_free (info->filename);
	info->filename = NULL;
    }

}

ImageInfo *
image_info_get_from_pixbuf (const gchar * filename, GdkPixbuf * image)
{
    ImageInfo *ii;
    struct stat st;

    if (stat (filename, &st))
	return NULL;

    ii = g_new0 (ImageInfo, 1);
    ii->filename = g_strdup (filename);

    if (image)
    {
	ii->width = gdk_pixbuf_get_width (image);
	ii->height = gdk_pixbuf_get_height (image);
    }
    else
	ii->width = ii->height = -1;

    ii->ref_count = 1;
    ii->st = st;

    return ii;
}

ImageInfo *
image_info_get (const gchar * filename, gint width, gint height)
{
    ImageInfo *ii;
    struct stat st;

    if (stat (filename, &st))
	return NULL;

    ii = g_new0 (ImageInfo, 1);
    ii->filename = g_strdup (filename);

    ii->width = width;
    ii->height = height;

    ii->ref_count = 1;
    ii->st = st;

    return ii;
}

ImageInfo *
image_info_ref (ImageInfo * info)
{
    g_return_val_if_fail (info, NULL);

    info->ref_count++;

    return info;
}

void
image_info_unref (ImageInfo * info)
{
    g_return_if_fail (info);

    info->ref_count--;

    if (info->ref_count < 1)
    {
	image_info_finalize (info);
    }
}
