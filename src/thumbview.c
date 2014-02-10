/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                        (c) 2002,2003                         (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#ifdef ENABLE_XINE
#include "gtkxine.h"
#endif

#ifdef ENABLE_XINE_OLD
#include "gtkxine_old.h"
#endif

#include "dirtree.h"
#include "zalbum.h"

#include "charset.h"
#include "dialogs.h"
#include "file_type.h"
#include "file_utils.h"
#include "image.h"
#include "menu.h"
#include "pixbuf_utils.h"
#include "string_utils.h"

#ifdef ENABLE_EXIF
#include "exif-data.h"
#include "jpeg-data.h"
#include "exif_view.h"
#endif

#include "thumbview.h"
#include "cache.h"
#include "browser.h"
#include "dirview.h"
#include "fileutil.h"
#include "imageview.h"
#include "imageinfo.h"
#include "videoplay.h"
#include "prefs.h"
#include "viewtype.h"

#include "pixmaps/refresh.xpm"
#include "pixmaps/left.xpm"
#include "pixmaps/right.xpm"
#include "pixmaps/first.xpm"
#include "pixmaps/last.xpm"
#include "pixmaps/fullscreen.xpm"
#include "pixmaps/info.xpm"
#include "pixmaps/slideshow.xpm"
#include "pixmaps/no_zoom.xpm"
#include "pixmaps/zoom_fit.xpm"
#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"
#include "pixmaps/view_list.xpm"
#include "pixmaps/view_thumbs.xpm"

ThumbView *thumbview = NULL;

static void cb_thumbview_fullscreen (void);
#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
static void cb_thumbview_create_thumb (void);
#endif
static void cb_thumbview_exif_view (void);

static void cb_thumbview_select_all (void);
static void cb_thumbview_copy (void);
static void cb_thumbview_move (void);
static void cb_thumbview_rename (void);
static void cb_thumbview_delete (void);

static GtkUIManager *toolbar_ui;

static const gchar *thumbview_ui_xml =
"<ui>"
	"<toolbar name=\"ThumbviewToolbar\" action=\"ThumbviewToolBarAction\">"
		"<placeholder name=\"ToolItems\">"
			"<separator/>"
			"<toolitem name=\"Refresh\" action=\"RefreshAction\"/>"
			"<toolitem name=\"Info\" action=\"InfoAction\"/>"
			"<toolitem name=\"Thumbs\" action=\"ThumbsAction\"/>"
			"<toolitem name=\"List\" action=\"ListAction\"/>"
			"<toolitem name=\"FullScreen\" action=\"FullScreenAction\"/>"
			"<toolitem name=\"Slideshow\" action=\"SlideshowAction\"/>"
			"<toolitem name=\"NoZoom\" action=\"NoZoomAction\"/>"
			"<toolitem name=\"AutoZoom\" action=\"AutoZoomAction\"/>"
			"<toolitem name=\"ZoomIn\" action=\"ZoomInAction\"/>"
			"<toolitem name=\"ZoomOut\" action=\"ZoomOutAction\"/>"
			"<separator/>"
		"</placeholder>"
	"</toolbar>"
"</ui>";

/* TODO: all icons */
static GtkActionEntry thumbview_ui_entries[] = {
	{ "ThumbviewToolBarAction", NULL, NULL,		/* name, stock id, label */
		NULL, NULL,								/* accelerator, tooltip */
		NULL },									/* callback */
	{ "RefreshAction", "view-refresh", N_("_Refresh"),
		NULL, "Refresh",
		G_CALLBACK(cb_thumbview_refresh) },
	{ "InfoAction", GTK_STOCK_INFO, N_("_Properties"),
		NULL, "Properties",
		G_CALLBACK(cb_thumbview_info) },
	{ "ThumbsAction", GTK_STOCK_DND_MULTIPLE, N_("_View Thumbs"),
		NULL, "View Thumbs",
		G_CALLBACK(cb_thumbview_toggle_mode) },
	{ "ListAction", "gtk-justify-fill", N_("_View List"),
		NULL, "View List",
		G_CALLBACK(cb_thumbview_toggle_mode) },
	{ "FullScreenAction", GTK_STOCK_FULLSCREEN, N_("_Fullscreen"),
		NULL, "Fullscreen",
		G_CALLBACK(cb_thumbview_fullscreen_view) },
	{ "SlideshowAction", GTK_STOCK_DND_MULTIPLE, N_("_Slideshow"),
		NULL, "Slideshow",
		G_CALLBACK(cb_thumbview_slideshow_view) },
	{ "NoZoomAction", GTK_STOCK_NO, N_("_No Zoom"),
		NULL, "No Zoom",
		G_CALLBACK(cb_thumbview_no_zoom) },
	{ "AutoZoomAction", GTK_STOCK_YES, N_("_Auto Zoom"),
		NULL, "Auto Zoom",
		G_CALLBACK(cb_thumbview_zoom_auto) },
	{ "ZoomInAction", GTK_STOCK_ZOOM_IN, N_("_Zoom In"),
		NULL, "Zoom In",
		G_CALLBACK(cb_thumbview_zoom_in) },
	{ "ZoomOutAction", GTK_STOCK_ZOOM_OUT, N_("_Zoom Out"),
		NULL, "Zoom Out",
		G_CALLBACK(cb_thumbview_zoom_out) },
};

static guint n_thumbview_ui_entries = G_N_ELEMENTS(thumbview_ui_entries);


/*
 *-------------------------------------------------------------------
 * callback functions
 *-------------------------------------------------------------------
 */

static void thumbview_update_popupmenu (gint file_type,
					GtkItemFactory * ifactory);
static GtkWidget *create_progs_submenu (ThumbView * tv);
static GtkWidget *create_scripts_submenu (ThumbView * tv);

static  gint
cb_thumbview_button_press (GtkWidget * widget, GdkEventButton * event,
			   ThumbView * tv)
{

}

static  gint
cb_thumbview_button_release (GtkWidget * widget,
			     GdkEventButton * event, ThumbView * tv)
{
    return FALSE;
}

static  gint
cb_thumbview_key_press (GtkWidget * widget, GdkEventKey * event,
			ThumbView * tv)
{
    g_return_val_if_fail (widget && event, FALSE);

    switch (event->keyval)
    {
      case GDK_Return:
	  cb_thumbview_fullscreen ();
	  return TRUE;
    }

    return FALSE;
}

static void thumbview_toolbar_update (ThumbView * tv);

#ifdef ENABLE_MOVIE
static gint cb_load_image_timeout;
static gint cb_load_movie_timeout;

static  gboolean
cb_load_image (gpointer data)
{
    gchar  *path = data;

    imageview_set_image (path);

    return FALSE;
}

static  gboolean
cb_load_movie (gpointer data)
{
    gchar  *path = data;

    videoplay_play_file (path);

    return FALSE;
}
#endif

static  gint
cb_thumbview_cell_select (GtkWidget * widget, ZAlbumCell * cell,
			  ThumbView * tv)
{
    gint    type;

    if (ZLIST (tv->album)->focus == -1)
	return FALSE;

    if (g_list_length (ZLIST (tv->album)->selection) > 0)
	return FALSE;

    type =
	(gint) zalbum_get_cell_data (ZALBUM (tv->album),
				     ZLIST (tv->album)->focus);

    if (type == FILETYPE_IMAGE)
    {
#ifdef ENABLE_MOVIE
	if (viewtype_get () != VIEWTYPE_IMAGEVIEW
	    && viewtype_get () != VIEWTYPE_VIDEOPLAY && IMAGEVIEW_IS_HIDE)
	{
	    videoplay_clear ();

	    viewtype_set (VIEWTYPE_IMAGEVIEW);
	    thumbview_toolbar_update (tv);
	    tv->current = ZLIST (tv->album)->focus;

	    if (cb_load_image_timeout)
		g_source_remove(cb_load_image_timeout);

	    cb_load_image_timeout =
		g_timeout_add(250, cb_load_image, cell->name);

	    return FALSE;
	}
	else
	{
	    videoplay_clear ();
	    viewtype_set (VIEWTYPE_IMAGEVIEW);
#endif
	    thumbview_toolbar_update (tv);
	    tv->current = ZLIST (tv->album)->focus;

	    imageview_set_image (cell->name);

#ifdef ENABLE_MOVIE
	    return FALSE;
	}
    }
    else
    {
	imageview_stop ();

	viewtype_set (VIEWTYPE_VIDEOPLAY);
	thumbview_toolbar_update (tv);
	tv->current = ZLIST (tv->album)->focus;

	if (cb_load_movie_timeout)
	    g_source_remove(cb_load_movie_timeout);

	cb_load_movie_timeout =
	    g_timeout_add(250, cb_load_movie, cell->name);

	return FALSE;
#endif
    }

    return FALSE;
}

static  gint
cb_thumbview_cell_unselect (GtkWidget * widget, ZAlbumCell * cell,
			    ThumbView * tv)
{
    return FALSE;
}

    /*
     * toolbar callback functions
     */

static void
cb_thumbview_refresh (GtkWidget * widget, ThumbView * tv)
{
    gchar  *path;

    path = g_strdup (browser->current_path->str);
    browser->last_path = g_string_assign (browser->last_path, " ");

    browser_select_dir (path);

    g_free (path);
}

static void
cb_thumbview_previous (GtkWidget * widget, ThumbView * tv)
{
    gint    index;

    index = tv->current - 1;

    if (index < 0)
	return;

    zlist_cell_focus (ZLIST (tv->album), index);
}

static void
cb_thumbview_next (GtkWidget * widget, ThumbView * tv)
{
    gint    index;

    index = tv->current + 1;

    if (index > (ZALBUM (tv->album)->len - 1))
	return;

    zlist_cell_focus (ZLIST (tv->album), index);
}

static void
cb_thumbview_first (GtkWidget * widget, ThumbView * tv)
{
    zlist_cell_focus (ZLIST (tv->album), 0);
}

static void
cb_thumbview_last (GtkWidget * widget, ThumbView * tv)
{
    zlist_cell_focus (ZLIST (tv->album), ZALBUM (tv->album)->len - 1);
}

static void
cb_thumbview_info (GtkWidget * widget, ThumbView * tv)
{
    imageinfo_dialog ();
}

static void thumbview_thumbs_interrupt (void);
static gint thumbview_thumbs_next (void);

static void
cb_thumbview_toggle_mode (GtkWidget * widget, ThumbView * tv)
{
    if (ZALBUM (tv->album)->mode == ZALBUM_MODE_LIST)
    {
	zalbum_freeze (thumbview->album);
	zalbum_set_mode (ZALBUM (tv->album), ZALBUM_MODE_PREVIEW);
	zalbum_thawn (thumbview->album);

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/List"), TRUE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Thumbs"), FALSE);

	if (ZALBUM (tv->album)->len != 0)
	{
	    thumbview_thumbs_interrupt ();

	    thumbview->thumbs_running = TRUE;
	    while (thumbview_thumbs_next ());
	}
    }
    else
    {
	thumbview_thumbs_interrupt ();

	thumbview_clear ();
	zalbum_set_mode (ZALBUM (thumbview->album), ZALBUM_MODE_LIST);
	thumbview_add (browser->filelist);

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/List"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Thumbs"), TRUE);
    }
}

