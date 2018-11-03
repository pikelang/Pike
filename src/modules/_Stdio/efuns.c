/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "fdlib.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "pike_macros.h"
#include "fd_control.h"
#include "threads.h"
#include "module_support.h"
#include "constants.h"
#include "backend.h"
#include "operators.h"
#include "builtin_functions.h"
#include "bignum.h"

#include "file_machine.h"
#include "file.h"

#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <signal.h>
#include <errno.h>

#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif /* HAVE_SYS_XATTR_H */

#if 0
#ifdef HAVE_LIBZFS_INIT
#ifdef HAVE_LIBZFS_H
#include <libzfs.h>
#endif /* HAVE_LIBZFS_H */
static libzfs_handle_t *libzfs_handle;
#endif /* HAVE_LIBZFS_INIT */
#endif /* 0 */

#define sp Pike_sp

/* #define READDIR_DEBUG */
#ifdef READDIR_DEBUG
#define RDWERR(...) fprintf(stderr,__VA_ARGS__)
#else
#define RDWERR(...)
#endif

#ifdef __NT__

#include <winbase.h>
#include <io.h>
#include <direct.h>

/* Old versions of the headerfiles don't have this constant... */
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

/* Dynamic load of functions that doesn't exist in all Windows versions. */

static HINSTANCE kernel32lib;

#define LINKFUNC(RET,NAME,TYPE) \
  typedef RET (WINAPI * PIKE_CONCAT(NAME,type)) TYPE ; \
  static PIKE_CONCAT(NAME,type) NAME

LINKFUNC(BOOL, movefileex, (
  LPCTSTR lpExistingFileName,  /* file name     */
  LPCTSTR lpNewFileName,       /* new file name */
  DWORD dwFlags                /* move options  */
));

#endif /* __NT__ */

struct array *encode_stat(PIKE_STAT_T *s)
{
  struct array *a;
  a=allocate_array(7);
  a->type_field = BIT_INT;
  ITEM(a)[0].u.integer=s->st_mode;
  switch(S_IFMT & s->st_mode)
  {
  case S_IFREG:
    push_int64((INT64)s->st_size);
    stack_pop_to_no_free (ITEM(a) + 1);
    if (TYPEOF(ITEM(a)[1]) == T_OBJECT) a->type_field |= BIT_OBJECT;
    break;

  case S_IFDIR: ITEM(a)[1].u.integer=-2; break;
#ifdef S_IFLNK
  case S_IFLNK: ITEM(a)[1].u.integer=-3; break;
#endif
  default:
    ITEM(a)[1].u.integer=-4;
    break;
  }
  ITEM(a)[2].u.integer = (INT_TYPE) s->st_atime;
  ITEM(a)[3].u.integer = (INT_TYPE) s->st_mtime;
  ITEM(a)[4].u.integer = (INT_TYPE) s->st_ctime;
  ITEM(a)[5].u.integer=s->st_uid;
  ITEM(a)[6].u.integer=s->st_gid;
  return a;
}

#if defined(HAVE_FSETXATTR) && defined(HAVE_FGETXATTR) && defined(HAVE_FLISTXATTR)
/*! @decl array(string) listxattr( string file, void|int(0..1) symlink )
 *!
 *! Return an array of all extended attributes set on the file
 */

#ifdef HAVE_DARWIN_XATTR
#define LISTXATTR(PATH, BUF, SZ)	listxattr(PATH, BUF, SZ, 0)
#define LLISTXATTR(PATH, BUF, SZ)	listxattr(PATH, BUF, SZ, XATTR_NOFOLLOW)
#else
#define LISTXATTR(PATH, BUF, SZ)	listxattr(PATH, BUF, SZ)
#define LLISTXATTR(PATH, BUF, SZ)	llistxattr(PATH, BUF, SZ)
#endif /* !HAVE_DARWIN_XATTR */

static void f_listxattr(INT32 args)
{
  char buffer[1024];
  char *ptr = buffer;
  char *name;
  int do_free = 0;
  int nofollow = 0;
  ssize_t res;
  get_all_args( "listxattr", args, "%s.%d", &name, &nofollow );

  THREADS_ALLOW();
  do {
    /* First try, for speed.*/
    if (nofollow)
      res = LLISTXATTR(name, buffer, sizeof(buffer));
    else
      res = LISTXATTR( name, buffer, sizeof(buffer));
  } while( res < 0 && errno == EINTR );
  THREADS_DISALLOW();

  if( res<0 && errno==ERANGE )
  {
    /* Too little space in stackbuffer.*/
    size_t blen = 65536;
    ptr = NULL;
    do_free = 1;
    do {
      if (!ptr) {
	ptr = xalloc(blen);
      } else {
	char *tmp = realloc( ptr, blen );
	if( !tmp )
	  break;
	ptr = tmp;
      }
      THREADS_ALLOW();
      do {
	if (nofollow)
	  res = LLISTXATTR(name, ptr, sizeof(blen));
	else
	  res = LISTXATTR(name, ptr, blen);
      } while( res < 0 && errno == EINTR );
      THREADS_DISALLOW();
      blen *= 2;
    } while( (res < 0) && (errno == ERANGE) );
  }

  pop_n_elems( args );
  if (res < 0)
  {
    if (do_free)
      free(ptr);
    push_int(0);
    return;
  }

  push_string( make_shared_binary_string( ptr, res ) );
  ptr[0]=0;
  push_string( make_shared_binary_string( ptr, 1 ) );
  o_divide();
  push_empty_string();
  f_aggregate(1);
  o_subtract();

  if (do_free)
    free(ptr);
}

#ifdef HAVE_DARWIN_XATTR
#define GETXATTR(PATH, NAME, BUF, SZ)	getxattr(PATH, NAME, BUF, SZ, 0, 0)
#define LGETXATTR(PATH, NAME, BUF, SZ)	getxattr(PATH, NAME, BUF, SZ, 0, XATTR_NOFOLLOW)
#else
#define GETXATTR(PATH, NAME, BUF, SZ)	getxattr(PATH, NAME, BUF, SZ)
#define LGETXATTR(PATH, NAME, BUF, SZ)	lgetxattr(PATH, NAME, BUF, SZ)
#endif /* !HAVE_DARWIN_XATTR */

/*! @decl string getxattr(string file, string attr, void|int(0..1) symlink)
 *!
 *! Return the value of a specified attribute, or 0 if it does not exist.
 */
