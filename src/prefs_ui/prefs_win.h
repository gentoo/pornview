/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __PREFS_WIN_H__
#define __PREFS_WIN_H__

#include "prefs.h"
#include "prefs_ui_utils.h"

typedef enum
{
  PREFS_WIN_OK,
  PREFS_WIN_APPLY,
  PREFS_WIN_CANCEL
}
PrefsWinButton;

typedef GtkWidget *(*PrefsWinCreatePageFunc) (void);
typedef void    (*PrefsWinApplyFunc) (Config * src, Config * dest, PrefsWinButton state);

typedef struct PrefsWinPage_Tag
{
    const gchar    *const path;
#if 0
    gint            priority_hint;
#else
    const gchar    *const sibling;
#endif
    const gchar    *const icon;
    const gchar    *const icon_open;
    PrefsWinCreatePageFunc create_page_fn;
    PrefsWinApplyFunc apply_fn;

}
PrefsWinPage;

void            prefs_win_open (const gchar * path);
void            prefs_win_open_idle (const gchar * path);
void            prefs_win_set_page (const gchar * path);
gboolean        prefs_win_is_opened (void);

#if 0

typedef struct PrefsWin_Tag PrefsWin;
typedef struct PrefsWinClass_Tag PrefsWinClass;

typedef enum
{
    PrefsWinModeNotebook,
    PrefsWinModeTree,
    PrefsWinModeStackButtons,
}
PrefsWinMode;

struct PrefsWin_Tag
{
    GtkDialog       parent;

    GtkWidget      *nav_tree;
    GtkWieget      *notebook;
};

struct PrefsWinClass_Tag
{
    GtkDialogClass  parent_class;

    void            (*ok) (PrefsWin * window);
    void            (*apply) (PrefsWin * window);
    void            (*cancel) (PrefsWin * window);
};

void            prefs_win_new (PrefsWinMode mode);
void            prefs_win_new_new_with_pages (PrefsWinMode mode,
					      PrefsWinPageEntry * page[],
					      gint n_pages);
void            prefs_win_show_modal (PrefsWin * window);
void            prefs_win_create_pages (PrefsWin * widnow,
					PrefsWinPageEntry * page[],
					gint n_pages);
GtkWidget      *prefs_win_append_page (PrefsWin * window,
				       PrefsWinPageEntry * page);
void            prefs_win_remove_page (const gchar * path);

#endif

#endif /* __PREFS_WIN_H__ */
