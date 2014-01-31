/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are based on widget/gimv_player.c in GImageView.
 * GImageView author: Takuro Ashie
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gtkmplayer.h"

#ifdef ENABLE_MPLAYER

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#define GTK_MPLAYER_REFRESH_RATE 100	/* 1/10 [sec] */
#define GTK_MPLAYER_BUF_SIZE 1024

#define gtk_object_class_add_signals(class, func, type)

enum
{
    PLAY_SIGNAL,
    STOP_SIGNAL,
    PAUSE_SIGNAL,
    POS_CHANGED_SIGNAL,
    LAST_SIGNAL
};


typedef struct ChildContext_Tag ChildContext;
typedef void (*ProcessLineFunc) (ChildContext * context,
				 const gchar * line,
				 gint len, gboolean is_stderr);

struct ChildContext_Tag
{
    pid_t   pid;
    GList  *args;
    gint    checker_id;

    GtkMPlayer *player;

    int     stdout_fd;
    int     stderr_fd;
    int     stdin_fd;

    gchar   stdout[GTK_MPLAYER_BUF_SIZE];
    gint    stdout_size;
    gchar   stderr[GTK_MPLAYER_BUF_SIZE];
    gint    stderr_size;

    ProcessLineFunc process_line_fn;
    gpointer data;
    GtkDestroyNotify destroy_fn;
};


typedef struct GetDriversContext_Tag
{
    const gchar *separator;
    gint    sep_len;
    gboolean passing_header;
    GList  *drivers_list;
}
GetDriversContext;

/* object class */
static void gtk_mplayer_class_init (GtkMPlayerClass * class);
static void gtk_mplayer_init (GtkMPlayer * player);
static void gtk_mplayer_destroy (GtkObject * object);

/* widget class */
static void gtk_mplayer_realize (GtkWidget * widget);
static void gtk_mplayer_unrealize (GtkWidget * widget);
static void gtk_mplayer_size_allocate (GtkWidget * widget,
				       GtkAllocation * allocation);

/* mplayer class */
static ChildContext *gtk_mplayer_get_player_child (GtkMPlayer * player);


/* MPlayer handling */
static ChildContext *start_command (GtkMPlayer * player,
				    /*
				     * include command string
				     */
				    GList * arg_list,
				    ProcessLineFunc func,
				    gpointer data,
				    GtkDestroyNotify destroy_fn);
static gint timeout_check_child (gpointer data);
static gboolean process_output (ChildContext * context);

static void child_context_destroy (ChildContext * context);

/* line process functions */
static void process_line (ChildContext * context,
			  const gchar * line, gint len, gboolean is_stderr);
static void process_line_get_drivers (ChildContext * context,
				      const gchar * line,
				      gint len, gboolean is_stderr);

static GtkWidgetClass *parent_class = NULL;
static gint gtk_mplayer_signals[LAST_SIGNAL] = { 0 };

static GHashTable *player_context_table = NULL;
static GHashTable *vo_drivers_table = NULL;
static GHashTable *ao_drivers_table = NULL;

GtkType
gtk_mplayer_get_type (void)
{
    static GtkType gtk_mplayer_type = 0;

#if (GTK_MAJOR_VERSION >= 2)
    if (!gtk_mplayer_type)
    {
	static const GTypeInfo gtk_mplayer_info = {
	    sizeof (GtkMPlayerClass),
	    NULL,		/* base_init */
	    NULL,		/* base_finalize */
	    (GClassInitFunc) gtk_mplayer_class_init,
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof (GtkMPlayer),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) gtk_mplayer_init,
	};

	gtk_mplayer_type
	    = g_type_register_static (GTK_TYPE_WIDGET,
				      "GtkMPlayer", &gtk_mplayer_info, 0);
    }
#else /* (GTK_MAJOR_VERSION >= 2) */
    if (!gtk_mplayer_type)
    {
	static const GtkTypeInfo gtk_mplayer_info = {
	    "GtkMPlayer",
	    sizeof (GtkMPlayer),
	    sizeof (GtkMPlayerClass),
	    (GtkClassInitFunc) gtk_mplayer_class_init,
	    (GtkObjectInitFunc) gtk_mplayer_init,
	    /*
	     * reserved_1
	     */ NULL,
	    /*
	     * reserved_2
	     */ NULL,
	    (GtkClassInitFunc) NULL,
	};

	gtk_mplayer_type
	    = gtk_type_unique (gtk_widget_get_type (), &gtk_mplayer_info);
    }
