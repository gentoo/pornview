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

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "file_utils.h"
#include "file_type.h"
#include "intl.h"

#define MAX_PATH_LEN 1024

gchar  *
home_dir (void)
{
    gchar  *home = getenv ("HOME");

    if (home)
	return home;
    else
    {
	struct passwd *pw = getpwuid (getuid ());

	if (pw)
	    return pw->pw_dir;
	else
	    return NULL;
    }
}

gboolean
isname (const gchar * s)
{
    struct stat st;

    if ((!s) || (!*s))
	return FALSE;

    if (stat (s, &st) < 0)
	return FALSE;

    return TRUE;
}

gboolean
isfile (const gchar * path)
{
    struct stat st;

    if ((!path) || (!*path))
	return FALSE;

    if (stat (path, &st) < 0)
	return FALSE;

    if (S_ISREG (st.st_mode))
	return TRUE;

    return FALSE;
}

gboolean
isdir (const gchar * path)
{
    struct stat st;

    if ((!path) || (!*path))
	return FALSE;

    if (stat (path, &st) < 0)
	return FALSE;

    if (S_ISDIR (st.st_mode))
	return TRUE;

    return FALSE;
}

gboolean
islink (const gchar * path)
{
    struct stat st;

    if (lstat (path, &st) < 0)
	return FALSE;

    if (S_ISLNK (st.st_mode))
	return TRUE;

    return FALSE;
}

gboolean
iswritable (const gchar * path)
{
    gint    uid = getuid ();
    gint    gid = getgid ();
    struct stat st;

    if (stat (path, &st))
	return FALSE;

    if (!S_ISDIR (st.st_mode))
	return FALSE;

    if (uid == st.st_uid && (st.st_mode & S_IWUSR))
	return TRUE;
    else if (gid == st.st_gid && (st.st_mode & S_IWGRP))
	return TRUE;
    else if (st.st_mode & S_IWOTH)
	return TRUE;

    return FALSE;
}

gboolean
isexecutable (const char *path)
{
    struct stat st;

    if (stat (path, &st))
	return FALSE;

    if (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)
    {
	return TRUE;
    }

    return FALSE;
}

gboolean
isempty (const gchar * path)
{
    DIR    *dp;
    struct dirent *dir;

    if ((dp = opendir (path)) == NULL)
	return FALSE;

    while ((dir = readdir (dp)) != NULL)
    {
	gchar  *name = dir->d_name;

	if (dir->d_ino > 0 &&
	    !(name[0] == '.'
	      && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))))
	{
	    closedir (dp);
	    return FALSE;
	}
    }

    closedir (dp);

    return TRUE;
}

gboolean
file_exists (const char *path)
{
    struct stat st;

    if ((!path) || (!*path))
	return FALSE;
    if (stat (path, &st) < 0)
	return FALSE;
    return TRUE;
}

time_t
filetime (const gchar * path)
{
    struct stat st;

    if ((!path) || (!*path))
	return 0;

    if (stat (path, &st) < 0)
	return 0;

    return st.st_mtime;
}

gint
filesize (const gchar * path)
{
    struct stat st;

    if ((!path) || (!*path))
	return 0;

    if (stat (path, &st) < 0)
	return 0;

    return (int) st.st_size;
}

gchar  *
extension (const gchar * path)
{
    gchar  *p = strrchr (path, '.');

    if (p == NULL)
	return NULL;
    else
	return p + 1;
}

gboolean
extension_is (const char *filename, const char *ext)
{
    int     len1, len2;

    if (!filename)
	return FALSE;
    if (!*filename)
	return FALSE;
    if (!ext)
	return FALSE;
    if (!*ext)
	return FALSE;

    len1 = strlen (filename);
    len2 = strlen (ext);

    if (len1 < len2)
	return FALSE;

    return !strcasecmp (filename + len1 - len2, ext);
}

gboolean
extension_truncate (gchar * path, const gchar * ext)
{
    gint    l;
    gint    el;

    if (!path || !ext)
	return FALSE;

    l = strlen (path);
    el = strlen (ext);

    if (l < el || strcmp (path + (l - el), ext) != 0)
	return FALSE;

    path[l - el] = '\0';

    return TRUE;
}

