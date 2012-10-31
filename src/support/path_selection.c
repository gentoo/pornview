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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#include "path_selection.h"
#include "clist_edit.h"
#include "generic_dialog.h"
#include "file_utils.h"
#include "dialogs.h"
#include "string_utils.h"

#define DEST_WIDTH 250
#define DEST_HEIGHT 200

#define RENAME_PRESS_DELAY 333	/* 1/3 second, to allow double clicks */

/* #define PATH_SEL_USE_HEADINGS to enable list headings */

typedef struct _Dest_Data Dest_Data;
struct _Dest_Data
{
    GtkWidget *d_clist;
    GtkWidget *f_clist;
    GtkWidget *entry;
    gchar  *filter;
    gchar  *path;

    GList  *filter_list;
    GList  *filter_text_list;
    GtkWidget *filter_combo;

    gint    show_hidden;
    GtkWidget *hidden_button;

    guint32 rename_time;
    gint    right_click_row;

    void    (*select_func) (const gchar * path, gpointer data);
    gpointer select_data;

    GenericDialog *gd;		/* any open confirm dialogs ? */
};

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static void
dest_free_data (GtkWidget * widget, gpointer data)
{
    Dest_Data *dd = data;

    if (dd->gd)
    {
	GenericDialog *gd = dd->gd;
	generic_dialog_close (gd);
    }

    g_free (dd->filter);
    g_free (dd->path);
    g_free (dd);
}

static  gint
dest_check_filter (const gchar * filter, const gchar * file)
{
    const gchar *f_ptr = filter;
    const gchar *strt_ptr;
    gint    i;
    gint    l;

    l = strlen (file);

    if (filter[0] == '*')
	return TRUE;
    while (f_ptr < filter + strlen (filter))
    {
	strt_ptr = f_ptr;
	i = 0;
	while (*f_ptr != ';' && *f_ptr != '\0')
	{
	    f_ptr++;
	    i++;
	}
	if (*f_ptr != '\0' && f_ptr[1] == ' ')
	    f_ptr++;		/* skip space immediately after separator */
	f_ptr++;
	if (l >= i && strncasecmp (file + l - i, strt_ptr, i) == 0)
	    return TRUE;
    }

    return FALSE;
}

static  gint
cb_dest_sort (void *a, void *b)
{
    return strcmp ((gchar *) a, (gchar *) b);
}

static  gint
is_hidden (const gchar * name)
{
    if (name[0] != '.')
	return FALSE;
    if (name[1] == '\0')
	return FALSE;
    if (name[1] == '.' && name[2] == '\0')
	return FALSE;
    return TRUE;
}

