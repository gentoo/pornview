/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <gtk/gtk.h>

GtkWidget      *button_create (GtkWidget * box, gchar ** pixmap_data,
			       gint toggle, GtkTooltips * tooltips,
			       const gchar * tip_text, GtkSignalFunc func,
			       gpointer data);

#endif /* __BUTTON_H__ */
