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

#ifndef __NAVWINDOW_H__
#define __NAVWINDOW_H__

#include "imageview.h"

#define NAV_WIN_SIZE 128	/* Max size of the window. */

typedef struct
{
    ImageView      *iv;

    gint            x_root, y_root;

    GtkWidget      *popup_win;
    GtkWidget      *preview;
    GdkPixmap      *pixmap;
    GdkBitmap      *mask;

    GdkGC          *gc;

    gint            image_width, image_height;

    gint            popup_x, popup_y, popup_width, popup_height;
    gint            sqr_x, sqr_y, sqr_width, sqr_height;
    gint            fix_x_pos, fix_y_pos;

    gint            motion_hand_id, keypress_hand_id;

    gdouble         factor;
}
NavWindow;

NavWindow      *navwin_create (ImageView * iv,
			       gfloat x_root,
			       gfloat y_root,
			       GdkPixmap * pixmap, GdkBitmap * mask);

#endif /* __NAVWINDOW_H__ */
