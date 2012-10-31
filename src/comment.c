/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are mostly taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#pragma GCC optimize ("O0")

#include "pornview.h"

#include "gtk2-compat.h"

#include "charset.h"
#include "file_utils.h"
#include "file_type.h"
#include "generic_dialog.h"
#include "string_utils.h"

#include "comment.h"
#include "cache.h"
#include "prefs.h"

typedef enum
{
    FILE_SAVED,
    FILE_DELETED,
    LAST_SIGNAL
}
CommentSignalType;

static void comment_init (Comment * comment);
static void comment_class_init (CommentClass * klass);
#ifdef USE_GTK2
static void comment_finalize (GObject * object);
#else
static void comment_finalize (GtkObject * object);
#endif

static gchar *defval_time (ImageInfo * info, gpointer data);
static gchar *defval_file_url (ImageInfo * info, gpointer data);
static gchar *defval_file_mtime (ImageInfo * info, gpointer data);
static gchar *defval_img_type (ImageInfo * info, gpointer data);
static gchar *defval_img_size (ImageInfo * info, gpointer data);

static GtkObjectClass *parent_class = NULL;
static gint comment_signals[LAST_SIGNAL] = { 0 };

CommentDataEntry comment_data_entry[] = {
    {"X-IMG-Title", N_("Title"), NULL, TRUE, FALSE, TRUE, FALSE, NULL},
    {"X-IMG-Date", N_("Date"), NULL, TRUE, FALSE, TRUE, FALSE, NULL},
    {"X-IMG-Category", N_("Category"), NULL, TRUE, FALSE, TRUE, FALSE, NULL},
    {"X-IMG-Model", N_("Model"), NULL, TRUE, FALSE, TRUE, FALSE, NULL},
    {"X-IMG-Comment-Time", N_("Comment Time"), NULL, TRUE, TRUE, FALSE, FALSE,
     defval_time},
    {"X-IMG-File-URL", N_("URL"), NULL, TRUE, TRUE, TRUE, FALSE,
     defval_file_url},
    {"X-IMG-File-MTime", N_("Modification Time"), NULL, TRUE, TRUE, TRUE,
     FALSE, defval_file_mtime},
    {"X-IMG-Image-Type", N_("Image Type"), NULL, TRUE, TRUE, TRUE, FALSE,
     defval_img_type},
    {"X-IMG-Image-Size", N_("Image Size"), NULL, TRUE, TRUE, TRUE, FALSE,
     defval_img_size},
    {NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, NULL},
};

GList  *comment_data_entry_list = NULL;

/*
 *-------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------
 */

static const gchar *
get_file_charset (void)
{
    const gchar *lang;


    if (conf.comment_charset && *conf.comment_charset
	&& strcasecmp (conf.comment_charset, "default"))
    {
	return conf.comment_charset;
    }

    lang = get_lang ();

    if (!lang || !*lang)
	return charset_get_internal ();

    /*
     * get default charset for each language 
     */
    if (!strncmp (lang, "ja", 2))
    {				/* japanese */
	return CHARSET_JIS;
    }
    else
    {
	return charset_get_internal ();
    }
}

static gchar *
defval_time (ImageInfo * info, gpointer data)
{
    time_t  current_time;
    struct tm tm_buf;
    gchar   buf[256];

    time (&current_time);
    tm_buf = *localtime (&current_time);
    strftime (buf, 256, "%Y-%m-%d %H:%M:%S %Z", &tm_buf);

    return charset_locale_to_internal (buf);
}

static gchar *
defval_file_url (ImageInfo * info, gpointer data)
{
    const gchar *filename;

    g_return_val_if_fail (info, NULL);

    filename = info->filename;

    if (filename)
    {
	return charset_to_internal (filename, conf.charset_filename,
				    conf.charset_auto_detect_fn,
				    conf.comment_charset_read_mode);
    }
    else
    {
	return NULL;
    }
}

static gchar *
defval_file_mtime (ImageInfo * info, gpointer data)
{
    struct tm tm_buf;
    gchar   buf[256];

    g_return_val_if_fail (info, NULL);

    tm_buf = *localtime (&info->st.st_mtime);
    strftime (buf, 256, "%Y-%m-%d %H:%M:%S %Z", &tm_buf);

    return charset_locale_to_internal (buf);
}

