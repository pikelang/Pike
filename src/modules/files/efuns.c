/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
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
#include "security.h"
#include "bignum.h"

#include "file_machine.h"
#include "file.h"

RCSID("$Id: efuns.c,v 1.98 2004/10/04 08:25:20 grubba Exp $");

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <signal.h>
#include <errno.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#ifdef HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
#   define dirent direct
#   define NAMLEN(dirent) (dirent)->d_namlen
# else /* !HAVE_SYS_NDIR_H */
#  ifdef HAVE_SYS_DIR_H
#   include <sys/dir.h>
#   define dirent direct
#   define NAMLEN(dirent) (dirent)->d_namlen
#  else /* !HAVE_SYS_DIR_H */
#   ifdef HAVE_NDIR_H
#    include <ndir.h>
#    define dirent direct
#    define NAMLEN(dirent) (dirent)->d_namlen
#   else /* !HAVE_NDIR_H */
#    ifdef HAVE_DIRECT_H
#     include <direct.h>
#     define NAMLEN(dirent) strlen((dirent)->d_name)
#    endif /* HAVE_DIRECT_H */
#   endif /* HAVE_NDIR_H */
#  endif /* HAVE_SYS_DIR_H */
# endif /* HAVE_SYS_NDIR_H */
#endif /* HAVE_DIRENT_H */

#include "dmalloc.h"

#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

/* #define DEBUG_FILE */
/* #define READDIR_DEBUG */

#ifdef __NT__

/* Dynamic load of functions that doesn't exist in all Windows versions. */

static HINSTANCE kernel32lib;

#define LINKFUNC(RET,NAME,TYPE) \
  typedef RET (WINAPI * PIKE_CONCAT(NAME,type)) TYPE ; \
  static PIKE_CONCAT(NAME,type) NAME

LINKFUNC(BOOL, movefileex, (
  LPCTSTR lpExistingFileName,  // file name
  LPCTSTR lpNewFileName,       // new file name
  DWORD dwFlags                // move options
));

#endif

struct array *encode_stat(struct stat *s)
{
  struct array *a;
  a=allocate_array(7);
  ITEM(a)[0].u.integer=s->st_mode;
  switch(S_IFMT & s->st_mode)
  {
  case S_IFREG:
    push_int64((INT64)s->st_size);
    ITEM(a)[1] = *(--sp);
    break;
    
  case S_IFDIR: ITEM(a)[1].u.integer=-2; break;
#ifdef S_IFLNK
  case S_IFLNK: ITEM(a)[1].u.integer=-3; break;
#endif
  default:
#ifdef DEBUG_FILE
    fprintf(stderr, "encode_stat(): mode:%ld\n", (long)S_IFMT & s->st_mode);
#endif /* DEBUG_FILE */
    ITEM(a)[1].u.integer=-4;
    break;
  }
  ITEM(a)[2].u.integer=s->st_atime;
  ITEM(a)[3].u.integer=s->st_mtime;
  ITEM(a)[4].u.integer=s->st_ctime;
  ITEM(a)[5].u.integer=s->st_uid;
  ITEM(a)[6].u.integer=s->st_gid;
  return a;
}

void f_file_stat(INT32 args)
{
  struct stat st;
  int i, l;
  struct pike_string *str;
  
  if(args<1)
    Pike_error("Too few arguments to file_stat()\n");
  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to file_stat()\n");

  VALID_FILE_IO("file_stat","read");

  str = sp[-args].u.string;
  l = (args>1 && !IS_ZERO(sp+1-args))?1:0;

  if (strlen(str->str) != (size_t)str->len) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  THREADS_ALLOW_UID();
#ifdef HAVE_LSTAT
  if(l)
    i=fd_lstat(str->str, &st);
  else
#endif
    i=fd_stat(str->str, &st);

  THREADS_DISALLOW_UID();
  pop_n_elems(args);
  if(i==-1)
  {
    push_int(0);
  }else{
    push_stat(&st);
  }
}

