/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

gchar          *home_dir (void);
gboolean        isname (const gchar * s);
gboolean        isfile (const gchar * path);
gboolean        isdir (const gchar * path);
gboolean        islink (const gchar * path);
gboolean        iswritable (const gchar * path);
gboolean        isexecutable (const char *path);
gboolean        isempty (const gchar * path);
gboolean        file_exists (const char *path);
time_t          filetime (const gchar * path);
gint            filesize (const gchar * path);
gchar          *extension (const gchar * path);
gboolean        extension_is (const char *filename, const char *ext);
gboolean        extension_truncate (gchar * path, const gchar * ext);
gchar          *extension_find_dot (gchar * path);
const gchar    *extension_from_path (const gchar * path);
gchar          *remove_extension_from_path (const gchar * path);
gboolean        makedir (const gchar * path);
gboolean        mkdirs (const char *path);
gboolean        copy_file_attributes (const gchar * s, const gchar * t);
gboolean        copy_file (const gchar * s, const gchar * t);
gboolean        move_file (const gchar * s, const gchar * t);
gboolean        path_list (const gchar * path, GList ** files, GList ** dirs);
void            path_list_free (GList * list);
gchar          *unique_filename (const gchar * path, const gchar * ext,
				 const gchar * divider, gint pad);
gchar          *unique_filename_simple (const gchar * path);
const gchar    *filename_from_path (const gchar * path);
gchar          *remove_level_from_path (const gchar * path);
gchar          *concat_dir_and_file (const gchar * base, const gchar * name);
gchar          *size2str (size_t size, int space);
gchar          *time2str (time_t time);
gchar          *uid2str (uid_t uid);
gchar          *gid2str (gid_t gid);
gchar          *mode2str (mode_t mode);

typedef enum
{
    GETDIR_READ_DOT = 1 << 0,
    GETDIR_FOLLOW_SYMLINK = 1 << 1,
    GETDIR_DETECT_EXT = 1 << 2,
    GETDIR_GET_ARCHIVE = 1 << 3,	/* use with GETDIR_DETECT_EXT */
    GETDIR_DISP_STDOUT = 1 << 4,
    GETDIR_DISP_STDERR = 1 << 5,
    GETDIR_ENABLE_CANCEL = 1 << 6,
    GETDIR_RECURSIVE = 1 << 7,
    GETDIR_RECURSIVE_IS_BRANCH = 1 << 8	/* for internal use */
}
GetDirFlags;

void            get_dir (const gchar * dirname,
			 GetDirFlags flags,
			 GList ** filelist, GList ** dirlist);
void            get_dir_stop (void);

#endif /* __FILE_UTILS_H__ */
