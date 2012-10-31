/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2003                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __PREFS_UI_PLUGIN_H__
#define __PREFS_UI_PLUGIN_H__

#include "prefs_win.h"

GtkWidget      *prefs_plugin_page (void);
void            prefs_plugin_apply (Config * src, Config * dest,
				    PrefsWinButton state);

#endif /* __PREFS_UI_PLUGIN_H__ */
