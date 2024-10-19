/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "fdlib.h"
#include "interpret.h"
#include "svalue.h"
#include "bignum.h"
#include "mapping.h"
#include "object.h"
#include "builtin_functions.h"
#include "operators.h"
#include "program_id.h"
#include "file_machine.h"
#include "pike_types.h"
#include "sprintf.h"

#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#define sp Pike_sp
#define fp Pike_fp

/*! @module Stdio
 */
/*! @class Stat
 *!
 *!   This object is used to represent file status information
 *!   from e.g. @[file_stat()].
 *!
 *!   It contains the following items usually found in a C @tt{struct
 *!   stat@}:
 *!   @dl
 *!     @item mode
 *!       File mode (see @tt{mknod(2)@}).
 *!     @item size
 *!       File size in bytes.
 *!     @item uid
 *!       User ID of the file's owner.
 *!     @item gid
 *!       Group ID of the file's owner.
 *!     @item atime
 *!       Time of last access in seconds since 00:00:00 UTC, 1970-01-01.
 *!     @item mtime
 *!       Time of last data modification.
 *!     @item ctime
 *!       Time of last file status change.
 *!     @item atime_nsec
 *!       Time of last access in nanoseconds, added to atime to get
 *!       sub-second time
 *!     @item mtime_nsec
 *!       Time of last modification in nanoseconds, added to mtime to get
 *!       sub-second time
 *!     @item ctime_nsec
 *!       Time of last file status change in nanoseconds, added to ctime to
 *!       get sub-second time
 *!     @item ino
 *!       Inode number.
 *!     @item nlink
 *!       Number of links.
 *!     @item dev
 *!       ID of the device containing a directory entry for this file.
 *!     @item rdev
 *!       ID of the device.
 *!   @enddl
 *!
 *!   It also contains some items that correspond to the C @tt{IS*@} macros:
 *!   @dl
 *!     @item isreg
 *!       Set if the file is a regular file.
 *!     @item isdir
 *!       Set if the file is a directory.
 *!     @item islnk
 *!       Set if the file is a symbolic link. Note that symbolic links
 *!       are normally followed by the stat functions, so this might
 *!       only be set if you turn that off, e.g. by giving a nonzero
 *!       second argument to @[file_stat()].
 *!     @item isfifo
 *!       Set if the file is a FIFO (aka named pipe).
 *!     @item issock
 *!       Set if the file is a socket.
 *!     @item ischr
 *!       Set if the file is a character device.
 *!     @item isblk
 *!       Set if the file is a block device.
 *!   @enddl
 *!
 *!   There are also some items that provide alternative representations
 *!   of the above:
 *!   @dl
 *!     @item type
 *!       The type as a string, can be any of @expr{"reg"@},
 *!       @expr{"dir"@}, @expr{"lnk"@}, @expr{"fifo"@}, @expr{"sock"@},
 *!       @expr{"chr"@}, @expr{"blk"@}, and @expr{"unknown"@}.
 *!     @item mode_string
 *!       The file mode encoded as a string in @tt{ls -l@} style, e.g.
 *!       @expr{"drwxr-xr-x"@}.
 *!   @enddl
 *!
 *!   Note that some items might not exist or have meaningful values
 *!   on some platforms.
 *!
 *!   Additionally, the object may be initialized from or casted to an
 *!   @expr{array@} on the form of a 'traditional' LPC stat-array, and
 *!   it's also possible to index the object directly with integers as
 *!   if it were such an array. The stat-array has this format:
 *!
 *!   @array
 *!     @elem int 0
 *!       File mode, same as @tt{mode@}.
 *!     @elem int 1
 *!       If zero or greater, the file is a regular file and this is
 *!       its size in bytes. If less than zero it gives the type:
 *!       -2=directory, -3=symlink and -4=device.
 *!     @elem int 2
 *!       Time of last access, same as @tt{atime@}.
 *!     @elem int 3
 *!       Time of last data modification, same as @tt{mtime@}.
 *!     @elem int 4
 *!       Time of last file status change, same as @tt{ctime@}.
 *!     @elem int 5
 *!       User ID of the file's owner, same as @tt{uid@}.
 *!     @elem int 6
 *!       Group ID of the file's owner, same as @tt{gid@}.
 *!   @endarray
 *!
 *!   It's possible to modify the stat struct by assigning values to
 *!   the items. They essentially work as variables, although some of
 *!   them affect others, e.g. setting @expr{isdir@} clears @expr{isreg@}
 *!   and setting @expr{mode_string@} changes many of the other items.
 */

/* Let's define these mode flags if they don't exist, so reading and
 * writing the Stat structure will behave identically regardless of
 * OS. */

