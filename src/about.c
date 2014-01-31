/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                        (c) 2002,2003                         (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "about.h"
#include "browser.h"

static GtkWidget *dialog = NULL;

static void
cb_about_close (GtkWidget * widget, gpointer data)
{
    gtk_widget_destroy (GTK_WIDGET (data));
    dialog = NULL;
}

static void
cb_about_destroy (gpointer data)
{
    cb_about_close (NULL, data);
}

void
about_dialog (void)
{
    GtkWidget *dialog_vbox;
    GtkWidget *vbox;
    GtkWidget *frame;
    GdkPixbuf *pixbuf;
    GdkPixmap *pixmap;
    GdkBitmap *mask;
    GtkWidget *logo;
    GtkWidget *label;
    GtkWidget *dialog_action_area;
    GtkWidget *button;
    gchar  *text;

    if (dialog != NULL)
	return;

    dialog = gtk_dialog_new ();
    gtk_object_set_data (GTK_OBJECT (dialog), "dialog", dialog);

    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
    gtk_window_set_title (GTK_WINDOW (dialog), _("About"));
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
				  GTK_WINDOW (BROWSER_WINDOW));

    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC (cb_about_destroy), NULL);

    dialog_vbox = GTK_DIALOG (dialog)->vbox;
    gtk_object_set_data (GTK_OBJECT (dialog), "dialog_vbox", dialog_vbox);
    gtk_widget_show (dialog_vbox);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox, TRUE, TRUE, 0);

    frame = gtk_frame_new (NULL);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

    gtk_widget_realize (dialog);

    pixbuf =
	gdk_pixbuf_new_from_file (g_strconcat
				  (PACKAGE_DATA_DIR, "/logo.png", NULL),
				  NULL);

    if (pixbuf != NULL)
    {
	gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &mask, 127);

	gdk_pixbuf_unref (pixbuf);

	logo = gtk_pixmap_new (pixmap, mask);
	gtk_widget_show (logo);
	gtk_container_add (GTK_CONTAINER (frame), logo);
    }

    text =
	g_strconcat ("\n", _("version "), VERSION, "\n(c) 2002,2003, trem0r\n",
		     NULL);
    label = gtk_label_new (text);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    dialog_action_area = GTK_DIALOG (dialog)->action_area;
    gtk_widget_show (dialog_action_area);

    gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area), 10);

    gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area),
			       GTK_BUTTONBOX_SPREAD);

    button = gtk_button_new_with_label (_("Close"));
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (dialog_action_area), button, FALSE, TRUE, 0);

    gtk_widget_set_usize (GTK_WIDGET (button), 300, -2);

    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_about_close), dialog);

    gtk_widget_show (dialog);
}
