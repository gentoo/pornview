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

#include "gtkxine.h"

#ifdef ENABLE_XINE

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
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmain.h>
#include <gdk/gdkx.h>
#include <xine.h>

#include "gtk2-compat.h"

enum
{
    PLAY_SIGNAL,
    STOP_SIGNAL,
    PLAYBACK_FINISHED_SIGNAL,
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

    if (!gtk_xine_type)
    {
	static const GtkTypeInfo gtk_xine_info = {
	    "gtkxine",
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

    widget_class->realize = gtk_xine_realize;
    widget_class->unrealize = gtk_xine_unrealize;

    widget_class->size_allocate = gtk_xine_size_allocate;

    widget_class->expose_event = gtk_xine_expose;

    class->play = NULL;
    class->stop = NULL;
    class->playback_finished = NULL;
}

static void
gtk_xine_init (GtkXine * this)
{
    this->widget.requisition.width = 420;
    this->widget.requisition.height = 278;

    /*
     * create a new xine instance, load config values
     */

    this->xine = xine_new ();

    snprintf (this->configfile, 255, "%s/.xine/config", getenv ("HOME"));
    xine_config_load (this->xine, this->configfile);

    this->stream = NULL;
    this->event_queue = NULL;
    this->vo_driver = NULL;
    this->ao_driver = NULL;
    this->display = NULL;
    this->fullscreen_mode = FALSE;

    xine_init (this->xine);
}

static void
dest_size_cb (void *gxine_gen,
	      int video_width, int video_height,
	      double video_pixel_aspect,
	      int *dest_width, int *dest_height, double *dest_pixel_aspect)
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

    *dest_pixel_aspect = video_pixel_aspect * gxine->display_ratio;
}

static void
frame_output_cb (void *gxine_gen,
		 int video_width, int video_height,
		 double video_pixel_aspect,
		 int *dest_x, int *dest_y,
		 int *dest_width, int *dest_height,
		 double *dest_pixel_aspect, int *win_x, int *win_y)
{
    GtkXine *gxine = (GtkXine *) gxine_gen;

    *dest_x = 0;
    *dest_y = 0;
    *win_x = gxine->xpos;
    *win_y = gxine->ypos;

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

    *dest_pixel_aspect = video_pixel_aspect * gxine->display_ratio;
}

static xine_vo_driver_t *
load_video_out_driver (GtkXine * this)
{
    double  res_h, res_v;
    x11_visual_t vis;
    const char *video_driver_id;
    xine_vo_driver_t *vo_driver;

    vis.display = this->display;
    vis.screen = this->screen;
    vis.d = this->video_window;
    res_h = (DisplayWidth (this->display, this->screen) * 1000
	     / DisplayWidthMM (this->display, this->screen));
    res_v = (DisplayHeight (this->display, this->screen) * 1000
	     / DisplayHeightMM (this->display, this->screen));
    this->display_ratio = res_h / res_v;

    if (fabs (this->display_ratio - 1.0) < 0.01)
    {
	this->display_ratio = 1.0;
    }

    vis.dest_size_cb = dest_size_cb;
    vis.frame_output_cb = frame_output_cb;
    vis.user_data = this;

    /*
     * try to init video with stored information 
     */
    video_driver_id = xine_config_register_string (this->xine,
						   "video.driver", "auto",
						   "video driver to use",
						   NULL, 10, NULL, NULL);
    if (strcmp (video_driver_id, "auto"))
    {

	vo_driver = xine_open_video_driver (this->xine,
					    video_driver_id,
					    XINE_VISUAL_TYPE_X11,
					    (void *) &vis);
	if (vo_driver)
	    return vo_driver;
	else
	    printf ("gtkxine: video driver %s failed.\n", video_driver_id);	/* => auto-detect */
    }

    printf ("gtkxine: auto-detecting video driver...\n");

    return xine_open_video_driver (this->xine, NULL,
				   XINE_VISUAL_TYPE_X11, (void *) &vis);
}

static xine_ao_driver_t *
load_audio_out_driver (GtkXine * this)
{
    xine_ao_driver_t *ao_driver;
    const char *audio_driver_id;

    /*
     * try to init audio with stored information 
     */
    audio_driver_id = xine_config_register_string (this->xine,
						   "audio.driver", "auto",
						   "audio driver to use",
						   NULL, 10, NULL, NULL);

    if (!strcmp (audio_driver_id, "null"))
	return NULL;

    if (strcmp (audio_driver_id, "auto"))
    {

	ao_driver =
	    xine_open_audio_driver (this->xine, audio_driver_id, NULL);
	if (ao_driver)
	    return ao_driver;
	else
	    printf ("audio driver %s failed\n", audio_driver_id);
    }

    /*
     * autoprobe 
     */
    return xine_open_audio_driver (this->xine, NULL, NULL);
}

static void *
xine_thread (void *this_gen)
{
    GtkXine *this = (GtkXine *) this_gen;
    /*
     * GtkWidget  *widget = &this->widget; 
     */

    while (1)
    {
	XEvent  event;

	XNextEvent (this->display, &event);

	/*
	 * printf ("gtkxine: got an event (%d)\n", event.type);  
	 */
	switch (event.type)
	{
	  case Expose:

	      if (event.xexpose.count != 0)
		  break;

	      xine_gui_send_vo_data (this->stream,
				     XINE_GUI_SEND_EXPOSE_EVENT, &event);
	      break;

	  case FocusIn:	/* happens only in fullscreen mode */
	      XLockDisplay (this->display);
	      XSetInputFocus (this->display,
			      this->toplevel, RevertToNone, CurrentTime);
	      XUnlockDisplay (this->display);
	      break;
	}

	if (event.type == this->completion_event)
	{
	    xine_gui_send_vo_data (this->stream,
				   XINE_GUI_SEND_COMPLETION_EVENT, &event);
	    /*
	     * printf ("gtkxine: completion event\n"); 
	     */
	}
    }

    pthread_exit (NULL);
    return NULL;
}

static void
configure_cb (GtkWidget * widget,
	      GdkEventConfigure * event, gpointer user_data)
{
    GtkXine *this;

    this = GTK_XINE (user_data);

    this->xpos = event->x;
    this->ypos = event->y;
}

void
event_listener (void *data, const xine_event_t * event)
{
    GtkXine *gtx = GTK_XINE (data);

    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (event);

    switch (event->type)
    {
      case XINE_EVENT_UI_PLAYBACK_FINISHED:
	  g_signal_emit (G_OBJECT (gtx),
			 gtk_xine_signals[PLAYBACK_FINISHED_SIGNAL], 0);
	  break;

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
    XGCValues values;
    Pixmap  bm_no;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_XINE (widget));

    this = GTK_XINE (widget);

    /*
     * set realized flag 
     */

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    /*
     * create our own video window
     */

    this->video_window
	= XCreateSimpleWindow (gdk_display,
			       GDK_WINDOW_XWINDOW
			       (gtk_widget_get_parent_window
				(widget)), 0, 0,
			       widget->allocation.width,
			       widget->allocation.height, 1,
			       BlackPixel (gdk_display,
					   DefaultScreen (gdk_display)),
			       BlackPixel (gdk_display,
					   DefaultScreen (gdk_display)));

    widget->window = gdk_window_foreign_new (this->video_window);

    /*
     * prepare for fullscreen playback
     */

    this->fullscreen_width =
	DisplayWidth (gdk_display, DefaultScreen (gdk_display));
    this->fullscreen_height =
	DisplayHeight (gdk_display, DefaultScreen (gdk_display));

    this->fullscreen_mode = 0;

    this->toplevel =
	GDK_WINDOW_XWINDOW (gdk_window_get_toplevel
			    (gtk_widget_get_parent_window (widget)));

    /*
     * track configure events of toplevel window
     */
    g_signal_connect_after (G_OBJECT (gtk_widget_get_toplevel (widget)),
			    "configure-event",
			    G_CALLBACK (configure_cb), this);

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

    XSelectInput (this->display, this->video_window,
		  StructureNotifyMask | ExposureMask |
		  ButtonPressMask | PointerMotionMask);

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
     * create a stream object
     */

    this->stream = xine_stream_new (this->xine, this->ao_driver,
				    this->vo_driver);

    this->event_queue = xine_event_new_queue (this->stream);
    xine_event_create_listener_thread (this->event_queue, event_listener,
				       this);

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
    this->no_cursor = XCreatePixmapCursor (this->display, bm_no, bm_no,
					   (XColor *) &
					   BlackPixel (gdk_display,
						       DefaultScreen
						       (gdk_display)),
					   (XColor *) &
					   BlackPixel (gdk_display,
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

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_XINE (widget));

    this = GTK_XINE (widget);

    /*
     * stop the playback 
     */
    xine_close (this->stream);
    xine_event_dispose_queue (this->event_queue);
    xine_dispose (this->stream);
    this->stream = NULL;

    /*
     * stop event thread 
     */
    pthread_cancel (this->thread);

    /*
     * kill the drivers 
     */
    if (this->vo_driver != NULL)
	xine_close_video_driver (this->xine, this->vo_driver);
    if (this->ao_driver != NULL)
	xine_close_audio_driver (this->xine, this->ao_driver);

    /*
     * save configuration 
     */
    printf ("gtkxine: saving configuration...\n");
    xine_config_save (this->xine, this->configfile);

    /*
     * exit xine 
     */
    xine_exit (this->xine);
    this->xine = NULL;

    g_signal_handlers_disconnect_by_func (G_OBJECT
					  (gtk_widget_get_toplevel (widget)),
					  G_CALLBACK (configure_cb), this);

    /*
     * Hide all windows 
     */

    if (GTK_WIDGET_MAPPED (widget))
	gtk_widget_unmap (widget);

    GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

    /*
     * this destroys widget->window and unsets the realized flag 
     */
    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
	(*GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

GtkWidget *
gtk_xine_new (void)
{
    GtkWidget *this = GTK_WIDGET (gtk_type_new (gtk_xine_get_type ()));

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

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_XINE (widget));

    this = GTK_XINE (widget);

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
    {
	gdk_window_move_resize (widget->window,
				allocation->x,
				allocation->y,
				allocation->width, allocation->height);
    }
}

gint
gtk_xine_open (GtkXine * gtx, const gchar * mrl)
{
    g_return_val_if_fail (gtx != NULL, -1);
    g_return_val_if_fail (GTK_IS_XINE (gtx), -1);
    g_return_val_if_fail (gtx->xine != NULL, -1);

    printf ("gtkxine: calling xine_open, mrl = '%s'\n", mrl);

    return xine_open (gtx->stream, mrl);
}

gint
gtk_xine_play (GtkXine * gtx, gint pos, gint start_time)
{
    gint    retval;

    g_return_val_if_fail (gtx != NULL, -1);
    g_return_val_if_fail (GTK_IS_XINE (gtx), -1);
    g_return_val_if_fail (gtx->xine != NULL, -1);

    printf ("gtkxine: calling xine_play start_pos = %d, start_time = %d\n",
	    pos, start_time);

    retval = xine_play (gtx->stream, pos, start_time);

    if (retval)
	g_signal_emit (G_OBJECT (gtx), gtk_xine_signals[PLAY_SIGNAL], 0);

    return retval;
}

gint
gtk_xine_trick_mode (GtkXine * gtx, gint mode, gint value)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_trick_mode (gtx->stream, mode, value);
}

gint
gtk_xine_get_pos_length (GtkXine * gtx, gint * pos_stream,
			 gint * pos_time, gint * length_time)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_pos_length (gtx->stream, pos_stream, pos_time,
				length_time);
}

void
gtk_xine_stop (GtkXine * gtx)
{
    gint    i;

    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->stream != NULL);