void f_file_truncate(INT32 args)
{
#ifdef HAVE_LSEEK64
  long long len;
#else
  INT32 len;
#endif
  struct pike_string *str;
  int res;

  if(args<1 || sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to file_truncate(string filename,int length).\n");
  if(args<2 || sp[1-args].type != T_INT)
    Pike_error("Bad argument 2 to file_truncate(string filename,int length).\n");

  str = sp[-args].u.string;
  len = sp[1-args].u.integer;

  if (strlen(str->str) != (size_t)str->len) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  VALID_FILE_IO("file_truncate","write");

#ifdef __NT__
  {
    HANDLE h = CreateFile(str->str, GENERIC_WRITE,
			  FILE_SHARE_READ|FILE_SHARE_WRITE,
			  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h == DO_NOT_WARN(INVALID_HANDLE_VALUE)) {
      errno = GetLastError();
      res=-1;
    } else {
      if(SetFilePointer(h, len, NULL, FILE_BEGIN) != 0xffffffff &&
	 SetEndOfFile(h))
	res=0;
      else {
	errno = GetLastError();
	res=-1;
      }
      CloseHandle(h);
    }
  }
#else  /* !__NT__ */
  res=truncate(str->str, len);
#endif /* __NT__ */

  pop_n_elems(args);

  push_int(!res);
}

#ifdef __NT__

void f_filesystem_stat( INT32 args )
{
  char *path;
  DWORD sectors_per_cluster = -1;
  DWORD bytes_per_sector = -1;
  DWORD free_clusters = -1;
  DWORD total_clusters = -1;
  char _p[4];
  char *p = _p;
  unsigned int free_sectors;
  unsigned int total_sectors;

  VALID_FILE_IO("filesystem_stat","read");

  get_all_args( "filesystem_stat", args, "%s", &path );

  if(sp[-1].u.string->len < 2 || path[1] != ':')
  {
    p = 0;
  } else {
    p[0] = path[0];
    p[1] = ':';
    p[2] = '\\';
    p[3] = 0;
  }
  
  if(!GetDiskFreeSpace( p, &sectors_per_cluster, 
			&bytes_per_sector,
			&free_clusters, 
			&total_clusters ))
  {
    pop_n_elems(args);
    push_int( 0 );
    return;
  }

  free_sectors = sectors_per_cluster  * free_clusters;
  total_sectors = sectors_per_cluster * total_clusters;
  
  pop_n_elems( args );
  push_text("blocksize");
  push_int(bytes_per_sector);
  push_text("blocks");
  push_int(total_sectors);
  push_text("bfree");
  push_int(free_sectors);
  push_text("bavail");
  push_int(free_sectors);
  f_aggregate_mapping( 8 );
}

#else /* !__NT__ */

#if  !defined(HAVE_STRUCT_STATFS) && !defined(HAVE_STRUCT_FS_DATA)
#undef HAVE_STATFS
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
    /* Should not be reached */
#error No struct to hold statfs() data.
#endif /* HAVE_STRUCT_FS_DATA */
#endif /* HAVE_STRUCT_STATFS */
#else /* !HAVE_STATFS */
#ifdef HAVE_USTAT
  struct stat statbuf;
  struct ustat st;
#else /* !HAVE_USTAT */
  /* Should not be reached */
#error No stat function for filesystems.
#endif /* HAVE_USTAT */
#endif /* HAVE_STATFS */
#endif /* HAVE_STATVFS */
  int i;
  struct pike_string *str;

  if(args<1)
    Pike_error("Too few arguments to filesystem_stat()\n");
  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to filesystem_stat()\n");

  str = sp[-args].u.string;

  if (strlen(str->str) != (size_t)str->len) {
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
  /* Should not be reached */
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
    int num_fields = 8;
#ifdef HAVE_STATVFS
#if 0
    push_text("id");         push_int(st.f_fsid);
#else
    num_fields--;
#endif
    push_text("blocksize");  push_int(st.f_frsize);
    push_text("blocks");     push_int(st.f_blocks);
    push_text("bfree");      push_int(st.f_bfree);
    push_text("bavail");     push_int(st.f_bavail);
    push_text("files");      push_int(st.f_files);
    push_text("ffree");      push_int(st.f_ffree);
    push_text("favail");     push_int(st.f_favail);
#ifdef HAVE_STATVFS_F_FSTR
    push_text("fsname");     push_text(st.f_fstr);
    num_fields++;
#endif /* HAVE_STATVFS_F_FSTR */
#ifdef HAVE_STATVFS_F_BASETYPE
    push_text("fstype");     push_text(st.f_basetype);
    num_fields++;
#endif /* HAVE_STATVFS_F_BASETYPE */
    f_aggregate_mapping(num_fields*2);
#else /* !HAVE_STATVFS */
#ifdef HAVE_STATFS
#ifdef HAVE_STRUCT_STATFS
#if 0 && HAVE_STATFS_F_FSID
    push_text("id");           push_int(st.f_fsid);
#else
    num_fields--;
#endif
    push_text("blocksize");    push_int(st.f_bsize);
    push_text("blocks");       push_int(st.f_blocks);
    push_text("bfree");        push_int(st.f_bfree);
    push_text("files");        push_int(st.f_files);
    push_text("ffree");        push_int(st.f_ffree);
    push_text("favail");       push_int(st.f_ffree);
#ifdef HAVE_STATFS_F_BAVAIL
    push_text("bavail");       push_int(st.f_bavail);
    f_aggregate_mapping((num_fields-1)*2);
#else
    f_aggregate_mapping((num_fields-2)*2);
#endif /* HAVE_STATFS_F_BAVAIL */
#else /* !HAVE_STRUCT_STATFS */
#ifdef HAVE_STRUCT_FS_DATA
    /* ULTRIX */
    push_text("blocksize");    push_int(st.fd_bsize);
    push_text("blocks");       push_int(st.fd_btot);
    push_text("bfree");        push_int(st.fd_bfree);
    push_text("bavail");       push_int(st.fd_bfreen);
    f_aggregate_mapping(4*2);
#else /* !HAVE_STRUCT_FS_DATA */
    /* Should not be reached */
#error No struct to hold statfs() data.
#endif /* HAVE_STRUCT_FS_DATA */
#endif /* HAVE_STRUCT_STATFS */
#else /* !HAVE_STATFS */
#ifdef HAVE_USTAT
    push_text("bfree");      push_int(st.f_tfree);
    push_text("ffree");      push_int(st.f_tinode);
    push_text("fsname");     push_text(st.f_fname);
    f_aggregate_mapping(3*2);
#else
    /* Should not be reached */
#error No stat function for filesystems.
#endif /* HAVE_USTAT */
#endif /* HAVE_STATFS */
#endif /* HAVE_STATVFS */
  }
}
  
