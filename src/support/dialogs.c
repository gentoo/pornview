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
#include <sys/time.h>
#include <sys/types.h>

#include "intl.h"
#include "dialogs.h"
#include "pixbuf_utils.h"
#include "file_utils.h"

#include "pixmaps/alert.xpm"
#include "pixmaps/question.xpm"

static  gint
cb_dummy (GtkWidget * button, gpointer data)
{
    return TRUE;
}

static void
cb_dialog_confirm_yes (GtkWidget * button, ConfirmType * type)
{
    *type = CONFIRM_YES;
    gtk_main_quit ();
}

static void
cb_dialog_confirm_no (GtkWidget * button, ConfirmType * type)
{
    *type = CONFIRM_NO;
    gtk_main_quit ();
}

ConfirmType
dialog_confirm (const gchar * title, const gchar * message,
		GtkWidget * parent)
{
    ConfirmType retval = CONFIRM_NO;
    GtkWidget *dialog;
    GtkWidget *vbox, *hbox, *button, *label;
    GtkWidget *icon;

    dialog = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dialog), title);

#ifndef USE_GTK2
    GTK_WINDOW (dialog)->type = GTK_WINDOW_DIALOG;
#endif

    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    if (parent)
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
				      GTK_WINDOW (parent));

    gtk_widget_set_usize (GTK_WIDGET (dialog), 350, -1);

    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC (cb_dialog_confirm_no), NULL);

    /*
     * message area 
     */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE,
			0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 15);
    gtk_widget_show (vbox);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_widget_show (hbox);

    /*
     * icon 
     */
    icon = pixbuf_create_pixmap_from_xpm_data (question_xpm);
    gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
    gtk_widget_show (icon);

    /*
     * message 
     */
    label = gtk_label_new (message);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_set_usize (GTK_WIDGET (label), 220, -1);
    gtk_widget_show (label);

#ifdef USE_GTK2
    gtk_button_box_set_layout (GTK_BUTTON_BOX
			       (GTK_DIALOG (dialog)->action_area),
			       GTK_BUTTONBOX_END);
#endif

    /*
     * buttons
     */
    button = gtk_button_new_with_label (_("Yes"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			button, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_confirm_yes), &retval);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_show (button);

#ifndef USE_GTK2
    gtk_widget_set_usize (GTK_WIDGET (button), 150, 35);
#endif

    button = gtk_button_new_with_label (_("No"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			button, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_confirm_no), &retval);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

#ifndef USE_GTK2
    gtk_widget_set_usize (GTK_WIDGET (button), 150, 35);
#endif

    gtk_widget_show (button);

    gtk_widget_grab_default (button);

    gtk_widget_show (dialog);

    gtk_grab_add (dialog);
    gtk_main ();
    gtk_grab_remove (dialog);

    gtk_widget_destroy (dialog);

    return retval;
}

static void
cb_dialog_message_quit (GtkWidget * button, gpointer data)
{
    gtk_widget_destroy (data);
}

void
dialog_message (const gchar * title, const gchar * message,
		GtkWidget * parent)
{
    GtkWidget *dialog;
    GtkWidget *dialog_vbox;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *dialog_action_area;
    GtkWidget *button;
    GtkWidget *icon;

    dialog = gtk_dialog_new ();
    gtk_object_set_data (GTK_OBJECT (dialog), "dialog", dialog);
    gtk_window_set_title (GTK_WINDOW (dialog), title);

#ifndef USE_GTK2
    GTK_WINDOW (dialog)->type = GTK_WINDOW_DIALOG;
#endif

    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_widget_set_usize (GTK_WIDGET (dialog), 350, -1);

    if (parent)
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
				      GTK_WINDOW (parent));

    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC (gtk_widget_destroy), dialog);

    dialog_vbox = GTK_DIALOG (dialog)->vbox;
    gtk_object_set_data (GTK_OBJECT (dialog), "dialog_vbox", dialog_vbox);
    gtk_widget_show (dialog_vbox);

    /*
     * message area 
     */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox);
    gtk_object_set_data_full (GTK_OBJECT (dialog), "vbox", vbox,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 15);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_widget_show (hbox);

    /*
     * icon 
     */
    icon = pixbuf_create_pixmap_from_xpm_data (alert_xpm);
    gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
    gtk_widget_show (icon);

    /*
     * message 
     */
    label = gtk_label_new (message);
    gtk_widget_ref (label);
    gtk_object_set_data_full (GTK_OBJECT (dialog), "label", label,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_widget_set_usize (GTK_WIDGET (label), 220, -1);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    dialog_action_area = GTK_DIALOG (dialog)->action_area;
    gtk_object_set_data (GTK_OBJECT (dialog), "dialog_action_area",
			 dialog_action_area);
    gtk_widget_show (dialog_action_area);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area), 5);

