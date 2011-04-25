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

/* handles blocks, freelist and fragmented blocks */

#include "internal.h"

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "dmalloc.h"

static const char RCSID[]=
   "$Id$";

#ifdef SUPERMASSIVE_DEBUG
#define BLOCK_DEBUG
#endif
#ifdef BLOCK_DEBUG
#include <stdio.h>
#endif


MIRD_RES mird_low_block_read(struct mird *db,UINT32 block,
			     unsigned char *dest,
			     UINT32 n)
{
   int fd=db->db_fd;
   MIRD_OFF_T res;

#ifdef BLOCK_DEBUG
   fprintf(stderr,"! read block %lxh\n",block);
#endif

   MIRD_SYSCALL_COUNT(db,4);
   res=LSEEK(fd,((MIRD_OFF_T)block)*db->block_size,SEEK_SET);
   if (res==-1)
      return mird_generate_error(MIRDE_DB_LSEEK,block,errno,0);
   
   for (;;)
   {
      MIRD_SYSCALL_COUNT(db,5);
      res=(MIRD_OFF_T)read(fd,(void*)dest,db->block_size*n);
      if (res==-1) {
	 if (errno==EINTR) 
	    continue; /* try again */
	 else 
	    return mird_generate_error(MIRDE_DB_READ,block,errno,0);
      }
      if (res!=(MIRD_OFF_T)(db->block_size*n))
	 return mird_generate_error(MIRDE_DB_READ_SHORT,
				    block,res,db->block_size*n);

/*        fprintf(stderr,"! read block %lxh [%c%c%c%c]\n",block, */
/*  	      FOURC(READ_BLOCK_LONG(dest,2))); */

      return MIRD_OK;
   }
}

MIRD_RES mird_low_block_write(struct mird *db,UINT32 block,
			      unsigned char *data,UINT32 n)
{
   int fd=db->db_fd;
   MIRD_OFF_T res;

#ifdef BLOCK_DEBUG
   fprintf(stderr,"! write block %lxh\n",block);
#endif

   MIRD_SYSCALL_COUNT(db,4);
   res=LSEEK(fd,((MIRD_OFF_T)block)*db->block_size,SEEK_SET);
   if (res==-1)
      return mird_generate_error(MIRDE_DB_LSEEK,block,errno,0);
   
   for (;;)
   {
      MIRD_SYSCALL_COUNT(db,6);
      res=(MIRD_OFF_T)write(fd,(void*)data,db->block_size*n);
      if (res==-1) {
	 if (errno==EINTR) 
	    continue; /* try again */
	 else 
	    return mird_generate_error(MIRDE_DB_WRITE,block,errno,0);
      }
      if (res!=(MIRD_OFF_T)(db->block_size*n))
	 return mird_generate_error(MIRDE_DB_WRITE_SHORT,
				     block,res,db->block_size*n);

      return MIRD_OK;
   }
}

/*
 * writes checksum to a block and the block to disk
 */

MIRD_RES mird_block_write(struct mird *db,UINT32 block,
			    unsigned char *data)
{
   long checksum;

   checksum=mird_checksum((UINT32*)data,(db->block_size>>2)-1);
   WRITE_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1,checksum);

   return mird_low_block_write(db,block,data,1);
}

/*
 * reads a block from disk and checks it's checksum
 * note: data space must be allocated previous to call
 */

MIRD_RES mird_block_read(struct mird *db,UINT32 block,
			   unsigned char *data)
{
   MIRD_RES res;
   UINT32 checksum1,checksum2;

   res=mird_low_block_read(db,block,data,1);
   if (res) return res;

   checksum1=mird_checksum((UINT32*)data,(db->block_size>>2)-1);
   checksum2=READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1);

   if (checksum1!=checksum2)
      return mird_generate_error(MIRDE_DB_READ_CHECKSUM,block,0,0);
   
   return MIRD_OK;
}


/*****************************************************************
  mird disk cache system
*****************************************************************/

