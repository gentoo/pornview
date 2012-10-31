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

#ifndef __THUMB_LOADER_H__
#define __THUMB_LOADER_H__

#include "image_loader.h"

typedef struct _ThumbLoader ThumbLoader;

struct _ThumbLoader
{
    GdkPixbuf      *pixbuf;	/* contains final (scaled) image when done */
    ImageLoader    *il;
    gchar          *path;

    gint            from_cache;
    gfloat          percent_done;

    gint            max_w;
    gint            max_h;

    void            (*func_error) (ThumbLoader *, gpointer);
    void            (*func_done) (ThumbLoader *, gpointer);
    void            (*func_percent) (ThumbLoader *, gfloat, gpointer);

    gpointer        data_error;
    gpointer        data_done;
    gpointer        data_percent;

    gint            idle_done_id;
};


void            thumb_loader_set_error_func (ThumbLoader * tl,
					     void (*func_error) (ThumbLoader
								 *, gpointer),
					     gpointer data_error);
void            thumb_loader_set_percent_func (ThumbLoader * tl,
					       void (*func_percent)
					       (ThumbLoader *, gfloat,
						gpointer),
					       gpointer data_percent);
gint            thumb_loader_start (ThumbLoader * tl,
				    void (*func_done) (ThumbLoader *,
						       gpointer),
				    gpointer data_done);
gint            thumb_loader_to_pixmap (ThumbLoader * tl, GdkPixmap ** pixmap,
					GdkBitmap ** mask);
gint            thumb_loader_get_space (ThumbLoader * tl);
ThumbLoader    *thumb_loader_new (gchar * path, gint width, gint height);
void            thumb_loader_free (ThumbLoader * tl);

#endif /* __THUMB_LOADER_H__ */
