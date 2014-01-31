/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *      Modified to use EggTrayIcon by freelight <evaykher@optonline.net>     *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "pixbuf_utils.h"

#include "dock.h"
#include "about.h"
#include "browser.h"
#include "prefs.h"
#include "eggtrayicon.h"
#include "eggtrayicon.c"
#include "pixmaps/dock.xpm"

#define dock_exit   browser_destroy
#define dock_about  about_dialog

static void dock_main_window_hide (GtkWidget * widget, gpointer data);
static void dock_main_window_show (GtkWidget * widget, gpointer data);

static GtkItemFactoryEntry dock_popupmenu_factory[] = {
    {N_("/Exit"), NULL, dock_exit, 1, NULL},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/About"), NULL, dock_about, 1, NULL},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/Show Main Window"), NULL, dock_main_window_show, 1, NULL},
    {N_("/Hide Main Window"), NULL, dock_main_window_hide, 1, NULL}
};

static int dock_popupmenu_factory_count =
    sizeof (dock_popupmenu_factory) / sizeof (GtkItemFactoryEntry);

static GtkItemFactory *popup_item_factory;
static GtkWidget *popup_menu;
static EggTrayIcon *dock = NULL;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
dock_main_window_hide (GtkWidget * widget, gpointer data)
{
    gtk_widget_hide (BROWSER_WINDOW);
}

static void
dock_main_window_show (GtkWidget * widget, gpointer data)
{
    gtk_widget_show (BROWSER_WINDOW);
}

static void
dock_toggle_callback (GtkWidget * widget, gpointer data)
{
    if (GTK_WIDGET_VISIBLE (GTK_WIDGET (BROWSER_WINDOW)))
	gtk_widget_hide (BROWSER_WINDOW);
    else
	gtk_widget_show (BROWSER_WINDOW);
}

static void
cb_dock_clicked (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
    if (event == NULL)
	return;

    if ((event->button == 1) && (event->type == GDK_BUTTON_PRESS))
	dock_toggle_callback (NULL, NULL);
    else if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
	if (GTK_WIDGET_VISIBLE (GTK_WIDGET (BROWSER_WINDOW)))
	{
	    gtk_widget_set_sensitive (gtk_item_factory_get_item
				      (popup_item_factory,
				       "/Show Main Window"), FALSE);
	    gtk_widget_set_sensitive (gtk_item_factory_get_item
				      (popup_item_factory,
				       "/Hide Main Window"), TRUE);
	}
	else
	{
	    gtk_widget_set_sensitive (gtk_item_factory_get_item
				      (popup_item_factory,
				       "/Show Main Window"), TRUE);
	    gtk_widget_set_sensitive (gtk_item_factory_get_item
				      (popup_item_factory,
				       "/Hide Main Window"), FALSE);
	}

	gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL, NULL, NULL,
			event->button, event->time);
    }
}

static void
dock_build ()
{
    GtkWidget *image;
    GtkWidget *eventbox;

    dock = egg_tray_icon_new ("Pornview");
    eventbox = gtk_event_box_new ();
    image = pixbuf_create_pixmap_from_xpm_data (dock_xpm);

    GTK_WIDGET_SET_FLAGS (image, GTK_NO_WINDOW);
    image->requisition.width = 22;
    image->requisition.height = 22;



    gtk_widget_set_events (GTK_WIDGET (eventbox),
			   gtk_widget_get_events (eventbox) |
			   GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK);

    gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
			GTK_SIGNAL_FUNC (cb_dock_clicked), NULL);

    gtk_widget_show (eventbox);

    /*
     * add the status to the plug
     */
    gtk_object_set_data (GTK_OBJECT (dock), "pixmapg", image);
    gtk_container_add (GTK_CONTAINER (eventbox), image);
    gtk_container_add (GTK_CONTAINER (dock), eventbox);
    gtk_widget_show_all (GTK_WIDGET (dock));

    /*
     * add the popup menu
     */
    popup_item_factory =
	gtk_item_factory_new (GTK_TYPE_MENU, "<popup>", NULL);

#ifdef ENABLE_NLS
    gtk_item_factory_set_translate_func (popup_item_factory,
					 (GtkTranslateFunc) gettext, NULL,
					 NULL);
#endif

    gtk_item_factory_create_items (popup_item_factory,
				   dock_popupmenu_factory_count,
				   dock_popupmenu_factory, NULL);
    popup_menu = gtk_item_factory_get_widget (popup_item_factory, "<popup>");
}

static void
dock_setup_properties (GdkWindow * window)
{
    glong   data[1];
    GdkAtom kwm_dockwindow_atom;
    GdkAtom kde_net_system_tray_window_for_atom;

    kwm_dockwindow_atom = gdk_atom_intern ("KWM_DOCKWINDOW", FALSE);
    kde_net_system_tray_window_for_atom =
	gdk_atom_intern ("_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", FALSE);

    /*
     * this is the old KDE 1.0 and GNOME 1.2 way...
     */
    data[0] = TRUE;
    gdk_property_change (window, kwm_dockwindow_atom,
			 kwm_dockwindow_atom, 32, GDK_PROP_MODE_REPLACE,
			 (guchar *) & data, 1);

    /*
     * this is needed to support KDE 2.0
     */
    data[0] = 0;
    gdk_property_change (window, kde_net_system_tray_window_for_atom,
			 XA_WINDOW, 32, GDK_PROP_MODE_REPLACE,
			 (guchar *) & data, 1);
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
dock_init (void)
{
    if (dock)
	return;

    if (!conf.enable_dock)
	return;

    dock = (GtkWindow *) gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (dock), "proview_status");
    gtk_window_set_wmclass (GTK_WINDOW (dock), "PV_StatusDock", "proview");
    gtk_widget_set_usize (GTK_WIDGET (dock), 22, 22);
    gtk_widget_realize (GTK_WIDGET (dock));

    dock_build (dock);
    dock_setup_properties (GTK_WIDGET (dock)->window);

    gtk_widget_show (GTK_WIDGET (dock));
}

void
dock_free (void)
{
    if (dock == NULL)
	return;

    gtk_widget_destroy (GTK_WIDGET (dock));

    dock = NULL;
}
