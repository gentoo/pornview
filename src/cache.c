/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 *  These codes are based on src/cache_maint.c in GQview.
 *  GQview author: John Ellis
 */

#include "pornview.h"

#include "file_utils.h"
#include "generic_dialog.h"

#include "cache.h"
#include "prefs.h"

typedef struct _CMData CMData;
struct _CMData
{
    GList  *list;
    GList  *done_list;
    gint    idle_id;
    GenericDialog *gd;
    GtkWidget *entry;
    gint    clear;
    CacheType type;
};

#define PURGE_DIALOG_WIDTH 400

/*
 *-------------------------------------------------------------------
 * cache path location utils
 *-------------------------------------------------------------------
 */

gint
cache_ensure_dir_exists (gchar * path, mode_t mode)
{
    if (!path)
	return FALSE;

    if (!isdir (path))
    {
	gchar  *p = path;
	while (p[0] != '\0')
	{
	    p++;
	    if (p[0] == '/' || p[0] == '\0')
	    {
		gint    end = TRUE;
		if (p[0] != '\0')
		{
		    p[0] = '\0';
		    end = FALSE;
		}
		if (!isdir (path))
		{
		    if (mkdir (path, mode) < 0)
		    {
			return FALSE;
		    }
		}
		if (!end)
		    p[0] = '/';
	    }
	}
    }
    return TRUE;
}

gchar  *
cache_get_location (CacheType type, const gchar * source, gint include_name,
		    const gchar * ext, mode_t * mode)
{
    gchar  *path = NULL;
    gchar  *base;
    gchar  *name = NULL;

    if (!source)
	return NULL;

    base = remove_level_from_path (source);

    if (include_name)
    {
	name = g_strconcat ("/", filename_from_path (source), NULL);
    }
    else
    {
	ext = NULL;
    }

    if (type == CACHE_THUMBS)
    {
	if (conf.enable_thumb_dirs && access (base, W_OK) == 0)
	{
	    path =
		g_strconcat (base, "/", PORNVIEW_CACHE_DIR, name, ext, NULL);
	    if (mode)
		*mode = 0775;
	}

	if (!path)
	{
	    path =
		g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_THUMBS, base,
			     name, ext, NULL);
	    if (mode)
		*mode = 0755;
	}
    }
    else
    {
	path =
	    g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_COMMENTS, base,
			 name, ext, NULL);
	if (mode)
	    *mode = 0755;
    }

    g_free (base);

    if (name)
	g_free (name);

    return path;
}

gchar  *
cache_find_location (CacheType type, const gchar * source, const gchar * ext)
{
    gchar  *path;
    const gchar *name;
    gchar  *base;

    if (!source)
	return NULL;

    name = filename_from_path (source);
    base = remove_level_from_path (source);

    if (type == CACHE_THUMBS)
    {
	if (conf.enable_thumb_dirs)
	{
	    path =
		g_strconcat (base, "/", PORNVIEW_CACHE_DIR, "/", name, ext,
			     NULL);
	}
	else
	{
	    path =
		g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_THUMBS, source,
			     ext, NULL);
	}
    }
    else
    {
	path =
	    g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_COMMENTS, source,
			 ext, NULL);
    }

    if (!isfile (path))
    {
	g_free (path);

	/*
	 * try the opposite method if not found 
	 */
	if (type == CACHE_THUMBS)
	{

	    if (!conf.enable_thumb_dirs)
	    {
		path =
		    g_strconcat (base, "/", PORNVIEW_CACHE_DIR, "/", name,
				 ext, NULL);
	    }
	    else
	    {
		path =
		    g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_THUMBS,
				 source, ext, NULL);
	    }
	}
	else
	{
	    path =
		g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_COMMENTS,
			     source, ext, NULL);
	}
    }

    if (!isfile (path))
    {
	g_free (path);
	path = NULL;
    }

    g_free (base);
    return path;
}

/*
 *-------------------------------------------------------------------
 * cache maintenance
 *-------------------------------------------------------------------
 */

static void
cache_maintain_home_close (CMData * cm)
{
    if (cm->idle_id != -1)
	gtk_idle_remove (cm->idle_id);

    if (cm->gd)
	generic_dialog_close (cm->gd);

    path_list_free (cm->list);

    g_list_free (cm->done_list);
    g_free (cm);
}

