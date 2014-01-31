/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __MENU_H__
#define __MENU_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

gint            menu_count_ifactory_entry_num (GtkItemFactoryEntry * entries);
GtkWidget      *menubar_create (GtkWidget * window,
				GtkItemFactoryEntry * entries,
				guint n_entries,
				const gchar * path, gpointer data);

GtkWidget      *menu_create_items (GtkWidget * window,
				   GtkItemFactoryEntry * entries,
				   guint n_entries,
				   const gchar * path, gpointer data);

void            menu_item_set_sensitive (GtkWidget * widget,
					 gchar * path, gboolean sensitive);
void            menu_check_item_set_active (GtkWidget * widget,
					    gchar * path, gboolean active);
gboolean        menu_check_item_get_active (GtkWidget * widget, gchar * path);
void            menu_set_submenu (GtkWidget * widget,
				  const gchar * path, GtkWidget * submenu);
GtkWidget      *menu_get_submenu (GtkWidget * widget, const gchar * path);
void            menu_remove_submenu (GtkWidget * widget,
				     const gchar * path, GtkWidget * submenu);
GtkWidget      *menu_option_simple (const gchar ** menu_items,
				    gint def_val, gint * data);
GtkWidget      *menu_option (const gchar ** menu_items,
			     gint def_val, gpointer func, gpointer data);

void            menu_modal_cb (gpointer data,
			       guint action, GtkWidget * menuitem);
gint            menu_popup_modal (GtkWidget * popup,
				  GtkMenuPositionFunc pos_func,
				  gpointer pos_data,
				  GdkEventButton * event, gpointer user_data);
void            menu_calc_popup_position (GtkMenu * menu,
					  gint * x_ret, gint * y_ret,
					  gboolean * push_in,
					  gpointer data);

#endif /* __MENU_H__ */
