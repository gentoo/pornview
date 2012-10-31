/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                        (c) 2002,2003                         (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __PREFS_H__
#define __PREFS_H__

typedef struct _Config
{
/* startup directory */
    gint            startup_dir_mode;
    gchar          *startup_dir;

/* main window */
    gboolean        save_win_state;
    gboolean        enable_dock;
    gint            window_width;
    gint            window_height;
    gint            dirtree_width;
    gint            dirtree_height;

/* filter */
    gchar          *imgtype_disables;
    gchar          *imgtype_user_defs;

/* charset */
    gchar          *charset_locale;
    gchar          *charset_internal;
    gint            charset_auto_detect_lang;
    gint            charset_filename_mode;
    gchar          *charset_filename;
    gpointer        charset_auto_detect_fn;

/* scan directory */
    gboolean        scan_dir;
    gboolean        check_hlinks;
    gboolean        show_dotfile;

/* dirtree style */
    gint            dirtree_line_style;
    gint            dirtree_expander_style;

/* thumbnails */
    gint            thumbnail_quality;

/* cache */
    gboolean        enable_thumb_dirs;
    gboolean        enable_thumb_caching;

/* thumbview mode */
    gint            thumbview_mode;

/* image */
    gint            image_quality;
    gint            image_dithering;

/* image zoom */
    gboolean        image_fit;

/* commnent */
    gchar          *comment_key_list;
    gint            comment_charset_read_mode;
    gint            comment_charset_write_mode;
    gchar          *comment_charset;

/* slideshow interval */
    gint            slideshow_interval;

/* external program & scripts */
    gchar          *progs[16];
    gchar          *scripts_search_dir_list;
    gboolean        scripts_show_dialog;

/* plugin */
    gchar          *plugin_search_dir_list;
}
Config;

extern Config   conf;

#define PORNVIEW_RC_DIR ".pornview"
#define PORNVIEW_RC_FILE "pornviewrc"

void            prefs_load_config (void);
void            prefs_save_config (void);

#endif /* __PREFS_H__ */