#endif /* (GTK_MAJOR_VERSION >= 2) */

    return gtk_mplayer_type;
}

static void
gtk_mplayer_class_init (GtkMPlayerClass * class)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass *) class;
    widget_class = (GtkWidgetClass *) class;

    parent_class = gtk_type_class (gtk_widget_get_type ());

    gtk_mplayer_signals[PLAY_SIGNAL]
	= g_signal_new ("play",
			G_TYPE_FROM_CLASS (object_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GtkMPlayerClass, play),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    gtk_mplayer_signals[STOP_SIGNAL]
	= g_signal_new ("stop",
			G_TYPE_FROM_CLASS (object_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GtkMPlayerClass, stop),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    gtk_mplayer_signals[PAUSE_SIGNAL]
	= g_signal_new ("pause",
			G_TYPE_FROM_CLASS (object_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GtkMPlayerClass, pause),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    gtk_mplayer_signals[POS_CHANGED_SIGNAL]
	= g_signal_new ("position-chaned",
			G_TYPE_FROM_CLASS (object_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GtkMPlayerClass, position_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    /*
     * object class methods
     */
    object_class->destroy = gtk_mplayer_destroy;

    /*
     * widget class methods
     */
    widget_class->realize = gtk_mplayer_realize;
    widget_class->unrealize = gtk_mplayer_unrealize;
    widget_class->size_allocate = gtk_mplayer_size_allocate;

    /*
     * mplayer class methods
     */
    class->play = NULL;
    class->stop = NULL;
    class->pause = NULL;
}

/* object class methods */
static void
gtk_mplayer_media_info_init (GtkMPlayerMediaInfo * info)
{
    g_return_if_fail (info);

    info->flags = 0;
    info->length = -1;
    info->width = -1;
    info->height = -1;
}

static void
gtk_mplayer_init (GtkMPlayer * player)
{
    player->filename = NULL;
    player->pos = 0.0;
    player->speed = 1.0;
    player->status = GtkMPlayerStatusStop;
    player->flags = GtkMPlayerEmbedFlag | GtkMPlayerAllowFrameDroppingFlag;

    player->command = g_strdup ("mplayer");
    player->vo = NULL;
    player->ao = NULL;
    player->args = NULL;

    gtk_mplayer_media_info_init (&player->media_info);
}

static void
gtk_mplayer_destroy (GtkObject * object)
{
    GtkMPlayer *player = GTK_MPLAYER (object);

    g_return_if_fail (GTK_IS_MPLAYER (player));

    g_free (player->filename);
    player->filename = NULL;

    g_free (player->command);
    player->command = NULL;

    g_free (player->vo);
    player->vo = NULL;

    g_free (player->ao);
    player->ao = NULL;

    /*
     * FIXME: free player->args
     */

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

/* widget class methods */

static void gtk_mplayer_send_configure (GtkMPlayer * player);

static void
gtk_mplayer_realize (GtkWidget * widget)
{
    GtkMPlayer *player;
    GdkWindowAttr attributes;
    gint    attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_MPLAYER (widget));

    player = GTK_MPLAYER (widget);
    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask =
	gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				     &attributes, attributes_mask);
    gdk_window_set_user_data (widget->window, player);

    widget->style = gtk_style_attach (widget->style, widget->window);
    gtk_style_set_background (widget->style, widget->window,
			      GTK_STATE_NORMAL);

    gtk_mplayer_send_configure (player);

    gdk_window_set_background (widget->window, &widget->style->black);
}

static void
gtk_mplayer_unrealize (GtkWidget * widget)
{
    GtkMPlayer *player = GTK_MPLAYER (widget);

    if (gtk_mplayer_is_running (player))
	gtk_mplayer_stop (player);

    if (parent_class->unrealize)
	(*parent_class->unrealize) (widget);
}

static void
gtk_mplayer_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_MPLAYER (widget));
    g_return_if_fail (allocation != NULL);

    widget->allocation = *allocation;
    /*
     * FIXME, TODO-1.3: back out the MAX() statements
     */
    widget->allocation.width = MAX (1, widget->allocation.width);
    widget->allocation.height = MAX (1, widget->allocation.height);

    if (GTK_WIDGET_REALIZED (widget))
    {
	gdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);

	gtk_mplayer_send_configure (GTK_MPLAYER (widget));
    }
}