gchar  *
extension_find_dot (gchar * path)
{
    gchar  *ptr;

    if (!path || *path == '\0')
	return NULL;

    ptr = path;
    while (*ptr != '\0')
	ptr++;

    while (ptr > path && *ptr != '.')
	ptr--;

    if (ptr == path)
	return NULL;

    return ptr;
}

const gchar *
extension_from_path (const gchar * path)
{
    if (!path)
	return NULL;

    return strrchr (path, '.');
}

gchar  *
remove_extension_from_path (const gchar * path)
{
    gchar  *new_path;
    const gchar *ptr = path;
    gint    p;

    if (!path)
	return NULL;

    if (strlen (path) < 2)
	return g_strdup (path);

    p = strlen (path) - 1;

    while (ptr[p] != '.' && p > 0)
	p--;

    if (p == 0)
	p = strlen (path) - 1;

    new_path = g_strndup (path, (guint) p);

    return new_path;
}

gboolean
makedir (const gchar * dir)
{
    if ((!dir) || (!*dir))
	return FALSE;

    if (!mkdir (dir, S_IRWXU))
	return TRUE;

    return FALSE;
}

gboolean
mkdirs (const char *path)
{
    char    ss[MAX_PATH_LEN];
    int     i, ii;

    i = 0;
    ii = 0;

    while (path[i] && i < MAX_PATH_LEN)
    {
	ss[ii++] = path[i];
	ss[ii] = '\0';

	if (path[i] == '/')
	{
	    if (!file_exists (ss))
	    {
		if (!makedir (ss))
		    return FALSE;
	    }
	    else if (!isdir (ss))
		return FALSE;
	}
	i++;
    }

    return TRUE;
}

gboolean
copy_file_attributes (const gchar * s, const gchar * t)
{
    struct stat st;
    gboolean ret = FALSE;

    if (stat (s, &st) == 0)
    {
	struct utimbuf tb;

	ret = TRUE;

	/*
	 * set the dest file attributes to that of source (ignoring errors) 
	 */

	if (chown (t, st.st_uid, st.st_gid) < 0)
	    ret = FALSE;

	if (chmod (t, st.st_mode) < 0)
	    ret = FALSE;

	tb.actime = st.st_atime;
	tb.modtime = st.st_mtime;

	if (utime (t, &tb) < 0)
	    ret = FALSE;
    }

    return ret;
}

gboolean
copy_file (const gchar * s, const gchar * t)
{
    FILE   *fi, *fo;
    gchar   buf[4096];
    gint    b;

    fi = fopen (s, "rb");

    if (!fi)
    {
	return FALSE;
    }

    fo = fopen (t, "wb");

    if (!fo)
    {
	fclose (fi);
	return FALSE;
    }

    while ((b = fread (buf, sizeof (char), 4096, fi)) && b != 0)
    {
	if (fwrite (buf, sizeof (char), b, fo) != b)
	{
	    fclose (fi);
	    fclose (fo);
	    return FALSE;
	}

	while (gtk_events_pending ())
	    gtk_main_iteration ();
    }

    fclose (fi);
    fclose (fo);

    copy_file_attributes (s, t);

    return TRUE;
}

gboolean
move_file (const gchar * s, const gchar * t)
{
    if (rename (s, t) < 0)
    {
	/*
	 * this may have failed because moving a file across filesystems
	 * was attempted, so try copy and delete instead 
	 */
	if (copy_file (s, t))
	{
	    if (unlink (s) < 0)
	    {
		/*
		 * err, now we can't delete the source file so return FALSE 
		 */
		return FALSE;
	    }
	}
	else
	    return FALSE;
    }

    return TRUE;
}

