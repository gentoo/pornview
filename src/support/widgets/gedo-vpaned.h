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

#ifndef __GEDO_VPANED_H__
#define __GEDO_VPANED_H__

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

#define GEDO_VPANED(obj)          GTK_CHECK_CAST (obj, gedo_vpaned_get_type (), GedoVPaned)
#define GEDO_VPANED_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gedo_vpaned_get_type (), GedoVPanedClass)
#define GEDO_IS_VPANED(obj)       GTK_CHECK_TYPE (obj, gedo_vpaned_get_type ())

    typedef struct _GedoVPaned GedoVPaned;
    typedef struct _GedoVPanedClass GedoVPanedClass;

    struct _GedoVPaned
    {
	GedoPaned       paned;
    };

    struct _GedoVPanedClass
    {
	GedoPanedClass  parent_class;
    };

    guint           gedo_vpaned_get_type (void);
    GtkWidget      *gedo_vpaned_new (void);

#else				/* USE_NORMAL_PANED */

#include <gtk/gtkvpaned.h>

#define GedoVPaned GtkVPaned
#define gedo_vpaned_new() gtk_vpaned_new()

#endif				/* USE_NORMAL_PANED */

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GEDO_VPANED_H__ */
