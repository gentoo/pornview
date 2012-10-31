/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtk.h>

#include "file_list.h"
#include "file_type.h"
#include "file_utils.h"

static  gint
file_list_valcomp (gpointer a, gpointer b)
{
    FileListNode *node1, *node2;

    node1 = (FileListNode *) a;
    node2 = (FileListNode *) b;

    return strcmp ((gchar *) node1->file_name, (gchar *) node2->file_name);
}

FileList *
file_list_init (void)
{
    FileList *fl;

    fl = g_new0 (FileList, 1);
    file_list_alloc (fl);

    return fl;
}

void
file_list_free (FileList * fl)
{

    if (fl->list != NULL)
	file_list_clear (fl);

    g_free (fl);
}

void
file_list_alloc (FileList * fl)
{
    FileListNode *data;

    fl->list = g_list_alloc ();
    data = g_malloc (sizeof (FileListNode));
    data->file_name = g_strdup ("");
    data->file_type = -1;

    fl->list->data = data;
}

void
file_list_add (FileList * fl, gchar * name, gint type)
{
    FileListNode *data;

    data = g_malloc (sizeof (FileListNode));
    data->file_name = g_strdup (name);
    data->file_type = type;

    fl->list =
	g_list_insert_sorted (fl->list, data,
			      (GCompareFunc) file_list_valcomp);

    fl->num++;
}

void
file_list_clear (FileList * fl)
{
    GList  *list_node;
    FileListNode *data;

    if (fl->list == NULL)
	return;

    for (list_node = fl->list; list_node; list_node = list_node->next)
    {
	data = list_node->data;

	if (data)
	{
	    if (data->file_name)
		g_free (data->file_name);

	    g_free (data);
	}

	list_node->data = NULL;
    }

    g_list_free (fl->list);

    fl->list = NULL;
    fl->num = 0;
}

void
file_list_create (FileList * fl, gchar * path)
{
    DIR    *dir;
    struct dirent *file;

    if (fl->list != NULL)
	file_list_clear (fl);

    if (path == NULL)
	return;

    dir = opendir (path);

    if (!dir)
	return;

    file_list_alloc (fl);

    while ((file = readdir (dir)))
    {
	if ((file->d_name[0] == '.' && file->d_name[1] == 0)
	    || (file->d_name[0] == '.' && file->d_name[1] == '.'
		&& file->d_name[2] == 0))
	    continue;

	if (isfile (file->d_name)
	    && file_type_detect_type_by_ext (file->d_name))
	{
	    gint    file_type = FILETYPE_IMAGE;

	    if (file_type_is_movie (file->d_name))
		file_type = FILETYPE_VIDEO;

	    file_list_add (fl, concat_dir_and_file (path, file->d_name),
			   file_type);
	}

	while (gtk_events_pending ())
	    gtk_main_iteration ();
    }

    closedir (dir);
}
