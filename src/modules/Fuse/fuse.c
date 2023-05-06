/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "module.h"
#include "config.h"
#include "operators.h"
#include "interpret.h"
#include "pike_error.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "program.h"
#include "bignum.h"
#include "backend.h"
#include "pike_types.h"

#ifdef HAVE_LIBFUSE
/* Attempt to use FUSE API version 2.9 (if possible). */
#define FUSE_USE_VERSION 29
#include <fuse.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

// All docs live in module.pmod.in This file, and Fuse, requires C99.

static struct object *global_fuse_obj; // There Can Be Only One

#define DEFAULT_ERRNO() do{if((TYPEOF(Pike_sp[-1]) == T_INT) && Pike_sp[-1].u.integer) return -Pike_sp[-1].u.integer;return -ENOENT;}while(0)

enum dispatch_id {
    PF_GETATTR,
    PF_READLINK,
    PF_GETDIR,
    PF_MKNOD,
    PF_MKDIR,
    PF_UNLINK,
    PF_RMDIR,
    PF_SYMLINK,
    PF_RENAME,
    PF_LINK,
    PF_CHMOD,
    PF_CHOWN,
    PF_TRUNCATE,
#if FUSE_VERSION < 26
    PF_UTIME,
#else
    PF_UTIMENS,
#endif
    PF_OPEN,
    PF_READ,
    PF_WRITE,
    PF_STATFS,
    PF_RELEASE,
    PF_FSYNC,
    PF_SETXATTR,
    PF_GETXATTR,
    PF_LISTXATTR,
    PF_REMOVEXATTR,
    PF_CREAT,
    PF_ACCESS,
    PF_LOCK
};

struct dispatch_struct {
    enum dispatch_id id;
    int ret;

    union {
        struct {
            const char *path;
            struct stat *stbuf;
        } pf_getattr;
        struct {
            const char *path;
            char *buf;
            size_t size;
        } pf_readlink;
        struct {
            const char *path;
            fuse_dirh_t h;
            fuse_dirfil_t filler;
        } pf_getdir;
        struct {
            const char *path;
            mode_t mode;
            dev_t rdev;
        } pf_mknod;
        struct {
            const char *path;
            mode_t mode;
        } pf_mkdir, pf_chmod;
        struct {
            const char *path;
        } pf_unlink, pf_rmdir;
        struct {
            const char *from;
            const char *to;
        } pf_symlink, pf_rename, pf_link;
        struct {
            const char *path;
            uid_t uid;
            gid_t gid;
        } pf_chown;
        struct {
            const char *path;
            off_t size;
        } pf_truncate;
#if FUSE_VERSION < 26
        struct {
            const char *path;
            struct utimbuf *buf;
        } pf_utime;
#else
        struct {
            const char *path;
            const struct timespec *tv;
        } pf_utimens;
#endif
        struct {
            const char *path;
            struct fuse_file_info *fi;
        } pf_open, pf_release;
        struct {
            const char *path;
            char *buf;
            size_t size;
            off_t offset;
            struct fuse_file_info *fi;
        } pf_read;
        struct {
            const char *path;
            const char *buf;
            size_t size;
            off_t offset;
            struct fuse_file_info *fi;
        } pf_write;
        struct {
            const char *path;
            struct statvfs *stbuf;
        } pf_statfs;
        struct {
            const char *path;
            int isdatasync;
            struct fuse_file_info *fi;
        } pf_fsync;
        struct {
            const char *path;
            const char *name;
            const char *value;
            size_t size;
            int flags;
        } pf_setxattr;
        struct {
            const char *path;
            const char *name;
            char *value;
            size_t size;
        } pf_getxattr;
        struct {
            const char *path;
            char *list;
            size_t size;
        } pf_listxattr;
        struct {
            const char *path;
            const char *name;
        } pf_removexattr;
        struct {
            const char *path;
            mode_t mode;
            struct fuse_file_info *fi;
        } pf_creat;
        struct {
            const char *path;
            int mode;
        } pf_access;
        struct {
            const char *path;
            struct fuse_file_info *fi;
            int cmd;
            struct flock *lck;
        } pf_lock;
    } sig;
};

static void low_dispatch(void*);

