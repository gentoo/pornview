/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2003                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

void            plugin_init (void);
void            plugin_free (void);

void            plugin_create_search_dir_list (void);
GList          *plugin_get_search_dir_list (void);

#endif /* __PLUGIN_H__ */
