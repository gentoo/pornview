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

#ifndef __GEDO_HPANED_H__
#define __GEDO_HPANED_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdk.h>
#include "gedo-paned.h"

#ifdef __cplusplus
extern          "C"
{
#endif				/* __cplusplus */

#ifndef USE_NORMAL_PANED

#define GEDO_HPANED(obj)          GTK_CHECK_CAST (obj, gedo_hpaned_get_type (), GedoHPaned)
#define GEDO_HPANED_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gedo_hpaned_get_type (), GedoHPanedClass)
#define GEDO_IS_HPANED(obj)       GTK_CHECK_TYPE (obj, gedo_hpaned_get_type ())

    typedef struct _GedoHPaned GedoHPaned;
    typedef struct _GedoHPanedClass GedoHPanedClass;

    struct _GedoHPaned
    {
	GedoPaned       paned;
    };

    struct _GedoHPanedClass
    {
	GedoPanedClass  parent_class;
    };

    guint           gedo_hpaned_get_type (void);
    GtkWidget      *gedo_hpaned_new (void);

#else				/* USE_NORMAL_PANED */

#include <gtk/gtkhpaned.h>

#define GedoHPaned GtkHPaned
#define gedo_hpaned_new() gtk_hpaned_new()

#endif				/* USE_NORMAL_PANED */

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GEDO_HPANED_H__ */
