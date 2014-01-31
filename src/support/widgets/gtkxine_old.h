/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 *  These codes are mostly taken from xine project.
 */

#ifndef __GTK_XINE_OLD_H__
#define __GTK_XINE_OLD_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_XINE_OLD

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <xine.h>
#include <gtk/gtk.h>
#include <xine/video_out_x11.h>

#ifdef __cplusplus
extern          "C"
{
#endif				/* __cplusplus */

#define GTK_XINE(obj)            (GTK_CHECK_CAST ((obj), gtk_xine_get_type (), GtkXine))
#define GTK_XINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), gtk_xine_get_type (), GtkXineClass))
#define GTK_IS_XINE(obj)         (GTK_CHECK_TYPE (obj, gtk_xine_get_type ()))
#define GTK_IS_XINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), gtk_xine_get_type ()))

    typedef enum
    {
	GTK_XINE_STATUS_EXIT = -1,
	GTK_XINE_STATUS_IDLE
    }
    GtkXineStatus;

    typedef struct _GtkXine GtkXine;
    typedef struct _GtkXineClass GtkXineClass;

    struct _GtkXine
    {
	GtkWidget       widget;

	xine_t         *xine;
	gchar           configfile[256];
	config_values_t *config;
	vo_driver_t    *vo_driver;
	ao_driver_t    *ao_driver;
	x11_visual_t    vis;
	Display        *display;
	gint            screen;
	Window          video_window;
	GC              gc;
	pthread_t       thread;
	gint            completion_event;

	gint            xpos, ypos;

	gint            status;

	/*
	 * fullscreen stuff
	 */
	int             fullscreen_mode;
	int             fullscreen_width, fullscreen_height;
	Window          fullscreen_window, toplevel;
	Cursor          no_cursor;

	GdkVisibilityState visibility;
    };

    struct _GtkXineClass
    {
	GtkWidgetClass  parent_class;

	void            (*play) (GtkXine * gtx);
	void            (*stop) (GtkXine * gtx);
	void            (*playback_finished) (GtkXine * gtx);
	gchar          *(*need_next_mrl) (GtkXine * gtx);
	void            (*branched) (GtkXine * gtx);
    };

    GtkType         gtk_xine_get_type (void);
    GtkWidget      *gtk_xine_new (void);
    void            gtk_xine_set_visibility (GtkXine * gtx,
					     GdkVisibilityState state);
    void            gtk_xine_resize (GtkXine * gtx,
				     gint x, gint y, gint width, gint height);

    void            gtk_xine_show_name (GtkXine * gtx, gchar * name);

    gint            gtk_xine_play (GtkXine * gtx,
				   const gchar * mrl,
				   gint pos, gint start_time);
    void            gtk_xine_set_speed (GtkXine * gtx, gint speed);
    gint            gtk_xine_get_speed (GtkXine * gtx);
    gint            gtk_xine_get_position (GtkXine * gtx);
    void            gtk_xine_stop (GtkXine * gtx);
    void            gtk_xine_set_fullscreen (GtkXine * gtx,
					     gboolean fullscreen);
    gint            gtk_xine_is_fullscreen (GtkXine * gtx);
    void            gtk_xine_set_audio_channel (GtkXine * gtx,
						gint audio_channel);
    gint            gtk_xine_get_audio_channel (GtkXine * gtx);

    gchar         **gtk_xine_get_autoplay_plugins (GtkXine * gtx);

    gint            gtk_xine_get_current_time (GtkXine * gtx);
    gint            gtk_xine_get_stream_length (GtkXine * gtx);

    gint            gtk_xine_is_playing (GtkXine * gtx);
    gint            gtk_xine_is_seekable (GtkXine * gtx);
    void            gtk_xine_save_config (GtkXine * gtx);
    void            gtk_xine_set_video_property (GtkXine * gtx,
						 gint property, gint value);
    gint            gtk_xine_get_video_property (GtkXine * gtx,
						 gint property);

    gint            gtk_xine_get_log_section_count (GtkXine * gtx);
    gchar         **gtk_xine_get_log_names (GtkXine * gtx);
    gchar         **gtk_xine_get_log (GtkXine * gtx, gint buf);

    gchar         **gtk_xine_get_browsable_input_plugin_ids (GtkXine * gtx);
    gchar         **gtk_xine_get_autoplay_input_plugin_ids (GtkXine * gtx);
    gchar         **gtk_xine_get_autoplay_mrls (GtkXine * gtx,
						gchar * plugin_id,
						gint * num_mrls);

    guchar         *gtk_xine_get_current_frame_rgb (GtkXine * gtx,
						    gint * width_ret,
						    gint * height_ret);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* ENABLE_XINE_OLD */

#endif				/* __GTK_XINE_OLD_H__ */
