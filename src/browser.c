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

#include "support/file_utils.h"

Browser *browser = NULL;

#define browser_about  about_dialog

static const gchar *browser_ui_xml =
"<ui>"
	"<menubar name=\"MainMenu\">"
		"<menu name=\"FileMenu\" action=\"FileMenuAction\">"
			"<menuitem name=\"Preferences\" action=\"PrefsAction\" />"
			"<menuitem name=\"Quit\" action=\"QuitAction\" />"
			"<placeholder name=\"FileMenuAdditions\" />"
		"</menu>"
		"<menu name=\"ToolsMenu\""
			"action=\"ToolsMenuAction\">"
			"<menuitem name=\"DeleteOldThumb\""
					"action=\"DeleteOldThumbAction\"/>"
			"<menuitem name=\"DeleteAllThumb\""
					"action=\"DeleteAllThumbAction\"/>"
			"<menuitem name=\"DeleteOldComments\""
					"action=\"DeleteOldCommentsAction\"/>"
			"<menuitem name=\"DeleteAllComments\""
					"action=\"DeleteAllCommentsAction\"/>"
	    "</menu>"
		"<menu name=\"HelpMenu\""
				"action=\"HelpMenuAction\">"
			"<menuitem name=\"About\" action=\"AboutAction\" />"
		"</menu>"
	"</menubar>"
"</ui>";

static GtkActionEntry browser_ui_entries[] = {
	/*
	 * menu
	 */

	/* file menu */
	{ "FileMenuAction", NULL, N_("_File"),	/* name, stock id, label */
		NULL, NULL,							/* accelerator, tooltip */
		NULL },					 			/* callback */
	{ "PrefsAction", GTK_STOCK_PROPERTIES, N_("_Preferences"),
		"<control>P", "Preferences",
		G_CALLBACK(browser_prefs) },
	{ "QuitAction", GTK_STOCK_QUIT, N_("_Quit"),
		"<control>Q", "Quit",
		G_CALLBACK(browser_destroy) },
	/* help menu */
	{ "HelpMenuAction", GTK_STOCK_HELP, N_("_Help"),
		NULL, NULL,
		NULL },
	{ "AboutAction", GTK_STOCK_ABOUT, N_("_About"),
		"<control>B", "About",
		G_CALLBACK(browser_about) },
	/* tools menu */
	{ "ToolsMenuAction", NULL, N_("_Tools"),
		NULL, NULL,
		NULL },
	{ "DeleteOldThumbAction", NULL, N_("_Delete Old Thumbnails"),
		NULL, NULL,
		G_CALLBACK(browser_remove_old_thumbs) },
	{ "DeleteAllThumbAction", NULL, N_("Delete All Thumbnails"),
		NULL, NULL,
		G_CALLBACK(browser_clear_cache) },
	{ "DeleteOldCommentsAction", NULL, N_("Delete Old Comments"),
		NULL, NULL,
		G_CALLBACK(browser_remove_old_comments) },
	{ "DeleteAllCommentsAction", NULL, N_("Delete All Comments"),
		NULL, NULL,
		G_CALLBACK(browser_clear_comments) }

};

static guint n_browser_ui_entries = G_N_ELEMENTS(browser_ui_entries);

static gint timer_id;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
browser_remove_old_thumbs(void)
{
    cache_maintain_home(CACHE_THUMBS, FALSE);
}

static void
browser_clear_cache(void)
{
    cache_maintain_home(CACHE_THUMBS, TRUE);
	thumbview_refresh();
}

static void
browser_remove_old_comments(void)
{
    cache_maintain_home(CACHE_COMMENTS, FALSE);
}

static void
browser_clear_comments(void)
{
    cache_maintain_home(CACHE_COMMENTS, TRUE);
}

