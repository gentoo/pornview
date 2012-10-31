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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#ifdef ENABLE_EXIF

#include "jpeg-data.h"
#include "exif-data.h"

#include "exif_view.h"
#include "dialogs.h"
#include "intl.h"

#define BUF_SIZE 4096

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_exif_view_destroy (GtkWidget * widget, ExifView * ev)
{
    g_return_if_fail (ev);

    if (ev->exif_data)
	exif_data_unref (ev->exif_data);
    ev->exif_data = NULL;

    if (ev->jpeg_data)
	jpeg_data_unref (ev->jpeg_data);
    ev->jpeg_data = NULL;

    g_free (ev);
}

static void
cb_exif_window_close (GtkWidget * button, ExifView * ev)
{
    g_return_if_fail (ev);

    gtk_widget_destroy (ev->window);
}

/*
 *-------------------------------------------------------------------
 * other private functions
 *-------------------------------------------------------------------
 */

static void
exif_view_content_list_set_data (GtkWidget * clist, ExifContent * content)
{
    const gchar *text[2];
    gint    i;

    g_return_if_fail (clist);
    g_return_if_fail (content);

    gtk_clist_clear (GTK_CLIST (clist));

    for (i = 0; i < content->count; i++)
    {
	text[0] = exif_tag_get_name (content->entries[i]->tag);
	if (text[0] && *text[0])
	    text[0] = _(text[0]);
	text[1] = exif_entry_get_value (content->entries[i]);
	if (text[1] && *text[1])
	    text[1] = _(text[1]);
	gtk_clist_append (GTK_CLIST (clist), (gchar **) text);
    }
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

ExifView *
exif_view_create_window (const gchar * filename)
{
    ExifView *ev;
    GtkWidget *button;
    gchar   buf[BUF_SIZE];

    g_return_val_if_fail (filename && *filename, NULL);

    ev = exif_view_create (filename);
    if (!ev)
	return NULL;

    ev->window = gtk_dialog_new ();
    g_snprintf (buf, BUF_SIZE, _("%s EXIF data"), filename);
    gtk_window_set_title (GTK_WINDOW (ev->window), buf);
    gtk_window_set_policy (GTK_WINDOW (ev->window), FALSE, FALSE, FALSE);

    gtk_widget_set_usize (GTK_WIDGET (ev->window), 500, 400);

    gtk_window_set_position (GTK_WINDOW (ev->window), GTK_WIN_POS_CENTER);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ev->window)->vbox),
			ev->container, TRUE, TRUE, 0);

    gtk_widget_show_all (ev->window);

    /*
     * button 
     */
    button = gtk_button_new_with_label (_("Close"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ev->window)->action_area),
			button, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_exif_window_close), ev);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_show (button);

    gtk_widget_grab_focus (button);

    return ev;
}

ExifView *
exif_view_create (const gchar * filename)
{
    JPEGData *jdata;
    ExifData *edata;
    ExifView *ev = NULL;
    ExifContent *contents[16];
    GtkWidget *notebook, *label;
    gint    i;

    gchar  *labels[] = {
	N_("IFD 0"), N_("IFD 1"), N_("Exif IFD"), N_("GPS IFD"),
	N_("Interoperability")
    };
    gint    num_labels = sizeof (labels) / sizeof (gchar *);

    gchar  *titles[] = {
	N_("Tag"), N_("Value"),
    };

    g_return_val_if_fail (filename && *filename, NULL);

    jdata = jpeg_data_new_from_file (filename);
    if (!jdata)
    {
	dialog_message (_("Error"), _("EXIF data not found."), NULL);
	return NULL;
    }

    edata = jpeg_data_get_exif_data (jdata);
    if (!edata)
    {
	dialog_message (_("Error"), _("EXIF data not found."), NULL);
	goto ERROR;
    }

    ev = g_new0 (ExifView, 1);
    ev->exif_data = edata;
    ev->jpeg_data = jdata;

    contents[0] = edata->ifd[EXIF_IFD_0];
    contents[1] = edata->ifd[EXIF_IFD_1];
    contents[2] = edata->ifd[EXIF_IFD_EXIF];
    contents[3] = edata->ifd[EXIF_IFD_GPS];
    contents[4] = edata->ifd[EXIF_IFD_INTEROPERABILITY];

    ev->container = gtk_vbox_new (FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (ev->container), "destroy",
			GTK_SIGNAL_FUNC (cb_exif_view_destroy), ev);
    gtk_widget_show (ev->container);

    notebook = gtk_notebook_new ();
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
    gtk_box_pack_start (GTK_BOX (ev->container), notebook, TRUE, TRUE, 0);
    gtk_widget_show (notebook);

    /*
     * Tag Tables 
     */
    for (i = 0; i < num_labels - 1; i++)
    {
	GtkWidget *scrolledwin, *clist;

	/*
	 * scrolled window & clist 
	 */
	label = gtk_label_new (_(labels[i]));
	scrolledwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
					GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwin), 5);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				  scrolledwin, label);
	gtk_widget_show (scrolledwin);

	clist = gtk_clist_new_with_titles (2, titles);
	gtk_clist_set_selection_mode (GTK_CLIST (clist),
				      GTK_SELECTION_SINGLE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (clist), 0, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (clist), 1, TRUE);

	gtk_container_add (GTK_CONTAINER (scrolledwin), clist);
	gtk_widget_show (clist);

	exif_view_content_list_set_data (clist, contents[i]);
    }

    return ev;

  ERROR:
    jpeg_data_unref (jdata);
    return NULL;
}

#endif /* ENABLE_EXIF */
