/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __IMAGE_INFO_H__
#define __IMAGE_INFO_H__

#include <sys/stat.h>

typedef struct _ImageInfo
{
    gchar          *filename;
    const gchar    *format;
    struct stat     st;
    gint            width;
    gint            height;

    /*
     * reference count 
     */
    guint           ref_count;
}
ImageInfo;

ImageInfo      *image_info_get_from_pixbuf (const gchar * filename,
					    GdkPixbuf * image);
ImageInfo      *image_info_get (const gchar * filename, gint width,
				gint height);
ImageInfo      *image_info_ref (ImageInfo * info);
void            image_info_unref (ImageInfo * info);

#endif /* __IMAGE_INFO_H__ */