static void
dest_populate (Dest_Data * dd, const gchar * path)
{
    DIR    *dp;
    struct dirent *dir;
    struct stat ent_sbuf;
    gchar  *buf[2];
    GList  *path_list = NULL;
    GList  *file_list = NULL;
    GList  *list;

    buf[1] = NULL;

    if (!path || (dp = opendir (path)) == NULL)
    {
	/*
	 * dir not found 
	 */
	return;
    }
    while ((dir = readdir (dp)) != NULL)
    {
	/*
	 * skips removed files 
	 */
	if (dir->d_ino > 0 && (dd->show_hidden || !is_hidden (dir->d_name)))
	{
	    gchar  *name = dir->d_name;
	    gchar  *filepath = g_strconcat (path, "/", name, NULL);
	    if (stat (filepath, &ent_sbuf) >= 0 && S_ISDIR (ent_sbuf.st_mode))
	    {
		path_list = g_list_prepend (path_list, g_strdup (name));
	    }
	    else if (dd->f_clist)
	    {
		if (!dd->filter
		    || (dd->filter && dest_check_filter (dd->filter, name)))
		    file_list = g_list_prepend (file_list, g_strdup (name));
	    }
	    g_free (filepath);
	}
    }
    closedir (dp);

    path_list = g_list_sort (path_list, (GCompareFunc) cb_dest_sort);
    file_list = g_list_sort (file_list, (GCompareFunc) cb_dest_sort);

    gtk_clist_freeze (GTK_CLIST (dd->d_clist));
    gtk_clist_clear (GTK_CLIST (dd->d_clist));

    list = path_list;
    while (list)
    {
	gint    row;
	gchar  *filepath;
	if (strcmp (list->data, ".") == 0)
	{
	    filepath = g_strdup (path);
	}
	else if (strcmp (list->data, "..") == 0)
	{
	    gchar  *p;
	    filepath = g_strdup (path);
	    p = (gchar *) filename_from_path (filepath);
	    if (p - 1 != filepath)
		p--;
	    p[0] = '\0';
	}
	else
	{
	    filepath = concat_dir_and_file (path, list->data);
	}

	buf[0] = list->data;
	row = gtk_clist_append (GTK_CLIST (dd->d_clist), buf);
	gtk_clist_set_row_data_full (GTK_CLIST (dd->d_clist), row,
				     filepath, (GtkDestroyNotify) g_free);
	g_free (list->data);
	list = list->next;
    }

    g_list_free (path_list);

    gtk_clist_thaw (GTK_CLIST (dd->d_clist));

    if (dd->f_clist)
    {
	gtk_clist_freeze (GTK_CLIST (dd->f_clist));
	gtk_clist_clear (GTK_CLIST (dd->f_clist));

	list = file_list;
	while (list)
	{
	    gint    row;
	    gchar  *filepath;
	    const gchar *name = list->data;

	    filepath = concat_dir_and_file (path, name);

	    buf[0] = list->data;
	    row = gtk_clist_append (GTK_CLIST (dd->f_clist), buf);
	    gtk_clist_set_row_data_full (GTK_CLIST (dd->f_clist), row,
					 filepath, (GtkDestroyNotify) g_free);
	    g_free (list->data);
	    list = list->next;
	}

	g_list_free (file_list);

	gtk_clist_thaw (GTK_CLIST (dd->f_clist));
    }

    g_free (dd->path);
    dd->path = g_strdup (path);
}

static void
dest_change_dir (Dest_Data * dd, const gchar * path, gint retain_name)
{
    gchar  *old_name = NULL;
    gint    s = 0;

    if (retain_name)
    {
	const gchar *buf = gtk_entry_get_text (GTK_ENTRY (dd->entry));
	if (!isdir (buf))
	{
	    if (path && strcmp (path, "/") == 0)
	    {
		old_name = g_strdup (filename_from_path (buf));
	    }
	    else
	    {
		old_name = g_strconcat ("/", filename_from_path (buf), NULL);
		s = 1;
	    }
	}
    }

    gtk_entry_set_text (GTK_ENTRY (dd->entry), path);

    dest_populate (dd, path);

    /*
     * remember filename 
     */
    if (old_name)
    {
	gtk_entry_append_text (GTK_ENTRY (dd->entry), old_name);
	gtk_entry_select_region (GTK_ENTRY (dd->entry), strlen (path) + s,
				 strlen (path) + strlen (old_name));
	g_free (old_name);
    }
}

/*
 *-------------------------------------------------------------------
 * destination widget file management utils
 *-------------------------------------------------------------------
 */

static  gint
cb_dest_row_edit_rename (ClistEditData * ced, const gchar * old,
			 const gchar * new, gpointer data)
{
    const gchar *old_path;
    gchar  *new_path;
    gchar  *buf;

    old_path = gtk_clist_get_row_data (GTK_CLIST (ced->clist), ced->row);
    if (!old_path)
	return FALSE;

    buf = remove_level_from_path (old_path);
    new_path = concat_dir_and_file (buf, new);
    g_free (buf);

    if (isname (new_path))
    {
	buf = g_strdup_printf (_("File name %s already exists."), new);
	dialog_message ("Rename failed", buf, NULL);
	g_free (buf);
    }
    else if (rename (old_path, new_path) != 0)
    {
	buf = g_strdup_printf (_("Failed to rename %s to %s."), old, new);
	dialog_message ("Rename failed", buf, NULL);
	g_free (buf);
    }
    else
    {
	gtk_clist_set_row_data_full (GTK_CLIST (ced->clist), ced->row,
				     new_path, (GtkDestroyNotify) g_free);
	return TRUE;
    }

    g_free (new_path);
    return FALSE;
}

