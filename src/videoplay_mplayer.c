/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#ifdef ENABLE_MPLAYER

#include "gtkmplayer.h"

#include "button.h"
#include "pixbuf_utils.h"

#include "videoplay.h"
#include "thumbview.h"

#include "pixmaps/play.xpm"
#include "pixmaps/pause.xpm"
#include "pixmaps/stop.xpm"
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
	  gtk_widget_set_sensitive (vp->seek, FALSE);
	  break;

      case VIDEOPLAY_STATUS_PLAYING:
	  gtk_widget_set_sensitive (vp->play_btn, FALSE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, TRUE);
	  gtk_widget_set_sensitive (vp->seek, TRUE);
	  break;

      case VIDEOPLAY_STATUS_SLOW:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, TRUE);
	  gtk_widget_set_sensitive (vp->seek, TRUE);
	  break;

      case VIDEOPLAY_STATUS_PAUSE:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, FALSE);
	  gtk_widget_set_sensitive (vp->seek, FALSE);
	  break;

      case VIDEOPLAY_STATUS_STOP:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, FALSE);
	  gtk_widget_set_sensitive (vp->pause_btn, FALSE);
	  gtk_widget_set_sensitive (vp->seek, FALSE);
	  break;

      case VIDEOPLAY_STATUS_FORWARD:
	  gtk_widget_set_sensitive (vp->play_btn, TRUE);
	  gtk_widget_set_sensitive (vp->stop_btn, TRUE);
	  gtk_widget_set_sensitive (vp->pause_btn, TRUE);
	  gtk_widget_set_sensitive (vp->seek, TRUE);
	  break;
    }

    gtk_widget_set_sensitive (vp->previous_btn, thumbview_is_prev ());
    gtk_widget_set_sensitive (vp->next_btn, thumbview_is_next ());

    thumbview_vupdate ();
}

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_mplayer_play (GtkMPlayer * mp, gpointer data)
{
    VideoPlay *vp = data;

    vp->status = VIDEOPLAY_STATUS_PLAYING;
    update_seek_buttons (vp);
}

static void
cb_mplayer_stop (GtkMPlayer * mp, gpointer data)
{
    VideoPlay *vp = data;

    vp->status = VIDEOPLAY_STATUS_STOP;
    gtk_label_set_text (GTK_LABEL (vp->seek_label), "00:00:00");
    gtk_adjustment_set_value (GTK_ADJUSTMENT (vp->seek_adj), 0);
    update_seek_buttons (vp);
}

static void
cb_mplayer_pause (GtkMPlayer * mp, gpointer data)
{
    VideoPlay *vp = data;

    vp->status = VIDEOPLAY_STATUS_PAUSE;
    update_seek_buttons (vp);
}

static void
cb_mplayer_pos_changed (GtkMPlayer * mp, gpointer data)
{
    gchar   timestr[32];
    gint    h, m, s;
    gfloat  pos;
    VideoPlay *vp = data;

    pos = gtk_mplayer_get_position (mp);

    h = pos / 3600;
    m = ((gint) pos - h * 3600) / 60;
    s = (gint) pos - h * 3600 - m * 60;
    if ((gint) pos != vp->position)
    {
	g_snprintf (timestr, sizeof (timestr) / sizeof (gchar),
		    "%02d:%02d:%02d", h, m, s);
	gtk_label_set_text (GTK_LABEL (vp->seek_label), timestr);
    }
    vp->position = pos;

    update_seek_buttons (vp);
}

static void
cb_videoplay_play (GtkWidget * widget, gpointer data)
{
    VideoPlay *vp = data;

    gtk_mplayer_play (GTK_MPLAYER (vp->mplayer));
}

static void
cb_videoplay_stop (GtkWidget * widget, gpointer data)
{
    VideoPlay *vp = data;

    gtk_mplayer_stop (GTK_MPLAYER (vp->mplayer));
}

