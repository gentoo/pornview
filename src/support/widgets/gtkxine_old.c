/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                        (c) 2002,2003                         (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 *  These codes are mostly taken from xine project.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gtkxine_old.h"

#ifdef ENABLE_XINE_OLD

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pwd.h>
#include <sys/types.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <xine.h>

#include "gtk2-compat.h"

enum
{
    PLAY_SIGNAL,
    STOP_SIGNAL,
    PLAYBACK_FINISHED_SIGNAL,
    NEED_NEXT_MRL_SIGNAL,
    BRANCHED_SIGNAL,
    LAST_SIGNAL
};

/* missing stuff from X includes */
#ifndef XShmGetEventBase
extern int XShmGetEventBase (Display *);
#endif

static void gtk_xine_class_init (GtkXineClass * klass);
static void gtk_xine_init (GtkXine * gxine);

static void gtk_xine_realize (GtkWidget * widget);
static void gtk_xine_unrealize (GtkWidget * widget);

static gint gtk_xine_expose (GtkWidget * widget, GdkEventExpose * event);

static void gtk_xine_size_allocate (GtkWidget * widget,
				    GtkAllocation * allocation);

static GtkWidgetClass *parent_class = NULL;

static gint gtk_xine_signals[LAST_SIGNAL] = { 0 };

GtkType
gtk_xine_get_type (void)
{
    static GtkType gtk_xine_type = 0;

#if (GTK_MAJOR_VERSION >= 2)
    if (!gtk_xine_type)
    {
	static const GTypeInfo gtk_xine_info = {
	    sizeof (GtkXineClass),
	    NULL,		/* base_init */
	    NULL,		/* base_finalize */
	    (GClassInitFunc) gtk_xine_class_init,
	    NULL,		/* class_finalize */
	    NULL,		/* class_data */
	    sizeof (GtkXine),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) gtk_xine_init,
	};

	gtk_xine_type = g_type_register_static (GTK_TYPE_WIDGET,
						"GtkXine", &gtk_xine_info, 0);
    }
#else /* (GTK_MAJOR_VERSION >= 2) */
    if (!gtk_xine_type)
    {
	static const GtkTypeInfo gtk_xine_info = {
	    "GtkXine",
	    sizeof (GtkXine),
	    sizeof (GtkXineClass),
	    (GtkClassInitFunc) gtk_xine_class_init,
	    (GtkObjectInitFunc) gtk_xine_init,
	    /*
	     * reserved_1 
	     */ NULL,
	    /*
	     * reserved_2 
	     */ NULL,
	    (GtkClassInitFunc) NULL,
	};

	gtk_xine_type =
	    gtk_type_unique (gtk_widget_get_type (), &gtk_xine_info);
    }
#endif /* (GTK_MAJOR_VERSION >= 2) */

    return gtk_xine_type;
}

static void
gtk_xine_class_init (GtkXineClass * class)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass *) class;
    widget_class = (GtkWidgetClass *) class;

    parent_class = gtk_type_class (gtk_widget_get_type ());

#if (defined USE_GTK2) && (defined GTK_DISABLE_DEPRECATED)
    gtk_xine_signals[PLAY_SIGNAL]
	= gtk_signal_new ("play",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, play),
			  g_cclosure_marshal_VOID__VOID, GTK_TYPE_NONE, 0);

    gtk_xine_signals[STOP_SIGNAL]
	= gtk_signal_new ("stop",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, stop),
			  g_cclosure_marshal_VOID__VOID, GTK_TYPE_NONE, 0);

    gtk_xine_signals[PLAYBACK_FINISHED_SIGNAL]
	= gtk_signal_new ("playback_finished",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, playback_finished),
			  g_cclosure_marshal_VOID__VOID, GTK_TYPE_NONE, 0);

    gtk_xine_signals[NEED_NEXT_MRL_SIGNAL]
	= gtk_signal_new ("need_next_mrl",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, need_next_mrl),
			  g_cclosure_marshal_VOID__POINTER,
			  GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

    gtk_xine_signals[BRANCHED_SIGNAL]
	= gtk_signal_new ("branched",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, branched),
			  g_cclosure_marshal_VOID__VOID, GTK_TYPE_NONE, 0);
#else /* (defined USE_GTK2) && (defined GTK_DISABLE_DEPRECATED) */
    gtk_xine_signals[PLAY_SIGNAL]
	= gtk_signal_new ("play",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, play),
			  gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    gtk_xine_signals[STOP_SIGNAL]
	= gtk_signal_new ("stop",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, stop),
			  gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    gtk_xine_signals[PLAYBACK_FINISHED_SIGNAL]
	= gtk_signal_new ("playback_finished",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, playback_finished),
			  gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    gtk_xine_signals[NEED_NEXT_MRL_SIGNAL]
	= gtk_signal_new ("need_next_mrl",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, need_next_mrl),
			  gtk_marshal_NONE__POINTER,
			  GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

    gtk_xine_signals[BRANCHED_SIGNAL]
	= gtk_signal_new ("branched",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (GtkXineClass, branched),
			  gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    gtk_object_class_add_signals (object_class, gtk_xine_signals,
				  LAST_SIGNAL);
#endif /* (defined USE_GTK2) && (defined GTK_DISABLE_DEPRECATED) */

    widget_class->realize = gtk_xine_realize;
    widget_class->unrealize = gtk_xine_unrealize;

    widget_class->size_allocate = gtk_xine_size_allocate;

    widget_class->expose_event = gtk_xine_expose;

    class->play = NULL;
    class->stop = NULL;
    class->playback_finished = NULL;
    class->need_next_mrl = NULL;
    class->branched = NULL;
}

static void
gtk_xine_init (GtkXine * gxine)
{
    gxine->widget.requisition.width = 8;
    gxine->widget.requisition.height = 8;

    gxine->config = NULL;
    gxine->xine = NULL;
    gxine->vo_driver = NULL;
    gxine->ao_driver = NULL;
    gxine->display = NULL;
    gxine->fullscreen_mode = FALSE;
    gxine->status = GTK_XINE_STATUS_IDLE;
}

static void
dest_size_cb (void *gxine_gen,
	      int video_width, int video_height,
	      int *dest_width, int *dest_height)
{
    GtkXine *gxine = (GtkXine *) gxine_gen;

    if (gxine->fullscreen_mode)
    {
	*dest_width = gxine->fullscreen_width;
	*dest_height = gxine->fullscreen_height;
    }
    else
    {
	*dest_width = gxine->widget.allocation.width;
	*dest_height = gxine->widget.allocation.height;
    }
}

static void
frame_output_cb (void *gxine_gen,
		 int video_width, int video_height,
		 int *dest_x, int *dest_y, int *dest_width, int *dest_height
#if (XINE_MAJOR_VERSION == 0) && (XINE_MINOR_VERSION == 9) && (XINE_SUB_VERSION < 9)
    )
#else
		 , int *win_x, int *win_y)