static void
browser_prefs(void)
{
    prefs_win_open_idle("/General");
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */
void
browser_create(const gchar *start_path)
{
    GtkWidget *vbox;
    GdkPixbuf *icon_pix;
    GtkWidget *menu;
    GtkWidget *hpaned;
    GtkWidget *vpaned;
    GtkWidget *hbox;
    GtkWidget *label;
	GError *error;
	guint ui_id;

	GtkActionGroup *action_group;

    browser = g_new(Browser, 1);

	/* check path argument, if any */
	if (isdir(start_path)) {
		browser->current_path = g_string_new(start_path);
	} else {
		if (start_path)
			fprintf(stderr, "\"%s\" is not a valid dir!\n", start_path);
	    browser->current_path = g_string_new(conf.startup_dir);
	}

    browser->last_path = g_string_new("");
    browser->filelist = (FileList *)file_list_init();

    browser->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (browser->window), "PornView");
    gtk_widget_realize(browser->window);

    icon_pix = gdk_pixbuf_new_from_xpm_data(pornview_xpm);

    gtk_window_set_icon(GTK_WINDOW(browser->window), icon_pix);

    g_signal_connect(G_OBJECT(browser->window), "delete_event",
			G_CALLBACK(browser_destroy), NULL);

    gtk_window_set_default_size(GTK_WINDOW(browser->window),
				 conf.window_width, conf.window_height);

	/* TODO: use gtk_box_new for gtk+-3 */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(browser->window), vbox);


    /*
     * main menu
     */
	browser->menu_toolbar = gtk_ui_manager_new();

	action_group = gtk_action_group_new("BrowserActions");
	gtk_action_group_add_actions(action_group, browser_ui_entries, n_browser_ui_entries, NULL);
	gtk_ui_manager_insert_action_group(browser->menu_toolbar, action_group, 0);

	error = NULL;
	ui_id = gtk_ui_manager_add_ui_from_string(browser->menu_toolbar,
			browser_ui_xml,
			-1,
			&error);
	if (error) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
	}


	/*
	 * menubar/toolbar
	 */
	/* TODO: use gtk_ui_manager_remove_ui() later */
	menu = gtk_ui_manager_get_widget(browser->menu_toolbar, "/MainMenu");

	gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);

	gtk_window_add_accel_group(GTK_WINDOW(browser->window),
			gtk_ui_manager_get_accel_group(browser->menu_toolbar));

	gtk_widget_show (menu);


	/*
	 * prepare boxes for dirtree, thumbview etc
	 */
	hpaned = gedo_hpaned_new ();
	gtk_container_add (GTK_CONTAINER (vbox), hpaned);
	gtk_widget_show (hpaned);

	vpaned = gedo_vpaned_new ();
	gedo_paned_add1 (GEDO_PANED (hpaned), vpaned);
	gtk_widget_show (vpaned);


	/*
	 * dirtree
	 */
	dirview_create (browser->current_path->str, browser->window);
	gedo_paned_add1 (GEDO_PANED (vpaned), DIRVIEW_CONTAINER);
	commentview = comment_view_create ();
	browser->notebook = commentview->notebook;

	gtk_widget_set_size_request(dirview->scroll_win,
			conf.dirtree_width, conf.dirtree_height);


    /*
     * videoplay
     */
/* #ifdef ENABLE_MOVIE */
    /* videoplay_create (); */
    /* label = gtk_label_new (_(" Preview ")); */
    /* gtk_notebook_prepend_page (GTK_NOTEBOOK (browser->notebook), */
				   /* VIDEOPLAY_CONTAINER, label); */

    /* gtk_notebook_set_current_page(GTK_NOTEBOOK (browser->notebook), 0); */

/* #endif */


    /*
     * imageview
     */
	imageview_create ();
	label = gtk_label_new (_(" Preview "));
	gtk_notebook_prepend_page (GTK_NOTEBOOK (browser->notebook),
				   IMAGEVIEW_CONTAINER, label);

#ifndef ENABLE_MOVIE
	gtk_notebook_set_current_page(GTK_NOTEBOOK (browser->notebook), 0);
