/*
 * $Id: stat.c,v 1.4 2000/08/27 22:36:00 grubba Exp $
 */

#include "global.h"
RCSID("$Id: stat.c,v 1.4 2000/08/27 22:36:00 grubba Exp $");
#include "fdlib.h"
#include "interpret.h"
#include "svalue.h"
#include "bignum.h"
#include "mapping.h"
#include "object.h"
#include "builtin_functions.h"
#include "operators.h"

#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

struct program *stat_program=NULL;

struct stat_storage
{
   struct stat s;
};

static struct mapping *stat_map=NULL;

enum stat_query
{STAT_DEV=1, STAT_INO, STAT_MODE, STAT_NLINK, STAT_UID, STAT_GID, STAT_RDEV, 
 STAT_SIZE, STAT_BLKSIZE, STAT_BLOCKS, STAT_ATIME, STAT_MTIME, STAT_CTIME,
/* is... */
 STAT_ISLNK, STAT_ISREG, STAT_ISDIR, STAT_ISCHR, 
 STAT_ISBLK, STAT_ISFIFO, STAT_ISSOCK,
/* special */
 STAT_TYPE, STAT_MODE_STRING};

#define THIS ((struct stat_storage*)(fp->current_storage))

static void stat_push_compat(INT_TYPE n)
{
   switch (n)
   {
      case 0: push_int(THIS->s.st_mode); break;
      case 1: 
	 switch(S_IFMT & THIS->s.st_mode)
	 {
	    case S_IFREG:
	       push_int64((INT64)THIS->s.st_size);
	       break;
    
	    case S_IFDIR: push_int(-2); break;
#ifdef S_IFLNK
	    case S_IFLNK: push_int(-3); break;
#endif
	    default:
#ifdef DEBUG_FILE
	       fprintf(stderr, "encode_stat(): mode:%ld\n", 
		       (long)S_IFMT & THIS->s.st_mode);
#endif /* DEBUG_FILE */
	       push_int(-4);
	       break;
	 }
	 break;
      case 2: push_int(THIS->s.st_atime); break;
      case 3: push_int(THIS->s.st_mtime); break;
      case 4: push_int(THIS->s.st_ctime); break;
      case 5: push_int(THIS->s.st_uid); break;
      case 6: push_int(THIS->s.st_gid); break;
      default:
      {
	 INT32 args=1;
	 SIMPLE_BAD_ARG_ERROR("Stat `[]",1,"int(0..6)");
      }
   }
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
	 stack_swap();
	 f_index(2);

	 code = sp[-1].u.integer;	/* always integer there now */
	 pop_stack();
	 
	 switch (code)
	 {
	    case STAT_DEV: push_int(THIS->s.st_dev); break;
	    case STAT_INO: push_int(THIS->s.st_ino); break;
	    case STAT_MODE: push_int(THIS->s.st_mode); break;
	    case STAT_NLINK: push_int(THIS->s.st_nlink); break;
	    case STAT_UID: push_int(THIS->s.st_uid); break;
	    case STAT_GID: push_int(THIS->s.st_gid); break;
	    case STAT_RDEV: push_int(THIS->s.st_rdev); break;
	    case STAT_SIZE: push_int(THIS->s.st_size); break;
#ifdef STAT_BLKSIZE
	    case STAT_BLKSIZE: push_int(THIS->s.st_blksize); break;
#endif
#ifdef STAT_BLOCKS
	    case STAT_BLOCKS: push_int(THIS->s.st_blocks); break;
#endif
	    case STAT_ATIME: push_int(THIS->s.st_atime); break;
	    case STAT_MTIME: push_int(THIS->s.st_mtime); break;
	    case STAT_CTIME: push_int(THIS->s.st_ctime); break;

	    case STAT_ISREG: 
	       push_int(S_ISREG(THIS->s.st_mode));
	       break;
	    case STAT_ISLNK:
#ifdef S_ISLNK
	       push_int(S_ISLNK(THIS->s.st_mode));
#else
	       push_int(0);
#endif
	       break;
	    case STAT_ISDIR: 
#ifdef S_ISDIR
	       push_int(S_ISDIR(THIS->s.st_mode));
#else
	       push_int(0);
#endif
	       break;
	    case STAT_ISCHR: 
#ifdef S_ISCHR
	       push_int(S_ISCHR(THIS->s.st_mode));
#else
	       push_int(0);
#endif
	       break;
	    case STAT_ISBLK: 
#ifdef S_ISBLK
	       push_int(S_ISBLK(THIS->s.st_mode));
#else
	       push_int(0);
#endif
	       break;
	    case STAT_ISFIFO: 
#ifdef S_ISFIFO
	       push_int(S_ISFIFO(THIS->s.st_mode));
#else
	       push_int(0);
#endif
	       break;
	    case STAT_ISSOCK: 
#ifdef S_ISSOCK
	       push_int(S_ISSOCK(THIS->s.st_mode));
#else
	       push_int(0);
#endif
	       break;

	    case STAT_TYPE:
	       switch (THIS->s.st_mode & S_IFMT)
	       {
		  case S_IFREG:
		     push_constant_text("reg");
		     break;
#ifdef S_IFDIR
		  case S_IFDIR:
		     push_constant_text("dir");
		     break;
#endif
#ifdef S_IFLNK
		  case S_IFLNK:
		     push_constant_text("lnk");
		     break;
#endif
#ifdef S_IFCHR
		  case S_IFCHR:
		     push_constant_text("chr");
		     break;
#endif
#ifdef S_IFBLK
		  case S_IFBLK:
		     push_constant_text("blk");
		     break;
#endif
#ifdef S_IFFIFO
		  case S_IFFIFO:
		     push_constant_text("fifo");
		     break;
#endif
#ifdef S_IFSOCK
		  case S_IFSOCK:
		     push_constant_text("sock");
		     break;
#endif
		  default:
		     push_constant_text("unknown");
		     break;
	       }
	       break;
	       
	    case STAT_MODE_STRING:
	       switch (THIS->s.st_mode & S_IFMT)
	       {
		  case S_IFREG:
		     push_constant_text("-");
		     break;
#ifdef S_IFDIR
		  case S_IFDIR:
		     push_constant_text("d");
		     break;
#endif
#ifdef S_IFLNK
		  case S_IFLNK:
		     push_constant_text("l");
		     break;
#endif
#ifdef S_IFCHR
		  case S_IFCHR:
		     push_constant_text("c");
		     break;
#endif
#ifdef S_IFBLK
		  case S_IFBLK:
		     push_constant_text("b");
		     break;
#endif
#ifdef S_IFFIFO
		  case S_IFFIFO:
		     push_constant_text("f");
		     break;
#endif
#ifdef S_IFSOCK
		  case S_IFSOCK:
		     push_constant_text("s");
		     break;
#endif
		  default:
		     push_constant_text("?");
		     break;
	       }

	       if ( (THIS->s.st_mode & S_IRUSR) )
		  push_constant_text("r");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS->s.st_mode & S_IWUSR) )
		  push_constant_text("w");
	       else
		  push_constant_text("-");
		  