static void
gtk_mplayer_send_configure (GtkMPlayer * player)
{
    GtkWidget *widget;
    GdkEventConfigure event;

    widget = GTK_WIDGET (player);

    event.type = GDK_CONFIGURE;
    event.window = widget->window;
    event.send_event = TRUE;
    event.x = widget->allocation.x;
    event.y = widget->allocation.y;
    event.width = widget->allocation.width;
    event.height = widget->allocation.height;

    gtk_widget_event (widget, (GdkEvent *) & event);
}

/* mplayer class methods */

gfloat
gtk_mplayer_get_position (GtkMPlayer * player)
{
    g_return_val_if_fail (GTK_IS_MPLAYER (player), 0.0);
    return player->pos;
}

GtkMPlayerStatus
gtk_mplayer_get_status (GtkMPlayer * player)
{
    g_return_val_if_fail (GTK_IS_MPLAYER (player), GtkMPlayerStatusStop);
    return player->status;
}

static ChildContext *
gtk_mplayer_get_player_child (GtkMPlayer * player)
{
    if (!player_context_table)
	return NULL;

    return g_hash_table_lookup (player_context_table, player);
}

gboolean
gtk_mplayer_is_running (GtkMPlayer * player)
{
    if (gtk_mplayer_get_player_child (player))
	return TRUE;

    return FALSE;
}

GtkWidget *
gtk_mplayer_new (void)
{
    GtkMPlayer *player = GTK_MPLAYER (gtk_type_new (gtk_mplayer_get_type ()));

    return GTK_WIDGET (player);
}

static void
playback_done (gpointer data)
{
    GtkMPlayer *player = data;

    g_return_if_fail (GTK_IS_MPLAYER (player));

    player->pos = 0.0;
    player->status = GtkMPlayerStatusStop;
    gtk_signal_emit (GTK_OBJECT (player), gtk_mplayer_signals[STOP_SIGNAL]);
}

void
gtk_mplayer_play (GtkMPlayer * player)
{
    gtk_mplayer_start (player, 0.0, 1.0);
}

void
gtk_mplayer_start (GtkMPlayer * player, gfloat pos, gfloat speed)
{
    GList  *arg_list = NULL;
    ChildContext *context;
    gchar   buf[16];
    gint    buf_size = sizeof (buf) / sizeof (gchar);

    g_return_if_fail (GTK_IS_MPLAYER (player));
    g_return_if_fail (player->filename);

    if (player->status == GtkMPlayerStatusPause)
    {
	gtk_mplayer_toggle_pause (player);
	return;
    }

    if (gtk_mplayer_is_running (player))
	return;

    /*
     * create argument list
     */
    arg_list = g_list_append (arg_list, g_strdup (player->command));
    arg_list = g_list_append (arg_list, g_strdup ("-slave"));

    if (GTK_WIDGET_REALIZED (GTK_WIDGET (player)))
    {
      if (player->flags & GtkMPlayerEmbedFlag)
	{
	    g_snprintf (buf, buf_size, "%ld",
			GDK_WINDOW_XWINDOW (GTK_WIDGET (player)->window));
	    arg_list = g_list_append (arg_list, g_strdup ("-wid"));
	    arg_list = g_list_append (arg_list, g_strdup (buf));
	}

	g_snprintf (buf, buf_size, "scale=%d:%d",
		    GTK_WIDGET (player)->allocation.width,
		    GTK_WIDGET (player)->allocation.height);
	arg_list = g_list_append (arg_list, g_strdup ("-vop"));
	arg_list = g_list_append (arg_list, g_strdup (buf));
    }

    if (player->vo)
    {
	arg_list = g_list_append (arg_list, g_strdup ("-vo"));
	arg_list = g_list_append (arg_list, g_strdup (player->vo));
    }

    if (player->ao)
    {
	arg_list = g_list_append (arg_list, g_strdup ("-ao"));
	arg_list = g_list_append (arg_list, g_strdup (player->ao));
    }

    if (player->flags & GtkMPlayerAllowFrameDroppingFlag)
    {
	arg_list = g_list_append (arg_list, g_strdup ("-framedrop"));
    }

    /*
     * pos
     */
    if (pos > 0.01)
    {
	arg_list = g_list_append (arg_list, g_strdup ("-ss"));
	g_snprintf (buf, buf_size, "%f", pos);
	arg_list = g_list_append (arg_list, g_strdup (buf));
    }

    /*
     * speed
     */
    if (speed < 0.01 && speed > 100.01)
    {
	player->speed = 1.0;
    }
    else
    {
	player->speed = speed;
    }
    arg_list = g_list_append (arg_list, g_strdup ("-speed"));
    g_snprintf (buf, buf_size, "%f", player->speed);
    arg_list = g_list_append (arg_list, g_strdup (buf));

    /*
     * FIXME: add player->args
     */

    arg_list = g_list_append (arg_list, g_strdup (player->filename));

    /*
     * real start
     */
    context =
	start_command (player, arg_list, process_line, player, playback_done);

    /*
     * add to hash table
     */
    if (!player_context_table)
	player_context_table =
	    g_hash_table_new (g_direct_hash, g_direct_equal);
    g_hash_table_insert (player_context_table, player, context);
}

