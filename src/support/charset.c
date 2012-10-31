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

#ifdef USE_GTK2
#
#else
#  ifdef HAVE_ICONV
#     include <stdlib.h>
#     include <iconv.h>
#     include <errno.h>
#  endif /* HAVE_ICONV */
#
#  ifdef HAVE_LIBCHARSET
#     include <libcharset.h>
#  elif defined (HAVE_LANGINFO_CODESET)
#     include <langinfo.h>
#  elif defined (USE_INCLUDED_LIBINTL)
extern const char *locale_charset (void);
#  else
#  endif /* HAVE_LIBCHARSET */
#endif /* USE_GTK2 */

extern const gchar *japanese_locale_charset (const gchar * locale);

CharsetDetectLocaleFn charset_locale_fn_table[] = {
    japanese_locale_charset,
};

#ifndef USE_GTK2
#ifndef HAVE_ICONV

 /*
  * See lib/encodings.def in libiconv 
  */
gchar  *charset_ascii_defs[] = {
    CHARSET_ASCII,
    "ASCII",
    "ISO646-US",
    "ISO_646.IRV:1991",
    "ISO-IR-6",
    "ANSI_X3.4-1968",
    "ANSI_X3.4-1986",
    "CP367",
    "IBM367",
    "US",
    "csASCII",
};
gint    charset_ascii_defs_num =
    sizeof (charset_ascii_defs) / sizeof (gchar *);

extern gchar *japanese_conv (const gchar * src,
			     const gchar * src_codeset,
			     const gchar * dest_codeset);

CharsetConvFn charset_conv_fn_table[] = {
    japanese_conv,
};
#endif /* HAVE_ICONV */
#endif /* USE_GTK2 */

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

#ifdef USE_GTK2
    if (!g_get_charset (&charset))
	charset = NULL;
#elif defined (HAVE_LIBCHARSET)
    charset = locale_charset ();
#elif defined (HAVE_LANGINFO_CODESET) && defined (HAVE_GLIBC21)
    charset = nl_langinfo (CODESET);
#elif defined (USE_INCLUDED_LIBINTL)
    charset = locale_charset ();
#else
#endif /* USE_GTK2 */

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

#ifdef USE_GTK2
    charset = CHARSET_UTF8;
#else /* USE_GTK2 */
    charset = charset_get_locale ();
#endif /* USE_GTK2 */

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

#ifdef USE_GTK2
    {
	gssize  len = -1;
	gsize   bytes_read, bytes_written;

	return g_locale_to_utf8 (src, len, &bytes_read, &bytes_written, NULL);
    }
#else /* USE_GTK2 */
    return charset_conv (src, charset_get_locale (), charset_get_internal ());
#endif /* USE_GTK2 */
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

#ifdef USE_GTK2
    {
	gssize  len = -1;
	gsize   bytes_read, bytes_written;
	return g_locale_from_utf8 (src, len, &bytes_read, &bytes_written,
				   NULL);
    }
#else /* USE_GTK2 */
    return charset_conv (src, charset_get_internal (), charset_get_locale ());
#endif /* USE_GTK2 */
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

#ifdef USE_GTK2
    {
	gint    rbytes, wbytes;
	return g_convert (src, -1, dest_codeset, src_codeset,
			  &rbytes, &wbytes, NULL);
    }
#else /* USE_GTK2 */
#   ifdef HAVE_ICONV
    {
	unsigned char *buf, *ret;
	iconv_t cd;
	size_t  insize = 0;
	size_t  outsize = 0;
	size_t  nconv = 0;
#ifdef ICONV_CONST
	ICONV_CONST char *inptr;
#else /* ICONV_CONST */
	char   *inptr;
#endif
	char   *outptr;

	buf = g_malloc (strlen (src) * 4 + 1);
	if (!buf)
	    return NULL;

	insize = strlen (src);
	inptr = (char *) src;
	outsize = strlen (src) * 4;
	outptr = buf;

	cd = iconv_open (dest_codeset, src_codeset);
	if (cd == (iconv_t) - 1)
	{
	    switch (errno)
	    {
	      case EINVAL:
		  g_free (buf);
		  return g_strdup (src);
	      default:
		  break;
	    }
	}

	nconv = iconv (cd, &inptr, &insize, &outptr, &outsize);
	if (nconv == (size_t) - 1)
	{
	    switch (errno)
	    {
	      case EINVAL:
		  g_free (buf);
		  return g_strdup (src);
		  break;
	      default:
		  break;
	    }
	}
	else
	{
	    iconv (cd, NULL, NULL, &outptr, &outsize);
	}

	*outptr = '\0';
	iconv_close (cd);

	ret = g_strdup (buf);
	g_free (buf);

	return ret;
    }
