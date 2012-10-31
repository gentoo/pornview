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

#ifndef __MARSHAL_H__
#define __MARSHAL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#if (GTK_MAJOR_VERSION >= 2)

#include	<glib-object.h>

extern void     gtk_marshal_INT__INT_INT (GClosure * closure,
					  GValue * return_value,
					  guint n_param_values,
					  const GValue * param_values,
					  gpointer invocation_hint,
					  gpointer marshal_data);

#else /* (GTK_MAJOR_VERSION >= 2) */

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>

void            gtk_marshal_INT__INT_INT (GtkObject * object,
					  GtkSignalFunc func,
					  gpointer func_data, GtkArg * args);

#endif /* (GTK_MAJOR_VERSION >= 2) */

#endif /* __MARSHAL_H__ */