#ifndef S_IFMT
#define S_IFMT	0xf000
#endif /* !S_IFMT */
#ifndef S_IFREG
#define S_IFREG	0x8000
#endif /* !S_IFREG */
#ifndef S_IFLNK
#define S_IFLNK	0xA000
#endif /* !S_IFLNK */
#ifndef S_IFDIR
#define S_IFDIR	0x4000
#endif /* !S_IFDIR */
#ifndef S_IFCHR
#define S_IFCHR	0x2000
#endif /* !S_IFCHR */
#ifndef S_IFBLK
#define S_IFBLK	0x6000
#endif /* !S_IFBLK */
#ifndef S_IFIFO
#define S_IFIFO	0x1000
#endif /* !S_IFIFO */
#ifndef S_IFSOCK
#define S_IFSOCK 0xC000
#endif /* !S_IFSOCK */

#ifndef S_IRUSR
#define S_IRUSR 0400
#endif /* !S_IRUSR */
#ifndef S_IWUSR
#define S_IWUSR 0200
#endif /* !S_IWUSR */
#ifndef S_IXUSR
#define S_IXUSR 0100
#endif /* !S_IXUSR */
#ifndef S_IRGRP
#define S_IRGRP 040
#endif /* !S_IRGRP */
#ifndef S_IWGRP
#define S_IWGRP 020
#endif /* !S_IWGRP */
#ifndef S_IXGRP
#define S_IXGRP 010
#endif /* !S_IXGRP */
#ifndef S_IROTH
#define S_IROTH 04
#endif /* !S_IROTH */
#ifndef S_IWOTH
#define S_IWOTH 02
#endif /* !S_IWOTH */
#ifndef S_IXOTH
#define S_IXOTH 01
#endif /* !S_IXOTH */

#ifndef S_ISUID
#define S_ISUID 0x800
#endif /* !S_ISUID */
#ifndef S_ISGID
#define S_ISGID 0x400
#endif /* !S_ISGID */
#ifndef S_ISVTX
#define S_ISVTX 0x200
#endif /* !S_ISVTX */

PMOD_EXPORT struct program *stat_program=NULL;

struct stat_storage
{
   /* Note: stat_create assumes there are no refs in here. */
   PIKE_STAT_T s;
};

static struct mapping *stat_map=NULL;

enum stat_query
{STAT_DEV=1, STAT_INO, STAT_MODE, STAT_NLINK, STAT_UID, STAT_GID, STAT_RDEV,
 STAT_SIZE,
#ifdef HAVE_STRUCT_STAT_BLOCKS
 STAT_BLKSIZE, STAT_BLOCKS,
#endif
 STAT_ATIME, STAT_MTIME, STAT_CTIME,
 STAT_ATIME_NSEC,STAT_MTIME_NSEC, STAT_CTIME_NSEC,
/* is... */
 STAT_ISLNK, STAT_ISREG, STAT_ISDIR, STAT_ISCHR,
 STAT_ISBLK, STAT_ISFIFO, STAT_ISSOCK,
/* special */
 STAT_TYPE, STAT_MODE_STRING,
/* end marker */
 STAT_ENUM_END};

static struct pike_string *stat_index_strs[STAT_ENUM_END];

static struct pike_string *str_type_reg, *str_type_dir, *str_type_lnk,
  *str_type_chr, *str_type_blk, *str_type_fifo, *str_type_sock, *str_type_unknown;

#define THIS_STAT ((struct stat_storage*)(Pike_fp->current_storage))

static void stat_index_set (INT32 args);
static void _stat_index_set (INT_TYPE pos, struct svalue *val, int is_int, INT64 intval);

static int stat_compat_set (INT_TYPE pos, INT64 val)
{
  if (pos < 0) pos += 7;
  switch (pos) {
    case 0: THIS_STAT->s.st_mode = (int) val; break;
    case 1:
      if (val >= 0) {
	THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFREG;
        THIS_STAT->s.st_size = (off_t) val;
      }
      else {
	THIS_STAT->s.st_size = 0;
	if (val == -2)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFDIR;
	else if (val == -3)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFLNK;
	else
	  THIS_STAT->s.st_mode = THIS_STAT->s.st_mode & ~S_IFMT;
      }
      break;
    case 2: THIS_STAT->s.st_atime = (time_t) val; break;
    case 3: THIS_STAT->s.st_mtime = (time_t) val; break;
    case 4: THIS_STAT->s.st_ctime = (time_t) val; break;
    case 5: THIS_STAT->s.st_uid = (int) val; break;
    case 6: THIS_STAT->s.st_gid = (int) val; break;
    default: return 0;
  }
  return 1;
}

static void stat_push_compat(INT_TYPE n)
{
   if (n < 0) n += 7;
   switch (n)
   {
      case 0: push_int(THIS_STAT->s.st_mode); break;
      case 1:
	 switch(S_IFMT & THIS_STAT->s.st_mode)
	 {
	    case S_IFREG:
	       push_int64(THIS_STAT->s.st_size);
	       break;

	    case S_IFDIR: push_int(-2); break;
	    case S_IFLNK: push_int(-3); break;
	    default:
               push_int(-4);
	       break;
	 }
	 break;
      case 2: push_int64(THIS_STAT->s.st_atime); break;
      case 3: push_int64(THIS_STAT->s.st_mtime); break;
      case 4: push_int64(THIS_STAT->s.st_ctime); break;
      case 5: push_int(THIS_STAT->s.st_uid); break;
      case 6: push_int(THIS_STAT->s.st_gid); break;
      default:
      {
	 INT32 args=1;
	 SIMPLE_ARG_TYPE_ERROR("`[]",1,"int(0..6)");
      }
   }
}