    xine_stop (gtx->stream);

    for (i = 0; i < 1; i++)
    {
	usleep (1000);
	XFlush (gtx->display);
    }

    g_signal_emit (G_OBJECT (gtx), gtk_xine_signals[STOP_SIGNAL], 0);
}

gint
gtk_xine_get_error (GtkXine * gtx)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_error (gtx->stream);
}

gint
gtk_xine_get_status (GtkXine * gtx)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_status (gtx->stream);
}

void
gtk_xine_set_param (GtkXine * gtx, gint param, gint value)
{
    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->stream != NULL);

    xine_set_param (gtx->stream, param, value);
}

gint
gtk_xine_get_param (GtkXine * gtx, gint param)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_param (gtx->stream, param);
}

gint
gtk_xine_get_audio_lang (GtkXine * gtx, gint channel, gchar * lang)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_audio_lang (gtx->stream, channel, lang);
}

gint
gtk_xine_get_spu_lang (GtkXine * gtx, gint channel, gchar * lang)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_spu_lang (gtx->stream, channel, lang);
}

gint
gtk_xine_get_stream_info (GtkXine * gtx, gint info)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_stream_info (gtx->stream, info);
}

const gchar *
gtk_xine_get_meta_info (GtkXine * gtx, gint info)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_meta_info (gtx->stream, info);
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
    static char *window_title = "gtkxine fullscreen window";
    XSizeHints hint;
    Atom    prop;
    MWMHints mwmhints;
    XEvent  xev;

    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->stream != NULL);

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

	gtx->fullscreen_window = XCreateSimpleWindow (gtx->display,
						      RootWindow (gtx->
								  display,
								  DefaultScreen
								  (gtx->
								   display)),
						      0, 0,
						      gtx->fullscreen_width,
						      gtx->fullscreen_height,
						      1,
						      BlackPixel (gtx->
								  display,
								  DefaultScreen
								  (gtx->
								   display)),
						      BlackPixel (gtx->
								  display,
								  DefaultScreen
								  (gtx->
								   display)));

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

	xine_gui_send_vo_data (gtx->stream,
			       XINE_GUI_SEND_DRAWABLE_CHANGED,
			       (void *) gtx->fullscreen_window);

	/*
	 * switch off mouse cursor
	 */

	XDefineCursor (gtx->display, gtx->fullscreen_window, gtx->no_cursor);
	XFlush (gtx->display);
    }
    else
    {
	xine_gui_send_vo_data (gtx->stream,
			       XINE_GUI_SEND_DRAWABLE_CHANGED,
			       (void *) gtx->video_window);

	XDestroyWindow (gtx->display, gtx->fullscreen_window);
    }

    XUnlockDisplay (gtx->display);
}