gboolean
path_list (const gchar * path, GList ** files, GList ** dirs)
{
    DIR    *dp;
    struct dirent *dir;
    struct stat ent_sbuf;
    GList  *f_list = NULL;
    GList  *d_list = NULL;

    if (!path)
	return FALSE;

    if ((dp = opendir (path)) == NULL)
    {
	/*
	 * dir not found 
	 */
	return FALSE;
    }

    /*
     * root dir fix 
     */
    if (path[0] == '/' && path[1] == '\0')
	path = "";

    while ((dir = readdir (dp)) != NULL)
    {
	/*
	 * skip removed files 
	 */
	if (dir->d_ino > 0)
	{
	    gchar  *name = dir->d_name;
	    gchar  *filepath = g_strconcat (path, "/", name, NULL);

	    if (stat (filepath, &ent_sbuf) >= 0)
	    {
		if (dirs && S_ISDIR (ent_sbuf.st_mode) &&
		    !(name[0] == '.'
		      && (name[1] == '\0'
			  || (name[1] == '.' && name[2] == '\0'))))
		{
		    d_list = g_list_prepend (d_list, filepath);
		    filepath = NULL;
		}
		else if (files && S_ISREG (ent_sbuf.st_mode))
		{
		    f_list = g_list_prepend (f_list, filepath);
		    filepath = NULL;
		}
		g_free (filepath);
	    }
	}
    }
    closedir (dp);

    if (dirs)
	*dirs = g_list_reverse (d_list);

    if (files)
	*files = g_list_reverse (f_list);

    return TRUE;
}

void
path_list_free (GList * list)
{
    g_list_foreach (list, (GFunc) g_free, NULL);
    g_list_free (list);
}

gchar  *
unique_filename (const gchar * path, const gchar * ext, const gchar * divider,
		 gint pad)
{
    gchar  *unique;
    gint    n = 1;

    if (!ext)
	ext = "";

    if (!divider)
	divider = "";

    unique = g_strconcat (path, ext, NULL);

    while (isname (unique))
    {
	g_free (unique);
	if (pad)
	{
	    unique = g_strdup_printf ("%s%s%03d%s", path, divider, n, ext);
	}
	else
	{
	    unique = g_strdup_printf ("%s%s%d%s", path, divider, n, ext);
	}

	n++;

	if (n > 999)
	{
	    /*
	     * well, we tried
	     */
	    g_free (unique);
	    return NULL;
	}
    }

    return unique;
}

gchar  *
unique_filename_simple (const gchar * path)
{
    gchar  *unique;
    const gchar *name;
    const gchar *ext;

    if (!path)
	return NULL;

    name = filename_from_path (path);

    if (!name)
	return NULL;

    ext = extension_from_path (name);

    if (!ext)
    {
	unique = unique_filename (path, NULL, "_", TRUE);
    }
    else
    {
	gchar  *base;

	base = remove_extension_from_path (path);
	unique = unique_filename (base, ext, "_", TRUE);
	g_free (base);
    }

    return unique;
}

const gchar *
filename_from_path (const gchar * path)
{
    if (!path)
	return NULL;

    return g_basename (path);
}

gchar  *
remove_level_from_path (const gchar * path)
{
    gchar  *new_path;
    const gchar *ptr = path;
    gint    p;

    if (!path)
	return NULL;

    p = strlen (path) - 1;

    if (p < 0)
	return NULL;

    while (ptr[p] != '/' && p > 0)
	p--;

    if (p == 0 && ptr[p] == '/')
	p++;

    new_path = g_strndup (path, (guint) p);

    return new_path;
}

gchar  *
concat_dir_and_file (const gchar * base, const gchar * name)
{
    if (!base || !name)
	return NULL;

    if (strcmp (base, "/") == 0)
	return g_strconcat (base, name, NULL);

    return g_strconcat (base, "/", name, NULL);
}