static gchar *
defval_img_type (ImageInfo * info, gpointer data)
{
    gchar  *type = NULL;

    g_return_val_if_fail (info, NULL);

    type = file_type_detect_type_by_ext (info->filename);

    if (type)
	return g_strdup (type);
    else
	return NULL;
}

static gchar *
defval_img_size (ImageInfo * info, gpointer data)
{
    gchar   buf[256];

    g_return_val_if_fail (info, NULL);

    g_snprintf (buf, 256, "%dx%d", info->width, info->height);

    return g_strdup (buf);
}
static Comment *
comment_new ()
{
    Comment *comment;

    comment = COMMENT (gtk_type_new (comment_get_type ()));
    g_return_val_if_fail (comment, NULL);

    return comment;
}

static void
parse_comment_file (Comment * comment)
{
    FILE   *file;
    gchar   buf[BUF_SIZE];
    gchar **pair, *tmpstr;
    gint    i;
    gboolean is_note = FALSE;
    gchar  *key_internal, *value_internal = NULL;

    g_return_if_fail (comment);
    g_return_if_fail (comment->filename);

    if (!file_exists (comment->filename))
	return;

    file = fopen (comment->filename, "r");
    if (!file)
    {
	g_warning (_("Can't open comment file for read."));
	return;
    }

    while (fgets (buf, sizeof (buf), file))
    {
	gchar  *key, *value = NULL;

	if (is_note)
	{
	    if (comment->note)
	    {
		tmpstr = g_strconcat (comment->note, buf, NULL);
		g_free (comment->note);
		comment->note = tmpstr;
	    }
	    else
	    {
		comment->note = g_strdup (buf);
	    }
	    continue;
	}

	if (buf[0] == '\n')
	{
	    is_note = TRUE;
	    continue;
	}

	pair = g_strsplit (buf, ":", -1);
	if (!pair[0])
	    continue;
	if (!*pair[0])
	    goto ERROR;
	key = g_strdup (pair[0]);
	g_strstrip (key);

	if (pair[1])
	{
	    value = g_strdup (pair[1]);
	    for (i = 2; pair[i]; i++)
	    {			/* concat all value to one string */
		gchar  *tmpstr = value;
		value = g_strconcat (value, ":", pair[i], NULL);
		g_free (tmpstr);
	    }
	    g_strstrip (value);
	    if (!*value)
	    {
		g_free (value);
		value = NULL;
	    }
	}
	else
	{
	    value = NULL;
	}

	key_internal = charset_to_internal (key, get_file_charset (),
					    conf.charset_auto_detect_fn,
					    conf.comment_charset_read_mode);
	if (value && *value)
	{
	    value_internal = charset_to_internal (value, get_file_charset (),
						  conf.charset_auto_detect_fn,
						  conf.
						  comment_charset_read_mode);
	}
	else
	{
	    value_internal = NULL;
	}

	comment_append_data (comment, key_internal, value_internal);

	g_free (key_internal);
	g_free (value_internal);

	g_free (key);
	g_free (value);

      ERROR:
	g_strfreev (pair);
    }

    fclose (file);

    if (comment->note && *comment->note)
    {
	gchar  *note_internal;

	note_internal =
	    charset_to_internal (comment->note, get_file_charset (),
				 conf.charset_auto_detect_fn,
				 conf.comment_charset_read_mode);

	g_free (comment->note);
	comment->note = note_internal;
    }
}

static void
comment_set_default_value (Comment * comment)
{
    GList  *list = comment_get_data_entry_list ();

    while (list)
    {
	CommentDataEntry *template = list->data, *entry;

	list = g_list_next (list);

	if (!template)
	    continue;

	entry = g_new0 (CommentDataEntry, 1);

	*entry = *template;
	entry->key = g_strdup (template->key);
	entry->display_name = g_strdup (template->display_name);

	entry->value = NULL;
	comment->data_list = g_list_append (comment->data_list, entry);
    }
}

/*
 *-------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------
 */

gchar  *
comment_get_path (const gchar * img_path)
{
    gchar   buf[MAX_PATH_LEN];

    g_return_val_if_fail (img_path && *img_path, NULL);
    g_return_val_if_fail (img_path[0] == '/', NULL);

    g_snprintf (buf, MAX_PATH_LEN, "%s/%s%s" ".txt",
		g_getenv ("HOME"), PORNVIEW_RC_DIR_COMMENTS, img_path);

    return g_strdup (buf);
}