#endif
{
    GtkXine *gxine = (GtkXine *) gxine_gen;

    *dest_x = 0;
    *dest_y = 0;
#if (XINE_MAJOR_VERSION == 0) && (XINE_MINOR_VERSION == 9) && (XINE_SUB_VERSION < 9)
#else
    *win_x = gxine->xpos;
    *win_y = gxine->ypos;
#endif
    if (gxine->fullscreen_mode)
    {
	*dest_width = gxine->fullscreen_width;
	*dest_height = gxine->fullscreen_height;
    }
    else
    {
	*dest_width = gxine->widget.allocation.width;
	*dest_height = gxine->widget.allocation.height;
    }
}

static vo_driver_t *
load_video_out_driver (GtkXine * this)
{
    double  res_h, res_v;
    x11_visual_t vis;
    char  **driver_ids;
    int     i;
    char   *video_driver_id;
    vo_driver_t *vo_driver;

    vis.display = this->display;
    vis.screen = this->screen;
    vis.d = this->video_window;
    res_h = (DisplayWidth (this->display, this->screen) * 1000
	     / DisplayWidthMM (this->display, this->screen));
    res_v = (DisplayHeight (this->display, this->screen) * 1000
	     / DisplayHeightMM (this->display, this->screen));
    vis.display_ratio = res_h / res_v;

    if (fabs (vis.display_ratio - 1.0) < 0.01)
    {
	vis.display_ratio = 1.0;
    }

#if (XINE_MAJOR_VERSION == 0) && (XINE_MINOR_VERSION == 9) && (XINE_SUB_VERSION < 9)
    vis.calc_dest_size = dest_size_cb;
    vis.request_dest_size = frame_output_cb;
#else
    vis.dest_size_cb = dest_size_cb;
    vis.frame_output_cb = frame_output_cb;
#endif
    vis.user_data = this;

    /*
     * video output driver auto-probing 
     */
    driver_ids = xine_list_video_output_plugins (VISUAL_TYPE_X11);

    /*
     * try to init video with stored information 
     */
    video_driver_id = this->config->register_string (this->config,
						     "video.driver", "auto",
						     "video driver to use",
						     NULL, NULL, NULL);
    if (strcmp (video_driver_id, "auto"))
    {
	vo_driver = xine_load_video_output_plugin (this->config,
						   video_driver_id,
						   VISUAL_TYPE_X11,
						   (void *) &vis);
	if (vo_driver)
	{
	    free (driver_ids);
	    return vo_driver;
	}
    }

    i = 0;
    while (driver_ids[i])
    {
	video_driver_id = driver_ids[i];

	vo_driver = xine_load_video_output_plugin (this->config,
						   video_driver_id,
						   VISUAL_TYPE_X11,
						   (void *) &vis);
	if (vo_driver)
	{
	    this->config->update_string (this->config,
					 "video.driver", video_driver_id);
	    return vo_driver;
	}

	i++;
    }

    return NULL;
}

static ao_driver_t *
load_audio_out_driver (GtkXine * this)
{
    ao_driver_t *ao_driver = NULL;
    char   *audio_driver_id;
    char  **driver_ids = xine_list_audio_output_plugins ();
    int     i = 0;

    /*
     * try to init audio with stored information 
     */
    audio_driver_id = this->config->register_string (this->config,
						     "audio.driver", "auto",
						     "audio driver to use",
						     NULL, NULL, NULL);

    if (strcmp (audio_driver_id, "auto"))
	return xine_load_audio_output_plugin (this->config, audio_driver_id);


    while (driver_ids[i])
    {
	audio_driver_id = driver_ids[i];

	ao_driver = xine_load_audio_output_plugin (this->config,
						   driver_ids[i]);

	if (ao_driver)
	{
	    printf ("main: ...worked, using '%s' audio driver.\n",
		    driver_ids[i]);

	    this->config->update_string (this->config, "audio.driver",
					 audio_driver_id);

	    return ao_driver;
	}
	i++;
    }

    return ao_driver;
}

static void *
xine_thread (void *this_gen)
{
    GtkXine *this = (GtkXine *) this_gen;
    /*
     * GtkWidget  *widget = &this->widget; 
     */

    for (;;)
    {
	XEvent  xevent;
	gint    status;

	if (this->status == GTK_XINE_STATUS_EXIT)
	    break;

	status = xine_get_status (this->xine);
	if (status != XINE_PLAY)
	{
	    usleep (1000);
	    continue;
	}

	XNextEvent (this->display, &xevent);

	/*
	 * printf ("gtkxine: got an event (%d)\n", event.type);  
	 */

	switch (xevent.type)
	{
	  case Expose:
	      if (xevent.xexpose.count != 0)
		  break;

	      this->vo_driver->gui_data_exchange (this->vo_driver,
						  GUI_DATA_EX_EXPOSE_EVENT,
						  &xevent);
	      break;

#if (XINE_MAJOR_VERSION == 0) && (XINE_MINOR_VERSION == 9) && (XINE_SUB_VERSION < 9)
	  case ConfigureNotify:
	  {
	      XConfigureEvent *ev = (XConfigureEvent *) & xevent;
	      x11_rectangle_t area;
	      area.x = 0;
	      area.y = 0;
	      area.w = ev->width;
	      area.h = ev->height;
	      this->vo_driver->gui_data_exchange (this->vo_driver,
						  GUI_DATA_EX_DEST_POS_SIZE_CHANGED,
						  &area);
	      break;
	  }
#endif

	  case FocusIn:	/* happens only in fullscreen mode */
	      XLockDisplay (this->display);
	      XSetInputFocus (this->display,
			      this->toplevel, RevertToNone, CurrentTime);
	      XUnlockDisplay (this->display);
	      break;
	}

	if (xevent.type == this->completion_event)
	{
	    this->vo_driver->gui_data_exchange (this->vo_driver,
						GUI_DATA_EX_COMPLETION_EVENT,
						&xevent);
	    /*
	     * printf ("gtkxine: completion event\n"); 
	     */
	}
    }

    pthread_exit (NULL);
    return NULL;
}

static  gint
configure_cb (GtkWidget * widget,
	      GdkEventConfigure * event, gpointer user_data)
{
    GtkXine *this;

    this = GTK_XINE (user_data);

    this->xpos = event->x;
    this->ypos = event->y;

    return TRUE;
}