gint
gtk_xine_is_fullscreen (GtkXine * gtx)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return gtx->fullscreen_mode;
}

gint
gtk_xine_get_current_frame (GtkXine * gtx,
			    gint * width,
			    gint * height,
			    gint * ratio_code, gint * format, uint8_t * img)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->stream != NULL, 0);

    return xine_get_current_frame (gtx->stream, width, height, ratio_code,
				   format, img);
}

gint
gtk_xine_get_log_section_count (GtkXine * gtx)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine != NULL, 0);

    return xine_get_log_section_count (gtx->xine);
}

gchar **
gtk_xine_get_log_names (GtkXine * gtx)
{
    g_return_val_if_fail (gtx != NULL, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine != NULL, NULL);

    return (gchar **) xine_get_log_names (gtx->xine);
}

gchar **
gtk_xine_get_log (GtkXine * gtx, gint buf)
{
    g_return_val_if_fail (gtx != NULL, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine != NULL, NULL);

    return (gchar **) xine_get_log (gtx->xine, buf);
}

void
gtk_xine_register_log_cb (GtkXine * gtx, xine_log_cb_t cb, void *user_data)
{
    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine != NULL);

    return xine_register_log_cb (gtx->xine, cb, user_data);
}

gchar **
gtk_xine_get_browsable_input_plugin_ids (GtkXine * gtx)
{
    g_return_val_if_fail (gtx != NULL, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine != NULL, NULL);

    return (gchar **) xine_get_browsable_input_plugin_ids (gtx->xine);
}

