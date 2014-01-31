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

#ifndef __CLIST_EDIT_H__
#define __CLIST_EDIT_H__

typedef struct _ClistEditData ClistEditData;

struct _ClistEditData
{
    GtkWidget      *window;
    GtkWidget      *entry;

    gchar          *old_name;
    gchar          *new_name;

	gint (*edit_func) (ClistEditData * ced, const gchar * oldname,
			   const gchar * newname, gpointer data);

    gpointer        edit_data;

    GtkCList       *clist;
    gint            row;
    gint            column;
};

/*
 * edit_func: return TRUE is rename successful, FALSE on failure.
 */
gint            clist_edit_by_row (GtkCList * clist, gint row, gint column,
				   gint (*edit_func) (ClistEditData *,
						      const gchar *,
						      const gchar *,
						      gpointer),
				   gpointer data);
/*
 * use this when highlighting a right-click menued or dnd clist row.
 */
void            clist_edit_set_highlight (GtkWidget * clist, gint row,
					  gint set);

/*
 * Various g_list utils, do not really fit anywhere, so they are here.
 */
GList          *uig_list_insert_link (GList * list, GList * link,
				      gpointer data);
GList          *uig_list_insert_list (GList * parent, GList * insert_link,
				      GList * list);

#endif /* __CLIST_EDIT_H__ */
