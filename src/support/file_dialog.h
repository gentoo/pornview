/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GQview.
 * GQview author: John Ellis
 */

#ifndef __FILE_DIALOG_H__
#define __FILE_DIALOG_H__

typedef struct _FileDialog FileDialog;
struct _FileDialog
{
    GenericDialog   gd;

    GtkWidget      *entry;

    gint            type;
    gint            multiple_files;

    gchar          *source_path;
    GList          *source_list;

    gchar          *dest_path;
};

FileDialog     *file_dialog_new (const gchar * title, const gchar * message,
				 const gchar * wmclass,
				 const gchar * wmsubclass,
				 void (*cb_cancel) (FileDialog *, gpointer),
				 gpointer data);

void            file_dialog_close (FileDialog * fd);

GtkWidget      *file_dialog_add_button (FileDialog * fd, const gchar * text,
					void (*cb_func) (FileDialog *,
							 gpointer),
					gint is_default);

void            file_dialog_add_path_widgets (FileDialog * fd,
					      const gchar * default_path,
					      const gchar * path,
					      const gchar * filter,
					      const gchar * filter_desc);

#endif /* __FILE_DIALOG_H__ */