static void
cb_videoplay_pause (GtkWidget * widget, gpointer data)
{
    VideoPlay *vp = data;

    gtk_mplayer_toggle_pause (GTK_MPLAYER (vp->mplayer));
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

static void
cb_seekbar_value_changed (GtkAdjustment * adj, gpointer data)
{
    VideoPlay *vp = data;

    gtk_mplayer_seek (GTK_MPLAYER (vp->mplayer), adj->value);
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

    videoplay->frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.33333, FALSE);
    gtk_box_pack_start (GTK_BOX (videoplay->vbox), videoplay->frame, TRUE,
			TRUE, 0);
    gtk_widget_show (videoplay->frame);

    videoplay->mplayer = gtk_mplayer_new ();
    gtk_container_add (GTK_CONTAINER (videoplay->frame), videoplay->mplayer);
    gtk_widget_show (videoplay->mplayer);

    gtk_signal_connect (GTK_OBJECT (videoplay->mplayer), "play",
			GTK_SIGNAL_FUNC (cb_mplayer_play), videoplay);
    gtk_signal_connect (GTK_OBJECT (videoplay->mplayer), "stop",
			GTK_SIGNAL_FUNC (cb_mplayer_stop), videoplay);
    gtk_signal_connect (GTK_OBJECT (videoplay->mplayer), "pause",
			GTK_SIGNAL_FUNC (cb_mplayer_pause), videoplay);
    gtk_signal_connect (GTK_OBJECT (videoplay->mplayer), "position_changed",
			GTK_SIGNAL_FUNC (cb_mplayer_pos_changed), videoplay);

    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (videoplay->frame), GTK_CAN_FOCUS);

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
	gtk_adjustment_new (0.0, 0.0, 100.0, 0.01, 1.0, 1.0);

    GTK_ADJUSTMENT (videoplay->seek_adj)->value = 0;

    videoplay->seek = gtk_hscale_new (GTK_ADJUSTMENT (videoplay->seek_adj));
    gtk_widget_show (videoplay->seek);
    gtk_box_pack_start (GTK_BOX (videoplay->seek_vbox), videoplay->seek, TRUE,
			TRUE, 10);
    gtk_scale_set_draw_value (GTK_SCALE (videoplay->seek), FALSE);
    gtk_range_set_update_policy (GTK_RANGE (videoplay->seek),
				 GTK_UPDATE_CONTINUOUS);

    videoplay->seek_label = gtk_label_new ("00:00:00");
    gtk_box_pack_start (GTK_BOX (videoplay->seek_vbox), videoplay->seek_label,
			TRUE, TRUE, 0);
    gtk_widget_show (videoplay->seek_label);

    gtk_signal_connect (GTK_OBJECT (videoplay->seek_adj), "value_changed",
			GTK_SIGNAL_FUNC (cb_seekbar_value_changed),
			videoplay);
}

void
videoplay_destroy (void)
{
    if (videoplay)
    {
	if (gtk_mplayer_is_running (GTK_MPLAYER (videoplay->mplayer)))
	    gtk_mplayer_stop (GTK_MPLAYER (videoplay->mplayer));

	gtk_widget_destroy (videoplay->mplayer);

	g_free (videoplay);

	videoplay = NULL;
    }
}

void
videoplay_play_file (gchar * video_name)
{
    if (gtk_mplayer_is_running (GTK_MPLAYER (videoplay->mplayer)))
	gtk_mplayer_stop (GTK_MPLAYER (videoplay->mplayer));

    gtk_mplayer_set_file (GTK_MPLAYER (videoplay->mplayer), video_name);

    gtk_mplayer_play (GTK_MPLAYER (videoplay->mplayer));
}

void
videoplay_play (void)
{
    gtk_mplayer_play (GTK_MPLAYER (videoplay->mplayer));
}

void
videoplay_pause (void)
{
    if (gtk_mplayer_is_running (GTK_MPLAYER (videoplay->mplayer)))
	gtk_mplayer_toggle_pause (GTK_MPLAYER (videoplay->mplayer));
}

void
videoplay_stop (void)
{
    if (gtk_mplayer_is_running (GTK_MPLAYER (videoplay->mplayer)))
	gtk_mplayer_stop (GTK_MPLAYER (videoplay->mplayer));
}

void
videoplay_ff (void)
{
}

void
videoplay_fs (void)
{
}

void
videoplay_clear (void)
{
    if (gtk_mplayer_is_running (GTK_MPLAYER (videoplay->mplayer)))
	gtk_mplayer_stop (GTK_MPLAYER (videoplay->mplayer));
}

void
videoplay_set_fullscreen (void)
{
}

void
videoplay_create_thumb (void)
{
}

#endif /* ENABLE_MPLAYER */