static void stat_init (struct object *UNUSED(o))
{
  memset (&THIS_STAT->s, 0, sizeof (THIS_STAT->s));
}

/*! @decl void create (void|object|array stat);
 *!
 *! A new @[Stdio.Stat] object can be initialized in two ways:
 *!
 *! @ul
 *!   @item
 *!     @[stat] is an object, typically another @[Stdio.Stat]. The
 *!     stat info is copied from the object by getting the values of
 *!     @expr{mode@}, @expr{size@}, @expr{atime@}, @expr{mtime@},
 *!     @expr{ctime@}, @expr{uid@}, @expr{gid@}, @expr{dev@}, @expr{ino@},
 *!     @expr{nlink@}, and @expr{rdev@}.
 *!   @item
 *!     @[stat] is a seven element array on the 'traditional' LPC
 *!     stat-array form (see the class doc).
 *! @endul
 */
void f_min(INT32 args);
void f_max(INT32 args);

static void _stat_index(INT_TYPE code)
{
  if (!code) {
    /* Fall back to a normal index on this object, in case
     * someone inherited us. */
    struct svalue res;
    object_index_no_free2 (&res, fp->current_object, 0, sp-1);
    pop_stack();
    *sp++ = res;
    return;
  }

  switch (code)
  {
    case STAT_DEV: push_int(THIS_STAT->s.st_dev); break;
    case STAT_INO: push_int(THIS_STAT->s.st_ino); break;
    case STAT_MODE: push_int(THIS_STAT->s.st_mode); break;
    case STAT_NLINK: push_int(THIS_STAT->s.st_nlink); break;
    case STAT_UID: push_int(THIS_STAT->s.st_uid); break;
    case STAT_GID: push_int(THIS_STAT->s.st_gid); break;
    case STAT_RDEV: push_int(THIS_STAT->s.st_rdev); break;
    case STAT_SIZE: push_int64(THIS_STAT->s.st_size); break;
#ifdef HAVE_STRUCT_STAT_BLOCKS
    case STAT_BLKSIZE: push_int(THIS_STAT->s.st_blksize); break;
    case STAT_BLOCKS: push_int(THIS_STAT->s.st_blocks); break;
#endif
    case STAT_ATIME: push_int64(THIS_STAT->s.st_atime); break;
    case STAT_MTIME: push_int64(THIS_STAT->s.st_mtime); break;
    case STAT_CTIME: push_int64(THIS_STAT->s.st_ctime); break;
#ifdef HAVE_STRUCT_STAT_NSEC
    case STAT_ATIME_NSEC: push_int64(THIS_STAT->s.st_atim.tv_nsec); break;
    case STAT_MTIME_NSEC: push_int64(THIS_STAT->s.st_mtim.tv_nsec); break;
    case STAT_CTIME_NSEC: push_int64(THIS_STAT->s.st_ctim.tv_nsec); break;
#else
    case STAT_ATIME_NSEC: case STAT_MTIME_NSEC:    case STAT_CTIME_NSEC:
      push_int(0);
      break;
#endif
    case STAT_ISREG:
      push_int((THIS_STAT->s.st_mode & S_IFMT) == S_IFREG); break;
    case STAT_ISLNK:
      push_int((THIS_STAT->s.st_mode & S_IFMT) == S_IFLNK); break;
    case STAT_ISDIR:
      push_int((THIS_STAT->s.st_mode & S_IFMT) == S_IFDIR); break;
    case STAT_ISCHR:
      push_int((THIS_STAT->s.st_mode & S_IFMT) == S_IFCHR); break;
    case STAT_ISBLK:
      push_int((THIS_STAT->s.st_mode & S_IFMT) == S_IFBLK); break;
    case STAT_ISFIFO:
      push_int((THIS_STAT->s.st_mode & S_IFMT) == S_IFIFO); break;
    case STAT_ISSOCK:
      push_int((THIS_STAT->s.st_mode & S_IFMT) == S_IFSOCK); break;

    case STAT_TYPE:
      switch (THIS_STAT->s.st_mode & S_IFMT)
      {
        case S_IFREG: ref_push_string(str_type_reg); break;
        case S_IFDIR: ref_push_string(str_type_dir); break;
        case S_IFLNK: ref_push_string(str_type_lnk); break;
        case S_IFCHR: ref_push_string(str_type_chr); break;
        case S_IFBLK: ref_push_string(str_type_blk); break;
        case S_IFIFO: ref_push_string(str_type_fifo); break;
        case S_IFSOCK: ref_push_string(str_type_sock); break;
        default: ref_push_string(str_type_unknown); break;
      }
      break;

    case STAT_MODE_STRING:
      switch (THIS_STAT->s.st_mode & S_IFMT)
      {
        case S_IFREG:
          push_static_text("-");
          break;
        case S_IFDIR:
          push_static_text("d");
          break;
        case S_IFLNK:
          push_static_text("l");
          break;
        case S_IFCHR:
          push_static_text("c");
          break;
        case S_IFBLK:
          push_static_text("b");
          break;
        case S_IFIFO:
          push_static_text("f");
          break;
        case S_IFSOCK:
          push_static_text("s");
          break;
        default:
          push_static_text("?");
          break;
      }

      if ( (THIS_STAT->s.st_mode & S_IRUSR) )
        push_static_text("r");
      else
        push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_IWUSR) )
        push_static_text("w");
      else
        push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_ISUID) )
        if ( (THIS_STAT->s.st_mode & S_IXUSR) )
          push_static_text("s");
        else
          push_static_text("S");
      else
        if ( (THIS_STAT->s.st_mode & S_IXUSR) )
          push_static_text("x");
        else
          push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_IRGRP) )
        push_static_text("r");
      else
        push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_IWGRP) )
        push_static_text("w");
      else
        push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_ISGID) )
        if ( (THIS_STAT->s.st_mode & S_IXGRP) )
          push_static_text("s");
        else
          push_static_text("S");
      else
        if ( (THIS_STAT->s.st_mode & S_IXGRP) )
          push_static_text("x");
        else
          push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_IROTH) )
        push_static_text("r");
      else
        push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_IWOTH) )
        push_static_text("w");
      else
        push_static_text("-");

      if ( (THIS_STAT->s.st_mode & S_ISVTX) )
        if ( (THIS_STAT->s.st_mode & S_IXOTH) )
          push_static_text("t");
        else
          push_static_text("T");
      else
        if ( (THIS_STAT->s.st_mode & S_IXOTH) )
          push_static_text("x");
        else
          push_static_text("-");

      f_add(10);
      break;
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("stat_index is not kept up-to-date with stat_map.\n");
#endif
  }
}