static  gint
cb_dest_press (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
    Dest_Data *dd = data;
    gint    row = -1;
    gint    col = -1;

    if (!event)
	return FALSE;
    gtk_clist_get_selection_info (GTK_CLIST (widget), event->x, event->y,
				  &row, &col);
    if (row == -1 || col == -1)
	return FALSE;

    if (widget == dd->d_clist)
    {
	/*
	 * dir pressed 
	 */

	if (event->button == 3 && row > 1)
	{
	    /*
	     * right click menu 
	     */

	}

	return FALSE;
    }
    else
    {
	/*
	 * file pressed 
	 */

	if (event->button == 3)
	{
	    /*
	     * right click menu 
	     */

	    return FALSE;
	}

	if (event->button != 1 ||
	    event->type != GDK_BUTTON_PRESS ||
	    event->time - dd->rename_time < RENAME_PRESS_DELAY)
	{
	    dd->rename_time = event->time;
	    return FALSE;
	}

	if (!GTK_CLIST (dd->f_clist)->selection ||
	    row != GPOINTER_TO_INT (GTK_CLIST (dd->f_clist)->selection->data))
	{
	    dd->rename_time = event->time;
	    return FALSE;
	}

	clist_edit_by_row (GTK_CLIST (dd->f_clist), row, 0,
			   cb_dest_row_edit_rename, dd);
    }

    return FALSE;
}

/*
 *-------------------------------------------------------------------
 * destination widget file selection, traversal, view options
 *-------------------------------------------------------------------
 */

static void
cb_dest_select (GtkWidget * clist, gint row, gint column,
		GdkEventButton * bevent, gpointer data)
{
    Dest_Data *dd = data;
    gchar  *path = g_strdup (gtk_clist_get_row_data (GTK_CLIST (clist), row));

    if (clist == dd->d_clist)
    {
	dest_change_dir (dd, path, (dd->f_clist != NULL));
    }
    else
    {
	gtk_entry_set_text (GTK_ENTRY (dd->entry), path);

	if (bevent)
	    dd->rename_time = bevent->time;

	if (bevent && bevent->button == 1 && bevent->type == GDK_2BUTTON_PRESS
	    && dd->select_func)
	{
	    dd->select_func (path, dd->select_data);
	}
    }

    g_free (path);
}

static void
cb_dest_home (GtkWidget * widget, gpointer data)
{
    Dest_Data *dd = data;

    dest_change_dir (dd, home_dir (), (dd->f_clist != NULL));
}

static void
cb_dest_show_hidden (GtkWidget * widget, gpointer data)
{
    Dest_Data *dd = data;
    gchar  *buf;

    dd->show_hidden =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dd->hidden_button));

    buf = g_strdup (dd->path);
    dest_populate (dd, buf);
    g_free (buf);
}

static void
cb_dest_entry_changed (GtkEditable * editable, gpointer data)
{
    Dest_Data *dd = data;
    const gchar *path;
    gchar  *buf;

    path = gtk_entry_get_text (GTK_ENTRY (dd->entry));
    if (strcmp (path, dd->path) == 0)
	return;

    buf = remove_level_from_path (path);

    if (buf && strcmp (buf, dd->path) != 0)
    {
	gchar  *tmp = remove_trailing_slash (path);
	if (isdir (tmp))
	{
	    dest_populate (dd, tmp);
	}
	else if (isdir (buf))
	{
	    dest_populate (dd, buf);
	}
	g_free (tmp);
    }
    g_free (buf);
}