#endif /* HAVE_STATVFS || HAVE_STATFS || HAVE_USTAT */
#endif /* __NT__ */

void f_werror(INT32 args)
{
  if(!args)
    Pike_error("Too few arguments to werror.\n");
  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to werror().\n");

  if(args> 1)
  {
    f_sprintf(args);
    args=1;
  }

  VALID_FILE_IO("werror","werror");

  write_to_stderr(sp[-args].u.string->str, sp[-args].u.string->len);
  pop_n_elems(args);
}

void f_rm(INT32 args)
{
  struct stat st;
  INT32 i;
  struct pike_string *str;

  destruct_objects_to_destruct();

  if(!args)
    Pike_error("Too few arguments to rm()\n");

  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to rm().\n");

  VALID_FILE_IO("rm","write");

  str = sp[-args].u.string;
  
  if (strlen(str->str) != (size_t)str->len) {
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
      i=rmdir(str->str) != -1;
    }else{
      i=unlink(str->str) != -1;
    }
#ifdef __NT__
    /* NT looks at the permissions on the file itself and refuses to
     * remove files we don't have write access to. Thus we chmod it
     * and try again, to make rm() more unix-like. */
    if (!i && errno == EACCES && !(st.st_mode & _S_IWRITE)) {
      if (chmod(str->str, st.st_mode | _S_IWRITE) == -1)
	errno = EACCES;
      else {
	if(S_IFDIR == (S_IFMT & st.st_mode))
	{
	  i=rmdir(str->str) != -1;
	}else{
	  i=unlink(str->str) != -1;
	}
	if (!i) {		/* Failed anyway; try to restore the old mode. */
	  int olderrno = errno;
	  chmod(str->str, st.st_mode);
	  errno = olderrno;
	}
      }
    }
#endif
  }
  THREADS_DISALLOW_UID();
      
  pop_n_elems(args);
  push_int(i);
}

