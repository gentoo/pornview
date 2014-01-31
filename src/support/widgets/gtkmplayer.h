/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are based on widget/gimv_player.h in GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __GTK_MPLAYER_H__
#define __GTK_MPLAYER_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern          "C"
{
#endif				/* __cplusplus */

#define GTK_MPLAYER(obj)            (GTK_CHECK_CAST ((obj), gtk_mplayer_get_type (), GtkMPlayer))
#define GTK_MPLAYER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), gtk_mplayer_get_type (), GtkMPlayerClass))
#define GTK_IS_MPLAYER(obj)         (GTK_CHECK_TYPE (obj, gtk_mplayer_get_type ()))
#define GTK_IS_MPLAYER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), gtk_mplayer_get_type ()))


    typedef struct GtkMPlayer_Tag GtkMPlayer;
    typedef struct GtkMPlayerClass_Tag GtkMPlayerClass;
    typedef struct GtkMPlayerMediaInfo_Tag GtkMPlayerMediaInfo;


    typedef enum
    {
	GtkMPlayerStatusStop,
	GtkMPLayerStatusDetecting,
	GtkMPlayerStatusPlay,
	GtkMPlayerStatusPause
    }
    GtkMPlayerStatus;


    typedef enum
    {
	GtkMPlayerEmbedFlag = 1 << 0,
	GtkMPlayerAllowFrameDroppingFlag = 1 << 1,
    }
    GtkMPlayerFlags;


    typedef enum
    {
	GtkMPlayerMediaInfoNoVideoFlag = 1 << 0,
	GtkMPlayerMediaInfoNoAudioFlag = 1 << 1,
    }
    GtkMPlayerMediaInfoFlags;


    struct GtkMPlayerMediaInfo_Tag
    {
	GtkMPlayerMediaInfoFlags flags;
	gfloat          length;

	/*
	 * video
	 */
	gint            width;
	gint            height;

	/*
	 * audio
	 */


	/*
	 * other
	 */
#if 0
	StreamType      stream_type;
	union StreamInfo
	{
	    GtkMPlayerDvdInfo *dvdinfo;
	    GtkMPlayerVCDInfo *vcdinfo;
	}
	stream_info;
#endif
    };


    struct GtkMPlayer_Tag
    {
	GtkWidget       parent;

	gchar          *filename;
	gfloat          pos;
	gfloat          speed;
	GtkMPlayerStatus status;
	GtkMPlayerFlags flags;

	/*
	 * command
	 */
	gchar          *command;
	gchar          *vo;	/* video out driver's name */
	gchar          *ao;	/* audio out driver's name */
	GList          *args;	/* other arguments */

	/*
	 * media info
	 */
	GtkMPlayerMediaInfo media_info;
    };

    struct GtkMPlayerClass_Tag
    {
	GtkWidgetClass  parent_class;

	/*
	 * signals
	 */

	void            (*play) (GtkMPlayer * player);
	void            (*stop) (GtkMPlayer * player);
	void            (*pause) (GtkMPlayer * player);
	void            (*position_changed) (GtkMPlayer * player);
    };


    GtkType         gtk_mplayer_get_type (void);
    GtkWidget      *gtk_mplayer_new (void);
    void            gtk_mplayer_set_file (GtkMPlayer * player,
					  const gchar * file);
    void            gtk_mplayer_set_flags (GtkMPlayer * player,
					   GtkMPlayerFlags flags);
    void            gtk_mplayer_unset_flags (GtkMPlayer * player,
					     GtkMPlayerFlags flags);
    GtkMPlayerFlags gtk_mplayer_get_flags (GtkMPlayer * player,
					   GtkMPlayerFlags flags);
    void            gtk_mplayer_set_video_out_driver (GtkMPlayer * player,
						      const gchar *
						      vo_driver);
    void            gtk_mplayer_set_audio_out_driver (GtkMPlayer * player,
						      const gchar *
						      ao_driver);

    const GList    *gtk_mplayer_get_video_out_drivers (GtkMPlayer * player,
						       gboolean refresh);
    const GList    *gtk_mplayer_get_audio_out_drivers (GtkMPlayer * player,
						       gboolean refresh);

    gfloat          gtk_mplayer_get_position (GtkMPlayer * player);
    GtkMPlayerStatus gtk_mplayer_get_status (GtkMPlayer * player);
    gboolean        gtk_mplayer_is_running (GtkMPlayer * player);

    void            gtk_mplayer_play (GtkMPlayer * player);
    void            gtk_mplayer_start (GtkMPlayer * player,
				       gfloat pos, gfloat speed);
    void            gtk_mplayer_stop (GtkMPlayer * player);
    void            gtk_mplayer_toggle_pause (GtkMPlayer * player);
    void            gtk_mplayer_set_speed (GtkMPlayer * player, gfloat speed);
    gfloat          gtk_mplayer_get_speed (GtkMPlayer * player);
    void            gtk_mplayer_seek (GtkMPlayer * player, gfloat percentage);
    void            gtk_mplayer_seek_by_time (GtkMPlayer * player,
					      gfloat second);
    void            gtk_mplayer_seek_relative (GtkMPlayer * player,
					       gfloat second);
    void            gtk_mplayer_set_audio_delay (GtkMPlayer * player,
						 gfloat second);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GTK_MPLAYER_H__ */