struct block_cache
{
   UINT32 block_no; /* host byte order always; 4x2 bytes = */
   UINT32 flags;    /* 8 bytes */
#define BLOCK_CACHE_HEADER 8

#define BLOCK_CACHE_UNUSED 1
#define BLOCK_CACHE_DIRTY  2

   UINT32 data[4];
};

#define BLOCK_CACHE_BLOCK_SIZE(DB) ((DB)->block_size+BLOCK_CACHE_HEADER)
#define BLOCK_DATA(BLOCK) ((unsigned char*)(&(BLOCK)->data))
#define DATA_BLOCK(DATA) \
        ((struct block_cache*)( ((unsigned char*)(DATA)) - BLOCK_CACHE_HEADER))
#define BLOCK_CACHE(DB,N) \
	((struct block_cache*)( (DB)->cache + BLOCK_CACHE_BLOCK_SIZE(DB)*(N)))

#define BLOCK_NO_HASH_VALUE(NO) ( (391828139*(NO)) )
#define BLOCK_NO_CACHE_START(DB,NO) ( (391828139*(NO))%((DB)->cache_size ) )

#define CACHE_LOOP(DB,I,N,Z,B)						\
	for ((I)=(DB)->cache_size,					\
	     (N)=BLOCK_CACHE_BLOCK_SIZE(DB),				\
	     (Z)=(DB)->cache;						\
	     (B)=(struct block_cache*)(Z),(I)--;			\
	     (Z)+=(N))

#define CACHE_SEARCH_LOOP(DB,START,I,N,Z,B,E)				\
	for ((I)=(DB)->cache_search_length,				\
	     (N)=BLOCK_CACHE_BLOCK_SIZE(DB),				\
	     (Z)=(DB)->cache+(N)*(START),				\
	     (E)=(DB)->cache+(N)*((DB)->cache_size-1);			\
	     (B)=(struct block_cache*)(Z),(I)--;			\
	     ((Z)==(E)?(Z)=(unsigned char*)(DB)->cache:((Z)+=(N))))
		

/*
 * initiates the cache
 */

MIRD_RES mird_cache_initiate(struct mird *db)
{
   long i,n;
   unsigned char *z;
   struct block_cache *b;

   db->journal_cache=smalloc((6*4)*db->journal_writecache_n);
   if (!db->journal_cache)
      return mird_generate_error(MIRDE_RESOURCE_MEM,
				  (6*4)*db->journal_writecache_n,
				  0,0);
   db->journal_cache_n=0;

   db->cache=smalloc(BLOCK_CACHE_BLOCK_SIZE(db)*db->cache_size);
   if (!db->cache)
      return mird_generate_error(MIRDE_RESOURCE_MEM,
				  BLOCK_CACHE_BLOCK_SIZE(db)*db->cache_size,
				  0,0);

   CACHE_LOOP(db,i,n,z,b)
      b->flags=BLOCK_CACHE_UNUSED;

   return MIRD_OK;
}

/*
 * frees the cache (hard; no check for dirty blocks)
 */

void mird_cache_free(struct mird *db)
{
   if (db->cache) sfree(db->cache);
   if (db->journal_cache) sfree(db->journal_cache);
}

/*
 * resets the cache (read-only sync purpose)
 */

void mird_cache_reset(struct mird *db)
{
   long i,n;
   unsigned char *z;
   struct block_cache *b;

   CACHE_LOOP(db,i,n,z,b)
      b->flags=BLOCK_CACHE_UNUSED;
}

/*
 * flushs one block
 */

MIRD_RES mird_cache_flush_block(struct mird *db,
				struct block_cache *b)
{
   MIRD_RES res;

#ifdef BLOCK_DEBUG
   fprintf(stderr,"flush cache %d; block %lxh [%c%c%c%c]\n",
	   (((unsigned char*)b)-db->cache)/
	   BLOCK_CACHE_BLOCK_SIZE(db),b->block_no,
	   FOURC(READ_BLOCK_LONG(b->data,2)));
#endif
   
