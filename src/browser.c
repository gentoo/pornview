/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "dirview.h"
#include "gedo-hpaned.h"
#include "gedo-vpaned.h"
#include "gedo-paned.h"
#include "zalbum.h"

#include "pixbuf_utils.h"
#include "string_utils.h"

#include "browser.h"
#include "about.h"
#include "cache.h"
#include "comment.h"
#include "comment_view.h"
#include "thumbview.h"
#include "imageview.h"
#include "videoplay.h"
#include "prefs.h"
#include "prefs_win.h"
#include "viewtype.h"

#include "pixmaps/pornview.xpm"
#include "pixmaps/exit.xpm"
#include "pixmaps/options.xpm"

Browser *browser = NULL;

static void browser_remove_old_thumbs (void);
static void browser_clear_cache (void);
static void browser_remove_old_comments (void);
static void browser_clear_comments (void);

#define browser_about  about_dialog

static GtkItemFactoryEntry browser_menu_factory[] = {
    {N_("/_File"), NULL, NULL, 0, "<Branch>"},
    {N_("/File/E_xit"), "<control>X", browser_destroy, 0, NULL},
    {N_("/_Tools"), NULL, NULL, 0, "<Branch>"},
    {N_("/Tools/Delete Old Thumbnails"), NULL, browser_remove_old_thumbs, 0,
     NULL},
    {N_("/Tools/Delete All Thumbnails"), NULL, browser_clear_cache, 0, NULL},
    {N_("/Tools/---"), NULL, NULL, 0, "<Separator>"},
    {N_("/Tools/Delete Old Comments"), NULL, browser_remove_old_comments, 0,
     NULL},
    {N_("/Tools/Delete All Comments"), NULL, browser_clear_comments, 0, NULL},
    {N_("/_Help"), NULL, NULL, 0, "<LastBranch>"},
    {N_("/Help/A_bout"), "<control>B", browser_about, 0, NULL}
};

static int browser_menu_factory_count =
    sizeof (browser_menu_factory) / sizeof (GtkItemFactoryEntry);

static gint timer_id;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static int
cb_browser_select_dir (gpointer data)
{
    browser_select_dir ((gchar *) data);

    return FALSE;
}

static void
browser_remove_old_thumbs (void)
{
    cache_maintain_home (CACHE_THUMBS, FALSE);
}

static void
browser_clear_cache (void)
{
    cache_maintain_home (CACHE_THUMBS, TRUE);
    thumbview_refresh ();
}

static void
browser_remove_old_comments (void)
{
    cache_maintain_home (CACHE_COMMENTS, FALSE);
}

static void
browser_clear_comments (void)
{
    cache_maintain_home (CACHE_COMMENTS, TRUE);
}

static void
browser_prefs ()
{
    prefs_win_open_idle ("/General");
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
browser_create (void)
{
    GtkWidget *vbox;
    GdkPixmap *icon_pix;
    GdkBitmap *icon_mask;
    GtkAccelGroup *accel;
    GtkWidget *menu;
    GtkWidget *toolbar;
    GtkWidget *hpaned;
    GtkWidget *vpaned;
    GtkWidget *iconw;
    GtkWidget *hbox;
    GtkWidget *label;

    browser = g_new (Browser, 1);

    browser->current_path = g_string_new (conf.startup_dir);
    browser->last_path = g_string_new ("");
    browser->filelist = (FileList *) file_list_init ();

    browser->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (browser->window), "PornView");
    gtk_widget_realize (browser->window);

    icon_pix =
	gdk_pixmap_create_from_xpm_d (G_WINDOW (browser->window), &icon_mask,
				      NULL, pornview_xpm);
    gdk_window_set_icon (G_WINDOW (browser->window), NULL, icon_pix,
			 icon_mask);

    gtk_signal_connect (GTK_OBJECT (browser->window), "delete_event",
			GTK_SIGNAL_FUNC (browser_destroy), NULL);

    gtk_window_set_default_size (GTK_WINDOW (browser->window),
				 conf.window_width, conf.window_height);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (browser->window), vbox);

    /*
     * main menu 
     */
    accel = gtk_accel_group_new ();
    browser->menu =
	gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<browser_menu>", accel);

#ifdef ENABLE_NLS
    gtk_item_factory_set_translate_func (browser->menu,
					 (GtkTranslateFunc) menu_translate,
					 NULL, NULL);
#endif

    gtk_item_factory_create_items (browser->menu, browser_menu_factory_count,
				   browser_menu_factory, NULL);
    menu = gtk_item_factory_get_widget (browser->menu, "<browser_menu>");

    gtk_box_pack_start (GTK_BOX (vbox), menu, FALSE, FALSE, 0);
    gtk_widget_show (menu);

#ifndef USE_GTK2
    gtk_accel_group_attach (accel, GTK_OBJECT (browser->window));
#endif

    /*
     * toolbar 
     */
#ifdef USE_GTK2
    toolbar = gtk_toolbar_new ();
#else
    toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
#endif

    gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);
    gtk_widget_show (toolbar);

    iconw = pixbuf_create_pixmap_from_xpm_data (exit_xpm);
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Exit"), NULL,
			     iconw, (GtkSignalFunc) browser_destroy, NULL);

#ifndef USE_GTK2
    gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