static void
dest_filter_list_sync (Dest_Data * dd)
{
    gchar  *old_text;
    GList  *fwork;
    GList  *cwork;

    if (!dd->filter_list || !dd->filter_combo)
	return;

    old_text =
	g_strdup (gtk_entry_get_text
		  (GTK_ENTRY (GTK_COMBO (dd->filter_combo)->entry)));

    gtk_combo_set_popdown_strings (GTK_COMBO (dd->filter_combo),
				   dd->filter_text_list);

    fwork = dd->filter_list;
    cwork = GTK_LIST (GTK_COMBO (dd->filter_combo)->list)->children;
    while (fwork && cwork)
    {
	gchar  *filter = fwork->data;

	gtk_combo_set_item_string (GTK_COMBO (dd->filter_combo),
				   GTK_ITEM (cwork->data), filter);

	if (strcmp (old_text, filter) == 0)
	{
	    gtk_entry_set_text (GTK_ENTRY
				(GTK_COMBO (dd->filter_combo)->entry),
				filter);
	}

	fwork = fwork->next;
	cwork = cwork->next;
    }

    g_free (old_text);
}

static void
dest_filter_add (Dest_Data * dd, const gchar * filter,
		 const gchar * description, gint set)
{
    GList  *work;
    gchar  *buf;
    gint    c = 0;

    if (!filter)
	return;

    work = dd->filter_list;
    while (work)
    {
	gchar  *f = work->data;

	if (strcmp (f, filter) == 0)
	{
	    if (set)
		gtk_combo_set_value_in_list (GTK_COMBO (dd->filter_combo), c,
					     FALSE);
	    return;
	}
	work = work->next;
	c++;
    }

    dd->filter_list =
	uig_list_insert_link (dd->filter_list, g_list_last (dd->filter_list),
			      g_strdup (filter));

    if (description)
    {
	buf = g_strdup_printf ("%s  ( %s )", description, filter);
    }
    else
    {
	buf = g_strdup_printf ("( %s )", filter);
    }
    dd->filter_text_list =
	uig_list_insert_link (dd->filter_text_list,
			      g_list_last (dd->filter_text_list), buf);

    if (set)
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (dd->filter_combo)->entry),
			    filter);
    dest_filter_list_sync (dd);
}

static void
dest_filter_clear (Dest_Data * dd)
{
    path_list_free (dd->filter_list);
    dd->filter_list = NULL;

    path_list_free (dd->filter_text_list);
    dd->filter_text_list = NULL;

    dest_filter_add (dd, "*", _("All Files"), TRUE);
}

static void
cb_dest_filter_changed (GtkEditable * editable, gpointer data)
{
    Dest_Data *dd = data;
    const gchar *buf;
    gchar  *path;

    buf =
	gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (dd->filter_combo)->entry));

    g_free (dd->filter);
    dd->filter = NULL;
    if (strlen (buf) > 0)
	dd->filter = g_strdup (buf);

    path = g_strdup (dd->path);
    dest_populate (dd, path);
    g_free (path);
}

/*
 *-------------------------------------------------------------------
 * destination widget setup routines (public)
 *-------------------------------------------------------------------
 */

GtkWidget *
path_selection_new_with_files (GtkWidget * entry, const gchar * path,
			       const gchar * filter,
			       const gchar * filter_desc)
{
    GtkWidget *hbox2;
    Dest_Data *dd;
    GtkWidget *scrolled;
    GtkWidget *button;
    GtkWidget *table;
    GtkWidget *paned;
#ifdef PATH_SEL_USE_HEADINGS
    gchar  *d_title[] = { "Directories", NULL };
    gchar  *f_title[] = { "Files", NULL };
#endif

    dd = g_new0 (Dest_Data, 1);
    dd->show_hidden = FALSE;
    dd->select_func = NULL;
    dd->select_data = NULL;
    dd->gd = NULL;

    table = gtk_table_new (4, 3, FALSE);
    gtk_table_set_col_spacing (GTK_TABLE (table), 1, 5);
    gtk_widget_show (table);

    dd->entry = entry;
    gtk_object_set_data (GTK_OBJECT (dd->entry), "destination_data", dd);

    hbox2 = gtk_hbox_new (FALSE, 5);
    gtk_table_attach (GTK_TABLE (table), hbox2, 0, 1, 0, 1,
		      GTK_EXPAND | GTK_FILL, FALSE, 0, 5);
    gtk_widget_show (hbox2);

    button = gtk_button_new_with_label (_("Home"));
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (cb_dest_home), dd);
    gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