void f_mkdir(INT32 args)
{
  struct pike_string *str;
  char *s, *s_dup;
  int mode;
  int i;

  if(!args)
    Pike_error("Too few arguments to mkdir()\n");

  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to mkdir().\n");

  mode = 0777;			/* &'ed with ~umask anyway. */

  if(args > 1)
  {
    if(sp[1-args].type != T_INT)
      Pike_error("Bad argument 2 to mkdir.\n");

    mode = sp[1-args].u.integer;
  }

  VALID_FILE_IO("mkdir","write");

  str = sp[-args].u.string;

  if (strlen(str->str) != (size_t)str->len) {
    /* Filenames with NUL are not supported. */
    errno = EINVAL;
    pop_n_elems(args);
    push_int(0);
    return;
  }

#if MKDIR_ARGS == 2
  /* Remove trailing / which is not accepted by all mkdir()
     implementations (e.g. Mac OS X) */
  s = str->str;
  s_dup = 0;
  if (s[strlen(s) - 1] == '/') {
    if (s_dup = strdup(s)) {
      s = s_dup;
      s[strlen(s) - 1] = '\0';
    }
  }
  
  THREADS_ALLOW_UID();
  i = mkdir(s, mode) != -1;
  THREADS_DISALLOW_UID();
  
  if (s_dup)
    free(s_dup);
#else

#ifdef HAVE_LSTAT
#define LSTAT lstat
#else
#define LSTAT stat
#endif

  {
    /* Most OS's should have MKDIR_ARGS == 2 nowadays fortunately. */
    int mask = umask(0);
    THREADS_ALLOW_UID();
    i = mkdir(str->str) != -1;
    umask(mask);
    if (i) {
      /* Attempt to set the mode.
       *
       * This code needs to be as paranoid as possible.
       */
      struct stat statbuf1;
      struct stat statbuf2;
      i = LSTAT(str->str, &statbuf1) != -1;
      if (i) {
	i = ((statbuf1.st_mode & S_IFMT) == S_IFDIR);
      }
      if (i) {
	mode = ((mode & 0777) | (statbuf1.st_mode & ~0777)) & ~mask;
	do {
	  i = chmod(str->str, mode) != -1;
	} while (!i && (errno == EINTR));
      }
      if (i) {
	i = LSTAT(str->str, &statbuf2) != -1;
      }
      if (i) {
	i = (statbuf2.st_mode == mode) && (statbuf1.st_ino == statbuf2.st_ino);
	if (!i) {
	  errno = EPERM;
	}
      }
      if (!i) {
	rmdir(str->str);
      }
    }
    THREADS_DISALLOW_UID();
  }
#endif
  pop_n_elems(args);
  push_int(i);
}

#undef HAVE_READDIR_R
#if defined(HAVE_SOLARIS_READDIR_R) || defined(HAVE_HPUX_READDIR_R) || \
    defined(HAVE_POSIX_READDIR_R)
#define HAVE_READDIR_R
#endif