void
event_listener (void *data, xine_event_t * event)
{
    GtkXine *gtx = GTK_XINE (data);

    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (event);

    switch (event->type)
    {
      case XINE_EVENT_MOUSE_BUTTON:
	  break;
      case XINE_EVENT_MOUSE_MOVE:
	  break;
      case XINE_EVENT_SPU_BUTTON:
	  break;
      case XINE_EVENT_SPU_CLUT:
	  break;
      case XINE_EVENT_UI_CHANNELS_CHANGED:
	  break;
      case XINE_EVENT_UI_SET_TITLE:
	  g_print ("\"XINE_EVENT_UI_SET_TITLE\" signal\n");
	  break;
      case XINE_EVENT_INPUT_MENU1:
	  break;
      case XINE_EVENT_INPUT_MENU2:
	  break;
      case XINE_EVENT_INPUT_MENU3:
	  break;
      case XINE_EVENT_INPUT_UP:
	  break;
      case XINE_EVENT_INPUT_DOWN:
	  break;
      case XINE_EVENT_INPUT_LEFT:
	  break;
      case XINE_EVENT_INPUT_RIGHT:
	  break;
      case XINE_EVENT_INPUT_SELECT:
	  break;
      case XINE_EVENT_PLAYBACK_FINISHED:
#ifdef USE_GTK2
	  g_signal_emit (G_OBJECT (gtx),
			 gtk_xine_signals[PLAYBACK_FINISHED_SIGNAL], 0);
#else /* USE_GTK2 */
	  gtk_signal_emit (GTK_OBJECT (gtx),
			   gtk_xine_signals[PLAYBACK_FINISHED_SIGNAL]);
#endif /* USE_GTK2 */
	  break;
      case XINE_EVENT_BRANCHED:
	  /*
	   * g_print ("\"XINE_EVENT_BRANCHED\" signal\n"); 
	   */
#ifdef USE_GTK2
	  g_signal_emit (G_OBJECT (gtx),
			 gtk_xine_signals[BRANCHED_SIGNAL], 0);
#else /* USE_GTK2 */
	  gtk_signal_emit (GTK_OBJECT (gtx),
			   gtk_xine_signals[BRANCHED_SIGNAL]);
#endif /* USE_GTK2 */
	  break;
      case XINE_EVENT_NEED_NEXT_MRL:
      {
	  xine_next_mrl_event_t *uevent = (xine_next_mrl_event_t *) event;
	  gchar  *next_mrl;

#ifdef USE_GTK2
	  g_signal_emit (G_OBJECT (gtx),
			 gtk_xine_signals[NEED_NEXT_MRL_SIGNAL],
			 0, &next_mrl);
#else /* USE_GTK2 */
	  gtk_signal_emit (GTK_OBJECT (gtx),
			   gtk_xine_signals[NEED_NEXT_MRL_SIGNAL], &next_mrl);
#endif /* USE_GTK2 */

	  if (next_mrl && *next_mrl)
	  {
	      uevent->handled = 1;
	      uevent->mrl = next_mrl;
	  }
	  break;
      }
      case XINE_EVENT_INPUT_NEXT:
	  break;
      case XINE_EVENT_INPUT_PREVIOUS:
	  break;
      case XINE_EVENT_INPUT_ANGLE_NEXT:
	  break;
      case XINE_EVENT_INPUT_ANGLE_PREVIOUS:
	  break;
      case XINE_EVENT_SPU_FORCEDISPLAY:
	  break;
      case XINE_EVENT_FRAME_CHANGE:
	  break;
      case XINE_EVENT_CLOSED_CAPTION:
	  break;
#if (XINE_MAJOR_VERSION == 0) && (XINE_MINOR_VERSION == 9) && (XINE_SUB_VERSION < 9)
      case XINE_EVENT_INPUT_BUTTON_FORCE:
	  break;
#endif

      default:
	  break;
    }
}

static unsigned char bm_no_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static void
gtk_xine_realize (GtkWidget * widget)
{
    /*
     * GdkWindowAttr attributes; 
     */
    /*
     * gint          attributes_mask; 
     */
    GtkXine *this;
    Window  parent;
    XGCValues values;
    Pixmap  bm_no;
    GdkWindow *toplevel;

    g_return_if_fail (widget);
    g_return_if_fail (GTK_IS_XINE (widget));

    this = GTK_XINE (widget);

    /*
     * set realized flag 
     */

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    /*
     * create our own video window
     */
    parent = GDK_WINDOW_XWINDOW (gtk_widget_get_parent_window (widget));
    this->video_window
	= XCreateSimpleWindow (gdk_display,
			       parent,
			       0, 0,
			       widget->allocation.width,
			       widget->allocation.height, 1,
			       BlackPixel (gdk_display,
					   DefaultScreen (gdk_display)),
			       BlackPixel (gdk_display,
					   DefaultScreen (gdk_display)));

    widget->window = gdk_window_foreign_new (this->video_window);
    /*
     * gdk_window_add_filter (widget->window, gtk_xine_event_filter, this);
     */

    /*
     * prepare for fullscreen playback
     */
    this->fullscreen_width = DisplayWidth (gdk_display,
					   DefaultScreen (gdk_display));
    this->fullscreen_height = DisplayHeight (gdk_display,
					     DefaultScreen (gdk_display));
    this->fullscreen_mode = 0;

    toplevel =
	gdk_window_get_toplevel (gtk_widget_get_parent_window (widget));
    this->toplevel = GDK_WINDOW_XWINDOW (toplevel);

    /*
     * track configure events of toplevel window
     */
#ifdef USE_GTK2
    g_signal_connect_after (G_OBJECT (gtk_widget_get_toplevel (widget)),
			    "configure-event",
			    G_CALLBACK (configure_cb), this);
#else /* USE_GTK2 */
    gtk_signal_connect_after (GTK_OBJECT (gtk_widget_get_toplevel (widget)),
			      "configure-event",
			      GTK_SIGNAL_FUNC (configure_cb), this);
#endif /* USE_GTK2 */

    printf ("xine_thread: init threads\n");

    if (!XInitThreads ())
    {
	printf
	    ("XInitThreads failed - looks like you don't have a thread-safe xlib.\n");
	return;
    }

    printf ("xine_thread: open display\n");

    this->display = XOpenDisplay (NULL);

    if (!this->display)
    {
	printf ("XOpenDisplay failed!\n");
	return;
    }

    XLockDisplay (this->display);

    this->screen = DefaultScreen (this->display);

    if (XShmQueryExtension (this->display) == True)
    {
	this->completion_event =
	    XShmGetEventBase (this->display) + ShmCompletion;
    }
    else
    {
	this->completion_event = -1;
    }

#if (XINE_MAJOR_VERSION == 0) && (XINE_MINOR_VERSION == 9) && (XINE_SUB_VERSION < 9)
    XSelectInput (this->display, this->video_window,
		  /*
		   * StructureNotifyMask | 
		   */ ExposureMask
		  /*
		   * | ButtonPressMask | PointerMotionMask 
		   */ );
#else
    XSelectInput (this->display, this->video_window,
		  StructureNotifyMask | ExposureMask
		  /*
		   * | ButtonPressMask | PointerMotionMask 
		   */ );
#endif

    /*
     * generate and init a config "object"
     */
    g_snprintf (this->configfile, 255, "%s/.xine/config", getenv ("HOME"));
#if (XINE_MAJOR_VERSION == 0) && (XINE_MINOR_VERSION == 9) && (XINE_SUB_VERSION < 9)
    this->config = config_file_init (this->configfile);
#else
    this->config = xine_config_file_init (this->configfile);
#endif

    /*
     * load audio, video drivers
     */
    this->vo_driver = load_video_out_driver (this);

    if (!this->vo_driver)
    {
	printf ("couldn't open video driver\n");
	return;
    }

    this->ao_driver = load_audio_out_driver (this);

    /*
     * init xine engine
     */
    this->xine = xine_init (this->vo_driver, this->ao_driver, this->config);
    xine_set_locale ();

    xine_register_event_listener (this->xine, event_listener, this);

    values.foreground = BlackPixel (this->display, this->screen);
    values.background = WhitePixel (this->display, this->screen);

    this->gc = XCreateGC (this->display, this->video_window,
			  (GCForeground | GCBackground), &values);


    XUnlockDisplay (this->display);

    /*
     * create mouse cursors
     */
    bm_no = XCreateBitmapFromData (this->display,
				   this->video_window, bm_no_data, 8, 8);
    this->no_cursor
	= XCreatePixmapCursor (this->display, bm_no, bm_no,
			       (XColor *) & BlackPixel (gdk_display,
							DefaultScreen
							(gdk_display)),
			       (XColor *) & BlackPixel (gdk_display,
							DefaultScreen
							(gdk_display)), 0, 0);

    /*
     * now, create a xine thread
     */

    pthread_create (&this->thread, NULL, xine_thread, this);

    return;
}

