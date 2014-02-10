/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __BROWSER_H__
#define __BROWSER_H__

#include <gtk/gtk.h>

#include "file_list.h"

#define BROWSER               browser
#define BROWSER_WINDOW        browser->window
#define BROWSER_VIEW          browser->notebook
#define BROWSER_PROGRESS      browser->progress
#define BROWSER_STATUS_DIR    browser->status_dir
#define BROWSER_STATUS_NAME   browser->status_name
#define BROWSER_STATUS_IMAGE  browser->status_image
#define BROWSER_CURRENT_PATH  browser->current_path->str
#define BROWSER_LAST_PATH     browser->last_path->str
#define BROWSER_FILE_LIST     browser->filelist

typedef struct _Browser Browser;

/**
 * Describes the browser and commonly
 * used properties.
 */
struct _Browser {
	/**
	 * Toplevel window.
	 */
    GtkWidget *window;
	/**
	 * Menu + Toolbar.
	 */
    GtkUIManager *menu_toolbar;
    GtkWidget *notebook;

    GtkWidget *status_dir;
    GtkWidget *status_name;
    GtkWidget *status_image;
    GtkWidget *progress;

    GString *current_path;
    GString *last_path;
	/**
	 * All image files in the current directory.
	 */
    FileList *filelist;
};

extern Browser *browser;

/**
 * Remove old cached thumbnails.
 */
static void browser_remove_old_thumbs(void);

/**
 * Remove all cached thumbnails.
 */
static void browser_clear_cache(void);

/**
 * Remove old cached comments.
 */
static void browser_remove_old_comments(void);

/**
 * Remove all cached comments
 */
static void browser_clear_comments(void);

/**
 * Open the preferences window.
 */
static void browser_prefs(void);

/**
 * Create a new browser window.
 *
 * @param start_path initial path for the dirview
 */
void browser_create(const gchar* start_path);

/**
 * Destroy the browser and clear all widgets/allocs/refs.
 */
void browser_destroy(void);

/**
 * Select a directory, update thumbnails, clear imageview
 * and videoplay.
 */
void browser_select_dir(gchar *path);


#endif /* __BROWSER_H__ */