void f_get_dir(INT32 args)
{
  struct svalue *save_sp=sp;
  DIR *dir;
  struct dirent *d;
  struct array *a=0;
  struct pike_string *str;

  VALID_FILE_IO("get_dir","read");

  get_all_args("get_dir",args,"%S",&str);

  if (strlen(str->str) != (size_t)str->len) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

#if defined(_REENTRANT) && defined(HAVE_READDIR_R)
  THREADS_ALLOW_UID();
  dir = opendir(str->str);
  THREADS_DISALLOW_UID();
  if(dir)
  {
#define FPR 1024
    char buffer[MAXPATHLEN * 4];
    char *ptrs[FPR];
    ptrdiff_t lens[FPR];
    struct dirent *tmp;

    if (!(tmp =
#if defined(HAVE_SOLARIS_READDIR_R) || defined(_PC_NAME_MAX)
	  malloc(sizeof(struct dirent) + 
		 ((pathconf(str->str, _PC_NAME_MAX) < 1024)?1024:
		  pathconf(str->str, _PC_NAME_MAX)) + 1)
#else
#ifndef NAME_MAX
#define NAME_MAX 1024
#endif
	  malloc(sizeof(struct dirent) + NAME_MAX+ 1024 + 1)
#endif /* HAVE_SOLARIS_READDIR_R */
      )) {
      closedir(dir);
      Pike_error("get_dir(): Out of memory\n");
    }

    while(1)
    {
      int e;
      int num_files=0;
      char *bufptr=buffer;
      int err = 0;

      THREADS_ALLOW();

      while(1)
      {
#if defined(HAVE_SOLARIS_READDIR_R)
	/* Solaris readdir_r returns the second arg on success,
	 * and returns NULL on error or at end of dir.
	 */
	errno=0;
	do {
	  d=readdir_r(dir, tmp);
	} while ((!d) && ((errno == EAGAIN)||(errno == EINTR)));
	if (!d) {
	  /* Solaris readdir_r seems to set errno to ENOENT sometimes.
	   */
	  if (errno == ENOENT) {
	    err = 0;
	  } else {
	    err = errno;
	  }
	  break;
	}
#elif defined(HAVE_HPUX_READDIR_R)
	/* HPUX's readdir_r returns an int instead:
	 *
	 *  0	- Successfull operation.
	 * -1	- End of directory or encountered an error (sets errno).
	 */
	errno=0;
	if (readdir_r(dir, tmp)) {
	  d = NULL;
	  err = errno;
	  break;
	} else {
	  d = tmp;
	}
#elif defined(HAVE_POSIX_READDIR_R)
	/* POSIX readdir_r returns 0 on success, and ERRNO on failure.
	 * at end of dir it sets the third arg to NULL.
	 */
	d = NULL;
	errno = 0;
	if ((err = readdir_r(dir, tmp, &d)) || !d) {
#ifdef READDIR_DEBUG
	  fprintf(stderr, "POSIX readdir_r(\"%s\") => err %d\n",
		  str->str, err);
	  fprintf(stderr, "POSIX readdir_r(), d= 0x%08x\n",
		  (unsigned int)d);
#endif /* READDIR_DEBUG */
	  if (err == -1) {
	    /* Solaris readdir_r returns -1, and sets errno. */
	    err = errno;
	  }
#ifdef READDIR_DEBUG
	  fprintf(stderr, "POSIX readdir_r(\"%s\") => errno %d\n",
		  str->str, err);
#endif /* READDIR_DEBUG */
	  /* Solaris readdir_r seems to set errno to ENOENT sometimes.
	   *
	   * AIX readdir_r seems to set errno to EBADF at end of dir.
	   */
	  if ((err == ENOENT) || (err == EBADF)) {
	    err = 0;
	  }
	  break;
	}
#ifdef READDIR_DEBUG
	fprintf(stderr, "POSIX readdir_r(\"%s\") => \"%s\"\n",
		str->str, d->d_name);
#endif /* READDIR_DEBUG */
#else
#error Unknown readdir_r variant
#endif
	if(d->d_name[0]=='.')
	{
	  if(!d->d_name[1]) continue;
	  if(d->d_name[1]=='.' && !d->d_name[2]) continue;
	}
	if(num_files >= FPR) break;
	lens[num_files]=NAMLEN(d);
	if(bufptr+lens[num_files] >= buffer+sizeof(buffer)) break;
	MEMCPY(bufptr, d->d_name, lens[num_files]);
	ptrs[num_files]=bufptr;
	bufptr+=lens[num_files];
	num_files++;
      }
      THREADS_DISALLOW();
      if ((!d) && err) {
	Pike_error("get_dir(): readdir_r(\"%s\") failed: %d\n", str->str, err);
      }
      for(e=0;e<num_files;e++)
      {
	push_string(make_shared_binary_string(ptrs[e],lens[e]));
      }
      if(d)
	push_string(make_shared_binary_string(d->d_name,NAMLEN(d)));
      else
	break;
    }
    THREADS_ALLOW();
    free(tmp);
    closedir(dir);
    THREADS_DISALLOW();
    a = aggregate_array(DO_NOT_WARN((INT32)(sp - save_sp)));
  }
#else
  dir = opendir(str->str);
  if(dir)
  {
    for(d=readdir(dir); d; d=readdir(dir))
    {
      if(d->d_name[0]=='.')
      {
	if(!d->d_name[1]) continue;
	if(d->d_name[1]=='.' && !d->d_name[2]) continue;
      }
      push_string(make_shared_binary_string(d->d_name, NAMLEN(d)));
    }
    closedir(dir);
    a=aggregate_array(sp-save_sp);
  }
#endif

  pop_n_elems(args);
  if(a)
    push_array(a);
  else
    push_int(0);
}

