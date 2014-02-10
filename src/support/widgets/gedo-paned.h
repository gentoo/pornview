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

#ifndef __GEDO_PANED_H__
#define __GEDO_PANED_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk/gtkvpaned.h>
#include <gtk/gtkhpaned.h>


#ifdef __cplusplus
extern          "C"
{
#endif				/* __cplusplus */

#include <gtk/gtkpaned.h>

#define GedoPaned GtkPaned
#define GEDO_PANED(paned)                    GTK_PANED(paned)
#define gedo_paned_add1(paned, widget)       gtk_paned_add1(paned, widget)
#define gedo_paned_add2(paned, widget)       gtk_paned_add2(paned, widget)
#define gedo_paned_set_position(paned, size) gtk_paned_set_position(paned, size)
#define gedo_paned_get_position(paned)       gtk_paned_get_position(paned)
#define gedo_paned_split(paned) \
{ \
   gtk_widget_show (paned->child1); \
   gtk_widget_show (paned->child2); \
}
#define gedo_paned_hide_child1(paned)        gtk_widget_hide (paned->child1)
#define gedo_paned_hide_child2(paned)        gtk_widget_hide (paned->child2)

    guint           gedo_paned_which_hidden (GedoPaned * paned);

#define GedoHPaned GtkHPaned
#define gedo_hpaned_new() gtk_hpaned_new()

#define GedoVPaned GtkVPaned
#define gedo_vpaned_new() gtk_vpaned_new()

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GEDO_PANED_H__ */