gchar  *
comment_find_file (const gchar * img_path)
{
    gchar  *path = comment_get_path (img_path);

    if (!path)
	return NULL;

    if (file_exists (path))
    {
	return path;
    }
    else
    {
	g_free (path);
	return NULL;
    }
}

CommentDataEntry *
comment_data_entry_find_template_by_key (const gchar * key)
{
    GList  *list;

    g_return_val_if_fail (key && *key, NULL);

    list = comment_get_data_entry_list ();
    while (list)
    {
	CommentDataEntry *template = list->data;

	list = g_list_next (list);

	if (!template)
	    continue;

	if (template->key && *template->key && !strcmp (key, template->key))
	    return template;
    }

    return NULL;
}

CommentDataEntry *
comment_find_data_entry_by_key (Comment * comment, const gchar * key)
{
    GList  *list;

    g_return_val_if_fail (comment, NULL);
    g_return_val_if_fail (key, NULL);

    list = comment->data_list;
    while (list)
    {
	CommentDataEntry *entry = list->data;

	list = g_list_next (list);

	if (!entry)
	    continue;

	if (entry->key && !strcmp (key, entry->key))
	    return entry;
    }

    return NULL;
}

/*
 *  comment_append_data:
 *     @ Append a key & value pair.
 *     @ Before enter this function, character set must be converted to internal
 *       code.
 *
 *  comment :
 *  key     :
 *  value   :
 *  Return  :
 */
CommentDataEntry *
comment_append_data (Comment * comment, const gchar * key,
		     const gchar * value)
{
    CommentDataEntry *entry;

    g_return_val_if_fail (comment, NULL);
    g_return_val_if_fail (key, NULL);

    entry = comment_find_data_entry_by_key (comment, key);

    if (!entry)
    {
	CommentDataEntry *template;

	entry = g_new0 (CommentDataEntry, 1);
	g_return_val_if_fail (entry, NULL);
	comment->data_list = g_list_append (comment->data_list, entry);

	/*
	 * find template 
	 */
	template = comment_data_entry_find_template_by_key (key);
	if (template)
	{
	    *entry = *template;
	    entry->key = g_strdup (template->key);
	    entry->display_name = g_strdup (template->display_name);
	}
	else
	{
	    entry->key = g_strdup (key);
	    entry->display_name = g_strdup (key);
	    entry->value = NULL;
	    entry->enable = TRUE;
	    entry->auto_val = FALSE;
	    entry->display = TRUE;
	    entry->def_val_fn = NULL;
	}
    }

    /*
     * free old value 
     */
    if (entry->value)
    {
	g_free (entry->value);
	entry->value = NULL;
    }

    /*
     * set new value 
     */
    if (entry->auto_val && entry->def_val_fn)
    {
	entry->value = entry->def_val_fn (comment->info, NULL);
    }
    else if (value)
    {
	entry->value = g_strdup (value);
    }
    else
    {
	entry->value = NULL;
    }

    return entry;
}

/*
 *  comment_update_note:
 *     @ Apply "note" value.
 *     @ Before enter this function, character set must be converted to internal
 *       code.
 *
 *  comment :
 *  note    :
 *  Return  :
 */
gboolean
comment_update_note (Comment * comment, gchar * note)
{
    g_return_val_if_fail (comment, FALSE);

    if (comment->note)
	g_free (comment->note);

    comment->note = g_strdup (note);

    return TRUE;
}

void
comment_data_entry_remove (Comment * comment, CommentDataEntry * entry)
{
    GList  *list;

    g_return_if_fail (comment);
    g_return_if_fail (entry);

    list = g_list_find (comment->data_list, entry);
    if (list)
    {
	comment->data_list = g_list_remove (comment->data_list, entry);
	comment_data_entry_delete (entry);
    }
}

void
comment_data_entry_remove_by_key (Comment * comment, const gchar * key)
{
    GList  *list;

    g_return_if_fail (comment);
    g_return_if_fail (key && *key);

    list = comment->data_list;
    while (list)
    {
	CommentDataEntry *entry = list->data;

	list = g_list_next (list);

	if (!entry)
	    continue;

	if (entry->key && !strcmp (key, entry->key))
	{
	    comment->data_list = g_list_remove (comment->data_list, entry);
	    comment_data_entry_delete (entry);
	}
    }
}

