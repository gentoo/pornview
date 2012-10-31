/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __CHECK_BUTTON_H__
#define __CHECK_BUTTON_H__

#include <gtk/gtk.h>

GtkWidget      *check_button_create (const gchar * label_text,
				     gboolean def_val, gpointer func,
				     gpointer data);
void            check_button_get_data_from_toggle_cb (GtkWidget * toggle,
						      gboolean * data);

#endif /* __CHECK_BUTTON_H__ */
