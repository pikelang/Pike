/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: memory.c,v 1.31 2005/02/25 23:36:04 nilsson Exp $
*/

/*! @module System
 */

/*! @class Memory
 *!	A popular demand is a class representing a raw piece
 *!	of memory or a @tt{mmap@}'ed file. This is usually not the most
 *!	effective way of solving a problem, but it might help in
 *!	rare situations.
 *!
 *!	Using @tt{mmap@} can lead to segmentation faults in some cases.
 *!	Beware, and read about @tt{mmap@} before you try anything.
 *!	Don't blame Pike if you shoot your foot off.
 */
#include "global.h"
#include "system_machine.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

#ifdef HAVE_SYS_SHM_H
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_CYGWIN_SHM_H
#include <cygwin/ipc.h>
#include <cygwin/shm.h>
#define HAVE_SYS_SHM_H
#endif

/* something on AIX defines these */
#ifdef T_INT
#undef T_INT
#endif
#ifdef T_FLOAT
#undef T_FLOAT
#endif

/* some systems call it PAGESIZE, some PAGE_SIZE */
#ifndef PAGE_SIZE
#ifdef PAGESIZE
#define PAGE_SIZE PAGESIZE
#endif
#endif

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "program.h"
#include "operators.h"
#include "fdlib.h"
#include "bignum.h"

#include "system.h"


#define sp Pike_sp

static void memory__mmap(INT32 args,int complain,int private);
static void memory_allocate(INT32 args);
static void memory_shm(INT32 args);

/*** Memory object *******************************************************/

/* MicroSoft defines this macro. */
#ifdef THIS
#undef THIS
#endif /* THIS */

#define THISOBJ (Pike_fp->current_object)
#define THIS ((struct memory_storage *)(Pike_fp->current_storage))

static void init_memory(struct object *o)
{
   THIS->p=NULL;
   THIS->size=0;
   THIS->flags=0;
}

static void MEMORY_FREE( struct memory_storage *storage )
{
  if( storage->flags & MEM_FREE_FREE )
    free( storage->p );
#ifdef HAVE_MMAP
  else if( storage->flags & MEM_FREE_MUNMAP )
    munmap( (void*)storage->p, storage->size );
#endif
#ifdef HAVE_SYS_SHM_H
  else if( storage->flags & MEM_FREE_SHMDEL )
    shmdt( storage->p );
#endif
#ifdef WIN32SHM
  else if( storage->flags & MEM_FREE_SHMDEL )
  {
    UnmapViewOfFile( storage->p );
    CloseHandle( (HANDLE)storage->extra );
  }
#endif
  storage->flags = 0;
  storage->p = 0;
  storage->size = 0;
}


#define MEMORY_VALID(STORAGE,FUNC)					\
   if (!(STORAGE->p))							\
      Pike_error("%s: no memory in this Memory object\n",FUNC);

static void exit_memory(struct object *o)
{
   MEMORY_FREE(THIS);
}

/*! @decl void create()
 *! @decl void create(string|Stdio.File filename_to_mmap)
 *! @decl void create(int shmkey, int shmsize, int shmflg)
 *! @decl void create(int bytes_to_allocate)
 *!
 *!	Will call @[mmap()] or @[allocate()]
 *!	depending on argument, either @tt{mmap@}'ing a file
 *!	(in shared mode, writeable if possible) or allocating
 *!	a chunk of memory.
 */
static void memory_create(INT32 args)
{
   if (args)
   {
      if (sp[-args].type==T_STRING ||
	  sp[-args].type==T_OBJECT) /* filename to mmap */
	memory__mmap(args,1,0);
      else if (sp[-args].type==T_INT && args==1) /* bytes to allocate */
	memory_allocate(args);
      else if(sp[-args].type==T_INT && sp[-args+1].type==T_INT && args==2 )
	memory_shm( args );
      else
	 SIMPLE_BAD_ARG_ERROR("Memory",1,"int or string");
   }
   else
   {
      MEMORY_FREE(THIS);
      push_int(0);
   }
}