static void
cb_thumbview_fullscreen_view (GtkWidget * widget, ThumbView * tv)
{
    gint    type;

    type = (gint) zalbum_get_cell_data (ZALBUM (tv->album), tv->current);

    if (type == FILETYPE_IMAGE)
	imageview_set_fullscreen ();
#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
    else
	videoplay_set_fullscreen ();
#endif
}

static void
cb_thumbview_fullscreen (void)
{
    cb_thumbview_fullscreen_view (NULL, thumbview);
}

static void
cb_thumbview_slideshow_view (GtkWidget * widget, ThumbView * tv)
{
    imageview_start_slideshow ();
}

static void
cb_thumbview_no_zoom (GtkWidget * widget, ThumbView * tv)
{
    imageview_no_zoom ();
}

static void
cb_thumbview_zoom_auto (GtkWidget * widget, ThumbView * tv)
{
    imageview_zoom_auto ();
}

static void
cb_thumbview_zoom_in (GtkWidget * widget, ThumbView * tv)
{
    imageview_zoom_in ();
}

static void
cb_thumbview_zoom_out (GtkWidget * widget, ThumbView * tv)
{
    imageview_zoom_out ();
}

/* file operations functions */

static void
cb_thumbview_select_all (void)
{
    if (ZALBUM (thumbview->album)->len != 0)
	zlist_select_all (ZLIST (thumbview->album));
}

