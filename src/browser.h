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

#include "file_list.h"

typedef struct _Browser Browser;

struct _Browser
{
    GtkWidget      *window;
    GtkItemFactory *menu;
    GtkWidget      *notebook;
    
    GtkWidget      *status_dir;
    GtkWidget      *status_name;
    GtkWidget      *status_image;
    GtkWidget      *progress;

    GString        *current_path;
    GString        *last_path;

    FileList       *filelist;
};

extern Browser *browser;

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

void            browser_create (gchar* path);
void            browser_destroy (void);
void            browser_select_dir (gchar * path);

#endif /* __BROWSER_H__ */