/*! @decl int mmap(string|Stdio.File file)
 *! @decl int mmap(string|Stdio.File file,int offset,int size)
 *! @decl int mmap_private(string|Stdio.File file)
 *! @decl int mmap_private(string|Stdio.File file,int offset,int size)
 *!
 *!	@tt{mmap@} a file. This will always try to mmap the file in
 *!	PROT_READ|PROT_WRITE, readable and writable, but if it fails
 *!	it will try once more in PROT_READ only.
 */
static INLINE off_t file_size(int fd)
{
  PIKE_STAT_T tmp;
  if((!fd_fstat(fd, &tmp)) &&
     ( tmp.st_mode & S_IFMT) == S_IFREG)
     return (off_t)tmp.st_size;
  return -1;
}

#define RETURN(ZERO)							\
   do									\
   {									\
      pop_n_elems(args);						\
      push_int(ZERO);							\
      return;								\
   }									\
   while (0)

static void memory_shm( INT32 args )
{
#ifdef HAVE_SYS_SHM_H
  int id;

  MEMORY_FREE(THIS);

  if( args < 2 )
    SIMPLE_TOO_FEW_ARGS_ERROR("Memory.shmat",2);
  if (Pike_sp[1-args].type!=T_INT )
    SIMPLE_BAD_ARG_ERROR("Memory.shmat",1,"int(0..)");
  if (Pike_sp[-args].type!=T_INT )
    SIMPLE_BAD_ARG_ERROR("Memory.shmat",0,"int(0..)");

  if( (id = shmget( Pike_sp[0-args].u.integer,
		    Pike_sp[1-args].u.integer,
		    IPC_CREAT|0666 )) < 0 )
  {
    switch( errno )
    {
      case EINVAL:
	Pike_error("Too large or small shared memory segment\n");
	break;
      case ENOSPC:
	Pike_error("Out of resources, cannot create segment\n");
	break;
    }
  }
  THIS->p = shmat( id, 0, 0 );
  THIS->size = Pike_sp[1-args].u.integer;
  THIS->flags = MEM_READ|MEM_WRITE|MEM_FREE_SHMDEL;
  pop_n_elems(args);
  push_int(1);
#else /* HAVE_MMAP */
#ifdef WIN32SHM
  {
    HANDLE handle;
    char id[4711];
    sprintf( id, "pike.%ld", Pike_sp[-args].u.integer );
    THIS->size = Pike_sp[1-args].u.integer;
    THIS->flags = MEM_READ|MEM_WRITE|MEM_FREE_SHMDEL;
    pop_n_elems(args);

    handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
			       0, THIS->size, id);
    if( handle != NULL )
    {
      THIS->extra = (void*)handle;
      THIS->p = MapViewOfFile(handle,FILE_MAP_WRITE|FILE_MAP_READ,0,0,0);
      if( !THIS->p )
      {
	THIS->flags = 0;
	CloseHandle( handle );
	Pike_error("Failed to create segment\n");
      }
    }
    else
    {
	THIS->flags = 0;
	CloseHandle( handle );
	Pike_error("Failed to create segment\n");
    }
  }
#else
   Pike_error("Memory.shmat(): system has no shmat() (sorry)\n");
#endif
#endif
}

