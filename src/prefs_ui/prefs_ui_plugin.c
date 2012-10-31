/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2003                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#include "pornview.h"

#include "plugin.h"
#include "prefs.h"
#include "prefs_ui_plugin.h"

extern Config *config_changed;
extern Config *config_prechanged;

GtkWidget *
prefs_plugin_page (void)
{
    GtkWidget *main_vbox, *frame;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    frame = prefs_ui_dir_list_prefs (_("Directories list to search plugins"),
				     _("Select plugin directory"),
				     config_prechanged->
				     plugin_search_dir_list,
				     &config_changed->plugin_search_dir_list,
				     ',');
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

    gtk_widget_show_all (main_vbox);

    return main_vbox;
}

void
prefs_plugin_apply (Config * src, Config * dest, PrefsWinButton state)
{
    if (dest->plugin_search_dir_list != src->plugin_search_dir_list)
	plugin_create_search_dir_list ();
}
