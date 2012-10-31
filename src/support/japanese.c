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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charset.h"

/* force convert hankaku SJIS character to zenkaku */
#undef NO_HANKAKU_SJIS

#define NUL     0
#define LF      10
#define FF      12
#define CR      13
#define ESC     27
#define SS2     142

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

enum
{
    KC_ASCII,
    KC_EUC,
    KC_JIS,
    KC_SJIS,
    KC_EUCORSJIS,
    KC_UTF8
};

#ifndef USE_GTK2
#ifndef HAVE_ICONV

/* See lib/encodings.def in libiconv */
extern gchar *charset_ascii_defs[];
extern gint charset_ascii_defs_num;

static gchar *charset_euc_defs[] = {
    CHARSET_EUC_JP,
    "EUCJP",
    "Extended_UNIX_Code_Packed_Format_for_Japanese",
    "csEUCPkdFmtJapanese",
};

static gchar *charset_jis_defs[] = {
    CHARSET_JIS,
    "csISO2022JP",
    "csISO2022JP-1",
};

static gchar *charset_sjis_defs[] = {
    CHARSET_SJIS,
    "SJIS",
    "SHIFT-JIS",
    "MS_KANJI",
    "csShiftJIS",
};

#endif /* HAVE_ICONV */
#endif /*  USE_GTK2 */

static const gchar *locale_euc[] = {
    "ja", "ja_JP", "ja_JP.ujis", "ja_JP.EUC", "ja_JP.eucJP", "ja_JP.eucjp",
};
static const gchar *locale_jis[] = {
    "ja_JP.JIS", "ja_JP.jis", "ja_JP.iso-2022-jp",
};
static const gchar *locale_sjis[] = {
    "ja_JP.SJIS", "ja_JP.sjis", "ja_JP.Shift_JIS",
};

const gchar *
japanese_locale_charset (const gchar * locale)
{
    gint    i;

    if (!locale || !*locale)
	return NULL;

    for (i = 0; i < sizeof (locale_euc) / sizeof (gchar *); i++)
	if (!strcasecmp (locale_euc[i], locale))
	    return CHARSET_EUC_JP;
    for (i = 0; i < sizeof (locale_jis) / sizeof (gchar *); i++)
	if (!strcasecmp (locale_jis[i], locale))
	    return CHARSET_JIS;
    for (i = 0; i < sizeof (locale_sjis) / sizeof (gchar *); i++)
	if (!strcasecmp (locale_sjis[i], locale))
	    return CHARSET_SJIS;

    return NULL;
};


/******************************************************************************
 *
 *  these codes are mostly taken from kanji_conv.c
 *
 *   Copyright (C) 2000 Takuo Kitame <kitame@gnome.gr.jp>
 *
 *****************************************************************************/
