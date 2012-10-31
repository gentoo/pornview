/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                        (c) 2002,2003                         (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)

#ifdef ENABLE_XINE
#include "gtkxine.h"
#else
#include "gtkxine_old.h"
#endif

#include "zalbum.h"

#include "button.h"
#include "file_utils.h"
#include "image_info.h"
#include "pixbuf_utils.h"

#include "videoplay.h"
#include "cache.h"
#include "browser.h"
#include "comment_view.h"
#include "prefs.h"
#include "thumbview.h"

#include "pixmaps/play.xpm"
#include "pixmaps/pause.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/ff.xpm"
#include "pixmaps/fs.xpm"
#include "pixmaps/prev.xpm"
#include "pixmaps/next.xpm"

VideoPlay *videoplay = NULL;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
update_seek_buttons (VideoPlay * vp)
{
    switch (vp->status)
    {
      case VIDEOPLAY_STATUS_NULL:
	  gtk_widget_set_sensitive (vp->play_btn, FALSE);
	  gtk_widget_set_sensitive (vp->stop_btn, FALSE);
	  gtk_widget_set_sensitive (vp->pause_btn, FALSE);
	  gtk_widget_set_sensitive (vp->ff_btn, FALSE);
	  gtk_widget_set_sensitive (vp->fs_btn, FALSE);
	  gtk_widget_set_sensitive (vp->seek, FALSE);
	  break;

      case VIDEOPLAY_STATUS_PLAYING:
	  gtk_widget_set_sensitive (vp->play_btn, FALSE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, TRUE);
	  gtk_widget_set_sensitive (vp->ff_btn, TRUE);
	  gtk_widget_set_sensitive (vp->fs_btn, TRUE);
	  gtk_widget_set_sensitive (vp->seek, TRUE);
	  break;

      case VIDEOPLAY_STATUS_SLOW:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, TRUE);
	  gtk_widget_set_sensitive (vp->ff_btn, TRUE);
	  gtk_widget_set_sensitive (vp->fs_btn, TRUE);
	  gtk_widget_set_sensitive (vp->seek, TRUE);
	  break;

      case VIDEOPLAY_STATUS_PAUSE:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, FALSE);
	  gtk_widget_set_sensitive (vp->ff_btn, TRUE);
	  gtk_widget_set_sensitive (vp->fs_btn, TRUE);
	  gtk_widget_set_sensitive (vp->seek, FALSE);
	  break;

      case VIDEOPLAY_STATUS_STOP:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, FALSE);
	  gtk_widget_set_sensitive (vp->pause_btn, FALSE);
	  gtk_widget_set_sensitive (vp->ff_btn, FALSE);
	  gtk_widget_set_sensitive (vp->fs_btn, FALSE);
	  gtk_widget_set_sensitive (vp->seek, FALSE);
	  break;

      case VIDEOPLAY_STATUS_FORWARD:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, TRUE);
	  gtk_widget_set_sensitive (vp->ff_btn, TRUE);
	  gtk_widget_set_sensitive (vp->fs_btn, TRUE);
	  gtk_widget_set_sensitive (vp->seek, TRUE);
	  break;
    }

    gtk_widget_set_sensitive (vp->previous_btn, thumbview_is_prev ());
    gtk_widget_set_sensitive (vp->next_btn, thumbview_is_next ());
}

