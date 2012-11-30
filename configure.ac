dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([pornview], [0.2.1], [])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall])
AM_PROG_AR
LT_INIT([disable-static])
PACKAGE=pornview

dnl version number
MAJOR_VERSION=0
MINOR_VERSION=2
MICRO_VERSION=1
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION

dnl
AC_DEFINE_UNQUOTED([PACKAGE], ["$PACKAGE"], [packetname])
AC_DEFINE_UNQUOTED([VERSION], ["$VERSION"], [packetversion])

dnl Specify a header configuration file
AC_CONFIG_HEADERS([config.h])

dnl ******************************
dnl Checks for programs.
dnl ******************************
AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PATH_X
AC_PROG_INSTALL
AC_PATH_PROG([PKGCONFIG], [pkg-config])
AS_IF([test "x$PKGCONFIG" = "x"],
    [AC_MSG_ERROR([pkg-config not found!])])

dnl ******************************
dnl libtool
dnl ******************************
AC_PROG_LIBTOOL

dnl ******************************
dnl required library version
dnl ******************************
XINE_REQUIRED=1.0.0

AC_SUBST([GLIB_REQUIRED])
AC_SUBST([GTK_REQUIRED])
AC_SUBST([GDK_PIXBUF_REQUIRED])
AC_SUBST([XINE_REQUIRED])

# iconv
AM_ICONV

# Gtk+-2.0
GLIB2_REQUIRED=2.0.0
GMODULE2_REQUIRED=2.0.0
GTK2_REQUIRED=2.0.0
GDK_PIXBUF2_REQUIRED=2.0.0

AC_SUBST([GLIB2_REQUIRED])
AC_SUBST([GMODULE2_REQUIRED])
AC_SUBST([GTK2_REQUIRED])
AC_SUBST([GDK_PIXBUF2_REQUIRED])

dnl ******************************
dnl Haiku does not use libm for the math functions
dnl ******************************
AC_SEARCH_LIBS([floor], [m], [], [AC_MSG_ERROR([unable to find the floor function])])

dnl ******************************
dnl Check for GTK
dnl ******************************
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])
PKG_CHECK_MODULES([GTK], [
   glib-2.0 >= $GLIB2_REQUIRED
   gmodule-2.0 >= $GMODULE2_REQUIRED
   gdk-pixbuf-2.0 >= $GDK_PIXBUF2_REQUIRED
   gtk+-2.0 >= $GTK2_REQUIRED
], [], [
   AC_MSG_ERROR([Test for GTK+-2.0 failed. See the file 'INSTALL' for help])
])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])
AC_DEFINE([USE_GTK2], [1], [use GTK2])
AC_DEFINE([USE_NORMAL_PANED], [1], [use normal paned])
AC_DEFINE([ENABLE_TREEVIEW], [1], [enable treeview])

enable_normal_paned=no
AC_ARG_WITH([normal-paned],
   [  --with-normal-paned]     Use standard gtk+ paned,
   [enable_normal_paned=yes], [enable_normal_paned=no])
if test "$enable_normal_paned" = yes; then
   AC_DEFINE([USE_NORMAL_PANED], [1], [use normal paned])
fi

dnl ********************
dnl Test for libpng
dnl ********************
PNG_CFLAGS=""
PNG_LIBS=""
AC_CHECK_LIB(png, png_read_info, [
   AC_CHECK_HEADER(png.h,
      [ png_ok=yes ],
      [ png_ok=no ])
], [
   AC_MSG_ERROR([*** (PNG library is not found) ***])
], -lz -lm)