void
gtk_mplayer_stop (GtkMPlayer * player)
{
    ChildContext *context;

    g_return_if_fail (GTK_IS_MPLAYER (player));

    context = gtk_mplayer_get_player_child (player);
    if (!context)
	return;

    g_hash_table_remove (player_context_table, context->player);
    child_context_destroy (context);

    gtk_widget_queue_draw (GTK_WIDGET (player));
}

void
gtk_mplayer_toggle_pause (GtkMPlayer * player)
{
    ChildContext *context;
    const gchar *command = "pause\n";

    g_return_if_fail (GTK_IS_MPLAYER (player));

    if (!gtk_mplayer_is_running (player))
	return;

    if (!player_context_table)
	return;

    context = g_hash_table_lookup (player_context_table, player);
    if (!context)
	return;

    g_return_if_fail (context->stdin_fd > 0);

    write (context->stdin_fd, command, strlen (command));
}

void
gtk_mplayer_set_speed (GtkMPlayer * player, gfloat speed)
{
    g_return_if_fail (GTK_IS_MPLAYER (player));
    g_return_if_fail (speed > 0.00999 && speed < 100.00001);

    if (player->status == GtkMPlayerStatusPlay
	|| player->status == GtkMPlayerStatusPause)
    {
	gfloat  pos = player->pos;
	gtk_mplayer_stop (player);
	gtk_mplayer_start (player, pos, speed);
    }
    else
    {
	player->speed = speed;
    }
}

gfloat
gtk_mplayer_get_speed (GtkMPlayer * player)
{
    g_return_val_if_fail (GTK_IS_MPLAYER (player), 1.0);

    return player->speed;
}

void
gtk_mplayer_seek (GtkMPlayer * player, gfloat percentage)
{
    ChildContext *context;
    gchar   buf[64];

    g_return_if_fail (GTK_IS_MPLAYER (player));

    if (!gtk_mplayer_is_running (player))
	return;

    g_return_if_fail (percentage > -0.000001 && percentage < 100.000001);

    if (!player_context_table)
	return;

    context = g_hash_table_lookup (player_context_table, player);
    if (!context)
	return;

    g_return_if_fail (context->stdin_fd > 0);

    g_snprintf (buf, 64, "seek %f 1\n", percentage);

    write (context->stdin_fd, buf, strlen (buf));
}

void
gtk_mplayer_seek_by_time (GtkMPlayer * player, gfloat second)
{
    ChildContext *context;
    gchar   buf[64];

    g_return_if_fail (GTK_IS_MPLAYER (player));

    if (!gtk_mplayer_is_running (player))
	return;

    g_return_if_fail (second >= 0);

    if (!player_context_table)
	return;

    context = g_hash_table_lookup (player_context_table, player);
    if (!context)
	return;

    g_return_if_fail (context->stdin_fd > 0);

    g_snprintf (buf, 64, "seek %f 2\n", second);

    write (context->stdin_fd, buf, strlen (buf));
}

void
gtk_mplayer_seek_relative (GtkMPlayer * player, gfloat second)
{
    ChildContext *context;
    gchar   buf[64];

    g_return_if_fail (GTK_IS_MPLAYER (player));

    if (!gtk_mplayer_is_running (player))
	return;

    g_return_if_fail (second >= 0);

    if (!player_context_table)
	return;

    context = g_hash_table_lookup (player_context_table, player);
    if (!context)
	return;

    g_return_if_fail (context->stdin_fd > 0);

    g_snprintf (buf, 64, "seek %f 0\n", second);

    write (context->stdin_fd, buf, strlen (buf));
}