static GList *selection_list_create (ThumbView * tv);

static void
cb_thumbview_copy (void)
{
    if (g_list_length (ZLIST (thumbview->album)->selection) > 1)

	file_util_copy (NULL, selection_list_create (thumbview),
			BROWSER_CURRENT_PATH);
    else
    {
	ZAlbumCell *cell;

	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   (guint) ZLIST (thumbview->album)->
				   selection->data);
	file_util_copy (cell->name, NULL, BROWSER_CURRENT_PATH);
    }
}

static void
cb_thumbview_move (void)
{
    if (g_list_length (ZLIST (thumbview->album)->selection) > 1)

	file_util_move (NULL, selection_list_create (thumbview),
			BROWSER_CURRENT_PATH);
    else
    {
	ZAlbumCell *cell;

	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   (guint) ZLIST (thumbview->album)->
				   selection->data);

	file_util_move (cell->name, NULL, BROWSER_CURRENT_PATH);
    }
}

static void
cb_thumbview_rename (void)
{
    if (g_list_length (ZLIST (thumbview->album)->selection) > 1)

	file_util_rename (NULL, selection_list_create (thumbview));
    else
    {
	ZAlbumCell *cell;

	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   (guint) ZLIST (thumbview->album)->
				   selection->data);

	file_util_rename (cell->name, NULL);
    }
}

static void
cb_thumbview_delete (void)
{
    if (g_list_length (ZLIST (thumbview->album)->selection) > 1)

	file_util_delete (NULL, selection_list_create (thumbview));
    else
    {
	ZAlbumCell *cell;

	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   (guint) ZLIST (thumbview->album)->
				   selection->data);

	file_util_delete (cell->name, NULL);
    }
}

#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
static void
cb_thumbview_create_thumb (void)
{
    videoplay_create_thumb ();
}
#endif

#ifdef ENABLE_EXIF
static void
cb_thumbview_exif_view (void)
{
    ZAlbumCell *cell;
    ExifView *ev;

    cell =
	ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album), thumbview->current);

    ev = exif_view_create_window (cell->name);
}
#endif

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static GtkWidget *
thumbview_create_toolbar (ThumbView * tv)
{
    GtkWidget *toolbar;
    GtkWidget *iconw;
	GtkActionGroup *action_group;
	guint ui_id;

    g_return_val_if_fail (tv, NULL);

    toolbar_ui = gtk_ui_manager_new();
	action_group = gtk_action_group_new("ThumbviewAction");

	gtk_action_group_add_actions(action_group, thumbview_ui_entries,
			n_thumbview_ui_entries, tv);
	gtk_ui_manager_insert_action_group(toolbar_ui, action_group, 0);

	GError *error;
	error = NULL;
	ui_id = gtk_ui_manager_add_ui_from_string(toolbar_ui,
			thumbview_ui_xml,
			-1,
			&error);
	if (error) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
	}

    toolbar = gtk_ui_manager_get_widget(toolbar_ui, "/ThumbviewToolbar");

	/* TODO: register accels */
	/* gtk_box_pack_start(GTK_BOX(dirview->container), toolbar, FALSE, FALSE, 0); */

	/* gtk_window_add_accel_group(GTK_WINDOW(tv->scroll_win), */
			/* gtk_ui_manager_get_accel_group(toolbar_ui)); */


    /* gtk_widget_show_all (toolbar); */

    return toolbar;
}

