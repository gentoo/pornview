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

#ifndef __IMAGE_LOADER_H__
#define __IMAGE_LOADER_H__

typedef struct _ImageLoader ImageLoader;

struct _ImageLoader
{
    GdkPixbuf      *pixbuf;
    gchar          *path;

    gint            bytes_read;
    gint            bytes_total;

    guint           buffer_size;

    gint            done;
    gint            idle_id;
    gint            idle_priority;

    GdkPixbufLoader *loader;
    gint            load_fd;

    void            (*func_area_ready) (ImageLoader *, guint x, guint y,
					guint w, guint h, gpointer);
    void            (*func_error) (ImageLoader *, gpointer);
    void            (*func_done) (ImageLoader *, gpointer);
    void            (*func_percent) (ImageLoader *, gfloat, gpointer);

    gpointer        data_area_ready;
    gpointer        data_error;
    gpointer        data_done;
    gpointer        data_percent;

    gint            idle_done_id;
};

ImageLoader    *image_loader_new (const gchar * path);
void            image_loader_free (ImageLoader * il);

void            image_loader_set_area_ready_func (ImageLoader * il,
						  void (*func_area_ready)
						  (ImageLoader *, guint,
						   guint, guint, guint,
						   gpointer),
						  gpointer data_area_ready);
void            image_loader_set_error_func (ImageLoader * il,
					     void (*func_error) (ImageLoader
								 *, gpointer),
					     gpointer data_error);
void            image_loader_set_percent_func (ImageLoader * il,
					       void (*func_percent)
					       (ImageLoader *, gfloat,
						gpointer),
					       gpointer data_percent);
void            image_loader_set_buffer_size (ImageLoader * il, guint size);

/* this only has effect if used before image_loader_start()
 * default is G_PRIORITY_DEFAULT_IDLE
 */
void            image_loader_set_priority (ImageLoader * il, gint priority);
gint            image_loader_start (ImageLoader * il,
				    void (*func_done) (ImageLoader *,
						       gpointer),
				    gpointer data_done);
GdkPixbuf      *image_loader_get_pixbuf (ImageLoader * il);
gfloat          image_loader_get_percent (ImageLoader * il);
gint            image_loader_get_is_done (ImageLoader * il);
gint            image_load_dimensions (const gchar * path, gint * width,
				       gint * height);

#endif /* __IMAGE_LOADER_H__ */
