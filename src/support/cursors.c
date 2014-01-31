/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#include "cursors.h"

#include "cursors/hand-open-data.xbm"
#include "cursors/hand-open-mask.xbm"
#include "cursors/hand-closed-data.xbm"
#include "cursors/hand-closed-mask.xbm"
#include "cursors/void-data.xbm"
#include "cursors/void-mask.xbm"

static struct
{
    char   *data;
    char   *mask;
    int     data_width;
    int     data_height;
    int     mask_width;
    int     mask_height;
    int     hot_x, hot_y;
}

cursors[] =
{
    {
    hand_open_data_bits, hand_open_mask_bits,
	    hand_open_data_width, hand_open_data_height,
	    hand_open_mask_width, hand_open_mask_height,
	    hand_open_data_width / 2, hand_open_data_height / 2}
    ,
    {
    hand_closed_data_bits, hand_closed_mask_bits,
	    hand_closed_data_width, hand_closed_data_height,
	    hand_closed_mask_width, hand_closed_mask_height,
	    hand_closed_data_width / 2, hand_closed_data_height / 2}
    ,
    {
    void_data_bits, void_mask_bits,
	    void_data_width, void_data_height,
	    void_mask_width, void_mask_height, void_data_width / 2,
	    void_data_height / 2}
    ,
    {
    NULL, NULL, 0, 0, 0, 0}
};

/**
 * cursor_get:
 * @window: Window whose screen and colormap determine the cursor's.
 * @type: A cursor type.
 *
 * Creates a cursor.
 *
 * Return value: The newly-created cursor.
 **/
GdkCursor *
cursor_get (GdkWindow * window, CursorType type)
{
    GdkBitmap *data;
    GdkBitmap *mask;
    GdkColor black, white;
    GdkCursor *cursor;

    g_return_val_if_fail (window != NULL, NULL);
    g_return_val_if_fail (type >= 0 && type < CURSOR_NUM_CURSORS, NULL);

    g_assert (cursors[type].data_width == cursors[type].mask_width);
    g_assert (cursors[type].data_height == cursors[type].mask_height);

    data = gdk_bitmap_create_from_data (window,
					cursors[type].data,
					cursors[type].data_width,
					cursors[type].data_height);
    mask = gdk_bitmap_create_from_data (window,
					cursors[type].mask,
					cursors[type].mask_width,
					cursors[type].mask_height);

    g_assert (data != NULL && mask != NULL);

    gdk_color_black (gdk_window_get_colormap (window), &black);
    gdk_color_white (gdk_window_get_colormap (window), &white);

    cursor = gdk_cursor_new_from_pixmap (data, mask, &white, &black,
					 cursors[type].hot_x,
					 cursors[type].hot_y);
    g_assert (cursor != NULL);

    gdk_bitmap_unref (data);
    gdk_bitmap_unref (mask);

    return cursor;
}