static void
gtk_xine_unrealize (GtkWidget * widget)
{
    GtkXine *this;

    g_return_if_fail (widget);
    g_return_if_fail (GTK_IS_XINE (widget));

    this = GTK_XINE (widget);

    gtk_xine_stop (this);

    /*
     * stop event thread 
     */
    this->status = GTK_XINE_STATUS_EXIT;
    XFlush (this->display);
    /*
     * pthread_join (this->thread, NULL);
     * pthread_detach (this->thread);
     */

    /*
     * save configuration 
     */
    this->config->save (this->config);

    xine_exit (this->xine);

#ifdef USE_GTK2
    g_signal_handlers_disconnect_by_func (G_OBJECT
					  (gtk_widget_get_toplevel (widget)),
					  G_CALLBACK (configure_cb), this);
#else /* USE_GTK2 */
    gtk_signal_disconnect_by_func (GTK_OBJECT
				   (gtk_widget_get_toplevel (widget)),
				   GTK_SIGNAL_FUNC (configure_cb), this);
#endif /* USE_GTK2 */

    /*
     * Hide all windows 
     */
    if (GTK_WIDGET_MAPPED (widget))
	gtk_widget_unmap (widget);

    GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

    /*
     * This destroys widget->window and unsets the realized flag 
     */
    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
	(*GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

GtkWidget *
gtk_xine_new (void)
{
    GtkWidget *this;

#if (GTK_MAJOR_VERSION >= 2)
    this = GTK_WIDGET (g_object_new (gtk_xine_get_type (), NULL));
#else /* (GTK_MAJOR_VERSION >= 2) */
    this = GTK_WIDGET (gtk_type_new (gtk_xine_get_type ()));
#endif /* (GTK_MAJOR_VERSION >= 2) */

    return this;
}

static  gint
gtk_xine_expose (GtkWidget * widget, GdkEventExpose * event)
{
    /*
     * GtkXine *this = GTK_XINE (widget);  
     */

    return TRUE;
}

static void
gtk_xine_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
    GtkXine *this;

    g_return_if_fail (widget);
    g_return_if_fail (GTK_IS_XINE (widget));

    this = GTK_XINE (widget);

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
    {
	gdk_window_move_resize (widget->window,
				allocation->x,
				allocation->y,
				allocation->width - 2,
				allocation->height - 2);
    }
}

gint
gtk_xine_play (GtkXine * gtx, const gchar * mrl, gint pos, gint start_time)
{
    gint    retval;

    g_return_val_if_fail (gtx, -1);
    g_return_val_if_fail (GTK_IS_XINE (gtx), -1);
    g_return_val_if_fail (gtx->xine, -1);

    printf ("gtkxine: calling xine_play start_pos = %d, start_time = %d\n",
	    pos, start_time);

    retval = xine_play (gtx->xine, (char *) mrl, pos, start_time);

    if (retval)
#ifdef USE_GTK2
	g_signal_emit (G_OBJECT (gtx), gtk_xine_signals[PLAY_SIGNAL], 0);
#else /* USE_GTK2 */
	gtk_signal_emit (GTK_OBJECT (gtx), gtk_xine_signals[PLAY_SIGNAL]);
#endif /* USE_GTK2 */

    return retval;
}

void
gtk_xine_set_speed (GtkXine * gtx, gint speed)
{
    g_return_if_fail (gtx);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine);

    xine_set_speed (gtx->xine, speed);
}

gint
gtk_xine_get_speed (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, SPEED_NORMAL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), SPEED_NORMAL);
    g_return_val_if_fail (gtx->xine, SPEED_NORMAL);

    return xine_get_speed (gtx->xine);
}

void
gtk_xine_stop (GtkXine * gtx)
{
    gint    i;

    g_return_if_fail (gtx);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine);

    xine_stop (gtx->xine);

    for (i = 0; i < 1; i++)
    {
	usleep (1000);
	XFlush (gtx->display);
    }

#ifdef USE_GTK2
    g_signal_emit (G_OBJECT (gtx), gtk_xine_signals[STOP_SIGNAL], 0);
#else /* USE_GTK2 */
    gtk_signal_emit (GTK_OBJECT (gtx), gtk_xine_signals[STOP_SIGNAL]);
#endif /* USE_GTK2 */
}

gint
gtk_xine_get_position (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    if (gtk_xine_is_playing (gtx))
	return xine_get_current_position (gtx->xine);
    else
	return 0;
}

void
gtk_xine_set_audio_channel (GtkXine * gtx, gint audio_channel)
{
    g_return_if_fail (gtx);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine);

    xine_select_audio_channel (gtx->xine, audio_channel);
}

gint
gtk_xine_get_audio_channel (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return xine_get_audio_selection (gtx->xine);
}

#define MWM_HINTS_DECORATIONS   (1L << 1)
#define PROP_MWM_HINTS_ELEMENTS 5
typedef struct
{
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
}
MWMHints;