static void
videoplay_print_status (VideoPlay * vp)
{
    gchar  *status = NULL;
    gchar  *buf = NULL;

    if (vp->status == VIDEOPLAY_STATUS_NULL)
	buf = g_strdup (" ");
    else if (vp->status == VIDEOPLAY_STATUS_STOP)
	status = g_strdup (_("stop"));
    else
    {
	switch (gtk_xine_get_speed (GTK_XINE (vp->xine)))
	{
	  case SPEED_NORMAL:
	      status = g_strdup (_("play"));
	      break;

	  case SPEED_FAST_2:
	      status = g_strdup (_("forward x2"));
	      break;

	  case SPEED_FAST_4:
	      status = g_strdup (_("forward x4"));
	      break;

	  case SPEED_PAUSE:
	      status = g_strdup (_("pause"));
	      break;

	  case SPEED_SLOW_2:
	      status = g_strdup (_("forward x0.5"));
	      break;

	  case SPEED_SLOW_4:
	      status = g_strdup (_("forward x0.25"));
	      break;
	}
    }

    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1);

    if (vp->video_name)
    {
	buf = g_strdup_printf (_("  %s"), g_basename (vp->video_name));
	gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1, buf);
	g_free (buf);
    }
    else
	gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1, "");

    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1);

    if (status)
	buf = g_strdup_printf (_("  [ %s ]"), status);

    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1, buf);

    if (status)
	g_free (status);

    if (buf)
	g_free (buf);
}

#ifdef ENABLE_XINE
static char *
time_to_string (int time)
{
    int     sec, min, hour;
    gchar  *string;

    sec = time % 60;
    time = time - sec;
    min = (time % (60 * 60)) / 60;
    time = time - (min * 60);
    hour = time / (60 * 60);

    string = g_strdup_printf ("%02d:%02d:%02d", hour, min, sec);

    return string;
}
#endif

static void
update_seek_time (VideoPlay * vp)
{
    if (vp->status != VIDEOPLAY_STATUS_STOP
	&& vp->status != VIDEOPLAY_STATUS_NULL)
    {
	static char buf[30];
	gint    current, total;
#ifdef ENABLE_XINE_OLD
	gint    current_h;
	gint    current_m;
	gint    current_s;
	gint    total_h;
	gint    total_m;
	gint    total_s;

	current = gtk_xine_get_current_time (GTK_XINE (vp->xine));
	total = gtk_xine_get_stream_length (GTK_XINE (vp->xine));

	current_h = current / 3600;
	current_m = (current % 3600) / 60;
	current_s = current % 60;
	total_h = total / 3600;
	total_m = (total % 3600) / 60;
	total_s = total % 60;

	sprintf (buf, "%02d:%02d:%02d of %02d:%02d:%02d", current_h,
		 current_m, current_s, total_h, total_m, total_s);
#else
	current = gtk_xine_get_current_time (GTK_XINE (vp->xine)) / 1000;
	total = gtk_xine_get_stream_length (GTK_XINE (vp->xine)) / 1000;

	sprintf (buf, "%s of %s", time_to_string (current),
		 time_to_string (total));
#endif

	gtk_label_set_text (GTK_LABEL (vp->seek_label), buf);
    }
    else
	gtk_label_set_text (GTK_LABEL (vp->seek_label),
			    "00:00:00 of 00:00:00");

    update_seek_buttons (vp);
    thumbview_vupdate ();
    videoplay_print_status (vp);
}

static void
free_rgb_buffer (guchar * pixels, gpointer data)
{
    g_free (pixels);
}

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static int
cb_update_seek (gpointer data)
{
    VideoPlay *vp = data;

    if (vp->is_hide)
	return TRUE;

    update_seek_time (vp);

    if (vp->seek_lock == FALSE)
    {
	if (vp->status != VIDEOPLAY_STATUS_STOP
	    && vp->status != VIDEOPLAY_STATUS_NULL)
	{
	    gfloat  pos = 0.0;
	    pos = (gfloat) gtk_xine_get_position (GTK_XINE (vp->xine));
	    gtk_adjustment_set_value (GTK_ADJUSTMENT (vp->seek_adj), pos);
	}
	else
	    gtk_adjustment_set_value (GTK_ADJUSTMENT (vp->seek_adj), 0);
    }

    return TRUE;
}

