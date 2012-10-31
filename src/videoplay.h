/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __VIDEOPLAY_H__
#define __VIDEOPLAY_H__

typedef struct _VideoPlay VideoPlay;

struct _VideoPlay
{
    GtkWidget      *container;
    GtkWidget      *vbox;

#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
    GtkWidget      *event_box;
    GtkWidget      *xine;
#endif

#ifdef ENABLE_MPLAYER
    GtkWidget      *frame;
    GtkWidget      *mplayer;
#endif

    GtkWidget      *hbox;
    GtkTooltips    *button_tooltips;
    GtkWidget      *button_vbox;
    GtkWidget      *button_hbox;
    GtkWidget      *play_btn;
    GtkWidget      *pause_btn;
    GtkWidget      *stop_btn;
    GtkWidget      *ff_btn;
    GtkWidget      *fs_btn;
    GtkWidget      *previous_btn;
    GtkWidget      *next_btn;

    /*
     * seek 
     */
    GtkWidget      *seek_vbox;
    GtkWidget      *seek_label;
    GtkWidget      *seek;
    GtkObject      *seek_adj;
    gint            seek_timer_id;
    gboolean        seek_lock;

    gboolean        is_fullscreen;
    gboolean        is_hide;

/* video file name */
    gchar          *video_name;
/* current status */
    gint            status;

#ifdef ENABLE_MPLAYER
    gint            position;
#endif
};

typedef enum
{
    VIDEOPLAY_STATUS_NULL,
    VIDEOPLAY_STATUS_PLAYING,
    VIDEOPLAY_STATUS_STOP,
    VIDEOPLAY_STATUS_PAUSE,
    VIDEOPLAY_STATUS_FORWARD,
    VIDEOPLAY_STATUS_SLOW
}
VideoPlayStatus;

extern VideoPlay *videoplay;

#define VIDEOPLAY            videoplay
#define VIDEOPLAY_CONTAINER  videoplay->container
#define VIDEOPLAY_IS_HIDE    videoplay->is_hide
#define VIDEOPLAY_STATUS     videoplay->status

void            videoplay_create (void);
void            videoplay_destroy (void);
void            videoplay_play_file (gchar * name);
void            videoplay_play (void);
void            videoplay_pause (void);
void            videoplay_stop (void);
void            videoplay_ff (void);
void            videoplay_fs (void);
void            videoplay_clear (void);
void            videoplay_set_fullscreen (void);
void            videoplay_create_thumb (void);

#endif /* __VIDEOPLAY_H__ */
