/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __VIEWTYPE_H__
#define __VIEWTYPE_H__

typedef enum
{
    VIEWTYPE_IMAGEVIEW,
    VIEWTYPE_VIDEOPLAY,
    VIEWTYPE_COMMENT_DATA,
    VIEWTYPE_COMMENT_NOTE
}
ViewType;

void            viewtype_set (ViewType type);
ViewType        viewtype_get (void);
void            viewtype_set_force (ViewType type);

#endif /* __VIEWTYPE_H__ */
