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

/* #define MSL_COUNTER */
#ifdef MSL_COUNTER
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <unistd.h>

#include "internal.h"

#include "dmalloc.h"

static const char RCSID[]=
   "$Id$";

#ifdef MASSIVE_DEBUG
#define USAGE_DEBUG
#endif
#ifdef USAGE_DEBUG
#include <stdio.h>
#else
#ifdef MSL_COUNTER
#include <stdio.h>
#endif
#endif


/*
 * a status list is an open hash table
 */

#define MUL_UNKNOWN 0 /* must be zero */
#define MUL_USED 1
#define MUL_FREE 2

#define MUL_INIT_SIZE 128

#define MUL_GROW_THRESHOLD(ENTRIES,SIZE) ((ENTRIES)*3>(SIZE)*2)

MIRD_RES mird_status_new(struct mird *db,struct mird_status_list **mul)
{
   MIRD_RES res;

   if ( (res=MIRD_MALLOC(sizeof(struct mird_status_list),mul)) )
      return res;
 
   mul[0]->size=MUL_INIT_SIZE;

   if ( (res=MIRD_MALLOC(sizeof(struct mul_entry*)*mul[0]->size,
			 &(mul[0]->entries))) )
      return res;

   memset(mul[0]->entries,0,sizeof(struct mul_entry*)*mul[0]->size);

   mul[0]->first_pool=NULL;
   mul[0]->pool_pos=POOL_ENTRIES;
   mul[0]->n=0;
   mul[0]->lock=0;

#ifdef MSL_COUNTER
   mul[0]->c1=0;
   mul[0]->c2=0;
   mul[0]->c3=0;
   gettimeofday(&(mul[0]->tv),NULL);
#endif

   return MIRD_OK;
}

void mird_status_free(struct mird_status_list *mul)
{
   struct mul_pool *mulp;

#ifdef MSL_COUNTER
   struct timeval tv2;
   gettimeofday(&tv2,NULL);

   fprintf(stderr,"status usage: size %ld/%ld, set %ld, get %ld, "
	   "marks %ld, time %.5f\n",
	   mul->n,mul->size,mul->c1,mul->c2,mul->c3,
	   (tv2.tv_usec-mul->tv.tv_usec)*1e-6+(tv2.tv_sec-mul->tv.tv_sec));
#endif

   while ( (mulp=mul->first_pool) )
   {
      mul->first_pool=mulp->next;
      sfree(mulp);
   }

   sfree(mul->entries);
   mul->entries=NULL; /* precaution */

   sfree(mul);
}

#define IND(X,Y) (((X)*12841)+((Y)*89189))

MIRD_RES mird_status_set(struct mird_status_list *mul,
			 UINT32 x,UINT32 y,UINT32 status)
{
   MIRD_RES res;

   struct mul_entry **entp=mul->entries+(IND(x,y)&(mul->size-1));
      
   struct mul_entry *ent;

#ifdef MSL_COUNTER
   mul->c1++; /* set */
#endif

#ifdef USAGE_DEBUG
   fprintf(stderr,"set %lxh:%lxh (%ld:%ld) status to %d\n",
	   x,y,x,y,status);
#endif

   ent=*entp;
   while (ent)
   {
      if (ent->x==x && ent->y==y)
      {
	 if (mul->lock && ent->status!=status)
	 {
#ifdef USAGE_DEBUG
	    fprintf(stderr,"changing mul entry status (%d->%d) %ld:%ld\n",
		    ent->status,status,x,y);
#endif
	    return mird_generate_error_s(MIRDE_GARBLED,
					 "changing used status",0,0,0);
	 }
	 ent->status=status;
	 return MIRD_OK; /* already there */
      }
      ent=ent->next;
   }

   if (MUL_GROW_THRESHOLD(mul->n,mul->size))
   {
      struct mul_entry **entries;
      long n;
      long mod=mul->size*2-1;

      if ( (res=MIRD_MALLOC(sizeof(struct mul_entry*)*mul->size*2,
			    &entries)) )
	 return res;

      memset(entries,0,sizeof(struct mul_entry*)*mul->size*2);

      n=mul->size;
      entp=mul->entries;
      while (n--)
      {
	 while (*entp)
	 {
	    ent=*entp;
	    entp[0]=ent->next;
	    ent->next=entries[IND(ent->x,ent->y)&mod];
	    entries[IND(ent->x,ent->y)&mod]=ent;
	 }
	 entp++;
      }
      sfree(mul->entries);
      mul->entries=entries;
      mul->size*=2;
      entp=(mul->entries+(IND(x,y)&(mul->size-1)));
   }

   if (mul->pool_pos==POOL_ENTRIES)
   {
      struct mul_pool *pool;
      if ( (res=MIRD_MALLOC(sizeof(struct mul_pool),&pool)) )
	 return res;
      pool->next=mul->first_pool;
      mul->first_pool=pool;
      mul->pool_pos=0;
   }
   ent=((struct mul_entry*)mul->first_pool->entries)+mul->pool_pos++;
   ent->next=*entp;
   ent->x=x;
   ent->y=y;
   ent->status=status;
   *entp=ent;
   mul->n++;

   return MIRD_OK;
}

