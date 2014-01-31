/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __IMAGEVIEW_H__
#define __IMAGEVIEW_H__

#include "image_loader.h"

typedef enum
{
    IMAGEVIEW_ZOOM_100,		/* Real Size */
    IMAGEVIEW_ZOOM_IN,		/* Increase Image Size 5% */
    IMAGEVIEW_ZOOM_OUT,		/* Decrease Image Size 5% */
    IMAGEVIEW_ZOOM_FIT,		/* Fit to Widnow */
    IMAGEVIEW_ZOOM_AUTO		/* Fit to Widnow Only If Image Is Larger */
}
ImgZoomType;

typedef enum
{
    IMAGEVIEW_ROTATE_0,
    IMAGEVIEW_ROTATE_90,
    IMAGEVIEW_ROTATE_180,
    IMAGEVIEW_ROTATE_270
}
ImgRotateType;

#define IMG_MIN_SCALE 5
#define IMG_MAX_SCALE 100000

#define IMGWIN_FULLSCREEN_HIDE_CURSOR_DELAY 1000

typedef struct _ImageView ImageView;

struct _ImageView
{
    GtkWidget      *container;
    GtkWidget      *table;
    GtkWidget      *event_box;

    GtkWidget      *draw_area;
    GtkWidget      *hscrollbar;
    GtkWidget      *vscrollbar;
    GtkWidget      *nav_button;
    GtkWidget      *menu_popup;

    GtkAdjustment  *hadj, *vadj;

    ImageLoader    *loader;
    GdkPixbuf      *image;
    GdkPixmap      *pixmap;
    GdkBitmap      *mask;

    gchar          *image_name;
    gint            image_width;
    gint            image_height;

    gint            width;
    gint            height;

    gboolean        fit_to_frame;
    gboolean        auto_zoom;
    gboolean        keep_aspect;
    ImgZoomType     zoom;
    ImgRotateType   rotate;
    gfloat          x_scale;
    gfloat          y_scale;

    /*
     * information about image dragging
     */
    guint           button;
    gboolean        pressed;
    gboolean        dragging;
    gint            drag_startx;
    gint            drag_starty;
    gint            x_pos;
    gint            y_pos;
    gint            x_pos_drag_start;
    gint            y_pos_drag_start;

    GtkWidget      *fullscreen;
    gboolean        is_fullscreen;
    gint            hide_cursor_timer_id;
    gint            f_direction;
    gint            f_current;
    gint            f_max;
    gint            f_load_timer_id;
    gint            f_load_lock;

    gboolean        is_slideshow;
    gint            slideshow_timer_id;

    gint            rotate_timer_id;
    gint            zoom_timer_id;
    gboolean        is_hide;
};

ImageView      *imageview;

#define IMAGEVIEW imageview
#define IMAGEVIEW_CONTAINER imageview->container
#define IMAGEVIEW_IS_HIDE   imageview->is_hide
#define IMAGEVIEW_ZOOM_TYPE imageview->zoom

void            imageview_create (void);
void            imageview_destroy (void);
void            imageview_stop (void);
void            imageview_set_image (gchar * path);
void            imageview_no_zoom (void);
void            imageview_zoom_fit (void);
void            imageview_zoom_auto (void);
void            imageview_zoom_in (void);
void            imageview_zoom_out (void);
void            imageview_rotate_90 (void);
void            imageview_rotate_180 (void);
void            imageview_rotate_270 (void);

void            imageview_set_fullscreen (void);
void            imageview_start_slideshow (void);

void            imageview_get_image_frame_size (ImageView * iv, gint * width,
						gint * height);
void            imageview_zoom_image (ImageView * iv, ImgZoomType zoom,
				      gfloat x_scale, gfloat y_scale);
void            imageview_moveto (ImageView * iv, gint x, gint y);

#endif /* __IMAGEVIEW_H__ */