static void f_getxattr(INT32 args)
{
  char buffer[1024];
  char *ptr = buffer;
  int do_free = 0;
  ssize_t res;
  char *name, *file;
  int nofollow=0;
  get_all_args( "getxattr", args, "%s%s.%d", &file, &name, &nofollow );

  THREADS_ALLOW();
  do {
    /* First try, for speed.*/
    if (nofollow)
      res = LGETXATTR(file, name, buffer, sizeof(buffer));
    else
      res = GETXATTR(file, name, buffer, sizeof(buffer));
  } while( res < 0 && errno == EINTR );
  THREADS_DISALLOW();

  if( res<0 && errno==ERANGE )
  {
    /* Too little space in buffer.*/
    size_t blen = 65536;
    do_free = 1;
    ptr = NULL;
    do {
      if (!ptr) {
	ptr = xalloc(blen);
      } else {
	char *tmp = realloc( ptr, blen );
	if( !tmp )
	  break;
	ptr = tmp;
      }
      THREADS_ALLOW();
      do {
	if (nofollow)
	  res = LGETXATTR(file, name, ptr, blen);
	else
	  res = GETXATTR(file, name, ptr, blen);
      } while( res < 0 && errno == EINTR );
      THREADS_DISALLOW();
      blen *= 2;
    } while( (res < 0) && (errno == ERANGE) );
  }

  if( res < 0 )
  {
    if( do_free && ptr )
      free(ptr);
    push_int(0);
    return;
  }

  push_string( make_shared_binary_string( ptr, res ) );
  if( do_free && ptr )
    free( ptr );
}


#ifdef HAVE_DARWIN_XATTR
#define REMOVEXATTR(PATH, NAME)		removexattr(PATH, NAME, 0)
#define LREMOVEXATTR(PATH, NAME)	removexattr(PATH, NAME, XATTR_NOFOLLOW)
#else
#define REMOVEXATTR(PATH, NAME)		removexattr(PATH, NAME)
#define LREMOVEXATTR(PATH, NAME)	lremovexattr(PATH, NAME)
#endif /* !HAVE_DARWIN_XATTR */

/*! @decl void removexattr( string file, string attr , void|int(0..1) symlink)
 *! Remove the specified extended attribute.
 */
static void f_removexattr( INT32 args )
{
  char *name, *file;
  int nofollow=0, rv;

  get_all_args( "removexattr", args, "%s%s.%d", &file, &name, &nofollow );

  THREADS_ALLOW();
  if (nofollow) {
    while(((rv=LREMOVEXATTR(file, name)) < 0) && (errno == EINTR))
      ;
  } else {
    while(((rv=REMOVEXATTR(file, name)) < 0) && (errno == EINTR))
      ;
  }
  THREADS_DISALLOW();

  pop_n_elems(args);
  if( rv < 0 )
  {
    push_int(0);
  }
  else
  {
    push_int(1);
  }
}

#ifdef HAVE_DARWIN_XATTR
#define SETXATTR(PATH, NAME, BUF, SZ, FL)	setxattr(PATH, NAME, BUF, SZ, 0, FL)
#define LSETXATTR(PATH, NAME, BUF, SZ, FL)	setxattr(PATH, NAME, BUF, SZ, 0, (FL)|XATTR_NOFOLLOW)
#else
#define SETXATTR(PATH, NAME, BUF, SZ, FL)	setxattr(PATH, NAME, BUF, SZ, FL)
#define LSETXATTR(PATH, NAME, BUF, SZ, FL)	lsetxattr(PATH, NAME, BUF, SZ, FL)
#endif /* !HAVE_DARWIN_XATTR */

/*! @decl void setxattr( string file, string attr, string value, int flags,void|int(0..1) symlink)
 *!
 *! Set the attribute @[attr] to the value @[value].
 *!
 *! The flags parameter can be used to refine the semantics of the operation.
 *!
 *! @[Stdio.XATTR_CREATE] specifies a pure create, which
 *! fails if the named attribute exists already.
 *!
 *! @[Stdio.XATTR_REPLACE] specifies a pure replace operation, which
 *! fails if the named attribute does not already exist.
 *!
 *! By default (no flags), the extended attribute will be created if need be,
 *! or will simply replace the value if the attribute exists.
 *!
 *! @returns
 *! 1 if successful, 0 otherwise, setting errno.
 */
static void f_setxattr( INT32 args )
{
  char *ind, *file;
  struct pike_string *val;
  int flags;
  int rv;
  int nofollow=0;
  get_all_args( "setxattr", args, "%s%s%S%d.%d", &file, &ind, &val, &flags, &nofollow );

  THREADS_ALLOW();
  if (nofollow) {
    while(((rv=LSETXATTR(file, ind, val->str,
			  (val->len<<val->size_shift), flags )) < 0) &&
	  (errno == EINTR))
      ;
  } else {
    while(((rv=SETXATTR(file, ind, val->str,
			(val->len<<val->size_shift), flags )) < 0) &&
	  (errno == EINTR))
      ;
  }
  THREADS_DISALLOW();
  pop_n_elems(args);
  if( rv < 0 )
  {
    push_int(0);
  }
  else
    push_int(1);
}
#endif


/*! @decl Stdio.Stat file_stat(string path, void|int(0..1) symlink)
 *!
 *! Stat a file.
 *!
 *! If the argument @[symlink] is @expr{1@} symlinks will not be followed.
 *!
 *! @returns
 *!   If the path specified by @[path] doesn't exist @expr{0@} (zero) will
 *!   be returned.
 *!
 *!   Otherwise an object describing the properties of @[path] will be
 *!   returned.
 *!
 *! @note
 *!   In Pike 7.0 and earlier this function returned an array with 7 elements.
 *!   The old behaviour can be simulated with the following function:
 *! @code
 *! array(int) file_stat(string path, void|int(0..1) symlink)
 *! {
 *!   File.Stat st = predef::file_stat(path, symlink);
 *!   if (!st) return 0;
 *!   return (array(int))st;
 *! }
 *! @endcode
 *!
 *! @seealso
 *!   @[Stdio.Stat], @[Stdio.File->stat()]
 */