static void memory__mmap(INT32 args,int complain,int private)
{
#ifdef HAVE_MMAP
   int fd=-1;
   int doclose=0;
   off_t osize=0;
   size_t offset=0,size=0;
   char *mem;
   int flags=0; 
   int resflags=MEM_FREE_MUNMAP|MEM_WRITE|MEM_READ;
  
   MEMORY_FREE(THIS); /* we expect this even on error */

   if (args<1) 
      SIMPLE_TOO_FEW_ARGS_ERROR("Memory.mmap",1);

   if (args>=2) {
      if (sp[1-args].type!=T_INT ||
	  sp[1-args].u.integer<0)
	 SIMPLE_BAD_ARG_ERROR("Memory.mmap",2,"int(0..)");
      else
	 offset=sp[1-args].u.integer;
   }

   if (args>=3) {
      if (sp[2-args].type!=T_INT ||
	  sp[2-args].u.integer<0)
	 SIMPLE_BAD_ARG_ERROR("Memory.mmap",3,"int(0..)");
      else
	 size=sp[2-args].u.integer;
   }

   if (sp[-args].type==T_OBJECT)
   {
      struct object *o=sp[-args].u.object;
      ref_push_object(o);
      push_text("query_fd");
      f_index(2);
      if (sp[-1].type==T_INT)
	 SIMPLE_BAD_ARG_ERROR("Memory.mmap",1,
			      "(string or) Stdio.File (missing query_fd)");
      f_call_function(1);
      if (sp[-1].type!=T_INT)
	 SIMPLE_BAD_ARG_ERROR("Memory.mmap",1,
			      "(string or) Stdio.File (weird query_fd)");
      fd=sp[-1].u.integer;
      sp--;
      if (fd<0) {
	 if (complain)
	    SIMPLE_BAD_ARG_ERROR("Memory.mmap",1,
				 "(string or) Stdio.File (file not open)");
	 else 
	    RETURN(0);
      }

      THREADS_ALLOW();
      osize=file_size(fd);
      THREADS_DISALLOW();
   }
   else if (sp[-args].type==T_STRING)
   {
      char *filename;
      get_all_args("Memory.mmap",args,"%s",&filename); /* 8 bit! */
      
      THREADS_ALLOW();
      fd = fd_open(filename,fd_RDWR,0);
      if (fd>=0) osize=file_size(fd);
      THREADS_DISALLOW();

      if (fd<0) {
	 if (complain)
	    Pike_error("Memory.mmap(): Failed to open file\n");
	 else
	    RETURN(0);
      }
      doclose=1;
   }

   if (osize<0)
   {
      if (doclose) fd_close(fd); 					
      if (!complain)
	 RETURN(0);
      else
	 Pike_error("Memory.mmap(): Not a regular file\n");
   }

   if (!size) size=((size_t)osize)-offset;
   if (offset+size>(size_t)osize)
      Pike_error("Memory.mmap(): mapped area outside file\n");

#ifdef PAGE_SIZE
   if (offset%PAGE_SIZE)
      Pike_error("Memory.mmap(): mapped offset not aligned to PAGE_SIZE "
		 "(%d aka system.PAGE_SIZE)\n",(int)offset);
#endif

   if (private) flags|=MAP_PRIVATE;
   else flags|=MAP_SHARED;

   mem=mmap(NULL,size,PROT_READ|PROT_WRITE,flags,fd,offset);
#ifndef MAP_FAILED
#define MAP_FAILED ((void*)(ptrdiff_t)-1)
#endif
   if ((mem==(void *)MAP_FAILED) && (errno==EACCES)) /* try without write */
   {
      resflags&=~MEM_WRITE;
      mem=mmap(NULL,size,PROT_READ,flags,fd,offset);
   }

   if (doclose) fd_close(fd); /* don't need this one anymore */

   if (mem==(void *)MAP_FAILED)
   {
      if (!complain)
	 RETURN(0);
      else switch (errno)
      {
	 case EBADF:  Pike_error("Memory.mmap(): error: not a valid fd\n");
	 case EACCES: Pike_error("Memory.mmap(): error: no access\n");
	 case EINVAL: 
	    Pike_error("Memory.mmap(): error: invalid parameters "
		       "(probably non-aligned offset or size)\n");
	 case EAGAIN: Pike_error("Memory.mmap(): error: file is locked\n");
	 case ENOMEM: 
	    Pike_error("Memory.mmap(): error: out of address space\n");
	 default:     
	    Pike_error("Memory.mmap(): unknown error: errno=%d\n",errno);
      }
   }
   
/* need to do this again, due to threads */
   MEMORY_FREE(THIS);

   THIS->size=size;
   THIS->p = (unsigned char *)mem;
   THIS->flags=resflags;

   RETURN(1); /* ok */

#else /* HAVE_MMAP */
   Pike_error("Memory.mmap(): system has no mmap() (sorry)\n");
#endif
}

#ifdef HAVE_MMAP

static void memory_mmap(INT32 args)
{
   memory__mmap(args,0,0);
}

static void memory_mmap_private(INT32 args)
{
   memory__mmap(args,0,1);
}

#endif