MIRD_RES mird_status_get(struct mird_status_list *mul,
			 UINT32 x,UINT32 y,UINT32 *status)
{
   struct mul_entry **entp=&(mul->entries[IND(x,y)&(mul->size-1)]);
   struct mul_entry *ent;

#ifdef MSL_COUNTER
   mul->c2++;
#endif

   ent=*entp;
   while (ent)
   {
      if (ent->x==x &&
	  ent->y==y)
      {
	 status[0]=ent->status;
#ifdef USAGE_DEBUG
	 fprintf(stderr,"get %lxh:%lxh (%ld:%ld) status: %d\n",
		 x,y,x,y,status[0]);
#endif
	 return MIRD_OK; /* already there */
      }
      ent=ent->next;
   }
   status[0]=MUL_UNKNOWN;
#ifdef USAGE_DEBUG
   fprintf(stderr,"get %lxh:%lxh (%ld:%ld) status: unknown\n",
	   x,y,x,y);
#endif
   return MIRD_OK;
}

MIRD_RES mird_mark_used(struct mird_status_list *mul,UINT32 block_no)
{
   return mird_status_set(mul,block_no,0,MUL_USED);
}

MIRD_RES mird_mark_free(struct mird_status_list *mul,UINT32 block_no)
{
   return mird_status_set(mul,block_no,0,MUL_FREE);
}


/*
 * checks one block for usage 
 */