#ifdef S_ISVTX
	       if ( (THIS->s.st_mode & S_ISVTX) )
		  push_constant_text("S");
	       else 
#endif
#ifdef S_ISUID
		  if ( (THIS->s.st_mode & S_ISUID) )
		     push_constant_text("s");
		  else 
#endif
		     if ( (THIS->s.st_mode & S_IXUSR) )
			push_constant_text("x");
		     else
			push_constant_text("-");
		  
	       if ( (THIS->s.st_mode & S_IRGRP) )
		  push_constant_text("r");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS->s.st_mode & S_IWGRP) )
		  push_constant_text("w");
	       else
		  push_constant_text("-");
		  
#ifdef S_ISUID
	       if ( (THIS->s.st_mode & S_ISGID) )
		  push_constant_text("s");
	       else 
#endif
		  if ( (THIS->s.st_mode & S_IXGRP) )
		     push_constant_text("x");
		  else
		     push_constant_text("-");

	       if ( (THIS->s.st_mode & S_IROTH) )
		  push_constant_text("r");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS->s.st_mode & S_IWOTH) )
		  push_constant_text("w");
	       else
		  push_constant_text("-");
		  
	       if ( (THIS->s.st_mode & S_IXOTH) )
		  push_constant_text("x");
	       else
		  push_constant_text("-");
		  
	       f_add(10);

	       break;
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

      /* make in range 0..6 */
      push_int(6);
      f_min(2);
      stack_swap();
      push_int(0);
      f_max(2);
      stack_swap();
      
      if (sp[-2].type!=T_INT)
	 SIMPLE_BAD_ARG_ERROR("Stat `[..]",2,"int(0..6)");
      if (sp[-1].type!=T_INT)
	 SIMPLE_BAD_ARG_ERROR("Stat `[..]",1,"int(0..6)");

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

