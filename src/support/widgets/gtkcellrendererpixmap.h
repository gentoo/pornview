/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 *  These codes are based on gtk/gtkcellrendererpixbuf.h in gtk+-2.0.6
 *  Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 */

#ifndef __GTK_CELL_RENDERER_PIXMAP_H__
#define __GTK_CELL_RENDERER_PIXMAP_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#if (GTK_MAJOR_VERSION >= 2)

#include <gtk/gtkcellrenderer.h>

#ifdef __cplusplus
extern          "C"
{
#endif				/* __cplusplus */


#define GTK_TYPE_CELL_RENDERER_PIXMAP			    (gtk_cell_renderer_pixmap_get_type ())
#define GTK_CELL_RENDERER_PIXMAP(obj)			    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CELL_RENDERER_PIXMAP, GtkCellRendererPixmap))
#define GTK_CELL_RENDERER_PIXMAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CELL_RENDERER_PIXMAP, GtkCellRendererPixmapClass))
#define GTK_IS_CELL_RENDERER_PIXMAP(obj)		    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CELL_RENDERER_PIXMAP))
#define GTK_IS_CELL_RENDERER_PIXMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CELL_RENDERER_PIXMAP))
#define GTK_CELL_RENDERER_PIXMAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CELL_RENDERER_PIXMAP, GtkCellRendererPixmapClass))

    typedef struct GtkCellRendererPixmap_Tag GtkCellRendererPixmap;
    typedef struct GtkCellRendererPixmapClass_Tag GtkCellRendererPixmapClass;

    struct GtkCellRendererPixmap_Tag
    {
	GtkCellRenderer parent;

	GdkPixmap      *pixmap;
	GdkBitmap      *mask;
	GdkPixmap      *pixmap_expander_open;
	GdkBitmap      *mask_expander_open;
	GdkPixmap      *pixmap_expander_closed;
	GdkBitmap      *mask_expander_closed;
    };

    struct GtkCellRendererPixmapClass_Tag
    {
	GtkCellRendererClass parent_class;

	/*
	 * Padding for future expansion
	 */
	void            (*_gtk_reserved1) (void);
	void            (*_gtk_reserved2) (void);
	void            (*_gtk_reserved3) (void);
	void            (*_gtk_reserved4) (void);
    };

    GType         gtk_cell_renderer_pixmap_get_type (void);
    GtkCellRenderer *gtk_cell_renderer_pixmap_new (void);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GTK_CELL_RENDERER_PIXMAP_H__ */

#endif				/* (GTK_MAJOR_VERSION >= 2) */