static  gint
cb_cache_maintain_home (gpointer data)
{
    CMData *cm = data;
    GList  *dlist = NULL;
    GList  *list = NULL;
    gchar  *path;
    gint    just_done = FALSE;
    gint    still_have_a_file = TRUE;
    gint    base_length;

    if (cm->type == CACHE_THUMBS)
	base_length =
	    strlen (home_dir ()) + strlen ("/") +
	    strlen (PORNVIEW_RC_DIR_THUMBS);
    else
	base_length =
	    strlen (home_dir ()) + strlen ("/") +
	    strlen (PORNVIEW_RC_DIR_COMMENTS);

    if (!cm->list)
    {
	cm->idle_id = -1;
	cache_maintain_home_close (cm);
	return FALSE;
    }

    path = cm->list->data;
    if (g_list_find (cm->done_list, path) == NULL)
    {
	cm->done_list = g_list_prepend (cm->done_list, path);
	if (path_list (path, &list, &dlist))
	{
	    GList  *work;

	    just_done = TRUE;
	    still_have_a_file = FALSE;
	    work = list;

	    while (work)
	    {
		gchar  *path_buf = work->data;
		gchar  *dot;

		dot = extension_find_dot (path_buf);

		if (dot)
		    *dot = '\0';

		if (cm->clear ||
		    (strlen (path_buf) > base_length
		     && !isfile (path_buf + base_length)))
		{
		    if (dot)
			*dot = '.';
		    if (unlink (path_buf) < 0)
			printf ("failed to delete:%s\n", path_buf);
		}
		else
		{
		    still_have_a_file = TRUE;
		}
		work = work->next;
	    }
	}
    }

    path_list_free (list);

    cm->list = g_list_concat (dlist, cm->list);

    if (cm->list && g_list_find (cm->done_list, cm->list->data) != NULL)
    {
	/*
	 * check if the dir is empty 
	 */

	if (cm->list->data == path && just_done)
	{
	    if (!still_have_a_file && !dlist && cm->list->next
		&& rmdir (path) < 0)
	    {
		printf ("Unable to delete dir: %s\n", path);
	    }
	}
	else
	{
	    /*
	     * must re-check for an empty dir 
	     */
	    path = cm->list->data;
	    if (isempty (path) && cm->list->next && rmdir (path) < 0)
	    {
		printf ("Unable to delete dir: %s\n", path);
	    }
	}

	path = cm->list->data;
	cm->done_list = g_list_remove (cm->done_list, path);
	cm->list = g_list_remove (cm->list, path);
	g_free (path);
    }

    if (cm->list)
    {
	const gchar *buf;

	path = cm->list->data;

	if (strlen (path) > base_length)
	{
	    buf = path + base_length;
	}
	else
	{
	    buf = "...";
	}

	gtk_entry_set_text (GTK_ENTRY (cm->entry), buf);
    }

    return TRUE;
}

static void
cb_cache_maintain_home_cancel (GenericDialog * gd, gpointer data)
{
    CMData *cm = data;

    cm->gd = NULL;
    cache_maintain_home_close (cm);
}

/* sorry for complexity (cm->done_list), but need it to remove empty dirs */
void
cache_maintain_home (CacheType type, gint clear)
{
    CMData *cm;
    GList  *dlist = NULL;
    gchar  *base;
    const gchar *msg;

    if (type == CACHE_THUMBS)
	base = g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_THUMBS, NULL);
    else
	base = g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_COMMENTS, NULL);

    if (!path_list (base, NULL, &dlist))
    {
	g_free (base);
	return;
    }

    dlist = g_list_append (dlist, base);
    cm = g_new0 (CMData, 1);
    cm->list = dlist;
    cm->done_list = NULL;
    cm->clear = clear;
    cm->type = type;

    if (clear)
    {
	if (type == CACHE_THUMBS)
	    msg = _("Clearing thumbnails...");
	else
	    msg = _("Clearing comments...");
    }
    else
    {
	if (type == CACHE_THUMBS)
	    msg = _("Purging old thumbnails...");
	else
	    msg = _("Purging old comments...");
    }

    if (type == CACHE_THUMBS)
	cm->gd = generic_dialog_new (_("Purge thumbnails"), msg,
				     "PornView", "purge_thumbnails",
				     TRUE, cb_cache_maintain_home_cancel, cm);
    else
	cm->gd = generic_dialog_new (_("Purge comments"), msg,
				     "PornView", "purge_comments",
				     TRUE, cb_cache_maintain_home_cancel, cm);

    gtk_window_set_position (GTK_WINDOW (cm->gd->dialog), GTK_WIN_POS_CENTER);
    gtk_widget_set_usize (cm->gd->dialog, PURGE_DIALOG_WIDTH, -1);
    cm->entry = gtk_entry_new ();
    gtk_widget_set_sensitive (cm->entry, FALSE);
    gtk_box_pack_start (GTK_BOX (cm->gd->vbox), cm->entry, FALSE, FALSE, 5);
    gtk_widget_show (cm->entry);
    gtk_widget_show (cm->gd->dialog);
    cm->idle_id = gtk_idle_add (cb_cache_maintain_home, cm);
}