xine_mrl_t **
gtk_xine_get_browse_mrls (GtkXine * gtx,
			  const gchar * plugin_id,
			  const gchar * start_mrl, gint * num_mrls)
{
    g_return_val_if_fail (gtx != NULL, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine != NULL, NULL);

    return (xine_mrl_t **) xine_get_browse_mrls (gtx->xine, plugin_id,
						 start_mrl, num_mrls);
}


gchar **
gtk_xine_get_autoplay_input_plugin_ids (GtkXine * gtx)
{
    g_return_val_if_fail (gtx != NULL, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine != NULL, NULL);

    return (gchar **) xine_get_autoplay_input_plugin_ids (gtx->xine);
}

gchar **
gtk_xine_get_autoplay_mrls (GtkXine * gtx,
			    const gchar * plugin_id, gint * num_mrls)
{
    g_return_val_if_fail (gtx != NULL, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine != NULL, NULL);

    return (gchar **) xine_get_autoplay_mrls (gtx->xine, plugin_id, num_mrls);
}

int
gtk_xine_config_get_first_entry (GtkXine * gtx, xine_cfg_entry_t * entry)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine != NULL, 0);

    return xine_config_get_first_entry (gtx->xine, entry);
}

int
gtk_xine_config_get_next_entry (GtkXine * gtx, xine_cfg_entry_t * entry)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine != NULL, 0);

    return xine_config_get_next_entry (gtx->xine, entry);
}

