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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#include "menu.h"

#include "intl.h"

static void cb_get_data_from_menuitem (GtkWidget * widget, gint * conf);
static void menu_shell_deactivated (GtkMenuShell * menu_shell, gpointer data);

/*
 *  menu_count_ifactory_entry_num:
 *     @ Count NULL terminated GtkItemFactoryEntry array length.
 *
 *  Return : length of array.
 */
gint
menu_count_ifactory_entry_num (GtkItemFactoryEntry * entries)
{
    gint    i;

    if (!entries)
	return -1;

    for (i = 0; entries[i].path; i++)
    {
	continue;
    }
    return i;
}

static void
cb_menu_destroy (GtkWidget * widget, GtkItemFactory * factory)
{
    g_return_if_fail (factory);
    g_return_if_fail (GTK_IS_ITEM_FACTORY (factory));

    gtk_object_unref (GTK_OBJECT (factory));
}

/*
 *  menu_create:
 *     @ Create menu bar widget.
 *
 *  window    : Window widget that attach accel group.
 *  entries   : Menu item entries.
 *  n_entries : Number of menu items.
 *  path      : Root menu path.
 *  data      : User data for menu callback.
 *  Return    : Menubar widget.
 */
GtkWidget *
menubar_create (GtkWidget * window,
		GtkItemFactoryEntry * entries,
		guint n_entries, const gchar * path, gpointer data)
{
    GtkItemFactory *factory;
    GtkAccelGroup *accel_group;
    GtkWidget *widget;

    accel_group = gtk_accel_group_new ();
    factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, path, accel_group);
#ifdef ENABLE_NLS
    gtk_item_factory_set_translate_func (factory, (GtkTranslateFunc) gettext,
					 NULL, NULL);
#endif /* ENABLE_NLS */
    gtk_item_factory_create_items (factory, n_entries, entries, data);
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    widget = gtk_item_factory_get_widget (factory, path);
    gtk_signal_connect (GTK_OBJECT (widget), "destroy",
			GTK_SIGNAL_FUNC (cb_menu_destroy), factory);

    return widget;
}

/*
 *  menu_create_items:
 *     @ Create menu item for menu widget (like a popup menu).
 *
 *  window    : Window widget that attach accel group.
 *  entries   : Menu item entries.
 *  n_entries : Number of menu items.
 *  path      : Root menu path.
 *  data      : User data for menu callback.
 *  Return    : menu widget.
 */
GtkWidget *
menu_create_items (GtkWidget * window,
		   GtkItemFactoryEntry * entries,
		   guint n_entries, const gchar * path, gpointer data)
{
    GtkItemFactory *factory;
    GtkAccelGroup *accel_group = NULL;
    GtkWidget *widget;

    if (window)
	accel_group = gtk_accel_group_new ();

    factory = gtk_item_factory_new (GTK_TYPE_MENU, path, accel_group);
#ifdef ENABLE_NLS
    gtk_item_factory_set_translate_func (factory, (GtkTranslateFunc) gettext,
					 NULL, NULL);
#endif /* ENABLE_NLS */
    gtk_item_factory_create_items (factory, n_entries, entries, data);

    if (window)
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    widget = gtk_item_factory_get_widget (factory, path);
    gtk_signal_connect (GTK_OBJECT (widget), "destroy",
			GTK_SIGNAL_FUNC (cb_menu_destroy), factory);

    return widget;
}

void
menu_item_set_sensitive (GtkWidget * widget, gchar * path, gboolean sensitive)
{
    GtkWidget *menuitem;
    GtkItemFactory *ifactory;

    ifactory = gtk_item_factory_from_widget (widget);
    menuitem = gtk_item_factory_get_item (ifactory, path);
    gtk_widget_set_sensitive (menuitem, sensitive);
}

/*
 *  menu_set_check_item:
 *     @ Set check menu item's value (TRUE or FALSE).
 *
 *  widget : Menu widget that contains check menu item.
 *  path   : Menu path to check menu item.
 *  active : Value for set.
 */
