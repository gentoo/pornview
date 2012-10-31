/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "dirtree.h"

#include "menu.h"

#include "prefs_ui_dirview.h"
#include "dirview.h"

typedef struct PrefsWin_Tag
{
    GtkWidget *check_dir_vbox;
}
PrefsWin;

static PrefsWin prefs_win;
extern Config *config_changed;

static const gchar *ctree_line_style_items[] = {
    N_("None"),
    N_("Solid"),
    N_("Dotted"),
    N_("Tabbed"),
    NULL
};

static const gchar *ctree_expander_style_items[] = {
    N_("None"),
    N_("Square"),
    N_("Triangle"),
    N_("Circular"),
    NULL
};

typedef struct _CurrentDirViewState
{
    gboolean apply;
    gboolean scan_dir;
    gboolean check_hlinks;
    gboolean show_dotfile;
    gboolean dirtree_line_style;
    gboolean dirtree_expander_style;
}
CurrentDirViewState;

static CurrentDirViewState current_dirview_state;

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_check_dir (GtkWidget * checkbutton, gpointer data)
{
    config_changed->scan_dir =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data));

    if (config_changed->scan_dir)
	gtk_widget_set_sensitive (GTK_WIDGET (prefs_win.check_dir_vbox),
				  TRUE);
    else
	gtk_widget_set_sensitive (GTK_WIDGET (prefs_win.check_dir_vbox),
				  FALSE);
}

static void
cb_check_dir_mode (GtkWidget * radiobutton, gpointer data)
{
    gint    idx = GPOINTER_TO_INT (data);

    if (!GTK_TOGGLE_BUTTON (radiobutton)->active)
	return;

    switch (idx)
    {
      case 0:
	  config_changed->check_hlinks = TRUE;
	  break;
      case 1:
	  config_changed->check_hlinks = FALSE;
	  break;
      default:
	  break;
    }
}

static void
cb_show_dotfile (GtkWidget * checkbutton, gpointer data)
{
    config_changed->show_dotfile =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data));
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

