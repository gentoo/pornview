/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#ifdef USE_GTK2
#define GTK_ENABLE_BROKEN 1
#include <gtk/gtktext.h>
#endif

#include "zalbum.h"

#include "file_type.h"
#include "file_utils.h"
#include "image.h"

#include "imageinfo.h"
#include "browser.h"
#include "thumbview.h"

static ImageWindow *iw;
static GtkWidget *text = NULL;
static GtkWidget *label_name = NULL;
static GtkWidget *label = NULL;
static GtkWidget *button_prev = NULL;
static GtkWidget *button_next = NULL;

static guint list_length;
static guint list_idx;
static GList *list = NULL;
static gint timer_id;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
imageinfo_set_data (void)
{
    ZAlbumCell *cell;
    char    buf[PATH_MAX];
    gchar  *time;
    gchar  *size;
    struct stat st;
    gchar  *user;
    gchar  *group;
    gchar  *mode;
    gint    type;

    cell =
	ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album), (guint) list->data);

    type =
	(gint) zalbum_get_cell_data (ZALBUM (thumbview->album),
				     (guint) list->data);
    image_set_path (iw, cell->name);

    time = time2str (filetime (cell->name));
    size = size2str ((size_t) filesize (cell->name), 0);

    stat (cell->name, &st);
    user = uid2str (st.st_uid);
    group = gid2str (st.st_gid);
    mode = mode2str (st.st_mode);

    snprintf (buf, PATH_MAX, "\n\n %s: %s\n %s: %s\n"
	      " %s: %s\n %s: %s\n %s: %s\n\t%s\n",
	      _("type"), (type == FILETYPE_VIDEO) ? _("movie") : _("image"),
	      _("size"), size, _("time"), time, _("user"), user, _("group"),
	      group, mode);

    g_free (time);
    g_free (size);
    g_free (user);
    g_free (group);
    g_free (mode);

    gtk_label_set_text (GTK_LABEL (label_name), g_basename (cell->name));

    gtk_text_freeze (GTK_TEXT (text));
    gtk_text_backward_delete (GTK_TEXT (text),
			      gtk_text_get_length (GTK_TEXT (text)));

#ifdef USE_GTK2
    gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, buf, -1);
#else
    gtk_text_insert (GTK_TEXT (text),
		     gdk_font_load
		     ("-misc-fixed-medium-r-normal-*-13-*-*-*-c-*-*-*"), NULL,
		     NULL, buf, -1);
#endif

    gtk_text_thaw (GTK_TEXT (text));

    if (list_length > 0)
    {
	snprintf (buf, PATH_MAX, "%d/%d", list_idx + 1, list_length + 1);
	gtk_label_set_text (GTK_LABEL (label), buf);
    }

    gtk_widget_set_sensitive (button_prev, list_idx != 0);
    gtk_widget_set_sensitive (button_next, list_length != list_idx);
}

/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void
cb_imageinfo_dialog_prev (GtkWidget * widget, gpointer data)
{
    list_idx--;

    if (list_idx < 0)
	list_idx = 0;

    list = g_list_previous (list);

    imageinfo_set_data ();
}

static void
cb_imageinfo_dialog_next (GtkWidget * widget, gpointer data)
{
    list_idx++;

    if (list_idx > list_length)
	list_idx = list_length;

    list = g_list_next (list);

    imageinfo_set_data ();
}

static void
cb_imageinfo_dialog_close (GtkWidget * widget, gpointer data)
{
    if (list)
	g_list_free (list);

    gtk_widget_destroy (GTK_WIDGET (data));
}

static void
cb_imageinfo_dialog_destroy (gpointer data)
{
    cb_imageinfo_dialog_close (NULL, data);
}

static  gint
valcomp (gconstpointer a, gconstpointer b)
{
    if ((guint) a > (guint) b)
	return 1;
    else if ((guint) a < (guint) b)
	return -1;

    return 0;
}

