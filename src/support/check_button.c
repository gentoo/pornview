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

#include "check_button.h"

#include "intl.h"

/*
 *  check_button_create:
 *     @ Create toggle button widget.
 *
 *  label   : Label text for toggle button.
 *  def_val : Default value.
 *  Return  : Toggle button widget.
 */
GtkWidget *
check_button_create (const gchar * label_text, gboolean def_val,
		     gpointer func, gpointer data)
{
    GtkWidget *toggle;

    toggle = gtk_check_button_new_with_label (_(label_text));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), def_val);

    if (func)
	gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			    GTK_SIGNAL_FUNC (func), data);

    return toggle;
}

void
check_button_get_data_from_toggle_cb (GtkWidget * toggle, gboolean * data)
{
    g_return_if_fail (data);

    *data = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle));
}
