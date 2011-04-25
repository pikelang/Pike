/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
** libMird by Mirar <mirar@mirar.org>
** please submit bug reports and patches to the author
**
** also see http://www.mirar.org/mird/
*/

#include "internal.h"

#include <stdlib.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif


#include "config.h"

#define IS_MEM_C
#include "dmalloc.h"

#ifdef MEM_STATS
#define MAX_MEM_SIZE 8192
static unsigned long counter[MAX_MEM_SIZE];
static unsigned long counter2[MAX_MEM_SIZE];
#endif

/* ---------------------------------------------------------------- */

#ifdef MEM_DEBUG

#define BORDERSZ 16
#define FILENAMESZ 32
/*
#define DEBUG_SIZE 10000
*/
/*#define DEBUG*/

#include <stdio.h>

unsigned long mem_allocated=0,mem_allocations=0,mem_frees=0,mem_reallocs=0;
static unsigned long serial=0;

struct memnode
{
#ifdef BORDERSZ
   unsigned char border1[BORDERSZ];
#endif
   unsigned char check1;
   unsigned long sz;
   char file[FILENAMESZ];
   unsigned long line;
   unsigned short alloc;
   unsigned char check2;
#ifdef BORDERSZ
   unsigned char border2[BORDERSZ];
#endif
#define ISALLOC 4711u
   struct memnode *next,*prev;
} *firstmem=NULL;

#ifdef BORDERSZ

INLINE int test_border(unsigned char *l,unsigned char t)
{
   int i;
   for (i=0; i<BORDERSZ; i++) if (l[i]!=t) break;
   return i;
}

INLINE void write_border(unsigned char t,unsigned char *l)
{
   int i;
   for (i=0; i<BORDERSZ; i++) l[i]=t;
}

INLINE void test_for_borderbreak(struct memnode *mn, char *file,
				 int line,char *type)
{
   int i;
   if ((i=test_border(mn->border1,mn->check1))!=BORDERSZ)
      fprintf(stderr,"BORDER BREAK: %s from %s:%d front border, "
	      "byte %d/%ld\n",
	      type,file,line,i-sizeof(struct memnode),mn->sz);
   if ((i=test_border(mn->border2,mn->check1))!=BORDERSZ)
      fprintf(stderr,"BORDER BREAK: %s from %s:%d mid border, "
	      "byte %d/%ld\n",
	      type,file,line,i-BORDERSZ,mn->sz);
   if ((i=test_border(((unsigned char*)(mn+1)+mn->sz),
		      mn->check1))!=BORDERSZ)
      fprintf(stderr,"BORDER BREAK: %s from %s:%d "
	      "back border, byte %d/%ld\n",
	      type,file,line,i,mn->sz);
}

#endif /* BORDERSZ */

void *_smalloc(unsigned long sz,char *file,int line)
{
   void *m;
   struct memnode *mn;
   if (sz==0) sz=1;
#ifdef MEM_STATS
   if (sz<MAX_MEM_SIZE) counter[sz]++;
#endif
#ifdef BORDERSZ
   m=malloc(sz+sizeof(struct memnode)+BORDERSZ*sizeof(long));
#else
   m=malloc(sz+sizeof(struct memnode));
#endif
   if (!m) return NULL;
   mn=m;
   mn->check2=(unsigned char)(serial^~(((serial+0x5555)<<16)+0x5555));
   mn->check1=(unsigned char)~mn->check2;
   serial++;
   mn->alloc=ISALLOC;
   mn->sz=sz;
   strncpy(mn->file,file,FILENAMESZ);
   mn->line=line;
   mn->prev=NULL;
   if (firstmem) firstmem->prev=mn;
   mn->next=firstmem;
   firstmem=mn;
#ifdef DEBUG_SIZE
   if (sz>DEBUG_SIZE) 
      printf("allocate: %x <%x> (%x<>%x) %ld bytes (%s line %d)\n",
	     mn+1,mn,mn->prev,mn->next,sz,file,line);
#endif
#ifdef DEBUG
   printf("allocate: %x <%x> (%x<>%x) %ld bytes (%s line %d)\n",
          mn+1,mn,mn->prev,mn->next,sz,file,line);
   fflush(stdout);
#endif
   mem_allocated+=sz;
#ifdef BORDERSZ
   write_border(mn->check1,mn->border1);
   write_border(mn->check1,mn->border2);
   write_border(mn->check1,((unsigned char*)(mn+1)+sz));
#endif
   mem_allocations++;
   return (void*)(((unsigned char*)m)+sizeof(struct memnode));
}

