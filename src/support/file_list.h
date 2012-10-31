/*----------------------------------------------------------------------------*
 *                                                                 .---.      *
 *                           PornView                             (_,/\ \     *
 *           photo/movie collection viewer and manager           (`a a(  )    *
 *                    trem0r <trem0r@tlen.pl>                    ) \=  ) (    *
 *                           (c) 2002                           (.--' '--.)   *
 *                                                              / (_)^(_) \   *
 *----------------------------------------------------------------------------*/

#ifndef __FILE_LIST_H__
#define __FILE_LIST_H__

typedef struct _FileList FileList;
typedef struct _FileListNode FileListNode;

struct _FileListNode
{
    gchar          *file_name;
    gint            file_type;
};

struct _FileList
{
    GList          *list;
    guint           num;
};

FileList       *file_list_init (void);
void            file_list_free (FileList * fl);
void            file_list_alloc (FileList * fl);
void            file_list_add (FileList * fl, gchar * name, gint type);
void            file_list_clear (FileList * fl);
void            file_list_create (FileList * fl, gchar * path);

#endif /* __FILE_LIST_H__ */
