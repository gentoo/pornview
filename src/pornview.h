/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __PORNVIEW_H__
#define __PORNVIEW_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <utime.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <png.h>
#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include <gdk/gdkkeysyms.h>

#include <X11/Xatom.h>

#define G_WINDOW(widget) widget->window

#define BUF_SIZE      4096
#define MAX_PATH_LEN  1024

#include "intl.h"

#endif /* __PORNVIEW_H__ */