/*! @decl void allocate(int bytes)
 *! @decl void allocate(int bytes,int(0..255) fill)
 */
static void memory_allocate(INT32 args)
{
   INT_TYPE c=0;
   INT_TYPE size;
   unsigned char *mem;

   if (args>=2)
      get_all_args("Memory.allocate",args,"%+%+",&size,&c);
   else
      get_all_args("Memory.allocate",args,"%+",&size);

   /* just to be sure */
   if (size<0)
      SIMPLE_BAD_ARG_ERROR("Memory.allocate",1,"int(0..)");
   
   pop_n_elems(args);
   if (size>1024*1024) /* threshold */
   {
      THREADS_ALLOW();
      mem = (unsigned char *)xalloc(size);
      MEMSET(mem,c,size);
      THREADS_DISALLOW();
   }
   else
   {
      mem = (unsigned char *)xalloc(size);
      MEMSET(mem,c,size);
   }

   MEMORY_FREE(THIS);
   THIS->p=mem;
   THIS->size=size;
   THIS->flags=MEM_READ|MEM_WRITE|MEM_FREE_FREE;
}


/*! @decl void free()
 *!
 *!	Free the allocated or <tt>mmap</tt>ed memory.
 */
static void memory_free(INT32 args)
{
   pop_n_elems(args);
   push_int(0);
   MEMORY_FREE(THIS);
}



/*! @decl int _sizeof()
 *!
 *! returns the size of the memory (bytes).
 *! note: throws if not allocated
 */
static void memory__sizeof(INT32 args)
{
   MEMORY_VALID(THIS,"Memory._sizeof");
   pop_n_elems(args);
   push_int64(THIS->size);
}

/*! @decl int(0..1) valid()
 *!
 *! returns 1 if the memory is valid, 0 if not allocated
 */
static void memory_valid(INT32 args)
{
   pop_n_elems(args);
   push_int(!!(THIS->p));
}

/*! @decl int(0..1) writeable()
 *!
 *! returns 1 if the memory is writeable, 0 if not
 */
static void memory_writeable(INT32 args)
{
   pop_n_elems(args);
   push_int(!!(THIS->flags&MEM_WRITE));
}

/*! @decl string|array cast(string to)
 *!
 *!	Cast to string or array.
 *!
 *! @note
 *!   Throws if not allocated.
 */
static void memory_cast(INT32 args)
{
   char *s;
   get_all_args("Memory.cast",args,"%s",&s);

   MEMORY_VALID(THIS,"Memory.cast");
   
   if (strncmp(s,"string",5)==0)
   {
      pop_n_elems(args);
      push_string(make_shared_binary_string((char *)THIS->p, THIS->size));
      return;
   }
   if (strncmp(s,"array",5)==0)
   {
      struct array *a;
      size_t i,sz=THIS->size;
      struct svalue *sv;

      pop_n_elems(args);

      a=low_allocate_array(sz,0);

      sv=ITEM(a);
      for (i=0; i<sz; i++)
      {
	 sv->u.integer=(((unsigned char*)(THIS->p)))[i];
	 sv++;
      }
      a->type_field = BIT_INT;

      push_array(a);

      return;
   }
}

/*! @decl string pread(int(0..) pos,int(0..) len)
 *! @decl string pread16(int(0..) pos,int(0..) len)
 *! @decl string pread32(int(0..) pos,int(0..) len)
 *! @decl string pread16i(int(0..) pos,int(0..) len)
 *! @decl string pread32i(int(0..) pos,int(0..) len)
 *! @decl string pread16n(int(0..) pos,int(0..) len)
 *! @decl string pread32n(int(0..) pos,int(0..) len)
 *!
 *!	Read a string from the memory. The 16 and 32 variants reads
 *!	widestrings, 16 or 32 bits (2 or 4 bytes) wide, the i variants
 *!	in intel byteorder, the normal in network byteorder, and the n
 *!	variants in native byteorder.
 *!
 *!	@[len] is the number of characters, wide or not. @[pos]
 *!	is the byte position (!).
 */
