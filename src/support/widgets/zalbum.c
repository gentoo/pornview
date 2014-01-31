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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include "gtk2-compat.h"
#include "zalbum.h"

#define ZALBUM_CELL(ptr) ((ZAlbumCell *) (ptr))

#define bw(widget)   GTK_CONTAINER(widget)->border_width

#define CELL_PADDING 4
#define LINE_PADDING 2
#define LINE_HEIGHT(font) ((font)->ascent + (font)->descent + LINE_PADDING)
#define STRING_BUFFER_SIZE 1024

#define CELL_STATE(cell)  (ZALBUM_CELL (cell)->flags & ZALBUM_CELL_SELECTED ? GTK_STATE_SELECTED : GTK_STATE_NORMAL)

typedef void (*DrawFunc) (ZAlbum * album, ZAlbumCell * cell,
			  GdkRectangle * cell_area, GdkRectangle * area);

static void zalbum_class_init (ZAlbumClass * klass);
static void zalbum_init (ZAlbum * album);

static void zalbum_finalize (GObject * object);

static void zalbum_clear (ZList * list);
static void zalbum_size_allocate (GtkWidget * widget,
				  GtkAllocation * allocation);
static void zalbum_cell_size_request (ZList * list, gpointer cell,
				      GtkRequisition * requisition);
static void zalbum_draw_cell (ZList * list, gpointer cell,
			      GdkRectangle * cell_area, GdkRectangle * area);
static void zalbum_draw_list (ZAlbum * album, ZAlbumCell * cell,
			      GdkRectangle * cell_area, GdkRectangle * area);
static void zalbum_draw_preview (ZAlbum * album, ZAlbumCell * cell,
				 GdkRectangle * cell_area,
				 GdkRectangle * area);
static void zalbum_prepare_cell (ZAlbum * album, ZAlbumCell * cell,
				 GdkRectangle * cell_area,
				 GdkRectangle * area);
static void zalbum_cell_draw_focus (ZList * list, gpointer cell,
				    GdkRectangle * cell_area);
static void zalbum_cell_draw_default (ZList * list, gpointer cell,
				      GdkRectangle * cell_area);
static void zalbum_cell_select (ZList * list, gpointer cell);
static void zalbum_cell_unselect (ZList * list, gpointer cell);
static void make_string (GdkFont * font, char *string, int max_width,
			 char *buffer, int buffer_size);
static void zalbum_draw_string (GtkWidget * widget, ZAlbumCell * cell,
				char *string, int x, int y,
				int max_width, int center);

static GtkWidgetClass *parent_class = NULL;

static ZAlbumColumnInfo default_cinfo[ZALBUM_COLUMNS] = {
    {200, 10, 0},
    {80, 10, 0},
    {80, 10, 0}
};

static DrawFunc zalbum_draw_funcs[] = {
    zalbum_draw_list,
    zalbum_draw_preview,
    NULL
};

GtkType
zalbum_get_type ()
{
    static GtkType type = 0;

    if (!type)
    {
	static GtkTypeInfo info = {
	    "ZAlbum",
	    sizeof (ZAlbum),
	    sizeof (ZAlbumClass),
	    (GtkClassInitFunc) zalbum_class_init,
	    (GtkObjectInitFunc) zalbum_init,
	    /*
	     * reserved 1
	     */
	    NULL,
	    /*
	     * reserved 2
	     */
	    NULL,
	    (GtkClassInitFunc) NULL
	};

	type = gtk_type_unique (zlist_get_type (), &info);
    }

    return type;
}

static void
zalbum_class_init (ZAlbumClass * klass)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;
    ZListClass *zlist_class;

    parent_class = gtk_type_class (zlist_get_type ());

    object_class = (GtkObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    zlist_class = (ZListClass *) klass;

    OBJECT_CLASS_SET_FINALIZE_FUNC (klass, zalbum_finalize);

    widget_class->size_allocate = zalbum_size_allocate;

    zlist_class->clear = zalbum_clear;
    zlist_class->cell_draw = zalbum_draw_cell;
    zlist_class->cell_size_request = zalbum_cell_size_request;
    zlist_class->cell_draw_focus = zalbum_cell_draw_focus;
    zlist_class->cell_draw_default = zalbum_cell_draw_default;
    zlist_class->cell_select = zalbum_cell_select;
    zlist_class->cell_unselect = zalbum_cell_unselect;
}