#ifndef USE_GTK2
    gtk_widget_set_usize (button, 100, -1);
#else
    gtk_widget_set_usize (button, 85, 28);
#endif

    gtk_widget_show (button);

    dd->hidden_button = gtk_check_button_new_with_label (_("Show hidden"));
    gtk_signal_connect (GTK_OBJECT (dd->hidden_button), "clicked",
			GTK_SIGNAL_FUNC (cb_dest_show_hidden), dd);
    gtk_box_pack_end (GTK_BOX (hbox2), dd->hidden_button, FALSE, FALSE, 0);
    gtk_widget_show (dd->hidden_button);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    if (filter)
    {
	paned = gtk_hpaned_new ();
	gtk_table_attach (GTK_TABLE (table), paned, 0, 3, 1, 2,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show (paned);
	gtk_paned_add1 (GTK_PANED (paned), scrolled);
    }
    else
    {
	paned = NULL;
	gtk_table_attach (GTK_TABLE (table), scrolled, 0, 1, 1, 2,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    }

    gtk_widget_show (scrolled);

#ifdef PATH_SEL_USE_HEADINGS
    dd->d_clist = gtk_clist_new_with_titles (1, d_title);
    gtk_clist_column_titles_passive (GTK_CLIST (dd->d_clist));
#else
    dd->d_clist = gtk_clist_new (1);
#endif
    gtk_clist_set_column_auto_resize (GTK_CLIST (dd->d_clist), 0, TRUE);
    gtk_signal_connect (GTK_OBJECT (dd->d_clist), "button_press_event",
			GTK_SIGNAL_FUNC (cb_dest_press), dd);
    gtk_signal_connect (GTK_OBJECT (dd->d_clist), "select_row",
			GTK_SIGNAL_FUNC (cb_dest_select), dd);
    gtk_signal_connect (GTK_OBJECT (dd->d_clist), "destroy",
			GTK_SIGNAL_FUNC (dest_free_data), dd);
    gtk_widget_set_usize (dd->d_clist, DEST_WIDTH, DEST_HEIGHT);
    gtk_container_add (GTK_CONTAINER (scrolled), dd->d_clist);
    gtk_widget_show (dd->d_clist);

    if (filter)
    {
	GtkWidget *label;

	hbox2 = gtk_hbox_new (FALSE, 5);
	gtk_table_attach (GTK_TABLE (table), hbox2, 2, 3, 0, 1,
			  GTK_EXPAND | GTK_FILL, FALSE, 0, 5);
	gtk_widget_show (hbox2);

	label = gtk_label_new (_("Filter:"));
	gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	dd->filter_combo = gtk_combo_new ();
	gtk_combo_set_case_sensitive (GTK_COMBO (dd->filter_combo), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox2), dd->filter_combo, TRUE, TRUE, 0);
	gtk_widget_show (dd->filter_combo);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_ALWAYS);
	if (paned)
	{
	    gtk_paned_add2 (GTK_PANED (paned), scrolled);
	}
	else
	{
	    gtk_table_attach (GTK_TABLE (table), scrolled, 2, 3, 1, 2,
			      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0,
			      0);
	}
	gtk_widget_show (scrolled);

#ifdef PATH_SEL_USE_HEADINGS
	dd->f_clist = gtk_clist_new_with_titles (1, f_title);
	gtk_clist_column_titles_passive (GTK_CLIST (dd->f_clist));
#else
	dd->f_clist = gtk_clist_new (1);
#endif
	gtk_clist_set_column_auto_resize (GTK_CLIST (dd->f_clist), 0, TRUE);
	gtk_widget_set_usize (dd->f_clist, DEST_WIDTH, DEST_HEIGHT);
	gtk_signal_connect (GTK_OBJECT (dd->f_clist), "button_press_event",
			    GTK_SIGNAL_FUNC (cb_dest_press), dd);
	gtk_signal_connect (GTK_OBJECT (dd->f_clist), "select_row",
			    GTK_SIGNAL_FUNC (cb_dest_select), dd);
	gtk_container_add (GTK_CONTAINER (scrolled), dd->f_clist);
	gtk_widget_show (dd->f_clist);

	dest_filter_clear (dd);
	dest_filter_add (dd, filter, filter_desc, TRUE);

	dd->filter =
	    g_strdup (gtk_entry_get_text
		      (GTK_ENTRY (GTK_COMBO (dd->filter_combo)->entry)));
    }

    if (path && path[0] == '/' && isdir (path))
    {
	dest_populate (dd, path);
    }
    else
    {
	gchar  *buf = remove_level_from_path (path);
	if (buf && buf[0] == '/' && isdir (buf))
	{
	    dest_populate (dd, buf);
	}
	else
	{
	    dest_populate (dd, (gchar *) home_dir ());
	    if (path)
		gtk_entry_append_text (GTK_ENTRY (dd->entry), "/");
	    if (path)
		gtk_entry_append_text (GTK_ENTRY (dd->entry), path);
	}
	g_free (buf);
    }

    if (dd->filter_combo)
    {
	gtk_signal_connect (GTK_OBJECT (GTK_COMBO (dd->filter_combo)->entry),
			    "changed",
			    GTK_SIGNAL_FUNC (cb_dest_filter_changed), dd);
    }
    gtk_signal_connect (GTK_OBJECT (dd->entry), "changed",
			GTK_SIGNAL_FUNC (cb_dest_entry_changed), dd);

    return table;
}

