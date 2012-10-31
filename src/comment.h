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

#ifndef __COMMENT_H__
#define __COMMENT_H__

#include "image_info.h"

#define TYPE_COMMENT            (comment_get_type ())
#define COMMENT(obj)            (GTK_CHECK_CAST ((obj), TYPE_COMMENT, Comment))
#define COMMENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_COMMENT, CommentClass))
#define IS_COMMENT(obj)         (GTK_CHECK_TYPE ((obj), TYPE_COMMENT))
#define IS_COMMENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_COMMENT))

typedef struct Comment_Tag Comment;
typedef struct CommentClass_Tag CommentClass;

struct Comment_Tag
{
    GtkObject       parent;

    gchar          *filename;
    ImageInfo      *info;	/* 1:1 relation */

    GList          *data_list;	/* comment data list */
    gchar          *note;
};

struct CommentClass_Tag
{
    GtkObjectClass  parent_class;

    /*
     * -- Signals -- 
     */
    void            (*file_saved) (Comment * comment, ImageInfo * info);
    void            (*file_deleted) (Comment * comment, ImageInfo * info);
};

typedef gchar  *(*CommentDataGetDefValFn) (ImageInfo * info, gpointer data);

typedef struct CommentDataEntry_Tag
{
    gchar          *key;
    gchar          *display_name;
    gchar          *value;
    gboolean        enable;
    gboolean        auto_val;
    gboolean        display;
    gboolean        userdef;
    CommentDataGetDefValFn def_val_fn;
}
CommentDataEntry;


guint           comment_get_type (void);
Comment        *comment_ref (Comment * comment);
void            comment_unref (Comment * comment);

void            comment_update_data_entry_list (void);
GList          *comment_get_data_entry_list (void);

gchar          *comment_get_path (const gchar * img_path);
gchar          *comment_find_file (const gchar * img_path);
CommentDataEntry *comment_data_entry_find_template_by_key (const gchar * key);
CommentDataEntry *comment_find_data_entry_by_key (Comment * comment,
						  const gchar * key);
CommentDataEntry *comment_append_data (Comment * comment,
				       const gchar * key,
				       const gchar * value);
void            comment_data_entry_remove (Comment * comment,
					   CommentDataEntry * entry);
void            comment_data_entry_remove_by_key (Comment * comment,
						  const gchar * key);
CommentDataEntry *comment_data_entry_dup (CommentDataEntry * src);
void            comment_data_entry_delete (CommentDataEntry * entry);
gboolean        comment_update_note (Comment * comment, gchar * note);
gboolean        comment_save_file (Comment * comment);
void            comment_delete_file (Comment * comment);
Comment        *comment_get_from_image_info (ImageInfo * info);

#endif /* __COMMENT_H__ */
