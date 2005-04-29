/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: fuse.c,v 1.1 2005/04/29 14:01:20 per Exp $
*/

#include "global.h"
#include "config.h"
#include "module.h"
#include "operators.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "program.h"
#include "bignum.h"

#ifdef HAVE_LIBFUSE
#define FUSE_USE_VERSION 22
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

// All docs live in module.pmod.in This file, and Fuse, requires C99.

#define THISC ((struct fuse_cmd_storage *)Pike_fp->current_storage)
static struct program *fuse_cmd_program;
struct fuse_cmd_storage
{
    struct fuse *f;
    struct fuse_cmd *cmd;
};

static void f_fuse_cmd_process( INT32 args )
{
    struct svalue *sp = Pike_sp;
    fuse_process_cmd(THISC->f, THISC->cmd);
    pop_n_elems( Pike_sp-sp );
    push_int(0);
}

static void push_fuse_cmd(struct fuse_cmd *cmd, struct fuse *f)
{
    struct object *o = clone_object( fuse_cmd_program, 0);
    ((struct fuse_cmd_storage *)o->storage)->cmd = cmd;
    ((struct fuse_cmd_storage *)o->storage)->f = f;
    push_object( o );
}

static struct object *global_fuse_obj; // There Can Be Only One

static int our_fuse_loop( struct fuse *f )
{
    if (f == NULL)
        return -1;

    while (1) 
    {
        struct fuse_cmd *cmd;

        if (fuse_exited(f))
	{
            break;
	}
	THREADS_ALLOW();
        cmd = fuse_read_cmd(f);
	THREADS_DISALLOW();
        if (cmd == NULL)
            continue;
	push_fuse_cmd( cmd,f );
        apply( global_fuse_obj, "___process_cmd", 1 );
	pop_stack();
    }
    return 0;
}


#define DEFAULT_ERRNO() do{if(Pike_sp[-1].u.integer) return -Pike_sp[-1].u.integer;return -ENOENT;}while(0)

static int pf_getattr(const char *path, struct stat *stbuf)
{
    extern struct program *stat_program;
    struct stat *st;
    push_text( path );
    apply( global_fuse_obj, "getattr", 1 );
    if( Pike_sp[-1].type != PIKE_T_OBJECT ||
	!(st = (struct stat *)get_storage( Pike_sp[-1].u.object, stat_program)) )
	DEFAULT_ERRNO();
    *stbuf = *st;
    return 0;
}

static int pf_readlink(const char *path, char *buf, size_t size)
{
    int res;
    push_text( path );
    apply( global_fuse_obj, "readlink", 1 );
    
    res = readlink(path, buf, size - 1);
    if(res == -1)
        return -errno;
    if( Pike_sp[-1].type != PIKE_T_STRING )
	DEFAULT_ERRNO();
    if( Pike_sp[-1].u.string->len >= size )
	return -ENAMETOOLONG;
    memcpy( buf, Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len);
    buf[Pike_sp[-1].u.string->len] = '\0';
    return 0;
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
    int type;
    int ino;
    get_all_args( "getdircallback", args, "%s%d%d", &name, &type, &ino );
    THISGD->filler( THISGD->h, name, type, ino );
}

static void push_getdir_callback( fuse_dirh_t h, fuse_dirfil_t filler )
{
    struct object *o=clone_object( getdir_program, 0 );
    ((struct getdir_storage*)o->storage)->h = h;
    ((struct getdir_storage*)o->storage)->filler = filler;
    push_object( o );
}

