/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: stat.c,v 1.30 2004/09/18 20:50:57 nilsson Exp $
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

struct program *stat_program=NULL;

struct stat_storage
{
   /* Note: stat_create assumes there are no refs in here. */
   PIKE_STAT_T s;
};

static struct mapping *stat_map=NULL;

enum stat_query
{STAT_DEV=1, STAT_INO, STAT_MODE, STAT_NLINK, STAT_UID, STAT_GID, STAT_RDEV, 
 STAT_SIZE,
#if 0
 STAT_BLKSIZE, STAT_BLOCKS,
#endif
 STAT_ATIME, STAT_MTIME, STAT_CTIME,
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

static int stat_compat_set (INT_TYPE pos, INT64 val)
{
  if (pos < 0) pos += 7;
  switch (pos) {
    case 0: DO_NOT_WARN(THIS_STAT->s.st_mode = val); break;
    case 1:
      if (val >= 0) {
	THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFREG;
	DO_NOT_WARN(THIS_STAT->s.st_size = val);
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
    case 2: DO_NOT_WARN(THIS_STAT->s.st_atime = val); break;
    case 3: DO_NOT_WARN(THIS_STAT->s.st_mtime = val); break;
    case 4: DO_NOT_WARN(THIS_STAT->s.st_ctime = val); break;
    case 5: DO_NOT_WARN(THIS_STAT->s.st_uid = val); break;
    case 6: DO_NOT_WARN(THIS_STAT->s.st_gid = val); break;
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
#ifdef DEBUG_FILE
	       fprintf(stderr, "encode_stat(): mode:%ld\n", 
		       (long)S_IFMT & THIS_STAT->s.st_mode);
#endif /* DEBUG_FILE */
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
	 SIMPLE_BAD_ARG_ERROR("Stat `[]",1,"int(0..6)");
      }
   }
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
static void stat_create (INT32 args)
{
  if (args >= 1) {
    pop_n_elems (args - 1);
    args = 1;

    if (sp[-1].type == T_OBJECT)
      if (sp[-1].u.object->prog == stat_program) {
	*THIS_STAT = *(struct stat_storage *) sp[-1].u.object->storage;
	pop_stack();
	return;
      }

    if ((1 << sp[-1].type) & (BIT_PROGRAM|BIT_OBJECT|BIT_MAPPING)) {
      MEMSET ((char *) &THIS_STAT->s, 0, sizeof (THIS_STAT->s));

#define ASSIGN_INDEX(ENUM)						\
      do {								\
	stack_dup();							\
	ref_push_string (stat_index_strs[ENUM]);			\
	sp[-1].subtype = 1;						\
	o_index();							\
	if (!IS_UNDEFINED (sp-1)) {					\
	  ref_push_string (stat_index_strs[ENUM]);			\
	  stack_swap();							\
	  stat_index_set (2);						\
	}								\
	pop_stack();							\
      } while (0)

      ASSIGN_INDEX (STAT_MODE);
      ASSIGN_INDEX (STAT_SIZE);
      ASSIGN_INDEX (STAT_ATIME);
      ASSIGN_INDEX (STAT_MTIME);
      ASSIGN_INDEX (STAT_CTIME);
      ASSIGN_INDEX (STAT_UID);
      ASSIGN_INDEX (STAT_GID);
      ASSIGN_INDEX (STAT_DEV);
      ASSIGN_INDEX (STAT_INO);
      ASSIGN_INDEX (STAT_NLINK);
      ASSIGN_INDEX (STAT_RDEV);
#if 0
      ASSIGN_INDEX (STAT_BLKSIZE);
      ASSIGN_INDEX (STAT_BLOCKS);
#endif
    }

    else if (sp[-1].type == T_ARRAY) {
      struct array *a = sp[-1].u.array;
      int i;
      if (a->size != 7)
	SIMPLE_BAD_ARG_ERROR ("Stat create", 1, "stat array with 7 elements");
      for (i = 0; i < 7; i++) {
	INT64 val;
	if (ITEM(a)[i].type == T_INT)
	  val = ITEM(a)[i].u.integer;
#ifdef AUTO_BIGNUM
	else if (ITEM(a)[i].type == T_OBJECT &&
		 is_bignum_object (ITEM(a)[i].u.object)) {
	  if (!int64_from_bignum (&val, ITEM(a)[i].u.object))
	    Pike_error ("Stat create: Too big integer in stat array.\n");
	}
#endif /* AUTO_BINUM */
	else
	  SIMPLE_BAD_ARG_ERROR ("Stat create", 1, "array(int)");
	stat_compat_set (i, val);
      }
    }

    else
      SIMPLE_BAD_ARG_ERROR ("Stat create", 1, "void|Stdio.Stat|array(int)");
  }

  else
    MEMSET ((char *) &THIS_STAT->s, 0, sizeof (THIS_STAT->s));

  pop_n_elems (args);
}

void f_min(INT32 args);
void f_max(INT32 args);

static void stat_index(INT32 args)
{
   if (!args) 
      SIMPLE_TOO_FEW_ARGS_ERROR("Stat `[]",1);
   else if (args==1)
   {
      if (sp[-1].type==T_INT)
      {
	 int index = sp[-1].u.integer;
	 pop_stack();
	 stat_push_compat(index);
      }
      else if (sp[-1].type==T_STRING)
      {
	 INT_TYPE code;

	 ref_push_mapping(stat_map);
	 push_svalue (sp-2);
	 f_index(2);
	 code = sp[-1].u.integer;	/* always integer there now */
	 pop_stack();

	 if (!code) {
	   /* Fall back to a normal index on this object, in case
	    * someone inherited us. */
	   struct svalue res;
	   object_index_no_free2 (&res, fp->current_object, sp-1);
	   pop_stack();
	   *sp++ = res;
	   return;
	 }
	 pop_stack();
	 
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
#if 0
	    case STAT_BLKSIZE: push_int(THIS_STAT->s.st_blksize); break;
	    case STAT_BLOCKS: push_int(THIS_STAT->s.st_blocks); break;
#endif /* 0 */
	    case STAT_ATIME: push_int64(THIS_STAT->s.st_atime); break;
	    case STAT_MTIME: push_int64(THIS_STAT->s.st_mtime); break;
	    case STAT_CTIME: push_int64(THIS_STAT->s.st_ctime); break;

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
		     push_constant_text("-");
		     break;
		  case S_IFDIR:
		     push_constant_text("d");
		     break;
		  case S_IFLNK:
		     push_constant_text("l");
		     break;
		  case S_IFCHR:
		     push_constant_text("c");
		     break;
		  case S_IFBLK:
		     push_constant_text("b");
		     break;
		  case S_IFIFO:
		     push_constant_text("f");
		     break;
		  case S_IFSOCK:
		     push_constant_text("s");
		     break;
		  default:
		     push_constant_text("?");
		     break;
	       }

	       if ( (THIS_STAT->s.st_mode & S_IRUSR) )
		  push_constant_text("r");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS_STAT->s.st_mode & S_IWUSR) )
		  push_constant_text("w");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS_STAT->s.st_mode & S_ISUID) )
		 if ( (THIS_STAT->s.st_mode & S_IXUSR) )
		   push_constant_text("s");
		 else
		   push_constant_text("S");
	       else
		 if ( (THIS_STAT->s.st_mode & S_IXUSR) )
		   push_constant_text("x");
		 else
		   push_constant_text("-");
		  
	       if ( (THIS_STAT->s.st_mode & S_IRGRP) )
		  push_constant_text("r");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS_STAT->s.st_mode & S_IWGRP) )
		  push_constant_text("w");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS_STAT->s.st_mode & S_ISGID) )
		  if ( (THIS_STAT->s.st_mode & S_IXGRP) )
		     push_constant_text("s");
		  else
		     push_constant_text("S");
	       else
		  if ( (THIS_STAT->s.st_mode & S_IXGRP) )
		     push_constant_text("x");
		  else
		     push_constant_text("-");

	       if ( (THIS_STAT->s.st_mode & S_IROTH) )
		  push_constant_text("r");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS_STAT->s.st_mode & S_IWOTH) )
		  push_constant_text("w");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS_STAT->s.st_mode & S_ISVTX) )
		 if ( (THIS_STAT->s.st_mode & S_IXOTH) )
		   push_constant_text("t");
		 else
		   push_constant_text("T");
	       else
		 if ( (THIS_STAT->s.st_mode & S_IXOTH) )
		   push_constant_text("x");
		 else
		   push_constant_text("-");
		  
	       f_add(10);

	       break;

	   default:
	     Pike_fatal ("stat_index is not kept up-to-date with stat_map.\n");
	 }
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Stat `[]",1,"int(0..6)|string");
   }
   else if (args>=2) /* range */
   {
      INT_TYPE from, to, n=0;

      if (args > 2) {
	pop_n_elems(args - 2);
	args = 2;
      }

#if AUTO_BIGNUM
      if (sp[-2].type!=T_INT &&
	  !(sp[-2].type == T_OBJECT && is_bignum_object (sp[-2].u.object)))
	 SIMPLE_BAD_ARG_ERROR("Stat `[..]",1,"int");

      if (sp[-1].type!=T_INT &&
	  !(sp[-1].type == T_OBJECT && is_bignum_object (sp[-1].u.object)))
	 SIMPLE_BAD_ARG_ERROR("Stat `[..]",2,"int");