static int
detect_kanji (const guchar * str)
{
    int     expected = KC_ASCII;
    register int c;
    int     c1, c2;
    int     euc_c = 0, sjis_c = 0;
    const guchar *ptr = str;

    g_return_val_if_fail (str && *str, 0);

    while ((c = *ptr) != '\0')
    {
	if (c == ESC)
	{
	    if ((c = *(++ptr)) == '\0')
		break;
	    if (c == '$')
	    {
		if ((c = *(++ptr)) == '\0')
		    break;
		if (c == 'B' || c == '@')
		    return KC_JIS;
	    }
	    ptr++;
	    continue;
	}

	if ((c >= 0x81 && c <= 0x8d) || (c >= 0x8f && c <= 0x9f))
	    return KC_SJIS;

	if (c == SS2)
	{
	    if ((c = *(++ptr)) == '\0')
		break;
	    if ((c >= 0x40 && c <= 0x7e)
		|| (c >= 0x80 && c <= 0xa0) || (c >= 0xe0 && c <= 0xfc))
	    {
		return KC_SJIS;
	    }
	    if (c >= 0xa1 && c <= 0xdf)
		break;

	    ptr++;
	    continue;
	}

	if (c >= 0xa1 && c <= 0xdf)
	{
	    if ((c = *(++ptr)) == '\0')
		break;

	    if (c >= 0xe0 && c <= 0xfe)
		return KC_EUC;
	    if (c >= 0xa1 && c <= 0xdf)
	    {
		expected = KC_EUCORSJIS;
		ptr++;
		continue;
	    }
#if 1
	    if (c == 0xa0 || (0xe0 <= c && c <= 0xfe))
	    {
		return KC_EUC;
	    }
	    else
	    {
		expected = KC_EUCORSJIS;
		ptr++;
		continue;
	    }
#else
	    if (c <= 0x9f)
		return KC_SJIS;
	    if (c >= 0xf0 && c <= 0xfe)
		return KC_EUC;
#endif

	    if (c >= 0xe0 && c <= 0xef)
	    {
		expected = KC_EUCORSJIS;
		while (c >= 0x40)
		{
		    if (c >= 0x81)
		    {
			if (c <= 0x8d || (c >= 0x8f && c <= 0x9f))
			{
			    return KC_SJIS;
			}
			else if (c >= 0xfd && c <= 0xfe)
			{
			    return KC_EUC;
			}
		    }
		    if ((c = *(++ptr)) == '\0')
			break;
		}
		ptr++;
		continue;
	    }

	    if (c >= 0xe0 && c <= 0xef)
	    {
		if ((c = *(++ptr)) == '\0')
		    break;
		if ((c >= 0x40 && c <= 0x7e) || (c >= 0x80 && c <= 0xa0))
		    return KC_SJIS;
		if (c >= 0xfd && c <= 0xfe)
		    return KC_EUC;
		if (c >= 0xa1 && c <= 0xfc)
		    expected = KC_EUCORSJIS;
	    }
	}
#if 1
	if (0xf0 <= c && c <= 0xfe)
	    return KC_EUC;
#endif
	ptr++;
    }

    ptr = str;
    c2 = 0;
    while ((c1 = *ptr++) != '\0')
    {
	if (((c2 > 0x80 && c2 < 0xa0) || (c2 >= 0xe0 && c2 < 0xfd)) &&
	    ((c1 >= 0x40 && c1 < 0x7f) || (c1 >= 0x80 && c1 < 0xfd)))
	{
	    sjis_c++, c1 = *ptr++;
	}
	c2 = c1;
	if (c1 == '\0')
	    break;
    }

/*
  if(sjis_c == 0)
  expected = KC_EUC;
  else {
*/
    {
	ptr = str, c2 = 0;
	while ((c1 = *ptr++) != '\0')
	{
	    if ((c2 > 0xa0 && c2 < 0xff) && (c1 > 0xa0 && c1 < 0xff))
	    {
		euc_c++, c1 = *ptr++;
	    }
	    c2 = c1;
	    if (c1 == '\0')
		break;
	}
	if (sjis_c > euc_c)
	    expected = KC_SJIS;
	else if (euc_c > 0)
	    expected = KC_EUC;
	else
	    expected = KC_ASCII;
    }

    /*
     * FIXME!! nnnnm 
     */
    if (g_utf8_validate (str, strlen (str), NULL))
	return KC_UTF8;
    else
	return expected;
}

const gchar *
japanese_detect_charset (const gchar * str)
{
    gint    detected;

#if defined (USE_GTK2) || defined (HAVE_ICONV)
    if (g_utf8_validate (str, strlen (str), NULL))
	return CHARSET_UTF8;
#endif

    detected = detect_kanji (str);

    switch (detected)
    {
      case KC_EUC:
	  return CHARSET_EUC_JP;
      case KC_JIS:
	  return CHARSET_JIS;
      case KC_SJIS:
	  return CHARSET_SJIS;
      case KC_UTF8:
	  return CHARSET_UTF8;
      default:
	  return CHARSET_ASCII;
    }

    return NULL;
}