static void
zalbum_init (ZAlbum * zalbum)
{
    zalbum->mode = ZALBUM_MODE_LIST;
    zalbum->max_pix_width = 100;
    zalbum->max_pix_height = 100;
}

GtkWidget *
zalbum_new (ZAlbumMode mode)
{
    ZAlbum *album;

    g_return_val_if_fail (mode < ZALBUM_MODES, NULL);

    album = (ZAlbum *) gtk_type_new (zalbum_get_type ());
    g_return_val_if_fail (album != NULL, NULL);

    memcpy (&album->cinfo, &default_cinfo, sizeof (default_cinfo));

    zlist_construct (ZLIST (album), 0);
    zlist_set_selection_mode (ZLIST (album), GTK_SELECTION_EXTENDED);
    zalbum_set_mode (album, mode);

    album->len = 0;

    return (GtkWidget *) album;
}

static void
zalbum_finalize (GObject * object)
{
    zalbum_clear (ZLIST (object));

    OBJECT_CLASS_FINALIZE_SUPER (parent_class, object);
}

guint
zalbum_add (ZAlbum * album, const gchar * name, gpointer data,
	    GtkDestroyNotify destroy)
{
    ZAlbumCell *cell;

    g_return_val_if_fail (IS_ZALBUM (album), 0);

    cell = g_new (ZAlbumCell, 1);

    if (name)
	cell->name = g_strdup (name);
    else
	cell->name = NULL;

    cell->ipix = NULL;
    cell->flags = 0;

    if (data)
	cell->user_data = data;
    else
	cell->user_data = NULL;

    if (destroy)
	cell->destroy = destroy;
    else
	cell->destroy = NULL;

    album->len++;

    return zlist_add (ZLIST (album), cell);
}

void
zalbum_set_mode (ZAlbum * album, ZAlbumMode mode)
{
    GtkRequisition requisition;
    g_return_if_fail (album != NULL && mode >= 0 && mode < ZALBUM_MODES);

    if (album->mode == mode)
	return;

    album->mode = mode;
    zalbum_cell_size_request (ZLIST (album), NULL, &requisition);

    switch (mode)
    {
      case ZALBUM_MODE_LIST:
	  zlist_set_cell_padding (ZLIST (album), 0, 0);
	  zlist_set_cell_size (ZLIST (album), -1, requisition.height);
	  zlist_set_1 (ZLIST (album), 1);
	  break;
      case ZALBUM_MODE_PREVIEW:
	  zlist_set_cell_padding (ZLIST (album), 4, 4);
	  zlist_set_1 (ZLIST (album), 0);
	  zlist_set_cell_size (ZLIST (album), requisition.width,
			       requisition.height);
	  break;
      default:
	  break;
    }
}

static void
zalbum_clear (ZList * list)
{
    ZAlbumCell *cell;
    gint    i;

    g_return_if_fail (list);

    for (i = 0; i < list->cell_count; i++)
    {
	cell = ZLIST_CELL_FROM_INDEX (list, i);
	g_free ((gpointer) cell->name);

	if (cell->ipix)
	    gdk_pixmap_unref (cell->ipix);

	cell->ipix = NULL;

	if (cell->user_data && cell->destroy)
	    cell->destroy (cell->user_data);

	cell->destroy = NULL;
	cell->user_data = NULL;

	g_free (cell);
    }

    ZALBUM (list)->len = 0;
}

static void
zalbum_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
    ZAlbum *album;
    int     i, w = 0;

    album = ZALBUM (widget);

    /*
     * expand the first column
     */
    for (i = 1; i < ZALBUM_COLUMNS; i++)
    {
	w += album->cinfo[i].pad;
	w += album->cinfo[i].width;
    }

    album->cinfo[0].width = allocation->width - w - album->cinfo[0].pad;

    if (parent_class && parent_class->size_allocate)
	(*parent_class->size_allocate) (widget, allocation);
}