void _sfree(void *m,char *file,int line)
{
   struct memnode *mn;

   if (!m)
   {
      fprintf(stderr,"free of null; freed from %s line %d\n",
	      file,line);
      abort();
   }

   m=(((unsigned char*)m)-sizeof(struct memnode));
   mn=m;
   if (mn->alloc==(unsigned short)~ISALLOC)
   {
      fprintf(stderr,"free of freed object %p <%p>; alloced from %s line %ld, freed from %s line %d\n",
	      mn+1,mn,mn->file,mn->line,file,line);
      abort();
   }
   if ((((unsigned long)m)>>24)==0xff || mn->alloc!=ISALLOC)
   {
      fprintf(stderr,"free of non-allocated object %p from %s line %d\n",
	      m,file,line);
      abort();
   }
#ifdef BORDERSZ
   test_for_borderbreak(mn,file,line,"free");
#endif
#ifdef DEBUG
   printf("free: %x <%x> (%x<>%x) %ld bytes (%s line %d) %s line %d\n",
          mn+1,mn,mn->prev,mn->next,mn->sz,mn->file,mn->line,file,line);
   fflush(stdout);
#endif
   if (mn->sz>=0xfffffff || mn->alloc==(unsigned short)~ISALLOC)
   {
      fprintf(stderr,"free of already freed object from %s line %d\n",
	      file,line);
      if (mn->alloc==(unsigned short)~ISALLOC)
	 fprintf(stderr,"allocated from %s line %ld\n",
		 mn->file,mn->line);
      abort();
   }
#ifdef DEBUG_SIZE
   if (mn->sz>DEBUG_SIZE) 
      printf("free: %x <%x> (%x<>%x) %ld bytes (%s line %d) %s line %d\n",
	     mn+1,mn,mn->prev,mn->next,mn->sz,mn->file,mn->line,file,line);
#endif
   if (mn->check1!=(unsigned char)~mn->check2)
   {
      fprintf(stderr,"memory disrupted (at point of free) from %s line %d\n",
	      file,line);
      abort();
   }
   mem_allocated-=mn->sz;
   if (mn->prev) mn->prev->next=mn->next;
   else firstmem=mn->next;
   if (mn->next) mn->next->prev=mn->prev;
#if 0
   memset(m,255,mn->sz+sizeof(struct memnode));
#else
   memset(mn+1,255,mn->sz);
#endif
   mn->sz=0xffffffff;
   mn->prev=mn->next=NULL;
   mn->alloc=(unsigned short)~ISALLOC;

#if 0
   strncpy(mn->file,file,FILENAMESZ);
   mn->line=line;
#endif

#if 0
   free(m);
#endif
   mem_frees++;
}