static MIRD_RES mird_status_check_block(struct mird *db,
					struct mird_status_list *mul,
					struct mird_status_list *checked,
					UINT32 block_no,
					int survey)
{
   UINT32 table_id;
   unsigned char *bdata;
   UINT32 *a,n,*keys,k,key;
   UINT32 cont=0;
   MIRD_RES res;

   if (block_no>db->last_used)
      return MIRD_OK; /* it's free, and we don't care */

   keys=((UINT32*)db->buffer);

#ifdef USAGE_DEBUG
   if (!survey) fprintf(stderr,"check block %ld\n",block_no);
#endif

   if ( (res=mird_block_get(db,block_no,&bdata)) )
   {
      /* broken! definitely free */
      mird_free_error(res);
      if (!survey) 
      {
#ifdef USAGE_DEBUG
	 fprintf(stderr,"block %ld broken (free<tm>) \n",block_no);
#endif
	 if ( (res=mird_freelist_push(db,block_no)) )
	    return res;
      }
      if ( (res=mird_mark_free(mul,block_no)) )
	 return res;
      return MIRD_OK;
   }

   table_id=READ_BLOCK_LONG(bdata,3);

   switch (READ_BLOCK_LONG(bdata,2))
   {
      case BLOCK_BIG:
	 cont=READ_BLOCK_LONG(bdata,4);
	 keys[0]=READ_BLOCK_LONG(bdata,6);
	 k=1;
	 break;

      case BLOCK_FRAG:
	 a=(UINT32*)(bdata+16);
	 k=0;

	 n=1<<db->frag_bits;
	 while (n-- && a[1])
	 {
	    key=READ_BLOCK_LONG(bdata+READ_BLOCK_LONG(a,0),1);
	    if (!k || keys[k-1]!=key) /* quick optimization :) */
	       keys[k++]=key;
#ifdef USAGE_DEBUG
	    if (!survey) fprintf(stderr,"add key %lxh [%ld]\n",key,k);
#endif
	    a++;
	 }
	 break;

      case BLOCK_FRAG_PROGRESS:
      case BLOCK_FREELIST:
	 /* definitely free */
	 if (!survey) 
	    if ( (res=mird_freelist_push(db,block_no)) )
	       return res;
	 if ( (res=mird_mark_free(mul,block_no)) )
	    return res;
	 return MIRD_OK;

      default:
	 return mird_generate_error(MIRDE_WRONG_BLOCK,block_no,
				    0,READ_BLOCK_LONG(bdata,2));
   }

   while (k--)
   {
#ifdef USAGE_DEBUG
      fprintf(stderr,"check key %lxh\n",keys[k]);
#endif
      if ( checked &&
	   (res=mird_status_get(checked,table_id,keys[k],&n)) )
	 return res;

      if (n==1) continue; /* checked that already */

      if ( checked &&
	   (res=mird_status_set(checked,table_id,keys[k],1)) )
	 return res;

      if ( (res=mird_tables_mark_use(db,db->tables,table_id,
				     keys[k],mul)) )
	 return res;
#ifdef MSL_COUNTER
      mul->c3++;
#endif
   }

   if ( (res=mird_status_get(mul,block_no,0,&n)) )
      return res;

   if (n==MUL_UNKNOWN) /* so... free this block */
   {
#ifdef USAGE_DEBUG
      fprintf(stderr,"free block %ld\n",block_no);
#endif

      if (!survey) 
	 if ( (res=mird_freelist_push(db,block_no)) )
	    return res;
      if ( (res=mird_mark_free(mul,block_no)) )
	 return res;

      while (cont)
      {
	 UINT32 b=CHUNK_ID_2_BLOCK(db,cont);
	 n=CHUNK_ID_2_FRAG(db,cont);
	 if (n)
	    return mird_status_check_block(db,mul,checked,b,survey);

	 if ( (res=mird_block_get(db,b,&bdata)) )
	 {
	    /* broken, don't continue */
	    mird_free_error(res);
	    return MIRD_OK;
	 }
	 cont=READ_BLOCK_LONG(bdata,4);
	 if (!survey)
	 {
#ifdef USAGE_DEBUG
	    fprintf(stderr,"free cont block %ld\n",b);
#endif
	    if ( (res=mird_freelist_push(db,b)) )
	       return res;
	 }
	 if ( (res=mird_mark_free(mul,b)) )
	    return res;
      }
   }
   return MIRD_OK;
}

/*
 * reads the journal file and frees unused block 
 */

MIRD_RES mird_check_usage(struct mird *db)
{
   MIRD_RES res;
   UINT32 *ent=NULL,*cur;
   off_t pos=0;
   UINT32 n;

   const UINT32 unusen=htonl(MIRDJ_BLOCK_UNUSED);

   struct mird_status_list *mul=NULL;
   struct mird_status_list *checked=NULL;

#ifdef USAGE_DEBUG
   fprintf(stderr,"check usage\n");
#endif

   if ( (res=MIRD_MALLOC(MIRD_JOURNAL_ENTRY_SIZE*db->journal_readback_n,
			 &ent)) )
      return res;

   if ( (res=mird_status_new(db,&mul)) )
      goto clean;
   if (! (db->flags & MIRD_NO_KEY_CHECKED_LIST) )
      if ( (res=mird_status_new(db,&checked)) )
	 goto clean;

   mul->lock=1; /* crash if we change a status */
   if (checked) checked->lock=1;

   for (;;)
   {
      if ( (res=mird_journal_get(db,pos,
				 db->journal_readback_n,
				 (unsigned char*)ent,&n)) )
	 goto clean; 
      
#ifdef USAGE_DEBUG
      fprintf(stderr,"check usage n=%ld\n",n);
#endif

      if (!n)
      {
	 /* all done */
	 res=MIRD_OK;
	 goto clean; 
      }

      cur=ent;
      while (n--)
      {
#ifdef USAGE_DEBUG
	 fprintf(stderr,"check usage %c%c%c%c %ld:%ld %ld\n",
		 FOURC(READ_BLOCK_LONG(cur,0)),
		 READ_BLOCK_LONG(cur,1),
		 READ_BLOCK_LONG(cur,2),
		 READ_BLOCK_LONG(cur,3));
#endif
	 if (cur[0]==unusen)
	 {
	    UINT32 z;

	    if ( (res=mird_status_get(mul,READ_BLOCK_LONG(cur,3),0,&z)) )
	       goto clean;

	    if (z==MUL_UNKNOWN &&
		(res=mird_status_check_block(db,mul,checked,
					     READ_BLOCK_LONG(cur,3),0)))
	       goto clean;
	 }
	 cur+=MIRD_JOURNAL_ENTRY_SIZE/4;
	 pos+=MIRD_JOURNAL_ENTRY_SIZE;
      }
   }
clean:
   if (ent) sfree(ent);
   if (mul) mird_status_free(mul);
   if (checked) mird_status_free(checked);

   return res;
}