static void stat_index(INT32 args)
{
  if( !args )
    SIMPLE_WRONG_NUM_ARGS_ERROR("Stat `[]",1);
  else if( args == 1 )
  {
    if (TYPEOF(sp[-1]) == T_INT)
    {
      int index = sp[-1].u.integer;
      pop_stack();
      stat_push_compat(index);
    }
    else if( TYPEOF(sp[-1]) == T_STRING )
    {
      struct svalue *tmp;
      tmp = low_mapping_string_lookup( stat_map, sp[-1].u.string );
      _stat_index( tmp ? tmp->u.integer : 0 );
    }
    else
      SIMPLE_ARG_TYPE_ERROR("`[]",1,"int(0..6)|string");
  }
  else if (args>=2) /* range */
  {
    INT_TYPE from, to, n=0;

    if (args > 2) {
      pop_n_elems(args - 2);
      args = 2;
    }

    if (TYPEOF(sp[-2]) != T_INT &&
        !(TYPEOF(sp[-2]) == T_OBJECT && is_bignum_object (sp[-2].u.object)))
      SIMPLE_ARG_TYPE_ERROR("`[..]",1,"int");

    if (TYPEOF(sp[-1]) != T_INT &&
        !(TYPEOF(sp[-1]) == T_OBJECT && is_bignum_object (sp[-1].u.object)))
      SIMPLE_ARG_TYPE_ERROR("`[..]",2,"int");

    /* make in range 0..6 */
    push_int(6);
    f_min(2);
    stack_swap();
    push_int(0);
    f_max(2);
    stack_swap();

    from = sp[-2].u.integer;
    to = sp[-1].u.integer;

    pop_n_elems(args);

    while (from<=to)
    {
      stat_push_compat(from++);
      n++;
    }
    f_aggregate(n);
  }
}