int
gtk_xine_config_lookup_entry (GtkXine * gtx,
			      const gchar * key, xine_cfg_entry_t * entry)
{
    g_return_val_if_fail (gtx != NULL, 0);
    g_return_val_if_fail (GTK_IS_XINE (gtx), 0);
    g_return_val_if_fail (gtx->xine != NULL, 0);

    return xine_config_lookup_entry (gtx->xine, key, entry);
}

void
gtk_xine_config_update_entry (GtkXine * gtx, xine_cfg_entry_t * entry)
{
    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine != NULL);

    xine_config_update_entry (gtx->xine, entry);
}

void
gtk_xine_config_load (GtkXine * gtx, const gchar * cfg_filename)
{
    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine != NULL);

    xine_config_load (gtx->xine, cfg_filename);
}

void
gtk_xine_config_save (GtkXine * gtx, const gchar * cfg_filename)
{
    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine != NULL);

    xine_config_save (gtx->xine, cfg_filename);
}

void
gtk_xine_config_reset (GtkXine * gtx)
{
    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->xine != NULL);

    xine_config_reset (gtx->xine);
}

void
gtk_xine_event_send (GtkXine * gtx, const xine_event_t * event)
{
    g_return_if_fail (gtx != NULL);
    g_return_if_fail (GTK_IS_XINE (gtx));
    g_return_if_fail (gtx->stream != NULL);

    xine_event_send (gtx->stream, event);
}

void
gtk_xine_set_speed (GtkXine * gtx, gint speed)
{
    gtk_xine_set_param (gtx, XINE_PARAM_SPEED, speed);
}

gint
gtk_xine_get_speed (GtkXine * gtx)
{
    return gtk_xine_get_param (gtx, XINE_PARAM_SPEED);
}

gint
gtk_xine_get_position (GtkXine * gtx)
{
    gint    pos_stream, pos_time, length_time;

    gtk_xine_get_pos_length (gtx, &pos_stream, &pos_time, &length_time);

    return pos_stream;
}

gint
gtk_xine_get_current_time (GtkXine * gtx)
{
    gint    pos_stream, pos_time, length_time;

    gtk_xine_get_pos_length (gtx, &pos_stream, &pos_time, &length_time);

    return pos_time;
}

gint
gtk_xine_get_stream_length (GtkXine * gtx)
{
    gint    pos_stream, pos_time, length_time;

    gtk_xine_get_pos_length (gtx, &pos_stream, &pos_time, &length_time);

    return length_time;
}

/*
 *  For screen shot.
 */

#define PIXSZ 3
#define BIT_DEPTH 8

/* internal function use to scale yuv data */
typedef void (*scale_line_func_t) (uint8_t * source, uint8_t * dest,
				   int width, int step);

/* Holdall structure */