/*
 * debug: loop over all blocks,
 * check if the freelist agrees which blocks are free
 *
 * print out those block that doesn't agree (should be none)
 * (call this directly after sync)
 */

#include <stdio.h>

void mird_debug_check_free(struct mird *db,int silent)
{
   struct mird_status_list *mul;
   struct mird_status_list *checked;
   struct mird_status_list *freel;
   UINT32 b,n,i,z;
   MIRD_RES res;
   UINT32 irregular=0;

   if ( (res=mird_status_new(db,&mul)) ) goto clean;
   if ( (res=mird_status_new(db,&checked)) ) goto clean;
   if ( (res=mird_status_new(db,&freel)) ) goto clean;

   mul->lock=checked->lock=freel->lock=1; /* just to make sure */

   if (!silent) fprintf(stderr,"debug_check_free...\n");

   /* read in free list */
   if (db->free_list.n)
   {
      fprintf(stderr,"debug_check_free: free_list has read-in entries\n"
	      "call this immediately after sync!\n");
      return;
   }

   b=db->free_list.next;
   while (b)
   {
      unsigned char *data;
      if ( (res=mird_block_get(db,b,&data)) )
	 goto clean;

      if ( (res=mird_status_set(mul,b,0,MUL_USED)) ) /* mark freelist */
	 goto clean;

      for (i=0; i<READ_BLOCK_LONG(data,4); i++)
	 if ( (res=mird_status_set(freel,READ_BLOCK_LONG(data,5+i),0,1)) )
	    goto clean;

      b=READ_BLOCK_LONG(data,3);
   }
   
   if (!silent || (freel->n>=db->last_used))
   {
      fprintf(stderr,"free blocks......%ld (%ld%%)\n",freel->n,
	      (100*freel->n)/(db->last_used+1));
      fprintf(stderr,"used blocks......%ld\n",db->last_used+1);
   }

   for (b=0; b<db->last_used; b++)
   {
      for (i=1; i<b+2; i<<=2)
	 if (i-1==b) goto loop; /* superblock, skip */

      if ( (res=mird_status_get(mul,b,0,&z)) ) goto clean;
      
      if (z==MUL_UNKNOWN &&
	  (res=mird_status_check_block(db,mul,checked,b,1)))
	 goto clean;

      if ( (res=mird_status_get(mul,b,0,&z)) ) goto clean;
      if ( (res=mird_status_get(freel,b,0,&n)) ) goto clean;

      if (n==1 && z==MUL_USED)
      {
	 fprintf(stderr,"block %lxh (%ld) is marked free but in use:\n",b,b);
	 mird_describe_block(db,b);
	 irregular=1;
      }
      else if (n==0 && z==MUL_FREE)
      {
	 fprintf(stderr,"block %lxh (%ld) is free but not marked free:\n",b,b);
	 mird_describe_block(db,b);
	 irregular=1;
      }
loop:
      ;
   }
   
clean:   
   if (freel) mird_status_free(freel);
   if (mul) mird_status_free(mul);
   if (checked) mird_status_free(checked);

   if (res) 
   {
      mird_perror("mird_debug_check_free",res);
      mird_free_error(res);
   }
   if (irregular) mird_fatal("irregular\n");
}