   if ( (res=mird_block_write(db,b->block_no,(unsigned char*)&(b->data))) )
      return res;

   b->flags&=~BLOCK_CACHE_DIRTY;
   return MIRD_OK;
}

/*
 * flushs the cache
 */

MIRD_RES mird_cache_flush(struct mird *db)
{
   long i,n;
   unsigned char *z;
   struct block_cache *b;

   MIRD_RES res1=MIRD_OK;
   MIRD_RES res2;
   
   CACHE_LOOP(db,i,n,z,b)
      if ( (b->flags & BLOCK_CACHE_DIRTY) )
	 if ( (res2=mird_cache_flush_block(db,b)) )
	 { 
	    /* always try more, but report the first error */
	    if (res1) mird_free_error(res2);
	    else res1=res2;
	 }

   return res1;
}

/*
 * flushs one transaction in the cache
 */

MIRD_RES mird_cache_flush_transaction(struct mird_transaction *mtr)
{
   long i,n;
   unsigned char *z;
   struct block_cache *b;

   UINT32 msbn,lsbn;
   MIRD_RES res1=MIRD_OK;
   MIRD_RES res2;

   msbn=htonl(mtr->no.msb);
   lsbn=htonl(mtr->no.lsb);

   CACHE_LOOP(mtr->db,i,n,z,b)
      if ( b->data[1] == lsbn &&
	   b->data[0] == msbn &&
	   (b->flags & BLOCK_CACHE_DIRTY) )
	 if ( (res2=mird_cache_flush_block(mtr->db,b)) )
	 { /* always try more, but report the first error */
	    if (res1) mird_free_error(res2);
	    else res1=res2;
	 }

   return res1;
}
				       
/*
 * cancels one transaction in the cache
 */

MIRD_RES mird_cache_cancel_transaction(struct mird_transaction *mtr)
{
   long i,n;
   unsigned char *z;
   struct block_cache *b;

   UINT32 msbn,lsbn;

   msbn=htonl(mtr->no.msb);
   lsbn=htonl(mtr->no.lsb);

   CACHE_LOOP(mtr->db,i,n,z,b)
      if ( b->data[1] == lsbn &&
	   b->data[0] == msbn &&
	   (b->flags & BLOCK_CACHE_DIRTY) )
	 b->flags=BLOCK_CACHE_UNUSED; /* bwahaha */

   return MIRD_OK;
}
				       
/*
 * finds, finds a free or frees a block in the range for usage
 */

/* use clean blocks before flushing dirty ones */
#define FIRST_CLEAN
#define CLEAN_IS_NOT_FRAG

static MIRD_RES mird_cache_zot(struct mird *db,
				 UINT32 block_no,
				 struct block_cache **dest)
{
   long i,n;
   unsigned char *z,*e;
   struct block_cache *b;
   struct block_cache *unused=NULL;
#ifdef FIRST_CLEAN
   struct block_cache *clean=NULL;
#endif

   long start=BLOCK_NO_CACHE_START(db,block_no);

   if (block_no==0) /* wont happen! */
   {
      mird_fatal("zero block read\n");
      return mird_generate_error_s(MIRDE_GARBLED,"zero block read",0,0,0);
   }

   CACHE_SEARCH_LOOP(db,start,i,n,z,b,e)
      {
	 if ( (b->flags & BLOCK_CACHE_UNUSED) )
	    { if (!unused) unused=b; }
	 else if (b->block_no==block_no) /* found it! */
	 {
#ifdef BLOCK_DEBUG
	    fprintf(stderr,"found it at %d (>%d); block %xh (flags: %xh)\n",
		    (((unsigned char*)b)-db->cache)/n,
		    start,b->block_no,b->flags);
#endif
	    dest[0]=b;
	    return MIRD_OK;
	 }
#ifdef FIRST_CLEAN
#ifdef CLEAN_IS_NOT_FRAG
	 else if ( /* !(b->flags & BLOCK_CACHE_DIRTY) || */
		   (READ_BLOCK_LONG(b->data,2)!=BLOCK_FRAG_PROGRESS &&
		    READ_BLOCK_LONG(b->data,2)!=BLOCK_FRAG) )
	    { if (!clean) clean=b; }
#else
	 else if ( !(b->flags & BLOCK_CACHE_DIRTY) )
	    { if (!clean) clean=b; }
#endif
#endif
      }

