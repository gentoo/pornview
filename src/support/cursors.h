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

#ifndef __CURSORS_H__
#define __CURSORS_H__

typedef enum
{
    CURSOR_HAND_OPEN,
    CURSOR_HAND_CLOSED,
    CURSOR_VOID,
    CURSOR_NUM_CURSORS
}
CursorType;

GdkCursor      *cursor_get (GdkWindow * window, CursorType type);

#endif /* __CURSORS_H__ */
