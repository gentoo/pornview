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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "charset.h"
#include "intl.h"

extern const gchar *japanese_locale_charset (const gchar * locale);

CharsetDetectLocaleFn charset_locale_fn_table[] = {
    japanese_locale_charset,
};

static  gboolean
is_default_codeset (const gchar * charset)
{
    if (!charset && !*charset)
	return TRUE;

    if (!strcasecmp ("default", charset)
	|| !strcasecmp ("none", charset)
	|| !strcasecmp ("auto", charset)
	|| !strcasecmp ("auto detect", charset)
	|| !strcasecmp ("auto-detect", charset)
	|| !strcasecmp ("auto_detect", charset))
    {
	return TRUE;
    }

    return FALSE;
}

 /*
  * known character set list 
  */
static const gchar *knwon_charset_items[] = {
    "default",
    CHARSET_ASCII,
    CHARSET_JIS,
    CHARSET_EUC_JP,
    CHARSET_SJIS,
    CHARSET_UTF8,
};

static GList *known_charset_list = NULL;

/* FIXME */
GList  *
charset_get_known_list (const gchar * lang)
{
    gint    i, num = sizeof (knwon_charset_items) / sizeof (gchar *);

    if (known_charset_list)
	return known_charset_list;

    for (i = 0; i < num; i++)
    {
	known_charset_list = g_list_append (known_charset_list,
					    (gpointer)
					    knwon_charset_items[i]);
    }

    return known_charset_list;
}

 /*
  * auto detect method for each language. 
  */
extern const gchar *japanese_detect_charset (const gchar * str);

CharsetAutoDetectFn auto_detect_fn_table[] = {
    NULL,
    japanese_detect_charset,
};

const gchar *charset_auto_detect_labels[] = {
    N_("None"),
    N_("Japanese"),
    NULL
};

CharsetAutoDetectFn
charset_get_auto_detect_func (CharsetAutoDetectType type)
{
    gint    num =
	sizeof (auto_detect_fn_table) / sizeof (CharsetAutoDetectFn);

    if (type < 0 || type > num)
	return NULL;

    return auto_detect_fn_table[type];
}

 /*
  * detecting locale & internal charset. 
  */
gchar  *charset_locale = NULL;
gchar  *charset_internal = NULL;

const gchar *
get_lang (void)
{
    const gchar *lang = NULL;

    lang = g_getenv ("LANGUAGE");

    if (!lang)
	lang = g_getenv ("LC_ALL");

    if (!lang)
	lang = g_getenv ("LC_CTYPE");

    if (!lang)
	lang = g_getenv ("LC_MESSAGES");

    if (!lang)
	lang = g_getenv ("LANG");

    if (!lang)
	lang = "C";

    return lang;
}

void
charset_set_locale_charset (const gchar * charset)
{
    if (charset_locale)
	g_free (charset_locale);

    if (charset && *charset)
    {
	if (is_default_codeset (charset))
	{
	    charset_locale = NULL;
	}
	else
	{
	    charset_locale = g_strdup (charset);
	}
    }
    else
    {
	charset_locale = NULL;
    }
}

void
charset_set_internal_charset (const gchar * charset)
{
    if (charset_internal)
	g_free (charset_internal);

    if (charset && *charset)
    {
	if (is_default_codeset (charset))
	{
	    charset_internal = NULL;
	}
	else
	{
	    charset_internal = g_strdup (charset);
	}
    }
    else
    {
	charset_internal = NULL;
    }
}

const gchar *
charset_get_locale (void)
{
    const gchar *charset = NULL;

    if (charset_locale && *charset_locale)
	return charset_locale;

    if (!g_get_charset (&charset))
	charset = NULL;

    if (!charset || !*charset)
    {
	gint    i, num =
	    sizeof (charset_locale_fn_table) / sizeof (CharsetDetectLocaleFn);
	const gchar *lang;

	lang = get_lang ();

	for (i = 0; i < num; i++)
	{
	    charset = charset_locale_fn_table[i] (lang);

	    if (charset)
		break;
	}
    }

    if (charset && *charset)
    {
	if (charset_locale)
	    g_free (charset_locale);
	charset_locale = g_strdup (charset);

	return charset_locale;
    }

    return CHARSET_ASCII;
}

const gchar *
charset_get_internal (void)
{
    const gchar *charset;

    if (charset_internal && *charset_internal)
	return charset_internal;

    charset = CHARSET_UTF8;

    if (charset && *charset)
    {
	if (charset_internal)
	    g_free (charset_internal);
	charset_internal = g_strdup (charset);

	return charset_internal;

    }
    else
    {
	return CHARSET_ASCII;
    }
}

 /*
  * any code -> internal converter 
  */