void
gtk_xine_set_fullscreen (GtkXine * gtx, gint fullscreen)
{
    static char *window_title = "gtk-xine fullscreen window";
    XSizeHints hint;
    Atom    prop;
    MWMHints mwmhints;
    XEvent  xev;

    g_return_if_fail (gtx);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine);

    if (fullscreen == gtx->fullscreen_mode)
	return;

    gtx->fullscreen_mode = fullscreen;

    XLockDisplay (gtx->display);

    if (gtx->fullscreen_mode)
    {
	hint.x = 0;
	hint.y = 0;
	hint.width = gtx->fullscreen_width;
	hint.height = gtx->fullscreen_height;

	gtx->fullscreen_window
	    = XCreateSimpleWindow (gtx->display,
				   RootWindow (gtx->display,
					       DefaultScreen (gtx->display)),
				   0, 0,
				   gtx->fullscreen_width,
				   gtx->fullscreen_height,
				   1,
				   BlackPixel (gtx->display,
					       DefaultScreen (gtx->display)),
				   BlackPixel (gtx->display,
					       DefaultScreen (gtx->display)));

	hint.win_gravity = StaticGravity;
	hint.flags = PPosition | PSize | PWinGravity;

	XSetStandardProperties (gtx->display, gtx->fullscreen_window,
				window_title, window_title, None, NULL, 0, 0);

	XSetWMNormalHints (gtx->display, gtx->fullscreen_window, &hint);

	/*
	 * XSetWMHints (gtx->display, gtx->fullscreen_window, gVw->wm_hint);
	 */

	/*
	 * wm, no borders please
	 */
	prop = XInternAtom (gtx->display, "_MOTIF_WM_HINTS", False);
	mwmhints.flags = MWM_HINTS_DECORATIONS;
	mwmhints.decorations = 0;
	XChangeProperty (gtx->display, gtx->fullscreen_window, prop, prop, 32,
			 PropModeReplace, (unsigned char *) &mwmhints,
			 PROP_MWM_HINTS_ELEMENTS);

	XSetTransientForHint (gtx->display, gtx->fullscreen_window, None);
	XRaiseWindow (gtx->display, gtx->fullscreen_window);


	XSelectInput (gtx->display, gtx->fullscreen_window,
		      StructureNotifyMask | ExposureMask | FocusChangeMask);

	/*
	 * Map window. 
	 */
	XMapRaised (gtx->display, gtx->fullscreen_window);

	XFlush (gtx->display);

	/*
	 * Wait for map. 
	 */
	do
	{
	    XMaskEvent (gtx->display, StructureNotifyMask, &xev);
	}
	while (xev.type != MapNotify
	       || xev.xmap.event != gtx->fullscreen_window);


	XSetInputFocus (gtx->display,
			gtx->toplevel, RevertToNone, CurrentTime);
	XMoveWindow (gtx->display, gtx->fullscreen_window, 0, 0);

	gtx->vo_driver->gui_data_exchange (gtx->vo_driver,
					   GUI_DATA_EX_DRAWABLE_CHANGED,
					   (void *) gtx->fullscreen_window);

	/*
	 * switch off mouse cursor
	 */
	XDefineCursor (gtx->display, gtx->fullscreen_window, gtx->no_cursor);
	XFlush (gtx->display);

    }
    else
    {
	gtx->vo_driver->gui_data_exchange (gtx->vo_driver,
					   GUI_DATA_EX_DRAWABLE_CHANGED,
					   (void *) gtx->video_window);

	XDestroyWindow (gtx->display, gtx->fullscreen_window);
    }

    XUnlockDisplay (gtx->display);
}

gint
gtk_xine_is_fullscreen (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return gtx->fullscreen_mode;
}

gchar **
gtk_xine_get_autoplay_plugins (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);

    return xine_get_autoplay_input_plugin_ids (gtx->xine);
}

gint
gtk_xine_get_current_time (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return xine_get_current_time (gtx->xine);
}

gint
gtk_xine_get_stream_length (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return xine_get_stream_length (gtx->xine);
}

gint
gtk_xine_is_playing (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return xine_get_status (gtx->xine) == XINE_PLAY;
}

gint
gtk_xine_is_seekable (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return xine_is_stream_seekable (gtx->xine);
}

void
gtk_xine_save_config (GtkXine * gtx)
{
    g_return_if_fail (gtx);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine);

    gtx->config->save (gtx->config);
}

void
gtk_xine_set_video_property (GtkXine * gtx, gint property, gint value)
{
    g_return_if_fail (gtx);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine);

    gtx->vo_driver->set_property (gtx->vo_driver, property, value);
}

gint
gtk_xine_get_video_property (GtkXine * gtx, gint property)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return gtx->vo_driver->get_property (gtx->vo_driver, property);
}

#if 0
gint
gtk_xine_get_log_section_count (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine, 0);

    return xine_get_log_section_count (gtx->xine);
}

gchar **
gtk_xine_get_log_names (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);

    return xine_get_log_names (gtx->xine);
}
#endif

gchar **
gtk_xine_get_log (GtkXine * gtx, gint buf)
{
    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);

    return xine_get_log (gtx->xine, buf);
}

gchar **
gtk_xine_get_browsable_input_plugin_ids (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);

    return xine_get_browsable_input_plugin_ids (gtx->xine);
}

gchar **
gtk_xine_get_autoplay_input_plugin_ids (GtkXine * gtx)
{
    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);

    return xine_get_autoplay_input_plugin_ids (gtx->xine);
}

gchar **
gtk_xine_get_autoplay_mrls (GtkXine * gtx, gchar * plugin_id, gint * num_mrls)
{
    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);

    return xine_get_autoplay_mrls (gtx->xine, plugin_id, num_mrls);
}

/*
 *  For screen shot.
 */
#define PIXSZ 3
#define BIT_DEPTH 8

typedef void (*scale_line_func_t) (guchar * source, guchar * dest,
				   gint width, gint step);

struct prvt_image_s
{
    gint    width;
    gint    height;
    gint    ratio_code;
    gint    format;
    guchar *y, *u, *v, *yuy2;

    gint    u_width, v_width;
    gint    u_height, v_height;

    scale_line_func_t scale_line;
    unsigned long scale_factor;
};

static guchar *xine_frame_to_rgb (struct prvt_image_s *image);

guchar *
gtk_xine_get_current_frame_rgb (GtkXine * gtx, gint * width_ret,
				gint * height_ret)
{
    gint    err = 0;
    struct prvt_image_s *image;
    guchar *rgb = NULL;

    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);
    g_return_val_if_fail (width_ret && height_ret, NULL);

    image = g_new0 (struct prvt_image_s, 1);
    if (!image)
	return NULL;
    image->y = image->u = image->v = image->yuy2 = NULL;

    err = xine_get_current_frame (gtx->xine,
				  &image->width, &image->height,
				  &image->ratio_code,
				  &image->format,
				  &image->y, &image->u, &image->v);

    if (err == 0)
	goto ERROR;

    /*
     * the dxr3 driver does not allocate yuv buffers 
     */
    /*
     * image->u and image->v are always 0 for YUY2 
     */
    if (!image->y)
	goto ERROR;

    rgb = xine_frame_to_rgb (image);
    *width_ret = image->width;
    *height_ret = image->height;

  ERROR:
    g_free (image);
    return rgb;
}

/******************************************************************************
 *
 *   Private functions for snap shot.
 *
 *   These codes are mostly taken from xine-ui.
 *
 ******************************************************************************/
/*
 * Scale line with no horizontal scaling. For NTSC mpeg2 dvd input in
 * 4:3 output format (720x480 -> 720x540)
 */
