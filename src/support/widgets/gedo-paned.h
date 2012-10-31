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
#include <gtk/gtkcontainer.h>

#ifdef __cplusplus
extern          "C"
{
#endif				/* __cplusplus */

#ifndef USE_NORMAL_PANED

#define GEDO_TYPE_PANED                  (gedo_paned_get_type ())
#define GEDO_PANED(obj)                  (GTK_CHECK_CAST ((obj), GEDO_TYPE_PANED, GedoPaned))
#define GEDO_PANED_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GEDO_TYPE_PANED, GedoPanedClass))
#define GEDO_IS_PANED(obj)               (GTK_CHECK_TYPE ((obj), GEDO_TYPE_PANED))
#define GEDO_IS_PANED_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GEDO_TYPE_PANED))

    typedef struct _GedoPaned GedoPaned;
    typedef struct _GedoPanedClass GedoPanedClass;

    struct _GedoPaned
    {
	GtkContainer    container;

	GtkWidget      *child1;
	GtkWidget      *child2;

	GdkWindow      *handle;
	GdkGC          *xor_gc;

	/*
	 * < public >
	 */
	guint16         gutter_size;

	/*
	 * < private >
	 */
	gint            child1_size;
	gint            last_allocation;
	gint            min_position;
	gint            max_position;

	guint           position_set:1;
	guint           in_drag:1;
	guint           child1_shrink:1;
	guint           child1_resize:1;
	guint           child2_shrink:1;
	guint           child2_resize:1;

	gint16          handle_xpos;
	gint16          handle_ypos;

	/*
	 * whether it is an horizontal or a vertical paned. 
	 */
	guint           horizontal:1;

	/*
	 * minimal sizes for child1 and child2. (default value : 0) 
	 */
	gint            child1_minsize;
	gint            child2_minsize;

	/*
	 * whether the minimal size option is enabled or not. 
	 */
	guint           child1_use_minsize:1;
	guint           child2_use_minsize:1;

	/*
	 * stores what child is hiden, if any: 
	 * * 0   : no child collapsed.
	 * * 1,2 : index of the collapsed child. 
	 */
	guint           child_hidden:2;
    };

    struct _GedoPanedClass
    {
	GtkContainerClass parent_class;

	void            (*xor_line) (GedoPaned *);
    };


    GtkType         gedo_paned_get_type (void);
    void            gedo_paned_add1 (GedoPaned * paned, GtkWidget * child);
    void            gedo_paned_add2 (GedoPaned * paned, GtkWidget * child);
    void            gedo_paned_pack1 (GedoPaned * paned,
				      GtkWidget * child,
				      gboolean resize, gboolean shrink);
    void            gedo_paned_pack2 (GedoPaned * paned,
				      GtkWidget * child,
				      gboolean resize, gboolean shrink);
    void            gedo_paned_set_position (GedoPaned * paned,
					     gint position);
    gint            gedo_paned_get_position (GedoPaned * paned);

    void            gedo_paned_set_gutter_size (GedoPaned * paned,
						guint16 size);

    void            gedo_paned_xor_line (GedoPaned * paned);

/* Set a minimal size for a child. Unset the collapse option if setted. */
    void            gedo_paned_child1_use_minsize (GedoPaned * paned,
						   gboolean use_minsize,
						   gint minsize);
    void            gedo_paned_child2_use_minsize (GedoPaned * paned,
						   gboolean use_minsize,
						   gint minsize);

    void            gedo_paned_hide_child1 (GedoPaned * paned);
    void            gedo_paned_hide_child2 (GedoPaned * paned);
    void            gedo_paned_split (GedoPaned * paned);
    guint           gedo_paned_which_hidden (GedoPaned * paned);

/* Internal function */
    void            gedo_paned_compute_position (GedoPaned * paned,
						 gint allocation,
						 gint child1_req,
						 gint child2_req);

#define gray50_width 2
#define gray50_height 2
    extern char     gray50_bits[];

#else				/* USE_NORMAL_PANED */

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

#endif				/* USE_NORMAL_PANED */


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GEDO_PANED_H__ */