gchar  *
size2str (size_t size, int space)
{
    size_t  i = 0;
    gint    j = 0;
    gint    n_digit = 0;
    gchar   tmp[14];
    gchar   comma[14];
    gchar   buf[14];

    i = size;

    /*
     * detect digit num 
     */
    while (i > 0)
    {
	i = i / 10;
	n_digit++;
    }

    sprintf (tmp, "%d", size);

    if (strlen (tmp) < 4)
	return g_strdup (tmp);

    /*
     * until first comma 
     */
    if (n_digit % 3 != 0)
    {
	for (i = 0; i < n_digit % 3; i++)
	    comma[j++] = tmp[i];
	if (i != strlen (tmp))
	    comma[j++] = ',';
    }

    /*
     * until end of string 
     */
    while (tmp[i] != '\0')
    {
	comma[j++] = tmp[i++];
	comma[j++] = tmp[i++];
	comma[j++] = tmp[i++];
	if (tmp[i] != '\0')
	    comma[j++] = ',';
    }

    /*
     * end of string 
     */
    comma[j] = '\0';

    if (space)
    {
	snprintf (buf, 14, "%13s", comma);
	return g_strdup (buf);
    }
    else
    {
	return g_strdup (comma);
    }
}

gchar  *
time2str (time_t time)
{
    struct tm *jst = localtime (&time);
    gchar  *week[7] =
	{ _("Sun"), _("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"),
	_("Sat")
    };
    gchar   timestamp[256];

    snprintf (timestamp, 256, "%4d/%02d/%02d %s %02d:%02d",
	      jst->tm_year + 1900, jst->tm_mon + 1, jst->tm_mday,
	      week[jst->tm_wday], jst->tm_hour, jst->tm_min);

    return g_strdup (timestamp);
}

gchar  *
uid2str (uid_t uid)
{
    struct passwd *pw = getpwuid (uid);
    gchar   buf[16];

    if (pw)
    {
	return g_strdup (pw->pw_name);
    }
    else
    {
	snprintf (buf, 16, "%d", uid);
	return g_strdup (buf);
    }
}

gchar  *
gid2str (gid_t gid)
{
    struct group *gr = getgrgid (gid);
    gchar   buf[16];

    if (gr)
    {
	return g_strdup (gr->gr_name);
    }
    else
    {
	snprintf (buf, 16, "%d", gid);
	return g_strdup (buf);
    }
}

gchar  *
mode2str (mode_t mode)
{
    gchar   permission[11] = { "----------" };

    switch (mode & S_IFMT)
    {
      case S_IFREG:
	  permission[0] = '-';
	  break;
      case S_IFLNK:
	  permission[0] = 'l';
	  break;
      case S_IFDIR:
	  permission[0] = 'd';
	  break;
      default:
	  permission[0] = '?';
	  break;
    }

    if (mode & S_IRUSR)
	permission[1] = 'r';
    if (mode & S_IWUSR)
	permission[2] = 'w';
    if (mode & S_IXUSR)
	permission[3] = 'x';

    if (mode & S_IRGRP)
	permission[4] = 'r';
    if (mode & S_IWGRP)
	permission[5] = 'w';
    if (mode & S_IXGRP)
	permission[6] = 'x';

    if (mode & S_IROTH)
	permission[7] = 'r';
    if (mode & S_IWOTH)
	permission[8] = 'w';
    if (mode & S_IXOTH)
	permission[9] = 'x';

    if (mode & S_ISUID)
	permission[3] = 'S';
    if (mode & S_ISGID)
	permission[6] = 'S';
    if (mode & S_ISVTX)
	permission[9] = 'T';

    permission[11] = 0;

    return g_strdup (permission);
}

gboolean getting_dir = FALSE;
gboolean stop_getting_dir = FALSE;

static  gint
comp_spel (gconstpointer data1, gconstpointer data2)
{
    const gchar *str1 = data1;
    const gchar *str2 = data2;

    return strcmp (str1, str2);
}

/*
 *  get_dir:
 *     @ Get image files in specified directory.
 *
 *  dirname  : Directory to scan.
 *  files    : Pointer to OpenFiles struct for store directory list.
 */