#ifndef USE_GTK2
#ifndef HAVE_ICONV

/******************************************************************************
 *
 *   Convert methods
 *
 *   These codes are mostly taken from libjcode.
 *   Copy right (C) Kuramitsu Kimio, Tokyo Univ. 1996-97
 *
 *****************************************************************************/

#define CHAROUT(ch) *str2 = (unsigned char)(ch); str2++;

static unsigned char *
_to_jis (unsigned char *str)
{
    *str = (unsigned char) ESC;
    str++;
    *str = (unsigned char) '$';
    str++;
    *str = (unsigned char) 'B';
    str++;
    return str;
}

static unsigned char *
_to_ascii (unsigned char *str)
{
    *str = (unsigned char) ESC;
    str++;
    *str = (unsigned char) '(';
    str++;
    *str = (unsigned char) 'B';
    str++;
    return str;
}

/*-- shift JIS code to SJIS code -- */

static void
_jis_shift (int *p1, int *p2)
{
    unsigned char c1 = *p1;
    unsigned char c2 = *p2;
    int     rowOffset = c1 < 95 ? 112 : 176;
    int     cellOffset = c1 % 2 ? (c2 > 95 ? 32 : 31) : 126;

    *p1 = ((c1 + 1) >> 1) + rowOffset;
    *p2 += cellOffset;
}

/*-- shift SJIS code to JIS code -- */

static void
_sjis_shift (int *p1, int *p2)
{
    unsigned char c1 = *p1;
    unsigned char c2 = *p2;
    int     adjust = c2 < 159;
    int     rowOffset = c1 < 160 ? 112 : 176;
    int     cellOffset = adjust ? (c2 > 127 ? 32 : 31) : 126;

    *p1 = ((c1 - rowOffset) << 1) - adjust;
    *p2 -= cellOffset;
}

/* -- convert hankaku SJIS code to zenkaku -- */
#ifdef NO_HANKAKU_SJIS
#define HANKATA(a)  (a >= 161 && a <= 223)
#define ISMARU(a)   (a >= 202 && a <= 206)
#define ISNIGORI(a) ((a >= 182 && a <= 196) || (a >= 202 && a <= 206) || (a == 179))

static int stable[][2] = {
    {129, 66}, {129, 117}, {129, 118}, {129, 65}, {129, 69}, {131, 146}, {131,
									  64},
    {131, 66}, {131, 68}, {131, 70}, {131, 72}, {131, 131}, {131, 133}, {131,
									 135},
    {131, 98}, {129, 91}, {131, 65}, {131, 67}, {131, 69}, {131, 71}, {131,
								       73},
    {131, 74}, {131, 76}, {131, 78}, {131, 80}, {131, 82}, {131, 84}, {131,
								       86},
    {131, 88}, {131, 90}, {131, 92}, {131, 94}, {131, 96}, {131, 99}, {131,
								       101},
    {131, 103}, {131, 105}, {131, 106}, {131, 107}, {131, 108}, {131, 109},
    {131, 110}, {131, 113}, {131, 116}, {131, 119}, {131, 122}, {131, 125},
    {131, 126}, {131, 128}, {131, 129}, {131, 130}, {131, 132}, {131, 134},
    {131, 136}, {131, 137}, {131, 138}, {131, 139}, {131, 140}, {131, 141},
    {131, 143}, {131, 147}, {129, 74}, {129, 75}
};

static unsigned char *
_sjis_han2zen (unsigned char *str, int *p1, int *p2)
{
    register int c1, c2;

    c1 = (int) *str;
    str++;
    *p1 = stable[c1 - 161][0];
    *p2 = stable[c1 - 161][1];

    /*
     * 濁音、半濁音の処理 
     */
    c2 = (int) *str;
    if (c2 == 222 && ISNIGORI (c1))
    {
	if ((*p2 >= 74 && *p2 <= 103) || (*p2 >= 110 && *p2 <= 122))
	    (*p2)++;
	else if (*p1 == 131 && *p2 == 69)
	    *p2 = 148;
	str++;
    }

    if (c2 == 223 && ISMARU (c1) && (*p2 >= 110 && *p2 <= 122))
    {
	*p2 += 2;
	str++;
    }
    return str++;
}
#endif /* NO_HANKAKU_SJIS */