gchar  *
charset_to_internal (const gchar * src,
		     const gchar * src_codeset,
		     CharsetAutoDetectFn func, CharsetToInternalTypes type)
{
    g_return_val_if_fail (src, NULL);

    switch (type)
    {
      case CHARSET_TO_INTERNAL_NEVER:
	  return g_strdup (src);
      case CHARSET_TO_INTERNAL_LOCALE:
	  return charset_locale_to_internal (src);
      case CHARSET_TO_INTERNAL_AUTO:
	  return charset_to_internal_auto (src, func);
      case CHARSET_TO_INTERNAL_ANY:
	  if (is_default_codeset (src_codeset))
	  {
	      src_codeset = charset_get_locale ();
	  }
	  return charset_conv (src, src_codeset, charset_get_internal ());
      default:
	  break;
    }

    return g_strdup (src);
}

gchar  *
charset_locale_to_internal (const gchar * src)
{
    g_return_val_if_fail (src, NULL);

    {
	gssize  len = -1;
	gsize   bytes_read, bytes_written;

	return g_locale_to_utf8 (src, len, &bytes_read, &bytes_written, NULL);
    }
}

gchar  *
charset_to_internal_auto (const gchar * src, CharsetAutoDetectFn func)
{
    const gchar *charset = charset_get_internal ();
    if (charset)
	return charset_conv_auto (src, charset, func);

    return g_strdup (src);
}

 /*
  * any code -> locale converter 
  */
gchar  *
charset_to_locale (const gchar * src,
		   const gchar * src_codeset,
		   CharsetAutoDetectFn func, CharsetToLocaleTypes type)
{
    g_return_val_if_fail (src, NULL);

    switch (type)
    {
      case CHARSET_TO_LOCALE_NEVER:
	  return g_strdup (src);
      case CHARSET_TO_LOCALE_INTERNAL:
	  return charset_internal_to_locale (src);
      case CHARSET_TO_LOCALE_AUTO:
	  return charset_to_locale_auto (src, func);
      case CHARSET_TO_LOCALE_ANY:
	  if (is_default_codeset (src_codeset))
	      src_codeset = charset_get_internal ();
	  return charset_conv (src, src_codeset, charset_get_locale ());
      default:
	  break;
    }

    return g_strdup (src);
}

gchar  *
charset_internal_to_locale (const gchar * src)
{
    g_return_val_if_fail (src, NULL);

    {
	gssize  len = -1;
	gsize   bytes_read, bytes_written;
	return g_locale_from_utf8 (src, len, &bytes_read, &bytes_written,
				   NULL);
    }
}

gchar  *
charset_to_locale_auto (const gchar * src, CharsetAutoDetectFn func)
{
    const gchar *dest_charset;

    dest_charset = charset_get_locale ();
    if (dest_charset)
	return charset_conv_auto (src, dest_charset, func);
    else
	return g_strdup (src);

    return g_strdup (src);
}

 /*
  * internal -> any code converter 
  */
gchar  *
charset_from_internal (const gchar * src, const gchar * dest_codeset)
{
    g_return_val_if_fail (src, NULL);
    g_return_val_if_fail (dest_codeset && *dest_codeset, g_strdup (src));

    return charset_conv (src, charset_get_internal (), dest_codeset);
}

 /*
  * locale -> any code converter 
  */
gchar  *
charset_from_locale (const gchar * src, const gchar * dest_codeset)
{
    g_return_val_if_fail (src, NULL);
    g_return_val_if_fail (dest_codeset && *dest_codeset, g_strdup (src));

    return charset_conv (src, charset_get_locale (), dest_codeset);
}

 /*
  * any -> any code converter 
  */
gchar  *
charset_conv (const gchar * src,
	      const gchar * src_codeset, const gchar * dest_codeset)
{
    g_return_val_if_fail (src, NULL);
    g_return_val_if_fail (src_codeset && *src_codeset, g_strdup (src));
    g_return_val_if_fail (dest_codeset && *dest_codeset, g_strdup (src));

    {
	gint    rbytes, wbytes;
	return g_convert (src, -1, dest_codeset, src_codeset,
			  &rbytes, &wbytes, NULL);
    }

    return g_strdup (src);
}

gchar  *
charset_conv_auto (const gchar * src,
		   const gchar * dest_codeset, CharsetAutoDetectFn func)
{
    const gchar *src_codeset;

    g_return_val_if_fail (src, NULL);
    g_return_val_if_fail (func, g_strdup (src));
    g_return_val_if_fail (dest_codeset && *dest_codeset, g_strdup (src));

    src_codeset = func (src);

    g_return_val_if_fail (src_codeset && *src_codeset, g_strdup (src));

    return charset_conv (src, src_codeset, dest_codeset);
}