void
menu_check_item_set_active (GtkWidget * widget, gchar * path, gboolean active)
{
    GtkWidget *menuitem;
    GtkItemFactory *ifactory;

    ifactory = gtk_item_factory_from_widget (widget);
    menuitem = gtk_item_factory_get_item (ifactory, path);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), active);
}

/*
 *  menu_set_check_item:
 *     @ Set check menu item's value (TRUE or FALSE).
 *
 *  widget : Menu widget that contains check menu item.
 *  path   : Menu path to check menu item.
 *  active : Value for set.
 */
gboolean
menu_check_item_get_active (GtkWidget * widget, gchar * path)
{
    GtkWidget *menuitem;
    GtkItemFactory *ifactory;

    ifactory = gtk_item_factory_from_widget (widget);
    menuitem = gtk_item_factory_get_item (ifactory, path);
    return GTK_CHECK_MENU_ITEM (menuitem)->active;
}

/*
 *  menu_set_submenu:
 *     @ Set sub menu.
 *
 *  widget  : Menu widget to set sub menu.
 *  path    : Menu path to check menu item.
 *  submenu : Submenu widget.
 */
void
menu_set_submenu (GtkWidget * widget, const gchar * path, GtkWidget * submenu)
{
    GtkWidget *menuitem;
    GtkItemFactory *ifactory;

    ifactory = gtk_item_factory_from_widget (widget);
    menuitem = gtk_item_factory_get_item (ifactory, path);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
}

GtkWidget *
menu_get_submenu (GtkWidget * widget, const gchar * path)
{
    GtkWidget *menuitem;
    GtkItemFactory *ifactory;

    ifactory = gtk_item_factory_from_widget (widget);
    menuitem = gtk_item_factory_get_item (ifactory, path);
    g_return_val_if_fail (menuitem, NULL);
    return GTK_MENU_ITEM (menuitem)->submenu;
}

/*
 *  menu_remove_submenu:
 *     @ Set sub menu.
 *
 *  widget  : Menu widget to set sub menu.
 *  path    : Menu path to check menu item.
 *  submenu : Submenu widget.
 */
void
menu_remove_submenu (GtkWidget * widget, const gchar * path,
		     GtkWidget * submenu)
{
    GtkWidget *menuitem;
    GtkItemFactory *ifactory;

    ifactory = gtk_item_factory_from_widget (widget);
    menuitem = gtk_item_factory_get_item (ifactory, path);
    gtk_menu_item_remove_submenu (GTK_MENU_ITEM (menuitem));
}

/******************************************************************************
 *
 *   option menu
 *
 ******************************************************************************/
/*
 *  menu_option_simple:
 *     @ Create option menu widget. Return val will store to data.
 *
 *  menu_items : Menu entries.
 *  def_val    : Default value.
 *  data       : Pointer to gint for store return value when a menuitem has been
 *               selected.
 *  Return     : Option menu widget.
 */
GtkWidget *
menu_option_simple (const gchar ** menu_items, gint def_val, gint * data)
{
    GtkWidget *option_menu;
    GtkWidget *menu_item;
    GtkWidget *menu;
    gint    i;

    option_menu = gtk_option_menu_new ();
    menu = gtk_menu_new ();

    for (i = 0; menu_items[i]; i++)
    {
	menu_item = gtk_menu_item_new_with_label (_(menu_items[i]));
	gtk_object_set_data (GTK_OBJECT (menu_item), "num",
			     GINT_TO_POINTER (i));
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    GTK_SIGNAL_FUNC (cb_get_data_from_menuitem),
			    data);
	gtk_menu_append (GTK_MENU (menu), menu_item);
	gtk_widget_show (menu_item);
    }
    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), def_val);

    return option_menu;
}

/*
 *  menu_option:
 *     @ Create option menu widget.
 *
 *  menu_items : Menu entries.
 *  def_val    : Default value.
 *  func       : Callback function for each menu items.
 *  data       : Pointer to user data for callback function.
 *  Return     : Option menu widget.
 */
