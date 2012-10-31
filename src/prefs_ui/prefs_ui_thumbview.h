/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __PREFS_UI_THUMBVIEW_H__
#define __PREFS_UI_THUMBVIEW_H__

#include "prefs_win.h"

GtkWidget      *prefs_thumbview_page (void);
void            prefs_thumbview_apply (Config * src, Config * dest,
				       PrefsWinButton state);

#endif /* __PREFS_UI_THUMBVIEW_H__ */
