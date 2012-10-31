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

#ifndef __PREFS_UI_UTILS_H__
#define __PREFS_UI_UTILS_H__

#include <gtk/gtk.h>

GtkWidget      *prefs_ui_dir_list_prefs (const gchar * title,
					 const gchar * dialog_title,
					 const gchar * defval,
					 gchar ** prefsval, gchar sepchar);

#endif /* __PREFS_UI_UTILS_H__ */