void f_cd(INT32 args)
{
  INT32 i;
  struct pike_string *str;

  if(!args)
    Pike_error("Too few arguments to cd()\n");

  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to cd()\n");

  VALID_FILE_IO("cd","status");

  str = sp[-args].u.string;

  if (strlen(str->str) != (size_t)str->len) {
    /* Filenames with NUL are not supported. */
    errno = ENOENT;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  i = chdir(str->str) != -1;
  pop_n_elems(args);
  push_int(i);
}

void f_getcwd(INT32 args)
{
  char *e;
  char *tmp;
#if defined(HAVE_WORKING_GETCWD) || !defined(HAVE_GETWD)
  INT32 size;

  size=1000;
  do {
    tmp=(char *)xalloc(size);
    e = getcwd(tmp,1000); 
    if (e || errno!=ERANGE) break;
    free(tmp);
    tmp=0;
    size*=2;
  } while (size < 10000);
#else
#ifndef MAXPATHLEN
#define MAXPATHLEN 32768
#endif
  tmp=xalloc(MAXPATHLEN+1);
  THREADS_ALLOW_UID();
  e = getwd(tmp);
  THREADS_DISALLOW_UID();
#endif
  if(!e) {
    if (tmp) 
      free(tmp);
    Pike_error("Failed to fetch current path.\n");
  }

  pop_n_elems(args);
  push_string(make_shared_string(e));
  free(e);
}

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
    Pike_error("Too few arguments to exece().\n");

#ifdef PIKE_SECURITY
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
    Pike_error("exece: permission denied.\n");
#endif


  e=0;
  en=0;
  switch(args)
  {
  default:
    if(sp[2-args].type != T_MAPPING)
      Pike_error("Bad argument 3 to exece().\n");
    en=sp[2-args].u.mapping;
    mapping_fix_type_field(en);

    if(m_ind_types(en) & ~BIT_STRING)
      Pike_error("Bad argument 3 to exece().\n");
    if(m_val_types(en) & ~BIT_STRING)
      Pike_error("Bad argument 3 to exece().\n");

  case 2:
    if(sp[1-args].type != T_ARRAY)
      Pike_error("Bad argument 2 to exece().\n");
    array_fix_type_field(sp[1-args].u.array);

    if(sp[1-args].u.array->type_field & ~BIT_STRING)
      Pike_error("Bad argument 2 to exece().\n");

  case 1:
    if(sp[0-args].type != T_STRING)
      Pike_error("Bad argument 1 to exece().\n");
  }

  argv=(char **)xalloc((2+sp[1-args].u.array->size) * sizeof(char *));
  
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
    INT32 e;
    struct array *i,*v;

    env=(char **)xalloc((1+m_sizeof(en)) * sizeof(char *));

    i=mapping_indices(en);
    v=mapping_values(en);
    
    for(e=0;e<i->size;e++)
    {
      push_string(ITEM(i)[e].u.string);
      push_string(make_shared_string("="));
      push_string(ITEM(v)[e].u.string);
      f_add(3);
      env[e]=sp[-1].u.string->str;
      sp--;
      dmalloc_touch_svalue(sp);
    }
      
    free_array(i);
    free_array(v);
    env[e]=0;
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

void f_mv(INT32 args)
{
  INT32 i;
  struct pike_string *str1;
  struct pike_string *str2;
#ifdef __NT__
  int orig_errno = errno;
  int err;
  struct stat st;
#endif

  if(args<2)
    Pike_error("Too few arguments to mv()\n");

  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to mv().\n");

  if(sp[-args+1].type != T_STRING)
    Pike_error("Bad argument 2 to mv().\n");

  VALID_FILE_IO("mv","write");

  str1 = sp[-args].u.string;
  str2 = sp[1-args].u.string;

  if ((strlen(str1->str) != (size_t)str1->len) ||
      (strlen(str2->str) != (size_t)str2->len)) {
    /* Filenames with NUL are not supported. */
    if (strlen(str1->str) != (size_t)str1->len) {
      errno = ENOENT;
    } else {
      errno = EINVAL;
    }
    pop_n_elems(args);
    push_int(0);
    return;
  }

#ifndef __NT__
  i=rename(str1->str, str2->str);
#else

  /* Emulate the behavior of unix rename(2) when the destination exists. */

  if (movefileex) {
    if (MoveFileEx (str1->str, str2->str, 0)) {
      i = 0;
      goto no_nt_rename_kludge;
    }
    if ((i = GetLastError()) != ERROR_ALREADY_EXISTS)
      goto no_nt_rename_kludge;
  }
  else {
    /* Fall back to rename() for W98 and earlier. Unlike MoveFileEx,
     * it can't move directories between directories. */
    if (!rename (str1->str, str2->str)) {
      i = 0;
      goto no_nt_rename_kludge;
    }
    if ((i = errno) != EEXIST)
      goto no_nt_rename_kludge;
  }

#ifdef HAVE_LSTAT
  if (fd_lstat (str2->str, &st)) goto no_nt_rename_kludge;
#else
  if (fd_stat (str2->str, &st)) goto no_nt_rename_kludge;
#endif

  if ((st.st_mode & S_IFMT) != S_IFDIR && movefileex) {
    /* MoveFileEx can overwrite a file but not a dir. */
    if (!(st.st_mode & _S_IWRITE))
      /* Like in f_rm, we got to handle the case that NT looks on the
       * permissions on the file itself before removing it. */
      if (chmod (str2->str, st.st_mode | _S_IWRITE))
	goto no_nt_rename_kludge;
    if (movefileex (str1->str, str2->str, MOVEFILE_REPLACE_EXISTING))
      i = 0;			/* Success. */
    else
      chmod (str2->str, st.st_mode);
  }

  else {
    char *s = malloc (str2->len + 2), *p;
    if (!s) {
      i = movefileex ? ERROR_NOT_ENOUGH_MEMORY : ENOMEM;
      goto no_nt_rename_kludge;
    }
    memcpy (s, str2->str, str2->len);
    p = s + str2->len;
    p[2] = 0;

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
      /* Check first that the target is empty if it's a directory, so
       * that we won't even bother trying with the stunt below. */
      WIN32_FIND_DATA *dir = malloc (sizeof (WIN32_FIND_DATA));
      HANDLE h;
      if (!dir) {
	i = movefileex ? ERROR_NOT_ENOUGH_MEMORY : ENOMEM;
	goto nt_rename_kludge_end;
      }
      p[0] = '/', p[1] = '*';
      h = FindFirstFile (s, dir);
      if (h != INVALID_HANDLE_VALUE) {
	do {
	  if (dir->cFileName[0] != '.' ||
	      dir->cFileName[1] &&
	      (dir->cFileName[1] != '.' || dir->cFileName[2])) {
	    /* Target dir not empty. */
	    FindClose (h);
	    free (dir);
	    goto nt_rename_kludge_end;
	  }
	} while (FindNextFile (h, dir));
	FindClose (h);
      }
      free (dir);
    }

    /* Move away the target temporarily to do the move, then cleanup
     * or undo depending on how it went. */
    for (p[0] = 'A'; p[0] != 'Z'; p[0]++)
      for (p[1] = 'A'; p[1] != 'Z'; p[1]++) {
	if (movefileex) {
	  if (!movefileex (str2->str, s, 0))
	    if (GetLastError() == ERROR_ALREADY_EXISTS) continue;
	    else goto nt_rename_kludge_end;
	}
	else
	  if (rename (str2->str, s))
	    if (errno == EEXIST) continue;
	    else goto nt_rename_kludge_end;

	if (movefileex ?
	    movefileex (str1->str, str2->str, MOVEFILE_REPLACE_EXISTING) :
	    !rename (str1->str, str2->str)) {
	  if (!(st.st_mode & _S_IWRITE))
	    if (chmod (s, st.st_mode | _S_IWRITE))
	      goto nt_rename_kludge_fail;
	  if ((st.st_mode & S_IFMT) == S_IFDIR)
	    err = rmdir (s);
	  else
	    err = unlink (s);
	  if (!err) {		/* Success. */
	    i = 0;
	    goto nt_rename_kludge_end;
	  }

	nt_rename_kludge_fail:
	  /* Couldn't remove the old target, perhaps the directory
	   * grew files. */
	  if (!(st.st_mode & _S_IWRITE))
	    chmod (s, st.st_mode);
	  if (movefileex ?
	      !movefileex (str2->str, str1->str, MOVEFILE_REPLACE_EXISTING) :
	      rename (str2->str, str1->str)) {
	    /* Old target left behind but the rename is still
	     * successful, so we claim success anyway in lack of
	     * better error reporting capabilities. */
	    i = 0;
	    goto nt_rename_kludge_end;
	  }
	}

	rename (s, str2->str);
	goto nt_rename_kludge_end;
      }

  nt_rename_kludge_end:
    free (s);
  }

no_nt_rename_kludge:
  if (i) {
    if (movefileex)
      switch (i) {
	/* Try to translate NT errors to errno codes that NT's rename()
	 * would return. */
	case ERROR_INVALID_NAME:
	  errno = EINVAL;
	  break;
	case ERROR_NOT_ENOUGH_MEMORY:
	  errno = ENOMEM;
	  break;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	  errno = ENOENT;
	  break;
	case ERROR_ALREADY_EXISTS:
	  errno = EEXIST;
	  break;
	default:
	  errno = EACCES;
      }
  }
  else errno = orig_errno;
#endif /* __NT__ */

  pop_n_elems(args);
  push_int(!i);
}