static void
thumbview_toolbar_update (ThumbView * tv)
{
    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Refresh"), TRUE);

    if (ZALBUM (tv->album)->mode == ZALBUM_MODE_LIST)
    {
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/List"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Thumbs"), TRUE);
    }
    else
    {
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/List"), TRUE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Thumbs"), FALSE);
    }

    if (ZALBUM (tv->album)->len == 0 || ZLIST (tv->album)->focus < 0)
    {
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Info"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/FullScreen"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Slideshow"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/NoZoom"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/AutoZoom"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/ZoomIn"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/ZoomOut"), FALSE);
		return;
    }
    else
    {
	gint    type;

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Info"), TRUE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/FullScreen"), TRUE);

	type =
	    (gint) zalbum_get_cell_data (ZALBUM (tv->album),
					 ZLIST (tv->album)->focus);

	if (type == FILETYPE_IMAGE)
	{

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Slideshow"), TRUE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/NoZoom"),
				(IMAGEVIEW_ZOOM_TYPE == IMAGEVIEW_ZOOM_100) ? FALSE : TRUE);

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/AutoZoom"),
				(IMAGEVIEW_ZOOM_TYPE == IMAGEVIEW_ZOOM_AUTO) ? FALSE : TRUE);

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/ZoomIn"), TRUE);

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/ZoomOut"), TRUE);
	}
	else
	{

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Slideshow"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Slideshow"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/AutoZoom"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/ZoomIn"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/ZoomOut"), FALSE);

	}
    }
}

static  gint
valcomp (gpointer a, gpointer b)
{
    return strcmp ((const char *) a, (const char *) b);
}

static GList *
selection_list_create (ThumbView * tv)
{
    GList  *list = NULL, *node = NULL;
    GList  *ret = NULL;
    ZAlbumCell *cell;
    gint    c = 0;

    list = g_list_copy (ZLIST (tv->album)->selection);

    node = list;

    for (c = 0; c < g_list_length (list); c++)
    {
	cell = ZLIST_CELL_FROM_INDEX (ZLIST (tv->album), (guint) node->data);

	ret = g_list_prepend (ret, g_strdup (cell->name));

	node = g_list_next (node);
    }

    ret = g_list_reverse (ret);

    g_list_free (list);

    ret = g_list_sort (ret, (GCompareFunc) valcomp);

    return ret;
}

#ifdef ENABLE_MOVIE
static void thumbview_update_popupmenu_movie (GtkItemFactory * ifactory);
#endif
static void thumbview_update_popupmenu_zoom (GtkItemFactory * ifactory);

static void
thumbview_update_popupmenu (gint file_type, GtkItemFactory * ifactory)
{
    if (file_type == FILETYPE_VIDEO)
    {

#ifdef ENABLE_MOVIE
	gtk_widget_set_sensitive (gtk_item_factory_get_item
				  (ifactory, "/Movie"), TRUE);

	thumbview_update_popupmenu_movie (ifactory);
#endif

#ifdef ENABLE_EXIF
	gtk_widget_set_sensitive (gtk_item_factory_get_item
				  (ifactory, "/Scan EXIF Data"), FALSE);
#endif

	gtk_widget_set_sensitive (gtk_item_factory_get_item
				  (ifactory, "/Zoom"), FALSE);
	gtk_widget_set_sensitive (gtk_item_factory_get_item
				  (ifactory, "/Rotate"), FALSE);
    }
    else
    {
	ZAlbumCell *cell;

	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   thumbview->current);

#ifdef ENABLE_MOVIE
	gtk_widget_set_sensitive (gtk_item_factory_get_item
				  (ifactory, "/Movie"), FALSE);
#endif

#ifdef ENABLE_EXIF
	if (file_type_is_jpeg (cell->name))
	    gtk_widget_set_sensitive (gtk_item_factory_get_item
				      (ifactory, "/Scan EXIF Data"), TRUE);
	else
	    gtk_widget_set_sensitive (gtk_item_factory_get_item
				      (ifactory, "/Scan EXIF Data"), FALSE);
#endif

	gtk_widget_set_sensitive (gtk_item_factory_get_item
				  (ifactory, "/Rotate"), TRUE);

	thumbview_update_popupmenu_zoom (ifactory);
    }
}

static void
thumbview_update_popupmenu_zoom (GtkItemFactory * ifactory)
{
    gtk_widget_set_sensitive (gtk_item_factory_get_item
			      (ifactory, "/Zoom/Zoom In"), TRUE);
    gtk_widget_set_sensitive (gtk_item_factory_get_item
			      (ifactory, "/Zoom/Zoom Out"), TRUE);

    gtk_widget_set_sensitive (gtk_item_factory_get_item
			      (ifactory, "/Zoom/Fit to Window"),
			      (imageview->zoom !=
			       IMAGEVIEW_ZOOM_FIT) ? TRUE : FALSE);

    gtk_widget_set_sensitive (gtk_item_factory_get_item
			      (ifactory, "/Zoom/Original Size"),
			      (imageview->zoom !=
			       IMAGEVIEW_ZOOM_100) ? TRUE : FALSE);

    gtk_widget_set_sensitive (gtk_item_factory_get_item
			      (ifactory, "/Zoom/Auto Zoom"),
			      (imageview->zoom !=
			       IMAGEVIEW_ZOOM_AUTO) ? TRUE : FALSE);
}

#ifdef ENABLE_MOVIE
static void
thumbview_update_popupmenu_movie (GtkItemFactory * ifactory)
{
    switch (VIDEOPLAY_STATUS)
    {
      case VIDEOPLAY_STATUS_NULL:
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Fullscreen"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Play"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Pause"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Stop"), FALSE);
#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Fast Forward"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Slow Forward"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Create Thumbnail"),
				    FALSE);
