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

#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__

#include "generic_dialog.h"
#include "file_dialog.h"

void            file_maint_renamed (const gchar * source, const gchar * dest);
void            file_maint_removed (const gchar * path, GList * ignore_list);
void            file_maint_moved (const gchar * source, const gchar * dest,
				  GList * ignore_list);

GenericDialog  *file_util_gen_dlg (const gchar * title, const gchar * message,
				   const gchar * wmclass,
				   const gchar * wmsubclass, gint auto_close,
				   void (*cancel_cb) (GenericDialog *,
						      gpointer),
				   gpointer data);
FileDialog     *file_util_file_dlg (const gchar * title,
				    const gchar * message,
				    const gchar * wmclass,
				    const gchar * wmsubclass,
				    void (*cancel_cb) (FileDialog *,
						       gpointer),
				    gpointer data);
GenericDialog  *file_util_warning_dialog (const gchar * title,
					  const gchar * message);

void            file_util_delete (const gchar * source_path,
				  GList * source_list);
void            file_util_move (const gchar * source_path,
				GList * source_list, const gchar * dest_path);
void            file_util_copy (const gchar * source_path,
				GList * source_list, const gchar * dest_path);
void            file_util_rename (const gchar * source_path,
				  GList * source_list);
void            file_util_create_dir (const gchar * path);

/* these avoid the location entry dialog, list must be files only and
 * dest_path must be a valid directory path
*/
void            file_util_move_simple (GList * list, const gchar * dest_path);
void            file_util_copy_simple (GList * list, const gchar * dest_path);

#endif /* __FILEUTIL_H__ */
