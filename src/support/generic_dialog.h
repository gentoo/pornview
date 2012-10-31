/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GQview.
 * GQview author: John Ellis
 */

#ifndef __GENERIC_DIALOG_H__
#define __GENERIC_DIALOG_H__

#define GENERIC_DIALOG(gd) ((GenericDialog *)gd)

typedef struct _GenericDialog GenericDialog;

struct _GenericDialog
{
    GtkWidget      *dialog;	/* window */
    GtkWidget      *vbox;	/* place to add widgets */

    GtkWidget      *hbox;	/* button hbox */

    gint            auto_close;

    void            (*cb_default) (GenericDialog *, gpointer);
    void            (*cb_cancel) (GenericDialog *, gpointer);
    gpointer        data;

    /*
     * private 
     */
    GtkWidget      *cancel_button;
};

GenericDialog  *generic_dialog_new (const gchar * title,
				    const gchar * message,
				    const gchar * wmclass,
				    const gchar * wmsubclass, gint auto_close,
				    void (*cb_cancel) (GenericDialog *,
						       gpointer),
				    gpointer data);

void            generic_dialog_close (GenericDialog * gd);

GtkWidget      *generic_dialog_add (GenericDialog * gd, const gchar * text,
				    void (*cb_func) (GenericDialog *,
						     gpointer),
				    gint is_default);

void            generic_dialog_attach_default (GenericDialog * gd,
					       GtkWidget * widget);

void            generic_dialog_setup (GenericDialog * gd,
				      const gchar * title,
				      const gchar * message,
				      const gchar * wmclass,
				      const gchar * wmsubclass,
				      gint auto_close,
				      void (*cb_cancel) (GenericDialog *,
							 gpointer),
				      gpointer data);

#endif /* __GENERIC_DIALOG_H__ */
