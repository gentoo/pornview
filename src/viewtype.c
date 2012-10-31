/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "viewtype.h"
#include "browser.h"
#include "imageview.h"
#include "videoplay.h"

void
viewtype_set (ViewType type)
{
    gint    page;

    page = gtk_notebook_get_current_page (GTK_NOTEBOOK (BROWSER_VIEW));

#ifdef ENABLE_MOVIE
    if (type == VIEWTYPE_IMAGEVIEW && VIDEOPLAY_IS_HIDE == FALSE)
    {
	gtk_widget_show (gtk_notebook_get_nth_page
			 (GTK_NOTEBOOK (BROWSER_VIEW), 0));
	gtk_widget_hide (gtk_notebook_get_nth_page
			 (GTK_NOTEBOOK (BROWSER_VIEW), 1));

	IMAGEVIEW_IS_HIDE = FALSE;
	VIDEOPLAY_IS_HIDE = TRUE;
    }
    else if (type == VIEWTYPE_VIDEOPLAY && IMAGEVIEW_IS_HIDE == FALSE)
    {
	gtk_widget_show (gtk_notebook_get_nth_page
			 (GTK_NOTEBOOK (BROWSER_VIEW), 1));
	gtk_widget_hide (gtk_notebook_get_nth_page
			 (GTK_NOTEBOOK (BROWSER_VIEW), 0));

	IMAGEVIEW_IS_HIDE = TRUE;
	VIDEOPLAY_IS_HIDE = FALSE;
    }
#endif
}

ViewType
viewtype_get (void)
{
    return gtk_notebook_get_current_page (GTK_NOTEBOOK (BROWSER_VIEW));
}

void
viewtype_set_force (ViewType type)
{
    gtk_notebook_set_page (GTK_NOTEBOOK (BROWSER_VIEW), type);
}