static void
cb_videoplay_play (GtkWidget * widget, VideoPlay * vp)
{
    if (vp->status == VIDEOPLAY_STATUS_NULL)
	return;

    if (gtk_xine_get_speed (GTK_XINE (vp->xine)) != SPEED_NORMAL)
	gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_NORMAL);

#ifdef ENABLE_XINE
    else if (!gtk_xine_play (GTK_XINE (vp->xine), 0, 0))
#else
    else if (!gtk_xine_play (GTK_XINE (vp->xine), vp->video_name, 0, 0))
#endif
    {
	vp->status = VIDEOPLAY_STATUS_NULL;

	if (vp->seek_timer_id)
	    gtk_timeout_remove (vp->seek_timer_id);

	if (vp->video_name)
	    g_free (vp->video_name);
	vp->video_name = NULL;

	gtk_adjustment_set_value (GTK_ADJUSTMENT (vp->seek_adj), 0);

	update_seek_time (vp);
	comment_view_clear (commentview);

	return;
    }

    vp->status = VIDEOPLAY_STATUS_PLAYING;
}

static void
cb_videoplay_pause (GtkWidget * widget, VideoPlay * vp)
{
    if (vp->status != VIDEOPLAY_STATUS_NULL
	&& vp->status != VIDEOPLAY_STATUS_STOP)
    {
	gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_PAUSE);
	vp->status = VIDEOPLAY_STATUS_PAUSE;
    }
}

static void
cb_videoplay_stop (GtkWidget * widget, VideoPlay * vp)
{
    if (vp->status != VIDEOPLAY_STATUS_NULL
	&& vp->status != VIDEOPLAY_STATUS_STOP)
    {
	gtk_xine_stop (GTK_XINE (vp->xine));
	vp->status = VIDEOPLAY_STATUS_STOP;
    }
}

static void
cb_videoplay_ff (GtkWidget * widget, VideoPlay * vp)
{
    if (vp->status != VIDEOPLAY_STATUS_NULL
	&& vp->status != VIDEOPLAY_STATUS_STOP)
    {
	switch (gtk_xine_get_speed (GTK_XINE (vp->xine)))
	{
	  case SPEED_NORMAL:
	  case SPEED_PAUSE:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_FAST_2);
	      vp->status = VIDEOPLAY_STATUS_FORWARD;
	      break;
	  case SPEED_FAST_2:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_FAST_4);
	      vp->status = VIDEOPLAY_STATUS_FORWARD;
	      break;
	  case SPEED_FAST_4:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_NORMAL);
	      vp->status = VIDEOPLAY_STATUS_PLAYING;
	      break;
	  case SPEED_SLOW_4:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_SLOW_2);
	      vp->status = VIDEOPLAY_STATUS_SLOW;
	      break;
	  case SPEED_SLOW_2:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_NORMAL);
	      vp->status = VIDEOPLAY_STATUS_PLAYING;
	      break;
	}
    }
}

static void
cb_videoplay_fs (GtkWidget * widget, VideoPlay * vp)
{
    if (vp->status != VIDEOPLAY_STATUS_NULL
	&& vp->status != VIDEOPLAY_STATUS_STOP)
    {
	switch (gtk_xine_get_speed (GTK_XINE (vp->xine)))
	{
	  case SPEED_NORMAL:
	  case SPEED_PAUSE:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_SLOW_2);
	      vp->status = VIDEOPLAY_STATUS_SLOW;
	      break;
	  case SPEED_SLOW_2:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_SLOW_4);
	      vp->status = VIDEOPLAY_STATUS_SLOW;
	      break;
	  case SPEED_SLOW_4:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_PAUSE);
	      vp->status = VIDEOPLAY_STATUS_PAUSE;
	      break;
	  case SPEED_FAST_4:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_FAST_2);
	      vp->status = VIDEOPLAY_STATUS_FORWARD;
	      break;
	  case SPEED_FAST_2:
	      gtk_xine_set_speed (GTK_XINE (vp->xine), SPEED_NORMAL);
	      vp->status = VIDEOPLAY_STATUS_PLAYING;
	      break;
	}
    }
}

