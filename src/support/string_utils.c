/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>
#include <glib.h>

#include "string_utils.h"
#include "intl.h"

gchar  *
menu_translate (const gchar * path, gpointer data)
{
    return _(path);
}

gchar  *
remove_trailing_slash (const gchar * path)
{
    gchar  *ret;
    gint    l;
    if (!path)
	return NULL;

    ret = g_strdup (path);
    l = strlen (ret);
    if (l > 1 && ret[l - 1] == '/')
	ret[l - 1] = '\0';

    return ret;
}

gchar  *
boolean_to_text (gboolean boolval)
{
    if (boolval)
	return "TRUE";
    else
	return "FALSE";
}

gboolean
text_to_boolean (gchar * text)
{
    g_return_val_if_fail (text && *text, FALSE);

    if (!strcasecmp (text, "TRUE") || !strcasecmp (text, "ENABLE"))
	return TRUE;
    else
	return FALSE;
}

gboolean
string_is_ascii_graphable (const char *string)
{
    int     i = 0, printable;

    if (!string)
	return FALSE;

    while (string[i])
    {
	printable = isgraph (string[i]);
	if (!printable)
	    return FALSE;
	i++;
    }

    return TRUE;
}

gboolean
string_is_ascii_printable (const char *string)
{
    int     i = 0, printable;

    if (!string)
	return FALSE;

    while (string[i])
    {
	printable = isprint (string[i]);
	if (!printable)
	    return FALSE;
	i++;
    }

    return TRUE;
}