#endif
	  break;

      case VIDEOPLAY_STATUS_PLAYING:
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Fullscreen"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Play"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Pause"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Stop"), TRUE);
#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Fast Forward"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Slow Forward"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Create Thumbnail"),
				    TRUE);
#endif
	  break;

      case VIDEOPLAY_STATUS_PAUSE:
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Fullscreen"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Play"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Pause"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Stop"), TRUE);
#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Fast Forward"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Slow Forward"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Create Thumbnail"),
				    TRUE);
#endif
	  break;

      case VIDEOPLAY_STATUS_STOP:
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Fullscreen"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Play"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Pause"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Stop"), FALSE);
#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Fast Forward"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Slow Forward"), FALSE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Create Thumbnail"),
				    FALSE);
#endif
	  break;

      case VIDEOPLAY_STATUS_FORWARD:
      case VIDEOPLAY_STATUS_SLOW:
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Fullscreen"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Play"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Pause"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Stop"), TRUE);
#if (defined ENABLE_XINE) || (defined ENABLE_XINE_OLD)
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Fast Forward"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Slow Forward"), TRUE);
	  gtk_widget_set_sensitive (gtk_item_factory_get_item
				    (ifactory, "/Movie/Create Thumbnail"),
				    TRUE);
#endif
	  break;
    }
}
#endif

/*
 *-------------------------------------------------------------------
 * thumbloader functions
 *-------------------------------------------------------------------
 */

static void
thumbview_thumbs_progressbar (gfloat value)
{
    if (value > 1.0)
	value = 1.0;
    else if (value < 0)
	value = 0;

    gtk_progress_bar_update (GTK_PROGRESS_BAR (browser->progress), value);
}

static void
thumbview_thumbs_do (ThumbLoader * tl)
{
    GdkPixmap *pixmap = NULL;
    GdkBitmap *mask = NULL;
    gfloat  status;

    if (tl == NULL)
	return;

    if (thumbview->thumbs_stop)
	return;

    thumb_loader_to_pixmap (tl, &pixmap, &mask);
    zalbum_set_pixmap (ZALBUM (thumbview->album),
		       thumbview->thumbs_count, pixmap, mask);
    if (pixmap)
	g_object_unref(pixmap);

    if (mask)
	g_object_unref(mask);

    status =
	(gfloat) thumbview->thumbs_count / ZALBUM (thumbview->album)->len;

    thumbview_thumbs_progressbar (status);
}

static void
cb_thumbview_thumbs_error (ThumbLoader * tl, gpointer data)
{
    if (thumbview->thumbs_loader == tl)
    {
	thumbview_thumbs_do (NULL);
    }

    while (thumbview_thumbs_next ());
}

static void
cb_thumbview_thumbs_done (ThumbLoader * tl, gpointer data)
{
    if (thumbview->thumbs_loader == tl)
    {
	thumbview_thumbs_do (tl);
    }

    while (thumbview_thumbs_next ());
}

static void
thumbview_thumbs_cleanup (void)
{
    thumbview_thumbs_progressbar (0.0);
    thumbview->thumbs_count = -1;
    thumbview->thumbs_running = FALSE;
    thumbview->thumbs_stop = FALSE;
    thumb_loader_free (thumbview->thumbs_loader);
    thumbview->thumbs_loader = NULL;

	DIRTREE(dirview->dirtree)->check_events = TRUE;
}

static void
thumbview_thumbs_interrupt (void)
{
    if (thumbview->thumbs_running)
	thumbview_thumbs_cleanup ();
}

static  gint
thumbview_thumbs_next (void)
{
    ZAlbumCell *cell;

    thumbview->thumbs_count++;

    if (thumbview->thumbs_stop)
    {
	thumbview_thumbs_cleanup ();
	return FALSE;
    }

    if (thumbview->thumbs_count < ZALBUM (thumbview->album)->len)
    {
	gchar  *path;
	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   thumbview->thumbs_count);
	path = g_strdup (cell->name);

	if (file_type_is_movie (cell->name))
	{
	    GdkPixbuf *pixbuf = NULL;
	    GdkPixmap *pixmap;
	    GdkBitmap *mask;
	    gchar  *cache_dir;
	    mode_t  mode = 0755;
	    gfloat  status;
	    cache_dir =
		cache_get_location (CACHE_THUMBS, path, FALSE, NULL, &mode);


	    if (cache_ensure_dir_exists (cache_dir, mode))
	    {
		gchar  *cache_path;
		gchar *basename = g_path_get_basename(cell->name);
		cache_path =
		    g_strconcat (cache_dir, "/", basename,
				 PORNVIEW_CACHE_THUMB_EXT, NULL);
		g_free(basename);

		pixbuf = gdk_pixbuf_new_from_file (cache_path, NULL);

		if (pixbuf)
		{
		    gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap,
						       &mask, 128);
		    zalbum_set_pixmap (ZALBUM (thumbview->album),
				       thumbview->thumbs_count, pixmap, mask);
		    if (pixmap)
			g_object_unref(pixmap);
		    if (mask)
			g_object_unref(mask);
		    g_object_unref(pixbuf);
		}
		else
		{
		    pixbuf = image_get_video_pixbuf ();

		    gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap,
						       &mask, 128);
		    zalbum_set_pixmap (ZALBUM (thumbview->album),
				       thumbview->thumbs_count, pixmap, mask);
		    if (pixmap)
			g_object_unref(pixmap);
		    if (mask)
			g_object_unref(mask);
		    g_object_unref(pixbuf);
		}

		g_free (cache_path);
	    }

	    g_free (cache_dir);
	    g_free (path);

	    status =
		(gfloat) thumbview->thumbs_count /
		ZALBUM (thumbview->album)->len;
	    thumbview_thumbs_progressbar (status);

	    while (gtk_events_pending ())
		gtk_main_iteration ();

	    thumbview_thumbs_do (NULL);
	    return TRUE;
	}
	else
	{
	    thumb_loader_free (thumbview->thumbs_loader);
	    thumbview->thumbs_loader = thumb_loader_new (path, 100, 100);
	    g_free (path);
	    thumb_loader_set_error_func (thumbview->thumbs_loader,
					 cb_thumbview_thumbs_error, NULL);
	    if (!thumb_loader_start
		(thumbview->thumbs_loader, cb_thumbview_thumbs_done, NULL))
	    {
		thumbview_thumbs_do (NULL);
		return TRUE;
	    }

	}
	return FALSE;
    }
    else
    {
	thumbview_thumbs_cleanup ();
	return FALSE;
    }

    return TRUE;
}

