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

#ifndef GTK_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#endif /* GTK_DISABLE_DEPRECATED */

#include "gtkcellrendererpixmap.h"

#if (GTK_MAJOR_VERSION >= 2)

#include <stdlib.h>
#include "intl.h"

static void gtk_cell_renderer_pixmap_get_property (GObject * object,
						   guint param_id,
						   GValue * value,
						   GParamSpec * pspec);
static void gtk_cell_renderer_pixmap_set_property (GObject * object,
						   guint param_id,
						   const GValue * value,
						   GParamSpec * pspec);
static void gtk_cell_renderer_pixmap_init (GtkCellRendererPixmap * celltext);
static void gtk_cell_renderer_pixmap_class_init (GtkCellRendererPixmapClass *
						 class);
static void gtk_cell_renderer_pixmap_get_size (GtkCellRenderer * cell,
					       GtkWidget * widget,
					       GdkRectangle * rectangle,
					       gint * x_offset,
					       gint * y_offset, gint * width,
					       gint * height);
static void gtk_cell_renderer_pixmap_render (GtkCellRenderer * cell,
					     GdkWindow * window,
					     GtkWidget * widget,
					     GdkRectangle * background_area,
					     GdkRectangle * cell_area,
					     GdkRectangle * expose_area,
					     guint flags);

enum
{
    PROP_ZERO,
    PROP_PIXMAP,
    PROP_MASK,
    PROP_PIXMAP_EXPANDER_OPEN,
    PROP_MASK_EXPANDER_OPEN,
    PROP_PIXMAP_EXPANDER_CLOSED,
    PROP_MASK_EXPANDER_CLOSED
};

GType
gtk_cell_renderer_pixmap_get_type (void)
{
    static GType cell_pixmap_type = 0;

    if (!cell_pixmap_type)
    {
	static const GTypeInfo cell_pixmap_info = {
	    sizeof (GtkCellRendererPixmapClass),
	    NULL,		/* base_init */
	    NULL,		/* base_finalize */
	    (GClassInitFunc) gtk_cell_renderer_pixmap_class_init,
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof (GtkCellRendererPixmap),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) gtk_cell_renderer_pixmap_init,
	};

	cell_pixmap_type = g_type_register_static (GTK_TYPE_CELL_RENDERER,
						   "GtkCellRendererPixmap",
						   &cell_pixmap_info, 0);
    }

    return cell_pixmap_type;
}

static void
gtk_cell_renderer_pixmap_init (GtkCellRendererPixmap * cellpixmap)
{
}