static void
zalbum_cell_size_request (ZList * list, gpointer cell,
			  GtkRequisition * requisition)
{
    ZAlbum *album;
    int     i, w = 0;

    g_return_if_fail (list && requisition);

    album = ZALBUM (list);

    switch (album->mode)
    {
      case ZALBUM_MODE_LIST:
	  for (i = 0; i < ZALBUM_COLUMNS; i++)
	  {
	      w += album->cinfo[i].pad;
	      w += album->cinfo[i].width;
	  }
	  requisition->width = w + 2 * CELL_PADDING;
	  requisition->height =
	      LINE_HEIGHT (gtk_style_get_font (GTK_WIDGET (list)->style)) +
	      2 * CELL_PADDING;
	  break;

      case ZALBUM_MODE_PREVIEW:

	  requisition->width = album->max_pix_width + 2 * CELL_PADDING;
	  requisition->height = album->max_pix_height +
	      LINE_HEIGHT (gtk_style_get_font (GTK_WIDGET (list)->style)) +
	      2 * CELL_PADDING;
	  break;

      default:
	  break;
    }
}

static void
zalbum_draw_cell (ZList * list, gpointer cell, GdkRectangle * cell_area,
		  GdkRectangle * area)
{
    g_return_if_fail (list && cell);

    (*zalbum_draw_funcs[ZALBUM (list)->mode]) ((ZAlbum *) list,
					       (ZAlbumCell *) cell,
					       cell_area, area);
}

static void
zalbum_draw_list (ZAlbum * album, ZAlbumCell * cell,
		  GdkRectangle * cell_area, GdkRectangle * area)
{

    GtkWidget *widget;
    int     xdest, ydest;

    widget = GTK_WIDGET (album);

    zalbum_prepare_cell (album, cell, cell_area, area);

    ydest =
	cell_area->y + gtk_style_get_font (widget->style)->ascent +
	LINE_PADDING / 2;
    xdest = cell_area->x + album->cinfo[0].pad;

    zalbum_draw_string (widget, cell, g_basename (cell->name),
			xdest, ydest, album->cinfo[0].width, 0);
}

static void
zalbum_draw_preview (ZAlbum * album, ZAlbumCell * cell,
		     GdkRectangle * cell_area, GdkRectangle * area)
{
    GtkWidget *widget;
    GdkRectangle intersect_area;
    int     text_area_height, ydest;
    int     xpad, ypad;
    GdkRectangle widget_area, draw_area;

    g_return_if_fail (album && cell && cell_area);

    widget = GTK_WIDGET (album);

    if (!GTK_WIDGET_DRAWABLE (widget))
	return;

    widget_area.x = widget_area.y = 0;
    widget_area.width = widget->allocation.width;
    widget_area.height = widget->allocation.height;

    if (!area)
    {
	draw_area = widget_area;
    }
    else
    {
	if (!gdk_rectangle_intersect (area, &widget_area, &draw_area))
	    return;
    }

    zalbum_prepare_cell (album, cell, cell_area, &draw_area);

    text_area_height = LINE_HEIGHT (gtk_style_get_font (widget->style));

    if (cell->ipix)
    {
	GdkRectangle pixmap_area;
	int     w, h;

	w = cell->width;
	h = cell->height;
	xpad = (cell_area->width - w) / 2;
	ypad = (cell_area->height - text_area_height - h) / 2;

	if (xpad < 0)
	{
	    w += xpad;
	    xpad = 0;
	}

	if (ypad < 0)
	{
	    h += ypad;
	    ypad = 0;
	}

	pixmap_area.x = cell_area->x + xpad;
	pixmap_area.y = cell_area->y + ypad;
	pixmap_area.width = w;
	pixmap_area.height = h;

	if (gdk_rectangle_intersect (area, &pixmap_area, &intersect_area))
	{

	    if (cell->imask)
	    {
		gdk_gc_set_clip_mask (widget->style->fg_gc[GTK_STATE_NORMAL],
				      cell->imask);
		gdk_gc_set_clip_origin (widget->style->
					fg_gc[GTK_STATE_NORMAL],
					pixmap_area.x, pixmap_area.y);
	    }

	    gdk_draw_pixmap (widget->window,
			     widget->style->fg_gc[GTK_STATE_NORMAL],
			     cell->ipix,
			     intersect_area.x - pixmap_area.x,
			     intersect_area.y - pixmap_area.y,
			     intersect_area.x, intersect_area.y,
			     intersect_area.width, intersect_area.height);

	    if (cell->imask)
	    {
		gdk_gc_set_clip_origin (widget->style->
					fg_gc[GTK_STATE_NORMAL], 0, 0);
		gdk_gc_set_clip_mask (widget->style->fg_gc[GTK_STATE_NORMAL],
				      NULL);
	    }
	}
    }

    ydest = (cell_area->y + cell_area->height - text_area_height) +
	gtk_style_get_font (widget->style)->ascent + LINE_PADDING;
    zalbum_draw_string (widget, cell, g_basename (cell->name),
			cell_area->x, ydest, cell_area->width, 1);

    /*
     * ydest += LINE_HEIGHT(widget->style->font);
     *
     * snprintf (buff, 64, "%dx%dx%d", cell->image->width, cell->image->height, cell->image->bpp * 8);
     *
     * zalbum_draw_string (widget, cell, buff,
     * cell_area->x, ydest, cell_area->width, 1);
     */
}

