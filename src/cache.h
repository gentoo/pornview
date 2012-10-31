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

#ifndef __CACHE_H__
#define __CACHE_H__

typedef enum
{
    CACHE_THUMBS,
    CACHE_COMMENTS
}
CacheType;

#define PORNVIEW_RC_DIR_THUMBS     ".pornview/thumbnails"
#define PORNVIEW_RC_DIR_COMMENTS   ".pornview/comments"
#define PORNVIEW_CACHE_DIR         ".thumbnails"
#define PORNVIEW_CACHE_THUMB_EXT   ".png"
#define PORNVIEW_CACHE_COMMENT_EXT ".txt"

gint            cache_ensure_dir_exists (gchar * path, mode_t mode);
gchar          *cache_get_location (CacheType type, const gchar * source,
				    gint include_name, const gchar * ext,
				    mode_t * mode);
gchar          *cache_find_location (CacheType type, const gchar * source,
				     const gchar * ext);

void            cache_maintain_home (CacheType type, gint clear);
gint            cache_maintain_home_dir (CacheType type, const gchar * dir,
					 gint recursive, gint clear);
gint            cache_maintain_dir (CacheType type, const gchar * dir,
				    gint recursive, gint clear);

void            cache_maint_moved (CacheType type, const gchar * src,
				   const gchar * dest);
void            cache_maint_removed (CacheType type, const gchar * source);

#endif /* __CACHE_H__ */