static void
scale_line_1_1 (guchar * source, guchar * dest, gint width, gint step)
{
    memcpy (dest, source, width);
}

/*
 * Interpolates 64 output pixels from 45 source pixels using shifts.
 * Useful for scaling a PAL mpeg2 dvd input source to 1024x768
 * fullscreen resolution, or to 16:9 format on a monitor using square
 * pixels.
 * (720 x 576 ==> 1024 x 576)
 */
/* uuuuum */
static void
scale_line_45_64 (guchar * source, guchar * dest, gint width, gint step)
{
    gint    p1, p2;

    while ((width -= 64) >= 0)
    {
	p1 = source[0];
	p2 = source[1];
	dest[0] = p1;
	dest[1] = (1 * p1 + 3 * p2) >> 2;
	p1 = source[2];
	dest[2] = (5 * p2 + 3 * p1) >> 3;
	p2 = source[3];
	dest[3] = (7 * p1 + 1 * p2) >> 3;
	dest[4] = (1 * p1 + 3 * p2) >> 2;
	p1 = source[4];
	dest[5] = (1 * p2 + 1 * p1) >> 1;
	p2 = source[5];
	dest[6] = (3 * p1 + 1 * p2) >> 2;
	dest[7] = (1 * p1 + 7 * p2) >> 3;
	p1 = source[6];
	dest[8] = (3 * p2 + 5 * p1) >> 3;
	p2 = source[7];
	dest[9] = (5 * p1 + 3 * p2) >> 3;
	p1 = source[8];
	dest[10] = p2;
	dest[11] = (1 * p2 + 3 * p1) >> 2;
	p2 = source[9];
	dest[12] = (5 * p1 + 3 * p2) >> 3;
	p1 = source[10];
	dest[13] = (7 * p2 + 1 * p1) >> 3;
	dest[14] = (1 * p2 + 7 * p1) >> 3;
	p2 = source[11];
	dest[15] = (1 * p1 + 1 * p2) >> 1;
	p1 = source[12];
	dest[16] = (3 * p2 + 1 * p1) >> 2;
	dest[17] = p1;
	p2 = source[13];
	dest[18] = (3 * p1 + 5 * p2) >> 3;
	p1 = source[14];
	dest[19] = (5 * p2 + 3 * p1) >> 3;
	p2 = source[15];
	dest[20] = p1;
	dest[21] = (1 * p1 + 3 * p2) >> 2;
	p1 = source[16];
	dest[22] = (1 * p2 + 1 * p1) >> 1;
	p2 = source[17];
	dest[23] = (7 * p1 + 1 * p2) >> 3;
	dest[24] = (1 * p1 + 7 * p2) >> 3;
	p1 = source[18];
	dest[25] = (3 * p2 + 5 * p1) >> 3;
	p2 = source[19];
	dest[26] = (3 * p1 + 1 * p2) >> 2;
	dest[27] = p2;
	p1 = source[20];
	dest[28] = (3 * p2 + 5 * p1) >> 3;
	p2 = source[21];
	dest[29] = (5 * p1 + 3 * p2) >> 3;
	p1 = source[22];
	dest[30] = (7 * p2 + 1 * p1) >> 3;
	dest[31] = (1 * p2 + 3 * p1) >> 2;
	p2 = source[23];
	dest[32] = (1 * p1 + 1 * p2) >> 1;
	p1 = source[24];
	dest[33] = (3 * p2 + 1 * p1) >> 2;
	dest[34] = (1 * p2 + 7 * p1) >> 3;
	p2 = source[25];
	dest[35] = (3 * p1 + 5 * p2) >> 3;
	p1 = source[26];
	dest[36] = (3 * p2 + 1 * p1) >> 2;
	p2 = source[27];
	dest[37] = p1;
	dest[38] = (1 * p1 + 3 * p2) >> 2;
	p1 = source[28];
	dest[39] = (5 * p2 + 3 * p1) >> 3;
	p2 = source[29];
	dest[40] = (7 * p1 + 1 * p2) >> 3;
	dest[41] = (1 * p1 + 7 * p2) >> 3;
	p1 = source[30];
	dest[42] = (1 * p2 + 1 * p1) >> 1;
	p2 = source[31];
	dest[43] = (3 * p1 + 1 * p2) >> 2;
	dest[44] = (1 * p1 + 7 * p2) >> 3;
	p1 = source[32];
	dest[45] = (3 * p2 + 5 * p1) >> 3;
	p2 = source[33];
	dest[46] = (5 * p1 + 3 * p2) >> 3;
	p1 = source[34];
	dest[47] = p2;
	dest[48] = (1 * p2 + 3 * p1) >> 2;
	p2 = source[35];
	dest[49] = (1 * p1 + 1 * p2) >> 1;
	p1 = source[36];
	dest[50] = (7 * p2 + 1 * p1) >> 3;
	dest[51] = (1 * p2 + 7 * p1) >> 3;
	p2 = source[37];
	dest[52] = (1 * p1 + 1 * p2) >> 1;
	p1 = source[38];
	dest[53] = (3 * p2 + 1 * p1) >> 2;
	dest[54] = p1;
	p2 = source[39];
	dest[55] = (3 * p1 + 5 * p2) >> 3;
	p1 = source[40];
	dest[56] = (5 * p2 + 3 * p1) >> 3;
	p2 = source[41];
	dest[57] = (7 * p1 + 1 * p2) >> 3;
	dest[58] = (1 * p1 + 3 * p2) >> 2;
	p1 = source[42];
	dest[59] = (1 * p2 + 1 * p1) >> 1;
	p2 = source[43];
	dest[60] = (7 * p1 + 1 * p2) >> 3;
	dest[61] = (1 * p1 + 7 * p2) >> 3;
	p1 = source[44];
	dest[62] = (3 * p2 + 5 * p1) >> 3;
	p2 = source[45];
	dest[63] = (3 * p1 + 1 * p2) >> 2;
	source += 45;
	dest += 64;
    }

    if ((width += 64) <= 0)
	goto done;
    *dest++ = source[0];
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[0] + 3 * source[1]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[1] + 3 * source[2]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[2] + 1 * source[3]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[2] + 3 * source[3]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[3] + 1 * source[4]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[4] + 1 * source[5]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[4] + 7 * source[5]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[5] + 5 * source[6]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[6] + 3 * source[7]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = source[7];
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[7] + 3 * source[8]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[8] + 3 * source[9]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[9] + 1 * source[10]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[9] + 7 * source[10]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[10] + 1 * source[11]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[11] + 1 * source[12]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = source[12];
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[12] + 5 * source[13]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[13] + 3 * source[14]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = source[14];
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[14] + 3 * source[15]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[15] + 1 * source[16]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[16] + 1 * source[17]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[16] + 7 * source[17]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[17] + 5 * source[18]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[18] + 1 * source[19]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = source[19];
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[19] + 5 * source[20]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[20] + 3 * source[21]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[21] + 1 * source[22]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[21] + 3 * source[22]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[22] + 1 * source[23]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[23] + 1 * source[24]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[23] + 7 * source[24]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[24] + 5 * source[25]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[25] + 1 * source[26]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = source[26];
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[26] + 3 * source[27]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[27] + 3 * source[28]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[28] + 1 * source[29]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[28] + 7 * source[29]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[29] + 1 * source[30]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[30] + 1 * source[31]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[30] + 7 * source[31]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[31] + 5 * source[32]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[32] + 3 * source[33]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = source[33];
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[33] + 3 * source[34]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[34] + 1 * source[35]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[35] + 1 * source[36]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[35] + 7 * source[36]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[36] + 1 * source[37]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[37] + 1 * source[38]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = source[38];
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[38] + 5 * source[39]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[39] + 3 * source[40]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[40] + 1 * source[41]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[40] + 3 * source[41]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[41] + 1 * source[42]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[42] + 1 * source[43]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[42] + 7 * source[43]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[43] + 5 * source[44]) >> 3;
  done:;
}