static  gint
cb_set_data (gpointer data)
{
    list_length = g_list_length (ZLIST (thumbview->album)->selection);
    list_length--;
    list_idx = 0;

    list = g_list_copy (ZLIST (thumbview->album)->selection);
    list = g_list_sort (list, (GCompareFunc) valcomp);

    imageinfo_set_data ();

    return FALSE;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
imageinfo_dialog (void)
{
    GtkWidget *dialog;
    GtkWidget *dialog_vbox;
    GtkWidget *frame;
    GtkWidget *hbox1;
    GtkWidget *vseparator;
    GtkWidget *hbox2;
    GtkWidget *dialog_action_area;
    GtkWidget *hbox3;
    GtkWidget *button;
    GtkWidget *arrow;

    dialog = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dialog), _("File Properties"));

#ifndef USE_GTK2
    GTK_WINDOW (dialog)->type = GTK_WINDOW_DIALOG;
#endif

    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
				  GTK_WINDOW (BROWSER_WINDOW));

    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC (cb_imageinfo_dialog_destroy), NULL);

    dialog_vbox = GTK_DIALOG (dialog)->vbox;
    gtk_widget_show (dialog_vbox);

    label_name = gtk_label_new ("aaa");
    gtk_box_pack_start (GTK_BOX (dialog_vbox), label_name, TRUE, TRUE, 0);
    gtk_widget_show (label_name);

    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox1);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox1, TRUE, TRUE, 0);

    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (hbox1), frame, TRUE, TRUE, 0);

    hbox2 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox2);
    gtk_container_add (GTK_CONTAINER (frame), hbox2);

    iw = image_new (TRUE);
    gtk_container_add (GTK_CONTAINER (hbox2), iw->widget);
    gtk_widget_set_usize (GTK_WIDGET (iw->widget), 150, 150);
    gtk_widget_show (iw->widget);

    vseparator = gtk_vseparator_new ();
    gtk_widget_show (vseparator);
    gtk_box_pack_start (GTK_BOX (hbox2), vseparator, TRUE, TRUE, 0);

    text = gtk_text_new (NULL, NULL);
    gtk_widget_show (text);
    gtk_box_pack_start (GTK_BOX (hbox2), text, FALSE, FALSE, 0);

    gtk_widget_set_usize (GTK_WIDGET (text), 300, 150);

    dialog_action_area = GTK_DIALOG (dialog)->action_area;
    gtk_widget_show (dialog_action_area);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area), 5);

    hbox3 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox3);
    gtk_box_pack_start (GTK_BOX (dialog_action_area), hbox3, TRUE, TRUE, 0);

    button_prev = gtk_button_new ();
    arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    gtk_container_add (GTK_CONTAINER (button_prev), arrow);
    gtk_widget_show (arrow);

    gtk_widget_show (button_prev);
    gtk_widget_set_usize (GTK_WIDGET (button_prev), 25, 10);
    gtk_box_pack_start (GTK_BOX (hbox3), button_prev, FALSE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button_prev, GTK_CAN_DEFAULT);

    gtk_signal_connect (GTK_OBJECT (button_prev), "clicked",
			GTK_SIGNAL_FUNC (cb_imageinfo_dialog_prev), NULL);
    gtk_widget_set_sensitive (button_prev, FALSE);

    button_next = gtk_button_new ();
    arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    gtk_container_add (GTK_CONTAINER (button_next), arrow);
    gtk_widget_show (arrow);

    gtk_widget_show (button_next);
    gtk_widget_set_usize (GTK_WIDGET (button_next), 25, 10);

    gtk_box_pack_start (GTK_BOX (hbox3), button_next, FALSE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button_next, GTK_CAN_DEFAULT);

    gtk_signal_connect (GTK_OBJECT (button_next), "clicked",
			GTK_SIGNAL_FUNC (cb_imageinfo_dialog_next), NULL);

    label = gtk_label_new (" ");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox3), label, TRUE, TRUE, 0);

    button = gtk_button_new_with_label (_("Close"));
    gtk_widget_show (button);

#ifdef USE_GTK2
    gtk_widget_set_usize (GTK_WIDGET (button), 280, -2);
#else
    gtk_widget_set_usize (GTK_WIDGET (button), 280, 35);
#endif

    gtk_box_pack_start (GTK_BOX (hbox3), button, FALSE, TRUE, 0);

    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);

    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_imageinfo_dialog_close), dialog);

    gtk_widget_set_sensitive (button_prev, FALSE);
    gtk_widget_set_sensitive (button_next, FALSE);

    gtk_widget_show (dialog);

    timer_id = gtk_timeout_add (100, cb_set_data, NULL);
}
