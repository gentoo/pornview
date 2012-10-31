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

#ifndef __PREFS_UI_COMMON_H__
#define __PREFS_UI_COMMON_H__

#include "prefs_win.h"

GtkWidget      *prefs_common_page (void);
GtkWidget      *prefs_filter_page (void);
void            prefs_filter_apply (Config * src, Config * dest,
				    PrefsWinButton state);
GtkWidget      *prefs_charset_page (void);

#endif /* __PREFS_UI_COMMON_H__ */