void
gtk_mplayer_set_audio_delay (GtkMPlayer * player, gfloat second)
{
    ChildContext *context;
    gchar   buf[64];

    g_return_if_fail (GTK_IS_MPLAYER (player));

    if (!gtk_mplayer_is_running (player))
	return;

    g_return_if_fail (second >= 0);

    if (!player_context_table)
	return;

    context = g_hash_table_lookup (player_context_table, player);
    if (!context)
	return;

    g_return_if_fail (context->stdin_fd > 0);

    g_snprintf (buf, 64, "audio_delay %f\n", second);

    write (context->stdin_fd, buf, strlen (buf));
}

void
gtk_mplayer_set_file (GtkMPlayer * player, const gchar * file)
{
    g_return_if_fail (GTK_IS_MPLAYER (player));
    g_return_if_fail (!gtk_mplayer_is_running (player));

    g_free (player->filename);
    gtk_mplayer_media_info_init (&player->media_info);

    if (file && *file)
	player->filename = g_strdup (file);
    else
	player->filename = NULL;
}

void
gtk_mplayer_set_flags (GtkMPlayer * player, GtkMPlayerFlags flags)
{
    g_return_if_fail (GTK_IS_MPLAYER (player));

    player->flags |= flags;
}

void
gtk_mplayer_unset_flags (GtkMPlayer * player, GtkMPlayerFlags flags)
{
    g_return_if_fail (GTK_IS_MPLAYER (player));

    player->flags &= ~flags;
}

GtkMPlayerFlags
gtk_mplayer_get_flags (GtkMPlayer * player, GtkMPlayerFlags flags)
{
    g_return_val_if_fail (GTK_IS_MPLAYER (player), 0);

    return player->flags;
}

static void
get_drivers_done (gpointer data)
{
    gtk_main_quit ();
}

static GList *
get_drivers (GtkMPlayer * player, gboolean refresh,
	     GHashTable * drivers_table,
	     const gchar * separator, const gchar * option)
{
    GList  *drivers_list = NULL;
    const gchar *command = player ? player->command : "mplayer";
    gchar  *key;
    gboolean found;

    g_return_val_if_fail (drivers_table, NULL);
    g_return_val_if_fail (separator && option, NULL);

    found = g_hash_table_lookup_extended (drivers_table, command,
					  (gpointer) & key,
					  (gpointer) & drivers_list);
    if (!found)
	drivers_list = NULL;

    if (drivers_list && refresh)
    {
	g_hash_table_remove (drivers_table, command);
	g_free (key);
	key = NULL;
	g_list_foreach (drivers_list, (GFunc) g_free, NULL);
	g_list_free (drivers_list);
	drivers_list = NULL;
    }

    if (!drivers_list)
    {
	GetDriversContext dcontext;
	GList  *args = NULL;

	args = g_list_append (args, g_strdup (command));
	args = g_list_append (args, g_strdup (option));
	args = g_list_append (args, g_strdup ("help"));

	dcontext.separator = separator;
	dcontext.sep_len = strlen (separator);
	dcontext.passing_header = TRUE;
	dcontext.drivers_list = g_list_append (NULL, g_strdup ("default"));

	start_command (player, args, process_line_get_drivers,
		       &dcontext, get_drivers_done);
	gtk_main ();

	if (dcontext.drivers_list)
	    g_hash_table_insert (drivers_table,
				 g_strdup (command), dcontext.drivers_list);
	drivers_list = dcontext.drivers_list;
    }

    return drivers_list;
}

const GList *
gtk_mplayer_get_video_out_drivers (GtkMPlayer * player, gboolean refresh)
{
    if (player)
	g_return_val_if_fail (GTK_IS_MPLAYER (player), NULL);

    if (!vo_drivers_table)
	vo_drivers_table = g_hash_table_new (g_str_hash, g_str_equal);

    return get_drivers (player, refresh, vo_drivers_table,
			"Available video output drivers:", "-vo");
}

const GList *
gtk_mplayer_get_audio_out_drivers (GtkMPlayer * player, gboolean refresh)
{
    if (player)
	g_return_val_if_fail (GTK_IS_MPLAYER (player), NULL);

    if (!ao_drivers_table)
	ao_drivers_table = g_hash_table_new (g_str_hash, g_str_equal);

    return get_drivers (player, refresh, ao_drivers_table,
			"Available audio output drivers:", "-ao");
}

