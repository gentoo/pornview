/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "button.h"

GtkWidget *
button_create (GtkWidget * box, gchar ** pixmap_data, gint toggle,
	       GtkTooltips * tooltips, const gchar * tip_text,
	       GtkSignalFunc func, gpointer data)
{
    GtkWidget *button;
    GtkWidget *icon;
    GdkPixbuf *pixbuf;
    GdkPixmap *pixmap;
    GdkBitmap *mask;

    if (toggle)
    {
	button = gtk_toggle_button_new ();
    }
    else
    {
	button = gtk_button_new ();
    }

    gtk_signal_connect (GTK_OBJECT (button), "clicked", func, data);
    gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
    gtk_tooltips_set_tip (tooltips, button, tip_text, NULL);

    pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) pixmap_data);
    gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &mask, 128);
    gdk_pixbuf_unref (pixbuf);

    icon = gtk_pixmap_new (pixmap, mask);
    gtk_widget_show (icon);
    gtk_container_add (GTK_CONTAINER (button), icon);

    gdk_pixmap_unref (pixmap);

    if (mask)
	gdk_bitmap_unref (mask);

    return button;
}
