/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

/*
 * These codes are taken from GImageView.
 * GImageView author: Takuro Ashie
 */

#ifndef __EDITABLE_LIST_H__
#define __EDITABLE_LIST_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include "gtk2-compat.h"

#define EDITABLE_LIST(obj)         GTK_CHECK_CAST (obj, editable_list_get_type (), EditableList)
#define EDITABLE_LIST_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, editable_list_get_type, EditableListClass)
#define IS_EDITABLE_LIST(obj)      GTK_CHECK_TYPE (obj, editable_list_get_type ())

typedef struct EditableList_Tag EditableList;
typedef struct EditableListClass_Tag EditableListClass;

typedef struct EditableListColumnFuncTable_Tag EditableListColumnFuncTable;


typedef enum
{
    EDITABLE_LIST_EDIT_AREA_VALUE_INITIALIZED = 1 << 0,
    EDITABLE_LIST_EDIT_AREA_VALUE_CHANGED = 1 << 1,
}
EditableListEditAreaFlags;


typedef enum
{
    EDITABLE_LIST_CONFIRM_CANNOT_NEW = 1 << 0,
    EDITABLE_LIST_CONFIRM_CANNOT_ADD = 1 << 1,
    EDITABLE_LIST_CONFIRM_CANNOT_CHANGE = 1 << 2,
    EDITABLE_LIST_CONFIRM_CANNOT_DELETE = 1 << 3
}
EditableListConfirmFlags;


typedef enum
{
    EDITABLE_LIST_ACTION_RESET,
    EDITABLE_LIST_ACTION_ADD,
    EDITABLE_LIST_ACTION_CHANGE,
    EDITABLE_LIST_ACTION_DELETE,
    EDITABLE_LIST_ACTION_SET_SENSITIVE,
}
EditableListActionType;


typedef void    (*EditableListSetDataFn) (EditableList * editlist, GtkWidget * widget, gint row,	/* -1 if no item is selected */
					  gint col, const gchar * text,	/* NULL if no item is selected */
					  gpointer coldata);
typedef gchar  *(*EditableListGetDataFn) (EditableList * editlist,
					  EditableListActionType type,
					  GtkWidget * widget,
					  gpointer coldata);
typedef void    (*EditableListResetFn) (EditableList * editlist,
					GtkWidget * widget, gpointer coldata);
typedef         gboolean (*EditableListGetRowDataFn) (EditableList * editlist,
						      EditableListActionType
						      type,
						      gpointer * rowdata,
						      GtkDestroyNotify *
						      destroy_fn);

struct EditableList_Tag
{
    GtkVBox         parent;

    /*
     * public (read only)
     */
    GtkWidget      *clist;

    GtkWidget      *new_button;
    GtkWidget      *add_button;
    GtkWidget      *change_button;
    GtkWidget      *del_button;
    GtkWidget      *up_button;
    GtkWidget      *down_button;

    GtkWidget      *edit_area;
    GtkWidget      *move_button_area;
    GtkWidget      *action_button_area;

    /*
     * private
     */
    gint            max_row;
    gint            columns;
    gint            rows;
    gint            selected;
    gint            dest_row;

    EditableListEditAreaFlags edit_area_flags;
    EditableListColumnFuncTable **column_func_tables;
    EditableListGetRowDataFn get_rowdata_fn;

#if (GTK_MAJOR_VERSION >= 2)
    GHashTable     *rowdata_table;
    GHashTable     *rowdata_destroy_fn_table;
#endif				/* (GTK_MAJOR_VERSION >= 2) */
};

struct EditableListClass_Tag
{
    GtkVBoxClass    parent_class;

    void            (*list_updated) (EditableList * editlist);
    void            (*edit_area_set_data) (EditableList * editlist);
    void            (*action_confirm) (EditableList * editlist,
				       EditableListActionType action_type,
				       gint selected_row,
				       EditableListConfirmFlags * flags);
};

