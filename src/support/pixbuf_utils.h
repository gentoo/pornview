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

#ifndef __PIXBUF_UTILS_H__
#define __PIXBUF_UTILS_H__

gboolean        pixbuf_to_file_as_png (GdkPixbuf * pixbuf, gchar * filename);
void            pixbuf_render_pixmap_and_mask (GdkPixbuf * pixbuf,
					       GdkPixmap ** pixmap_return,
					       GdkBitmap ** mask_return,
					       GdkInterpType dither);
GtkWidget      *pixbuf_create_pixmap_from_xpm_data (const char **data);
GdkPixbuf      *pixbuf_copy_rotate_90 (GdkPixbuf * src,
				       gboolean counter_clockwise);
GdkPixbuf      *pixbuf_copy_mirror (GdkPixbuf * src, gboolean mirror,
				    gboolean flip);

#endif /* __PIXBUF_UTILS_H__ */
