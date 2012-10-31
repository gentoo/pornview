/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __DIALOG_H__
#define __DIALOG_H__

typedef enum
{
    CONFIRM_YES,
    CONFIRM_NO
}
ConfirmType;

typedef enum
{
    TEXT_ENTRY_WRAP_ENTRY = 1 << 1,
    TEXT_ENTRY_CURSOR_TOP = 1 << 2,
    TEXT_ENTRY_NO_EDITABLE = 1 << 3
}
TextEntryFlags;

ConfirmType     dialog_confirm (const gchar * title, const gchar * message,
				GtkWidget * parent);
void            dialog_message (const gchar * title, const gchar * message,
				GtkWidget * parent);

GtkWidget      *dialog_choose_dir (const gchar * title);
gchar          *dialog_choose_dir_modal (const gchar * title,
					 const gchar * default_path,
					 GtkWindow * window);

gchar          *dialog_textentry (const gchar * title,
				  const gchar * label_text,
				  const gchar * entry_text, GList * text_list,
				  gint entry_width, TextEntryFlags flags);

GtkWidget      *dialog_progress_create (gchar * title,
					gchar * initial_message,
					gboolean * cancel_pressed,
					gint width, gint height);
void            dialog_progress_update (GtkWidget * window,
					gchar * title,
					gchar * message,
					gchar * progress_text,
					gfloat progress);

#endif /* __DIALOG_H__ */