static void
cb_videoplay_prev (GtkWidget * widget, VideoPlay * vp)
{
    thumbview_select_prev ();
}

static void
cb_videoplay_next (GtkWidget * widget, VideoPlay * vp)
{
    thumbview_select_next ();
}

static  gint
cb_videoplay_key_press (GtkWidget * widget, GdkEventKey * event,
			VideoPlay * vp)
{
    if (vp->is_hide)
	return FALSE;

    switch (event->keyval)
    {
      case GDK_Return:
	  videoplay_set_fullscreen ();
	  return TRUE;
      case GDK_t:
      case GDK_T:
	  videoplay_create_thumb ();
	  return TRUE;
      case GDK_Q:
      case GDK_q:
      case GDK_Escape:
	  if (vp->is_fullscreen)
	  {
	      vp->is_fullscreen = FALSE;
	      gtk_xine_set_fullscreen (GTK_XINE (vp->xine), FALSE);
	      gdk_keyboard_ungrab (GDK_CURRENT_TIME);
	      gtk_grab_remove (vp->event_box);
	      gtk_widget_grab_focus (GTK_WIDGET (vp->event_box));
	  }
	  return TRUE;
    }

    return FALSE;
}

static  gint
cb_videoplay_button_press (GtkWidget * widget, GdkEventButton * event,
			   VideoPlay * vp)
{
    if (vp->is_hide)
	return FALSE;

    gtk_widget_grab_focus (GTK_WIDGET (vp->event_box));

    return FALSE;
}

static  gboolean
cb_seek_pressed (GtkWidget * widget, GdkEventButton * event, VideoPlay * vp)
{
    if (vp->is_hide)
	return FALSE;

    g_return_val_if_fail (vp, FALSE);

    vp->seek_lock = TRUE;

    return FALSE;
}

static  gboolean
cb_seek_released (GtkWidget * widget, GdkEventButton * event, VideoPlay * vp)
{
    if (vp->is_hide)
	return FALSE;

    g_return_val_if_fail (vp, FALSE);

    if (vp->status != VIDEOPLAY_STATUS_NULL
	&& vp->status != VIDEOPLAY_STATUS_STOP)
    {
	gint    speed;

	speed = gtk_xine_get_speed (GTK_XINE (vp->xine));

#ifdef ENABLE_XINE
	gtk_xine_play (GTK_XINE (vp->xine),
		       (gint) GTK_ADJUSTMENT (vp->seek_adj)->value, 0);
#else
	gtk_xine_play (GTK_XINE (vp->xine), vp->video_name,
		       (gint) GTK_ADJUSTMENT (vp->seek_adj)->value, 0);
#endif

	gtk_xine_set_speed (GTK_XINE (vp->xine), speed);
    }

    vp->seek_lock = FALSE;

    return FALSE;
}

