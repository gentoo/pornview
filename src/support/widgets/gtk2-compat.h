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

#ifndef __GTK2_COMPAT_H__
#define __GTK2_COMPAT_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#  define gtk_object_class_add_signals(class, func, type)
#  define gtk_spin_button_set_shadow_type(widget, type)

#  define OBJECT_CLASS_SET_FINALIZE_FUNC(klass, func) \
      G_OBJECT_CLASS(klass)->finalize = func

#  define OBJECT_CLASS_FINALIZE_SUPER(parent_class, object) \
      if (G_OBJECT_CLASS(parent_class)->finalize) \
         G_OBJECT_CLASS(parent_class)->finalize (object);

#endif /* __GTK2_COMPAT_H__ */