void
get_dir (const gchar * dirname, GetDirFlags flags,
	 GList ** filelist_ret, GList ** dirlist_ret)
{
    DIR    *dp;
    struct dirent *entry;
    gchar   buf[MAX_PATH_LEN], *path;
    GList  *filelist = NULL, *dirlist = NULL, *list;

    g_return_if_fail (dirname && *dirname);

    getting_dir = TRUE;

    if (flags & GETDIR_DISP_STDERR)
	fprintf (stderr, _("scandir = %s\n"), dirname);
    else if (flags & GETDIR_DISP_STDOUT)
	fprintf (stdout, _("scandir = %s\n"), dirname);

    if ((dp = opendir (dirname)))
    {
	while ((entry = readdir (dp)))
	{
	    if (flags & GETDIR_ENABLE_CANCEL)
	    {
		while (gtk_events_pending ())
		    gtk_main_iteration ();
		if (stop_getting_dir)
		    break;
	    }

	    /*
	     * ignore dot file 
	     */
	    if (!(flags & GETDIR_READ_DOT) && entry->d_name[0] == '.')
		continue;

	    /*
	     * get full path 
	     */
	    if (dirname[strlen (dirname) - 1] == '/')
		g_snprintf (buf, MAX_PATH_LEN, "%s%s", dirname,
			    entry->d_name);
	    else
		g_snprintf (buf, MAX_PATH_LEN, "%s/%s", dirname,
			    entry->d_name);

	    /*
	     * if path is file 
	     */
	    if (!isdir (buf)
		|| (!(flags & GETDIR_FOLLOW_SYMLINK) && islink (buf)))
	    {
		if (!filelist_ret)
		    continue;

		if (!(flags & GETDIR_DETECT_EXT)
		    || file_type_detect_type_by_ext (buf))
		{
		    path = g_strdup (buf);

		    if (flags & GETDIR_DISP_STDERR)
			fprintf (stderr, _("filename = %s\n"), path);
		    else if (flags & GETDIR_DISP_STDOUT)
			fprintf (stdout, _("filename = %s\n"), path);

		    filelist = g_list_append (filelist, path);
		}

		/*
		 * if path is dir 
		 */
	    }
	    else if (isdir (buf))
	    {
		if (dirlist_ret && strcmp (entry->d_name, ".")
		    && strcmp (entry->d_name, ".."))
		{
		    path = g_strdup (buf);

		    if (flags & GETDIR_DISP_STDERR)
			fprintf (stderr, _("dirname = %s\n"), path);
		    else if (flags & GETDIR_DISP_STDOUT)
			fprintf (stdout, _("dirname = %s\n"), path);

		    dirlist = g_list_append (dirlist, path);
		}
	    }
	}
	closedir (dp);
	if (filelist)
	    filelist = g_list_sort (filelist, comp_spel);
	if (dirlist)
	    dirlist = g_list_sort (dirlist, comp_spel);
    }
    else
    {
	g_warning ("cannot open directory: %s", dirname);
    }

    /*
     * recursive get 
     */
    if (flags & GETDIR_RECURSIVE)
    {
	GList  *tmplist = g_list_copy (dirlist);
	gint    tmp_flags = flags | GETDIR_RECURSIVE_IS_BRANCH;

	list = tmplist;
	while (list)
	{
	    GList  *tmp_filelist = NULL, *tmp_dirlist = NULL;
	    if (flags & GETDIR_ENABLE_CANCEL)
	    {
		while (gtk_events_pending ())
		    gtk_main_iteration ();
		if (stop_getting_dir)
		    break;
	    }
	    get_dir ((const gchar *) list->data, tmp_flags,
		     &tmp_filelist, &tmp_dirlist);
	    filelist = g_list_concat (filelist, tmp_filelist);
	    dirlist = g_list_concat (dirlist, tmp_dirlist);
	    list = g_list_next (list);
	}
	g_list_free (tmplist);
    }

    /*
     * return value 
     */
    if (filelist_ret)
	*filelist_ret = filelist;
    if (dirlist_ret)
	*dirlist_ret = dirlist;

    if (!(flags & GETDIR_RECURSIVE_IS_BRANCH))
    {
	getting_dir = FALSE;
	stop_getting_dir = FALSE;
    }
}

void
get_dir_stop (void)
{
    if (getting_dir)
	stop_getting_dir = TRUE;
}