#ifdef USE_GTK2
    gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area),
			       GTK_BUTTONBOX_SPREAD);
#endif

    button = gtk_button_new_with_label (_("Close"));
    gtk_widget_ref (button);
    gtk_object_set_data_full (GTK_OBJECT (dialog), "button", button,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (button);

#ifndef USE_GTK2
    gtk_widget_set_usize (GTK_WIDGET (button), 150, 35);
#else
    gtk_widget_set_usize (GTK_WIDGET (button), 150, -2);
#endif

    gtk_box_pack_start (GTK_BOX (dialog_action_area), button, FALSE, FALSE,
			0);

    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_message_quit), dialog);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    gtk_widget_grab_default (button);

    gtk_widget_show (dialog);
}

GtkWidget *
dialog_choose_dir (const gchar * title)
{
    GtkWidget *dialog;

    dialog = gtk_file_selection_new (title);

#ifndef USE_GTK2
    GTK_WINDOW (dialog)->type = GTK_WINDOW_DIALOG;
#endif

    gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

    gtk_widget_hide (GTK_FILE_SELECTION (dialog)->file_list->parent);
    gtk_widget_hide (GTK_FILE_SELECTION (dialog)->fileop_del_file);
    gtk_widget_hide (GTK_FILE_SELECTION (dialog)->fileop_ren_file);
    gtk_widget_hide (GTK_FILE_SELECTION (dialog)->fileop_c_dir);
    gtk_widget_hide (GTK_FILE_SELECTION (dialog)->selection_entry);
    gtk_widget_hide (GTK_FILE_SELECTION (dialog)->selection_text);

    return dialog;
}

static void
cb_dialog_choose_dir_modal_ok (GtkWidget * button, gboolean * retval)
{
    *retval = TRUE;
    gtk_main_quit ();
}

static void
cb_dialog_choose_dir_modal_cancel (GtkWidget * button, gboolean * retval)
{
    *retval = FALSE;
    gtk_main_quit ();
}

gchar  *
dialog_choose_dir_modal (const gchar * title, const gchar * default_path,
			 GtkWindow * window)
{
    GtkWidget *filesel = NULL;
    GtkWidget *button;
    gchar  *filename = NULL;
    gboolean retval = FALSE;

    filesel = dialog_choose_dir (title);

    gtk_signal_connect (GTK_OBJECT (filesel), "delete_event",
			GTK_SIGNAL_FUNC (cb_dummy), NULL);

    button = GTK_FILE_SELECTION (filesel)->ok_button;
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_choose_dir_modal_ok),
			&retval);
    button = GTK_FILE_SELECTION (filesel)->cancel_button;
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_choose_dir_modal_cancel),
			&retval);

    if (default_path && *default_path)
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
					 default_path);

    gtk_widget_show (filesel);

    if (window)
	gtk_window_set_transient_for (GTK_WINDOW (filesel), window);

    gtk_grab_add (filesel);
    gtk_main ();

    if (retval)
    {
	const gchar *tmpstr;
	tmpstr =
	    gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
	filename = g_strdup (tmpstr);
    }

    gtk_grab_remove (filesel);
    gtk_widget_destroy (filesel);

    return filename;
}

static void
cb_dialog_textentry_enter (GtkWidget * button, gboolean * ok_pressd)
{
    *ok_pressd = TRUE;
    gtk_main_quit ();
}


static void
cb_dialog_textentry_ok_button (GtkWidget * button, gboolean * ok_pressd)
{
    *ok_pressd = TRUE;
    gtk_main_quit ();
}


static void
cb_dialog_textentry_cancel_button (GtkWidget * button, gboolean * ok_pressd)
{
    *ok_pressd = FALSE;
    gtk_main_quit ();
}

