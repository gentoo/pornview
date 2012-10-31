/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "menu.h"

#include "prefs_ui_thumbview.h"
#include "thumbview.h"

extern Config *config_changed;

static const gchar *interpolation_items[] = {
    N_("Nearest"),
    N_("Tiles"),
    N_("Bilinear"),
    N_("Hyper"),
    NULL
};

typedef struct _CurrentThumbViewState
{
    gboolean apply;
    gint    mode;
}
CurrentThumbViewState;

static CurrentThumbViewState current_thumbview_state;

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_enable_thumb_caching (GtkWidget * button, gpointer data)
{
    config_changed->enable_thumb_caching =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data));
}

static void
cb_enable_thumb_dirs (GtkWidget * button, gpointer data)
{
    config_changed->enable_thumb_dirs =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data));
}

static void
cb_thumbview_mode (GtkWidget * radiobutton, gpointer data)
{
    gint    idx = GPOINTER_TO_INT (data);

    if (!GTK_TOGGLE_BUTTON (radiobutton)->active)
	return;

    config_changed->thumbview_mode = idx;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_thumbview_page (void)
{
    GtkWidget *main_vbox;
    GtkWidget *frame1;
    GtkWidget *hbox;
    GtkWidget *label1;
    GtkWidget *optionmenu;
    GtkWidget *frame2;
    GtkWidget *vbox1;
    GtkWidget *checkbutton1;
    GtkWidget *checkbutton2;
    GtkWidget *frame3;
    GtkWidget *vbox2;
    GSList *vbox_group = NULL;
    GtkWidget *radiobutton1;
    GtkWidget *radiobutton2;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    frame1 = gtk_frame_new (_(" Quality "));

    gtk_box_pack_start (GTK_BOX (main_vbox), frame1, FALSE, TRUE, 0);

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

    gtk_container_add (GTK_CONTAINER (frame1), hbox);

    label1 = gtk_label_new (_("Thumbnail quality:"));
    gtk_box_pack_start (GTK_BOX (hbox), label1, TRUE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

    optionmenu =
	menu_option_simple (interpolation_items, conf.thumbnail_quality,
			    (gint *) & config_changed->thumbnail_quality);

    gtk_box_pack_start (GTK_BOX (hbox), optionmenu, TRUE, TRUE, 0);

    gtk_widget_show_all (frame1);

    frame2 = gtk_frame_new (_(" Cache "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame2, FALSE, TRUE, 10);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), 10);

    gtk_container_add (GTK_CONTAINER (frame2), vbox1);

    checkbutton1 = gtk_check_button_new_with_label (_("Cache thumbnails"));
    gtk_box_pack_start (GTK_BOX (vbox1), checkbutton1, FALSE, FALSE, 0);

    checkbutton2 =
	gtk_check_button_new_with_label (_
					 ("Cache thumbnails into .thumbnails"));
    gtk_box_pack_start (GTK_BOX (vbox1), checkbutton2, FALSE, FALSE, 0);
    gtk_widget_show_all (frame2);

    frame3 = gtk_frame_new (_(" Mode "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame3, FALSE, TRUE, 10);

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);

    gtk_container_add (GTK_CONTAINER (frame3), vbox2);

    radiobutton1 =
	gtk_radio_button_new_with_label (vbox_group, _("list mode"));
    vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
    gtk_box_pack_start (GTK_BOX (vbox2), radiobutton1, FALSE, FALSE, 0);

    radiobutton2 =
	gtk_radio_button_new_with_label (vbox_group, _("preview mode"));
    vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
    gtk_box_pack_start (GTK_BOX (vbox2), radiobutton2, FALSE, FALSE, 0);
    gtk_widget_show_all (frame3);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1),
				  conf.enable_thumb_caching);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton2),
				  conf.enable_thumb_dirs);

    if (conf.thumbview_mode == 0)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton1), TRUE);
    else
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton2), TRUE);

    gtk_signal_connect (GTK_OBJECT (checkbutton1), "clicked",
			GTK_SIGNAL_FUNC
			(cb_enable_thumb_caching), checkbutton1);

    gtk_signal_connect (GTK_OBJECT (checkbutton2), "clicked",
			GTK_SIGNAL_FUNC (cb_enable_thumb_dirs), checkbutton2);

    gtk_signal_connect (GTK_OBJECT (radiobutton1), "toggled",
			GTK_SIGNAL_FUNC
			(cb_thumbview_mode), GINT_TO_POINTER (0));

    gtk_signal_connect (GTK_OBJECT (radiobutton2), "toggled",
			GTK_SIGNAL_FUNC
			(cb_thumbview_mode), GINT_TO_POINTER (1));

    return main_vbox;
}

void
prefs_thumbview_apply (Config * src, Config * dest, PrefsWinButton state)
{
    switch (state)
    {
      case PREFS_WIN_OK:
	  thumbview_set_mode (dest->thumbview_mode);

	  current_thumbview_state.apply = FALSE;
	  break;

      case PREFS_WIN_APPLY:
	  if (!current_thumbview_state.apply)
	      current_thumbview_state.mode = thumbview_get_mode ();

	  thumbview_set_mode (dest->thumbview_mode);

	  current_thumbview_state.apply = TRUE;
	  break;

      case PREFS_WIN_CANCEL:
	  if (current_thumbview_state.apply)
	      thumbview_set_mode (current_thumbview_state.mode);

	  current_thumbview_state.apply = FALSE;
	  break;
    }
}