GtkWidget *
prefs_dirview_page (void)
{
    GtkWidget *main_vbox;
    GtkWidget *frame1;
    GtkWidget *vbox1;
    GtkWidget *checkbutton1;
    GtkWidget *radiobutton1;
    GtkWidget *radiobutton2;
    GtkWidget *checkbutton2;
    GSList *vbox_group = NULL;
    GtkWidget *frame2;
    GtkWidget *table;
    GtkWidget *label1;
    GtkWidget *label2;
    GtkWidget *optionmenu1;
    GtkWidget *optionmenu2;

    main_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

    frame1 = gtk_frame_new (_(" Directory Scan "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame1, FALSE, TRUE, 0);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), 10);
    gtk_container_add (GTK_CONTAINER (frame1), vbox1);

    checkbutton1 =
	gtk_check_button_new_with_label (_("Check subdirectories"));
    gtk_box_pack_start (GTK_BOX (vbox1), checkbutton1, FALSE, FALSE, 0);

    prefs_win.check_dir_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (prefs_win.check_dir_vbox),
				    5);
    gtk_box_pack_start (GTK_BOX (vbox1), prefs_win.check_dir_vbox, FALSE,
			FALSE, 0);

    radiobutton1 =
	gtk_radio_button_new_with_label (vbox_group,
					 _("Check number of hard links"));
    vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
    gtk_box_pack_start (GTK_BOX (prefs_win.check_dir_vbox), radiobutton1,
			FALSE, FALSE, 0);

    radiobutton2 =
	gtk_radio_button_new_with_label (vbox_group,
					 _
					 ("Do not check number of hard links"));
    vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
    gtk_box_pack_start (GTK_BOX (prefs_win.check_dir_vbox), radiobutton2,
			FALSE, FALSE, 0);

    checkbutton2 = gtk_check_button_new_with_label (_("Show dotfile"));
    gtk_box_pack_start (GTK_BOX (vbox1), checkbutton2, FALSE, FALSE, 0);
    gtk_widget_show_all (frame1);

    frame2 = gtk_frame_new (_(" Style "));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame2, FALSE, TRUE, 10);

    table = gtk_table_new (2, 2, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 10);

    gtk_container_add (GTK_CONTAINER (frame2), table);

    label1 = gtk_label_new (_("Tree line style"));
    gtk_table_attach (GTK_TABLE (table), label1, 0, 1, 0, 1,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

    label2 = gtk_label_new (_("Tree expander style"));
    gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 1, 2,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

    optionmenu1 = menu_option_simple (ctree_line_style_items,
				      conf.dirtree_line_style,
				      (gint *) & config_changed->
				      dirtree_line_style);
    gtk_table_attach (GTK_TABLE (table), optionmenu1, 1, 2, 0, 1,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);


    optionmenu2 = menu_option_simple (ctree_expander_style_items,
				      conf.dirtree_expander_style,
				      (gint *) & config_changed->
				      dirtree_expander_style);

    gtk_table_attach (GTK_TABLE (table), optionmenu2, 1, 2, 1, 2,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);

    gtk_widget_show_all (frame2);

    gtk_signal_connect (GTK_OBJECT (checkbutton1), "clicked",
			GTK_SIGNAL_FUNC (cb_check_dir), checkbutton1);

    gtk_signal_connect (GTK_OBJECT (radiobutton1), "toggled",
			GTK_SIGNAL_FUNC
			(cb_check_dir_mode), GINT_TO_POINTER (0));

    gtk_signal_connect (GTK_OBJECT (radiobutton2), "toggled",
			GTK_SIGNAL_FUNC
			(cb_check_dir_mode), GINT_TO_POINTER (1));

    gtk_signal_connect (GTK_OBJECT (checkbutton2), "clicked",
			GTK_SIGNAL_FUNC (cb_show_dotfile), checkbutton2);

    if (conf.check_hlinks)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton1), TRUE);
    else
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton2), TRUE);

    if (conf.scan_dir)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);
    else
	gtk_widget_set_sensitive (GTK_WIDGET (prefs_win.check_dir_vbox),
				  FALSE);

    if (conf.scan_dir)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton2),
				      conf.show_dotfile);

    return main_vbox;
}

void
prefs_dirview_apply (Config * src, Config * dest, PrefsWinButton state)
{
    switch (state)
    {
      case PREFS_WIN_OK:
	  dirtree_set_mode (DIRTREE (DIRVIEW_DIRTREE),
			    dest->scan_dir,
			    dest->check_hlinks,
			    dest->show_dotfile,
			    dest->dirtree_line_style,
			    dest->dirtree_expander_style);

	  current_dirview_state.apply = FALSE;
	  break;

      case PREFS_WIN_APPLY:
	  if (!current_dirview_state.apply)
	  {
	      current_dirview_state.scan_dir =
		  DIRTREE (DIRVIEW_DIRTREE)->check_dir;
	      current_dirview_state.check_hlinks =
		  DIRTREE (DIRVIEW_DIRTREE)->check_hlinks;
	      current_dirview_state.show_dotfile =
		  DIRTREE (DIRVIEW_DIRTREE)->show_dotfile;
	      current_dirview_state.dirtree_line_style =
		  DIRTREE (DIRVIEW_DIRTREE)->line_style;
	      current_dirview_state.dirtree_expander_style =
		  DIRTREE (DIRVIEW_DIRTREE)->expander_style;
	  }

	  dirtree_set_mode (DIRTREE (DIRVIEW_DIRTREE),
			    dest->scan_dir,
			    dest->check_hlinks,
			    dest->show_dotfile,
			    dest->dirtree_line_style,
			    dest->dirtree_expander_style);

	  current_dirview_state.apply = TRUE;
	  break;

      case PREFS_WIN_CANCEL:
	  if (current_dirview_state.apply)
	      dirtree_set_mode (DIRTREE (DIRVIEW_DIRTREE),
				current_dirview_state.scan_dir,
				current_dirview_state.check_hlinks,
				current_dirview_state.show_dotfile,
				current_dirview_state.dirtree_line_style,
				current_dirview_state.dirtree_expander_style);

	  current_dirview_state.apply = FALSE;
	  break;
    }
}
