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
	gchar *init_path = NULL;
	const gchar *helpmsg = "usage: pornview [dir]\0";

	if (argc > 2 ) {
		printf("%s\n", helpmsg);
		return 0;
	} else if (argc == 2) {
		if (!strcmp(argv[1], "--help") ||
				!strcmp(argv[1], "-h")) {
			printf("%s\n", helpmsg);
			return 0;
		}
		init_path = argv[1];
	}

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

    browser_create (init_path);

    dock_init ();
    gtk_main ();

    plugin_free ();
    prefs_save_config ();

    return 0;
}