static int low_pf_getattr(const char *path, struct stat *stbuf)
{
    extern struct program *stat_program;
    struct stat *st;
    push_text( path );
    apply( global_fuse_obj, "getattr", 1 );
    if( TYPEOF(Pike_sp[-1]) != PIKE_T_OBJECT ||
	!(st = get_storage( Pike_sp[-1].u.object, stat_program)) )
	DEFAULT_ERRNO();
    *stbuf = *st;
    return 0;
}

static int pf_getattr(const char *path, struct stat *stbuf)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_GETATTR;
    dinfo.sig.pf_getattr.path = path;
    dinfo.sig.pf_getattr.stbuf = stbuf;
    call_with_interpreter(low_dispatch, &dinfo);
    return dinfo.ret;
}

static int low_pf_readlink(const char *path, char *buf, size_t size)
{
    int res;
    push_text( path );
    apply( global_fuse_obj, "readlink", 1 );
    if( (TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    if( Pike_sp[-1].u.string->len >= (int)size )
	return -ENAMETOOLONG;
    memcpy( buf, Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len+1);
    return 0;
}

static int pf_readlink(const char *path, char *buf, size_t size)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_READLINK;
    dinfo.sig.pf_readlink.path = path;
    dinfo.sig.pf_readlink.buf = buf;
    dinfo.sig.pf_readlink.size = size;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static struct program *getdir_program;
struct getdir_storage
{
    fuse_dirh_t h;
    fuse_dirfil_t filler;
};
#define THISGD ((struct getdir_storage *)Pike_fp->current_storage)
static void f_getdir_callback( INT32 args )
{
    char *name;
    get_all_args( NULL, args, "%c", &name );
    THISGD->filler( THISGD->h, name, 0, 0 );
}

static void push_getdir_callback( fuse_dirh_t h, fuse_dirfil_t filler )
{
    struct object *o=clone_object( getdir_program, 0 );
    ((struct getdir_storage*)o->storage)->h = h;
    ((struct getdir_storage*)o->storage)->filler = filler;
    push_object( o );
}

static int low_pf_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
    push_text( path );
    push_getdir_callback( h, filler );
    apply( global_fuse_obj, "readdir", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_GETDIR;
    dinfo.sig.pf_getdir.path = path;
    dinfo.sig.pf_getdir.h = h;
    dinfo.sig.pf_getdir.filler = filler;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_mknod(const char *path, mode_t mode, dev_t rdev)
{
    push_text( path );
    push_int( mode );
    push_int( rdev );
    apply( global_fuse_obj, "mknod", 3 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_mknod(const char *path, mode_t mode, dev_t rdev)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_MKNOD;
    dinfo.sig.pf_mknod.path = path;
    dinfo.sig.pf_mknod.mode = mode;
    dinfo.sig.pf_mknod.rdev = rdev;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_mkdir(const char *path, mode_t mode)
{
    push_text( path );
    push_int( mode );
    apply( global_fuse_obj, "mkdir", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_mkdir(const char *path, mode_t mode)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_MKDIR;
    dinfo.sig.pf_mkdir.path = path;
    dinfo.sig.pf_mkdir.mode = mode;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_unlink(const char *path)
{
    push_text( path );
    apply( global_fuse_obj, "unlink", 1 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_unlink(const char *path)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_UNLINK;
    dinfo.sig.pf_unlink.path = path;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_rmdir(const char *path)
{
    push_text( path );
    apply( global_fuse_obj, "rmdir", 1 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_rmdir(const char *path)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_RMDIR;
    dinfo.sig.pf_rmdir.path = path;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_symlink(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "symlink", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_symlink(const char *from, const char *to)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_SYMLINK;
    dinfo.sig.pf_symlink.from = from;
    dinfo.sig.pf_symlink.to = to;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_rename(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "rename", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_rename(const char *from, const char *to)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_RENAME;
    dinfo.sig.pf_rename.from = from;
    dinfo.sig.pf_rename.to = to;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_link(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "link", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_link(const char *from, const char *to)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_LINK;
    dinfo.sig.pf_link.from = from;
    dinfo.sig.pf_link.to = to;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_chmod(const char *path, mode_t mode)
{
    push_text( path );
    push_int( mode );
    apply( global_fuse_obj, "chmod", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_chmod(const char *path, mode_t mode)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_CHMOD;
    dinfo.sig.pf_chmod.path = path;
    dinfo.sig.pf_chmod.mode = mode;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_chown(const char *path, uid_t uid, gid_t gid)
{
    push_text( path );
    push_int( uid );
    push_int( gid );
    apply( global_fuse_obj, "chown", 3 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_chown(const char *path, uid_t uid, gid_t gid)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_CHOWN;
    dinfo.sig.pf_chown.path = path;
    dinfo.sig.pf_chown.uid = uid;
    dinfo.sig.pf_chown.gid = gid;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_truncate(const char *path, off_t size)
{
    push_text( path );
    push_int64( size );
    apply( global_fuse_obj, "truncate", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_truncate(const char *path, off_t size)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_TRUNCATE;
    dinfo.sig.pf_truncate.path = path;
    dinfo.sig.pf_truncate.size = size;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

#if FUSE_VERSION < 26
static int low_pf_utime(const char *path, struct utimbuf *buf)
{
    push_text( path );
    push_int( buf->actime );
    push_int( buf->modtime );
    apply( global_fuse_obj, "utime", 3 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}
static int pf_utime(const char *path, struct utimbuf *buf)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_UTIME;
    dinfo.sig.pf_utime.path = path;
    dinfo.sig.pf_utime.utimbuf = buf;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}
#else
static int low_pf_utimens(const char *path, const struct timespec tv[2])
{
    push_text( path );
    push_int64( (INT64)tv[0].tv_sec*(INT64)1000000000 + (INT64)tv[0].tv_nsec );
    push_int64( (INT64)tv[1].tv_sec*(INT64)1000000000 + (INT64)tv[1].tv_nsec );
    apply( global_fuse_obj, "utimens", 3 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_utimens(const char *path, const struct timespec tv[2])
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_UTIMENS;
    dinfo.sig.pf_utimens.path;
    dinfo.sig.pf_utimens.tv = tv;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}
#endif

static int low_pf_open(const char *path, struct fuse_file_info *fi)
{
    push_text( path );
    push_int( fi->flags );
    apply( global_fuse_obj, "open", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_open(const char *path, struct fuse_file_info *fi)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_OPEN;
    dinfo.sig.pf_open.path = path;
    dinfo.sig.pf_open.fi = fi;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *UNUSED(fi))
{
    push_text( path );
    push_int( size );
    push_int64( offset );
    apply( global_fuse_obj, "read", 3 );

    if( (TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    if (((size_t)Pike_sp[-1].u.string->len) > size) {
	return -ENAMETOOLONG;
    }
    memcpy( buf, Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len );
    return Pike_sp[-1].u.string->len;
}

static int pf_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    struct dispatch_struct dinfo;

    dinfo.id = PF_READ;
    dinfo.sig.pf_read.path = path;
    dinfo.sig.pf_read.buf = buf;
    dinfo.sig.pf_read.size = size;
    dinfo.sig.pf_read.offset = offset;
    dinfo.sig.pf_read.fi = fi;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_write(const char *path, const char *buf, size_t size,
		    off_t offset, struct fuse_file_info *UNUSED(fi))
{
    push_text( path );
    push_string( make_shared_binary_string(buf, size ) );
    push_int64( offset );
    apply( global_fuse_obj, "write", 3 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_write(const char *path, const char *buf, size_t size,
		    off_t offset, struct fuse_file_info *fi)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_WRITE;
    dinfo.sig.pf_write.path = path;
    dinfo.sig.pf_write.buf = buf;
    dinfo.sig.pf_write.size = size;
    dinfo.sig.pf_write.offset = offset;
    dinfo.sig.pf_write.fi = fi;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_statfs(const char *path, struct statvfs *stbuf)
{
    struct mapping *m;
    struct svalue *val;

    push_text( path );
    apply( global_fuse_obj, "statfs", 1 );
    if( TYPEOF(Pike_sp[-1]) != PIKE_T_MAPPING )
	DEFAULT_ERRNO();
    m = Pike_sp[-1].u.mapping;
    memset( stbuf, 0, sizeof(*stbuf) );
    stbuf->f_namemax = 4096;
    stbuf->f_bsize = 1024;
#define STSET(X)    do {				\
      if ((val=simple_mapping_string_lookup( m,#X )) &&	\
	  (TYPEOF(*val) == T_INT))			\
	stbuf->f_##X=val->u.integer;			\
    } while(0)
    STSET(bsize);
    STSET(blocks);
    STSET(bfree);
    STSET(bavail);
    STSET(files);
    STSET(ffree);
    if (((val=simple_mapping_string_lookup( m,"namemax" )) &&
	 (TYPEOF(*val) == T_INT)) ||
	/* namelen is compat. */
	((val=simple_mapping_string_lookup( m,"namelen" )) &&
	 (TYPEOF(*val) == T_INT))) {
      stbuf->f_namemax = val->u.integer;
    }
#undef STSET
    return 0;
}

static int pf_statfs(const char *path, struct statvfs *stbuf)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_STATFS;
    dinfo.sig.pf_statfs.path = path;
    dinfo.sig.pf_statfs.stbuf = stbuf;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_release(const char *path, struct fuse_file_info *UNUSED(fi))
{
    push_text( path );
    apply( global_fuse_obj, "release", 1 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_release(const char *path, struct fuse_file_info *fi)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_RELEASE;
    dinfo.sig.pf_release.path = path;
    dinfo.sig.pf_release.fi = fi;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *UNUSED(fi))
{
    push_text( path );
    push_int( isdatasync );
    apply( global_fuse_obj, "fsync", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_FSYNC;
    dinfo.sig.pf_fsync.path = path;
    dinfo.sig.pf_fsync.isdatasync = isdatasync;
    dinfo.sig.pf_fsync.fi = fi;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_setxattr(const char *path, const char *name,
                           const char *value, size_t size, int flags)
{
    push_text( path );
    push_text( name );
    push_string( make_shared_binary_string( value, size ) );
    push_int( flags );
    apply( global_fuse_obj, "setxattr", 4 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_SETXATTR;
    dinfo.sig.pf_setxattr.path = path;
    dinfo.sig.pf_setxattr.name = name;
    dinfo.sig.pf_setxattr.value = value;
    dinfo.sig.pf_setxattr.size = size;
    dinfo.sig.pf_setxattr.flags = flags;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_getxattr(const char *path, const char *name, char *value,
		       size_t size)
{
    unsigned int ds;
    push_text( path );
    push_text( name );
    apply( global_fuse_obj, "getxattr", 2 );
    if( TYPEOF(Pike_sp[-1]) != PIKE_T_STRING ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    ds = Pike_sp[-1].u.string->len <<Pike_sp[-1].u.string->size_shift;
    if( !size )
	return ds;
    if( size < ds )
	return -ERANGE;
    memcpy( value, Pike_sp[-1].u.string->str, ds );
    return ds;
}

static int pf_getxattr(const char *path, const char *name, char *value,
		       size_t size)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_GETXATTR;
    dinfo.sig.pf_getxattr.path = path;
    dinfo.sig.pf_getxattr.name = name;
    dinfo.sig.pf_getxattr.value = value;
    dinfo.sig.pf_getxattr.size = size;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_listxattr(const char *path, char *list, size_t size)
{
    unsigned int ds;

    push_text( path );
    apply( global_fuse_obj, "listxattr", 1 );
    if( TYPEOF(Pike_sp[-1]) != PIKE_T_ARRAY )
	DEFAULT_ERRNO();
    push_string( make_shared_binary_string( "\0", 1 ) );
    o_multiply();
    if( TYPEOF(Pike_sp[-1]) != PIKE_T_STRING ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    /* We need to account for the terminating NUL. */
    ds = (Pike_sp[-1].u.string->len + 1) <<Pike_sp[-1].u.string->size_shift;
    if( !size )
	return ds;
    if( size < ds )
	return -ERANGE;
    memcpy( list, Pike_sp[-1].u.string->str, ds );
    return ds;
}

static int pf_listxattr(const char *path, char *list, size_t size)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_LISTXATTR;
    dinfo.sig.pf_listxattr.path = path;
    dinfo.sig.pf_listxattr.list = list;
    dinfo.sig.pf_listxattr.size = size;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_removexattr(const char *path, const char *name)
{
    push_text( path );
    push_text( name );
    apply( global_fuse_obj, "removexattr", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_removexattr(const char *path, const char *name)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_REMOVEXATTR;
    dinfo.sig.pf_removexattr.path = path;
    dinfo.sig.pf_removexattr.name= name;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_creat( const char *path, mode_t mode,
                         struct fuse_file_info *fi)
{
    push_text( path );
    push_int( mode );
    push_int( fi->flags );
    apply( global_fuse_obj, "creat", 3 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_creat( const char *path, mode_t mode, struct fuse_file_info *fi)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_CREAT;
    dinfo.sig.pf_creat.path = path;
    dinfo.sig.pf_creat.mode = mode;
    dinfo.sig.pf_creat.fi = fi;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_access( const char *path, int mode)
{
    push_text( path );
    push_int( mode );
    apply( global_fuse_obj, "access", 2 );
    if (TYPEOF(Pike_sp[-1]) != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_access( const char *path, int mode)
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_ACCESS;
    dinfo.sig.pf_access.path = path;
    dinfo.sig.pf_access.mode = mode;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static int low_pf_lock( const char *path, struct fuse_file_info *fi, int cmd,
                        struct flock *lck )
{
  push_text( path );
  push_int( cmd );

  push_static_text("owner"); push_int( fi->lock_owner );
  ref_push_string(literal_type_string);  push_int( lck->l_type );
  push_static_text("whence");push_int( lck->l_whence );
  push_static_text("start");   push_int( lck->l_start );
  push_static_text("len");   push_int( lck->l_len );
  push_static_text("pid");   push_int( lck->l_pid );
  f_aggregate_mapping( 6 * 2 );
  apply( global_fuse_obj, "lock", 3 );

  if( TYPEOF(Pike_sp[-1]) == PIKE_T_MAPPING )
  {
    struct svalue *tmp;
#define COUT(X)                                                         \
    if((tmp=simple_mapping_string_lookup(Pike_sp[-1].u.mapping,#X))     \
       && TYPEOF(*tmp)==PIKE_T_INT) lck->l_##X = tmp->u.integer;
    COUT(type);  COUT(whence);
    COUT(start);  COUT(len);
    COUT(pid);
#undef COUT
    return 0;
  }
  else
  {
    if (TYPEOF(Pike_sp[-1]) != T_INT)
      DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
  }
}

static int pf_lock( const char *path, struct fuse_file_info *fi, int cmd, struct flock *lck )
{
    struct dispatch_struct dinfo;

    dinfo.id = PF_LOCK;
    dinfo.sig.pf_lock.path = path;
    dinfo.sig.pf_lock.fi = fi;
    dinfo.sig.pf_lock.cmd = cmd;
    dinfo.sig.pf_lock.lck = lck;
    call_with_interpreter(low_dispatch, &dinfo);

    return dinfo.ret;
}

static void low_dispatch(void *vinfo) {
    struct dispatch_struct *dinfo = vinfo;

    /* If we error in Pike code later, this will stand and ought to be
     * a better return value than most. */
    dinfo->ret = -ENOSYS;

    switch (dinfo->id) {
    case PF_GETATTR:
        dinfo->ret = low_pf_getattr(dinfo->sig.pf_getattr.path,
                                    dinfo->sig.pf_getattr.stbuf);
        return;
    case PF_READLINK:
        dinfo->ret = low_pf_readlink(dinfo->sig.pf_readlink.path,
                                     dinfo->sig.pf_readlink.buf,
                                     dinfo->sig.pf_readlink.size);
        return;
    case PF_GETDIR:
        dinfo->ret = low_pf_getdir(dinfo->sig.pf_getdir.path,
                                   dinfo->sig.pf_getdir.h,
                                   dinfo->sig.pf_getdir.filler);
        return;
    case PF_MKNOD:
        dinfo->ret = low_pf_mknod(dinfo->sig.pf_mknod.path,
                                  dinfo->sig.pf_mknod.mode,
                                  dinfo->sig.pf_mknod.rdev);
        return;
    case PF_MKDIR:
        dinfo->ret = low_pf_mkdir(dinfo->sig.pf_mkdir.path,
                                  dinfo->sig.pf_mkdir.mode);
        return;
    case PF_UNLINK:
        dinfo->ret = low_pf_unlink(dinfo->sig.pf_unlink.path);
        return;

    case PF_RMDIR:
        dinfo->ret = low_pf_rmdir(dinfo->sig.pf_rmdir.path);
        return;
    case PF_SYMLINK:
        dinfo->ret = low_pf_symlink(dinfo->sig.pf_symlink.from,
                                    dinfo->sig.pf_symlink.to);
        return;
    case PF_RENAME:
        dinfo->ret = low_pf_rename(dinfo->sig.pf_rename.from,
                                   dinfo->sig.pf_rename.to);
        return;
    case PF_LINK:
        dinfo->ret = low_pf_link(dinfo->sig.pf_link.from,
                                 dinfo->sig.pf_link.to);
        return;
    case PF_CHMOD:
        dinfo->ret = low_pf_chmod(dinfo->sig.pf_chmod.path,
                                  dinfo->sig.pf_chmod.mode);
        return;
    case PF_CHOWN:
        dinfo->ret = low_pf_chown(dinfo->sig.pf_chown.path,
                                  dinfo->sig.pf_chown.uid,
                                  dinfo->sig.pf_chown.gid);
        return;
    case PF_TRUNCATE:
        dinfo->ret = low_pf_truncate(dinfo->sig.pf_truncate.path,
                                     dinfo->sig.pf_truncate.size);
        return;
#if FUSE_VERSION < 26
    case PF_UTIME:
        dinfo->ret = low_pf_utime(dinfo->sig.pf_utime.path,
                                  dinfo->sig.pf_utime.buf);
        return;
#else
    case PF_UTIMENS:
        dinfo->ret = low_pf_utimens(dinfo->sig.pf_utimens.path,
                                    dinfo->sig.pf_utimens.tv);
        return;
#endif
    case PF_OPEN:
        dinfo->ret = low_pf_open(dinfo->sig.pf_open.path,
                                 dinfo->sig.pf_open.fi);
        return;
    case PF_READ:
        dinfo->ret = low_pf_read(dinfo->sig.pf_read.path,
                                 dinfo->sig.pf_read.buf,
                                 dinfo->sig.pf_read.size,
                                 dinfo->sig.pf_read.offset,
                                 dinfo->sig.pf_read.fi);
        return;
    case PF_WRITE:
        dinfo->ret = low_pf_write(dinfo->sig.pf_write.path,
                                  dinfo->sig.pf_write.buf,
                                  dinfo->sig.pf_write.size,
                                  dinfo->sig.pf_write.offset,
                                  dinfo->sig.pf_write.fi);
        return;
    case PF_STATFS:
        dinfo->ret = low_pf_statfs(dinfo->sig.pf_statfs.path,
                                   dinfo->sig.pf_statfs.stbuf);
        return;
    case PF_RELEASE:
        dinfo->ret = low_pf_release(dinfo->sig.pf_release.path,
                                    dinfo->sig.pf_release.fi);
        return;
    case PF_FSYNC:
        dinfo->ret = low_pf_fsync(dinfo->sig.pf_fsync.path,
                                  dinfo->sig.pf_fsync.isdatasync,
                                  dinfo->sig.pf_fsync.fi);
        return;
    case PF_SETXATTR:
        dinfo->ret = low_pf_setxattr(dinfo->sig.pf_setxattr.path,
                                     dinfo->sig.pf_setxattr.name,
                                     dinfo->sig.pf_setxattr.value,
                                     dinfo->sig.pf_setxattr.size,
                                     dinfo->sig.pf_setxattr.flags);
        return;
    case PF_GETXATTR:
        dinfo->ret = low_pf_getxattr(dinfo->sig.pf_getxattr.path,
                                     dinfo->sig.pf_getxattr.name,
                                     dinfo->sig.pf_getxattr.value,
                                     dinfo->sig.pf_getxattr.size);
        return;
    case PF_LISTXATTR:
        dinfo->ret = low_pf_listxattr(dinfo->sig.pf_listxattr.path,
                                      dinfo->sig.pf_listxattr.list,
                                      dinfo->sig.pf_listxattr.size);
        return;
    case PF_REMOVEXATTR:
        dinfo->ret = low_pf_removexattr(dinfo->sig.pf_removexattr.path,
                                        dinfo->sig.pf_removexattr.name);
        return;
    case PF_CREAT:
        dinfo->ret = low_pf_creat(dinfo->sig.pf_creat.path,
                                  dinfo->sig.pf_creat.mode,
                                  dinfo->sig.pf_creat.fi);
        return;
    case PF_ACCESS:
        dinfo->ret = low_pf_access(dinfo->sig.pf_access.path,
                                   dinfo->sig.pf_access.mode);
        return;
    case PF_LOCK:
        dinfo->ret = low_pf_lock(dinfo->sig.pf_lock.path,
                                 dinfo->sig.pf_lock.fi,
                                 dinfo->sig.pf_lock.cmd,
                                 dinfo->sig.pf_lock.lck);
        return;
    }
}
static struct fuse_operations pike_fuse_oper = {
    .getattr	= pf_getattr,
    .readlink	= pf_readlink,
    .getdir	= pf_getdir,
    .mknod	= pf_mknod,
    .mkdir	= pf_mkdir,
    .unlink	= pf_unlink,
    .rmdir	= pf_rmdir,
    .symlink	= pf_symlink,
    .rename	= pf_rename,
    .link	= pf_link,
    .chmod	= pf_chmod,
    .chown	= pf_chown,
    .truncate	= pf_truncate,
#if FUSE_VERSION >= 26
    .utimens	= pf_utimens,
#else
    .utime	= pf_utime,
#endif
#if FUSE_VERSION >= 26
    .lock	= pf_lock,
#endif
    .open	= pf_open,
    .read	= pf_read,
    .write	= pf_write,
    .statfs	= pf_statfs,
    .release	= pf_release,
    .fsync	= pf_fsync,
    .setxattr	= pf_setxattr,
    .getxattr	= pf_getxattr,
    .listxattr	= pf_listxattr,
    .removexattr= pf_removexattr,
    .access     = pf_access,
    .create     = pf_creat,
};

static void f_fuse_run( INT32 nargs )
{
    struct fuse *fuse;
    int i, multi, fd;
    char **argv, *mountpoint;
    struct array *args;
    int ret;

    if( global_fuse_obj )
	Pike_error("There can be only one.\n"
		   "You have to run multiple processes to have multiple FUSE filesystems\n");

    get_all_args( NULL, nargs, "%o%a", &global_fuse_obj, &args );

    argv = malloc( sizeof(char *) * args->size );
    for( i = 0; i<args->size; i++ )
    {
        if( TYPEOF(args->item[i]) != PIKE_T_STRING ||
	    string_has_null(args->item[i].u.string) )
	{
	    free( argv );
	    Pike_error("Argument %d is not a nonbinary string\n", i );
	}
	argv[i] = args->item[i].u.string->str;
    }

    enable_external_threads();
    THREADS_ALLOW();
    ret = fuse_main(args->size, argv, &pike_fuse_oper, NULL);
    THREADS_DISALLOW();

    free(argv);
    exit(ret);
}

PIKE_MODULE_EXIT
{
  free_program( getdir_program );
}

/*! @decl constant FUSE_MAJOR_VERSION
 *! @decl constant FUSE_MINOR_VERSION
 *!
 *!  The version of FUSE
 */

PIKE_MODULE_INIT
{
    ADD_FUNCTION( "run", f_fuse_run, tFunc(tObj tArr(tStr),tVoid ), 0 );

    start_new_program( );
    {
	ADD_STORAGE( struct getdir_storage );
	ADD_FUNCTION( "`()", f_getdir_callback, tFunc(tStr tOr(tInt,tVoid) tOr(tInt,tVoid),tVoid ), 0 );
    }
    getdir_program = end_program();

    add_integer_constant("F_RDLCK", F_GETLK, 0 );
    add_integer_constant("F_WRLCK", F_WRLCK, 0 );
    add_integer_constant("F_UNLCK", F_UNLCK, 0 );
    add_integer_constant("F_GETLK", F_GETLK, 0 );
    add_integer_constant("F_SETLK", F_SETLK, 0 );
    add_integer_constant("F_SETLKW", F_SETLKW, 0 );
    add_integer_constant("FUSE_MAJOR_VERSION", FUSE_MAJOR_VERSION, 0);
    add_integer_constant("FUSE_MINOR_VERSION", FUSE_MINOR_VERSION, 0);
}
#else
PIKE_MODULE_EXIT
{
}

PIKE_MODULE_INIT
{
  HIDE_MODULE();
}
#endif /* HAVE_LIBFUSE */
