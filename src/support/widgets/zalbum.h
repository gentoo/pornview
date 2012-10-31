/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 *  These codes are mostly taken from Another X image viewer.
 *
 *  Another X image viewer Author:
 *     David Ramboz <dramboz@users.sourceforge.net>
 */

#ifndef __ZALBUM_H__
#define __ZALBUM_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "zlist.h"

#define ZALBUM_COLUMNS 3

#define ZALBUM_TYPE               (zalbum_get_type ())
#define ZALBUM(widget)            (GTK_CHECK_CAST ((widget), ZALBUM_TYPE, ZAlbum))
#define ZALBUM_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), ZALBUM_TYPE, ZAlbumClass))
#define IS_ZALBUM(widget)	  (GTK_CHECK_TYPE ((widget), ZALBUM_TYPE))
#define IS_ZALBUM_CLASS(klass)	  (GTK_CHECK_CLASS_TYPE ((klass), ZALBUM_TYPE))

typedef struct _ZAlbum ZAlbum;
typedef struct _ZAlbumCell ZAlbumCell;
typedef struct _ZAlbumClass ZAlbumClass;
typedef struct _ZAlbumColumnInfo ZAlbumColumnInfo;

typedef enum
{
    ZALBUM_MODE_LIST,
    ZALBUM_MODE_PREVIEW,
    ZALBUM_MODES
}
ZAlbumMode;

enum
{
    ZALBUM_CELL_SELECTED = 1 << 1,
    ZALBUM_CELL_RENDERED = 1 << 2,
};

struct _ZAlbumColumnInfo
{
    int             width;
    int             pad;
    int             flags;
};

struct _ZAlbum
{
    ZList           list;

    int             mode;

    ZAlbumColumnInfo cinfo[ZALBUM_COLUMNS];

    int             max_pix_width;
    int             max_pix_height;

    gint            len;
};

struct _ZAlbumClass
{
    ZListClass      parent_class;
};

struct _ZAlbumCell
{
    int             flags;
    gchar          *name;

    GdkPixmap      *ipix;
    GdkBitmap      *imask;

    gint            width;
    gint            height;

    gpointer        user_data;
    GtkDestroyNotify destroy;
};

GtkType         zalbum_get_type ();

GtkWidget      *zalbum_new (ZAlbumMode mode);
guint           zalbum_add (ZAlbum * album, const gchar * name, gpointer data,
			    GtkDestroyNotify destroy);
void            zalbum_set_mode (ZAlbum * album, ZAlbumMode mode);

void            zalbum_set_pixmap (ZAlbum * album, guint idx,
				   GdkPixmap * pixmap, GdkBitmap * mask);
void            zalbum_set_cell_data (ZAlbum * album, guint idx,
				      gpointer user_data);
void            zalbum_set_cell_data_full (ZAlbum * album, guint idx,
					   gpointer data,
					   GtkDestroyNotify destroy);
gpointer        zalbum_get_cell_data (ZAlbum * album, guint idx);

#define		zalbum_freeze(album)	(scrolled_freeze (SCROLLED(album)))
#define		zalbum_thawn(album)	(scrolled_thawn  (SCROLLED(album)))

#endif /* __ZALBUM_H__ */