#endif

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

static void stat_index_set (INT32 args)
{
  int got_int_val = 0;
  INT64 int_val;

  if (args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR ("Stat `[]=", 2);

  if (args > 2) {
    pop_n_elems (args - 2);
    args = 2;
  }

  if (sp[-1].type == T_INT)
    int_val = sp[-1].u.integer, got_int_val = 1;

#if AUTO_BIGNUM
  else if (sp[-1].type == T_OBJECT && is_bignum_object (sp[-1].u.object)) {
    if (!int64_from_bignum (&int_val, sp[-1].u.object))
      Pike_error ("Stat `[]=: Too big integer as value.\n");
    else
      got_int_val = 1;
  }
#endif
  /* shouldn't there be an else clause here ? */
  /* No, the second argument is checked further below depending on
   * what the first is. /mast */

  if (sp[-2].type == T_INT) {
    if (!got_int_val)
      SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 2,
			    "integer when the first argument is an integer");
    if (!stat_compat_set (sp[-2].u.integer, int_val))
      SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 1, "int(0..6)|string");
  }

  else if (sp[-2].type == T_STRING) {
    INT_TYPE code;

    ref_push_mapping (stat_map);
    push_svalue (sp-3);
    f_index (2);
    code = sp[-1].u.integer;
    pop_stack();

    if (!code) {
      /* Fall back to a normal index set on this object, in case
       * someone inherited us. */
      object_set_index2 (fp->current_object, sp-2, sp-1);
      stack_swap();
      pop_stack();
      return;
    }

    switch (code) {
      case 0:
	SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 1, "a valid index");

      case STAT_MODE_STRING:
	if (sp[-1].type != T_STRING)
	  SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 2, "string");

	/* FIXME: Handle modes on the form u+rw, perhaps? */

	if (sp[-1].u.string->len != 10)
	  SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 2, "mode string with 10 chars");

	{
	  PCHARP str = MKPCHARP_STR (sp[-1].u.string);
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
	if (sp[-1].type != T_STRING)
	  SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 2, "string");

	if (sp[-1].u.string == str_type_reg)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFREG;
	else if (sp[-1].u.string == str_type_dir)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFDIR;
	else if (sp[-1].u.string == str_type_lnk)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFLNK;
	else if (sp[-1].u.string == str_type_chr)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFCHR;
	else if (sp[-1].u.string == str_type_blk)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFBLK;
	else if (sp[-1].u.string == str_type_fifo)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFIFO;
	else if (sp[-1].u.string == str_type_sock)
	  THIS_STAT->s.st_mode = (THIS_STAT->s.st_mode & ~S_IFMT) | S_IFSOCK;
	else if (sp[-1].u.string == str_type_unknown)
	  THIS_STAT->s.st_mode = THIS_STAT->s.st_mode & ~S_IFMT;
	else
	  SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 2, "valid type string");
	break;

      case STAT_ISREG:
	THIS_STAT->s.st_mode &= ~S_IFMT;
	if (!UNSAFE_IS_ZERO (sp-1)) THIS_STAT->s.st_mode |= S_IFREG;
	break;
      case STAT_ISDIR:
	THIS_STAT->s.st_mode &= ~S_IFMT;
	if (!UNSAFE_IS_ZERO (sp-1)) THIS_STAT->s.st_mode |= S_IFDIR;
	break;
      case STAT_ISLNK:
	THIS_STAT->s.st_mode &= ~S_IFMT;
	if (!UNSAFE_IS_ZERO (sp-1)) THIS_STAT->s.st_mode |= S_IFLNK;
	break;
      case STAT_ISCHR:
	THIS_STAT->s.st_mode &= ~S_IFMT;
	if (!UNSAFE_IS_ZERO (sp-1)) THIS_STAT->s.st_mode |= S_IFCHR;
	break;
      case STAT_ISBLK:
	THIS_STAT->s.st_mode &= ~S_IFMT;
	if (!UNSAFE_IS_ZERO (sp-1)) THIS_STAT->s.st_mode |= S_IFBLK;
	break;
      case STAT_ISFIFO:
	THIS_STAT->s.st_mode &= ~S_IFMT;
	if (!UNSAFE_IS_ZERO (sp-1)) THIS_STAT->s.st_mode |= S_IFIFO;
	break;
      case STAT_ISSOCK:
	THIS_STAT->s.st_mode &= ~S_IFMT;
	if (!UNSAFE_IS_ZERO (sp-1)) THIS_STAT->s.st_mode |= S_IFSOCK;
	break;

      default:
	if (!got_int_val)
	  SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 2, "integer");

	switch (code) {
	  case STAT_DEV: DO_NOT_WARN(THIS_STAT->s.st_dev = int_val); break;
	  case STAT_INO: DO_NOT_WARN(THIS_STAT->s.st_ino = int_val); break;
	  case STAT_MODE: DO_NOT_WARN(THIS_STAT->s.st_mode = int_val); break;
	  case STAT_NLINK: DO_NOT_WARN(THIS_STAT->s.st_nlink = int_val); break;
	  case STAT_UID: DO_NOT_WARN(THIS_STAT->s.st_uid = int_val); break;
	  case STAT_GID: DO_NOT_WARN(THIS_STAT->s.st_gid = int_val); break;
	  case STAT_RDEV: DO_NOT_WARN(THIS_STAT->s.st_rdev = int_val); break;
	  case STAT_SIZE: DO_NOT_WARN(THIS_STAT->s.st_size = int_val); break;
#if 0
	  case STAT_BLKSIZE: DO_NOT_WARN(THIS_STAT->s.st_blksize = int_val); break;
	  case STAT_BLOCKS: DO_NOT_WARN(THIS_STAT->s.st_blocks = int_val); break;
#endif /* 0 */
	  case STAT_ATIME: DO_NOT_WARN(THIS_STAT->s.st_atime = int_val); break;
	  case STAT_MTIME: DO_NOT_WARN(THIS_STAT->s.st_mtime = int_val); break;
	  case STAT_CTIME: DO_NOT_WARN(THIS_STAT->s.st_ctime = int_val); break;

	  default:
	    Pike_fatal ("stat_index_set is not kept up-to-date with stat_map.\n");
	}
    }
  }

  else SIMPLE_BAD_ARG_ERROR ("Stat `[]=", 1, "int(0..6)|string");

  stack_swap();
  pop_stack();
}