void f_file_stat(INT32 args)
{
  PIKE_STAT_T st;
  int i, l;
  struct pike_string *str;

  if(args<1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("file_stat", 1);
  if((TYPEOF(sp[-args]) != T_STRING) || sp[-args].u.string->size_shift)
    SIMPLE_ARG_TYPE_ERROR("file_stat", 1, "string(0..255)");

  str = sp[-args].u.string;
  l = (args>1 && !UNSAFE_IS_ZERO(sp+1-args))?1:0;

  if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  THREADS_ALLOW_UID();
  do {
#ifdef HAVE_LSTAT
    if(l)
      i=fd_lstat(str->str, &st);
    else
#endif
      i=fd_stat(str->str, &st);
  } while ((i == -1) && (errno == EINTR));

  THREADS_DISALLOW_UID();
  pop_n_elems(args);
  if(i==-1)
  {
    push_int(0);
  }else{
    push_stat(&st);
  }
}

/*! @decl int file_truncate(string file, int length)
 *!
 *! Truncates the file @[file] to the length specified in @[length].
 *!
 *! @returns
 *!   Returns 1 if ok, 0 if failed.
 */
void f_file_truncate(INT32 args)
{
  INT64 len = 0;
  struct pike_string *str;
  int res;

  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("file_truncate", 2);
  if(TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("file_truncate", 1, "string");

#if defined (HAVE_FTRUNCATE64) || SIZEOF_OFF_T > SIZEOF_INT_TYPE
  if(is_bignum_object_in_svalue(&Pike_sp[1-args])) {
    if (!int64_from_bignum(&len, Pike_sp[1-args].u.object))
      Pike_error ("Bad argument 2 to file_truncate(). Length too large.\n");
  }
  else
#endif
    if(TYPEOF(sp[1-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR("file_truncate", 2, "int");
    else
      len = sp[1-args].u.integer;

  str = sp[-args].u.string;

  if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  res = fd_truncate(str->str, len);

  pop_n_elems(args);

  push_int(!res);
}

/*! @decl mapping(string:int) filesystem_stat(string path)
 *!
 *! Returns a mapping describing the properties of the filesystem
 *! containing the path specified by @[path].
 *!
 *! @returns
 *!   If a filesystem cannot be determined @expr{0@} (zero) will be returned.
 *!
 *!   Otherwise a mapping(string:int) with the following fields will be
 *!   returned:
 *!   @mapping
 *!     @member int "blocksize"
 *!       Size in bytes of the filesystem blocks.
 *!     @member int "blocks"
 *!       Size of the entire filesystem in blocks.
 *!     @member int "bfree"
 *!       Number of free blocks in the filesystem.
 *!     @member int "bavail"
 *!       Number of available blocks in the filesystem.
 *!       This is usually somewhat less than the @expr{"bfree"@} value, and
 *!       can usually be adjusted with eg tunefs(1M).
 *!     @member int "files"
 *!       Total number of files (aka inodes) allowed by this filesystem.
 *!     @member int "ffree"
 *!       Number of free files in the filesystem.
 *!     @member int "favail"
 *!       Number of available files in the filesystem.
 *!       This is usually the same as the @expr{"ffree"@} value, and can
 *!       usually be adjusted with eg tunefs(1M).
 *!     @member string "fsname"
 *!       Name assigned to the filesystem. This item is not available
 *!       on all systems.
 *!     @member string "fstype"
 *!       Type of filesystem (eg @expr{"nfs"@}). This item is not
 *!       available on all systems. For some more uncommon filesystems
 *!       this may be an integer representing the magic number for the
 *!       filesystem type (cf @tt{statfs(2)@} on eg Linux systems).
 *!   @endmapping
 *!
 *! @note
 *!   Please note that not all members are present on all OSs.
 *!
 *! @seealso
 *!   @[file_stat()]
 */
#ifdef __NT__

void f_filesystem_stat( INT32 args )
{
  char *path;
  DWORD sectors_per_cluster = -1;
  DWORD bytes_per_sector = -1;
  DWORD free_clusters = -1;
  DWORD total_clusters = -1;
  p_wchar1 *root;
  char _p[4];
  char *p = _p;
  unsigned int free_sectors;
  unsigned int total_sectors;

  get_all_args( "filesystem_stat", args, "%s", &path );

  root = pike_dwim_utf8_to_utf16(path);
  if (root[0] && root[1] == ':') {
    root[2] = '\\';
    root[3] = 0;
  } else {
    free(root);
    root = NULL;
  }

  if(!GetDiskFreeSpaceW( root, &sectors_per_cluster,
			 &bytes_per_sector,
			 &free_clusters,
			 &total_clusters ))
  {
    if (root) free(root);
    pop_n_elems(args);
    push_int( 0 );
    return;
  }

  if (root) free(root);

  free_sectors = sectors_per_cluster  * free_clusters;
  total_sectors = sectors_per_cluster * total_clusters;

  pop_n_elems( args );
  push_static_text("blocksize");
  push_int(bytes_per_sector);
  push_static_text("blocks");
  push_int(total_sectors);
  push_static_text("bfree");
  push_int(free_sectors);
  push_static_text("bavail");
  push_int(free_sectors);
  f_aggregate_mapping( 8 );
}

#else /* !__NT__ */

#if !defined(HAVE_STRUCT_STATFS) && !defined(HAVE_STRUCT_FS_DATA)
#undef HAVE_STATFS
#endif

#if defined(HAVE_STATFS) && defined(HAVE_STATVFS) && !defined(HAVE_STATVFS_F_BASETYPE)
/* Linux libc doesn't provide fs type info in statvfs(2),
 * so use statfs(2) instead.
 */
#undef HAVE_STATVFS
#endif

#if defined(HAVE_STATVFS) || defined(HAVE_STATFS) || defined(HAVE_USTAT)
#ifdef HAVE_SYS_STATVFS_H
/* Kludge for broken SCO headerfiles */
#ifdef _SVID3
#include <sys/statvfs.h>
#else
#define _SVID3
#include <sys/statvfs.h>
#undef _SVID3
#endif /* _SVID3 */
#endif /* HAVE_SYS_STATVFS_H */
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif /* HAVE_SYS_VFS_H */
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#ifdef HAVE_LINUX_MAGIC_H
#include <linux/magic.h>
#endif /* HAVE_LINUX_MAGIC_H */
#endif /* HAVE_SYS_STATFS_H */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif /* HAVE_SYS_MOUNT_H */
#if !defined(HAVE_STATVFS) && !defined(HAVE_STATFS)
#ifdef HAVE_USTAT_H
#include <ustat.h>
#endif /* HAVE_USTAT_H */
#endif /* !HAVE_STATVFS && !HAVE_STATFS */
void f_filesystem_stat(INT32 args)
{
#ifdef HAVE_STATVFS
  struct statvfs st;
#else /* !HAVE_STATVFS */
#ifdef HAVE_STATFS
#ifdef HAVE_STRUCT_STATFS
  struct statfs st;
#else /* !HAVE_STRUCT_STATFS */
#ifdef HAVE_STRUCT_FS_DATA
  /* Probably only ULTRIX has this name for the struct */
  struct fs_data st;
#else /* !HAVE_STRUCT_FS_DATA */
#error No struct to hold statfs() data.
#endif /* HAVE_STRUCT_FS_DATA */
#endif /* HAVE_STRUCT_STATFS */
#else /* !HAVE_STATFS */
#ifdef HAVE_USTAT
  PIKE_STAT_T statbuf;
  struct ustat st;
#else /* !HAVE_USTAT */
#error No stat function for filesystems.
#endif /* HAVE_USTAT */
#endif /* HAVE_STATFS */
#endif /* HAVE_STATVFS */
  int i;
  struct pike_string *str;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("filesystem_stat", 1);
  if(TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("filesystem_stat", 1, "string");

  str = sp[-args].u.string;

  if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  THREADS_ALLOW();
#ifdef HAVE_STATVFS
  i = statvfs(str->str, &st);
#else /* !HAVE_STATVFS */
#ifdef HAVE_STATFS
#ifdef HAVE_SYSV_STATFS
  i = statfs(str->str, &st, sizeof(st), 0);
#else
  i = statfs(str->str, &st);
#endif /* HAVE_SYSV_STATFS */
#else /* !HAVE_STATFS */
#ifdef HAVE_USTAT
  if (!(i = fd_stat(str->str, &statbuf))) {
    i = ustat(statbuf.st_rdev, &st);
  }
#else
#error No stat function for filesystems.
#endif /* HAVE_USTAT */
#endif /* HAVE_STATFS */
#endif /* HAVE_STATVFS */
  THREADS_DISALLOW();
  pop_n_elems(args);
  if(i==-1)
  {
    push_int(0);
  }else{
    int num_fields = 0;
#ifdef HAVE_STATVFS
#if 0
    push_static_text("id");         push_int(st.f_fsid);
    num_fields++;
#endif
    push_static_text("blocksize");  push_int(st.f_frsize);
    push_static_text("blocks");     push_int(st.f_blocks);
    push_static_text("bfree");      push_int(st.f_bfree);
    push_static_text("bavail");     push_int(st.f_bavail);
    push_static_text("files");      push_int(st.f_files);
    push_static_text("ffree");      push_int(st.f_ffree);
    push_static_text("favail");     push_int(st.f_favail);
    num_fields += 7;
#ifdef HAVE_STATVFS_F_FSTR
    push_static_text("fsname");     push_text(st.f_fstr);
    num_fields++;
#endif /* HAVE_STATVFS_F_FSTR */
#ifdef HAVE_STATVFS_F_BASETYPE
    push_static_text("fstype");     push_text(st.f_basetype);
    num_fields++;
#endif /* HAVE_STATVFS_F_BASETYPE */
#else /* !HAVE_STATVFS */
#ifdef HAVE_STATFS
#ifdef HAVE_STRUCT_STATFS
#if 0 && HAVE_STATFS_F_FSID
    push_static_text("id");           push_int(st.f_fsid);
    num_fields++;
#endif
    push_static_text("blocksize");    push_int(st.f_bsize);
    push_static_text("blocks");       push_int(st.f_blocks);
    push_static_text("bfree");        push_int(st.f_bfree);
    push_static_text("files");        push_int(st.f_files);
    push_static_text("ffree");        push_int(st.f_ffree);
    push_static_text("favail");       push_int(st.f_ffree);
    num_fields += 6;
#ifdef HAVE_STATFS_F_BAVAIL
    push_static_text("bavail");       push_int(st.f_bavail);
    num_fields++;
#endif /* HAVE_STATFS_F_BAVAIL */
    push_static_text("fstype");
#ifdef HAVE_STATFS_F_FSTYPENAME
    push_text(st.f_fstypename);
#else
    switch(st.f_type) {
#ifdef BTRFS_SUPER_MAGIC
   case BTRFS_SUPER_MAGIC:	push_static_text("btrfs");	break;
#endif /* BTRFS_SUPER_MAGIC */
#ifdef EXT2_SUPER_MAGIC
    case EXT2_SUPER_MAGIC:	push_static_text("ext");	break;
#endif /* EXT2_SUPER_MAGIC */
#ifdef ISOFS_SUPER_MAGIC
    case ISOFS_SUPER_MAGIC:	push_static_text("isofs");	break;
#endif /* ISOFS_SUPER_MAGIC */
#ifdef JFFS2_SUPER_MAGIC
    case JFFS2_SUPER_MAGIC:	push_static_text("jffs2");	break;
#endif /* JFFS2_SUPER_MAGIC */
#ifdef MSDOS_SUPER_MAGIC
    case MSDOS_SUPER_MAGIC:	push_static_text("msdos");	break;
#endif /* MSDOS_SUPER_MAGIC */
#ifdef NFS_SUPER_MAGIC
    case NFS_SUPER_MAGIC:	push_static_text("nfs");	break;
#endif /* NFS_SUPER_MAGIC */
#ifndef NTFS_SB_MAGIC
#define NTFS_SB_MAGIC	0x5346544e
#endif
    case NTFS_SB_MAGIC:		push_static_text("ntfs");	break;
#ifdef PROC_SUPER_MAGIC
    case PROC_SUPER_MAGIC:	push_static_text("procfs");	break;
#endif /* PROC_SUPER_MAGIC */
#ifdef RAMFS_MAGIC
    case RAMFS_MAGIC:		push_static_text("ramfs");	break;
#endif /* RAMFS_MAGIC */
#ifdef REISERFS_SUPER_MAGIC
    case REISERFS_SUPER_MAGIC:	push_static_text("reiserfs");	break;
#endif /* REISERFS_SUPER_MAGIC */
#ifdef SMB_SUPER_MAGIC 
    case SMB_SUPER_MAGIC:	push_static_text("smb");	break;
#endif /* SMB_SUPER_MAGIC */
#ifdef SYSFS_MAGIC
    case SYSFS_MAGIC:		push_static_text("sysfs");	break;
#endif /* SYSFS_MAGIC */
#ifdef TMPFS_MAGIC
    case TMPFS_MAGIC:		push_static_text("tmpfs");	break;
#endif /* TMPFS_MAGIC */
#ifdef XENFS_SUPER_MAGIC
    case XENFS_SUPER_MAGIC:	push_static_text("xenfs");	break;
#endif /* XENFS_SUPER_MAGIC */
    default:
      push_int(st.f_type);
      break;
    }
#endif /* HAVE_STATFS_F_FSTYPENAME */
    num_fields++;
#else /* !HAVE_STRUCT_STATFS */
#ifdef HAVE_STRUCT_FS_DATA
    /* ULTRIX */
    push_static_text("blocksize");    push_int(st.fd_bsize);
    push_static_text("blocks");       push_int(st.fd_btot);
    push_static_text("bfree");        push_int(st.fd_bfree);
    push_static_text("bavail");       push_int(st.fd_bfreen);
    num_fields += 4;
#else /* !HAVE_STRUCT_FS_DATA */
#error No struct to hold statfs() data.
#endif /* HAVE_STRUCT_FS_DATA */
#endif /* HAVE_STRUCT_STATFS */
#else /* !HAVE_STATFS */
#ifdef HAVE_USTAT
    push_static_text("bfree");      push_int(st.f_tfree);
    push_static_text("ffree");      push_int(st.f_tinode);
    push_static_text("fsname");     push_text(st.f_fname);
    num_fields += 3;
#else
#error No stat function for filesystems.
#endif /* HAVE_USTAT */
#endif /* HAVE_STATFS */
#endif /* HAVE_STATVFS */
    f_aggregate_mapping(num_fields*2);
  }
}

#endif /* HAVE_STATVFS || HAVE_STATFS || HAVE_USTAT */
#endif /* __NT__ */

/*! @decl int rm(string f)
 *!
 *! Remove a file or directory.
 *!
 *! @returns
 *!   Returns @expr{0@} (zero) on failure, @expr{1@} otherwise.
 *!
 *! @note
 *!   May fail with @[errno()] set to @[EISDIR] or @[ENOTDIR]
 *!   if the file has changed to a directory during the call
 *!   or the reverse.
 *!
 *! @seealso
 *!   @[Stdio.File()->unlinkat()], @[mkdir()], @[Stdio.recursive_rm()]
 */
void f_rm(INT32 args)
{
  PIKE_STAT_T st;
  INT32 i;
  struct pike_string *str;

  destruct_objects_to_destruct();

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("rm", 1);

  if(TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("rm", 1, "string");

  str = sp[-args].u.string;

  if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  THREADS_ALLOW_UID();
#ifdef HAVE_LSTAT
  i=fd_lstat(str->str, &st) != -1;
#else
  i=fd_stat(str->str, &st) != -1;
#endif
  if(i)
  {
    if(S_IFDIR == (S_IFMT & st.st_mode))
    {
      while (!(i = fd_rmdir(str->str) != -1) && (errno == EINTR))
	;
    }else{
      while (!(i = fd_unlink(str->str) != -1) && (errno == EINTR))
	;
    }
  }
  THREADS_DISALLOW_UID();

  pop_n_elems(args);
  push_int(i);
}

/*! @decl int mkdir(string dirname, void|int mode)
 *!
 *! Create a directory.
 *!
 *! If @[mode] is specified, it's will be used for the new directory after
 *! being @expr{&@}'ed with the current umask (on OS'es that support this).
 *!
 *! @returns
 *!   Returns @expr{0@} (zero) on failure, @expr{1@} otherwise.
 *!
 *! @seealso
 *!   @[rm()], @[cd()], @[Stdio.mkdirhier()]
 */
void f_mkdir(INT32 args)
{
  struct pike_string *str;
  int mode;
  int i;
  char *s, *s_dup;

  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("mkdir", 1);

  if(TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("mkdir", 1, "string");

  mode = 0777;			/* &'ed with ~umask anyway. */

  if(args > 1)
  {
    if(TYPEOF(sp[1-args]) != T_INT)
      Pike_error("Bad argument 2 to mkdir.\n");

    mode = sp[1-args].u.integer;
  }

  str = sp[-args].u.string;

  if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = EINVAL;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  /* Remove trailing / or \ which is not accepted by all mkdir()
     implementations (e.g. Mac OS X and Windows) */
  s = str->str;
  s_dup = NULL;
  if (str->len && (s[str->len - 1] == '/' || s[str->len - 1] == '\\')) {
    if ((s_dup = strdup(s))) {
      s = s_dup;
      s[str->len - 1] = '\0';
    }
  }

  THREADS_ALLOW_UID();
  i = fd_mkdir(s, mode) != -1;
  THREADS_DISALLOW_UID();

  if (s_dup)
    free(s_dup);

  pop_n_elems(args);
  push_int(i);
}

#undef HAVE_READDIR_R
#if defined(HAVE_SOLARIS_READDIR_R) || defined(HAVE_HPUX_READDIR_R) || \
    defined(HAVE_POSIX_READDIR_R)
#define HAVE_READDIR_R
#endif

#if defined(_REENTRANT) && defined(HAVE_READDIR_R)

#ifdef _PC_NAME_MAX
#ifdef HAVE_FPATHCONF

#ifdef HAVE_FDOPENDIR
#define USE_FDOPENDIR
#define USE_FPATHCONF
#elif defined(HAVE_DIRFD)
#define USE_FPATHCONF
#endif

#endif /* HAVE_FPATHCONF */

#if defined(HAVE_PATHCONF) && !defined(USE_FPATHCONF)
#define USE_PATHCONF
#endif
#endif /* _PC_NAME_MAX */

#endif /* _REENTRANT && HAVE_READDIR_R */

/*! @decl array(string) get_dir(void|string dirname)
 *!
 *! Returns an array of all filenames in the directory @[dirname], or
 *! @expr{0@} (zero) if the directory does not exist. When no
 *! @[dirname] is given, current work directory is used.
 *!
 *! @seealso
 *!   @[mkdir()], @[cd()]
 */
#ifdef __NT__
void f_get_dir(INT32 args)
{
  HANDLE dir;
  WIN32_FIND_DATAW d;
  struct pike_string *str=0;
  p_wchar1 *pattern;
  size_t plen;

  get_all_args("get_dir", args, ".%S", &str);

  /* NB: The empty string is also an alias for the current directory.
   *     This is a convenience eg when recursing with dirname().
   */
  if(!str || !str->len) {
    push_static_text(".");
    str = Pike_sp[-1].u.string;
    args++;
  } else if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pattern = pike_dwim_utf8_to_utf16(str->str);

  if (!pattern) {
    SIMPLE_OUT_OF_MEMORY_ERROR("get_dir", (str->len + 4) * sizeof(p_wchar1));
  }

  plen = wcslen(pattern);

  /* Append "/" "*".
   *
   * NB: pike_dwim_utf8_to_utf16() allocates space for at
   *     least 3 extra characters.
   */
  if (plen && (pattern[plen-1] != '/') && (pattern[plen-1] != '\\')) {
    pattern[plen++] = '/';
  }
  pattern[plen++] = '*';
  pattern[plen] = 0;

  RDWERR("FindFirstFile(\"%S\")...\n", STR1(sb.s));

  dir = FindFirstFileW(pattern, &d);

  free(pattern);

  if (dir == INVALID_HANDLE_VALUE) {
    int err = GetLastError();
    RDWERR("  INVALID_HANDLE_VALUE, error %d\n", err);

    pop_n_elems(args);
    if (err == ERROR_FILE_NOT_FOUND) {
      /* Normally there should at least be a "." entry, so this seldom
       * happens. But it seems unwise to count on it, considering this
       * being Windows and all..
       *
       * Note: The error is ERROR_PATH_NOT_FOUND if the directory
       * doesn't exist.
       */
      push_empty_array();
    }
    else {
      set_errno_from_win32_error (err);
      push_int(0);
    }
    return;
  }

  {
    int err;

    BEGIN_AGGREGATE_ARRAY(10);

    do {
      ONERROR uwp;
      p_wchar0 *utf8_fname;

      RDWERR("  \"%S\"\n", d.cFileName);
      /* Filter "." and ".." from the list. */
      if(d.cFileName[0]=='.')
      {
	if(!d.cFileName[1]) continue;
	if(d.cFileName[1]=='.' && !d.cFileName[2]) continue;
      }

      utf8_fname = pike_utf16_to_utf8(d.cFileName);
      if (!utf8_fname) {
	SIMPLE_OUT_OF_MEMORY_ERROR("get_dir",
				   (wcslen(d.cFileName) + 4) * sizeof(p_wchar1));
      }

      SET_ONERROR(uwp, free, utf8_fname);
      push_text(utf8_fname);
      CALL_AND_UNSET_ONERROR(uwp);

      DO_AGGREGATE_ARRAY(120);
    } while(FindNextFileW(dir, &d));
    err = GetLastError();

    RDWERR("  DONE, error %d\n", err);

    FindClose(dir);

    END_AGGREGATE_ARRAY;

    stack_pop_n_elems_keep_top(args);

    if (err != ERROR_SUCCESS && err != ERROR_NO_MORE_FILES) {
      set_errno_from_win32_error (err);
      pop_stack();
      push_int (0);
      return;
    }
  }
}

#else /* !__NT__ */

/* Note: Also used from file_get_dir(). */
void low_get_dir(DIR *dir, ptrdiff_t UNUSED(name_max))
{
  if(dir) {
    struct dirent *d;
#if defined(_REENTRANT)
#define FPR 1024
    char buffer[MAXPATHLEN * 4];
    char *ptrs[FPR];
    ptrdiff_t lens[FPR];
#endif /* _REENTRANT */

    BEGIN_AGGREGATE_ARRAY(10);

#if defined(_REENTRANT)
    while(1)
    {
      int e;
      int num_files=0;
      char *bufptr=buffer;
      int err = 0;

      THREADS_ALLOW();

      while(1)
      {
	/* Modern impementations of readdir(3C) are thread-safe with
	 * respect to unique DIRs.
	 *
	 * Linux Glibc has deprecated readdir_r(3C).
	 */
	errno = 0;
	while (!(d = readdir(dir)) && (errno == EINTR))
	  ;
	if (!d) {
          RDWERR("readdir(), d= %p\n", d);
          RDWERR("readdir() => errno %d\n", errno);
          /* Solaris readdir_r seems to set errno to ENOENT sometimes.
	   *
	   * AIX readdir seems to set errno to EBADF at end of dir.
	   */
	  if ((errno == ENOENT) || (errno == EBADF)) {
	    errno = 0;
	  }
	  break;
	}
        RDWERR("POSIX readdir_r() => \"%s\"\n", d->d_name);
	/* Filter "." and ".." from the list. */
	if(d->d_name[0]=='.')
	{
	  if(NAMLEN(d)==1) continue;
	  if(d->d_name[1]=='.' && NAMLEN(d)==2) continue;
	}
	if(num_files >= FPR) break;
	lens[num_files]=NAMLEN(d);
	if(bufptr+lens[num_files] >= buffer+sizeof(buffer)) break;
	memcpy(bufptr, d->d_name, lens[num_files]);
	ptrs[num_files]=bufptr;
	bufptr+=lens[num_files];
	num_files++;
      }
      THREADS_DISALLOW();
      if ((!d) && err) {
	Pike_error("get_dir(): readdir_r() failed: %d\n", err);
      }
      RDWERR("Pushing %d filenames...\n", num_files);
      for(e=0;e<num_files;e++)
      {
	push_string(make_shared_binary_string(ptrs[e],lens[e]));
      }
      if(d)
	push_string(make_shared_binary_string(d->d_name,NAMLEN(d)));
      else
	break;
      DO_AGGREGATE_ARRAY(120);
    }
#else
    for(d=readdir(dir); d; d=readdir(dir))
    {
      RDWERR("readdir(): %s\n", d->d_name);
      /* Filter "." and ".." from the list. */
      if(d->d_name[0]=='.')
      {
	if(NAMLEN(d)==1) continue;
	if(d->d_name[1]=='.' && NAMLEN(d)==2) continue;
      }
      push_string(make_shared_binary_string(d->d_name, NAMLEN(d)));

      DO_AGGREGATE_ARRAY(120);
    }
#endif
    closedir(dir);

    END_AGGREGATE_ARRAY;
  } else {
    push_int(0);
  }
}

void f_get_dir(INT32 args)
{
#ifdef USE_FDOPENDIR
  int dir_fd;
#endif
  DIR *dir = NULL;
  ptrdiff_t name_max = -1;
  struct pike_string *str=0;

  get_all_args("get_dir",args,".%N",&str);

  /* NB: The empty string is also an alias for the current directory.
   *     This is a convenience eg when recursing with dirname().
   */
  if(!str || !str->len) {
#if defined(__amigaos4__)
    push_empty_string();
#else
    push_static_text(".");
#endif
    str = Pike_sp[-1].u.string;
    args++;
  }

  if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  THREADS_ALLOW_UID();
#ifdef USE_FDOPENDIR
  dir_fd = open(str->str, O_RDONLY);
  if (dir_fd != -1) {
#ifdef USE_FPATHCONF
    name_max = fpathconf(dir_fd, _PC_NAME_MAX);
#endif /* USE_FPATHCONF */
    dir = fdopendir(dir_fd);
    if (!dir) close(dir_fd);
  }
#else
  dir = opendir(str->str);
#ifdef USE_FPATHCONF
  if (dir) {
    name_max = fpathconf(dirfd(dir), _PC_NAME_MAX);
  }
#endif
#endif /* !HAVE_FDOPENDIR */
#ifdef USE_PATHCONF
  name_max = pathconf(str->str, _PC_NAME_MAX);
#endif

  THREADS_DISALLOW_UID();

  low_get_dir(dir, name_max);
  stack_pop_n_elems_keep_top(args);
}
#endif /* __NT__ */

/*! @decl int cd(string s)
 *!
 *! Change the current directory for the whole Pike process.
 *!
 *! @returns
 *!   Returns @expr{1@} for success, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[getcwd()]
 */
void f_cd(INT32 args)
{
  INT32 i;
  struct pike_string *str;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("cd", 1);

  if(TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("cd", 1, "string");

  str = sp[-args].u.string;

  if (string_has_null(str)) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  i = fd_chdir(str->str) != -1;
  pop_n_elems(args);
  push_int(i);
}

/*! @decl string getcwd()
 *!
 *! Returns the current working directory.
 *!
 *! @seealso
 *!   @[cd()]
 */
void f_getcwd(INT32 args)
{
  char *e = fd_get_current_dir_name();
  if (!e) {
    Pike_error("Failed to fetch current path.\n");
  }

  pop_n_elems(args);
  push_text(e);
  free(e);
}

#ifdef HAVE_EXECVE
/*! @decl int exece(string file, array(string) args)
 *! @decl int exece(string file, array(string) args, @
 *!                 mapping(string:string) env)
 *!
 *!   This function transforms the Pike process into a process running
 *!   the program specified in the argument @[file] with the arguments @[args].
 *!
 *!   If the mapping @[env] is present, it will completely replace all
 *!   environment variables before the new program is executed.
 *!
 *! @returns
 *!   This function only returns if something went wrong during @tt{exece(2)@},
 *!   and in that case it returns @expr{0@} (zero).
 *!
 *! @note
 *!   The Pike driver _dies_ when this function is called. You must either
 *!   use @[fork()] or @[Process.create_process()] if you wish to execute a
 *!   program and still run the Pike runtime.
 *!
 *!   This function is not available on all platforms.
 *!
 *! @seealso
 *!   @[Process.create_process()], @[fork()], @[Stdio.File->pipe()]
 */
void f_exece(INT32 args)
{
  INT32 e;
  char **argv, **env;
  struct svalue *save_sp;
  struct mapping *en;
#ifdef DECLARE_ENVIRON
  extern char **environ;
#endif

  save_sp=sp-args;

  if(args < 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("exece", 2);

  e=0;
  en=0;
  switch(args)
  {
  default:
    if(TYPEOF(sp[2-args]) != T_MAPPING)
      SIMPLE_ARG_TYPE_ERROR("exece", 3, "mapping(string:string)");
    en=sp[2-args].u.mapping;
    mapping_fix_type_field(en);

    if(m_ind_types(en) & ~BIT_STRING)
      SIMPLE_ARG_TYPE_ERROR("exece", 3, "mapping(string:string)");
    if(m_val_types(en) & ~BIT_STRING)
      SIMPLE_ARG_TYPE_ERROR("exece", 3, "mapping(string:string)");

    /* FALLTHRU */

  case 2:
    if(TYPEOF(sp[1-args]) != T_ARRAY)
      SIMPLE_ARG_TYPE_ERROR("exece", 2, "array(string)");


    if(array_fix_type_field(sp[1-args].u.array) & ~BIT_STRING)
      SIMPLE_ARG_TYPE_ERROR("exece", 2, "array(string)");

    /* FALLTHRU */

  case 1:
    if(TYPEOF(sp[0-args]) != T_STRING)
      SIMPLE_ARG_TYPE_ERROR("exece", 1, "string");
    break;
  }

  argv=xalloc((2+sp[1-args].u.array->size) * sizeof(char *));

  argv[0]=sp[0-args].u.string->str;

  for(e=0;e<sp[1-args].u.array->size;e++)
  {
    union anything *a;
    a=low_array_get_item_ptr(sp[1-args].u.array,e,T_STRING);
    argv[e+1]=a->string->str;
  }
  argv[e+1]=0;

  if(en)
  {
    INT32 e, i = 0;
    struct keypair *k;

    env=calloc(1+m_sizeof(en), sizeof(char *));
    if(!env) {
      free(argv);
      SIMPLE_OUT_OF_MEMORY_ERROR("exece", (1+m_sizeof(en)*sizeof(char *)));
    }

    NEW_MAPPING_LOOP(en->data) {
      ref_push_string(k->ind.u.string);
      push_static_text("=");
      ref_push_string(k->val.u.string);
      f_add(3);
      env[i++]=sp[-1].u.string->str;
      dmalloc_touch_svalue(sp-1);
    }

    env[i]=0;
  }else{
    env=environ;
  }

  my_set_close_on_exec(0,0);
  my_set_close_on_exec(1,0);
  my_set_close_on_exec(2,0);

  do_set_close_on_exec();

#ifdef __NT__
#define DOCAST(X) ((const char * const *)(X))
#else
#define DOCAST(X) (X)
#endif

#ifdef HAVE_BROKEN_F_SETFD
  do_close_on_exec();
#endif /* HAVE_BROKEN_F_SETFD */

  execve(argv[0],DOCAST(argv),DOCAST(env));

  free((char *)argv);
  if(env != environ) free((char *)env);
  pop_n_elems(sp-save_sp);
  push_int(0);
}
#endif

/*! @decl int mv(string from, string to)
 *!
 *! Rename or move a file or directory.
 *!
 *! If the destination already exists, it will be replaced.
 *! Replacement often only works if @[to] is of the same type as
 *! @[from], i.e. a file can only be replaced by another file and so
 *! on. Also, a directory will commonly be replaced only if it's
 *! empty.
 *!
 *! On some OSs this function can't move directories, only rename
 *! them.
 *!
 *! @returns
 *!   Returns @expr{0@} (zero) on failure, @expr{1@} otherwise. Call
 *!   @[errno()] to get more error info on failure.
 *!
 *! @seealso
 *!   @[rm()]
 */
void f_mv(INT32 args)
{
  INT32 i;
  struct pike_string *str1;
  struct pike_string *str2;

  if(args!=2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("mv", 2);

  if(TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("mv", 1, "string");

  if(TYPEOF(sp[-args+1]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("mv", 2, "string");

  str1 = sp[-args].u.string;
  str2 = sp[1-args].u.string;

  if (string_has_null(str1) || string_has_null(str2)) {
    /* Filenames with NUL are not supported. */
    if (string_has_null(str1)) {
      errno = ENOENT;
    } else {
      errno = EINVAL;
    }
    pop_n_elems(args);
    push_int(0);
    return;
  }

  i = fd_rename(str1->str, str2->str);

  pop_n_elems(args);
  push_int(!i);
}

/*! @decl string strerror(int errno)
 *!
 *! This function returns a description of an error code. The error
 *! code is usually obtained from eg @[Stdio.File->errno()].
 *!
 *! @note
 *!   On some platforms the string returned can be somewhat nondescriptive.
 */
void f_strerror(INT32 args)
{
  char *s;
  int err;

  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("strerror", 1);
  if(TYPEOF(sp[-args]) != T_INT)
    SIMPLE_ARG_TYPE_ERROR("strerror", 1, "int");

  err = sp[-args].u.integer;
  pop_n_elems(args);
  if(err < 0 || err > 256 )
    s=0;
  else {
#ifdef HAVE_STRERROR
    s=strerror(err);
#else
    s=0;
#endif
  }
  if(s)
    push_text(s);
  else {
    push_static_text("Error ");
    push_int(err);
    f_add(2);
  }
}

/*! @decl int errno()
 *!
 *! This function returns the system error from the last file operation.
 *!
 *! @note
 *!   Note that you should normally use @[Stdio.File->errno()] instead.
 *!
 *! @seealso
 *!   @[Stdio.File->errno()], @[strerror()]
 */
static void f_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(errno);
}

#ifdef HAVE__ACCESS
#define access(PATH, FLAGS)	_access(PATH, FLAGS)
#define HAVE_ACCESS
#endif

#ifdef HAVE_ACCESS

#ifndef R_OK
#define R_OK	4
#define W_OK	2
#define X_OK	1
#define F_OK	0
#endif

/*! @decl int access( string path, string|void mode )
 *!
 *! access() checks if the calling process can access the file
 *! @[path]. Symbolic links are dereferenced.
 *!
 *! @param mode
 *!   The @[mode] specifies the accessibility checks to be performed, and
 *!   is either not specified or empty, in which case access() just tests
 *!   if the file exists, or one or more of the characters @expr{"rwx"@}.
 *!
 *!   r, w, and x test whether the file exists and grants read, write,
 *!   and execute permissions, respectively.
 *!
 *! The check is done using the calling process's real UID and GID,
 *! rather than the effective IDs as is done when actually attempting
 *! an operation (e.g., open(2)) on the file. This allows set-user-ID
 *! programs to easily determine the invoking user's authority.
 *!
 *! If the calling process is privileged (i.e., its real UID is zero),
 *! then an X_OK check is successful for a regular file if execute
 *! permission is enabled for any of the file owner, group, or other.
 *!
 *! @returns
 *!   @int
 *!     @value 1
 *!       When the file is accessible using the given permissions.
 *!
 *!     @value 0
 *!       When the file is not accessible, in which case @[errno] is set
 *!       to one of the following values:
 *!
 *!       @int
 *!         @value EACCESS
 *!           Access denied.
 *!
 *!         @value ELOOP
 *!           Too many symbolic links.
 *!
 *!         @value ENAMETOOLONG
 *!           The path is too long.
 *!
 *!         @value ENOENT
 *!           The file does not exist.
 *!
 *!         @value ENOTDIR
 *!           One of the directories used in @[path] is not, in fact, a directory.
 *!
 *!         @value EROFS
 *!           The filesystem is read only and write access was requested.
 *!       @endint
 *!
 *!       Other errors can occur, but are not directly related to the
 *!       requested path, such as @expr{ENOMEM@}, etc.
 *!    @endint
 *!
 *! @seealso
 *!    @[errno()], @[Stdio.File]
 */
static void f_access( INT32 args )
{
    const char *path;
    int flags, res;
    if( args == 2 )
    {
        char *how;
        int i;
        get_all_args( "access", args, "%s%s", &path, &how );
        flags = 0;
        for( i=0; how[i]; i++ )
        {
            switch( how[i] )
            {
                case 'r': flags |= R_OK; break;
                case 'w': flags |= W_OK; break;
                case 'x': flags |= X_OK; break;
            }
        }
        if( !flags )
            flags = F_OK;
    }
    else
    {
        get_all_args( "access", args, "%s", &path );
        flags = F_OK;
    }

    THREADS_ALLOW_UID();
    do {
        res = access( path, flags );
    } while( (res == -1) && (errno == EINTR) )
    THREADS_DISALLOW_UID();

    pop_n_elems(args);
    push_int( !res );
}
#endif /* HAVE_ACCESS */

void init_stdio_efuns(void)
{
  set_close_on_exec(0,1);
  set_close_on_exec(1,1);
  set_close_on_exec(2,1);

#if 0
#ifdef HAVE_LIBZFS_INIT
  libzfs_handle = libzfs_init();
#endif
#endif /* 0 */

#ifdef __NT__
  {
    /* MoveFileEx doesn't exist in W98 and earlier. */
    /* Correction, it exists but does not work -Hubbe */

    OSVERSIONINFO osversion;
    osversion.dwOSVersionInfoSize=sizeof(osversion);
    if(GetVersionEx(&osversion))
    {
      switch(osversion.dwPlatformId)
      {
	case VER_PLATFORM_WIN32s: /* win32s */
	case VER_PLATFORM_WIN32_WINDOWS: /* win9x */
	  break;
	default:
	  if ((kernel32lib = LoadLibrary ("kernel32")))
	    movefileex = (movefileextype) GetProcAddress (kernel32lib, "MoveFileExA");
      }
    }
  }
#endif

/* function(string,int|void:object) */
  ADD_EFUN("file_stat",f_file_stat,tFunc(tStr tOr(tInt,tVoid),tObjIs_STDIO_STAT), OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

  /* function(string,int:int(0..1)) */
  ADD_EFUN("file_truncate",f_file_truncate,tFunc(tStr tInt,tInt),OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
#if defined(HAVE_ACCESS)
  ADD_EFUN("access", f_access, tFunc(tStr tOr(tVoid,tStr),tInt),OPT_EXTERNAL_DEPEND);
#endif
#if defined(HAVE_FSETXATTR) && defined(HAVE_FGETXATTR) && defined(HAVE_FLISTXATTR)
  ADD_EFUN( "listxattr", f_listxattr, tFunc(tStr tOr(tVoid,tInt),tArr(tStr)), OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
  ADD_EFUN( "setxattr", f_setxattr, tFunc(tStr tStr tStr tInt tOr(tVoid,tInt),tInt), OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
  ADD_EFUN( "getxattr", f_getxattr, tFunc(tStr tStr tOr(tVoid,tInt),tStr), OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
  ADD_EFUN( "removexattr", f_removexattr, tFunc(tStr tStr  tOr(tVoid,tInt),tInt), OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
#endif

#if defined(HAVE_STATVFS) || defined(HAVE_STATFS) || defined(HAVE_USTAT) || defined(__NT__)

/* function(string:mapping(string:string|int)) */
  ADD_EFUN("filesystem_stat", f_filesystem_stat,tFunc(tStr,tMap(tStr,tOr(tStr,tInt))), OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
#endif /* HAVE_STATVFS || HAVE_STATFS */

/* function(:int) */
  ADD_EFUN("errno",f_errno,tFunc(tNone,tInt),OPT_EXTERNAL_DEPEND);

/* function(string:int) */
  ADD_EFUN("rm",f_rm,tFunc(tStr,tInt),OPT_SIDE_EFFECT);

/* function(string,void|int:int) */
  ADD_EFUN("mkdir",f_mkdir,tFunc(tStr tOr(tVoid,tInt),tInt),OPT_SIDE_EFFECT);

/* function(string,string:int) */
  ADD_EFUN("mv", f_mv,tFunc(tStr tStr,tInt), OPT_SIDE_EFFECT);

/* function(string:string *) */
  ADD_EFUN("get_dir",f_get_dir,tFunc(tOr(tVoid,tStr),tArr(tStr)),OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

/* function(string:int) */
  ADD_EFUN("cd",f_cd,tFunc(tStr,tInt),OPT_SIDE_EFFECT);

/* function(:string) */
  ADD_EFUN("getcwd",f_getcwd,tFunc(tNone,tStr),OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

#ifdef HAVE_EXECVE
/* function(string,mixed*,void|mapping(string:string):int) */
  ADD_EFUN("exece",f_exece,tFunc(tStr tArr(tMix) tOr(tVoid,tMap(tStr,tStr)),tInt),OPT_SIDE_EFFECT);
#endif

/* function(int:string) */
  ADD_EFUN("strerror",f_strerror,tFunc(tInt,tStr),0);
}

void exit_stdio_efuns(void)
{
#ifdef __NT__
  if (kernel32lib) {
    if (FreeLibrary (kernel32lib))
      movefileex = 0;
    kernel32lib = 0;
  }
#endif
#if 0
#ifdef HAVE_LIBZFS_INIT
  libzfs_fini(libzfs_handle);
#endif
#endif /* 0 */
}
