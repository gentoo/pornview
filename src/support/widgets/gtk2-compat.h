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

#ifdef USE_GTK2
#  define gtk_object_class_add_signals(class, func, type)
#  define gtk_spin_button_set_shadow_type(widget, type)

#  define OBJECT_CLASS_SET_FINALIZE_FUNC(klass, func) \
      G_OBJECT_CLASS(klass)->finalize = func

#  define OBJECT_CLASS_FINALIZE_SUPER(parent_class, object) \
      if (G_OBJECT_CLASS(parent_class)->finalize) \
         G_OBJECT_CLASS(parent_class)->finalize (object);
#else /* USE_GTK2 */
#  define gtk_style_get_font(style) style->font

#  ifndef GTK_OBJECT_GET_CLASS
#     define GTK_OBJECT_GET_CLASS(object) GTK_OBJECT (object)->klass
#  endif

#  define OBJECT_CLASS_SET_FINALIZE_FUNC(klass, func) \
      GTK_OBJECT_CLASS(klass)->finalize = func

#  define OBJECT_CLASS_FINALIZE_SUPER(parent_class, object)\
      if (GTK_OBJECT_CLASS(parent_class)->finalize) \
         GTK_OBJECT_CLASS(parent_class)->finalize (object);

#  ifndef GTK_WIDGET_GET_CLASS
#     define GTK_WIDGET_GET_CLASS(widget) GTK_WIDGET_CLASS (GTK_OBJECT (widget)->klass)
#  endif

#  ifndef GTK_CLASS_TYPE
#     define GTK_CLASS_TYPE(object_class) object_class->type
#  endif
#endif

#endif /* __GTK2_COMPAT_H__ */