#endif

    gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

    iconw = pixbuf_create_pixmap_from_xpm_data (options_xpm);
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Preference"),
			     NULL, iconw, (GtkSignalFunc) browser_prefs,
			     NULL);

    hpaned = gedo_hpaned_new ();
    gtk_container_add (GTK_CONTAINER (vbox), hpaned);
    gtk_widget_show (hpaned);

    vpaned = gedo_vpaned_new ();
    gtk_widget_show (vpaned);
    gedo_paned_add1 (GEDO_PANED (hpaned), vpaned);

    /*
     * dirtree 
     */
    dirview_create (conf.startup_dir, browser->window);

    gedo_paned_add1 (GEDO_PANED (vpaned), DIRVIEW_CONTAINER);

    commentview = comment_view_create ();

    browser->notebook = commentview->notebook;

    /*
     * videoplay 
     */
#ifdef ENABLE_MOVIE
    videoplay_create ();
    label = gtk_label_new (_(" Preview "));
    gtk_notebook_prepend_page (GTK_NOTEBOOK (browser->notebook),
			       VIDEOPLAY_CONTAINER, label);

    gtk_notebook_set_page (GTK_NOTEBOOK (browser->notebook), 0);

#endif

    /*
     * imageview
     */
    imageview_create ();
    label = gtk_label_new (_(" Preview "));
    gtk_notebook_prepend_page (GTK_NOTEBOOK (browser->notebook),
			       IMAGEVIEW_CONTAINER, label);

#ifndef ENABLE_MOVIE
    gtk_notebook_set_page (GTK_NOTEBOOK (browser->notebook), 0);
#endif

    gedo_paned_add2 (GEDO_PANED (hpaned), commentview->main_vbox);

    /*
     * thumbview 
     */
    thumbview_create (browser->window);
    gedo_paned_add2 (GEDO_PANED (vpaned), THUMBVIEW_CONTAINER);

    /*
     * statusbar 
     */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_set_name (hbox, "StatusBarContainer");
    gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    browser->status_dir = gtk_statusbar_new ();
    gtk_widget_set_name (browser->status_dir, "StatusBar1");
    gtk_container_border_width (GTK_CONTAINER (browser->status_dir), 1);
    gtk_widget_set_usize (browser->status_dir, 80, 15);
    gtk_box_pack_start (GTK_BOX (hbox), browser->status_dir, FALSE, FALSE, 0);
    gtk_widget_show (browser->status_dir);

    browser->status_name = gtk_statusbar_new ();
    gtk_widget_set_name (browser->status_name, "StatusBar2");
    gtk_container_border_width (GTK_CONTAINER (browser->status_name), 1);
    gtk_widget_set_usize (browser->status_name, 150, -1);
    gtk_box_pack_start (GTK_BOX (hbox), browser->status_name, TRUE, TRUE, 0);
    gtk_widget_show (browser->status_name);

    browser->status_image = gtk_statusbar_new ();
    gtk_widget_set_name (browser->status_image, "StatusBar3");
    gtk_container_border_width (GTK_CONTAINER (browser->status_image), 1);
    gtk_widget_set_usize (browser->status_image, 150, -1);
    gtk_box_pack_start (GTK_BOX (hbox), browser->status_image, TRUE, TRUE, 0);
    gtk_widget_show (browser->status_image);

    browser->progress = gtk_progress_bar_new ();
    gtk_widget_set_name (browser->progress, "ProgressBar");
    gtk_box_pack_end (GTK_BOX (hbox), browser->progress, FALSE, FALSE, 0);
    gtk_widget_show (browser->progress);

    gtk_widget_show (browser->window);

#ifdef ENABLE_MOVIE
    gtk_widget_hide (gtk_notebook_get_nth_page
		     (GTK_NOTEBOOK (browser->notebook), 1));
    gtk_notebook_set_page (GTK_NOTEBOOK (browser->notebook), 0);
#endif

    dirview_scroll_center ();
    timer_id = gtk_timeout_add (100, cb_browser_select_dir, conf.startup_dir);
}

void
browser_destroy (void)
{
    file_list_free (BROWSER_FILE_LIST);

    if (conf.save_win_state)
    {
	conf.window_width = browser->window->allocation.width;
	conf.window_height = browser->window->allocation.height;
	conf.dirtree_width = DIRVIEW_WIDTH;
	conf.dirtree_height = DIRVIEW_HEIGHT;
    }

    if (conf.startup_dir_mode == 1)
    {
	if (conf.startup_dir != NULL)
	{
	    g_free (conf.startup_dir);
	    conf.startup_dir = g_strdup (browser->current_path->str);
	}
    }
    if (browser->current_path)
	g_string_free (browser->current_path, TRUE);

    if (browser->last_path)
	g_string_free (browser->last_path, TRUE);

#ifdef ENABLE_MOVIE
    videoplay_destroy ();
#endif

    imageview_destroy ();
    thumbview_destroy ();
    dirview_destroy ();

    g_free (browser);

    gtk_main_quit ();
}

void
browser_select_dir (gchar * path)
{
    gchar  *title;
    char    buf[PATH_MAX];

    if (!strcmp (browser->last_path->str, path))
	return;

#ifdef ENABLE_MOVIE
    videoplay_clear ();
#endif
    comment_view_clear (commentview);
    thumbview_stop ();
    imageview_stop ();

    viewtype_set (VIEWTYPE_IMAGEVIEW);

    g_string_assign (browser->last_path, path);
    g_string_assign (browser->current_path, path);

    thumbview_clear ();

    file_list_create (browser->filelist, path);

    title = g_strconcat ("PornView - ", g_basename (path), NULL);
    gtk_window_set_title (GTK_WINDOW (browser->window), title);
    g_free (title);

    snprintf (buf, PATH_MAX, "  %d", browser->filelist->num);
    gtk_statusbar_pop (GTK_STATUSBAR (BROWSER_STATUS_DIR), 1);
    gtk_statusbar_push (GTK_STATUSBAR (BROWSER_STATUS_DIR), 1, buf);

    thumbview_add (browser->filelist);
}