void
gtk_mplayer_set_video_out_driver (GtkMPlayer * player,
				  const gchar * vo_driver)
{
    const GList *list, *node;

    g_return_if_fail (GTK_IS_MPLAYER (player));

    g_free (player->vo);

    if (!vo_driver || !*vo_driver || !strcasecmp (vo_driver, "default"))
    {
	player->vo = NULL;
	return;
    }

    list = gtk_mplayer_get_video_out_drivers (player, FALSE);

    /*
     * validate
     */
    for (node = list; node; node = g_list_next (node))
    {
	gchar  *str = node->data;
	if (*str && !strcmp (str, vo_driver))
	{
	    player->vo = g_strdup (vo_driver);
	    return;
	}
    }

    player->vo = NULL;
}

void
gtk_mplayer_set_audio_out_driver (GtkMPlayer * player,
				  const gchar * ao_driver)
{
    const GList *list, *node;

    g_return_if_fail (GTK_IS_MPLAYER (player));

    g_free (player->ao);

    list = gtk_mplayer_get_audio_out_drivers (player, FALSE);

    if (!ao_driver | !*ao_driver || !strcasecmp (ao_driver, "default"))
    {
	player->ao = NULL;
	return;
    }

    /*
     * validate
     */
    for (node = list; node; node = g_list_next (node))
    {
	gchar  *str = node->data;

	if (*str && !strcmp (str, ao_driver))
	{
	    player->ao = g_strdup (ao_driver);
	    return;
	}
    }

    player->ao = NULL;
}

/******************************************************************************
 *
 *   MPlayer handling.
 *
 ******************************************************************************/
static ChildContext *
start_command (GtkMPlayer * player, GList * arg_list,	/* include command */
	       ProcessLineFunc func, gpointer data,
	       GtkDestroyNotify destroy_fn)
{
    ChildContext *context;
    pid_t   pid;
    int     in_fd[2], out_fd[2], err_fd[2];

    if (pipe (out_fd) < 0)
	goto ERROR0;
    if (pipe (err_fd) < 0)
	goto ERROR1;
    if (pipe (in_fd) < 0)
	goto ERROR2;

    pid = fork ();
    if (pid < 0)
	goto ERROR3;

    /*
     * child process
     */
    if (pid == 0)
    {
	GList  *list;
	gchar **argv;
	gint    i = 0, n_args;

	/*
	 * set pipe
	 */
	close (out_fd[0]);
	close (err_fd[0]);
	close (in_fd[1]);
	dup2 (out_fd[1], STDOUT_FILENO);
	dup2 (err_fd[1], STDERR_FILENO);
	dup2 (in_fd[0], STDIN_FILENO);

	/*
	 * set args
	 */
	n_args = g_list_length (arg_list);
	argv = g_new0 (gchar *, n_args + 1);
	for (list = arg_list; list; list = g_list_next (list))
	    argv[i++] = list->data;
	argv[i] = NULL;

	/*
	 * exec
	 */
	putenv ("LC_ALL=C");
	execvp (argv[0], argv);
	_exit (255);
    }


    /*
     * parent process
     */

    context = g_new0 (ChildContext, 1);
    context->pid = pid;
    context->args = arg_list;
    context->player = player;
    context->process_line_fn = func;
    context->data = data;
    context->destroy_fn = destroy_fn;

    context->stdout_fd = out_fd[0];
    fcntl (context->stdout_fd, F_SETFL, O_NONBLOCK);
    close (out_fd[1]);
    context->stdout[0] = '\0';
    context->stdout_size = 0;

    context->stderr_fd = err_fd[0];
    fcntl (context->stderr_fd, F_SETFL, O_NONBLOCK);
    close (err_fd[1]);
    context->stderr[0] = '\0';
    context->stderr_size = 0;

    context->stdin_fd = in_fd[1];
    close (in_fd[0]);

    /*
     * observe output
     */
    context->checker_id = gtk_timeout_add (GTK_MPLAYER_REFRESH_RATE,
					   timeout_check_child, context);
    return context;

  ERROR3:
    close (in_fd[0]);
    close (in_fd[1]);
  ERROR2:
    close (err_fd[0]);
    close (err_fd[1]);
  ERROR1:
    close (out_fd[0]);
    close (out_fd[1]);
  ERROR0:
    g_list_foreach (arg_list, (GFunc) g_free, NULL);
    g_list_free (arg_list);
    return NULL;
}

