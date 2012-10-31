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

#ifndef __FILE_TYPE_H__
#define __FILE_TYPE_H__

#define IMG_JPG "JPEG"
#define IMG_PNG "PNG"
#define IMG_UNKNOWN "UNKNOWN"

typedef enum
{
    FILETYPE_IMAGE,
    FILETYPE_VIDEO
}
FileType;

void            file_type_update_image_types (gchar * imgtype_disables,
					      gchar * imgtype_user_defs);
void            file_type_free_image_types (void);
gchar          *file_type_detect_type_by_ext (const gchar * str);
GList          *file_type_get_sysdef_ext_list_all (void);
GList          *file_type_get_sysdef_ext_list (void);
GList          *file_type__get_userdef_ext_list (void);
gchar          *file_type_get_type_from_ext (gchar * ext);
gboolean        file_type_is_movie (gchar * filename);
gboolean        file_type_is_jpeg (gchar * filename);

#endif /* __FILE_TYPE_H__ */