/* -- convert SJIS code -- */

#define SJIS1(A)    ((A >= 129 && A <= 159) || (A >= 224 && A <= 239))
#define SJIS2(A)    (A >= 64 && A <= 252)

static void
_shift2seven (const unsigned char *str, unsigned char *str2)
{
    int     p1, p2, esc_in = FALSE;

    while ((p1 = (int) *str) != '\0')
    {

	if (SJIS1 (p1))
	{
	    if ((p2 = (int) *(++str)) == '\0')
		break;
	    if (SJIS2 (p2))
	    {
		_sjis_shift (&p1, &p2);
		if (!esc_in)
		{
		    esc_in = TRUE;
		    str2 = _to_jis (str2);
		}
	    }
	    CHAROUT (p1);
	    CHAROUT (p2);
	    str++;
	    continue;
	}

#ifdef NO_HANKAKU_SJIS
	/*
	 * force convert hankaku SJIS code to zenkaku 
	 */
	if (HANKATA (p1))
	{
	    str = _sjis_han2zen (str, &p1, &p2);
	    _sjis_shift (&p1, &p2);
	    if (!esc_in)
	    {
		esc_in = TRUE;
		str2 = _to_jis (str2);
	    }
	    CHAROUT (p1);
	    CHAROUT (p2);
	    continue;
	}
#endif /* NO_HANKAKU_SJIS */

	if (esc_in)
	{
	    /*
	     * LF / CR の場合は、正常にエスケープアウトされる 
	     */
	    esc_in = FALSE;
	    str2 = _to_ascii (str2);
	}
	CHAROUT (p1);
	str++;
    }

    if (esc_in)
	str2 = _to_ascii (str2);
    *str2 = '\0';
}

/* -- convert SJIS to EUC -- */

static void
_shift2euc (const unsigned char *str, unsigned char *str2)
{
    int     p1, p2;

    while ((p1 = (int) *str) != '\0')
    {
	if (SJIS1 (p1))
	{
	    if ((p2 = (int) *(++str)) == '\0')
		break;
	    if (SJIS2 (p2))
	    {
		_sjis_shift (&p1, &p2);
		p1 += 128;
		p2 += 128;
	    }
	    CHAROUT (p1);
	    CHAROUT (p2);
	    str++;
	    continue;
	}

#ifdef NO_HANKAKU_SJIS
	/*
	 * force convert hankaku SJIS code to zenkaku 
	 */
	if (HANKATA (p1))
	{
	    str = _sjis_han2zen (str, &p1, &p2);
	    _sjis_shift (&p1, &p2);
	    p1 += 128;
	    p2 += 128;
	    CHAROUT (p1);
	    CHAROUT (p2);
	    continue;
	}
#endif /* NO_HANKAKU_SJIS */
	CHAROUT (p1);
	str++;
    }
    *str2 = '\0';
}

/* -- convert hankaku SJIS code -- */

static void
_shift_self (const unsigned char *str, unsigned char *str2)
{
    int     p1;
#ifdef NO_HANKAKU_SJIS
    int     p2;
#endif /* NO_HANKAKU_SJIS */

    while ((p1 = (int) *str) != '\0')
    {
#ifdef NO_HANKAKU_SJIS
	/*
	 * force convert hankaku SJIS code to zenkaku 
	 */
	if (HANKATA (p1))
	{
	    str = _sjis_han2zen (str, &p1, &p2);
	    CHAROUT (p1);
	    CHAROUT (p2);
	    continue;
	}
#endif /* NO_HANKAKU_SJIS */
	CHAROUT (p1);
	str++;
    }
    *str2 = '\0';
}

