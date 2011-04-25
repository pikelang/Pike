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

/* handles frags
 * a frag is a small piece of data, of limited size
 */

#include <unistd.h>

#include "internal.h"

/* needed for memmove */
#ifdef HAVE_STRING_H
#include <string.h> 
#endif

#include <stdio.h>

static const char RCSID[]=
   "$Id$";


#define BLOCKS_IN_FREE_LIST(DB) (LONGS_IN_BLOCK(DB)-6)

static MIRD_RES mird_freelist_flush(struct mird *db,
				     struct mird_free_list *fl,
				     int block);

/*
 * pops one block from the freelist to use
 */

MIRD_RES mird_freelist_pop(struct mird *db,
			   UINT32 *block)
{
   if (db->free_list.n==0)
   {
      MIRD_RES res;
      unsigned char *data;
      UINT32 n;
      UINT32 push;
   
      /* no free ones left in memory list */
      
      if (db->free_list.next==0)
      {
	 /* no more free on disk */

	 UINT32 i;
retry:
	 block[0]=++db->last_used;
	 for (i=1; i-1<=db->last_used; i<<=2)
	    if (i-1==block[0]) goto retry; /* avoid superblocks */

	 return MIRD_OK;
      }

      /* read next list of free blocks */

      push=db->free_list.next;

      if ( (res=mird_block_get(db,push,&data)) )
	 return res;

      if ( READ_BLOCK_LONG(data,0)!=SUPERBLOCK_MIRD )
	 return mird_generate_error(MIRDE_ILLEGAL_FREE,
				    push,0,0);
      if ( READ_BLOCK_LONG(data,2)!=BLOCK_FREELIST )
	 return mird_generate_error(MIRDE_WRONG_BLOCK, push,
				    BLOCK_FREELIST, READ_BLOCK_LONG(data,2));

      db->free_list.next=READ_BLOCK_LONG(data,3);
      db->free_list.n=n=READ_BLOCK_LONG(data,4);
      while (n--)
	 db->free_list.blocks[n]=READ_BLOCK_LONG(data,5+n);

      /* free that old freelist block */
      if ( (res=mird_freelist_push(db,push)) )
	 return res;
      
      if (!db->free_list.n) 
	 return mird_freelist_pop(db,block);
   }

   block[0]=db->free_list.blocks[--db->free_list.n];
   return MIRD_OK;
}

/*
 * pushes one free block onto the list
 */

MIRD_RES mird_freelist_push(struct mird *db,
			    UINT32 block)
{
   UINT32 a,b,n;

   if (db->new_free_list.n==BLOCKS_IN_FREE_LIST(db))
   {
      UINT32 no;
      MIRD_RES res;
      if ( (res=mird_freelist_pop(db,&no)) )
	 return res; /* error */
      
      if ( (res=mird_freelist_flush(db,&(db->new_free_list),no)) )
	 return res; /* error */
   }

   /* lets perform a binary insert sort,
    * to defragment (or at least line up) later chains
    */

   a=0;
   b=db->new_free_list.n;

   while (a<b)
   {
      n=(a+b)/2;
      if (block>db->new_free_list.blocks[n]) a=n+1;
      else b=n;
   }

   memmove(db->new_free_list.blocks+b+1,
	   db->new_free_list.blocks+b,
	   sizeof(UINT32)*(db->new_free_list.n-b));

   db->new_free_list.blocks[b]=block;
   db->new_free_list.n++;

   return MIRD_OK;
}

/*
 * initiate the list
 */

MIRD_RES mird_freelist_initiate(struct mird *db)
{
   MIRD_RES res;
   if ( (res=MIRD_MALLOC(sizeof(UINT32)*(BLOCKS_IN_FREE_LIST(db)+1),
			  &(db->free_list.blocks))) )
      return res; /* out of memory */
   if ( (res=MIRD_MALLOC(sizeof(UINT32)*(BLOCKS_IN_FREE_LIST(db)+1),
			  &(db->new_free_list.blocks))) )
      return res; /* out of memory */

   db->free_list.n=0; /* always read the list from disk */

   db->new_free_list.next=
      db->new_free_list.last=0;

   return MIRD_OK;
}

/*
 * flushes a list to disk
 */

static MIRD_RES mird_freelist_flush(struct mird *db,
				     struct mird_free_list *fl,
				     int block)
{
   MIRD_RES res;
   UINT32 i;
   unsigned char *data;

   /* find out where to save the list */

   if ( (res=mird_block_zot(db,block,&data)) )
      return res; /* error */

   memset(data,0,db->block_size); /* clean */

   WRITE_BLOCK_LONG(data,0,SUPERBLOCK_MIRD);
   WRITE_BLOCK_LONG(data,1,1 /* version */);
   WRITE_BLOCK_LONG(data,2,BLOCK_FREELIST);
   WRITE_BLOCK_LONG(data,3,fl->next);
   WRITE_BLOCK_LONG(data,4,fl->n);
      
   for (i=0; i<fl->n; i++)
      WRITE_BLOCK_LONG(data,i+5,fl->blocks[i]);

   fl->n=0;
   fl->next=block;
   if (!fl->last) fl->last=fl->next;

   /* block will be synced later */
   return MIRD_OK;
}

/*
 * joins old + new lists and saves them
 */

MIRD_RES mird_freelist_sync(struct mird *db)
{
   MIRD_RES res;
   UINT32 no;

   /* empty current block of old list into new list,
    * save one spare block 
    */

   /* fix to avoid perpetual resave of all blocks */
   /* this happens when new.n=N-(old.n-1) */
   if (db->new_free_list.n==
       BLOCKS_IN_FREE_LIST(db)-(db->free_list.n-1))
   {
      if ( (res=mird_freelist_pop(db,&no)) ) return res;
      if ( (res=mird_freelist_push(db,no)) ) return res;
   }

   if (db->free_list.n!=0 ||
       db->new_free_list.n!=0)
   {
      for (;;)
      {
	 if ( (res=mird_freelist_pop(db,&no)) ) return res;
	 if (!db->free_list.n) break;
	 if ( (res=mird_freelist_push(db,no)) ) return res;
      }

      /* sync the new list to that last block we never pushed */
      if ( (res=mird_freelist_flush(db,&(db->new_free_list),no)) )
	 return res; 
   }

   if (db->free_list.next)
   {
      unsigned char *data;

      /* tail the rest of the list at the bottom of the new list */

      if (db->new_free_list.last) /* hook in, or just use? */
      {
	 if ( (res=mird_block_get_w(db,db->new_free_list.last,&data)) )
	    return res;

	 WRITE_BLOCK_LONG(data,3,db->free_list.next);

	 /* block will be synced later */
      }
      else
	 db->new_free_list.next=db->free_list.next;
   }

   /* old list = new list */
   /* both counters (n) are zero now */

   db->free_list.next=db->new_free_list.next;

   db->new_free_list.next=
      db->new_free_list.last=0;

   return MIRD_OK;
}
