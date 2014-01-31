/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GQview.
 * GQview author: John Ellis
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h>	/* for keyboard values */

#include "generic_dialog.h"
#include "intl.h"

void
generic_dialog_close (GenericDialog * gd)
{
    gtk_widget_destroy (gd->dialog);
    g_free (gd);
}

static void
cb_generic_dialog_click (GtkWidget * widget, gpointer data)
{
    GenericDialog *gd = data;
    void    (*func) (GenericDialog *, gpointer);
    gint    auto_close;

    func = gtk_object_get_data (GTK_OBJECT (widget), "dialog_function");
    auto_close = gd->auto_close;

    if (func)
	func (gd, gd->data);
    if (auto_close)
	generic_dialog_close (gd);
}

static  gint
cb_generic_dialog_default_key_press (GtkWidget * widget, GdkEventKey * event,
				     gpointer data)
{
    GenericDialog *gd = data;

    if (event->keyval == GDK_Return && GTK_WIDGET_HAS_FOCUS (widget))
    {
	if (gd->cb_default)
	    gd->cb_default (gd, gd->data);
	if (gd->auto_close)
	    cb_generic_dialog_click (widget, data);
	return TRUE;
    }
    return FALSE;
}

void
generic_dialog_attach_default (GenericDialog * gd, GtkWidget * widget)
{
    if (!gd || !widget)
	return;
    gtk_signal_connect (GTK_OBJECT (widget), "key_press_event",
			GTK_SIGNAL_FUNC (cb_generic_dialog_default_key_press),
			gd);
}

static  gint
cb_generic_dialog_key_press (GtkWidget * widget, GdkEventKey * event,
			     gpointer data)
{
    GenericDialog *gd = data;

    if (event->keyval == GDK_Escape)
    {
	if (gd->cb_cancel)
	    gd->cb_cancel (gd, gd->data);
	if (gd->auto_close)
	    cb_generic_dialog_click (widget, data);
	return TRUE;
    }
    return FALSE;
}

static  gint
cb_generic_dialog_delete (GtkWidget * w, GdkEventAny * event, gpointer data)
{
    GenericDialog *gd = data;
    gint    auto_close;

    auto_close = gd->auto_close;

    if (gd->cb_cancel)
	gd->cb_cancel (gd, gd->data);
    if (auto_close)
	generic_dialog_close (gd);

    return TRUE;
}

static void
cb_generic_dialog_show (GtkWidget * widget, gpointer data)
{
    GenericDialog *gd = data;
    if (gd->cancel_button)
    {
	gtk_box_reorder_child (GTK_BOX (gd->hbox), gd->cancel_button, -1);
    }

    gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				   GTK_SIGNAL_FUNC (cb_generic_dialog_show),
				   gd);
}

GtkWidget *
generic_dialog_add (GenericDialog * gd, const gchar * text,
		    void (*cb_func) (GenericDialog *, gpointer),
		    gint is_default)
{
    GtkWidget *button;
    button = gtk_button_new_with_label (text);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_object_set_data (GTK_OBJECT (button), "dialog_function", cb_func);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_generic_dialog_click), gd);

    gtk_container_add (GTK_CONTAINER (gd->hbox), button);

    if (is_default)
    {
	gtk_widget_grab_default (button);
	gtk_widget_grab_focus (button);
	gd->cb_default = cb_func;
    }

    gtk_widget_show (button);

    return button;
}

void
generic_dialog_setup (GenericDialog * gd,
		      const gchar * title, const gchar * message,
		      const gchar * wmclass, const gchar * wmsubclass,
		      gint auto_close, void (*cb_cancel) (GenericDialog *,
							  gpointer),
		      gpointer data)
{
    GtkWidget *vbox;
    GtkWidget *label;

    gd->auto_close = auto_close;
    gd->data = data;
    gd->cb_cancel = cb_cancel;

    gd->dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_wmclass (GTK_WINDOW (gd->dialog), wmsubclass, wmclass);
    gtk_signal_connect (GTK_OBJECT (gd->dialog), "delete_event",
			GTK_SIGNAL_FUNC (cb_generic_dialog_delete), gd);
    gtk_signal_connect (GTK_OBJECT (gd->dialog), "key_press_event",
			GTK_SIGNAL_FUNC (cb_generic_dialog_key_press), gd);
    gtk_window_set_policy (GTK_WINDOW (gd->dialog), FALSE, TRUE, TRUE);
    gtk_window_set_title (GTK_WINDOW (gd->dialog), title);
    gtk_container_border_width (GTK_CONTAINER (gd->dialog), 5);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (gd->dialog), vbox);
    gtk_widget_show (vbox);

    gd->vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), gd->vbox, TRUE, TRUE, 5);
    gtk_widget_show (gd->vbox);

    if (message)
    {
	label = gtk_label_new (message);
	gtk_box_pack_start (GTK_BOX (gd->vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
    }

    label = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 5);
    gtk_widget_show (label);

    gd->hbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (gd->hbox), GTK_BUTTONBOX_END);

    gtk_button_box_set_spacing (GTK_BUTTON_BOX (gd->hbox), 0);
    gtk_button_box_set_child_size (GTK_BUTTON_BOX (gd->hbox), 95, 35);

    gtk_box_pack_start (GTK_BOX (vbox), gd->hbox, FALSE, FALSE, 0);
    gtk_widget_show (gd->hbox);

    if (gd->cb_cancel)
    {
	gd->cancel_button =
	    generic_dialog_add (gd, _("Cancel"), gd->cb_cancel, TRUE);
    }
    else
    {
	gd->cancel_button = NULL;
    }

    gtk_signal_connect (GTK_OBJECT (gd->dialog), "show",
			GTK_SIGNAL_FUNC (cb_generic_dialog_show), gd);

    gd->cb_default = NULL;
}

GenericDialog *
generic_dialog_new (const gchar * title, const gchar * message,
		    const gchar * wmclass, const gchar * wmsubclass,
		    gint auto_close, void (*cb_cancel) (GenericDialog *,
							gpointer),
		    gpointer data)
{
    GenericDialog *gd;

    gd = g_new0 (GenericDialog, 1);
    generic_dialog_setup (gd, title, message, wmclass, wmsubclass, auto_close,
			  cb_cancel, data);
    return gd;
}