/*
 * Interpolates 16 output pixels from 15 source pixels using shifts.
 * Useful for scaling a PAL mpeg2 dvd input source to 4:3 format on
 * a monitor using square pixels.
 * (720 x 576 ==> 768 x 576)
 */
/* uum */
static void
scale_line_15_16 (guchar * source, guchar * dest, gint width, gint step)
{
    gint    p1, p2;

    while ((width -= 16) >= 0)
    {
	p1 = source[0];
	dest[0] = p1;
	p2 = source[1];
	dest[1] = (1 * p1 + 7 * p2) >> 3;
	p1 = source[2];
	dest[2] = (1 * p2 + 7 * p1) >> 3;
	p2 = source[3];
	dest[3] = (1 * p1 + 3 * p2) >> 2;
	p1 = source[4];
	dest[4] = (1 * p2 + 3 * p1) >> 2;
	p2 = source[5];
	dest[5] = (3 * p1 + 5 * p2) >> 3;
	p1 = source[6];
	dest[6] = (3 * p2 + 5 * p1) >> 3;
	p2 = source[7];
	dest[7] = (1 * p1 + 1 * p1) >> 1;
	p1 = source[8];
	dest[8] = (1 * p2 + 1 * p1) >> 1;
	p2 = source[9];
	dest[9] = (5 * p1 + 3 * p2) >> 3;
	p1 = source[10];
	dest[10] = (5 * p2 + 3 * p1) >> 3;
	p2 = source[11];
	dest[11] = (3 * p1 + 1 * p2) >> 2;
	p1 = source[12];
	dest[12] = (3 * p2 + 1 * p1) >> 2;
	p2 = source[13];
	dest[13] = (7 * p1 + 1 * p2) >> 3;
	p1 = source[14];
	dest[14] = (7 * p2 + 1 * p1) >> 3;
	dest[15] = p1;
	source += 15;
	dest += 16;
    }

    if ((width += 16) <= 0)
	goto done;
    *dest++ = source[0];
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[0] + 7 * source[1]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[1] + 7 * source[2]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[2] + 3 * source[3]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[3] + 3 * source[4]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[4] + 5 * source[5]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[5] + 5 * source[6]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[6] + 1 * source[7]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (1 * source[7] + 1 * source[8]) >> 1;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[8] + 3 * source[9]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (5 * source[9] + 3 * source[10]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[10] + 1 * source[11]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (3 * source[11] + 1 * source[12]) >> 2;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[12] + 1 * source[13]) >> 3;
    if (--width <= 0)
	goto done;
    *dest++ = (7 * source[13] + 1 * source[14]) >> 3;
  done:;
}

int
scale_image (struct prvt_image_s *image)
{
    gint    i;
    gint    step = 1;		/* unused variable for the scale functions */

    /*
     * pointers for post-scaled line buffer 
     */
    guchar *n_y;
    guchar *n_u;
    guchar *n_v;

    /*
     * pointers into pre-scaled line buffers 
     */
    guchar *oy = image->y;
    guchar *ou = image->u;
    guchar *ov = image->v;
    guchar *oy_p = image->y;
    guchar *ou_p = image->u;
    guchar *ov_p = image->v;

    /*
     * pointers into post-scaled line buffers 
     */
    guchar *ny_p;
    guchar *nu_p;
    guchar *nv_p;

    /*
     * old line widths 
     */
    gint    oy_width = image->width;
    gint    ou_width = image->u_width;
    gint    ov_width = image->v_width;

    /*
     * new line widths NB scale factor is factored by 32768 for rounding 
     */
    gint    ny_width = (oy_width * image->scale_factor) / 32768;
    gint    nu_width = (ou_width * image->scale_factor) / 32768;
    gint    nv_width = (ov_width * image->scale_factor) / 32768;

    /*
     * allocate new buffer space space for post-scaled line buffers 
     */
    n_y = g_malloc (ny_width * image->height);
    if (!n_y)
	return 0;
    n_u = g_malloc (nu_width * image->u_height);
    if (!n_u)
	return 0;
    n_v = g_malloc (nv_width * image->v_height);
    if (!n_v)
	return 0;

    /*
     * set post-scaled line buffer progress pointers 
     */
    ny_p = n_y;
    nu_p = n_u;
    nv_p = n_v;

    /*
     * Do the scaling 
     */

    for (i = 0; i < image->height; ++i)
    {
	image->scale_line (oy_p, ny_p, ny_width, step);
	oy_p += oy_width;
	ny_p += ny_width;
    }

    for (i = 0; i < image->u_height; ++i)
    {
	image->scale_line (ou_p, nu_p, nu_width, step);
	ou_p += ou_width;
	nu_p += nu_width;
    }

    for (i = 0; i < image->v_height; ++i)
    {
	image->scale_line (ov_p, nv_p, nv_width, step);
	ov_p += ov_width;
	nv_p += nv_width;
    }

    /*
     * switch to post-scaled data and widths 
     */
    image->y = n_y;
    image->u = n_u;
    image->v = n_v;
    image->width = ny_width;
    image->u_width = nu_width;
    image->v_width = nv_width;

    if (image->yuy2)
    {
	g_free (oy);
	g_free (ou);
	g_free (ov);
    }

    return 1;
}

/*
 *  This function was pinched from filter_yuy2tov12.c, part of
 *  transcode, a linux video stream processing tool
 *
 *  Copyright (C) Thomas ®Östreich - June 2001
 *
 *  Thanks Thomas
 *      
 */
static void
yuy2toyv12 (struct prvt_image_s *image)
{
    gint    i, j, w2;

    /*
     * I420 
     */
    guchar *y = image->y;
    guchar *u = image->u;
    guchar *v = image->v;

    guchar *input = image->yuy2;

    gint    width = image->width;
    gint    height = image->height;

    w2 = width / 2;

    for (i = 0; i < height; i += 2)
    {
	for (j = 0; j < w2; j++)
	{

	    /*
	     * packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] 
	     */
	    *(y++) = *(input++);
	    *(u++) = *(input++);
	    *(y++) = *(input++);
	    *(v++) = *(input++);
	}

	/*
	 * down sampling 
	 */

	for (j = 0; j < w2; j++)
	{
	    /*
	     * skip every second line for U and V 
	     */
	    *(y++) = *(input++);
	    input++;
	    *(y++) = *(input++);
	    input++;
	}
    }
}