/* -- convert EUC to JIS -- */

#define ISEUC(A)    (A >= 161 && A <= 254)

static void
_euc2seven (const unsigned char *str, unsigned char *str2)
{
    int     p1, p2, esc_in = FALSE;

    while ((p1 = (int) *str) != '\0')
    {

	if (p1 == LF || p1 == CR)
	{
	    if (esc_in)
	    {
		esc_in = FALSE;
		str2 = _to_ascii (str2);
	    }
	    CHAROUT (p1);
	    str++;
	    continue;
	}

	if (ISEUC (p1))
	{
	    if ((p2 = (int) *(++str)) == '\0')
		break;
	    if (ISEUC (p2))
	    {

		if (!esc_in)
		{
		    esc_in = TRUE;
		    str2 = _to_jis (str2);
		}

		CHAROUT (p1 - 128);
		CHAROUT (p2 - 128);
		str++;
		continue;
	    }
	}

	if (esc_in)
	{
	    esc_in = FALSE;
	    str2 = _to_ascii (str2);
	}
	CHAROUT (p1);
	str++;
    }

    if (esc_in)
	str2 = _to_ascii (str2);
    *str2 = '\0';
}

/*-- convert EUC to SJIS -- */

static void
_euc2shift (const unsigned char *str, unsigned char *str2)
{
    int     p1, p2;

    while ((p1 = (int) *str) != '\0')
    {
	if (ISEUC (p1))
	{
	    if ((p2 = (int) *(++str)) == '\0')
		break;
	    if (ISEUC (p2))
	    {
		p1 -= 128;
		p2 -= 128;
		_jis_shift (&p1, &p2);
	    }
	    CHAROUT (p1);
	    CHAROUT (p2);
	    str++;
	    continue;
	}

	CHAROUT (p1);
	str++;
    }
    *str2 = '\0';
}

/* -- skip ESC sequence -- */

static const unsigned char *
_skip_esc (const unsigned char *str, int *esc_in)
{
    int     c;

    c = *(++str);
    if ((c == '$') || (c == '('))
	str++;
    if ((c == 'K') || (c == '$'))
	*esc_in = TRUE;
    else
	*esc_in = FALSE;

    if (*str != '\0')
	str++;
    return str;
}

/* -- convert JIS to SJIS -- */

static void
_seven2shift (const unsigned char *str, unsigned char *str2)
{
    int     p1, p2, esc_in = FALSE;

    while ((p1 = (int) *str) != '\0')
    {
	/*
	 * skip ESC sequence 
	 */
	if (p1 == ESC)
	{
	    str = _skip_esc (str, &esc_in);
	    continue;
	}

	if (p1 == LF || p1 == CR)
	{
	    if (esc_in)
		esc_in = FALSE;
	}

	if (esc_in)
	{			/* ISO-2022-JP code */
	    if ((p2 = (int) *(++str)) == '\0')
		break;

	    _jis_shift (&p1, &p2);

	    CHAROUT (p1);
	    CHAROUT (p2);
	}
	else
	{			/* ASCII code */
	    CHAROUT (p1);
	}
	str++;
    }
    *str2 = '\0';
}

/* -- convert JIS to EUC -- */

static void
_seven2euc (const unsigned char *str, unsigned char *str2)
{
    int     p1, esc_in = FALSE;

    while ((p1 = (int) *str) != '\0')
    {

	/*
	 * skip ESC sequence 
	 */
	if (p1 == ESC)
	{
	    str = _skip_esc (str, &esc_in);
	    continue;
	}

	if (p1 == LF || p1 == CR)
	{
	    if (esc_in)
		esc_in = FALSE;
	}

	if (esc_in)
	{			/* ISO-2022-JP code */
	    CHAROUT (p1 + 128);

	    if ((p1 = (int) *(++str)) == '\0')
		break;
	    CHAROUT (p1 + 128);
	}
	else
	{			/* ASCII code */
	    CHAROUT (p1);
	}
	str++;
    }
    *str2 = '\0';
}

