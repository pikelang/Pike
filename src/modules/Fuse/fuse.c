/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: fuse.c,v 1.10 2006/12/29 16:26:15 grubba Exp $
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
#include "backend.h"

#ifdef HAVE_LIBFUSE
#define FUSE_USE_VERSION 26
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
    if( (Pike_sp[-1].type != PIKE_T_STRING) ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    if( Pike_sp[-1].u.string->len >= (int)size )
	return -ENAMETOOLONG;
    memcpy( buf, Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len+1);
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
    get_all_args( "getdircallback", args, "%s", &name );
    THISGD->filler( THISGD->h, name, 0, 0 );
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
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_mknod(const char *path, mode_t mode, dev_t rdev)
{
    push_text( path );
    push_int( mode );
    push_int( rdev );
    apply( global_fuse_obj, "mknod", 3 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_mkdir(const char *path, mode_t mode)
{
    push_text( path );
    push_int( mode );
    apply( global_fuse_obj, "mkdir", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_unlink(const char *path)
{
    push_text( path );
    apply( global_fuse_obj, "unlink", 1 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_rmdir(const char *path)
{
    push_text( path );
    apply( global_fuse_obj, "rmdir", 1 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_symlink(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "symlink", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_rename(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "rename", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_link(const char *from, const char *to)
{
    push_text( from );
    push_text( to );
    apply( global_fuse_obj, "link", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_chmod(const char *path, mode_t mode)
{
    push_text( path );
    push_int( mode );
    apply( global_fuse_obj, "chmod", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_chown(const char *path, uid_t uid, gid_t gid)
{
    push_text( path );
    push_int( uid );
    push_int( gid );
    apply( global_fuse_obj, "chown", 3 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_truncate(const char *path, off_t size)
{
    push_text( path );
    push_int64( size );
    apply( global_fuse_obj, "truncate", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_utime(const char *path, struct utimbuf *buf)
{
    push_text( path );
    push_int( buf->actime );
    push_int( buf->modtime );
    apply( global_fuse_obj, "utime", 3 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}


static int pf_open(const char *path, struct fuse_file_info *fi)
{
    push_text( path );
    push_int( fi->flags );
    apply( global_fuse_obj, "open", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    push_text( path );
    push_int( size );
    push_int64( offset );
    apply( global_fuse_obj, "read", 3 );
    
    if( (Pike_sp[-1].type != PIKE_T_STRING) ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    if (((size_t)Pike_sp[-1].u.string->len) > size) {
	return -ENAMETOOLONG;
    }
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
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_statfs(const char *path, struct statvfs *stbuf)
{
    push_text( path );
    apply( global_fuse_obj, "statvfs", 1 );
    if( Pike_sp[-1].type != PIKE_T_MAPPING )
	DEFAULT_ERRNO();
    struct mapping *m = Pike_sp[-1].u.mapping;
    struct svalue *val;
    memset( stbuf, 0, sizeof(*stbuf) );
    stbuf->f_namemax = 4096;
    stbuf->f_bsize = 1024;
#define STSET(X)    do {				\
      if ((val=simple_mapping_string_lookup( m,#X )) &&	\
	  (val->type == T_INT))				\
	stbuf->f_##X=val->u.integer;			\
    } while(0)
    STSET(bsize);
    STSET(blocks);
    STSET(bfree);
    STSET(bavail);
    STSET(files);
    STSET(ffree);
    if (((val=simple_mapping_string_lookup( m,"namemax" )) &&
	 (val->type == T_INT)) ||
	/* namelen is compat. */
	((val=simple_mapping_string_lookup( m,"namelen" )) &&
	 (val->type == T_INT))) {
      stbuf->f_namemax = val->u.integer;
    }
#undef STSET
    return 0;
}

static int pf_release(const char *path, struct fuse_file_info *fi)
{
    push_text( path );
    apply( global_fuse_obj, "release", 1 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi)
{
    push_text( path );
    push_int( isdatasync );
    apply( global_fuse_obj, "fsync", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

static int pf_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
    push_text( path );
    push_text( name );
    push_string( make_shared_binary_string( value, size ) );
    push_int( flags );
    apply( global_fuse_obj, "setxattr", 4 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}
 
static int pf_getxattr(const char *path, const char *name, char *value,
		       size_t size)
{
    push_text( path );
    push_text( name );
    apply( global_fuse_obj, "getxattr", 2 );
    if( Pike_sp[-1].type != PIKE_T_STRING ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    unsigned int ds = Pike_sp[-1].u.string->len <<Pike_sp[-1].u.string->size_shift;
    if( !size )
	return ds;
    if( size < ds )
	return -ERANGE;
    memcpy( value, Pike_sp[-1].u.string->str, ds );
    return ds;
}

static int pf_listxattr(const char *path, char *list, size_t size)
{
    push_text( path );
    apply( global_fuse_obj, "listxattr", 1 );
    if( Pike_sp[-1].type != PIKE_T_ARRAY )
	DEFAULT_ERRNO();
    push_string( make_shared_binary_string( "\0", 1 ) );
    o_multiply();
    if( Pike_sp[-1].type != PIKE_T_STRING ||
	(Pike_sp[-1].u.string->size_shift) )
	DEFAULT_ERRNO();
    unsigned int ds = Pike_sp[-1].u.string->len <<Pike_sp[-1].u.string->size_shift;
    if( !size )
	return ds;
    if( size < ds )
	return -ERANGE;
    memcpy( list, Pike_sp[-1].u.string->str, ds );
    return ds;
}

static int pf_removexattr(const char *path, const char *name)
{
    push_text( path );
    push_text( name );
    apply( global_fuse_obj, "removexattr", 2 );
    if (Pike_sp[-1].type != T_INT)
	DEFAULT_ERRNO();
    return -Pike_sp[-1].u.integer;
}

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
    .setxattr	= pf_setxattr,
    .getxattr	= pf_getxattr,
    .listxattr	= pf_listxattr,
    .removexattr= pf_removexattr,
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
	if( args->item[i].type != PIKE_T_STRING ||
	    string_has_null(args->item[i].u.string) )
	{
	    free( argv );
	    Pike_error("Argument %d is not a nonbinary string\n", i );
	}
	argv[i] = args->item[i].u.string->str;
    }
    fuse = fuse_setup(args->size, argv, &pike_fuse_oper, sizeof(pike_fuse_oper), 
		      &mountpoint, &multithreaded, &fd);

    free( argv );

    if (fuse == NULL)
	Pike_error("Fuse init failed\n");

    our_fuse_loop( fuse );
    global_fuse_obj=NULL;
    fuse_teardown(fuse, mountpoint);
}

struct new_fuse_storage
{
  struct fd_callback_box box;
  char *mountpoint;
  int multithreaded;
  struct fuse *fuse;
};

#define THIS_FUSE	((struct new_fuse_storage *)(Pike_fp->current_storage))

static void init_new_fuse_storage(struct object *o)
{
  THIS_FUSE->box.fd = -1;
  THIS_FUSE->mountpoint = NULL;
  THIS_FUSE->multithreaded = 0;
  THIS_FUSE->fuse = NULL;
}

static void exit_new_fuse_storage(struct object *o)
{
  if (THIS_FUSE->fuse) {
    fuse_teardown(THIS_FUSE->fuse, THIS_FUSE->mountpoint);
  }
  init_new_fuse_storage(o);
}

static struct fuse_operations pike_new_fuse_oper = {
#if 0
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
    .setxattr	= pf_setxattr,
    .getxattr	= pf_getxattr,
    .listxattr	= pf_listxattr,
    .removexattr= pf_removexattr,
#else /* !0 */
    0,
#endif /* 0 */
};

static void f_new_fuse_setup(INT32 args)
{
  int i;
  char **argv;
  struct array *argvector;
  ONERROR tmp;

  if (THIS_FUSE->fuse) {
    Pike_error("This fuse has already been setup.\n");
  }
  get_all_args("setup", args, "%a", &argvector);

  argv = xalloc(sizeof(char *) * argvector->size);
  SET_ONERROR(tmp, free, argv);
  for (i=0; i < argvector->size; i++) {
    if ((argvector->item[i].type != PIKE_T_STRING) ||
	(argvector->item[i].u.string->size_shift)) {
      SIMPLE_BAD_ARG_ERROR("setup", 1, "array(string(8bit))");
    }
    argv[i] = argvector->item[i].u.string->str;
  }
  THIS_FUSE->fuse = fuse_setup(argvector->size, argv,
			       &pike_new_fuse_oper, sizeof(pike_new_fuse_oper),
			       &THIS_FUSE->mountpoint,
			       &THIS_FUSE->multithreaded,
			       &THIS_FUSE->box.fd);
  free(argv);
  /* FIXME: Do we need to save args for later? (eg mountpoint) */

  if (!THIS_FUSE->fuse) {
    Pike_error("Failed to setup fuse.\n");
  }
}

static void f_new_fuse_run( INT32 args )
{
  struct fuse *fuse;
  if (!THIS_FUSE->fuse) {
    Pike_error("This fuse has not been initialized yet.\n");
  }
  get_all_args( "run", args, "");

  while ((fuse = THIS_FUSE->fuse) && !fuse_exited(fuse)) 
  {
    struct fuse_cmd *cmd;

    THREADS_ALLOW();
    /* FIXME: Race-condition! */
    cmd = fuse_read_cmd(fuse);
    THREADS_DISALLOW();
    if (cmd) {
      fuse_process_cmd(fuse, cmd);
    }
  }
  pop_n_elems(args);
  push_int(0);
}

PIKE_MODULE_EXIT
{
  free_program( getdir_program );
  free_program( fuse_cmd_program );
}

PIKE_MODULE_INIT
{
  start_new_program();
  {
    ADD_STORAGE(struct new_fuse_storage);
    set_init_callback(init_new_fuse_storage);
    set_exit_callback(exit_new_fuse_storage);

    ADD_FUNCTION("setup", f_new_fuse_run, tFunc(tArr(tStr), tVoid), 0);
    ADD_FUNCTION("run", f_new_fuse_run, tFunc(tNone, tVoid), 0);

#if 0
#define FOO(NAME, TYPE)				\
    PIKE_CONCAT3(f_, NAME, _num) =		\
      ADD_FUNCTION(TOSTR(NAME), NULL, TYPE, 0)

    FOO(getattr, tFunc(tStr, tOr(tObjImpl_STDIO_STAT, tInt1Plus)));
    FOO(readlink, tFunc(tStr, tOr(tStr, tIntPlus)));
    FOO(readdir, tFunc(tStr tFunc(tStr, tVoid), tInt));
    FOO(mknod, tFunc(tStr tInt tInt, tInt));
    FOO(mkdir, tFunc(tStr tInt, tInt));
    FOO(symlink, tFunc(tStr tStr, tInt));
    FOO(unlink, tFunc(tStr, tInt));
    FOO(rmdir, tFunc(tStr, tInt));
    FOO(rename, tFunc(tStr tStr, tInt));
    FOO(link, tFunc(tStr tStr, tInt));
    FOO(chmod, tFunc(tStr tInt, tInt));
    FOO(chown, tFunc(tStr tInt tInt, tInt));
    FOO(truncate, tFunc(tStr tInt, tInt));
    FOO(utime, tFunc(tStr tInt tInt, tInt));
    FOO(open, tFunc(tStr tInt, tInt));
    FOO(read, tFunc(tStr tInt tInt, tInt));
    FOO(write, tFunc(tStr tStr tInt, tInt));
    FOO(statfs, tFunc(tStr, tMap(tStr, tInt)));
    FOO(release, tFunc(tStr, tInt));
    FOO(fsync, tFunc(tStr tInt, tInt));
    FOO(setxattr, tFunc(tStr tStr tStr tInt, tStr));
    FOO(getxattr, tFunc(tStr tStr tStr tInt, tInt));
    FOO(listxattr, tFunc(tStr, tOr(tArr(tStr), tInt)));
    FOO(removexattr, tFunc(tStr tStr, tInt));
#endif /* 0 */
  }
  end_class("Fuse", 0);
    ADD_FUNCTION( "run", f_fuse_run, tFunc(tObj tArr(tStr),tVoid ), 0 );
    start_new_program( );
    {
	ADD_STORAGE( struct getdir_storage );
	ADD_FUNCTION( "`()", f_getdir_callback, tFunc(tStr tOr(tInt,tVoid) tOr(tInt,tVoid),tVoid ), 0 );
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
  if(!TEST_COMPAT(7,6))
    HIDE_MODULE();
}
#endif /* HAVE_LIBFUSE */
