/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

gchar          *menu_translate (const gchar * path, gpointer data);
gchar          *remove_trailing_slash (const gchar * path);
gchar          *boolean_to_text (gboolean boolval);
gboolean        text_to_boolean (gchar * text);
gboolean        string_is_ascii_graphable (const char *string);
gboolean        string_is_ascii_printable (const char *string);

#endif /* __STRING_UTILS_H__ */