static void _stat_index_set (INT_TYPE code, struct svalue *val, int got_int_val, INT64 int_val )
{
#define BAD_ARG_2(X) bad_arg_error("`[]=", 2,2,X,val,msg_bad_arg,2,"`[]=",X)

  if( got_int_val == -1 )
  {
    if (TYPEOF(*val) == T_INT)
      int_val = val->u.integer, got_int_val = 1;
    else if (TYPEOF(*val) == T_OBJECT && is_bignum_object (val->u.object))
    {
      if (!int64_from_bignum (&int_val, val->u.object))
        Pike_error ("`[]=: Too big integer as value.\n");
      else
        got_int_val = 1;
    }
  }

  switch (code) {
    case STAT_MODE_STRING:
      if (TYPEOF(*val) != T_STRING)
        BAD_ARG_2("string");

      /* FIXME: Handle modes on the form u+rw, perhaps? */

      if (val->u.string->len != 10)
        BAD_ARG_2("mode string with 10 chars");

      {
        PCHARP str = MKPCHARP_STR (val->u.string);
        int mode = THIS_STAT->s.st_mode;

        switch (INDEX_PCHARP (str, 0)) {
          case '-': mode = (mode & ~S_IFMT) | S_IFREG; break;
          case 'd': mode = (mode & ~S_IFMT) | S_IFDIR; break;
          case 'l': mode = (mode & ~S_IFMT) | S_IFLNK; break;
          case 'c': mode = (mode & ~S_IFMT) | S_IFCHR; break;
          case 'b': mode = (mode & ~S_IFMT) | S_IFBLK; break;
          case 'f': mode = (mode & ~S_IFMT) | S_IFIFO; break;
          case 's': mode = (mode & ~S_IFMT) | S_IFSOCK; break;
        }

        switch (INDEX_PCHARP (str, 1)) {
          case '-': mode &= ~S_IRUSR; break;
          case 'r': mode |= S_IRUSR; break;
        }
        switch (INDEX_PCHARP (str, 2)) {
          case '-': mode &= ~S_IWUSR; break;
          case 'w': mode |= S_IWUSR; break;
        }
        switch (INDEX_PCHARP (str, 3)) {
          case '-': mode &= ~(S_IXUSR | S_ISUID); break;
          case 'x': mode = (mode & ~S_ISUID) | S_IXUSR; break;
          case 'S': mode = (mode & ~S_IXUSR) | S_ISUID; break;
          case 's': mode |= S_IXUSR | S_ISUID; break;
        }

        switch (INDEX_PCHARP (str, 4)) {
          case '-': mode &= ~S_IRGRP; break;
          case 'r': mode |= S_IRGRP; break;
        }
        switch (INDEX_PCHARP (str, 5)) {
          case '-': mode &= ~S_IWGRP; break;
          case 'w': mode |= S_IWGRP; break;
        }
        switch (INDEX_PCHARP (str, 6)) {
          case '-': mode &= ~(S_IXGRP | S_ISGID); break;
          case 'x': mode = (mode & ~S_ISGID) | S_IXGRP; break;
          case 'S': mode = (mode & ~S_IXGRP) | S_ISGID; break;
          case 's': mode |= S_IXGRP | S_ISGID; break;
        }

        switch (INDEX_PCHARP (str, 7)) {
          case '-': mode &= ~S_IROTH; break;
          case 'r': mode |= S_IROTH; break;
        }
        switch (INDEX_PCHARP (str, 8)) {
          case '-': mode &= ~S_IWOTH; break;
          case 'w': mode |= S_IWOTH; break;
        }
        switch (INDEX_PCHARP (str, 9)) {
          case '-': mode &= ~(S_IXOTH | S_ISVTX); break;
          case 'x': mode = (mode & ~S_ISVTX) | S_IXOTH; break;
          case 'T': mode = (mode & ~S_IXOTH) | S_ISVTX; break;
          case 't': mode |= S_IXOTH | S_ISVTX; break;
        }

        THIS_STAT->s.st_mode = mode;
      }
      break;

    case STAT_TYPE:
      if (TYPEOF(*val) != T_STRING)
        BAD_ARG_2("string");

      if (val->u.string == str_type_reg)
        THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFREG;
      else if (val->u.string == str_type_dir)
        THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFDIR;
      else if (val->u.string == str_type_lnk)
        THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFLNK;
      else if (val->u.string == str_type_chr)
        THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFCHR;
      else if (val->u.string == str_type_blk)
        THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFBLK;
      else if (val->u.string == str_type_fifo)
        THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFIFO;
      else if (val->u.string == str_type_sock)
        THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFSOCK;
      else if (val->u.string == str_type_unknown)
        THIS_STAT->s.st_mode = THIS_STAT->s.st_mode & ~S_IFMT;
      else
        BAD_ARG_2("valid type string");
      break;

    case STAT_ISREG:
      THIS_STAT->s.st_mode &= ~S_IFMT;
      if (!UNSAFE_IS_ZERO (val)) THIS_STAT->s.st_mode |= S_IFREG;
      break;
    case STAT_ISDIR:
      THIS_STAT->s.st_mode &= ~S_IFMT;
      if (!UNSAFE_IS_ZERO (val)) THIS_STAT->s.st_mode |= S_IFDIR;
      break;
    case STAT_ISLNK:
      THIS_STAT->s.st_mode &= ~S_IFMT;
      if (!UNSAFE_IS_ZERO (val)) THIS_STAT->s.st_mode |= S_IFLNK;
      break;
    case STAT_ISCHR:
      THIS_STAT->s.st_mode &= ~S_IFMT;
      if (!UNSAFE_IS_ZERO (val)) THIS_STAT->s.st_mode |= S_IFCHR;
      break;
    case STAT_ISBLK:
      THIS_STAT->s.st_mode &= ~S_IFMT;
      if (!UNSAFE_IS_ZERO (val)) THIS_STAT->s.st_mode |= S_IFBLK;
      break;
    case STAT_ISFIFO:
      THIS_STAT->s.st_mode &= ~S_IFMT;
      if (!UNSAFE_IS_ZERO (val)) THIS_STAT->s.st_mode |= S_IFIFO;
      break;
    case STAT_ISSOCK:
      THIS_STAT->s.st_mode &= ~S_IFMT;
      if (!UNSAFE_IS_ZERO (val)) THIS_STAT->s.st_mode |= S_IFSOCK;
      break;

    default:
      if (!got_int_val)
        BAD_ARG_2("integer");

      switch (code) {
        case STAT_DEV: THIS_STAT->s.st_dev = (int) int_val; break;
        case STAT_INO: THIS_STAT->s.st_ino = (int) int_val; break;
        case STAT_MODE: THIS_STAT->s.st_mode = (int) int_val; break;
        case STAT_NLINK: THIS_STAT->s.st_nlink = (int) int_val; break;
        case STAT_UID: THIS_STAT->s.st_uid = (int) int_val; break;
        case STAT_GID: THIS_STAT->s.st_gid = (int) int_val; break;
        case STAT_RDEV: THIS_STAT->s.st_rdev = (int) int_val; break;
        case STAT_SIZE: THIS_STAT->s.st_size = (off_t) int_val; break;
#ifdef HAVE_STRUCT_STAT_BLOCKS
        case STAT_BLKSIZE: THIS_STAT->s.st_blksize = int_val; break;
        case STAT_BLOCKS: THIS_STAT->s.st_blocks = int_val; break;
#endif
        case STAT_ATIME: THIS_STAT->s.st_atime = (time_t) int_val; break;
        case STAT_MTIME: THIS_STAT->s.st_mtime = (time_t) int_val; break;
        case STAT_CTIME: THIS_STAT->s.st_ctime = (time_t) int_val; break;

#ifdef HAVE_STRUCT_STAT_NSEC
        case STAT_ATIME_NSEC: THIS_STAT->s.st_atim.tv_nsec = (time_t) int_val; break;
        case STAT_MTIME_NSEC: THIS_STAT->s.st_mtim.tv_nsec = (time_t) int_val; break;
        case STAT_CTIME_NSEC: THIS_STAT->s.st_ctim.tv_nsec = (time_t) int_val; break;
#endif
      }
  }
}