GtkWidget *
path_selection_new (const gchar * path, GtkWidget * entry)
{
    return path_selection_new_with_files (entry, path, NULL, NULL);
}

void
path_selection_sync_to_entry (GtkWidget * entry)
{
    Dest_Data *dd =
	gtk_object_get_data (GTK_OBJECT (entry), "destination_data");
    const gchar *path;

    if (!dd)
	return;

    path = gtk_entry_get_text (GTK_ENTRY (entry));

    if (isdir (path) && strcmp (path, dd->path) != 0)
    {
	dest_populate (dd, path);
    }
    else
    {
	gchar  *buf = remove_level_from_path (path);
	if (isdir (buf) && strcmp (buf, dd->path) != 0)
	{
	    dest_populate (dd, buf);
	}
	g_free (buf);
    }
}

void
path_selection_add_select_func (GtkWidget * entry,
				void (*func) (const gchar *, gpointer),
				gpointer data)
{
    Dest_Data *dd =
	gtk_object_get_data (GTK_OBJECT (entry), "destination_data");

    if (!dd)
	return;

    dd->select_func = func;
    dd->select_data = data;
}

void
path_selection_add_filter (GtkWidget * entry, const gchar * filter,
			   const gchar * description, gint set)
{
    Dest_Data *dd =
	gtk_object_get_data (GTK_OBJECT (entry), "destination_data");

    if (!dd)
	return;
    if (!filter)
	return;

    dest_filter_add (dd, filter, description, set);
}

void
path_selection_clear_filter (GtkWidget * entry)
{
    Dest_Data *dd =
	gtk_object_get_data (GTK_OBJECT (entry), "destination_data");

    if (!dd)
	return;

    dest_filter_clear (dd);
}