static int pf_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
    push_text( path );
    push_getdir_callback( h, filler );
    apply( global_fuse_obj, "readdir", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_mknod(const char *path, mode_t mode, dev_t rdev)
{
    push_text( path );
    push_int( mode );
    push_int( rdev );
    apply( global_fuse_obj, "mknod", 3 );
    return -Pike_sp[-1].u.integer;
}

static int pf_mkdir(const char *path, mode_t mode)
{
    push_text( path );
    push_int( mode );
    apply( global_fuse_obj, "mkdir", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_unlink(const char *path)
{
    push_text( path );
    apply( global_fuse_obj, "unlink", 1 );
    return -Pike_sp[-1].u.integer;
}

static int pf_rmdir(const char *path)
{
    push_text( path );
    apply( global_fuse_obj, "rmdir", 1 );
    return -Pike_sp[-1].u.integer;
}

static int pf_symlink(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "symlink", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_rename(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "rename", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_link(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "link", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_chmod(const char *path, mode_t mode)
{
    push_text( path );
    push_int( mode );
    apply( global_fuse_obj, "chmod", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_chown(const char *path, uid_t uid, gid_t gid)
{
    push_text( path );
    push_int( uid );
    push_int( gid );
    apply( global_fuse_obj, "chown", 3 );
    return -Pike_sp[-1].u.integer;
}

static int pf_truncate(const char *path, off_t size)
{
    push_text( path );
    push_int64( size );
    apply( global_fuse_obj, "truncate", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_utime(const char *path, struct utimbuf *buf)
{
    push_text( path );
    push_int( buf->actime );
    push_int( buf->modtime );
    apply( global_fuse_obj, "utime", 3 );
    return -Pike_sp[-1].u.integer;
}


static int pf_open(const char *path, struct fuse_file_info *fi)
{
    push_text( path );
    push_int( fi->flags );
    apply( global_fuse_obj, "open", 2 );
    return -Pike_sp[-1].u.integer;
}

static int pf_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    push_text( path );
    push_int( size );
    push_int64( offset );
    apply( global_fuse_obj, "read", 3 );
    
    if( Pike_sp[-1].type != PIKE_T_STRING )
	DEFAULT_ERRNO();
    memcpy( buf, Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len );
    return Pike_sp[-1].u.string->len;
}

static int pf_write(const char *path, const char *buf, size_t size,
		    off_t offset, struct fuse_file_info *fi)
{
    push_text( path );
    push_string( make_shared_binary_string(buf, size ) );
    push_int64( offset );
    apply( global_fuse_obj, "write", 3 );
    return -Pike_sp[-1].u.integer;
}

static int pf_statfs(const char *path, struct statfs *stbuf)
{
    push_text( path );
    apply( global_fuse_obj, "statfs", 1 );
    if( Pike_sp[-1].type != PIKE_T_MAPPING )
	DEFAULT_ERRNO();
    struct mapping *m = Pike_sp[-1].u.mapping;
    struct svalue *val;
    memset( stbuf, 0, sizeof(*stbuf) );
    stbuf->f_namelen = 4096;
    stbuf->f_bsize = 1024;
#define STSET(X)    if(val=simple_mapping_string_lookup( m,#X )) stbuf->f_##X=val->u.integer;
    STSET(type);
    STSET(bsize);
    STSET(blocks);
    STSET(bfree);
    STSET(bavail);
    STSET(files);
    STSET(ffree);
    STSET(namelen);
#undef STSET
    return 0;
}

static int pf_release(const char *path, struct fuse_file_info *fi)
{
    push_text( path );
    apply( global_fuse_obj, "release", 1 );
    return -Pike_sp[-1].u.integer;
}

static int pf_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi)
{
    push_text( path );
    push_int( isdatasync );
    apply( global_fuse_obj, "fsync", 2 );
    return -Pike_sp[-1].u.integer;
}

//// TODO
// #ifdef HAVE_SETXATTR
// /* xattr operations are optional and can safely be left unimplemented */
// static int xmp_setxattr(const char *path, const char *name, const char *value,
//                         size_t size, int flags)
// {
//     int res = lsetxattr(path, name, value, size, flags);
//     if(res == -1)
//         return -errno;
//     return 0;
// }
// 
// static int xmp_getxattr(const char *path, const char *name, char *value,
//                     size_t size)
// {
//     int res = lgetxattr(path, name, value, size);
//     if(res == -1)
//         return -errno;
//     return res;
// }
// 
// static int xmp_listxattr(const char *path, char *list, size_t size)
// {
//     int res = llistxattr(path, list, size);
//     if(res == -1)
//         return -errno;
//     return res;
// }
// 
// static int xmp_removexattr(const char *path, const char *name)
// {
//     int res = lremovexattr(path, name);
//     if(res == -1)
//         return -errno;
//     return 0;
// }
// #endif /* HAVE_SETXATTR */


static struct fuse_operations pike_fuse_oper = {
    .getattr	= pf_getattr,
    .readlink	= pf_readlink,
    .getdir	= pf_getdir,
    .mknod	= pf_mknod,
    .mkdir	= pf_mkdir,
    .symlink	= pf_symlink,
    .unlink	= pf_unlink,
    .rmdir	= pf_rmdir,
    .rename	= pf_rename,
    .link	= pf_link,
    .chmod	= pf_chmod,
    .chown	= pf_chown,
    .truncate	= pf_truncate,
    .utime	= pf_utime,
    .open	= pf_open,
    .read	= pf_read,
    .write	= pf_write,
    .statfs	= pf_statfs,
    .release	= pf_release,
    .fsync	= pf_fsync,
/* #ifdef HAVE_SETXATTR */
/*     .setxattr	= pf_setxattr, */
/*     .getxattr	= pf_getxattr, */
/*     .listxattr	= pf_listxattr, */
/*     .removexattr= pf_removexattr, */
/* #endif */
};

static void f_fuse_run( INT32 nargs )
{
    struct fuse *fuse;
    char *mountpoint;
    int fd;
    int multithreaded;
    int argc, i;
    char **argv;
    struct array *args;
    if( global_fuse_obj )
	Pike_error("There can be only one.\n"
		   "You have to run multiple processes to have multiple FUSE filesystems\n");
    get_all_args( "run", nargs, "%o%a", &global_fuse_obj, &args );

    argv = malloc( sizeof(char *) * args->size );
    for( i = 0; i<args->size; i++ )
    {
	if( args->item[i].type != PIKE_T_STRING )
	{
	    free( argv );
	    Pike_error("Argument %d is not a string\n", i );
	}
	argv[i] = args->item[i].u.string->str;
    }
    fuse = fuse_setup(args->size, argv, &pike_fuse_oper, sizeof(pike_fuse_oper), 
		      &mountpoint, &multithreaded, &fd);

    free( argv );

    if (fuse == NULL)
	Pike_error("Fuse init failed\n");

    our_fuse_loop( fuse );

    fuse_teardown(fuse, fd, mountpoint);
}

PIKE_MODULE_EXIT
{
}

PIKE_MODULE_INIT
{
    ADD_FUNCTION( "run", f_fuse_run, tFunc(tObj tArr(tStr),tVoid ), 0 );
    start_new_program( );
    {
	ADD_STORAGE( struct getdir_storage );
	ADD_FUNCTION( "`()", f_getdir_callback, tFunc(tStr tInt tInt,tVoid ), 0 );
    }
    getdir_program = end_program();

    start_new_program( );
    {
	ADD_STORAGE( struct fuse_cmd_storage );
	ADD_FUNCTION( "`()", f_fuse_cmd_process, tFunc(tVoid,tVoid), 0 );
    }
    fuse_cmd_program = end_program();
}
#else
PIKE_MODULE_EXIT
{
}

PIKE_MODULE_INIT
{
}
#endif // HAVE_LIBFUSE