/*
 *-------------------------------------------------------------------
 * progs and scripts functions
 *-------------------------------------------------------------------
 */

/* signals */

static void
cb_open_image_by_external (GtkWidget * menuitem, ThumbView * tv)
{
    GList  *node;
    ZAlbumCell *cell;
    gint    action;
    gchar  *user_cmd, *cmd = NULL, *tmpstr = NULL, **pair;
    gboolean show_dialog = FALSE;

    g_return_if_fail (menuitem && tv);

    node = ZLIST (tv->album)->selection;
    if (!node)
	return;

    action =
	GPOINTER_TO_INT (g_object_get_data(G_OBJECT(menuitem), "num"));

    /*
     * find command
     */
    if (action < sizeof (conf.progs) / sizeof (conf.progs[0]))
    {
	pair = g_strsplit (conf.progs[action], ",", 3);
	if (!pair[1])
	{
	    g_strfreev (pair);
	    return;
	}
	else
	{
	    cmd = g_strdup (pair[1]);
	}
	if (pair[2] && !g_ascii_strcasecmp (pair[2], "TRUE"))
	    show_dialog = TRUE;
	g_strfreev (pair);
    }
    else
    {
	return;
    }

    /*
     * create command string
     */
    while (node)
    {
	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   (guint) node->data);

	tmpstr = g_strconcat (cmd, " ", "\"", cell->name, "\"", NULL);
	g_free (cmd);
	cmd = tmpstr;
	node = g_list_next (node);
    }

    tmpstr = g_strconcat (cmd, " &", NULL);
    g_free (cmd);
    cmd = tmpstr;
    tmpstr = NULL;

    tmpstr = cmd;
    cmd = charset_to_internal (cmd,
			       conf.charset_filename,
			       conf.charset_auto_detect_fn,
			       conf.charset_filename_mode);
    g_free (tmpstr);
    tmpstr = NULL;

    if (show_dialog)
    {
	user_cmd =
	    dialog_textentry (_("Execute command"),
			      _("Please enter options:"), cmd, NULL, 400,
			      TEXT_ENTRY_WRAP_ENTRY | TEXT_ENTRY_CURSOR_TOP);
    }
    else
    {
	user_cmd = g_strdup (cmd);
    }
    g_free (cmd);
    cmd = NULL;

    /*
     * exec command
     */
    if (user_cmd)
    {
	tmpstr = user_cmd;
	user_cmd = charset_internal_to_locale (user_cmd);
	g_free (tmpstr);
	tmpstr = NULL;
	system (user_cmd);
	g_free (user_cmd);
    }
}

static void
cb_open_image_by_script (GtkWidget * menuitem, ThumbView * tv)
{
    ZAlbumCell *cell;
    GList  *node;
    gchar  *script, *cmd = NULL, *tmpstr = NULL, *user_cmd;

    g_return_if_fail (menuitem && tv);

    node = ZLIST (tv->album)->selection;
    if (!node)
	return;

    script = g_object_get_data(G_OBJECT(menuitem), "script");
    if (!script || !script || !isexecutable (script))
	goto ERROR;

    cmd = g_strdup (script);

    /*
     * create command string
     */
    while (node)
    {
	cell =
	    ZLIST_CELL_FROM_INDEX (ZLIST (thumbview->album),
				   (guint) node->data);
	tmpstr = g_strconcat (cmd, " ", "\"", cell->name, "\"", NULL);
	g_free (cmd);
	cmd = tmpstr;
	node = g_list_next (node);
    }
    tmpstr = g_strconcat (cmd, " &", NULL);
    g_free (cmd);
    cmd = tmpstr;
    tmpstr = NULL;

    {
	tmpstr = cmd;
	cmd = charset_to_internal (cmd,
				   conf.charset_filename,
				   conf.charset_auto_detect_fn,
				   conf.charset_filename_mode);
	g_free (tmpstr);
	tmpstr = NULL;

	if (conf.scripts_show_dialog)
	{
	    user_cmd =
		dialog_textentry (_("Execute script"),
				  _("Please enter options:"), cmd, NULL,
				  400,
				  TEXT_ENTRY_WRAP_ENTRY |
				  TEXT_ENTRY_CURSOR_TOP);
	}
	else
	{
	    user_cmd = g_strdup (cmd);
	}
	g_free (cmd);
	cmd = NULL;

	tmpstr = user_cmd;
	user_cmd = charset_internal_to_locale (user_cmd);
	g_free (tmpstr);
	tmpstr = NULL;
    }

    /*
     * exec command
     */
    if (user_cmd)
    {
	{
	  /********** convert charset **********/
	    tmpstr = user_cmd;
	    user_cmd = charset_internal_to_locale (user_cmd);
	    g_free (tmpstr);
	    tmpstr = NULL;
	}
	system (user_cmd);
	g_free (user_cmd);
	user_cmd = NULL;
    }

  ERROR:
    return;
}