struct prvt_image_s
{
    int     width;
    int     height;
    int     ratio_code;
    int     format;
    uint8_t *y, *u, *v, *yuy2;
    uint8_t *img;

    int     u_width, v_width;
    int     u_height, v_height;

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
    gint    width, height;

    g_return_val_if_fail (gtx, NULL);
    g_return_val_if_fail (GTK_IS_XINE (gtx), NULL);
    g_return_val_if_fail (gtx->xine, NULL);
    g_return_val_if_fail (gtx->stream != NULL, 0);
    g_return_val_if_fail (width_ret && height_ret, NULL);

    image = g_new0 (struct prvt_image_s, 1);
    if (!image)
    {
	*width_ret = 0;
	*height_ret = 0;

	return NULL;
    }

    image->y = image->u = image->v = image->yuy2 = image->img = NULL;

    width = xine_get_stream_info (gtx->stream, XINE_STREAM_INFO_VIDEO_WIDTH);
    height =
	xine_get_stream_info (gtx->stream, XINE_STREAM_INFO_VIDEO_HEIGHT);

    image->img = g_malloc (width * height * 2);

    if (!image->img)
    {
	*width_ret = 0;
	*height_ret = 0;

	g_free (image);
	return NULL;
    }

    err = xine_get_current_frame (gtx->stream,
				  &image->width, &image->height,
				  &image->ratio_code,
				  &image->format, image->img);

    if (err == 0)
    {
	*width_ret = 0;
	*height_ret = 0;

	g_free (image->img);
	g_free (image);

	return NULL;
    }

    /*
     * the dxr3 driver does not allocate yuv buffers 
     */
    /*
     * image->u and image->v are always 0 for YUY2 
     */
    if (!image->img)
    {
	*width_ret = 0;
	*height_ret = 0;

	g_free (image->img);
	g_free (image);

	return NULL;
    }

    rgb = xine_frame_to_rgb (image);
    *width_ret = image->width;
    *height_ret = image->height;

    free (image->img);
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
 *  Copyright (C) Thomas ��streich - June 2001
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
/* FIXME !!!!!!!!! */
    image->y = g_malloc0 (image->height * image->width * 2);
    if (!image->y)
	goto ERROR0;

    image->u = g_malloc0 (image->u_height * image->u_width * 2);
    if (!image->u)
	goto ERROR1;

    image->v = g_malloc0 (image->v_height * image->v_width * 2);
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
      case XINE_VO_ASPECT_SQUARE:
	  image->scale_line = scale_line_1_1;
	  image->scale_factor = (32768 * 1) / 1;
	  break;

      case XINE_VO_ASPECT_4_3:
	  image->scale_line = scale_line_15_16;
	  image->scale_factor = (32768 * 16) / 15;
	  break;

      case XINE_VO_ASPECT_ANAMORPHIC:
	  image->scale_line = scale_line_45_64;
	  image->scale_factor = (32768 * 64) / 45;
	  break;

      case XINE_VO_ASPECT_DVB:
	  image->scale_line = scale_line_45_64;
	  image->scale_factor = (32768 * 64) / 45;
	  break;

      case XINE_VO_ASPECT_DONT_TOUCH:
	  image->scale_line = scale_line_1_1;
	  image->scale_factor = (32768 * 1) / 1;
	  break;

      default:
	  /*
	   * the mpeg standard has a few that we don't know about 
	   */
	  image->scale_line = scale_line_1_1;
	  image->scale_factor = (32768 * 1) / 1;
	  break;
    }

    switch (image->format)
    {
      case XINE_IMGFMT_YV12:
	  image->y = image->img;
	  image->u = image->img + (image->width * image->height);
	  image->v =
	      image->img + (image->width * image->height) +
	      (image->width * image->height) / 4;
	  image->u_width = ((image->width + 1) / 2);
	  image->v_width = ((image->width + 1) / 2);
	  image->u_height = ((image->height + 1) / 2);
	  image->v_height = ((image->height + 1) / 2);
	  break;

      case XINE_IMGFMT_YUY2:
	  image->yuy2 = image->img;
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

#endif /* ENABLE_XINE */