static void
cb_videoplay_playback_finished (GtkXine * gtx, VideoPlay * vp)
{
    vp->status = VIDEOPLAY_STATUS_STOP;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
videoplay_create (void)
{
    videoplay = g_new0 (VideoPlay, 1);
    videoplay->is_fullscreen = FALSE;
    videoplay->is_hide = TRUE;
    videoplay->seek_lock = FALSE;
    videoplay->status = VIDEOPLAY_STATUS_NULL;
    videoplay->video_name = NULL;

    videoplay->container = gtk_frame_new (NULL);
    gtk_widget_show (videoplay->container);

    videoplay->vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (videoplay->vbox);
    gtk_container_add (GTK_CONTAINER (videoplay->container), videoplay->vbox);

    videoplay->event_box = gtk_event_box_new ();
    gtk_box_pack_start (GTK_BOX (videoplay->vbox), videoplay->event_box,
			TRUE, TRUE, 0);

    gtk_widget_show (videoplay->event_box);

    videoplay->xine = gtk_xine_new ();
    gtk_container_add (GTK_CONTAINER (videoplay->event_box), videoplay->xine);
    gtk_widget_show (videoplay->xine);

    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (videoplay->event_box), GTK_CAN_FOCUS);

    gtk_widget_add_events (GTK_WIDGET (videoplay->event_box),
			   GDK_FOCUS_CHANGE
			   | GDK_BUTTON_PRESS_MASK | GDK_2BUTTON_PRESS
			   | GDK_KEY_PRESS | GDK_KEY_RELEASE
			   | GDK_BUTTON_RELEASE_MASK
			   | GDK_POINTER_MOTION_MASK
			   | GDK_POINTER_MOTION_HINT_MASK);


    gtk_signal_connect (GTK_OBJECT (videoplay->event_box),
			"key_press_event",
			GTK_SIGNAL_FUNC (cb_videoplay_key_press), videoplay);

    gtk_signal_connect_after (GTK_OBJECT (videoplay->event_box),
			      "button_press_event",
			      GTK_SIGNAL_FUNC (cb_videoplay_button_press),
			      videoplay);

    videoplay->hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (videoplay->hbox);
    gtk_box_pack_start (GTK_BOX (videoplay->vbox), videoplay->hbox, FALSE,
			FALSE, 10);

    videoplay->button_tooltips = gtk_tooltips_new ();

    videoplay->button_vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (videoplay->hbox), videoplay->button_vbox,
			FALSE, FALSE, 10);
    gtk_widget_show (videoplay->button_vbox);

    videoplay->button_hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (videoplay->button_vbox),
			videoplay->button_hbox, FALSE, FALSE, 10);
    gtk_widget_show (videoplay->button_hbox);

    videoplay->play_btn =
	button_create (videoplay->button_hbox, (gchar **) play_xpm, FALSE,
		       videoplay->button_tooltips, _("Play"),
		       GTK_SIGNAL_FUNC (cb_videoplay_play), videoplay);
    gtk_widget_set_usize (videoplay->play_btn, 30, 30);
    videoplay->pause_btn =
	button_create (videoplay->button_hbox, (gchar **) pause_xpm, FALSE,
		       videoplay->button_tooltips, _("Pause"),
		       GTK_SIGNAL_FUNC (cb_videoplay_pause), videoplay);
    gtk_widget_set_usize (videoplay->pause_btn, 30, 30);
    videoplay->stop_btn =
	button_create (videoplay->button_hbox, (gchar **) stop_xpm, FALSE,
		       videoplay->button_tooltips, _("Stop"),
		       GTK_SIGNAL_FUNC (cb_videoplay_stop), videoplay);
    gtk_widget_set_usize (videoplay->stop_btn, 30, 30);
    videoplay->fs_btn =
	button_create (videoplay->button_hbox, (gchar **) fs_xpm, FALSE,
		       videoplay->button_tooltips, _("Slow Forward"),
		       GTK_SIGNAL_FUNC (cb_videoplay_fs), videoplay);
    gtk_widget_set_usize (videoplay->fs_btn, 30, 30);
    videoplay->ff_btn =
	button_create (videoplay->button_hbox, (gchar **) ff_xpm, FALSE,
		       videoplay->button_tooltips, _("Fast Forward"),
		       GTK_SIGNAL_FUNC (cb_videoplay_ff), videoplay);
    gtk_widget_set_usize (videoplay->ff_btn, 30, 30);
    videoplay->previous_btn =
	button_create (videoplay->button_hbox, (gchar **) prev_xpm, FALSE,
		       videoplay->button_tooltips, _("Previous"),
		       GTK_SIGNAL_FUNC (cb_videoplay_prev), videoplay);
    gtk_widget_set_usize (videoplay->previous_btn, 30, 30);
    videoplay->next_btn =
	button_create (videoplay->button_hbox, (gchar **) next_xpm, FALSE,
		       videoplay->button_tooltips, _("Next"),
		       GTK_SIGNAL_FUNC (cb_videoplay_next), videoplay);
    gtk_widget_set_usize (videoplay->next_btn, 30, 30);

    videoplay->seek_vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (videoplay->hbox), videoplay->seek_vbox, TRUE,
			TRUE, 0);
    gtk_widget_show (videoplay->seek_vbox);

    videoplay->seek_adj =
	gtk_adjustment_new (0.0, 0.0, 65536.0, 1.0, 10.0, 1.0);

    GTK_ADJUSTMENT (videoplay->seek_adj)->value = 0;

    videoplay->seek = gtk_hscale_new (GTK_ADJUSTMENT (videoplay->seek_adj));
    gtk_widget_show (videoplay->seek);
    gtk_box_pack_start (GTK_BOX (videoplay->seek_vbox), videoplay->seek, TRUE,
			TRUE, 10);
    gtk_scale_set_draw_value (GTK_SCALE (videoplay->seek), FALSE);
    gtk_range_set_update_policy (GTK_RANGE (videoplay->seek),
				 GTK_UPDATE_CONTINUOUS);

    videoplay->seek_label = gtk_label_new ("00:00:00 of 00:00:00");
    gtk_box_pack_start (GTK_BOX (videoplay->seek_vbox), videoplay->seek_label,
			TRUE, TRUE, 0);
    gtk_widget_show (videoplay->seek_label);

    gtk_signal_connect (GTK_OBJECT (videoplay->seek),
			"button_press_event",
			GTK_SIGNAL_FUNC (cb_seek_pressed), videoplay);
    gtk_signal_connect (GTK_OBJECT (videoplay->seek),
			"button_release_event",
			GTK_SIGNAL_FUNC (cb_seek_released), videoplay);

    gtk_signal_connect (GTK_OBJECT (videoplay->xine), "playback_finished",
			GTK_SIGNAL_FUNC (cb_videoplay_playback_finished),
			videoplay);

    videoplay->seek_timer_id =
	gtk_timeout_add (500, cb_update_seek, videoplay);
}