#define MEMORY_PREADN(FUNC,NAME,N,MAKER)				\
   static void FUNC(INT32 args)						\
   {									\
      INT_TYPE pos,len;							\
      size_t rpos,rlen;							\
      									\
      get_all_args(NAME,args,"%+%+",&pos,&len);				\
      rpos=(size_t)pos;							\
      rlen=(size_t)len;							\
   									\
      MEMORY_VALID(THIS,NAME);						\
      if (rpos+rlen*N>THIS->size)					\
   	 Pike_error(NAME": reading (some) outside allocation\n");	\
   									\
      if (!rlen)							\
   	 push_text("");							\
      else								\
   	 push_string(MAKER((void*)(THIS->p+pos),len));			\
   									\
      stack_pop_n_elems_keep_top(args);					\
   }


static void copy_reverse_string1(unsigned char *d,unsigned char *s,size_t len)
{
   while (len--)
   {
      d[1]=s[0];
      d[0]=s[1];
      d+=2;
      s+=2;
   }
}

static void copy_reverse_string2(unsigned char *d,unsigned char *s,size_t len)
{
   while (len--)
   {
      d[3]=s[0];
      d[2]=s[1];
      d[1]=s[2];
      d[0]=s[3];
      d+=4;
      s+=4;
   }
}

static void copy_reverse_string0_to_1(unsigned char *d,
				      unsigned char *s,size_t len)
{
   while (len--)
   {
      d[1]=s[0];
      d[0]=0;
      d+=2;
      s++;
   }
}

static void copy_reverse_string0_to_2(unsigned char *d,
				      unsigned char *s,size_t len)
{
   while (len--)
   {
      d[3]=s[0];
      d[2]=0;
      d[1]=0;
      d[0]=0;
      d+=4;
      s++;
   }
}

static void copy_reverse_string1_to_2(unsigned char *d,
				      unsigned char *s,size_t len)
{
   while (len--)
   {
      d[3]=s[0];
      d[2]=s[1];
      d[1]=0;
      d[0]=0;
      d+=4;
      s+=2;
   }
}

#define MAKE_REVERSE_ORDER_STRINGN(N)					\
   static struct pike_string*						\
      make_reverse_order_string##N(unsigned char *s,size_t len)		\
   {									\
      struct pike_string *ps;						\
      ps=begin_wide_shared_string(len,N);				\
      copy_reverse_string##N(ps->str, (unsigned char *)s, len);		\
      return end_shared_string(ps);					\
   }

MAKE_REVERSE_ORDER_STRINGN(1)
MAKE_REVERSE_ORDER_STRINGN(2)

MEMORY_PREADN(memory_pread,"Memory.pread",1,make_shared_binary_string)
#if (PIKE_BYTEORDER == 1234)
MEMORY_PREADN(memory_pread16i,"Memory.pread16i",2,make_shared_binary_string1)
MEMORY_PREADN(memory_pread32i,"Memory.pread32i",4,make_shared_binary_string2)
MEMORY_PREADN(memory_pread16,"Memory.pread16",2,make_reverse_order_string1)
MEMORY_PREADN(memory_pread32,"Memory.pread32",4,make_reverse_order_string2)
#else
MEMORY_PREADN(memory_pread16,"Memory.pread16",2,make_shared_binary_string1)
MEMORY_PREADN(memory_pread32,"Memory.pread32",4,make_shared_binary_string2)
MEMORY_PREADN(memory_pread16i,"Memory.pread16i",2,make_reverse_order_string1)
MEMORY_PREADN(memory_pread32i,"Memory.pread32i",4,make_reverse_order_string2)
#endif

MEMORY_PREADN(memory_pread16n,"Memory.pread16n",2,make_shared_binary_string1)
MEMORY_PREADN(memory_pread32n,"Memory.pread32n",4,make_shared_binary_string2)

/*! @decl int pwrite(int(0..) pos,string data)
 *! @decl int pwrite16(int(0..) pos,string data)
 *! @decl int pwrite32(int(0..) pos,string data)
 *! @decl int pwrite16i(int(0..) pos,string data)
 *! @decl int pwrite32i(int(0..) pos,string data)
 *!
 *!	Write a string to the memory (and to the file, if it's mmap()ed). 
 *!	The 16 and 32 variants writes widestrings, 
 *!	16 or 32 bits (2 or 4 bytes) wide,
 *!	the 'i' variants in intel byteorder, the other in network byteorder.
 *!
 *! returns the number of bytes (not characters) written
 */