static void stat_cast(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Stat cast",1);
   if (sp[-args].type==T_STRING && !sp[-args].u.string->size_shift)
   {
      if (strncmp(sp[-args].u.string->str,"array",5)==0)
      {
	 pop_n_elems(args);
	 push_int(0);
	 push_int(6);
	 stat_index(2);
	 return;
      }
   }
   SIMPLE_BAD_ARG_ERROR("Stat cast",1,
			"string(\"array\")");
}

static void stat__sprintf(INT32 args)
{
   int n=0, x;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("_sprintf",2);

   if (sp[-args].type!=T_INT)
      SIMPLE_BAD_ARG_ERROR("_sprintf",0,"integer");

   x=sp[-args].u.integer;
   pop_n_elems(args);
   switch (x)
   {
      case 'O':
	 n++; push_constant_text("Stat(");
	 
	 ref_push_string(stat_index_strs[STAT_MODE_STRING]);
	 n++; stat_index(1);

	 n++; push_constant_text(" ");
	 
	 ref_push_string(stat_index_strs[STAT_SIZE]);
	 n++; stat_index(1);
	 n++; push_constant_text("b)");
	 f_add(n);
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

#undef THIS_STAT

void push_stat(PIKE_STAT_T *s)
{
   struct object *o;
   struct stat_storage *stor;
   o=clone_object(stat_program,0);
   stor=(struct stat_storage*)get_storage(o,stat_program);
   stor->s=*s;
   push_object(o);
}

/*! @endclass
 */
/*! @endmodule
 */

/* ---------------------------------------------------------------- */

void init_files_stat()
{
   INT_TYPE n=0;

   MAKE_CONSTANT_SHARED_STRING (str_type_reg, "reg");
   MAKE_CONSTANT_SHARED_STRING (str_type_dir, "dir");
   MAKE_CONSTANT_SHARED_STRING (str_type_lnk, "lnk");
   MAKE_CONSTANT_SHARED_STRING (str_type_chr, "chr");
   MAKE_CONSTANT_SHARED_STRING (str_type_blk, "blk");
   MAKE_CONSTANT_SHARED_STRING (str_type_fifo, "fifo");
   MAKE_CONSTANT_SHARED_STRING (str_type_sock, "sock");
   MAKE_CONSTANT_SHARED_STRING (str_type_unknown, "unknown");

#define INIT_INDEX(ENUM, TXT) do {					\
     MAKE_CONSTANT_SHARED_STRING (stat_index_strs[ENUM], TXT);		\
     n++; ref_push_string (stat_index_strs[ENUM]); push_int (ENUM);	\
   } while (0)

   INIT_INDEX (STAT_DEV, "dev");
   INIT_INDEX (STAT_INO, "ino");
   INIT_INDEX (STAT_MODE, "mode");
   INIT_INDEX (STAT_NLINK, "nlink");
   INIT_INDEX (STAT_UID, "uid");
   INIT_INDEX (STAT_GID, "gid");
   INIT_INDEX (STAT_RDEV, "rdev");
   INIT_INDEX (STAT_SIZE, "size");
#if 0
   INIT_INDEX (STAT_BLKSIZE, "blksize");
   INIT_INDEX (STAT_BLOCKS, "blocks");
#endif
   INIT_INDEX (STAT_ATIME, "atime");
   INIT_INDEX (STAT_MTIME, "mtime");
   INIT_INDEX (STAT_CTIME, "ctime");

   INIT_INDEX (STAT_ISLNK, "islnk");
   INIT_INDEX (STAT_ISREG, "isreg");
   INIT_INDEX (STAT_ISDIR, "isdir");
   INIT_INDEX (STAT_ISCHR, "ischr");
   INIT_INDEX (STAT_ISBLK, "isblk");
   INIT_INDEX (STAT_ISFIFO, "isfifo");
   INIT_INDEX (STAT_ISSOCK, "issock");

   INIT_INDEX (STAT_TYPE, "type");
   INIT_INDEX (STAT_MODE_STRING, "mode_string");

   f_aggregate_mapping(n*2);
   stat_map=sp[-1].u.mapping;
   sp--;
   dmalloc_touch_svalue(sp);

   START_NEW_PROGRAM_ID(STDIO_STAT);
   ADD_STORAGE(struct stat_storage);

   ADD_FUNCTION ("create", stat_create,
		 tFunc(tOr5(tVoid,tObjImpl_STDIO_STAT,tPrg(tObj),tMapping,tArr(tInt)),
		       tVoid), ID_STATIC);

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

   ADD_FUNCTION("cast",stat_cast,tFunc(tStr,tArray),0);
   ADD_FUNCTION("_sprintf",stat__sprintf,
		tFunc(tInt tOr(tVoid,tMapping),tString),0);
   ADD_FUNCTION("_indices",stat_indices,
		tFunc(tNone,tArr(tOr(tString,tInt))),0);
   ADD_FUNCTION("_values",stat_values,
		tFunc(tNone,tArr(tOr(tString,tInt))),0);

   stat_program=end_program();
   add_program_constant("Stat",stat_program,0);
}

void exit_files_stat()
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