static void stat_index_set (INT32 args)
{
  int got_int_val = 0;
  INT64 int_val = 0;

  if (args < 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("Stat `[]=", 2);

  if (args > 2) {
    pop_n_elems (args - 2);
    args = 2;
  }

  if (TYPEOF(sp[-1]) == T_INT)
    int_val = sp[-1].u.integer, got_int_val = 1;
  else if (TYPEOF(sp[-1]) == T_OBJECT && is_bignum_object (sp[-1].u.object))
  {
    if (!int64_from_bignum (&int_val, sp[-1].u.object))
      Pike_error ("Stat `[]=: Too big integer as value.\n");
    else
      got_int_val = 1;
  }

  /* shouldn't there be an else clause here ? */
  /* No, the second argument is checked further below depending on
   * what the first is. /mast */

  if (TYPEOF(sp[-2]) == T_INT) {
    if (!got_int_val)
      SIMPLE_ARG_TYPE_ERROR ("`[]=", 2,
                             "integer when the first argument is an integer");
    if (!stat_compat_set (sp[-2].u.integer, int_val))
      SIMPLE_ARG_TYPE_ERROR ("`[]=", 1, "int(0..6)|string");
  }
  else if (TYPEOF(sp[-2]) == T_STRING) {
    struct svalue *tmp;
    tmp = low_mapping_string_lookup( stat_map, sp[-2].u.string );
    if (!tmp)
    {
      /* Fall back to a normal index set on this object, in case
       * someone inherited us. */
      object_set_index2 (fp->current_object, 0, sp-2, sp-1);
      stack_swap();
      pop_stack();
      return;
    }
    _stat_index_set( tmp->u.integer, sp-1, got_int_val, int_val);
  }

  else SIMPLE_ARG_TYPE_ERROR ("`[]=", 1, "int(0..6)|string");

  stack_swap();
  pop_stack();
}

static void stat_indices(INT32 args);
static void stat_values(INT32 args);

/*! @decl mapping(string:int)|array cast (string to);
 *!
 *! Convert the stat object to a mapping or array.
 */
static void stat_cast(INT32 args)
{
  struct pike_string *type;

  if (args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("Stat cast",1);
  if (TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("cast",1,"string");

  type = Pike_sp[-args].u.string;
  pop_stack(); /* type have at least one more reference. */

  if (type == literal_array_string)
  {
    push_int(0);
    push_int(6);
    stat_index(2);
  }
  else if (type == literal_mapping_string)
  {
    stat_indices(0);
    stat_values(0);
    push_mapping(mkmapping(Pike_sp[-2].u.array, Pike_sp[-1].u.array));
    stack_pop_n_elems_keep_top(2);
  }
  else
    push_undefined();
}

/*! @decl int(0..1) _equal(mixed other)
 *!
 *! Compare this object with another @[Stat] object.
 *!
 *! @returns
 *!   Returns @expr{1@} if @[other] is a @[Stat] object with the
 *!   same content, and @expr{0@} (zero) otherwise.
 */
static void stat__equal(INT32 args)
{
  struct stat_storage *this_st = THIS_STAT;
  struct stat_storage *other_st;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("_equal", 1);

  if ((TYPEOF(Pike_sp[-args]) != T_OBJECT) ||
      !(other_st = get_storage(Pike_sp[-1].u.object, stat_program))) {
    push_int(0);
    return;
  }

  if (this_st == other_st) {
    push_int(1);
    return;
  }

  push_int(!memcmp(this_st, other_st, sizeof(struct stat_storage)));
}

static void stat__sprintf(INT32 args)
{
   int x;

   if (args<1)
      SIMPLE_WRONG_NUM_ARGS_ERROR("_sprintf",2);

   if (TYPEOF(sp[-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR("_sprintf",0,"integer");

   x=sp[-args].u.integer;
   pop_n_elems(args);
   switch (x)
   {
      case 'O':
        push_static_text("Stat(%s %db)");
        _stat_index(STAT_MODE_STRING);
        _stat_index(STAT_SIZE);
        f_sprintf(3);
        return;
      default:
	 push_int(0);
	 return;
   }
}

static void stat_indices(INT32 args)
{
   pop_n_elems(args);
   ref_push_mapping(stat_map);
   f_indices(1);
}

static void stat_values(INT32 args)
{
   int i,z;
   struct array *a;

   pop_n_elems(args);
   ref_push_mapping(stat_map);
   f_indices(1);
   z=(a=sp[-1].u.array)->size;
   for (i=0; i<z; i++)
   {
      ref_push_object(fp->current_object);
      push_svalue(a->item+i);
      f_index(2);
   }
   f_aggregate(z);
   stack_pop_keep_top();
}

#ifdef HAVE_PRAGMA_GCC_OPTIMIZE
/* Without this gcc inlines all the function calls to _index_set etc below.

   It's really rather pointless. And we really need a better way to
   avoid this than adding a lot of -Os around the code. :)
*/
#pragma GCC optimize "-Os"
#endif

static void stat_create (INT32 args)
{
  if (args >= 1 && !UNSAFE_IS_ZERO (Pike_sp - 1)) {
    pop_n_elems (args - 1);
    args = 1;

    if (TYPEOF(sp[-1]) == T_OBJECT)
      if (sp[-1].u.object->prog == stat_program) {
	*THIS_STAT = *(struct stat_storage *) sp[-1].u.object->storage;
	pop_stack();
	return;
      }

    if ((1 << TYPEOF(sp[-1])) & (BIT_PROGRAM|BIT_OBJECT|BIT_MAPPING))
    {
      struct keypair *k;
      int e;
      NEW_MAPPING_LOOP( stat_map->data )
      {
        push_svalue(&k->ind);
        SET_SVAL_SUBTYPE(sp[-1],1);
        o_index();
        if(!IS_UNDEFINED(sp-1)) {
          _stat_index_set( k->val.u.integer, sp-1, -1,0 );
        }
        pop_stack();
      }
    }
    else if (TYPEOF(sp[-1]) == T_ARRAY)
    {
      struct array *a = sp[-1].u.array;
      int i;
      if (a->size != 7)
	SIMPLE_ARG_TYPE_ERROR ("create", 1, "stat array with 7 elements");
      for (i = 0; i < 7; i++) {
	INT64 val;
	if (TYPEOF(ITEM(a)[i]) == T_INT)
	  val = ITEM(a)[i].u.integer;
	else if (TYPEOF(ITEM(a)[i]) == T_OBJECT &&
		 is_bignum_object (ITEM(a)[i].u.object)) {
	  if (!int64_from_bignum (&val, ITEM(a)[i].u.object))
	    Pike_error ("create: Too big integer in stat array.\n");
	}
	else
	  SIMPLE_ARG_TYPE_ERROR ("create", 1, "array(int)");
	stat_compat_set (i, val);
      }
    }

    else
      SIMPLE_ARG_TYPE_ERROR ("create", 1, "void|Stdio.Stat|array(int)");
  }

  pop_n_elems (args);
}
#undef THIS_STAT

void push_stat(PIKE_STAT_T *s)
{
   struct object *o;
   struct stat_storage *stor;
   o=clone_object(stat_program,0);
   stor=get_storage(o,stat_program);
   stor->s=*s;
   push_object(o);
}


/*! @endclass
 */
/*! @endmodule
 */

/* ---------------------------------------------------------------- */

void init_stdio_stat(void)
{
   unsigned int n=0;

   static const struct {
     const char *name;
     const INT_TYPE id;
   } __indices[] = {
     {"dev",STAT_DEV},
     {"ino",STAT_INO},
     {"mode",STAT_MODE},
     {"nlink",STAT_NLINK},
     {"uid",STAT_UID},
     {"gid",STAT_GID},
     {"rdev",STAT_RDEV},
     {"size",STAT_SIZE},
#ifdef HAVE_STRUCT_STAT_BLOCKS
     {"blksize",STAT_BLKSIZE},
     {"blocks",STAT_BLOCKS},
#endif
     {"atime",STAT_ATIME},
     {"mtime",STAT_MTIME},
     {"ctime",STAT_CTIME},

     {"atime_nsec",STAT_ATIME_NSEC},
     {"mtime_nsec",STAT_MTIME_NSEC},
     {"ctime_nsec",STAT_CTIME_NSEC},

     {"islnk",STAT_ISLNK},
     {"isreg",STAT_ISREG},
     {"isdir",STAT_ISDIR},
     {"ischr",STAT_ISCHR},
     {"isblk",STAT_ISBLK},
     {"isfifo",STAT_ISFIFO},
     {"issock",STAT_ISSOCK},

     {"type",STAT_TYPE},
     {"mode_string",STAT_MODE_STRING}
   };

   str_type_reg = make_shared_string("reg");
   str_type_dir = make_shared_string("dir");
   str_type_lnk = make_shared_string("lnk");
   str_type_chr = make_shared_string("chr");
   str_type_blk = make_shared_string("blk");
   str_type_fifo = make_shared_string("fifo");
   str_type_sock = make_shared_string( "sock");
   str_type_unknown = make_shared_string( "unknown");

   stat_map=allocate_mapping(1);
   push_int(0);
   for( n=0; n<sizeof(__indices)/sizeof(__indices[0]); n++ )
   {
     struct pike_string *s = make_shared_string(__indices[n].name);
     stat_index_strs[__indices[n].id]=s;
     sp[-1].u.integer = __indices[n].id;
     mapping_string_insert( stat_map, s, sp-1);
   }
   sp--;
   dmalloc_touch_svalue(sp);

   START_NEW_PROGRAM_ID(STDIO_STAT);
   ADD_STORAGE(struct stat_storage);

   ADD_FUNCTION ("create", stat_create,
		 tFunc(tOr5(tVoid,tObjImpl_STDIO_STAT,tPrg(tObj),tMapping,tArr(tInt)),
		       tVoid), ID_PROTECTED);

   ADD_FUNCTION("`[]",stat_index,
		tOr(tFunc(tOr(tStr,tInt06),tOr3(tStr,tInt,tFunction)),
		    tFunc(tInt tInt,tArr(tInt))),0);
   ADD_FUNCTION("`->",stat_index,
		tOr(tFunc(tOr(tStr,tInt06),tOr3(tStr,tInt,tFunction)),
		    tFunc(tInt tInt,tArr(tInt))),0);

   ADD_FUNCTION ("`[]=", stat_index_set,
		 tOr(tFunc(tInt06 tSetvar(0,tInt),tVar(0)),
		     tFunc(tString tSetvar(1,tOr(tInt,tString)),tVar(1))), 0);
   ADD_FUNCTION ("`->=", stat_index_set,
		 tOr(tFunc(tInt06 tSetvar(0,tInt),tVar(0)),
		     tFunc(tString tSetvar(1,tOr(tInt,tString)),tVar(1))), 0);

   ADD_FUNCTION("cast",stat_cast,tFunc(tStr,tOr(tMapping,tArray)),ID_PROTECTED);
   ADD_FUNCTION("_equal", stat__equal, tFunc(tMix,tInt01), ID_PROTECTED);
   ADD_FUNCTION("_sprintf",stat__sprintf,
		tFunc(tInt tOr(tVoid,tMapping),tString),0);
   ADD_FUNCTION("_indices",stat_indices,
		tFunc(tNone,tArr(tOr(tString,tInt))),0);
   ADD_FUNCTION("_values",stat_values,
		tFunc(tNone,tArr(tOr(tString,tInt))),0);

   set_init_callback (stat_init);

   stat_program=end_program();
   add_program_constant("Stat",stat_program,0);
}

void exit_stdio_stat(void)
{
   size_t i;

   free_program(stat_program);
   free_mapping(stat_map);

   for (i = 1; i < STAT_ENUM_END; i++)
     free_string (stat_index_strs[i]);

   free_string (str_type_reg);
   free_string (str_type_dir);
   free_string (str_type_lnk);
   free_string (str_type_chr);
   free_string (str_type_blk);
   free_string (str_type_fifo);
   free_string (str_type_sock);
   free_string (str_type_unknown);
}