/* wrapper */
static char *
toStringJIS (const char *str, int detected)
{
    unsigned char *buf, *ret;

    if (detected == KC_ASCII || detected == KC_JIS)
	return g_strdup (str);

    buf = g_malloc (strlen (str) * 2);
    if (!buf)
	return NULL;

    switch (detected)
    {
      case KC_SJIS:
	  _shift2seven (str, buf);
	  break;
      case KC_EUC:
	  _euc2seven (str, buf);
	  break;
      default:
	  g_free (buf);
	  return NULL;
	  break;
    }

    ret = g_strdup (buf);
    g_free (buf);
    return ret;
}

static char *
toStringEUC (const char *str, int detected)
{
    unsigned char *buf, *ret;

    if (detected == KC_ASCII || detected == KC_EUC)
	return g_strdup (str);

    buf = g_malloc (strlen (str) * 2);
    if (!buf)
	return NULL;

    switch (detected)
    {
      case KC_SJIS:
	  _shift2euc (str, buf);
	  break;
      case KC_JIS:
	  _seven2euc (str, buf);
	  break;
      default:
	  g_free (buf);
	  return NULL;
	  break;
    }

    ret = g_strdup (buf);
    g_free (buf);
    return ret;
}

static char *
toStringSJIS (const char *str, int detected)
{
    unsigned char *buf, *ret;

    if (detected == KC_ASCII)
	return g_strdup (str);

    buf = g_malloc (strlen (str) * 2);
    if (!buf)
	return NULL;

    switch (detected)
    {
      case KC_JIS:
	  _seven2shift (str, buf);
	  break;
      case KC_EUC:
	  _euc2shift (str, buf);
	  break;
      case KC_SJIS:
	  _shift_self (str, buf);
	  break;
      default:
	  g_free (buf);
	  return NULL;
	  break;
    }

    ret = g_strdup (buf);
    g_free (buf);
    return ret;
}

static  gint
japanese_charset_to_int (const gchar * charset)
{
    gint    i;

    if (!charset || !*charset)
	return -1;

    for (i = 0; i < charset_ascii_defs_num; i++)
	if (!strcasecmp (charset_ascii_defs[i], charset))
	    return KC_ASCII;
    for (i = 0; i < sizeof (charset_euc_defs) / sizeof (gchar *); i++)
	if (!strcasecmp (charset_euc_defs[i], charset))
	    return KC_EUC;
    for (i = 0; i < sizeof (charset_jis_defs) / sizeof (gchar *); i++)
	if (!strcasecmp (charset_jis_defs[i], charset))
	    return KC_JIS;
    for (i = 0; i < sizeof (charset_sjis_defs) / sizeof (gchar *); i++)
	if (!strcasecmp (charset_sjis_defs[i], charset))
	    return KC_SJIS;

    return -1;
}

gchar  *
japanese_conv (const gchar * src,
	       const gchar * src_codeset, const gchar * dest_codeset)
{
    gint    isrc, idest;

    g_return_val_if_fail (src, NULL);

    isrc = japanese_charset_to_int (src_codeset);
    if (isrc < 0)
	return NULL;

    idest = japanese_charset_to_int (dest_codeset);
    if (isrc < 0)
	return NULL;

    switch (idest)
    {
      case KC_ASCII:
	  return g_strdup (src);
	  break;
      case KC_EUC:
	  return toStringEUC (src, isrc);
	  break;
      case KC_JIS:
	  return toStringJIS (src, isrc);
	  break;
      case KC_SJIS:
	  return toStringSJIS (src, isrc);
	  break;
    }

    return NULL;
}

#endif /* HAVE_ICONV */
#endif /*  USE_GTK2 */