void *_srealloc(void *m,unsigned long sz,char *file,int line)
{
   struct memnode *mn;

   if (!m) return smalloc(sz);

   mn=(struct memnode*)(((unsigned char*)m)-sizeof(struct memnode));
#ifdef DEBUG
   printf("realloc: %x (%x<>%x) %ld -> %ld bytes (%s line %d)\n",
          mn,mn->prev,mn->next,mn->sz,sz,mn->file,mn->line);
   fflush(stdout);
#endif
#ifdef DEBUG_SIZE
   if (sz>DEBUG_SIZE) 
      printf("realloc: %x (%x<>%x) %ld -> %ld bytes (%s line %d)\n",
	     mn,mn->prev,mn->next,mn->sz,sz,mn->file,mn->line);
#endif
   if ((((unsigned long)m)>>24)==0xff || mn->alloc!=ISALLOC)
   {
      fprintf(stderr,"realloc of non-allocated object %p from %s line %d\n",
	      m,file,line);
      abort();
   }
#ifdef BORDERSZ
   test_for_borderbreak(mn,file,line,"realloc");
#endif
   if (mn->check1!=~mn->check2)
   {
      fprintf(stderr,"memory disrupted (at point of realloc) from %s line %d\n",file,line);
      abort();
   }
   mem_allocated-=mn->sz;
   if (sz==0) sz=1;
#ifdef BORDERSZ
   mn=realloc(mn,sz+sizeof(struct memnode)+sizeof(unsigned long)*BORDERSZ);
   write_border(mn->check1,((unsigned char*)(mn+1)+sz));
#else
   mn=realloc(mn,sz+sizeof(struct memnode));
#endif
   if (!mn) 
   {
      fprintf(stderr,"out of memory during realloc (from %s line %d)\n",
	      file,line);
      abort();
   }
   mn->sz=sz;
   mn->alloc=ISALLOC;
   mem_allocated+=mn->sz;
   if (mn->prev) mn->prev->next=mn; else firstmem=mn;
   if (mn->next) mn->next->prev=mn;
   mem_reallocs++;
   return (void*)(((unsigned char*)mn)+sizeof(struct memnode));
}

char *_sstrdup(char *s,char *file,int line)
{
   char *d=_smalloc(strlen(s)+1,file,line);
   if (!d) return NULL;
   memcpy(d,s,strlen(s)+1);
   return d;
}

static int memory_dump(void)
{
   struct memnode *mn;
   unsigned char *d;
   unsigned long i;

   fprintf(stderr,"allocations: %ld  frees: %ld  reallocs:  %ld\n",
	   mem_allocations,mem_frees,mem_reallocs);

   if (firstmem)
   {
      mn=firstmem;
      while (mn)
      {
	 fprintf(stderr,"%8lx %ld bytes allocated in %s line %lu (%lx<>%lx)\n      ",
		 (unsigned long)((unsigned char*)mn)+sizeof(struct memnode),mn->sz,mn->file,mn->line,(unsigned long)mn->prev,(unsigned long)mn->next);
	 d=(unsigned char*)(mn+1);
	 for (i=0; i<16&&i<mn->sz; i++) fprintf(stderr,"%02x ",d[i]);
	 for (i=0; i<16&&i<mn->sz; i++) fprintf(stderr,"%c",((d[i]&127)>31)?d[i]:'.');
	 fprintf(stderr,"\n");
	 mn=mn->next;
      }

      firstmem=NULL;
      mem_allocations=mem_frees=mem_reallocs=0;

      return 1;
   }

   mem_allocations=mem_frees=mem_reallocs=0;
   return 0;
}

#endif /* MEM_DEBUG */

/* ---------------------------------------------------------------- */

#ifndef HAS_INLINE
int mem_eq(unsigned char *a,unsigned char *b,unsigned long len)
{
   while (--len) if (*(a++)!=*(b++)) return 0;
   return 1;
}
void _smemmove(unsigned char *dest,unsigned char *src,unsigned long sze) 
{ 
   if (dest>src) { dest+=sze; src+=sze; while (sze--) { (*--dest)=(*--src); } }
   else { while (sze--) { (*(dest++))=(*(src++)); } } 
}
#endif



int mird_check_mem(void)
{
#ifdef MEM_STATS
   unsigned long i;
   for (i=0; i<MAX_MEM_SIZE; i++)
      if (counter[i]) 
	 fprintf(stderr,"%8lub%6ld%6ld\n",i,counter[i],counter2[i]);
#endif

#ifdef MEM_DEBUG
   return memory_dump();
#else
   return 0;
#endif
}