static  gint
timeout_check_child (gpointer data)
{
    ChildContext *context = data;
    GtkMPlayer *player = context->player;

    if (context->pid <= 0)
	return FALSE;

    if (!process_output (context))
    {
	pid_t   pid;
	int     status;

	pid = waitpid (context->pid, &status, WNOHANG);
	context->pid = 0;
	context->checker_id = 0;

	if (context == gtk_mplayer_get_player_child (player))
	    g_hash_table_remove (player_context_table, context->player);
	child_context_destroy (context);

	gtk_widget_queue_draw (GTK_WIDGET (player));

	return FALSE;
    }

    return TRUE;
}

#define LINE_END(end, str) \
{ \
   gchar *end1, *end2; \
   if (!str || !*str) { \
      end = NULL; \
   } else { \
      end1 = strchr (str, '\n'); \
      end2 = strchr (str, '\r'); \
      if (end1 && end2) \
         end = (end2 < end1) ? end2 : end1; \
      else \
         end = end1 ? end1 : end2; \
   } \
}

static void
process_lines (ChildContext * context,
	       gchar buf[GTK_MPLAYER_BUF_SIZE * 2], gint size,
	       gchar stock_buf[GTK_MPLAYER_BUF_SIZE], gint * remain_size,
	       gboolean is_stderr)
{
    gint    len;
    gchar  *src, *end;

    g_return_if_fail (buf && stock_buf);
    g_return_if_fail (size > 0 || size < GTK_MPLAYER_BUF_SIZE);
    g_return_if_fail (remain_size);

    src = buf;

    LINE_END (end, src);

    while (end && src)
    {
	*end = '\0';

	len = end - src;

	if (context->process_line_fn)
	    context->process_line_fn (context, src, len, is_stderr);

	src = end + 1;
	size -= (end - src);
	if (src)
	{
	    LINE_END (end, src);
	}
	else
	{
	    end = NULL;
	}
    }

    if (size <= 0 || size > GTK_MPLAYER_BUF_SIZE - 1 || !src || !*src)
    {
	stock_buf[0] = '\0';
	*remain_size = 0;
    }
    else
    {
	memcpy (stock_buf, src, size);
	stock_buf[size] = '\0';
	*remain_size = size;
    }
}

static  gboolean
process_output (ChildContext * context)
{
    gint    n, size;
    gchar   buf[GTK_MPLAYER_BUF_SIZE * 2], *next;
    gfloat  pos;

    /*
     * stderr
     */
    size = context->stderr_size;

    if (size > 0 && size < GTK_MPLAYER_BUF_SIZE)
    {
	memcpy (buf, context->stderr, size);
	next = buf + size;
    }
    else
    {
	next = buf;
    }

    n = read (context->stderr_fd, buf, GTK_MPLAYER_BUF_SIZE - 1);
    if (n == 0)
    {
    }
    else if (n > 0)
    {
	next[MIN (GTK_MPLAYER_BUF_SIZE - 1, n)] = '\0';
	size += MIN (GTK_MPLAYER_BUF_SIZE - 1, n);

	process_lines (context, buf, size,
		       context->stderr, &context->stderr_size, TRUE);
    }

    /*
     * stdout
     */
    size = context->stdout_size;

    if (size > 0 && size < GTK_MPLAYER_BUF_SIZE)
    {
	memcpy (buf, context->stdout, size);
	next = buf + size;
    }
    else
    {
	next = buf;
    }

    n = read (context->stdout_fd, next, GTK_MPLAYER_BUF_SIZE - 1);
    if (n == 0)
	return FALSE;
    else if (n < 0)
	return TRUE;

    next[MIN (GTK_MPLAYER_BUF_SIZE - 1, n)] = '\0';
    size += MIN (GTK_MPLAYER_BUF_SIZE - 1, n);
    pos = context->player->pos;

    process_lines (context, buf, size,
		   context->stdout, &context->stdout_size, FALSE);

    if (fabs (context->player->pos - pos) > 0.1)
	gtk_signal_emit (GTK_OBJECT (context->player),
			 gtk_mplayer_signals[POS_CHANGED_SIGNAL]);

    return TRUE;
}

