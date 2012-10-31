/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __THUMBVIEW_H__
#define __THUMBVIEW_H__

#include "file_list.h"
#include "thumb_loader.h"

typedef struct _ThumbView ThumbView;

struct _ThumbView
{
    GtkWidget      *container;
    GtkWidget      *toolbar;
    GtkWidget      *toolbar_eventbox;
    GtkWidget      *toolbar_refresh_btn;
    GtkWidget      *toolbar_previous_btn;
    GtkWidget      *toolbar_next_btn;
    GtkWidget      *toolbar_first_btn;
    GtkWidget      *toolbar_last_btn;
    GtkWidget      *toolbar_view_btn;
    GtkWidget      *toolbar_info_btn;

    GtkWidget      *toolbar_list_btn;
    GtkWidget      *toolbar_thumbs_btn;

    GtkWidget      *toolbar_slideshow_btn;
    GtkWidget      *toolbar_no_zoom_btn;
    GtkWidget      *toolbar_zoom_auto_btn;
    GtkWidget      *toolbar_zoom_fit_btn;
    GtkWidget      *toolbar_zoom_in_btn;
    GtkWidget      *toolbar_zoom_out_btn;

    GtkWidget      *scroll_win;
    GtkWidget      *frame;
    GtkWidget      *album;

    GList          *selection;
    gint            current;

    ThumbLoader    *thumbs_loader;
    gint            thumbs_count;
    gboolean        thumbs_running;
    gboolean        thumbs_stop;
};

extern ThumbView *thumbview;

#define THUMBVIEW            thumbview
#define THUMBVIEW_CONTAINER  thumbview->container
#define THUMBVIEW_ALBUM      thumbview->album

void            thumbview_create (GtkWidget * parent_win);
void            thumbview_destroy (void);
void            thumbview_add (FileList * fl);
void            thumbview_clear (void);
void            thumbview_stop (void);
void            thumbview_set_mode (gint mode);
gint            thumbview_get_mode (void);

void            thumbview_select_first (void);
void            thumbview_select_last (void);
void            thumbview_select_next (void);
void            thumbview_select_prev (void);
void            thumbview_refresh (void);

gboolean        thumbview_is_next (void);
gboolean        thumbview_is_prev (void);

void            thumbview_vupdate (void);
void            thumbview_iupdate (void);

#endif /* __THUMBVIEW_H__ */
