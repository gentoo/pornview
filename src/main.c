/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                        (c) 2002,2003                         (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#include "pornview.h"

#include "browser.h"
#include "dock.h"
#include "plugin.h"
#include "prefs.h"

int
main (int argc, char *argv[])
{

#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);

#ifdef USE_GTK2
    bind_textdomain_codeset (PACKAGE, "UTF-8");
#endif

    textdomain (PACKAGE);
    gtk_set_locale ();
#endif

    gtk_init (&argc, &argv);

    prefs_load_config ();
    plugin_init ();

    browser_create ();

    dock_init ();
    gtk_main ();

    plugin_free ();
    prefs_save_config ();

    return 0;
}
