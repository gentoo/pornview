/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __COMMENT_VIEW_H__
#define __COMMENT_VIEW_H__

#include "comment.h"
#include "imageinfo.h"

typedef struct CommentView_Tag
{
    Comment        *comment;

    GtkWidget      *window;
    GtkWidget      *main_vbox;
    GtkWidget      *notebook;
    GtkWidget      *button_area;

    GtkWidget      *data_page, *note_page;

    GtkWidget      *comment_clist;
    gint            selected_row;

    GtkWidget      *key_combo, *value_entry;
    GtkWidget      *selected_item;

    GtkWidget      *note_box;

    GtkWidget      *clear_button;
    GtkWidget      *apply_button;
    GtkWidget      *del_value_button;

    GtkWidget      *save_button;
    GtkWidget      *reset_button;
    GtkWidget      *delete_button;
}
CommentView;

extern CommentView *commentview;

void            comment_view_clear_data (CommentView * cv);
void            comment_view_clear (CommentView * cv);
gboolean        comment_view_change_file (CommentView * cv, ImageInfo * info);
CommentView    *comment_view_create ();
CommentView    *comment_view_create_window (ImageInfo * info);

#endif /* __COMMENT_VIEW_H__ */