#else /* HAVE_ICONV */
    {
	gint    i, num =
	    sizeof (charset_conv_fn_table) / sizeof (CharsetConvFn);
	gchar  *ret;

	for (i = 0; i < num; i++)
	{
	    ret = charset_conv_fn_table[i] (src, src_codeset, dest_codeset);
	    if (ret)
		return ret;
	}
    }
#endif /* HAVE_ICONV */
#endif /* USE_GTK2 */

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

/*
 *  these codes are taken from GLib-2.0.0 (glib/gutf8.c)
 *
 *  Copyright (C) 1999 Tom Tromey
 *  Copyright (C) 2000 Red Hat, Inc.
 *
 */
#ifndef USE_GTK2

#define UTF8_COMPUTE(Char, Mask, Len)                                         \
  if (Char < 128)                                                             \
    {                                                                         \
      Len = 1;                                                                \
      Mask = 0x7f;                                                            \
    }                                                                         \
  else if ((Char & 0xe0) == 0xc0)                                             \
    {                                                                         \
      Len = 2;                                                                \
      Mask = 0x1f;                                                            \
    }                                                                         \
  else if ((Char & 0xf0) == 0xe0)                                             \
    {                                                                         \
      Len = 3;                                                                \
      Mask = 0x0f;                                                            \
    }                                                                         \
  else if ((Char & 0xf8) == 0xf0)                                             \
    {                                                                         \
      Len = 4;                                                                \
      Mask = 0x07;                                                            \
    }                                                                         \
  else if ((Char & 0xfc) == 0xf8)                                             \
    {                                                                         \
      Len = 5;                                                                \
      Mask = 0x03;                                                            \
    }                                                                         \
  else if ((Char & 0xfe) == 0xfc)                                             \
    {                                                                         \
      Len = 6;                                                                \
      Mask = 0x01;                                                            \
    }                                                                         \
  else                                                                        \
    Len = -1;


#define UTF8_LENGTH(Char)              \
  ((Char) < 0x80 ? 1 :                 \
   ((Char) < 0x800 ? 2 :               \
    ((Char) < 0x10000 ? 3 :            \
     ((Char) < 0x200000 ? 4 :          \
      ((Char) < 0x4000000 ? 5 : 6)))))


#define UTF8_GET(Result, Chars, Count, Mask, Len)                             \
  (Result) = (Chars)[0] & (Mask);                                             \
  for ((Count) = 1; (Count) < (Len); ++(Count))                               \
    {                                                                         \
      if (((Chars)[(Count)] & 0xc0) != 0x80)                                  \
        {                                                                     \
          (Result) = -1;                                                      \
          break;                                                              \
        }                                                                     \
      (Result) <<= 6;                                                         \
      (Result) |= ((Chars)[(Count)] & 0x3f);                                  \
    }

#define UNICODE_VALID(Char)                   \
    ((Char) < 0x110000 &&                     \
     ((Char) < 0xD800 || (Char) >= 0xE000) && \
     (Char) != 0xFFFE && (Char) != 0xFFFF)

gboolean
g_utf8_validate (const gchar * str, gssize max_len, const gchar ** end)
{
    const gchar *p;

    g_return_val_if_fail (str != NULL, FALSE);

    if (end)
	*end = str;

    p = str;

    while ((max_len < 0 || (p - str) < max_len) && *p)
    {
	int     i, mask = 0, len;
	gunichar result;
	unsigned char c = (unsigned char) *p;

	UTF8_COMPUTE (c, mask, len);

	if (len == -1)
	    break;

	/*
	 * check that the expected number of bytes exists in str 
	 */
	if (max_len >= 0 && ((max_len - (p - str)) < len))
	    break;

	UTF8_GET (result, p, i, mask, len);

	if (UTF8_LENGTH (result) != len)	/* Check for overlong UTF-8 */
	    break;

	if (result == (gunichar) - 1)
	    break;

	if (!UNICODE_VALID (result))
	    break;

	p += len;
    }

    if (end)
	*end = p;

    /*
     *  See that we covered the entire length if a length was
     *  passed in, or that we ended on a nul if not
     */
    if (max_len >= 0 && p != (str + max_len))
	return FALSE;
    else if (max_len < 0 && *p != '\0')
	return FALSE;
    else
	return TRUE;
}

#endif /* USE_GTK2 */