if test "$png_ok" = yes; then
   AC_MSG_CHECKING([for png_structp in png.h])
   AC_TRY_COMPILE([ #include <png.h> ],
      [ png_structp pp;
        png_infop info;
        png_colorp cmap;
        png_get_header_ver(NULL);
        png_create_read_struct; ],
      [ png_ok=yes ],
      [ png_ok=no ])
   AC_MSG_RESULT([$png_ok])
   if test "$png_ok" = yes; then
      PNG_LIBS='-lpng -lz'
   else
      AC_MSG_ERROR([*** PNG library (libpng) is too old ***])
   fi
else
   AC_MSG_ERROR([*** PNG library (libpng) required ***])
fi

AM_CONDITIONAL([HAVE_PNG], [test "x$PNG_LIBS" != x])
AC_SUBST([PNG_CFLAGS])
AC_SUBST([PNG_LIBS])

dnl ********************
dnl Test for libjpeg
dnl ********************
JPEG_CFLAGS=""
JPEG_LIBS=""
available_jpeg=no
AC_CHECK_HEADER(jpeglib.h, [
   AC_CHECK_LIB(jpeg, jpeg_finish_decompress, [
      available_jpeg=yes; JPEG_LIBS=-ljpeg;
   ], [], $JPEG_LIBS)
])
AM_CONDITIONAL([HAVE_JPEG], [test "$available_jpeg" = "yes"])
AC_SUBST([JPEG_CFLAGS])
AC_SUBST([JPEG_LIBS])

movie_support="no"
dnl ******************************
dnl Test for movie support
dnl ******************************
AC_ARG_ENABLE(xine,
   [  --enable-xine           Enable Xine library for movie [default=no]],
   [enable_xine=yes], [enable_xine="${enableval}"])

if test "$enable_xine" = no; then
AC_ARG_ENABLE(mplayer,
   [  --enable-mplayer        Enable MPlayer for movie [default=no]],
   [enable_mplayer=yes], [enable_mplayer="${enableval}"])
fi

MOVIELIB_CFLAGS=""
MOVIELIB_LIBS=""

dnl ****************************
dnl Check for Xine library
dnl ****************************
if test "$enable_xine" = yes; then
   have_xine=no
   PKG_CHECK_MODULES([XINE], [libxine],
      [have_xine=yes], [have_xine=no])
   if test "$have_xine" = yes; then
      MOVIELIB_CFLAGS=${XINE_CFLAGS}
      MOVIELIB_LIBS=${XINE_LIBS}
      AC_DEFINE([ENABLE_XINE], [1], [enable Xine support])
      AC_DEFINE([ENABLE_MOVIE], [1], [enable Movie support])
      movie_support="Xine 1.x"
   else
      AC_MSG_WARN([Did not find xine-lib installed])
      enable_xine=no;
   fi
fi

AC_SUBST([MOVIELIB_CFLAGS])
AC_SUBST([MOVIELIB_LIBS])

dnl check for Xinerama
AC_PATH_XTRA
XINERAMA_LIBS=""
# need AC_CHECK_HEADERS cause of "#if HAVE_X11_EXTENSIONS_XINERAMA_H" in the code
AC_CHECK_HEADERS([X11/extensions/Xinerama.h])
PKG_CHECK_MODULES([XINERAMA], [xinerama], [], [AC_MSG_ERROR(Xinerama library not found!)])
PKG_CHECK_MODULES([X], [x11 xext], [], [AC_MSG_ERROR([X11 libraries not found!])])
AC_SUBST([XINERAMA_LIBS])
AC_SUBST([X_LIBS])

dnl ****************************
dnl Test for MPlayer
dnl ****************************
if test "$enable_mplayer" = yes; then
  AC_DEFINE([ENABLE_MPLAYER], [1], [enable mplayer support])
  AC_DEFINE([ENABLE_MOVIE], [1], [enable movie support])
  movie_support="MPlayer"
fi

dnl ******************************
dnl for gettext
dnl ******************************
ALL_LINGUAS="fr pl"
AM_GNU_GETTEXT([external])
localedir="\${datadir}/locale"
AC_SUBST(localedir)

dnl ******************************
dnl enable/disable EXIF support
dnl ******************************
AC_ARG_ENABLE(exif,
   [  --enable-exif           enable EXIF support [default=yes]],
   [enable_exif="${enableval}"], [enable_exif=yes])
AM_CONDITIONAL([ENABLE_EXIF], [test "$enable_exif" = yes])
if test "$enable_exif" = yes; then
   AC_DEFINE([ENABLE_EXIF], [1], [enable EXIF support])
   LIBEXIF="support/libexif/libexif.la"
   AC_SUBST([LIBEXIF])
fi

dnl ******************************
dnl Checks for header files.
dnl ******************************
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([							\
	alloca.h argz.h							\
	errno.h								\
	fcntl.h								\
	langinfo.h libintl.h limits.h locale.h				\
	malloc.h memory.h						\
	netinet/in.h nl_types.h						\
	stddef.h stdlib.h string.h strings.h sys/param.h sys/time.h	\
	unistd.h utime.h						\
])

dnl **************************************************************
dnl Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl **********************************
dnl Checks for library functions.
dnl **********************************
AC_FUNC_ALLOCA
dnl AC_FUNC_CHOWN
dnl AC_FUNC_FORK
dnl AC_FUNC_LSTAT
dnl AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
dnl AC_FUNC_MKTIME
dnl AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([						\
	__argz_count __argz_next __argz_stringify		\
	dcgettext dup2						\
	endgrent endpwent					\
	floor							\
	getcwd getpagesize getwd				\
	memmove mempcpy memset mkdir munmap			\
	nl_langinfo						\
	pow putenv setenv					\
	setlocale sqrt stpcpy strcasecmp strchr strcspn		\
	strdup strrchr strstr strtoul				\
	uname utime						\
])

dnl ******************************
dnl set data dir definition
dnl ******************************
dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], ["${ac_default_prefix}/share/${PACKAGE}"], [package datadir])
  else
    AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], ["${prefix}/share/${PACKAGE}"], [package datadir])
  fi
else
  AC_DEFINE_UNQUOTED([PACKAGE_DATA_DIR], ["${datadir}/${PACKAGE}"], [package datadir])
fi

dnl Set PACKAGE_SOURCE_DIR in config.h.
packagesrcdir=`cd $srcdir && pwd`
AC_DEFINE_UNQUOTED([PACKAGE_SOURCE_DIR], ["${packagesrcdir}"], [package datadir])

AC_CONFIG_FILES([
Makefile
icons/Makefile
src/Makefile
src/pixmaps/Makefile
src/scripts/Makefile
src/support/Makefile
src/prefs_ui/Makefile
src/support/cursors/Makefile
src/support/libexif/Makefile
src/support/pixmaps/Makefile
src/support/widgets/Makefile
src/support/widgets/pixmaps/Makefile
])

AC_OUTPUT

dnl
dnl Show some info:
dnl

AC_MSG_RESULT([
Configure Result :

   C Compiler    : ${CC} ${CFLAGS}
   Install path  : ${prefix}
   Normal paned	 : ${enable_normal_paned}
   Gtk+-2.0      : ${enable_gtk2}
   Movie support : ${movie_support}
   EXIF          : ${enable_exif}
])
