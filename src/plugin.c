/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2003                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "plugin.h"

#include "prefs.h"

static GList *search_dir_list = NULL;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
plugin_clear_search_dir_list (void)
{
    g_list_foreach (search_dir_list, (GFunc) g_free, NULL);
    g_list_free (search_dir_list);
    search_dir_list = NULL;
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

void
plugin_create_search_dir_list (void)
{
    gchar **dirs;
    gint    i;

    plugin_clear_search_dir_list ();

    if (!conf.plugin_search_dir_list || !*conf.plugin_search_dir_list)
	return;

    dirs = g_strsplit (conf.plugin_search_dir_list, ",", -1);

    for (i = 0; dirs && dirs[i]; i++)
    {
	if (*dirs[i])
	    search_dir_list =
		g_list_append (search_dir_list, g_strdup (dirs[i]));
    }

    g_strfreev (dirs);
}

GList  *
plugin_get_search_dir_list (void)
{
    return search_dir_list;
}

void
plugin_init (void)
{
    plugin_create_search_dir_list ();
}

void
plugin_free (void)
{
    plugin_clear_search_dir_list ();
}