   if (unused)
      dest[0]=unused;
#ifdef FIRST_CLEAN
   else if (clean)
   {
#ifdef CLEAN_IS_NOT_FRAG
      MIRD_RES res;

#ifdef BLOCK_DEBUG
      fprintf(stderr,"ditch cache %d (>%d); block %xh\n",n,start,b->block_no);
#endif
      if ( (clean->flags & BLOCK_CACHE_DIRTY) )
	 if ( (res=mird_cache_flush_block(db,clean)) ) 
	    return res; 
#endif
      dest[0]=clean;
   }
#endif
   else 
   {
      MIRD_RES res;
      n=(start+mird_random(db->cache_search_length))%db->cache_size;
      dest[0]=b=BLOCK_CACHE(db,n);

#ifdef BLOCK_DEBUG
      fprintf(stderr,"ditch cache %d (>%d); block %xh\n",n,start,b->block_no);
#endif
      if ( (b->flags & BLOCK_CACHE_DIRTY) )
	 if ( (res=mird_cache_flush_block(db,b)) ) 
	    return res; 
      return MIRD_OK;
   }

   return MIRD_OK;
}

/* fetch or prefetch block(s) */

/*  #define PREFETCH */

MIRD_RES mird_block_fetch(struct mird *db,
			  UINT32 block_no,
			  struct block_cache *b)
{
   MIRD_RES res;
#ifdef PREFETCH
   UINT32 a=block_no-20,n=40,c;
   unsigned char *buf=NULL;
   UINT32 checksum;

   if (a<1) a=1;

   if ( (res=MIRD_MALLOC(db->block_size*n,&buf)) )
      return res;
   
   if ( !(res=mird_low_block_read(db,a,buf,n)) )
   {
#ifdef BLOCK_DEBUG
      fprintf(stderr,"prefetched %lxh..%lxh for %lxh\n",
	      a,a+n-1,block_no);
#endif

      for (c=0; c<n; c++)
	 if (c+a!=block_no)
	 {
	    struct block_cache *bt;

	    if ( (res=mird_cache_zot(db,a+c,&bt)) )
	    {
	       sfree(buf);
	       return res; /* error */
	    }

	    if (bt->block_no!=c+a &&
		mird_checksum((UINT32*)(buf+db->block_size*c),
			      (db->block_size>>2)-1) ==
		READ_BLOCK_LONG((UINT32*)(buf+db->block_size*c),
				LONGS_IN_BLOCK(db)-1))
	    {
	       bt->block_no=c+a;
	       bt->flags=0;
	       memcpy(bt->data,buf+db->block_size*c,db->block_size);
/*  	       fprintf(stderr," - include %lxh\n",c+a); */
	    }
#ifdef BLOCK_DEBUG
	    else
	    {
	       if (bt->block_no==c+a)
		  fprintf(stderr," - disinclude %lxh (had it)\n",c+a);
	       else
		  fprintf(stderr," - disinclude %lxh (checksum %lxh!=%lxh)\n",
			  c+a,
			  mird_checksum((UINT32*)(buf+db->block_size*c),
					(db->block_size>>2)-1),
			  READ_BLOCK_LONG((UINT32*)(buf+db->block_size*c),
					  LONGS_IN_BLOCK(db)-1));
	    }
#endif
	 }

      c=block_no-a;

      b->block_no=block_no;
      b->flags=0;

      if ( mird_checksum((UINT32*)(buf+db->block_size*c),
			 (db->block_size>>2)-1) !=
	   READ_BLOCK_LONG((UINT32*)(buf+db->block_size*c),
			   LONGS_IN_BLOCK(db)-1) )
      {
	 sfree(buf);
	 return mird_generate_error(MIRDE_DB_READ_CHECKSUM,
				    block_no,0,0);
      }
      memcpy(b->data,buf+db->block_size*c,db->block_size);

      sfree(buf);

      return MIRD_OK;
   }