static void
child_context_destroy (ChildContext * context)
{
    if (context->checker_id > 0)
    {
	gtk_timeout_remove (context->checker_id);
	context->checker_id = 0;
    }

    if (context->pid > 0)
    {
	kill (context->pid, SIGKILL);
	waitpid (context->pid, NULL, WUNTRACED);
	context->pid = 0;
    }

    g_list_foreach (context->args, (GFunc) g_free, NULL);
    g_list_free (context->args);
    context->args = NULL;

    if (context->stdout_fd)
	close (context->stdout_fd);
    context->stdout_fd = 0;

    if (context->stderr_fd)
	close (context->stderr_fd);
    context->stderr_fd = 0;

    if (context->stdin_fd)
	close (context->stdin_fd);
    context->stdin_fd = 0;

    if (context->data && context->destroy_fn)
	context->destroy_fn (context->data);
    context->data = NULL;

    g_free (context);
}

/******************************************************************************
 *
 *   Line process funcs.
 *
 ******************************************************************************/
static void
process_line (ChildContext * context,
	      const gchar * line, gint len, gboolean is_stderr)
{
    GtkMPlayer *player = context->player;

    if (is_stderr)
    {
    }
    else
    {
	if (!strcmp (line, "------ PAUSED -------"))
	{
	    player->status = GtkMPlayerStatusPause;
	    gtk_signal_emit (GTK_OBJECT (player),
			     gtk_mplayer_signals[PAUSE_SIGNAL]);

	}
	else if (len > 2 && (!strncmp (line, "A:", 2)
			     || !strncmp (line, "V:", 2)))
	{
	    gchar   timestr[16];
	    const gchar *begin = line + 2, *end;

	    /*
	     * set status
	     */
	    if (player->status != GtkMPlayerStatusPlay)
	    {
		player->status = GtkMPlayerStatusPlay;
		gtk_signal_emit (GTK_OBJECT (player),
				 gtk_mplayer_signals[PLAY_SIGNAL]);
	    }

	    /*
	     * get movie position
	     */
	    while (isspace (*begin) && *begin)
		begin++;
	    if (*begin)
	    {
		end = begin + 1;
		while (!isspace (*end) && *end)
		    end++;
		len = end - begin;
		if (len > 0 && len < 15)
		{
		    memcpy (timestr, begin, len);
		    timestr[len] = '\0';
		    player->pos = atof (timestr);
		}
	    }

	}
	else if (!strncasecmp (line, "VIDEO: ", 7))
	{
	    if (!strncmp (line + 7, "no video!!!", 11))
	    {
		context->player->media_info.flags |=
		    GtkMPlayerMediaInfoNoVideoFlag;
	    }
	    else
	    {
	    }

	}
	else if (!strncasecmp (line, "Audio: ", 7))
	{
	    if (!strncmp (line + 7, "no sound!!!", 11))
	    {
		context->player->media_info.flags |=
		    GtkMPlayerMediaInfoNoAudioFlag;
	    }
	    else
	    {
	    }

	}
	else
	{
	}
    }
}

static void
process_line_get_drivers (ChildContext * context,
			  const gchar * line, gint len, gboolean is_stderr)
{
    GetDriversContext *dcontext;
    gchar  *str, *id_str, *exp_str = NULL, *end;

    g_return_if_fail (context);
    g_return_if_fail (context->data);
    if (!line || !*line || len <= 0)
	return;

    dcontext = context->data;

    if (dcontext->passing_header)
    {
	if (len >= dcontext->sep_len
	    && !strncmp (line, dcontext->separator, dcontext->sep_len))
	{
	    dcontext->passing_header = FALSE;
	}
	return;
    }

    str = id_str = g_strdup (line);

    /*
     * get id
     */
    while (isspace (*id_str) && *id_str)
	id_str++;
    if (!*id_str)
    {
	g_free (str);
	return;
    }
    end = id_str + 1;
    while (!isspace (*end) && *end)
	end++;
    if (*end)
	exp_str = end + 1;
    *end = '\0';

    /*
     * get name
     */
    if (exp_str)
    {
	/*
	 * exp_str = g_strstrip (exp_str);
	 */
    }

    /*
     * append to list
     */
    dcontext->drivers_list = g_list_append (dcontext->drivers_list,
					    g_strdup (id_str));

    g_free (str);
}

#endif /* ENABLE_MPLAYER */