CommentDataEntry *
comment_data_entry_dup (CommentDataEntry * src)
{
    CommentDataEntry *dest;
    g_return_val_if_fail (src, NULL);

    dest = g_new0 (CommentDataEntry, 1);
    *dest = *src;
    if (src->key)
	dest->key = g_strdup (src->key);
    if (src->display_name)
	dest->display_name = g_strdup (src->display_name);

    return dest;
}

void
comment_data_entry_delete (CommentDataEntry * entry)
{
    g_return_if_fail (entry);

    g_free (entry->key);
    entry->key = NULL;
    g_free (entry->display_name);
    entry->display_name = NULL;
    if (entry->value)
	g_free (entry->value);
    entry->value = NULL;
    g_free (entry);
}

gboolean
comment_save_file (Comment * comment)
{
    GList  *node;
    FILE   *file;
    gboolean success;
    gint    n;

    g_return_val_if_fail (comment, FALSE);
    g_return_val_if_fail (comment->filename, FALSE);

    success = mkdirs (comment->filename);
    if (!success)
    {
	g_warning (_("cannot make dir\n"));
	return FALSE;
    }

    file = fopen (comment->filename, "w");
    if (!file)
    {
	g_warning (_("Can't open comment file for write."));
	return FALSE;
    }

    node = comment->data_list;
    while (node)
    {
	CommentDataEntry *entry = node->data;

	node = g_list_next (node);

	if (!entry || !entry->key)
	    continue;

	{ /********** convert charset **********/
	    gchar  *tmpstr, buf[BUF_SIZE];

	    if (entry->value && *entry->value)
		g_snprintf (buf, BUF_SIZE, "%s: %s\n", entry->key,
			    entry->value);
	    else
		g_snprintf (buf, BUF_SIZE, "%s:\n", entry->key);

	    tmpstr = charset_from_internal (buf, get_file_charset ());

	    n = fprintf (file, "%s", tmpstr);

	    g_free (tmpstr);
	}

	if (n < 0)
	    goto ERROR;
    }

   /********** convert charset **********/
    if (comment->note && *comment->note)
    {
	gchar  *tmpstr;

	tmpstr = charset_from_internal (comment->note, get_file_charset ());

	n = fprintf (file, "\n%s", tmpstr);
	if (n < 0)
	    goto ERROR;

	g_free (tmpstr);
    }

    fclose (file);

    gtk_signal_emit (GTK_OBJECT (comment), comment_signals[FILE_SAVED]);

    return TRUE;

  ERROR:
    fclose (file);

    gtk_signal_emit (GTK_OBJECT (comment), comment_signals[FILE_SAVED]);
    return FALSE;
}

void
comment_delete_file (Comment * comment)
{
    g_return_if_fail (comment);
    g_return_if_fail (comment->filename);

    remove (comment->filename);
}

Comment *
comment_get_from_image_info (ImageInfo * info)
{
    Comment *comment;

    g_return_val_if_fail (info, NULL);

    comment = comment_new ();
    comment->info = image_info_ref (info);

    /*
     * get comment file path 
     */
    comment->filename = comment_get_path (info->filename);

    /*
     * get comment 
     */
    if (file_exists (comment->filename))
    {
	parse_comment_file (comment);
    }
    else
    {
	comment_set_default_value (comment);
    }

    return comment;
}

/*
 *-------------------------------------------------------------------
 * key list related functions
 *-------------------------------------------------------------------
 */
static void
comment_prefs_set_default_key_list (void)
{
    gchar  *string, *tmp;
    gint    i;

    if (conf.comment_key_list)
	return;

    string = g_strdup ("");

    for (i = 0; comment_data_entry[i].key; i++)
    {
	CommentDataEntry *entry = &comment_data_entry[i];

	tmp = string;
	if (*string)
	{
	    string = g_strconcat (string, "; ", NULL);
	    g_free (tmp);
	    tmp = string;
	}
	string = g_strconcat (string, entry->key, ",", entry->display_name,
			      ",", boolean_to_text (entry->enable),
			      ",", boolean_to_text (entry->auto_val),
			      ",", boolean_to_text (entry->display), NULL);
	g_free (tmp);
	tmp = NULL;
    }

    conf.comment_key_list = string;
}