void f_strerror(INT32 args)
{
  char *s;
  int err;

  if(!args) 
    Pike_error("Too few arguments to strerror()\n");
  if(sp[-args].type != T_INT)
    Pike_error("Bad argument 1 to strerror()\n");

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
    push_constant_text("Error ");
    push_int(err);
    f_add(2);
  }
}

void f_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(errno);
}

void init_files_efuns(void)
{
  set_close_on_exec(0,1);
  set_close_on_exec(1,1);
  set_close_on_exec(2,1);

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

/* function(string,int|void:int *) */
  ADD_EFUN("file_stat",f_file_stat,tFunc(tStr tOr(tInt,tVoid),tObj), OPT_EXTERNAL_DEPEND);

  ADD_EFUN("file_truncate",f_file_truncate,tFunc(tStr tInt,tInt),0);


#if defined(HAVE_STATVFS) || defined(HAVE_STATFS) || defined(HAVE_USTAT) || defined(__NT__)
  
/* function(string:mapping(string:string|int)) */
  ADD_EFUN("filesystem_stat", f_filesystem_stat,tFunc(tStr,tMap(tStr,tOr(tStr,tInt))), OPT_EXTERNAL_DEPEND);
#endif /* HAVE_STATVFS || HAVE_STATFS */
  
/* function(:int) */
  ADD_EFUN("errno",f_errno,tFunc(tNone,tInt),OPT_EXTERNAL_DEPEND);
  
/* function(string,void|mixed...:void) */
  ADD_EFUN("werror",f_werror,tFuncV(tStr,tOr(tVoid,tMix),tVoid),OPT_SIDE_EFFECT);
  
/* function(string:int) */
  ADD_EFUN("rm",f_rm,tFunc(tStr,tInt),OPT_SIDE_EFFECT);
  
/* function(string,void|int:int) */
  ADD_EFUN("mkdir",f_mkdir,tFunc(tStr tOr(tVoid,tInt),tInt),OPT_SIDE_EFFECT);
  
/* function(string,string:int) */
  ADD_EFUN("mv", f_mv,tFunc(tStr tStr,tInt), OPT_SIDE_EFFECT);
  
/* function(string:string *) */
  ADD_EFUN("get_dir",f_get_dir,tFunc(tStr,tArr(tStr)),OPT_EXTERNAL_DEPEND);
  
/* function(string:int) */
  ADD_EFUN("cd",f_cd,tFunc(tStr,tInt),OPT_SIDE_EFFECT);
  
/* function(:string) */
  ADD_EFUN("getcwd",f_getcwd,tFunc(tNone,tStr),OPT_EXTERNAL_DEPEND);
  
/* function(string,mixed*,void|mapping(string:string):int) */
  ADD_EFUN("exece",f_exece,tFunc(tStr tArr(tMix) tOr(tVoid,tMap(tStr,tStr)),tInt),OPT_SIDE_EFFECT); 

/* function(int:string) */
  ADD_EFUN("strerror",f_strerror,tFunc(tInt,tStr),0);
}

void exit_files_efuns(void)
{
#ifdef __NT__
  if (kernel32lib) {
    if (FreeLibrary (kernel32lib))
      movefileex = 0;
    kernel32lib = 0;
  }
#endif
}
