/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gedo-paned.h"

guint
gedo_paned_which_hidden(GedoPaned *paned)
{
    if (!gtk_widget_get_visible (gtk_paned_get_child1(paned))
	&& gtk_widget_get_visible (gtk_paned_get_child2(paned)))
    {
	return 1;

    }
    else if (gtk_widget_get_visible (gtk_paned_get_child1(paned))
	     && !gtk_widget_get_visible (gtk_paned_get_child2(paned)))
    {
	return 2;

    }
    else
    {
	return 0;
    }
}