GtkWidget *
menu_option (const gchar ** menu_items, gint def_val,
	     gpointer func, gpointer data)
{
    GtkWidget *option_menu;
    GtkWidget *menu_item;
    GtkWidget *menu;
    gint    i;

    option_menu = gtk_option_menu_new ();
    gtk_widget_set_name (option_menu, "/ThumbWin/DispModeOptionMenu");
    menu = gtk_menu_new ();

    for (i = 0; menu_items[i]; i++)
    {
	menu_item = gtk_menu_item_new_with_label (_(menu_items[i]));
	gtk_object_set_data (GTK_OBJECT (menu_item), "num",
			     GINT_TO_POINTER (i));
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    GTK_SIGNAL_FUNC (func), data);
	gtk_menu_append (GTK_MENU (menu), menu_item);
	gtk_widget_show (menu_item);
    }
    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), def_val);

    return option_menu;
}

/******************************************************************************
 *
 *   modal popup menu
 *
 ******************************************************************************/
static void
cb_get_data_from_menuitem (GtkWidget * widget, gint * conf)
{
    *conf =
	GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), "num"));
}

void
menu_modal_cb (gpointer data, guint action, GtkWidget * menuitem)
{
    gtk_object_set_data (GTK_OBJECT (menuitem->parent), "return_val",
			 GINT_TO_POINTER (action));
}

static void
menu_shell_deactivated (GtkMenuShell * menu_shell, gpointer data)
{
    gtk_main_quit ();
}

/*
 *  menu_popup_modal:
 *     @runs the popup menu modally and returns the callback_action value of the
 *      selected item entry, or -1 if none..
 *
 *  popup     : GtkMenu widget to popup.
 *  pos_func  :
 *  pos_data  :
 *  event     :
 *  user_data : not used yet.
 *  Return    : selected value.
 */
gint
menu_popup_modal (GtkWidget * popup,
		  GtkMenuPositionFunc pos_func,
		  gpointer pos_data,
		  GdkEventButton * event, gpointer user_data)
{
    guint   id;
    guint   button;
    guint32 timestamp;
    gint    retval;

    g_return_val_if_fail (popup != NULL, -1);
    g_return_val_if_fail (GTK_IS_WIDGET (popup), -1);

    gtk_object_set_data (GTK_OBJECT (popup), "return_val",
			 GINT_TO_POINTER (-1));

    id = gtk_signal_connect (GTK_OBJECT (popup), "deactivate",
			     (GtkSignalFunc) menu_shell_deactivated, NULL);

    if (event)
    {
	button = event->button;
	timestamp = event->time;
    }
    else
    {
	button = 0;
	timestamp = GDK_CURRENT_TIME;
    }

    gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
		    pos_func, pos_data, button, timestamp);
    gtk_grab_add (popup);
    gtk_main ();
    gtk_grab_remove (popup);

    gtk_signal_disconnect (GTK_OBJECT (popup), id);

    retval = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (popup),
						   "return_val"));
    return retval;
}

void
menu_calc_popup_position (GtkMenu * menu, gint * x_ret, gint * y_ret,
#ifdef USE_GTK2
			  gboolean * push_in,
#endif
			  gpointer data)
{
    GdkWindow *window = data;
    gint    x = 0, y = 0, w = 0, h = 0, cursor_x = 0, cursor_y = 0;
    GdkModifierType mask;

    g_return_if_fail (x_ret && y_ret);
    g_return_if_fail (window);

    gdk_window_get_pointer (window, &cursor_x, &cursor_y, &mask);

    gdk_window_get_origin (window, &x, &y);
    gdk_window_get_size (window, &w, &h);

    if (cursor_x < 0 || cursor_x > w || cursor_y < 0 || cursor_y > h)
    {
	*x_ret = x + w / 2;
	*y_ret = y + h / 2;
    }
    else
    {
	*x_ret = x + cursor_x;
	*y_ret = y + cursor_y;
    }
}