   sfree(buf);
   
/* fallback */
#endif

   if ( (res=mird_block_read(db,block_no,BLOCK_DATA(b))) )
   {
#ifdef BLOCK_DEBUG
      fprintf(stderr,"destroy cache %d\n",
	      (((unsigned char*)b)-db->cache)/
	      BLOCK_CACHE_BLOCK_SIZE(db));
#endif
      b->flags=BLOCK_CACHE_UNUSED; /* don't use this data! */
      return res; /* read error somehow */
   }
      
   return MIRD_OK;
}

/*
 * reads a block through cache
 */

MIRD_RES mird_block_get(struct mird *db,
			  UINT32 block_no,
			  unsigned char **dest)
{
   MIRD_RES res;
   struct block_cache *b;

#ifdef BLOCK_DEBUG
   fprintf(stderr,"get block  %xh\n",block_no);
#endif

   if ( (res=mird_cache_zot(db,block_no,&b)) )
      return res; /* error */

   if ((b->flags&BLOCK_CACHE_UNUSED) ||
       (b->block_no!=block_no)) /* "didn't find it, but use this space" */
   {
      if ( (res=mird_block_fetch(db,block_no,b)) )
	 return res;

      b->block_no=block_no;
      b->flags=0; /* neither dirty nor unused */
   }
#ifdef BLOCK_DEBUG
   fprintf(stderr,"get block -> found at %d\n",
	   (((unsigned char*)b)-db->cache)/
	   BLOCK_CACHE_BLOCK_SIZE(db));
#endif

   dest[0]=BLOCK_DATA(b);

   return MIRD_OK;
}

/*
 * reads a block through cache, mark it as dirty
 */

MIRD_RES mird_block_get_w(struct mird *db,
			    UINT32 block_no,
			    unsigned char **dest)
{
   MIRD_RES res;
   struct block_cache *b;

#ifdef BLOCK_DEBUG
   fprintf(stderr,"getw block %xh\n",block_no);
#endif

   if ( (res=mird_cache_zot(db,block_no,&b)) )
      return res; /* error */

   if (b->block_no!=block_no) /* "didn't find it, but use this space" */
   { 
      if ( (res=mird_block_fetch(db,block_no,b)) )
	 return res;

      b->block_no=block_no;
      b->flags=BLOCK_CACHE_DIRTY; 

/*        fprintf(stderr,"getw block %xh (%c%c%c%c)\n",block_no, */
/*  	      FOURC(READ_BLOCK_LONG(b->data,2))); */
   }
   else
      b->flags|=BLOCK_CACHE_DIRTY; 

   dest[0]=BLOCK_DATA(b);


   return MIRD_OK;
}

/*
 * points dest to clean block data
 * *do not fetch anything from disk*
 */

MIRD_RES mird_block_zot(struct mird *db,
			  UINT32 block_no,
			  unsigned char **dest)
{
   MIRD_RES res;
   struct block_cache *b;

#ifdef BLOCK_DEBUG
   fprintf(stderr,"zot  block %xh\n",block_no);
#endif

   if ( (res=mird_cache_zot(db,block_no,&b)) )
      return res; /* error */

   b->block_no=block_no;
   b->flags=BLOCK_CACHE_DIRTY; 

   dest[0]=BLOCK_DATA(b);

   /* clean this memory 
    * (for security but most of all database entropy purpose) */
   memset(dest[0],0,db->block_size); 

   return MIRD_OK;
}