/* create submenu */

static GtkWidget *
create_progs_submenu (ThumbView * tv)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    gint    i, conf_num = sizeof (conf.progs) / sizeof (conf.progs[0]);
    gchar **pair;

    menu = gtk_menu_new ();

    /*
     * count items num
     */
    for (i = 0; i < conf_num; i++)
    {
	if (!conf.progs[i])
	    continue;

	pair = g_strsplit (conf.progs[i], ",", 3);

	if (pair[0] && pair[1])
	{
	    gchar  *label;

	    if (pair[2] && !strcasecmp (pair[2], "TRUE"))
		label = g_strconcat (pair[0], "...", NULL);
	    else
		label = g_strdup (pair[0]);

	    menu_item = gtk_menu_item_new_with_label (label);
	    g_object_set_data(G_OBJECT(menu_item), "num",
				GINT_TO_POINTER(i));
	    g_signal_connect(G_OBJECT (menu_item), "activate",
				G_CALLBACK(cb_open_image_by_external), tv);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	    gtk_widget_show (menu_item);

	    g_free (label);
	}

	g_strfreev (pair);
    }

    return menu;
}

static GtkWidget *
create_scripts_submenu (ThumbView * tv)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    GList  *tmplist = NULL, *filelist = NULL, *list;
    gchar **dirs;
	gchar *basename;
    gint    i, flags;

    menu = gtk_menu_new ();

    dirs = g_strsplit (conf.scripts_search_dir_list, ",", -1);
    if (!dirs)
	return NULL;

    flags = 0 | GETDIR_FOLLOW_SYMLINK;
    for (i = 0; dirs[i]; i++)
    {
	if (!*dirs || !isdir (dirs[i]))
	    continue;
	get_dir (dirs[i], flags, &tmplist, NULL);
	filelist = g_list_concat (filelist, tmplist);
    }
    g_strfreev (dirs);

    for (list = filelist; list; list = g_list_next (list))
    {
	gchar  *filename = list->data;
	gchar  *label;

	if (!filename || !*filename || !isexecutable (filename))
	    continue;

	basename = g_path_get_basename (filename);
	if (conf.scripts_show_dialog)
	    label = g_strconcat (basename, "...", NULL);
	else
	    label = g_strdup (basename);
	g_free(basename);

	menu_item = gtk_menu_item_new_with_label (label);
	g_object_set_data_full(G_OBJECT (menu_item),
				  "script",
				  g_strdup (filename),
				  (GDestroyNotify) g_free);
	g_signal_connect (G_OBJECT (menu_item),
			    "activate",
			    G_CALLBACK (cb_open_image_by_script), tv);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	g_free (label);
    }

    g_list_foreach (filelist, (GFunc) g_free, NULL);
    g_list_free (filelist);

    return menu;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
thumbview_create (GtkWidget * parent_win)
{
    thumbview = g_new0 (ThumbView, 1);
    thumbview->thumbs_loader = NULL;
    thumbview->thumbs_count = -1;
    thumbview->thumbs_running = FALSE;
    thumbview->thumbs_stop = FALSE;
    thumbview->selection = NULL;

    /*
     * container
     */
    thumbview->container = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (thumbview->container);

    /*
     * toolbar
     */
    thumbview->toolbar_eventbox = gtk_event_box_new ();
    gtk_container_set_border_width (GTK_CONTAINER
				    (thumbview->toolbar_eventbox), 1);
    gtk_box_pack_start (GTK_BOX (thumbview->container),
			thumbview->toolbar_eventbox, FALSE, FALSE, 0);
    gtk_widget_show (thumbview->toolbar_eventbox);

    thumbview->toolbar = thumbview_create_toolbar (thumbview);
    gtk_container_add (GTK_CONTAINER (thumbview->toolbar_eventbox),
		       thumbview->toolbar);


    thumbview->frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (thumbview->frame), GTK_SHADOW_IN);

    gtk_box_pack_start (GTK_BOX (thumbview->container),
			thumbview->frame, TRUE, TRUE, 0);

    gtk_widget_show (thumbview->frame);

    /*
     * scrolled window
     */
    thumbview->scroll_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (thumbview->scroll_win), 1);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
				    (thumbview->scroll_win),
				    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	/* TODO: use macro/global or different func? */
	gtk_widget_set_size_request(GTK_WIDGET (thumbview->scroll_win),
			  50, 200);

    gtk_container_add (GTK_CONTAINER (thumbview->frame),
		       thumbview->scroll_win);

    gtk_widget_show (thumbview->scroll_win);

    /*
     * create zalbum widget
     */
    thumbview->album = zalbum_new (ZALBUM_MODE_PREVIEW);
    gtk_container_add (GTK_CONTAINER(thumbview->scroll_win),
		       thumbview->album);

    g_signal_connect_after(GTK_OBJECT (thumbview->album),
			      "button_press_event",
			      G_CALLBACK (cb_thumbview_button_press),
			      thumbview);

    g_signal_connect_after(GTK_OBJECT (thumbview->album),
			      "button_release_event",
			     G_CALLBACK
			      (cb_thumbview_button_release), thumbview);
    g_signal_connect(GTK_OBJECT(thumbview->album), "key_press_event",
			G_CALLBACK(cb_thumbview_key_press), thumbview);

    g_signal_connect(GTK_OBJECT(thumbview->album), "cell_select",
			G_CALLBACK(cb_thumbview_cell_select),
			thumbview);

    g_signal_connect(GTK_OBJECT(thumbview->album), "cell_unselect",
			G_CALLBACK(cb_thumbview_cell_unselect),
			thumbview);

    gtk_widget_show (thumbview->album);

    zalbum_set_mode (ZALBUM (thumbview->album), conf.thumbview_mode);
}