#endif

	gedo_paned_add2 (GEDO_PANED (hpaned), commentview->main_vbox);


    /*
     * thumbview
     */
	thumbview_create (browser->window);
	gedo_paned_add2 (GEDO_PANED (vpaned), THUMBVIEW_CONTAINER);

	/* this sets focus on the _dirview_ */
	gtk_widget_set_can_focus(THUMBVIEW_CONTAINER, TRUE);
	gtk_widget_grab_focus(THUMBVIEW_CONTAINER);


    /*
     * statusbar
     */
	/* TODO: use gtk_box_new for gtk+-3 */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (hbox, "StatusBarContainer");
	gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	browser->status_dir = gtk_statusbar_new ();
	gtk_widget_set_name (browser->status_dir, "StatusBar1");
	gtk_container_set_border_width (GTK_CONTAINER (browser->status_dir), 1);
	gtk_widget_set_size_request (browser->status_dir, 80, 15);
	gtk_box_pack_start (GTK_BOX (hbox), browser->status_dir, FALSE, FALSE, 0);
	gtk_widget_show (browser->status_dir);

	browser->status_name = gtk_statusbar_new ();
	gtk_widget_set_name (browser->status_name, "StatusBar2");
	gtk_container_set_border_width (GTK_CONTAINER (browser->status_name), 1);
	gtk_widget_set_size_request (browser->status_name, 150, -1);
	gtk_box_pack_start (GTK_BOX (hbox), browser->status_name, TRUE, TRUE, 0);
	gtk_widget_show (browser->status_name);

	browser->status_image = gtk_statusbar_new ();
	gtk_widget_set_name (browser->status_image, "StatusBar3");
	gtk_container_set_border_width (GTK_CONTAINER (browser->status_image), 1);
	gtk_widget_set_size_request (browser->status_image, 150, -1);
	gtk_box_pack_start (GTK_BOX (hbox), browser->status_image, TRUE, TRUE, 0);
	gtk_widget_show (browser->status_image);

	browser->progress = gtk_progress_bar_new ();
	gtk_widget_set_name (browser->progress, "ProgressBar");
	gtk_box_pack_end (GTK_BOX (hbox), browser->progress, FALSE, FALSE, 0);
	gtk_widget_show (browser->progress);

	gtk_widget_show (browser->window);

/* #ifdef ENABLE_MOVIE */
    /* gtk_widget_hide (gtk_notebook_get_nth_page */
			 /* (GTK_NOTEBOOK (browser->notebook), 1)); */
    /* gtk_notebook_set_current_page(GTK_NOTEBOOK (browser->notebook), 0); */
/* #endif */

	dirview_scroll_center ();

	browser_select_dir(browser->current_path->str);
}

void
browser_destroy(void)
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

/* FIXME: ignores uppercase .JPEG etc */
void
browser_select_dir(gchar *path)
{
    gchar *title,
		  *old_path = g_get_current_dir();
    char buf[PATH_MAX];

	/* gdk_window_freeze_updates(GDK_WINDOW(browser->window)); */

    if (!path || !strcmp(browser->last_path->str, path))
		return;

#ifdef ENABLE_MOVIE
    videoplay_clear();
#endif
	comment_view_clear(commentview);
	thumbview_stop();
	imageview_stop ();

    viewtype_set(VIEWTYPE_IMAGEVIEW);

    g_string_assign(browser->last_path, path);
    g_string_assign(browser->current_path, path);

	thumbview_clear();

	/* file_list_create relies on relative paths,
	 * chdir even if it is redundant */
	if (chdir(path)) /* chdir failed */
		return;
    file_list_create(browser->filelist, path);
	chdir(old_path);
	g_free(old_path);

    title = g_strconcat("PornView - ", g_path_get_basename(path), NULL);
    gtk_window_set_title(GTK_WINDOW(browser->window), title);
    g_free(title);

    snprintf(buf, PATH_MAX, "  %d", browser->filelist->num);
    gtk_statusbar_pop (GTK_STATUSBAR(BROWSER_STATUS_DIR), 1);
    gtk_statusbar_push (GTK_STATUSBAR(BROWSER_STATUS_DIR), 1, buf);

	thumbview_add(browser->filelist);

	/* gtk_widget_thaw_child_notify(GTK_WIDGET(dirview->dirtree)); */
}