static void pwrite_n(INT32 args,int shift,int reverse,char *func)
{
   INT_TYPE pos;
   size_t rpos,rlen;
   struct pike_string *ps;
   unsigned char *d;

   get_all_args(func,args,"%+%W",&pos,&ps);
   rpos=(size_t)pos;
   rlen=(ps->len<<shift);

   if (ps->size_shift>shift)
      Pike_error("%s: given string wider (%d) then what we write (%d)\n",
		 func,8<<ps->size_shift,8<<shift);

   MEMORY_VALID(THIS,func);
   if (!(THIS->flags&MEM_WRITE))
      Pike_error("%s: can't write in this memory\n",func);
   
   if (rpos+(rlen<<shift)>THIS->size)
      Pike_error("%s: writing outside allocation\n",func);

   d=THIS->p+rpos;

#if 0
   fprintf(stderr,"p=%p pos=%d d=%p s=%p len=%d d shift=%d  s shift=%d\n",
	   THIS->p,rpos,THIS->p+rpos,ps->str,ps->len,shift,ps->size_shift);
#endif

   if (rlen) 
      switch (ps->size_shift*10 + shift)
      {
	 case 22: /* 2 -> 2 */
	    if (reverse)
	      copy_reverse_string2(d, (unsigned char *)ps->str, ps->len);
	    else MEMCPY(d,ps->str,ps->len*4);
	    break;
	 case 12: /* 1 -> 2 */
	    if (reverse)
	      copy_reverse_string1_to_2(d, (unsigned char *)ps->str, ps->len);
	    else convert_1_to_2((p_wchar2*)d,(p_wchar1*)ps->str,ps->len);
	    break;
	 case 02: /* 0 -> 2 */
	    if (reverse)
	      copy_reverse_string0_to_2(d, (unsigned char *)ps->str, ps->len);
	    else convert_0_to_2((p_wchar2*)d,ps->str,ps->len);
	    break;
	 case 11: /* 1 -> 1 */
	    if (reverse)
	      copy_reverse_string1(d, (unsigned char *)ps->str,ps->len);
	    else MEMCPY(d,ps->str,ps->len*2);
	    break;
	 case 01: /* 0 -> 1 */
	    if (reverse)
	      copy_reverse_string0_to_1(d, (unsigned char *)ps->str,ps->len);
	    else convert_0_to_1((p_wchar1*)d,ps->str,ps->len);
	    break;
	 case 00:
	    MEMCPY(d,ps->str,ps->len);
	    break;
	 default:
	    Pike_error("Illegal state %d -> %d\n",ps->size_shift,shift);
      }

   pop_n_elems(args);
   push_int64(rlen);
}

#define PWRITEN(CFUNC,FUNC,SHIFT,REV)					\
   static void CFUNC(INT32 args)					\
   {									\
      pwrite_n(args,SHIFT,REV,FUNC);					\
   }

PWRITEN(memory_pwrite,"pwrite",0,0)
#if (PIKE_BYTEORDER == 1234)
PWRITEN(memory_pwrite16,"pwrite16",1,1)
PWRITEN(memory_pwrite32,"pwrite32",2,1)
PWRITEN(memory_pwrite16i,"pwrite16i",1,0)
PWRITEN(memory_pwrite32i,"pwrite32i",2,0)
#else
PWRITEN(memory_pwrite16,"pwrite16",1,0)
PWRITEN(memory_pwrite32,"pwrite32",2,0)
PWRITEN(memory_pwrite16i,"pwrite16i",1,1)
PWRITEN(memory_pwrite32i,"pwrite32i",2,1)
#endif
PWRITEN(memory_pwrite16n,"pwrite16n",1,0)
PWRITEN(memory_pwrite32n,"pwrite32n",2,0)

/*! @decl int `[](int pos)
 *! @decl string `[](int pos1,int pos2)
 */
