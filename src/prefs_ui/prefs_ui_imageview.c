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

#include "prefs_ui_imageview.h"
#include "prefs_win.h"
#include "imageview.h"
#include "thumbview.h"

extern Config *config_changed;
extern Config *config_prechanged;

static const gchar *interpolation_items[] = {
    N_("Nearest"),
    N_("Tiles"),
    N_("Bilinear"),
    N_("Hyper"),
    NULL
};

static const gchar *dithering_items[] = {
    N_("None"),
    N_("Normal"),
    N_("Best"),
    NULL
};

typedef struct _CurrentZoomState
{
    gboolean apply;
    gint    type;
    gint    x_scale;
    gint    y_scale;
    gint    x_pos;
    gint    y_pos;
}
CurrentZoomState;

static CurrentZoomState current_zoom_state;

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_image_fit (GtkWidget * radiobutton, gpointer data)
{
    gint    idx = GPOINTER_TO_INT (data);

    if (!GTK_TOGGLE_BUTTON (radiobutton)->active)
	return;

    config_changed->image_fit = idx;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_imageview_page (void)
{
    GtkWidget *main_vbox;
    GtkWidget *frame1;
    GtkWidget *table;
    GtkWidget *label1;
    GtkWidget *label2;
    GtkWidget *optionmenu1;
    GtkWidget *optionmenu2;
    GtkWidget *frame2;
    GtkWidget *vbox;
    GtkWidget *radiobutton1;
    GtkWidget *radiobutton2;
    GSList *vbox_group = NULL;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    frame1 = gtk_frame_new (_(" Quality "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame1, FALSE, TRUE, 0);

    table = gtk_table_new (2, 2, TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 10);

    gtk_container_add (GTK_CONTAINER (frame1), table);

    label1 = gtk_label_new (_("Dithering method:"));
    gtk_table_attach (GTK_TABLE (table), label1, 0, 1, 0, 1,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

    label2 = gtk_label_new (_("Zoom (scaling):"));
    gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 1, 2,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);


    optionmenu1 = menu_option_simple (interpolation_items, conf.image_quality,
				      (gint *) & config_changed->
				      image_quality);

    gtk_table_attach (GTK_TABLE (table), optionmenu1, 1, 2, 1, 2,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);

    optionmenu2 = menu_option_simple (dithering_items, conf.image_dithering,
				      (gint *) & config_changed->
				      image_dithering);

    gtk_table_attach (GTK_TABLE (table), optionmenu2, 1, 2, 0, 1,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);

    gtk_widget_show_all (frame1);

    frame2 = gtk_frame_new (_(" Zoom "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame2, FALSE, FALSE, 10);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

    gtk_container_add (GTK_CONTAINER (frame2), vbox);

    radiobutton1 =
	gtk_radio_button_new_with_label (vbox_group,
					 _("Zoom to original size"));
    vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
    gtk_box_pack_start (GTK_BOX (vbox), radiobutton1, FALSE, FALSE, 0);

    radiobutton2 =
	gtk_radio_button_new_with_label (vbox_group,
					 _
					 ("Fit image to window, if image is larger"));
    vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
    gtk_box_pack_start (GTK_BOX (vbox), radiobutton2, FALSE, FALSE, 0);

    gtk_widget_show_all (frame2);

    if (conf.image_fit)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton2), TRUE);
    else
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton1), TRUE);

    gtk_signal_connect (GTK_OBJECT (radiobutton1), "toggled",
			GTK_SIGNAL_FUNC
			(cb_image_fit), GINT_TO_POINTER (FALSE));

    gtk_signal_connect (GTK_OBJECT (radiobutton2), "toggled",
			GTK_SIGNAL_FUNC
			(cb_image_fit), GINT_TO_POINTER (TRUE));

    return main_vbox;
}

void
prefs_imageview_apply (Config * src, Config * dest, PrefsWinButton state)
{
    switch (state)
    {
      case PREFS_WIN_OK:
	  if (dest->image_fit)
	      imageview_zoom_auto ();
	  else
	      imageview_no_zoom ();

	  current_zoom_state.apply = FALSE;
	  break;

      case PREFS_WIN_APPLY:
	  if (!current_zoom_state.apply)
	  {
	      current_zoom_state.type = IMAGEVIEW_ZOOM_TYPE;
	      current_zoom_state.x_scale = IMAGEVIEW->x_scale;
	      current_zoom_state.y_scale = IMAGEVIEW->y_scale;
	      current_zoom_state.x_pos = IMAGEVIEW->x_pos;
	      current_zoom_state.y_pos = IMAGEVIEW->y_pos;
	  }

	  if (dest->image_fit)
	      imageview_zoom_auto ();
	  else
	      imageview_no_zoom ();

	  current_zoom_state.apply = TRUE;
	  break;

      case PREFS_WIN_CANCEL:
	  if (current_zoom_state.apply)
	  {
	      if (current_zoom_state.type == IMAGEVIEW_ZOOM_IN)
	      {
		  IMAGEVIEW->x_scale =
		      current_zoom_state.x_scale - IMG_MIN_SCALE;
		  IMAGEVIEW->y_scale =
		      current_zoom_state.y_scale - IMG_MIN_SCALE;
	      }
	      else if (current_zoom_state.type == IMAGEVIEW_ZOOM_OUT)
	      {
		  IMAGEVIEW->x_scale =
		      current_zoom_state.x_scale + IMG_MIN_SCALE;
		  IMAGEVIEW->y_scale =
		      current_zoom_state.y_scale + IMG_MIN_SCALE;
	      }

	      IMAGEVIEW->x_pos = current_zoom_state.x_pos;
	      IMAGEVIEW->y_pos = current_zoom_state.y_pos;

	      imageview_zoom_image (IMAGEVIEW, current_zoom_state.type, 0, 0);
	      thumbview_iupdate ();
	  }

	  current_zoom_state.apply = FALSE;
	  break;
    }
}