void
videoplay_destroy (void)
{
    if (videoplay)
    {
	if (videoplay->status != VIDEOPLAY_STATUS_STOP
	    && videoplay->status != VIDEOPLAY_STATUS_NULL)
	    gtk_xine_stop (GTK_XINE (videoplay->xine));

	if (videoplay->seek_timer_id)
	    gtk_timeout_remove (videoplay->seek_timer_id);

	gtk_widget_destroy (videoplay->xine);

	if (videoplay->video_name)
	    g_free (videoplay->video_name);

	g_free (videoplay);

	videoplay = NULL;
    }
}

static int comment_delay_id = 0;

static  gboolean
cb_comment (gpointer data)
{
#ifdef ENABLE_XINE_OLD
    gchar  *frame;
#endif
    gint    width;
    gint    height;

    VideoPlay *vp = data;

#ifdef ENABLE_XINE_OLD
    frame =
	gtk_xine_get_current_frame_rgb (GTK_XINE (vp->xine), &width, &height);
#else
    width =
	gtk_xine_get_stream_info (GTK_XINE (vp->xine),
				  XINE_STREAM_INFO_VIDEO_WIDTH);
    height =
	gtk_xine_get_stream_info (GTK_XINE (vp->xine),
				  XINE_STREAM_INFO_VIDEO_HEIGHT);
#endif

    if (!vp->is_hide)
    {
	ImageInfo *ii;

	ii = image_info_get (vp->video_name, width, height);

	comment_view_change_file (commentview, ii);
    }

#ifdef ENABLE_XINE_OLD
    g_free (frame);
#endif

    return FALSE;
}