static void memory_index(INT32 args)
{
   MEMORY_VALID(THIS,"Memory.`[]");

   if (args==1)
   {
      INT_TYPE pos;
      size_t rpos = 0;
      get_all_args("Memory.`[]",args,"%i",&pos);
      if (pos<0) {
	 if ((off_t)-pos>=DO_NOT_WARN((off_t)THIS->size))
	    Pike_error("Memory.`[]: Index is out of range\n");
	 else
	    rpos=(size_t)(DO_NOT_WARN((off_t)(THIS->size))+(off_t)pos);
      }
      else 
      {
	 rpos=(size_t)pos;
      
	 if (rpos>THIS->size)
	    Pike_error("Memory.`[]: Index is out of range\n");
      }

      push_int( (((unsigned char*)(THIS->p)))[rpos] );
   }
   else
   {
      if (THIS->size==0)
	 push_text("");
      else
      {
	 INT_TYPE pos1,pos2;
	 size_t rpos1,rpos2;

	 get_all_args("Memory.`[]",args,"%i%i",&pos1,&pos2);
	 if (pos1<0) rpos1=0; else rpos1=(size_t)pos1;
	 if ((size_t)pos2>=THIS->size) rpos2=THIS->size-1; 
	 else rpos2=(size_t)pos2;

	 if (rpos2<rpos1)
	    push_text("");
	 else
	    push_string(make_shared_binary_string((char *)THIS->p+rpos1,
						  rpos2-rpos1+1));
      }
   }
   stack_pop_n_elems_keep_top(args);
}

/*! @decl int `[]=(int pos,int char)
 *! @decl string `[]=(int pos1,int pos2,string str)
 */
static void memory_index_write(INT32 args)
{
   MEMORY_VALID(THIS,"Memory.`[]=");

   if (!(THIS->flags&MEM_WRITE))
      Pike_error("`[]=: can't write in this memory\n");

   if (args==2)
   {
      INT_TYPE pos,ch;
      size_t rpos = 0;
      get_all_args("Memory.`[]=",args,"%i%i",&pos,&ch);
      if (pos<0) 
	 if ((off_t)-pos>=DO_NOT_WARN((off_t)THIS->size))
	    Pike_error("Memory.`[]=: Index is out of range\n");
	 else
	    rpos=(size_t)(DO_NOT_WARN((off_t)(THIS->size))+(off_t)pos);
      else 
      {
	 rpos=(size_t)pos;
      
	 if (rpos>THIS->size)
	    Pike_error("Memory.`[]=: Index is out of range\n");
      }
      if (ch<0 || ch>255)
	 Pike_error("Memory.`[]=: Can only write bytes (0..255)\n");

      push_int( (((unsigned char*)(THIS->p)))[rpos] = ch );
   }
   else
   {
      if (THIS->size==0)
	 push_text("");
      else
      {
	 INT_TYPE pos1,pos2;
	 size_t rpos1,rpos2;
	 struct pike_string *ps;

	 get_all_args("Memory.`[]=",args,"%i%i%S",&pos1,&pos2,&ps);
	 if (pos1<0) rpos1=0; else rpos1=(size_t)pos1;
	 if ((size_t)pos2>=THIS->size) rpos2=THIS->size-1; 
	 else rpos2=(size_t)pos2;

	 if (pos2<pos1 ||
	     ps->len != pos2-pos1+1)
	    Pike_error("Memory.`[]=: source and destination "
		       "not equally long (%ld v/s %ld; can't resize memory)\n",
		       DO_NOT_WARN((long)ps->len),(long)pos2-(long)pos1);
	 else
	    MEMCPY(THIS->p+pos1,ps->str,ps->len);

	 ref_push_string(ps);
      }
   }
   stack_pop_n_elems_keep_top(args);
}

/*! @endclass
 */

/*! @endmodule
 */
  
/*** module init & exit & stuff *****************************************/