static void
gtk_cell_renderer_pixmap_class_init (GtkCellRendererPixmapClass * class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

    object_class->get_property = gtk_cell_renderer_pixmap_get_property;
    object_class->set_property = gtk_cell_renderer_pixmap_set_property;

    cell_class->get_size = gtk_cell_renderer_pixmap_get_size;
    cell_class->render = gtk_cell_renderer_pixmap_render;

    g_object_class_install_property (object_class,
				     PROP_PIXMAP,
				     g_param_spec_object ("pixmap",
							  _("Pixmap Object"),
							  _
							  ("The pixmap to render."),
							  GDK_TYPE_PIXMAP,
							  G_PARAM_READABLE |
							  G_PARAM_WRITABLE));

    g_object_class_install_property (object_class,
				     PROP_MASK,
				     g_param_spec_object ("mask",
							  _("Mask Object"),
							  _
							  ("The mask to render."),
							  GDK_TYPE_PIXMAP,
							  G_PARAM_READABLE |
							  G_PARAM_WRITABLE));

    g_object_class_install_property (object_class,
				     PROP_PIXMAP_EXPANDER_OPEN,
				     g_param_spec_object
				     ("pixmap_expander_open",
				      _("Pixmap Expander Open"),
				      _("Pixmap for open expander."),
				      GDK_TYPE_PIXMAP,
				      G_PARAM_READABLE | G_PARAM_WRITABLE));

    g_object_class_install_property (object_class,
				     PROP_MASK_EXPANDER_OPEN,
				     g_param_spec_object
				     ("mask_expander_open",
				      _("Mask Expander Open"),
				      _("Mask for open expander."),
				      GDK_TYPE_PIXMAP,
				      G_PARAM_READABLE | G_PARAM_WRITABLE));

    g_object_class_install_property (object_class,
				     PROP_PIXMAP_EXPANDER_CLOSED,
				     g_param_spec_object
				     ("pixmap_expander_closed",
				      _("Pixmap Expander Closed"),
				      _("Pixmap for closed expander."),
				      GDK_TYPE_PIXMAP,
				      G_PARAM_READABLE | G_PARAM_WRITABLE));

    g_object_class_install_property (object_class,
				     PROP_MASK_EXPANDER_CLOSED,
				     g_param_spec_object
				     ("mask_expander_closed",
				      _("Mask Expander Closed"),
				      _("Mask for closed expander."),
				      GDK_TYPE_PIXMAP,
				      G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
gtk_cell_renderer_pixmap_get_property (GObject * object,
				       guint param_id,
				       GValue * value, GParamSpec * pspec)
{
    GtkCellRendererPixmap *cellpixmap = GTK_CELL_RENDERER_PIXMAP (object);

    switch (param_id)
    {
      case PROP_PIXMAP:
	  g_value_set_object (value,
			      cellpixmap->pixmap
			      ? G_OBJECT (cellpixmap->pixmap) : NULL);
	  break;
      case PROP_MASK:
	  g_value_set_object (value,
			      cellpixmap->mask
			      ? G_OBJECT (cellpixmap->mask) : NULL);
	  break;
      case PROP_PIXMAP_EXPANDER_OPEN:
	  g_value_set_object (value,
			      cellpixmap->pixmap_expander_open
			      ? G_OBJECT (cellpixmap->
					  pixmap_expander_open) : NULL);
	  break;
      case PROP_MASK_EXPANDER_OPEN:
	  g_value_set_object (value,
			      cellpixmap->mask_expander_open
			      ? G_OBJECT (cellpixmap->
					  mask_expander_open) : NULL);
	  break;
      case PROP_PIXMAP_EXPANDER_CLOSED:
	  g_value_set_object (value,
			      cellpixmap->pixmap_expander_closed
			      ? G_OBJECT (cellpixmap->
					  pixmap_expander_closed) : NULL);
	  break;
      case PROP_MASK_EXPANDER_CLOSED:
	  g_value_set_object (value,
			      cellpixmap->mask_expander_closed
			      ? G_OBJECT (cellpixmap->
					  mask_expander_closed) : NULL);
	  break;
      default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	  break;
    }
}

static void
gtk_cell_renderer_pixmap_set_property (GObject * object,
				       guint param_id,
				       const GValue * value,
				       GParamSpec * pspec)
{
    GdkPixmap *pixmap;
    GdkBitmap *mask;
    GtkCellRendererPixmap *cellpixmap = GTK_CELL_RENDERER_PIXMAP (object);

    switch (param_id)
    {
      case PROP_PIXMAP:
	  pixmap = (GdkPixmap *) g_value_get_object (value);
	  if (pixmap)
	      g_object_ref (G_OBJECT (pixmap));
	  if (cellpixmap->pixmap)
	      g_object_unref (G_OBJECT (cellpixmap->pixmap));
	  cellpixmap->pixmap = pixmap;
	  break;
      case PROP_MASK:
	  mask = (GdkBitmap *) g_value_get_object (value);
	  if (mask)
	      g_object_ref (G_OBJECT (mask));
	  if (cellpixmap->mask)
	      g_object_unref (G_OBJECT (cellpixmap->mask));
	  cellpixmap->mask = mask;
	  break;
      case PROP_PIXMAP_EXPANDER_OPEN:
	  pixmap = (GdkPixmap *) g_value_get_object (value);
	  if (pixmap)
	      g_object_ref (G_OBJECT (pixmap));
	  if (cellpixmap->pixmap_expander_open)
	      g_object_unref (G_OBJECT (cellpixmap->pixmap_expander_open));
	  cellpixmap->pixmap_expander_open = pixmap;
	  break;
      case PROP_MASK_EXPANDER_OPEN:
	  mask = (GdkBitmap *) g_value_get_object (value);
	  if (mask)
	      g_object_ref (G_OBJECT (mask));
	  if (cellpixmap->mask_expander_open)
	      g_object_unref (G_OBJECT (cellpixmap->mask_expander_open));
	  cellpixmap->mask_expander_open = mask;
	  break;
      case PROP_PIXMAP_EXPANDER_CLOSED:
	  pixmap = (GdkPixmap *) g_value_get_object (value);
	  if (pixmap)
	      g_object_ref (G_OBJECT (pixmap));
	  if (cellpixmap->pixmap_expander_closed)
	      g_object_unref (G_OBJECT (cellpixmap->pixmap_expander_closed));
	  cellpixmap->pixmap_expander_closed = pixmap;
	  break;
      case PROP_MASK_EXPANDER_CLOSED:
	  mask = (GdkBitmap *) g_value_get_object (value);
	  if (mask)
	      g_object_ref (G_OBJECT (mask));
	  if (cellpixmap->mask_expander_closed)
	      g_object_unref (G_OBJECT (cellpixmap->mask_expander_closed));
	  cellpixmap->mask_expander_closed = mask;
	  break;
      default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	  break;
    }
}

GtkCellRenderer *
gtk_cell_renderer_pixmap_new (void)
{
    return
	GTK_CELL_RENDERER (g_object_new
			   (gtk_cell_renderer_pixmap_get_type (), NULL));
}

static void
gtk_cell_renderer_pixmap_get_size (GtkCellRenderer * cell,
				   GtkWidget * widget,
				   GdkRectangle * cell_area,
				   gint * x_offset,
				   gint * y_offset,
				   gint * width, gint * height)
{
    GtkCellRendererPixmap *cellpixmap = (GtkCellRendererPixmap *) cell;
    gint    pixmap_width = 0;
    gint    pixmap_height = 0;
    gint    calc_width;
    gint    calc_height;

    if (cellpixmap->pixmap)
    {
	gdk_drawable_get_size (cellpixmap->pixmap, &pixmap_width,
			       &pixmap_height);
    }
    if (cellpixmap->pixmap_expander_open)
    {
	gint    w, h;
	gdk_drawable_get_size (cellpixmap->pixmap_expander_open, &w, &h);
	pixmap_width = MAX (pixmap_width, w);
	pixmap_height = MAX (pixmap_height, h);
    }
    if (cellpixmap->pixmap_expander_closed)
    {
	gint    w, h;
	gdk_drawable_get_size (cellpixmap->pixmap_expander_closed, &w, &h);
	pixmap_width = MAX (pixmap_width, w);
	pixmap_height = MAX (pixmap_height, h);
    }

    calc_width =
	(gint) GTK_CELL_RENDERER (cellpixmap)->xpad * 2 + pixmap_width;
    calc_height =
	(gint) GTK_CELL_RENDERER (cellpixmap)->ypad * 2 + pixmap_height;

    if (x_offset)
	*x_offset = 0;
    if (y_offset)
	*y_offset = 0;

    if (cell_area && pixmap_width > 0 && pixmap_height > 0)
    {
	if (x_offset)
	{
	    *x_offset = GTK_CELL_RENDERER (cellpixmap)->xalign
		* (cell_area->width - calc_width -
		   (2 * GTK_CELL_RENDERER (cellpixmap)->xpad));
	    *x_offset =
		MAX (*x_offset, 0) + GTK_CELL_RENDERER (cellpixmap)->xpad;
	}
	if (y_offset)
	{
	    *y_offset = GTK_CELL_RENDERER (cellpixmap)->yalign
		* (cell_area->height - calc_height -
		   (2 * GTK_CELL_RENDERER (cellpixmap)->ypad));
	    *y_offset =
		MAX (*y_offset, 0) + GTK_CELL_RENDERER (cellpixmap)->ypad;
	}
    }

    if (width)
	*width = calc_width;

    if (height)
	*height = calc_height;
}

static void
gtk_cell_renderer_pixmap_render (GtkCellRenderer * cell,
				 GdkWindow * window,
				 GtkWidget * widget,
				 GdkRectangle * background_area,
				 GdkRectangle * cell_area,
				 GdkRectangle * expose_area, guint flags)
{
    GtkCellRendererPixmap *cellpixmap = (GtkCellRendererPixmap *) cell;
    GdkPixmap *pixmap;
    GdkBitmap *mask;
    GdkRectangle pix_rect;
    GdkRectangle draw_rect;
    GdkGC  *gc;

    pixmap = cellpixmap->pixmap;
    mask = cellpixmap->mask;
    if (cell->is_expander)
    {
	if (cell->is_expanded && cellpixmap->pixmap_expander_open != NULL)
	{
	    pixmap = cellpixmap->pixmap_expander_open;
	    mask = cellpixmap->mask_expander_open;
	}
	else if (!cell->is_expanded &&
		 cellpixmap->pixmap_expander_closed != NULL)
	{
	    pixmap = cellpixmap->pixmap_expander_closed;
	    mask = cellpixmap->mask_expander_closed;
	}
    }

    if (!pixmap)
	return;

    gtk_cell_renderer_pixmap_get_size (cell, widget, cell_area,
				       &pix_rect.x,
				       &pix_rect.y,
				       &pix_rect.width, &pix_rect.height);

    pix_rect.x += cell_area->x;
    pix_rect.y += cell_area->y;
    pix_rect.width -= cell->xpad * 2;
    pix_rect.height -= cell->ypad * 2;

    if (!gdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect))
	return;

    gc = widget->style->fg_gc[GTK_STATE_NORMAL];

    if (mask)
    {
	gdk_gc_set_clip_mask (gc, mask);
	gdk_gc_set_clip_origin (gc, pix_rect.x, pix_rect.y);
    }

    gdk_draw_pixmap (window,
		     gc,
		     pixmap,
		     draw_rect.x - pix_rect.x,
		     draw_rect.y - pix_rect.y,
		     draw_rect.x,
		     draw_rect.y, draw_rect.width, draw_rect.height);

    if (mask)
    {
	gdk_gc_set_clip_mask (gc, NULL);
	gdk_gc_set_clip_origin (gc, 0, 0);
    }
}

#endif /* (GTK_MAJOR_VERSION >= 2) */