static GList *
comment_get_system_data_entry_list (void)
{
    GList  *list = NULL;
    gint    i;

    for (i = 0; comment_data_entry[i].key; i++)
    {
	CommentDataEntry *entry = g_new0 (CommentDataEntry, 1);

	list = g_list_append (list, entry);

	*entry = comment_data_entry[i];
	entry->key = g_strdup (comment_data_entry[i].key);
	entry->display_name = g_strdup (comment_data_entry[i].display_name);
	if (entry->value && *entry->value)
	    entry->value = g_strdup (entry->value);
    }

    return list;
}

static void
free_data_entry_list (void)
{
    if (!comment_data_entry_list)
	return;

    g_list_foreach (comment_data_entry_list,
		    (GFunc) comment_data_entry_delete, NULL);
    g_list_free (comment_data_entry_list);
    comment_data_entry_list = NULL;
}

static CommentDataEntry *
comment_data_entry_new (gchar * key, gchar * name,
			gboolean enable, gboolean auto_val, gboolean display)
{
    CommentDataEntry *entry = NULL;
    gint    i, pos = -1;

    g_return_val_if_fail (key && *key, NULL);
    g_return_val_if_fail (name && *name, NULL);

    for (i = 0; comment_data_entry[i].key; i++)
    {
	if (!strcmp (key, comment_data_entry[i].key))
	{
	    pos = i;
	    break;
	}
    }

    entry = g_new0 (CommentDataEntry, 1);
    if (!entry)
	return NULL;

    if (pos < 0)
    {
	entry->key = g_strdup (key);
	entry->display_name = g_strdup (name);
	entry->value = NULL;
	entry->def_val_fn = NULL;
	entry->userdef = TRUE;
    }
    else
    {
	*entry = comment_data_entry[pos];
	entry->key = g_strdup (comment_data_entry[pos].key);
	entry->display_name = g_strdup (comment_data_entry[pos].display_name);
	entry->userdef = FALSE;
    }

    entry->enable = enable;
    if (entry->def_val_fn)
	entry->auto_val = auto_val;
    else
	entry->auto_val = FALSE;
    entry->display = display;

    return entry;
}

static  gint
compare_entry_by_key (gconstpointer a, gconstpointer b)
{
    const CommentDataEntry *entry = a;
    const gchar *string = b;

    g_return_val_if_fail (entry, TRUE);
    g_return_val_if_fail (string, TRUE);

    if (!entry->key || !*entry->key)
	return TRUE;
    if (!string || !*string)
	return TRUE;

    if (!strcmp (entry->key, string))
	return FALSE;

    return TRUE;
}

void
comment_update_data_entry_list (void)
{
    GList  *node = NULL, *list;
    gchar **sections, **values;
    gint    i, j;

    if (!conf.comment_key_list)
	comment_prefs_set_default_key_list ();

    if (comment_data_entry_list)
	free_data_entry_list ();

    sections = g_strsplit (conf.comment_key_list, ";", -1);
    if (!sections)
	return;

    /*
     * for undefined system entry in prefs 
     */
    list = comment_get_system_data_entry_list ();

    for (i = 0; sections[i]; i++)
    {
	CommentDataEntry *entry;
	gchar  *key, *name;

	if (!*sections[i])
	    continue;

	values = g_strsplit (sections[i], ",", 5);
	if (!values)
	    continue;

	/*
	 * strip space characters 
	 */
	for (j = 0; j < 5; j++)
	{
	    if (values[j] && *values[j])
		g_strstrip (values[j]);
	}
	if (!*values[0] || !*values[1])
	    goto ERROR1;

	key = values[0];
	name = charset_locale_to_internal (values[1]);

	/*
	 * check duplicate 
	 */
	node = NULL;
	node = g_list_find_custom (comment_data_entry_list,
				   key, compare_entry_by_key);
	if (node)
	{
	    comment_data_entry_delete (node->data);
	    list = g_list_remove (list, node->data);
	}

	/*
	 * add entry 
	 */
	entry = comment_data_entry_new (key, name,
					text_to_boolean (values[2]),
					text_to_boolean (values[3]),
					text_to_boolean (values[4]));
	g_free (name);

	if (!entry)
	    goto ERROR1;

	comment_data_entry_list
	    = g_list_append (comment_data_entry_list, entry);

	/*
	 * for undefined system entry in prefs 
	 */
	if (!entry->userdef)
	{
	    node = NULL;
	    if (list)
		node =
		    g_list_find_custom (list, entry->key,
					compare_entry_by_key);
	    if (node)
	    {			/* this entry is already defined in prefs */
		comment_data_entry_delete (node->data);
		list = g_list_remove (list, node->data);
	    }
	}

      ERROR1:
	g_strfreev (values);
	values = NULL;
    }

    /*
     * append system defined entry if it isn't exist in prefs 
     */
    for (node = list; node; node = g_list_next (node))
    {
	if (node && node->data)
	    comment_data_entry_list
		= g_list_append (comment_data_entry_list, node->data);
    }

    if (list)
	g_list_free (list);
    g_strfreev (sections);
}