struct EditableListColumnFuncTable_Tag
{
    GtkWidget      *widget;
    gpointer        coldata;
    GtkDestroyNotify destroy_fn;

    /*
     * will be called when an item is selected (or unselected)
     */
    EditableListSetDataFn set_data_fn;

    /*
     * will be called when an action button is pressed
     */
    EditableListGetDataFn get_data_fn;
    EditableListResetFn reset_fn;
};

guint           editable_list_get_type (void);
GtkWidget      *editable_list_new (gint colnum);
GtkWidget      *editable_list_new_with_titles (gint colnum, gchar * titles[]);
void            editable_list_set_column_title_visible
    (EditableList * editlist, gboolean visible);
void            editable_list_set_reorderable (EditableList * editlist,
					       gboolean reorderble);
gboolean        editable_list_get_reorderable (EditableList * editlist);
void            editable_list_set_auto_sort (EditableList * editlist,
					     gint column);
gint            editable_list_append_row (EditableList * editlist,
					  gchar * data[]);
void            editable_list_remove_row (EditableList * editlist, gint row);
gint            editable_list_get_n_rows (EditableList * editlist);
gint            editable_list_get_selected_row (EditableList * editlist);
gchar         **editable_list_get_row_text (EditableList * editlist,
					    gint row);
gchar          *editable_list_get_cell_text (EditableList * ditlist,
					     gint row, gint col);
#if 0
void            editable_list_set_row_text (EditableList * ditlist,
					    gint row, gchar * data[]);
void            editable_list_set_cell_text (EditableList * ditlist,
					     gint row,
					     gint col, gchar * data);
#endif
void            editable_list_set_row_data (EditableList * editlist,
					    gint row, gpointer data);
void            editable_list_set_row_data_full (EditableList * editlist,
						 gint row,
						 gpointer data,
						 GtkDestroyNotify destroy_fn);
gpointer        editable_list_get_row_data (EditableList * editlist,
					    gint row);
void            editable_list_unselect_all (EditableList * editlist);
void            editable_list_set_max_row (EditableList * editlist,
					   gint rownum);
void            editable_list_set_column_funcs (EditableList * editlist,
						GtkWidget * widget,
						gint column,
						EditableListSetDataFn
						set_data_fn,
						EditableListGetDataFn
						get_data_fn,
						EditableListResetFn reset_fn,
						gpointer coldata,
						GtkDestroyNotify destroy_fn);
void            editable_list_set_get_row_data_func (EditableList * editlist,
						     EditableListGetRowDataFn
						     get_rowdata_func);
void            editable_list_edit_area_set_value_changed (EditableList *
							   editlist);
EditableListConfirmFlags editable_list_action_confirm (EditableList *
						       editlist,
						       EditableListActionType
						       type);
void            editable_list_set_action_button_sensitive (EditableList *
							   editlist);


/* entry */
GtkWidget      *editable_list_create_entry (EditableList * editlist,
					    gint column,
					    const gchar * init_string,
					    gboolean allow_empty);

/* file name entry */
/*
GtkWidget    *editable_list_create_file_entry     (EditableList *editlist,
                                                   gint          column,
                                                   const gchar  *init_string,
                                                   gboolean      allow_empty)
*/

/* combo */
/*
GtkWidget    *editable_list_create_combo          (EditableList *editlist,
                                                   gint column);
*/

/* check button */
GtkWidget      *editable_list_create_check_button (EditableList * editlist,
						   gint column,
						   const gchar * label,
						   gboolean init_value,
						   const gchar * true_string,
						   const gchar *
						   false_string);

/* spinner */
/*
GtkWidget    *editable_list_create_spinner        (EditableList *editlist,
                                                   gint column,
                                                   gfloat init_val,
                                                   gflost min, gloat max,
                                                   gboolean set_as_integer);
*/

#endif /* __EDITABLE_LIST_H__ */
