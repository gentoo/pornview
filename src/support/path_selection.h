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

#ifndef __PATH_SELECTION_H__
#define __PATH_SELECTION_H__

GtkWidget      *path_selection_new_with_files (GtkWidget * entry,
					       const gchar * path,
					       const gchar * filter,
					       const gchar * filter_desc);
GtkWidget      *path_selection_new (const gchar * path, GtkWidget * entry);

void            path_selection_sync_to_entry (GtkWidget * entry);

void            path_selection_add_select_func (GtkWidget * entry,
						void (*func) (const gchar *,
							      gpointer),
						gpointer data);
void            path_selection_add_filter (GtkWidget * entry,
					   const gchar * filter,
					   const gchar * description,
					   gint set);
void            path_selection_clear_filter (GtkWidget * entry);

#endif /* __PATH_SELECTION_H__ */
