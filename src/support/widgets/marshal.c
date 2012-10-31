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

#include "marshal.h"

#if (GTK_MAJOR_VERSION >= 2)

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_int(v)      g_value_get_int (v)
#else /* !G_ENABLE_DEBUG */
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#endif /* !G_ENABLE_DEBUG */

void
gtk_marshal_INT__INT_INT (GClosure * closure,
			  GValue * return_value,
			  guint n_param_values,
			  const GValue * param_values,
			  gpointer invocation_hint, gpointer marshal_data)
{
    typedef gint (*GMarshalFunc_INT__INT_INT) (gpointer data1,
					       gint arg_1,
					       gint arg_2, gpointer data2);
    register GMarshalFunc_INT__INT_INT callback;
    register GCClosure *cc = (GCClosure *) closure;
    register gpointer data1, data2;
    gint    v_return;

    g_return_if_fail (return_value != NULL);
    g_return_if_fail (n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA (closure))
    {
	data1 = closure->data;
	data2 = g_value_peek_pointer (param_values + 0);
    }
    else
    {
	data1 = g_value_peek_pointer (param_values + 0);
	data2 = closure->data;
    }
    callback =
	(GMarshalFunc_INT__INT_INT) (marshal_data ? marshal_data : cc->
				     callback);

    v_return = callback (data1,
			 g_marshal_value_peek_int (param_values + 1),
			 g_marshal_value_peek_int (param_values + 2), data2);

    g_value_set_int (return_value, v_return);
}

#else /* (GTK_MAJOR_VERSION >= 2) */

typedef gint (*GtkSignal_INT__INT_INT) (GtkObject * object,
					gint arg1,
					gint arg2, gpointer user_data);

void
gtk_marshal_INT__INT_INT (GtkObject * object,
			  GtkSignalFunc func,
			  gpointer func_data, GtkArg * args)
{
    GtkSignal_INT__INT_INT rfunc;
    gint   *return_val;
    return_val = GTK_RETLOC_INT (args[2]);
    rfunc = (GtkSignal_INT__INT_INT) func;
    *return_val = (*rfunc) (object,
			    GTK_VALUE_INT (args[0]),
			    GTK_VALUE_INT (args[1]), func_data);
}

#endif /* (GTK_MAJOR_VERSION >= 2) */