int
cache_maintain_home_dir (CacheType type, const gchar * dir,
			 gint recursive, gint clear)
{
    gchar  *base;
    gint    base_length;
    GList  *dlist = NULL;
    GList  *flist = NULL;
    gint    still_have_a_file = FALSE;

    if (type == CACHE_THUMBS)
    {
	base_length =
	    strlen (home_dir ()) + strlen ("/") +
	    strlen (PORNVIEW_RC_DIR_THUMBS);
	base =
	    g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_THUMBS, dir, NULL);
    }
    else
    {
	base_length =
	    strlen (home_dir ()) + strlen ("/") +
	    strlen (PORNVIEW_RC_DIR_COMMENTS);
	base =
	    g_strconcat (home_dir (), "/", PORNVIEW_RC_DIR_COMMENTS, dir,
			 NULL);
    }

    if (path_list (base, &flist, &dlist))
    {
	GList  *work;
	work = dlist;
	while (work)
	{
	    gchar  *path = work->data;
	    if (recursive && strlen (path) > base_length &&
		!cache_maintain_home_dir (type, path + base_length,
					  recursive, clear))
	    {
		if (rmdir (path) < 0)
		{
		    printf ("Unable to delete dir: %s\n", path);
		}
	    }
	    else
	    {
		still_have_a_file = TRUE;
	    }
	    work = work->next;
	}

	work = flist;

	while (work)
	{
	    gchar  *path = work->data;
	    gchar  *dot;

	    dot = extension_find_dot (path);

	    if (dot)
		*dot = '\0';

	    if (clear ||
		(strlen (path) > base_length && !isfile (path + base_length)))
	    {
		if (dot)
		    *dot = '.';

		if (unlink (path) < 0)
		    printf ("failed to delete:%s\n", path);
	    }
	    else
	    {
		still_have_a_file = TRUE;
	    }

	    work = work->next;
	}
    }

    path_list_free (dlist);
    path_list_free (flist);
    g_free (base);

    return still_have_a_file;
}

/* This checks relative caches in dir/.thumbnails and
 * removes them if they have no source counterpart.
 */
gint
cache_maintain_dir (CacheType type, const gchar * dir,
		    gint recursive, gint clear)
{
    GList  *list = NULL;
    gchar  *cachedir;
    gint    still_have_a_file = FALSE;
    GList  *work;

    if (type != CACHE_THUMBS)
	return FALSE;

    cachedir = g_strconcat (dir, "/", PORNVIEW_CACHE_DIR, NULL);
    path_list (cachedir, &list, NULL);
    work = list;

    while (work)
    {
	const gchar *path;
	gchar  *source;

	path = work->data;
	work = work->next;
	source = g_strconcat (dir, "/", filename_from_path (path), NULL);

	if (clear || extension_truncate (source, PORNVIEW_CACHE_THUMB_EXT))
	{
	    if (!clear && isfile (source))
	    {
		still_have_a_file = TRUE;
	    }
	    else
	    {
		if (unlink (path) < 0)
		{
		    still_have_a_file = TRUE;
		}
	    }
	}
	else
	{
	    still_have_a_file = TRUE;
	}
	g_free (source);
    }

    path_list_free (list);
    g_free (cachedir);

    if (recursive)
    {
	list = NULL;
	path_list (dir, NULL, &list);
	work = list;

	while (work)
	{
	    const gchar *path = work->data;

	    work = work->next;
	    still_have_a_file |=
		cache_maintain_dir (type, path, recursive, clear);
	}

	path_list_free (list);
    }

    return still_have_a_file;
}

static void
cache_file_move (const gchar * src, const gchar * dest)
{
    if (!dest || !src || !isfile (src))
	return;

    if (!move_file (src, dest))
    {
	/*
	 * we remove it anyway - it's stale 
	 */
	unlink (src);
    }
}

void
cache_maint_moved (CacheType type, const gchar * src, const gchar * dest)
{
    gchar  *base;
    mode_t  mode = 0755;

    if (!src || !dest)
	return;

    base = cache_get_location (type, dest, FALSE, NULL, &mode);

    if (cache_ensure_dir_exists (base, mode))
    {
	gchar  *buf;
	gchar  *d;

	if (type == CACHE_THUMBS)
	{
	    buf = cache_find_location (type, src, PORNVIEW_CACHE_THUMB_EXT);
	    d = cache_get_location (type, dest, TRUE,
				    PORNVIEW_CACHE_THUMB_EXT, NULL);
	}
	else
	{
	    buf = cache_find_location (type, src, PORNVIEW_CACHE_COMMENT_EXT);
	    d = cache_get_location (type, dest, TRUE,
				    PORNVIEW_CACHE_COMMENT_EXT, NULL);
	}

	cache_file_move (buf, d);
	g_free (d);
	g_free (buf);
    }
    else
	g_free (base);
}

static void
cache_file_remove (const gchar * path)
{
    if (path && isfile (path) && unlink (path) < 0)
    {
	printf ("Failed to remove cache file %s\n", path);
    }
}

void
cache_maint_removed (CacheType type, const gchar * source)
{
    gchar  *buf;

    if (type == CACHE_THUMBS)
	buf = cache_find_location (type, source, PORNVIEW_CACHE_THUMB_EXT);
    else
	buf = cache_find_location (type, source, PORNVIEW_CACHE_COMMENT_EXT);

    cache_file_remove (buf);
    g_free (buf);
}
