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

#include "internal.h"

#ifndef NULL
#define NULL (0)
#endif

static const char RCSID[]=
   "$Id$";

#ifdef SUPERMASSIVE_DEBUG
#define FRAGS_DEBUG
#endif
#ifdef FRAGS_DEBUG
#include <stdio.h>
#endif

/* 
 * reads a frag from the database,
 * anonymously
 * resulting data area must *not* be free()d 
 * nor used past any other _frag or _block call
 */

MIRD_RES mird_frag_get_b(struct mird *db,
			 UINT32 chunk_id,
			 unsigned char **fdata,
			 unsigned char **bdata_p,
			 UINT32 *len)
{
   MIRD_RES res;
   long n=CHUNK_ID_2_FRAG(db,chunk_id);
   long start,end;
   unsigned char *bdata;

   if ( (res=mird_block_get(db,CHUNK_ID_2_BLOCK(db,chunk_id),&bdata)) )
      return res; /* error */

   if ( READ_BLOCK_LONG(bdata,2)!=BLOCK_FRAG &&
	READ_BLOCK_LONG(bdata,2)!=BLOCK_FRAG_PROGRESS )
      return mird_generate_error(MIRDE_WRONG_BLOCK,
				  CHUNK_ID_2_BLOCK(db,chunk_id),
				  BLOCK_FRAG,READ_BLOCK_LONG(bdata,2));

   start=READ_BLOCK_LONG(bdata,3+n);
   end=READ_BLOCK_LONG(bdata,3+n+1);
#ifdef FRAGS_DEBUG
   fprintf(stderr,"get   b=%lxh n=%ld start=%ld end=%ld\n",
	   CHUNK_ID_2_BLOCK(db,chunk_id),
	   n,start,end);
#endif
   if (n==0 || start==0 || end==0)
      return mird_generate_error(MIRDE_ILLEGAL_FRAG,
				  CHUNK_ID_2_BLOCK(db,chunk_id),n,0);
   len[0]=end-start;
   fdata[0]=bdata+start;
   if (bdata_p) bdata_p[0]=bdata;

   return MIRD_OK;
}

/*
 * fetches a frag from the database,
 * marks the block as dirty
 * resulting data area must *not* be free()d,
 * but can be written into (and then saved at the next clean)
 * may neither be used past any other _frag or _block call
 */


MIRD_RES mird_frag_get_w(struct mird_transaction *mtr,
			 UINT32 chunk_id,
			 unsigned char **fdata,
			 UINT32 *len)
{
   unsigned char *bdata;
   MIRD_RES res;
   long n=CHUNK_ID_2_FRAG(mtr->db,chunk_id);
   long start,end;
   struct mird *db=mtr->db;

   if ( (res=mird_block_get_w(db,
			       CHUNK_ID_2_BLOCK(db,chunk_id),
			       &bdata)) )
      return res; /* error */

   if ( READ_BLOCK_LONG(bdata,2)!=BLOCK_FRAG_PROGRESS &&
	READ_BLOCK_LONG(bdata,2)!=BLOCK_FRAG )
      return mird_generate_error(MIRDE_WRONG_BLOCK,
				 CHUNK_ID_2_BLOCK(db,chunk_id),
				 BLOCK_FRAG_PROGRESS,
				 READ_BLOCK_LONG(bdata,2));

   if (READ_BLOCK_LONG(bdata,1)!=mtr->no.lsb ||
       READ_BLOCK_LONG(bdata,0)!=mtr->no.msb)
      mird_fatal("mird_frag_get_w: not our transaction\n");

   start=READ_BLOCK_LONG(bdata,3+n);
   end=READ_BLOCK_LONG(bdata,3+n+1);

#ifdef FRAGS_DEBUG
   fprintf(stderr,"get_w b=%lxh n=%ld start=%ld end=%ld\n",
	   CHUNK_ID_2_BLOCK(db,chunk_id),
	   n,start,end);
#endif

   if (n==0 || start==0 || end==0)
   {
      return mird_generate_error(MIRDE_ILLEGAL_FRAG,
				  CHUNK_ID_2_BLOCK(db,chunk_id),n,0);
   }
   len[0]=end-start;
   fdata[0]=bdata+start;

   return MIRD_OK;
}

/* 
 * marks a frag block as finished 
 */

INLINE MIRD_RES mird_frag_finish(struct mird_transaction *mtr,
				 UINT32 block_no)
{
   unsigned char *bdata;
   MIRD_RES res;

   if ( (res=mird_block_get_w(mtr->db,block_no,&bdata)) )
      return res; /* error */

   WRITE_BLOCK_LONG(bdata,2,BLOCK_FRAG);

   return MIRD_OK;
}