/*
 *  This function is a fudge .. a hack.
 *
 *  It is in place purely to get snapshots going for YUY2 data
 *  longer term there needs to be a bit of a reshuffle to account
 *  for the two fundamentally different YUV formats. Things would
 *  have been different had I known how YUY2 was done before designing
 *  the flow. Teach me to make assumptions I guess.
 *
 *  So .. this function converts the YUY2 image to YV12. The downside
 *  being that as YV12 has half as many chroma rows as YUY2, there is
 *  a loss of image quality.
 */
/* uuuuuuuuuuuuuuum. */
static  gint
yuy2_fudge (struct prvt_image_s *image)
{
    image->y = g_malloc0 (image->height * image->width);
    if (!image->y)
	goto ERROR0;

    image->u = g_malloc0 (image->u_height * image->u_width);
    if (!image->u)
	goto ERROR1;

    image->v = g_malloc0 (image->v_height * image->v_width);
    if (!image->v)
	goto ERROR2;

    yuy2toyv12 (image);

    /*
     * image->yuy2 = NULL; 
 *//*
 * * * * * I will use this value as flag
 * * * * * to free yuv data in scale_image () 
 */

    return 1;

  ERROR2:
    g_free (image->u);
    image->u = NULL;
  ERROR1:
    g_free (image->y);
    image->y = NULL;
  ERROR0:
    return 0;
}

#define clip_8_bit(val)                                                        \
{                                                                              \
   if (val < 0)                                                                \
      val = 0;                                                                 \
   else                                                                        \
      if (val > 255) val = 255;                                                \
}

/*
 *   Create rgb data from yv12
 */
static guchar *
yv12_2_rgb (struct prvt_image_s *image)
{
    gint    i, j;

    gint    y, u, v;
    gint    r, g, b;

    gint    sub_i_u;
    gint    sub_i_v;

    gint    sub_j_u;
    gint    sub_j_v;

    guchar *rgb;

    rgb = g_malloc0 (image->width * image->height * PIXSZ * sizeof (guchar));
    if (!rgb)
	return NULL;

    for (i = 0; i < image->height; ++i)
    {
	/*
	 * calculate u & v rows 
	 */
	sub_i_u = ((i * image->u_height) / image->height);
	sub_i_v = ((i * image->v_height) / image->height);

	for (j = 0; j < image->width; ++j)
	{
	    /*
	     * calculate u & v columns 
	     */
	    sub_j_u = ((j * image->u_width) / image->width);
	    sub_j_v = ((j * image->v_width) / image->width);

	 /***************************************************
          *
          *  Colour conversion from 
          *    http://www.inforamp.net/~poynton/notes/colour_and_gamma/ColorFAQ.html#RTFToC30
          *
          *  Thanks to Billy Biggs <vektor@dumbterm.net>
          *  for the pointer and the following conversion.
          *
          *   R' = [ 1.1644         0    1.5960 ]   ([ Y' ]   [  16 ])
          *   G' = [ 1.1644   -0.3918   -0.8130 ] * ([ Cb ] - [ 128 ])
          *   B' = [ 1.1644    2.0172         0 ]   ([ Cr ]   [ 128 ])
          *
          *  Where in Xine the above values are represented as
          *
          *   Y' == image->y
          *   Cb == image->u
          *   Cr == image->v
          *
          ***************************************************/

	    y = image->y[(i * image->width) + j] - 16;
	    u = image->u[(sub_i_u * image->u_width) + sub_j_u] - 128;
	    v = image->v[(sub_i_v * image->v_width) + sub_j_v] - 128;

	    r = (1.1644 * y) + (1.5960 * v);
	    g = (1.1644 * y) - (0.3918 * u) - (0.8130 * v);
	    b = (1.1644 * y) + (2.0172 * u);

	    clip_8_bit (r);
	    clip_8_bit (g);
	    clip_8_bit (b);

	    rgb[(i * image->width + j) * PIXSZ + 0] = r;
	    rgb[(i * image->width + j) * PIXSZ + 1] = g;
	    rgb[(i * image->width + j) * PIXSZ + 2] = b;
	}
    }

    return rgb;
}

static guchar *
xine_frame_to_rgb (struct prvt_image_s *image)
{
    guchar *rgb;

    g_return_val_if_fail (image, NULL);

    switch (image->ratio_code)
    {
      case XINE_ASPECT_RATIO_SQUARE:
	  image->scale_line = scale_line_1_1;
	  image->scale_factor = (32768 * 1) / 1;
	  break;

      case XINE_ASPECT_RATIO_4_3:
	  image->scale_line = scale_line_15_16;
	  image->scale_factor = (32768 * 16) / 15;
	  break;

      case XINE_ASPECT_RATIO_ANAMORPHIC:
	  image->scale_line = scale_line_45_64;
	  image->scale_factor = (32768 * 64) / 45;
	  break;

      case XINE_ASPECT_RATIO_211_1:
	  image->scale_line = scale_line_45_64;
	  image->scale_factor = (32768 * 64) / 45;
	  break;

      case XINE_ASPECT_RATIO_DONT_TOUCH:
	  image->scale_line = scale_line_1_1;
	  image->scale_factor = (32768 * 1) / 1;
	  break;

      default:
	  /*
	   * the mpeg standard has a few that we don't know about 
	   */
	  g_print ("unknown aspect ratio. will assume 1:1\n");
	  image->scale_line = scale_line_1_1;
	  image->scale_factor = (32768 * 1) / 1;
	  break;
    }

    switch (image->format)
    {
      case XINE_IMGFMT_YV12:
	  image->u_width = ((image->width + 1) / 2);
	  image->v_width = ((image->width + 1) / 2);
	  image->u_height = ((image->height + 1) / 2);
	  image->v_height = ((image->height + 1) / 2);
	  break;

      case XINE_IMGFMT_YUY2:
	  image->u_width = ((image->width + 1) / 2);
	  image->v_width = ((image->width + 1) / 2);
	  image->u_height = ((image->height + 1) / 2);
	  image->v_height = ((image->height + 1) / 2);
	  break;

      default:
	  return NULL;
    }

    /*
     *  If YUY2 convert to YV12
     */
    if (image->format == XINE_IMGFMT_YUY2)
    {
	if (!yuy2_fudge (image))
	    return NULL;
    }

    scale_image (image);

    rgb = yv12_2_rgb (image);

    /*
     * FIXME 
     */
    g_free (image->y);
    g_free (image->u);
    g_free (image->v);
    image->y = NULL;
    image->u = NULL;
    image->v = NULL;

    return rgb;
}

#endif /* ENABLE_XINE_OLD */