void
thumbview_destroy (void)
{
    if (thumbview->thumbs_loader)
	thumb_loader_free (thumbview->thumbs_loader);

    gtk_widget_destroy (thumbview->album);

    g_free (thumbview);
    thumbview = NULL;
}

void
thumbview_add (FileList * fl)
{
    GList *node;
    FileListNode *data;

    thumbview_thumbs_interrupt ();

    zalbum_freeze (thumbview->album);

    for (node = fl->list->next; node; node = node->next) {
		data = node->data;
		zalbum_add(ZALBUM (thumbview->album), (gchar *) data->file_name,
				(gpointer) (data->file_type), NULL);
    }

    zalbum_thawn(thumbview->album);

    thumbview_toolbar_update(thumbview);

	DIRTREE(dirview->dirtree)->check_events = FALSE;

    if (ZALBUM(thumbview->album)->mode == ZALBUM_MODE_PREVIEW) {
		thumbview->thumbs_running = TRUE;
		while (thumbview_thumbs_next ());
    }
}

void
thumbview_clear (void)
{
    zalbum_freeze (thumbview->album);
    zlist_clear (ZLIST (thumbview->album));

    zalbum_thawn (thumbview->album);
}

void
thumbview_stop (void)
{
    if (thumbview->thumbs_running)
	thumbview->thumbs_stop = TRUE;
    ZLIST (thumbview->album)->focus = thumbview->current = -1;
}

void
thumbview_set_mode (gint mode)
{
    if (ZALBUM (thumbview->album)->mode == mode)
	return;

    if (mode == ZALBUM_MODE_PREVIEW)
    {
	zalbum_freeze (thumbview->album);
	zalbum_set_mode (ZALBUM (thumbview->album), ZALBUM_MODE_PREVIEW);
	zalbum_thawn (thumbview->album);

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/List"), TRUE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Thumbs"), FALSE);

	if (ZALBUM (thumbview->album)->len != 0)
	{
	    thumbview_thumbs_interrupt ();

	    thumbview->thumbs_running = TRUE;
	    while (thumbview_thumbs_next ());
	}
    }
    else
    {
	thumbview_thumbs_interrupt ();

	thumbview_clear ();
	zalbum_set_mode (ZALBUM (thumbview->album), ZALBUM_MODE_LIST);
	thumbview_add (browser->filelist);

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/List"), FALSE);
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/Thumbs"), TRUE);
    }
}

gint
thumbview_get_mode (void)
{
    return (ZALBUM (thumbview->album)->mode);
}

void
thumbview_select_first (void)
{
    cb_thumbview_first (NULL, thumbview);
}

void
thumbview_select_last (void)
{
    cb_thumbview_last (NULL, thumbview);
}

void
thumbview_select_next (void)
{
    cb_thumbview_next (NULL, thumbview);
}

void
thumbview_select_prev (void)
{
    cb_thumbview_previous (NULL, thumbview);
}

void
thumbview_refresh (void)
{
    cb_thumbview_refresh (NULL, thumbview);
}

gboolean
thumbview_is_next (void)
{
    gint    index;

    index = thumbview->current + 1;

    if (index > (ZALBUM (thumbview->album)->len - 1))
	return FALSE;

    return TRUE;
}

gboolean
thumbview_is_prev (void)
{
    gint    index;

    index = thumbview->current - 1;

    if (index < 0)
	return FALSE;

    return TRUE;
}

#ifdef ENABLE_MOVIE
void
thumbview_vupdate (void)
{
    if (!VIDEOPLAY_IS_HIDE)
    {
#ifndef ENABLE_MPLAYER
	if (VIDEOPLAY_STATUS != VIDEOPLAY_STATUS_NULL
	    && VIDEOPLAY_STATUS != VIDEOPLAY_STATUS_STOP
	    && VIDEOPLAY_STATUS != VIDEOPLAY_STATUS_PAUSE)

	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/FullScreen"), TRUE);
	else
	    gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
				"/ThumbviewToolbar/ToolItems/FullScreen"), FALSE);
#else
	gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
			"/ThumbviewToolbar/ToolItems/FullScreen"), FALSE);
#endif
    }
}
#endif

void
thumbview_iupdate (void)
{
	gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
			"/ThumbviewToolbar/ToolItems/NoZoom"),
			(IMAGEVIEW_ZOOM_TYPE == IMAGEVIEW_ZOOM_100) ? FALSE : TRUE);

	gtk_widget_set_sensitive (gtk_ui_manager_get_widget(toolbar_ui,
			"/ThumbviewToolbar/ToolItems/AutoZoom"),
			(IMAGEVIEW_ZOOM_TYPE == IMAGEVIEW_ZOOM_AUTO) ? FALSE : TRUE);
}