GList  *
comment_get_data_entry_list (void)
{
    if (!comment_data_entry_list)
	comment_update_data_entry_list ();

    return comment_data_entry_list;
}

/*
 *-------------------------------------------------------------------
 * comment class funcs
 *-------------------------------------------------------------------
 */

guint
comment_get_type (void)
{
    static guint comment_type = 0;

    if (!comment_type)
    {
	static const GtkTypeInfo comment_info = {
	    "Comment",
	    sizeof (Comment),
	    sizeof (CommentClass),
	    (GtkClassInitFunc) comment_class_init,
	    (GtkObjectInitFunc) comment_init,
	    NULL,
	    NULL,
	    (GtkClassInitFunc) NULL,
	};

	comment_type =
	    gtk_type_unique (gtk_object_get_type (), &comment_info);
    }

    return comment_type;
}

static void
comment_class_init (CommentClass * klass)
{
    GtkObjectClass *object_class;

    object_class = (GtkObjectClass *) klass;
    parent_class = gtk_type_class (gtk_object_get_type ());

    comment_signals[FILE_SAVED]
	= gtk_signal_new ("file_saved",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (CommentClass, file_saved),
			  gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    comment_signals[FILE_DELETED]
	= gtk_signal_new ("file_deleted",
			  GTK_RUN_FIRST,
			  GTK_CLASS_TYPE (object_class),
			  GTK_SIGNAL_OFFSET (CommentClass, file_deleted),
			  gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    gtk_object_class_add_signals (object_class, comment_signals, LAST_SIGNAL);

    OBJECT_CLASS_SET_FINALIZE_FUNC (klass, comment_finalize);

    klass->file_saved = NULL;
    klass->file_deleted = NULL;
}

static void
comment_init (Comment * comment)
{
    comment->filename = NULL;
    comment->info = NULL;

    comment->data_list = NULL;

    comment->note = NULL;

#ifdef USE_GTK2
    gtk_object_ref (GTK_OBJECT (comment));
    gtk_object_sink (GTK_OBJECT (comment));
#endif
}

static void
#ifdef USE_GTK2
comment_finalize (GObject * object)
#else
comment_finalize (GtkObject * object)
#endif
{
    Comment *comment = COMMENT (object);
    GList  *node;

    g_return_if_fail (comment);
    g_return_if_fail (IS_COMMENT (comment));

    g_free (comment->filename);
    comment->filename = NULL;
    if (comment->info)
    {
	image_info_unref (comment->info);
    }

    node = comment->data_list;
    while (node)
    {
	CommentDataEntry *entry = node->data;
	node = g_list_next (node);
	comment->data_list = g_list_remove (comment->data_list, entry);
	comment_data_entry_delete (entry);
    }

    g_free (comment->note);
    comment->note = NULL;

    OBJECT_CLASS_FINALIZE_SUPER (parent_class, object);
}

Comment *
comment_ref (Comment * comment)
{
    g_return_val_if_fail (comment, NULL);
    g_return_val_if_fail (IS_COMMENT (comment), NULL);

    gtk_object_ref (GTK_OBJECT (comment));

    return comment;
}

void
comment_unref (Comment * comment)
{
    g_return_if_fail (comment);
    g_return_if_fail (IS_COMMENT (comment));

    gtk_object_unref (GTK_OBJECT (comment));
}