/* 
 * allocates a new frag in the database;
 * the block is marked as dirty
 * resulting data area must *not* be free()d,
 * but can be written into (and then saved at the next clean)
 * may neither be used past any other _frag or _block call
 */

MIRD_RES mird_frag_new(struct mird_transaction *mtr,
		       UINT32 table_id,
		       UINT32 len,
		       UINT32 *chunk_id,
		       unsigned char **fdata)
{
   UINT32 i;
   long best=0x7fffffff,best_no=-1;
   long worst=0x7fffffff,worst_no=-1;
   MIRD_RES res;
   unsigned char *bdata;
   struct transaction_free_frags *ff;
   struct mird *db=mtr->db;

   if (len&3) len+=4-(len&3); /* align */

   /* find the block with the closest fit */
   for (i=0; i<mtr->n_free_frags; i++)
   {
      long x=mtr->free_frags[i].free_bytes-len;
      if (mtr->free_frags[i].table_id==table_id &&
	  x>=0 && best>x) best=x,best_no=i; 
      if (worst>x) worst=x,worst_no=i; 
   }
   if (best_no==-1) /* we'll have to get a new one then */
   {
      UINT32 block_no;

      if ( (res=mird_tr_new_block(mtr,&block_no,&bdata)) )
	 return res; /* error */

      /* write block header */
      WRITE_BLOCK_LONG(bdata,0,mtr->no.msb);
      WRITE_BLOCK_LONG(bdata,1,mtr->no.lsb);
      WRITE_BLOCK_LONG(bdata,2,BLOCK_FRAG_PROGRESS);
      WRITE_BLOCK_LONG(bdata,3,table_id);

      /* write start of each frag + end of last */
      WRITE_BLOCK_LONG(bdata,4,4*(MAX_FRAGS(db)+5)); /* 4 = start of frag 1 */
      
      if (mtr->n_free_frags<db->max_free_frag_blocks)
	 i=mtr->n_free_frags++; /* just add */
      else 
      {
	 i=worst_no; /* garb the worst one */
	 if ( (res=mird_frag_finish(mtr,mtr->free_frags[i].block_no)) )
	    return res;
	 if ( (res=mird_block_get_w(db,block_no,&bdata)) )
	    return res; /* error */
      }

      ff=mtr->free_frags+i;

      ff->block_no=block_no;
      ff->free_bytes=
	 db->block_size-24-4*MAX_FRAGS(db);
      ff->frag_no=1;
      ff->table_id=table_id;
   }
   else
   {
      ff=mtr->free_frags+best_no;

#ifdef FRAGS_DEBUG
      fprintf(stderr,"new   reuse b=%lxh\n",ff->block_no);
#endif

      if ( (res=mird_block_get_w(db,ff->block_no,&bdata)) )
	 return res; /* error */

      if (READ_BLOCK_LONG(bdata,1)!=mtr->no.lsb ||
	  READ_BLOCK_LONG(bdata,0)!=mtr->no.msb)
	 mird_fatal("mird_frag_new: not our transaction\n");

      if (READ_BLOCK_LONG(bdata,3+ff->frag_no)==0)
	 return mird_generate_error(MIRDE_ILLEGAL_FRAG,
				     ff->block_no,ff->frag_no-1,0);
   }

   WRITE_BLOCK_LONG(bdata,4+ff->frag_no,
		    READ_BLOCK_LONG(bdata,3+ff->frag_no)+len);

#ifdef FRAGS_DEBUG
   fprintf(stderr,"new   b=%lxh n=%ld start=%ld end=%ld\n",
	   ff->block_no,
	   ff->frag_no,
	   READ_BLOCK_LONG(bdata,3+ff->frag_no),
	   READ_BLOCK_LONG(bdata,4+ff->frag_no));
#endif

   fdata[0]=bdata+READ_BLOCK_LONG(bdata,3+ff->frag_no);
   chunk_id[0]=BLOCK_FRAG_2_CHUNK_ID(db,ff->block_no,ff->frag_no);

   ff->free_bytes-=len;
   if (ff->frag_no++==MAX_FRAGS(db))
      ff->free_bytes=0; /* garb this next time, don't use */

   return MIRD_OK;
}

MIRD_RES mird_frag_close(struct mird_transaction *mtr)
{
   UINT32 i;
   MIRD_RES res;
   for (i=0; i<mtr->n_free_frags; i++)
      if ( (res=mird_frag_finish(mtr,mtr->free_frags[i].block_no)) )
	 return res;
   return MIRD_OK;
}
