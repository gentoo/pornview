/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __IMAGE_WIN_H__
#define __IMAGE_WIN_H__

#include "image_loader.h"

typedef struct _ImageWindow ImageWindow;

struct _ImageWindow
{
/* use this to add it and show it */
    GtkWidget      *widget;
    GtkWidget      *eventbox;
    GtkWidget      *image;
    GdkPixmap      *pixmap;
    GdkBitmap      *mask;

/* image actual dimensions (pixels) */
    gint            image_width;
    gint            image_height;
    GdkPixbuf      *pixbuf;

/* allocated size of window (drawing area) */
    gint            window_width;
    gint            window_height;

/* size of scaled image (result) */
    gint            width;
    gint            height;

    gboolean        has_frame;
    ImageLoader    *il;
};

ImageWindow    *image_new (gboolean frame);
void            image_set_path (ImageWindow * iw, const gchar * path);

GdkPixbuf      *image_get_video_pixbuf (void);

#endif /* __IMAGE_WIN_H__ */
