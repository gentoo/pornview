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

#ifndef __GTK_XINE_H__
#define __GTK_XINE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_XINE

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <xine.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern          "C"
{
#endif				/* __cplusplus */

/* speed values */
#define SPEED_PAUSE   XINE_SPEED_PAUSE
#define SPEED_SLOW_4  XINE_SPEED_SLOW_4
#define SPEED_SLOW_2  XINE_SPEED_SLOW_2
#define SPEED_NORMAL  XINE_SPEED_NORMAL
#define SPEED_FAST_2  XINE_SPEED_FAST_2
#define SPEED_FAST_4  XINE_SPEED_FAST_4

#define GTK_XINE(obj)              (GTK_CHECK_CAST ((obj), gtk_xine_get_type (), GtkXine))
#define GTK_XINE_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), gtk_xine_get_type (), GtkXineClass))
#define GTK_IS_XINE(obj)           (GTK_CHECK_TYPE (obj, gtk_xine_get_type ()))
#define GTK_IS_XINE_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), gtk_xine_get_type ()))

    typedef struct _GtkXine GtkXine;
    typedef struct _GtkXineClass GtkXineClass;

    struct _GtkXine
    {
	GtkWidget       widget;

	xine_t         *xine;
	xine_stream_t  *stream;
	xine_event_queue_t *event_queue;

	char            configfile[256];
	xine_vo_driver_t *vo_driver;
	xine_ao_driver_t *ao_driver;
	x11_visual_t    vis;
	double          display_ratio;
	Display        *display;
	int             screen;
	Window          video_window;
	GC              gc;
	pthread_t       thread;
	int             completion_event;
	int             child_pid;

	int             xpos, ypos;

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
    };

    GtkType         gtk_xine_get_type (void);
    GtkWidget      *gtk_xine_new (void);
    void            gtk_xine_set_visibility (GtkXine * gtx,
					     GdkVisibilityState state);
    void            gtk_xine_resize (GtkXine * gtx,
				     gint x, gint y, gint width, gint height);
    gint            gtk_xine_open (GtkXine * gtx, const gchar * mrl);
    gint            gtk_xine_play (GtkXine * gtx, gint pos, gint start_time);
    gint            gtk_xine_trick_mode (GtkXine * gtx,
					 gint mode, gint value);
    gint            gtk_xine_get_stream_info (GtkXine * gtx, gint info);
    const gchar    *gtk_xine_get_meta_info (GtkXine * gtx, gint info);
    gint            gtk_xine_get_pos_length (GtkXine * gtx, gint * pos_stream,	/* 0..65535     */
					     gint * pos_time,	/* milliseconds */
					     gint * length_time);	/* milliseconds */
    void            gtk_xine_stop (GtkXine * gtx);

    gint            gtk_xine_get_error (GtkXine * gtx);
    gint            gtk_xine_get_status (GtkXine * gtx);

    void            gtk_xine_set_param (GtkXine * gtx,
					gint param, gint value);
    gint            gtk_xine_get_param (GtkXine * gtx, gint param);

    gint            gtk_xine_get_audio_lang (GtkXine * gtx,
					     gint channel, gchar * lang);
    gint            gtk_xine_get_spu_lang (GtkXine * gtx,
					   gint channel, gchar * lang);

    void            gtk_xine_set_fullscreen (GtkXine * gtx, gint fullscreen);

    gint            gtk_xine_is_fullscreen (GtkXine * gtx);


    gint            gtk_xine_get_current_frame (GtkXine * gtx,
						gint * width,
						gint * height,
						gint * ratio_code,
						gint * format, uint8_t * img);
    gint            gtk_xine_get_log_section_count (GtkXine * gtx);
    gchar         **gtk_xine_get_log_names (GtkXine * gtx);
    gchar         **gtk_xine_get_log (GtkXine * gtx, gint buf);
    void            gtk_xine_register_log_cb (GtkXine * gtx,
					      xine_log_cb_t cb,
					      void *user_data);

    gchar         **gtk_xine_get_browsable_input_plugin_ids (GtkXine * gtx);
    xine_mrl_t    **gtk_xine_get_browse_mrls (GtkXine * gtx,
					      const gchar * plugin_id,
					      const gchar * start_mrl,
					      gint * num_mrls);
    gchar         **gtk_xine_get_autoplay_input_plugin_ids (GtkXine * gtx);
    gchar         **gtk_xine_get_autoplay_mrls (GtkXine * gtx,
						const gchar * plugin_id,
						gint * num_mrls);

    int             gtk_xine_config_get_first_entry (GtkXine * gtx,
						     xine_cfg_entry_t *
						     entry);
    int             gtk_xine_config_get_next_entry (GtkXine * gtx,
						    xine_cfg_entry_t * entry);
    int             gtk_xine_config_lookup_entry (GtkXine * gtx,
						  const gchar * key,
						  xine_cfg_entry_t * entry);
    void            gtk_xine_config_update_entry (GtkXine * gtx,
						  xine_cfg_entry_t * entry);
    void            gtk_xine_config_load (GtkXine * gtx,
					  const gchar * cfg_filename);
    void            gtk_xine_config_save (GtkXine * gtx,
					  const gchar * cfg_filename);
    void            gtk_xine_config_reset (GtkXine * gtx);

    xine_event_queue_t *gtk_xine_event_new_queue (GtkXine * gtx);

    void            gtk_xine_event_send (GtkXine * gtx,
					 const xine_event_t * event);


    void            gtk_xine_set_speed (GtkXine * gtx, gint speed);
    gint            gtk_xine_get_speed (GtkXine * gtx);
    gint            gtk_xine_get_position (GtkXine * gtx);
    gint            gtk_xine_get_current_time (GtkXine * gtx);
    gint            gtk_xine_get_stream_length (GtkXine * gtx);

    guchar         *gtk_xine_get_current_frame_rgb (GtkXine * gtx,
						    gint * width_ret,
						    gint * height_ret);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* ENABLE_XINE */

#endif				/* __GTK_XINE_H__ */
