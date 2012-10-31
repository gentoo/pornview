/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __EXIF_VIEW_H__
#define __EXIF_VIEW_H__

#ifdef ENABLE_EXIF

typedef struct ExifView_Tag
{
    GtkWidget      *window;	/* if open in stand alone window, use this */
    GtkWidget      *container;	/* exif view */
    ExifData       *exif_data;
    JPEGData       *jpeg_data;
}
ExifView;


ExifView       *exif_view_create_window (const gchar * filename);
ExifView       *exif_view_create (const gchar * filename);

#endif /* ENABLE_EXIF */

#endif /* __EXIF_VIEW_H__ */