static void
make_string (GdkFont * font, char *string, int max_width, char *buffer,
	     int buffer_size)
{
    int     s, e, l, m = 0;

    s = 0;
    e = l = strlen (string);
    while (e - s > 1)
    {
	m = s + (e - s) / 2;
	if (gdk_text_width (font, string, m) > max_width)
	    e = m;
	else
	    s = m;
    }

    if (m + 4 > buffer_size)
	m = buffer_size - 4;

    if (m < l)
    {
	if (m < 4)
	{
	    *buffer = 0;
	    return;
	};
	memcpy (buffer, string, m - 3);
	buffer[m - 1] = '.';
	buffer[m - 2] = '.';
	buffer[m - 3] = '.';
    }
    else
	memcpy (buffer, string, m);

    buffer[m] = 0;
}

static void
zalbum_draw_string (GtkWidget * widget, ZAlbumCell * cell, char *string,
		    int x, int y, int max_width, int center)
{
    char    buffer[STRING_BUFFER_SIZE];
    int     x_pad;

    x_pad =
	(max_width -
	 gdk_string_width (gtk_style_get_font (widget->style), string)) / 2;

    if (x_pad < 0)
    {
	make_string (gtk_style_get_font (widget->style), string, max_width,
		     buffer, STRING_BUFFER_SIZE);
	string = buffer;
	x_pad = 0;
    }

    if (!center)
	x_pad = 0;

    gdk_draw_string (widget->window, gtk_style_get_font (widget->style),
		     widget->style->fg_gc[CELL_STATE (cell)],
		     x + x_pad, y, string);
}

static void
zalbum_prepare_cell (ZAlbum * album, ZAlbumCell * cell,
		     GdkRectangle * cell_area, GdkRectangle * area)
{
    GtkWidget *widget;
    GdkRectangle intersect_area;

    widget = GTK_WIDGET (album);
    /*
     * gdk_draw_rectangle (widget->window,
     * widget->style->dark_gc [GTK_STATE_NORMAL],
     * FALSE, cell_area->x, cell_area->y,
     * cell_area->width - 1, cell_area->height - 1);
     *
     * cell_area->x += 1; cell_area->y += 1; cell_area->width -= 2; cell_area->height -= 2;
     *
     * gdk_draw_rectangle (widget->window,
     * widget->style->light_gc [GTK_STATE_NORMAL],
     * FALSE, cell_area->x, cell_area->y,
     * cell_area->width, cell_area->height);
     *
     * cell_area->x += CELL_PADDING - 1; cell_area->y += CELL_PADDING - 1;
     * cell_area->width -= 2 * CELL_PADDING - 2; cell_area->height -= 2 * CELL_PADDING - 2;
     */

    if (gdk_rectangle_intersect (area, cell_area, &intersect_area))
	gdk_draw_rectangle (widget->window,
			    widget->style->bg_gc[CELL_STATE (cell)],
			    TRUE, intersect_area.x, intersect_area.y,
			    intersect_area.width, intersect_area.height);


    gtk_draw_shadow (widget->style, widget->window,
		     CELL_STATE (cell),
		     GTK_SHADOW_OUT,
		     cell_area->x, cell_area->y,
		     cell_area->width, cell_area->height);

    cell_area->x += CELL_PADDING;
    cell_area->y += CELL_PADDING;
    cell_area->width -= 2 * CELL_PADDING;
    cell_area->height -= 2 * CELL_PADDING;
}

