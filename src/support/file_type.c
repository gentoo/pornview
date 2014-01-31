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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <glib.h>

#include "file_type.h"
#include "file_utils.h"

/* for file name extension list */
typedef struct _img_ext
{
    gchar  *ext;
    gchar  *type;
}
img_ext;

static img_ext img_exts[] = {
    {"jpg", IMG_JPG},
    {"jpe", IMG_JPG},
    {"jpeg", IMG_JPG},
    {"png", IMG_PNG},
    {NULL, NULL}
};

static img_ext movie_exts[] = {
#if ENABLE_MOVIE
    {"mpg", "MPEG"},
    {"mpeg", "MPEG"},
    {"m2v", "MPEG"},
    {"vob", "MPEG"},
    {"avi", "AVI"},
    {"asf", "ASF"},
    {"wmv", "WMV"},
    {"qt", "QuickTime"},
    {"mov", "QuickTime"},
#if (defined ENABLE_MPLAYER) || (defined ENABLE_XINE)
    {"rm", "RealMedia"},
#endif
#endif
    {NULL, NULL}
};

static GList *img_system_ext_list_all = NULL;

static GList *img_system_ext_list = NULL;
static GHashTable *img_system_ext_table = NULL;

static GList *img_user_ext_list = NULL;
static GHashTable *img_user_ext_table = NULL;

static  gboolean
sysdef_check_enable (gchar * ext, gchar ** list)
{
    gint    i;

    if (!list)
	return TRUE;

    for (i = 0; list[i]; i++)
    {
	if (list[i] && *list[i] && !strcmp (ext, list[i]))
	    return FALSE;
    }

    return TRUE;
}

void
file_type_update_image_types (gchar * imgtype_disables,
			      gchar * imgtype_user_defs)
{
    gchar **sysdef_disables = NULL;
    gchar **user_defs = NULL, **sections = NULL;
    gchar  *ext, *type;
    gint    i, j;
    img_ext *exts;
    img_ext *exts_array[] = { img_exts, movie_exts };

    file_type_free_image_types ();

    /*
     * store system defined image types
     */
    if (imgtype_disables && *imgtype_disables)
    {
	sysdef_disables = g_strsplit (imgtype_disables, ",", -1);
	if (sysdef_disables)
	    for (i = 0; sysdef_disables[i]; i++)
		if (*sysdef_disables[i])
		    g_strstrip (sysdef_disables[i]);
    }

    if (!img_system_ext_table)
	img_system_ext_table = g_hash_table_new (g_str_hash, g_str_equal);

    for (j = 0; j < sizeof (exts_array) / sizeof (img_ext *); j++)
    {
	exts = exts_array[j];
	for (i = 0; exts[i].ext; i++)
	{
	    img_system_ext_list_all = g_list_append (img_system_ext_list_all,
						     exts[i].ext);

	    if (sysdef_check_enable (exts[i].ext, sysdef_disables))
	    {
		img_system_ext_list = g_list_append (img_system_ext_list,
						     exts[i].ext);
	    }

	    g_hash_table_insert (img_system_ext_table,
				 exts[i].ext, exts[i].type);
	}
    }

    if (sysdef_disables)
	g_strfreev (sysdef_disables);

    /*
     * store user defined image types
     */
    if (imgtype_user_defs && *imgtype_user_defs)
	user_defs = g_strsplit (imgtype_user_defs, ";", -1);
    if (!user_defs)
	return;

    if (!img_user_ext_table)
	img_user_ext_table = g_hash_table_new (g_str_hash, g_str_equal);

    for (i = 0; user_defs[i]; i++)
    {
	if (!*user_defs[i])
	    continue;

	sections = g_strsplit (user_defs[i], ",", 3);
	if (!sections)
	    continue;
	g_strstrip (sections[0]);
	g_strstrip (sections[1]);
	g_strstrip (sections[2]);
	if (!*sections[0])
	    goto ERROR;
	/*
	 * if (filter_check_duplicate (sections[0], -1, FALSE)) goto ERROR;
	 */
	if (!*sections[2] || strcasecmp (sections[2], "ENABLE"))
	    goto ERROR;

	ext = g_strdup (sections[0]);
	if (!*sections[1])
	    type = g_strdup ("UNKNOWN");
	else
	    type = g_strdup (sections[1]);

	img_user_ext_list = g_list_append (img_user_ext_list, ext);
	g_hash_table_insert (img_user_ext_table, ext, type);

      ERROR:
	g_strfreev (sections);
    }

    g_strfreev (user_defs);
}

void
file_type_free_image_types (void)
{
    if (img_system_ext_list_all)
	g_list_free (img_system_ext_list_all);
    img_system_ext_list_all = NULL;

    if (img_system_ext_list)
	g_list_free (img_system_ext_list);
    img_system_ext_list = NULL;

    if (img_system_ext_table)
	g_hash_table_destroy (img_system_ext_table);
    img_system_ext_table = NULL;

    if (img_user_ext_table)
	g_hash_table_destroy (img_user_ext_table);
    img_user_ext_table = NULL;

    if (img_user_ext_list)
    {
	g_list_foreach (img_user_ext_list, (GFunc) g_free, NULL);
	g_list_free (img_user_ext_list);
    }

    img_user_ext_list = NULL;
}

gchar  *
file_type_detect_type_by_ext (const gchar * str)
{
    gchar  *ext = extension (str);
    GList  *list;

    if (!ext)
	return NULL;

    for (list = g_list_first (img_system_ext_list); list;
	 list = g_list_next (list))
    {
	gchar  *sysext = list->data;

	if (!sysext)
	    continue;

	if (!g_ascii_strcasecmp (ext, sysext))
	{
	    gchar  *type = g_hash_table_lookup (img_system_ext_table, sysext);
	    return type;
	}
    }

    for (list = g_list_first (img_user_ext_list); list;
	 list = g_list_next (list))
    {
	gchar  *userext = list->data;

	if (!userext)
	    continue;

	if (!g_ascii_strcasecmp (ext, userext))
	{
	    gchar  *type = g_hash_table_lookup (img_user_ext_table, userext);
	    return type;
	}
    }

    return NULL;
}

GList  *
file_type_get_sysdef_ext_list_all (void)
{
    return img_system_ext_list_all;
}

GList  *
file_type_get_sysdef_ext_list (void)
{
    return img_system_ext_list;
}

GList  *
file_type__get_userdef_ext_list (void)
{
    return img_user_ext_list;
}

gchar  *
file_type_get_type_from_ext (gchar * ext)
{
    gchar  *type;

    g_return_val_if_fail (ext && *ext, NULL);

    type = g_hash_table_lookup (img_system_ext_table, ext);

    if (!type)
	type = g_hash_table_lookup (img_user_ext_table, ext);

    return type;
}

gboolean
file_type_is_movie (gchar * filename)
{
    gint    i;

    for (i = 0; movie_exts[i].ext; i++)
    {
	if (extension_is (filename, movie_exts[i].ext))
	{
	    return TRUE;
	}
    }

    return FALSE;
}

gboolean
file_type_is_jpeg (gchar * filename)
{
    gchar  *format;

    format = file_type_detect_type_by_ext (filename);

    if (!format || !*format || g_ascii_strcasecmp (format, IMG_JPG))
	return FALSE;

    return TRUE;
}