static void stat_cast(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Colortable->cast",1);
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
	 
	 ref_push_object(fp->current_object);
	 push_constant_text("mode_string");
	 n++; f_index(2);

	 n++; push_constant_text(" ");
	 
	 ref_push_object(fp->current_object);
	 push_constant_text("size");
	 n++; f_index(2);
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
   stack_pop_n_elems_keep_top(1);
}

#undef THIS

void push_stat(struct stat *s)
{
   struct object *o;
   struct stat_storage *stor;
   o=clone_object(stat_program,0);
   stor=(struct stat_storage*)get_storage(o,stat_program);
   stor->s=*s;
   push_object(o);
}

/* ---------------------------------------------------------------- */

void init_files_stat()
{
   INT_TYPE n=0;

   n++; push_text("dev"); push_int(STAT_DEV);
   n++; push_text("ino"); push_int(STAT_INO);
   n++; push_text("mode"); push_int(STAT_MODE);
   n++; push_text("nlink"); push_int(STAT_NLINK);
   n++; push_text("uid"); push_int(STAT_UID);
   n++; push_text("gid"); push_int(STAT_GID);
   n++; push_text("rdev"); push_int(STAT_RDEV);
   n++; push_text("size"); push_int(STAT_SIZE);
   n++; push_text("blksize"); push_int(STAT_BLKSIZE);
   n++; push_text("blocks"); push_int(STAT_BLOCKS);
   n++; push_text("atime"); push_int(STAT_ATIME);
   n++; push_text("mtime"); push_int(STAT_MTIME);
   n++; push_text("ctime"); push_int(STAT_CTIME);

   n++; push_text("islnk"); push_int(STAT_ISLNK);
   n++; push_text("isreg"); push_int(STAT_ISREG);
   n++; push_text("isdir"); push_int(STAT_ISDIR);
   n++; push_text("ischr"); push_int(STAT_ISCHR);
   n++; push_text("isblk"); push_int(STAT_ISBLK);
   n++; push_text("isfifo"); push_int(STAT_ISFIFO);
   n++; push_text("issock"); push_int(STAT_ISSOCK);

   n++; push_text("type"); push_int(STAT_TYPE);
   n++; push_text("mode_string"); push_int(STAT_MODE_STRING);
     
   f_aggregate_mapping(n*2);
   stat_map=sp[-1].u.mapping;
   sp--;
   dmalloc_touch_svalue(sp);

   start_new_program();
   ADD_STORAGE(struct stat_storage);

   ADD_FUNCTION("`[]",stat_index,
		tOr(tFunc(tOr(tStr,tIntPos),tOr3(tStr,tInt,tFunction)),
		    tFunc(tInt tInt,tArr(tInt))),0);

   ADD_FUNCTION("`->",stat_index,
		tOr(tFunc(tOr(tStr,tIntPos),tOr3(tStr,tInt,tFunction)),
		    tFunc(tInt tInt,tArr(tInt))),0);

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
   free_program(stat_program);
   free_mapping(stat_map);
}