void
videoplay_play_file (gchar * video_name)
{
    if (videoplay->video_name && video_name)
	if (strcmp (videoplay->video_name, video_name) == 0)
	    return;

    videoplay->status = VIDEOPLAY_STATUS_NULL;

    if (videoplay->video_name)
	g_free (videoplay->video_name);

    videoplay->video_name = g_strdup (video_name);

#ifdef ENABLE_XINE
    if (!gtk_xine_open (GTK_XINE (videoplay->xine), videoplay->video_name))
    {
	videoplay->status = VIDEOPLAY_STATUS_NULL;

	if (videoplay->seek_timer_id)
	    gtk_timeout_remove (videoplay->seek_timer_id);

	if (videoplay->video_name)
	    g_free (videoplay->video_name);
	videoplay->video_name = NULL;

	gtk_adjustment_set_value (GTK_ADJUSTMENT (videoplay->seek_adj), 0);

	update_seek_time (videoplay);
	comment_view_clear (commentview);

	return;
    }

    if (!gtk_xine_play (GTK_XINE (videoplay->xine), 0, 0))
#else
    if (!gtk_xine_play
	(GTK_XINE (videoplay->xine), videoplay->video_name, 0, 0))
#endif
    {
	videoplay->status = VIDEOPLAY_STATUS_NULL;

	if (videoplay->seek_timer_id)
	    gtk_timeout_remove (videoplay->seek_timer_id);

	if (videoplay->video_name)
	    g_free (videoplay->video_name);
	videoplay->video_name = NULL;

	gtk_adjustment_set_value (GTK_ADJUSTMENT (videoplay->seek_adj), 0);

	update_seek_time (videoplay);
	comment_view_clear (commentview);

	return;
    }

    gtk_adjustment_set_value (GTK_ADJUSTMENT (videoplay->seek_adj), 0);

    gtk_label_set_text (GTK_LABEL (videoplay->seek_label),
			"00:00:00 of 00:00:00");

    videoplay->status = VIDEOPLAY_STATUS_PLAYING;

    if (videoplay->seek_timer_id)
	gtk_timeout_remove (videoplay->seek_timer_id);

    videoplay->seek_timer_id =
	gtk_timeout_add (500, cb_update_seek, videoplay);

    comment_delay_id = gtk_timeout_add (500, cb_comment, videoplay);
}

void
videoplay_play (void)
{
    cb_videoplay_play (NULL, videoplay);
}

void
videoplay_pause (void)
{
    cb_videoplay_pause (NULL, videoplay);
}

void
videoplay_stop (void)
{
    cb_videoplay_stop (NULL, videoplay);
}

void
videoplay_ff (void)
{
    cb_videoplay_ff (NULL, videoplay);
}

void
videoplay_fs (void)
{
    cb_videoplay_fs (NULL, videoplay);
}

void
videoplay_clear (void)
{
    if (videoplay)
    {
	if (videoplay->is_hide)
	    return;

	if (videoplay->seek_timer_id)
	    gtk_timeout_remove (videoplay->seek_timer_id);

	if (videoplay->video_name)
	    g_free (videoplay->video_name);

	videoplay->video_name = NULL;

	if (videoplay->status != VIDEOPLAY_STATUS_NULL
	    && videoplay->status != VIDEOPLAY_STATUS_STOP)
	{
	    gtk_xine_stop (GTK_XINE (videoplay->xine));
	    videoplay->status = VIDEOPLAY_STATUS_NULL;
	}
    }

    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1);
    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_NAME), 1, "");
    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1);
    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_IMAGE), 1, "");
}

void
videoplay_set_fullscreen (void)
{
    if (videoplay->status == VIDEOPLAY_STATUS_PLAYING
	|| videoplay->status == VIDEOPLAY_STATUS_FORWARD
	|| videoplay->status == VIDEOPLAY_STATUS_SLOW)
    {
	gdk_keyboard_grab (videoplay->event_box->window, TRUE,
			   GDK_CURRENT_TIME);
	gtk_grab_add (videoplay->event_box);
	gtk_widget_grab_focus (GTK_WIDGET (videoplay->event_box));
	gtk_xine_set_fullscreen (GTK_XINE (videoplay->xine), TRUE);
	videoplay->is_fullscreen = TRUE;
    }
}