static void
zalbum_cell_draw_focus (ZList * list, gpointer cell, GdkRectangle * cell_area)
{

    gtk_draw_shadow (GTK_WIDGET (list)->style, GTK_WIDGET (list)->window,
		     CELL_STATE (cell),
		     GTK_SHADOW_IN,
		     cell_area->x, cell_area->y,
		     cell_area->width, cell_area->height);
    /*
     * gdk_draw_rectangle (GTK_WIDGET(list)->window,
     * GTK_WIDGET(list)->style->black_gc,
     * FALSE, cell_area->x, cell_area->y,
     * cell_area->width - 1, cell_area->height - 1); */
}

static void
zalbum_cell_draw_default (ZList * list, gpointer cell,
			  GdkRectangle * cell_area)
{
    gtk_draw_shadow (GTK_WIDGET (list)->style, GTK_WIDGET (list)->window,
		     CELL_STATE (cell),
		     GTK_SHADOW_OUT,
		     cell_area->x, cell_area->y,
		     cell_area->width, cell_area->height);
    /*
     * gdk_draw_rectangle (GTK_WIDGET(list)->window,
     * GTK_WIDGET(list)->style->white_gc, //dark_gc [GTK_STATE_NORMAL],
     * FALSE, cell_area->x, cell_area->y,
     * cell_area->width - 1, cell_area->height - 1);
     */
}

static void
zalbum_cell_select (ZList * list, gpointer cell)
{
    ZALBUM_CELL (cell)->flags |= ZALBUM_CELL_SELECTED;
}

static void
zalbum_cell_unselect (ZList * list, gpointer cell)
{
    ZALBUM_CELL (cell)->flags &= ~ZALBUM_CELL_SELECTED;
}

void
zalbum_set_pixmap (ZAlbum * album, guint idx, GdkPixmap * pixmap,
		   GdkBitmap * mask)
{
    ZAlbumCell *cell;

    g_return_if_fail (IS_ZALBUM (album));

    cell = ZLIST_CELL_FROM_INDEX (album, idx);

    g_return_if_fail (cell);

    if (cell->ipix)
	gdk_pixmap_unref (cell->ipix);
    cell->ipix = NULL;
    cell->imask = NULL;

    if (!pixmap)
	return;

    cell->ipix = gdk_pixmap_ref (pixmap);
    if (mask)
	cell->imask = gdk_bitmap_ref (mask);

    if (cell->ipix)
    {
	gdk_window_get_size (cell->ipix, &cell->width, &cell->height);

	album->max_pix_width = MAX (album->max_pix_width, cell->width);
	album->max_pix_height = MAX (album->max_pix_height, cell->height);

	if (!zlist_update_cell_size (ZLIST (album), (gpointer) cell))
	    zlist_draw_cell (ZLIST (album), idx);
    }
}

void
zalbum_set_cell_data (ZAlbum * album, guint idx, gpointer user_data)
{
    zalbum_set_cell_data_full (album, idx, user_data, NULL);
}


void
zalbum_set_cell_data_full (ZAlbum * album,
			   guint idx,
			   gpointer user_data, GtkDestroyNotify destroy)
{
    ZAlbumCell *cell;

    g_return_if_fail (IS_ZALBUM (album));

    if (idx < 0 || idx >= album->len)
	return;

    cell = ZLIST_CELL_FROM_INDEX (ZLIST (album), idx);
    g_return_if_fail (cell);

    cell->user_data = user_data;
    cell->destroy = destroy;
}


gpointer
zalbum_get_cell_data (ZAlbum * album, guint idx)
{
    ZAlbumCell *cell;

    g_return_val_if_fail (IS_ZALBUM (album), NULL);

    if (idx < 0 || idx >= album->len)
	return NULL;

    cell = ZLIST_CELL_FROM_INDEX (ZLIST (album), idx);
    g_return_val_if_fail (cell, NULL);

    return cell->user_data;
}
