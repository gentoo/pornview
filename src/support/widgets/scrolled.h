/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 *  These codes are mostly taken from Another X image viewer.
 *
 *  Another X image viewer Author:
 *     David Ramboz <dramboz@users.sourceforge.net>
 */

#ifndef __SCROLLED_H__
#define __SCROLLED_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#define SCROLLED_TYPE             (scrolled_get_type())
#define SCROLLED(obj)             GTK_CHECK_CAST( (obj), SCROLLED_TYPE, Scrolled)
#define SCROLLED_X(scrolled, x) (-SCROLLED(scrolled)->x_offset + (x))
#define SCROLLED_Y(scrolled, y) (-SCROLLED(scrolled)->y_offset + (y))
#define SCROLLED_VX(scrolled, x) (SCROLLED(scrolled)->x_offset + (x))
#define SCROLLED_VY(scrolled, y) (SCROLLED(scrolled)->y_offset + (y))

typedef struct _Scrolled Scrolled;
typedef struct _ScrolledClass ScrolledClass;

struct _Scrolled
{
    GtkContainer    container;

    int             x_offset;
    int             y_offset;

    GtkAdjustment  *h_adjustment;
    GtkAdjustment  *v_adjustment;

    GdkGC          *copy_gc;
    guint           freeze_count;
};

struct _ScrolledClass
{
    GtkContainerClass parent_class;

    void            (*set_scroll_adjustments) (GtkWidget * widget,
					       GtkAdjustment * hadjustment,
					       GtkAdjustment * vadjustment);
    void            (*adjust_adjustments) (Scrolled * scrolled);
};

GtkType         scrolled_get_type (void);

void            scrolled_realize (Scrolled * scrolled);
void            scrolled_unrealize (Scrolled * scrolled);
void            scrolled_freeze (Scrolled * scrolled);
void            scrolled_thawn (Scrolled * scrolled);

#endif /* __SCROLLED_H__ */
