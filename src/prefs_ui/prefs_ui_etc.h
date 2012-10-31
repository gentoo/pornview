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

#ifndef __PREFS_UI_ETC_H__
#define __PREFS_UI_ETC_H__

#include "prefs_win.h"

GtkWidget      *prefs_comment_page (void);
void            prefs_comment_apply (Config * src, Config * dest,
				     PrefsWinButton state);

GtkWidget      *prefs_slideshow_page (void);

#endif /* __PREFS_UI_ETC_H__ */