gchar  *
dialog_textentry (const gchar * title,
		  const gchar * label_text,
		  const gchar * entry_text,
		  GList * text_list, gint entry_width, TextEntryFlags flags)
{
    GtkWidget *window, *box, *hbox, *vbox, *button, *label, *combo, *entry;
    gboolean ok_pressed = FALSE;
    gchar  *str = NULL;

    /*
     * dialog window 
     */
    window = gtk_dialog_new ();

#ifndef USE_GTK2
    GTK_WINDOW (window)->type = GTK_WINDOW_DIALOG;
#endif

    gtk_window_set_title (GTK_WINDOW (window), title);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			GTK_SIGNAL_FUNC (cb_dummy), NULL);
    gtk_widget_show (window);

    /*
     * main area 
     */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox, TRUE, TRUE,
			0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
    gtk_widget_show (vbox);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_widget_show (hbox);

    /*
     * label 
     */
    label = gtk_label_new (label_text);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    /*
     * entry 
     */
    if (flags & TEXT_ENTRY_WRAP_ENTRY)
	box = vbox;
    else
	box = hbox;

    combo = gtk_combo_new ();
    entry = GTK_COMBO (combo)->entry;

    if (text_list)
	gtk_combo_set_popdown_strings (GTK_COMBO (combo), text_list);
    else
	gtk_widget_hide (GTK_COMBO (combo)->button);

    gtk_box_pack_start (GTK_BOX (box), combo, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (entry), "activate",
			GTK_SIGNAL_FUNC (cb_dialog_textentry_enter),
			&ok_pressed);
    if (entry_text)
	gtk_entry_set_text (GTK_ENTRY (entry), entry_text);
    if (flags & TEXT_ENTRY_CURSOR_TOP)
	gtk_entry_set_position (GTK_ENTRY (entry), 0);
    if (entry_width > 0)
	gtk_widget_set_usize (combo, entry_width, -1);
    gtk_widget_show (combo);

    if (flags & TEXT_ENTRY_NO_EDITABLE)
	gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);

    gtk_widget_grab_focus (entry);

    /*
     * button 
     */
    button = gtk_button_new_with_label (_("OK"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
			button, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_textentry_ok_button),
			&ok_pressed);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_show (button);

    button = gtk_button_new_with_label (_("Cancel"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
			button, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_textentry_cancel_button),
			&ok_pressed);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_show (button);

    gtk_grab_add (window);
    gtk_main ();
    gtk_grab_remove (window);

    if (ok_pressed)
	str = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    gtk_widget_destroy (window);

    return str;
}

static void
cb_dialog_progress_cancel (GtkWidget * button, gboolean * cancel_pressed)
{
    *cancel_pressed = TRUE;
}

void
dialog_progress_update (GtkWidget * window,
			gchar * title, gchar * message,
			gchar * progress_text, gfloat progress)
{
    GtkWidget *label;
    GtkWidget *progressbar;

    g_return_if_fail (window);

    label = gtk_object_get_data (GTK_OBJECT (window), "label");
    progressbar = gtk_object_get_data (GTK_OBJECT (window), "progressbar");

    g_return_if_fail (label && progressbar);

    if (title)
	gtk_window_set_title (GTK_WINDOW (window), _(title));
    if (message)
	gtk_label_set_text (GTK_LABEL (label), message);
    if (progress_text)
	gtk_progress_set_format_string (GTK_PROGRESS (progressbar),
					progress_text);
    if (progress > 0.0 && progress < 1.0)
	gtk_progress_bar_update (GTK_PROGRESS_BAR (progressbar), progress);
}

GtkWidget *
dialog_progress_create (gchar * title, gchar * initial_message,
			gboolean * cancel_pressed, gint width, gint height)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *progressbar;
    GtkWidget *button;

    g_return_val_if_fail (title && initial_message && cancel_pressed, NULL);

    *cancel_pressed = FALSE;

    /*
     * create dialog window 
     */
    window = gtk_dialog_new ();
    gtk_container_border_width (GTK_CONTAINER (window), 3);
    gtk_window_set_title (GTK_WINDOW (window), title);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (window), width, height);
    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			GTK_SIGNAL_FUNC (cb_dummy), NULL);

    /*
     * message area 
     */
    vbox = gtk_vbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox,
			TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
    gtk_widget_show (vbox);

    /*
     * label 
     */
    label = gtk_label_new (initial_message);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    /*
     * progress bar 
     */
    progressbar = gtk_progress_bar_new ();
    gtk_progress_set_show_text (GTK_PROGRESS (progressbar), TRUE);
    gtk_box_pack_start (GTK_BOX (vbox), progressbar, FALSE, FALSE, 0);

    /*
     * cancel button 
     */
    button = gtk_button_new_with_label (_("Cancel"));
    gtk_container_border_width (GTK_CONTAINER (button), 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
			TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dialog_progress_cancel),
			cancel_pressed);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    gtk_object_set_data (GTK_OBJECT (window), "label", label);
    gtk_object_set_data (GTK_OBJECT (window), "progressbar", progressbar);

    gtk_widget_show_all (window);

    return window;
}
