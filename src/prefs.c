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

#include "pornview.h"

#include "charset.h"
#include "file_utils.h"
#include "file_type.h"

#include "prefs.h"
#include "comment.h"

Config  conf;

typedef enum
{
    D_NULL,
    D_STRING,
    D_PATH,
    D_INT,
    D_FLOAT,
    D_BOOL,
    D_ENUM
}
DataType;

typedef struct _ConfParm
{
    gchar  *keyname;
    DataType data_type;
    gchar  *default_val;
    gpointer data;
}
ConfParam;

typedef struct _PrefsSection
{
    gchar  *section_name;
    ConfParam *param;
}
PrefsSection;


typedef struct _PrefsFile
{
    gchar  *filename;
    PrefsSection *sections;
}
PrefsFile;

/* default config value */

static ConfParam param_common[] = {
    {"startup_dir_mode", D_INT, "0", &conf.startup_dir_mode},
    {"startup_dir", D_STRING, NULL, &conf.startup_dir},
    {"save_win_state", D_BOOL, "TRUE", &conf.save_win_state},
    {"enable_dock", D_BOOL, "TRUE", &conf.enable_dock},
    {"window_width", D_INT, "800", &conf.window_width},
    {"window_height", D_INT, "600", &conf.window_height},
    {"dirtree_width", D_INT, "360", &conf.dirtree_width},
    {"dirtree_height", D_INT, "207", &conf.dirtree_height},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_filter[] = {
    {"imgtype_disables", D_STRING, NULL, &conf.imgtype_disables},
    {"imgtype_user_defs", D_STRING, NULL, &conf.imgtype_user_defs},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_charset[] = {
    {"charset_locale", D_STRING, "default", &conf.charset_locale},
    {"charset_internal", D_STRING, "default", &conf.charset_internal},
    {"charset_auto_detect_lang", D_ENUM, "0", &conf.charset_auto_detect_lang},
#ifdef USE_GTK2
    {"charset_filename_mode", D_ENUM, "1", &conf.charset_filename_mode},
#else
    {"charset_filename_mode", D_ENUM, "0", &conf.charset_filename_mode},
#endif
    {"charset_filename", D_STRING, "default", &conf.charset_filename},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_dirview[] = {
    {"scan_dir", D_BOOL, "TRUE", &conf.scan_dir},
    {"check_hlinks", D_BOOL, "TRUE", &conf.check_hlinks},
    {"show_dotfile", D_BOOL, "FALSE", &conf.show_dotfile},
    {"dirtree_line_style", D_INT, "2", &conf.dirtree_line_style},
    {"dirtree_expander_style", D_INT, "1", &conf.dirtree_expander_style},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_thumbview[] = {
    {"thumbnail_quality", D_INT, "1", &conf.thumbnail_quality},
    {"enable_thumb_dirs", D_BOOL, "FALSE", &conf.enable_thumb_dirs},
    {"enable_thumb_caching", D_BOOL, "TRUE", &conf.enable_thumb_caching},
    {"thumbview_mode", D_INT, "1", &conf.thumbview_mode},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_imageview[] = {
    {"image_quality", D_INT, "1", &conf.image_quality},
    {"image_dithering", D_INT, "1", &conf.image_dithering},
    {"image_fit", D_BOOL, "FALSE", &conf.image_fit},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_comment[] = {
    {"comment_key_list", D_STRING, NULL, &conf.comment_key_list},
    {"comment_charset_read_mode", D_ENUM, "3",
     &conf.comment_charset_read_mode},
    {"comment_charset_write_mode", D_ENUM, "3",
     &conf.comment_charset_write_mode},
    {"comment_charset", D_STRING, "default", &conf.comment_charset},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_slideshow[] = {
    {"slideshow_interval", D_INT, "4000", &conf.slideshow_interval},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_progs[] = {
    {"progs[0]", D_STRING, "Gimp,gimp-remote -n,FALSE", &conf.progs[0]},
    {"progs[1]", D_STRING, "ElectricEyes,ee,FALSE", &conf.progs[1]},
#ifdef ENABLE_MOVIE
    {"progs[2]", D_STRING, "Xine,xine,FALSE", &conf.progs[2]},
    {"progs[3]", D_STRING, "Mplayer,mplayer,FALSE", &conf.progs[3]},
#else
    {"progs[2]", D_STRING, NULL, &conf.progs[2]},
    {"progs[3]", D_STRING, NULL, &conf.progs[3]},
#endif
    {"progs[4]", D_STRING, NULL, &conf.progs[4]},
    {"progs[5]", D_STRING, NULL, &conf.progs[5]},
    {"progs[6]", D_STRING, NULL, &conf.progs[6]},
    {"progs[7]", D_STRING, NULL, &conf.progs[7]},
    {"progs[8]", D_STRING, NULL, &conf.progs[8]},
    {"progs[9]", D_STRING, NULL, &conf.progs[9]},
    {"progs[10]", D_STRING, NULL, &conf.progs[10]},
    {"progs[11]", D_STRING, NULL, &conf.progs[11]},
    {"progs[12]", D_STRING, NULL, &conf.progs[12]},
    {"progs[13]", D_STRING, NULL, &conf.progs[13]},
    {"progs[14]", D_STRING, NULL, &conf.progs[14]},
    {"progs[15]", D_STRING, NULL, &conf.progs[15]},
    {"scripts_search_dir_list", D_STRING, PACKAGE_DATA_DIR "/scripts",
     &conf.scripts_search_dir_list},
    {"scripts_show_dialog", D_BOOL, "FALSE", &conf.scripts_show_dialog},
    {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_plugin[] = {
    {"plugin_search_dir_list", D_STRING, PACKAGE_DATA_DIR "/plugins",
     &conf.plugin_search_dir_list},
    {NULL, D_NULL, NULL, NULL}
};

static PrefsSection conf_sections[] = {
    {"Common", param_common},
    {"Filtering", param_filter},
    {"Character Set", param_charset},
    {"Directory View", param_dirview},
    {"Image View", param_imageview},
    {"Thumbnail View", param_thumbview},
    {"Comment", param_comment},
    {"Slide Show", param_slideshow},
    {"External Program", param_progs},
    {"Plugin", param_plugin},
    {NULL, NULL}
};

static PrefsFile conf_files[] = {
    {PORNVIEW_RC_FILE, conf_sections},
    {NULL, NULL},
};

static void
store_config (gpointer data, gchar * val, DataType type)
{
    switch (type)
    {
      case D_STRING:
	  g_free (*((gchar **) data));
	  if (!val)
	      *((gchar **) data) = NULL;
	  else
	      *((gchar **) data) = g_strdup (val);
	  break;
      case D_INT:
	  if (!val)
	      break;
	  *((gint *) data) = atoi (val);
	  break;
      case D_FLOAT:
	  if (!val)
	      break;
	  *((gfloat *) data) = atof (val);
	  break;
      case D_BOOL:
	  if (!val)
	      break;
	  if (!g_strcasecmp (val, "TRUE"))
	      *((gboolean *) data) = TRUE;
	  else
	      *((gboolean *) data) = FALSE;
	  break;
      case D_ENUM:
	  if (!val)
	      break;
	  *((gint *) data) = atoi (val);
	  break;
      default:
	  break;
    }
}

static void
prefs_load_config_default (PrefsSection * sections)
{
    gint    i, j;

    for (j = 0; sections[j].section_name; j++)
    {
	ConfParam *param = sections[j].param;
	for (i = 0; param[i].keyname; i++)
	    store_config (param[i].data, param[i].default_val,
			  param[i].data_type);
    }

    if (conf.startup_dir_mode == 0 || isdir (conf.startup_dir) == FALSE)
	conf.startup_dir = g_strdup (getenv ("HOME"));
}

static void
prefs_load_rc (gchar * filename, PrefsSection * sections)
{
    gchar   rcfile[MAX_PATH_LEN];
    FILE   *pornviewrc;
    gchar   buf[BUF_SIZE];
    gchar **pair;
    gint    i, j;

    prefs_load_config_default (sections);

    g_snprintf (rcfile, MAX_PATH_LEN, "%s/%s/%s",
		getenv ("HOME"), PORNVIEW_RC_DIR, filename);

    pornviewrc = fopen (rcfile, "r");

    if (!pornviewrc)
    {
	return;
    }

    while (fgets (buf, sizeof (buf), pornviewrc))
    {
	if (buf[0] == '[' || buf[0] == '\n')
	    continue;

	g_strstrip (buf);

	pair = g_strsplit (buf, "=", 2);
	if (pair[0])
	    g_strstrip (pair[0]);
	if (pair[1])
	    g_strstrip (pair[1]);

	for (j = 0; sections[j].section_name; j++)
	{
	    ConfParam *param = sections[j].param;
	    for (i = 0; param[i].keyname; i++)
	    {
		if (!g_strcasecmp (param[i].keyname, pair[0]))
		{
		    store_config (param[i].data, pair[1], param[i].data_type);
		    break;
		}
	    }
	}
	g_strfreev (pair);
    }

    fclose (pornviewrc);

    if (!isdir (conf.startup_dir))
    {
	if (conf.startup_dir)
	    g_free (conf.startup_dir);

	conf.startup_dir = g_strdup (getenv ("HOME"));
    }
}

void
prefs_load_config (void)
{
    gint    i;

    for (i = 0; conf_files[i].filename; i++)
	prefs_load_rc (conf_files[i].filename, conf_files[i].sections);

    charset_set_locale_charset (conf.charset_locale);
    charset_set_internal_charset (conf.charset_internal);
    conf.charset_auto_detect_fn
	= charset_get_auto_detect_func (conf.charset_auto_detect_lang);

    comment_update_data_entry_list ();
    file_type_update_image_types (conf.imgtype_disables,
				  conf.imgtype_user_defs);
}

static void
prefs_save_rc (gchar * filename, PrefsSection * sections)
{
    gchar   dir[MAX_PATH_LEN], rcfile[MAX_PATH_LEN];
    FILE   *pornviewrc;
    gint    i, j;
    gchar  *bool;
    struct stat st;

    g_snprintf (dir, MAX_PATH_LEN, "%s/%s", getenv ("HOME"), PORNVIEW_RC_DIR);

    if (stat (dir, &st))
    {
	mkdir (dir, S_IRWXU);
    }
    else
    {
	if (!S_ISDIR (st.st_mode))
	{
	    return;
	}
    }

    g_snprintf (rcfile, MAX_PATH_LEN, "%s/%s/%s", getenv ("HOME"),
		PORNVIEW_RC_DIR, filename);
    pornviewrc = fopen (rcfile, "w");
    if (!pornviewrc)
    {
	return;
    }

    for (j = 0; sections[j].section_name; j++)
    {
	ConfParam *param = sections[j].param;
	fprintf (pornviewrc, "[%s]\n", sections[j].section_name);
	for (i = 0; param[i].keyname; i++)
	{
	    switch (param[i].data_type)
	    {
	      case D_STRING:
		  if (!*(gchar **) param[i].data)
		      fprintf (pornviewrc, "%s=\n", param[i].keyname);
		  else
		      fprintf (pornviewrc, "%s=%s\n", param[i].keyname,
			       *(gchar **) param[i].data);
		  break;
	      case D_INT:
		  fprintf (pornviewrc, "%s=%d\n", param[i].keyname,
			   *((gint *) param[i].data));
		  break;
	      case D_FLOAT:
		  fprintf (pornviewrc, "%s=%f\n", param[i].keyname,
			   *((gfloat *) param[i].data));
		  break;
	      case D_BOOL:
		  if (*((gboolean *) param[i].data))
		      bool = "TRUE";
		  else
		      bool = "FALSE";
		  fprintf (pornviewrc, "%s=%s\n", param[i].keyname, bool);
		  break;
	      case D_ENUM:
		  fprintf (pornviewrc, "%s=%d\n", param[i].keyname,
			   *((gint *) param[i].data));
		  break;
	      default:
		  break;
	    }
	}
	fprintf (pornviewrc, "\n");
    }

    fclose (pornviewrc);

    file_type_free_image_types ();
}

void
prefs_save_config (void)
{
    gint    i;

    for (i = 0; conf_files[i].filename; i++)
	prefs_save_rc (conf_files[i].filename, conf_files[i].sections);

    if (conf.startup_dir)
	g_free (conf.startup_dir);
}