void init_system_memory(void)
{
/* initiate system.Memory class */

/* MEMORY object */

   start_new_program();
   ADD_STORAGE(struct memory_storage);

   ADD_FUNCTION("create", 
		memory_create,    
		tOr3(tFunc(tVoid,tVoid),
		     tFunc(tOr(tStr,tObj) 
			   tOr(tIntPos,tVoid) tOr(tIntPos,tVoid),tVoid),
		     tFunc(tIntPos tOr(tByte,tVoid),tVoid)), 0);

#if defined(HAVE_SYS_SHM_H) || defined(WIN32SHM)
   ADD_FUNCTION("shmat", memory_shm, tFunc(tInt tInt, tInt), 0 );
#endif

#ifdef HAVE_MMAP
   ADD_FUNCTION("mmap",memory_mmap,
		tFunc(tOr(tStr,tObj) 
		      tOr(tIntPos,tVoid) tOr(tIntPos,tVoid),tInt),0);
   ADD_FUNCTION("mmap_private",memory_mmap_private,
		tFunc(tOr(tStr,tObj) 
		      tOr(tIntPos,tVoid) tOr(tIntPos,tVoid),tInt),0);
#endif

   ADD_FUNCTION("allocate",memory_allocate,
		tFunc(tIntPos tOr(tByte,tVoid),tVoid),0);
   
   ADD_FUNCTION("free",memory_free,tFunc(tVoid,tVoid),0);

   ADD_FUNCTION("valid",memory_valid,tFunc(tVoid,tInt01),0);
   ADD_FUNCTION("writeable",memory_writeable,tFunc(tVoid,tInt01),0);

   ADD_FUNCTION("_sizeof",memory__sizeof,tFunc(tVoid,tIntPos),0);
   ADD_FUNCTION("cast",memory_cast,
		tFunc(tStr,tOr(tArr(tInt),tStr)),0);

   ADD_FUNCTION("`[]",memory_index,
		tOr(tFunc(tInt,tInt),
		    tFunc(tInt tInt,tStr)),0);

   ADD_FUNCTION("`[]=",memory_index_write,
		tOr(tFunc(tInt tInt,tInt),
		    tFunc(tInt tInt tStr,tStr)),0);

   ADD_FUNCTION("pread",memory_pread,tFunc(tInt tInt,tStr),0);
   ADD_FUNCTION("pread16",memory_pread16,tFunc(tInt tInt,tStr),0);
   ADD_FUNCTION("pread32",memory_pread32,tFunc(tInt tInt,tStr),0);
   ADD_FUNCTION("pread16i",memory_pread16i,tFunc(tInt tInt,tStr),0);
   ADD_FUNCTION("pread32i",memory_pread32i,tFunc(tInt tInt,tStr),0);
   ADD_FUNCTION("pread16n",memory_pread16n,tFunc(tInt tStr,tInt),0);
   ADD_FUNCTION("pread32n",memory_pread32n,tFunc(tInt tStr,tInt),0);

   ADD_FUNCTION("pwrite",memory_pwrite,tFunc(tInt tStr,tInt),0);
   ADD_FUNCTION("pwrite16",memory_pwrite16,tFunc(tInt tStr,tInt),0);
   ADD_FUNCTION("pwrite32",memory_pwrite32,tFunc(tInt tStr,tInt),0);
   ADD_FUNCTION("pwrite16i",memory_pwrite16i,tFunc(tInt tStr,tInt),0);
   ADD_FUNCTION("pwrite32i",memory_pwrite32i,tFunc(tInt tStr,tInt),0);
   ADD_FUNCTION("pwrite16n",memory_pwrite16n,tFunc(tInt tStr,tInt),0);
   ADD_FUNCTION("pwrite32n",memory_pwrite32n,tFunc(tInt tStr,tInt),0);

   set_init_callback(init_memory);
   set_exit_callback(exit_memory);
   end_class("Memory",0);

#ifdef PAGE_SIZE
   ADD_INT_CONSTANT("PAGE_SIZE",PAGE_SIZE,0);
#endif
#ifdef PAGE_SHIFT
   ADD_INT_CONSTANT("PAGE_SHIFT",PAGE_SHIFT,0);
#endif
#ifdef PAGE_MASK
   ADD_INT_CONSTANT("PAGE_MASK",PAGE_MASK,0);
#endif

#ifdef HAVE_MMAP
   ADD_INT_CONSTANT("__MMAP__",1,0);
#endif
}