void
videoplay_create_thumb (void)
{
    if (videoplay->status != VIDEOPLAY_STATUS_STOP
	&& videoplay->status != VIDEOPLAY_STATUS_NULL)
    {
	guchar *pixels;
	gint    width;
	gint    height;
	pixels =
	    gtk_xine_get_current_frame_rgb (GTK_XINE (videoplay->xine),
					    &width, &height);
	if (pixels)
	{
	    GdkPixbuf *pixbuf;
	    pixbuf =
		gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, FALSE,
					  8, width, height, 3 * width,
					  free_rgb_buffer, NULL);
	    if (pixbuf)
	    {
		gint    scale_width;
		gint    scale_height;
		GdkPixbuf *scale_pixbuf;
		GdkPixmap *pixmap;
		GdkBitmap *mask;
		gchar  *path;
		gchar  *cache_dir;
		mode_t  mode = 0755;
		gint    pixbuf_width;
		gint    pixbuf_height;
		pixbuf_width = gdk_pixbuf_get_width (pixbuf);
		pixbuf_height = gdk_pixbuf_get_height (pixbuf);
		if (pixbuf_width > 100 || pixbuf_height > 100)
		{
		    if (((float) 100 / pixbuf_width) <
			((float) 100 / pixbuf_height))
		    {
			scale_width = 100;
			scale_height =
			    (float) scale_width / pixbuf_width *
			    pixbuf_height;
			if (scale_height < 1)
			    scale_height = 1;
		    }
		    else
		    {
			scale_height = 100;
			scale_width =
			    (float) scale_height / pixbuf_height *
			    pixbuf_width;
			if (scale_width < 1)
			    scale_width = 1;
		    }
		}
		else
		{
		    scale_width = pixbuf_width;
		    scale_height = pixbuf_height;
		}

		scale_pixbuf =
		    gdk_pixbuf_scale_simple (pixbuf, scale_width,
					     scale_height,
					     conf.thumbnail_quality);
		gdk_pixbuf_unref (pixbuf);

		path =
		    g_strconcat (videoplay->video_name,
				 PORNVIEW_CACHE_THUMB_EXT, NULL);

		cache_dir =
		    cache_get_location (CACHE_THUMBS, path, FALSE, NULL,
					&mode);

		if (cache_ensure_dir_exists (cache_dir, mode))
		{
		    gchar  *cache_path;
		    cache_path =
			g_strconcat (cache_dir, "/",
				     g_basename (videoplay->video_name),
				     PORNVIEW_CACHE_THUMB_EXT, NULL);
		    if (pixbuf_to_file_as_png (scale_pixbuf, cache_path))
		    {
			struct utimbuf ut;
			ut.actime = ut.modtime = filetime (path);
			if (ut.modtime > 0)
			    utime (cache_path, &ut);
		    }

		    g_free (cache_path);
		}

		gdk_pixbuf_render_pixmap_and_mask (scale_pixbuf, &pixmap,
						   &mask, 128);
		zalbum_set_pixmap (ZALBUM (THUMBVIEW_ALBUM),
				   ZLIST (THUMBVIEW_ALBUM)->focus, pixmap,
				   mask);
		if (pixmap)
		    gdk_pixmap_unref (pixmap);

		if (mask)
		    gdk_bitmap_unref (mask);

		gdk_pixbuf_unref (scale_pixbuf);

		g_free (path);
		g_free (cache_dir);
	    }
	    else
		g_free (pixels);
	}
    }
}

#endif /* ENABLE_XINE && ENABLE_XINE_OLD */
