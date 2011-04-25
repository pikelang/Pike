/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
** libMird by Mirar <mirar@mirar.org>
** please submit bug reports and patches to the author
*/

/* handles cells
 * a cell is some amount of data of unregulated size
 */

#include <unistd.h>
#include <stdlib.h>

#include "internal.h"

static const char RCSID[]=
   "$Id$";

#define BIG_THRESHOLD(DB) ((DB)->block_size-(4<<(DB)->frag_bits)-64)
#define DATA_IN_BIG(DB) ((DB)->block_size-(5+2)*4)
#define CELL_OVERHEAD (4*3)
#define CONT_OVERHEAD (4*2)

/* 
 * creates a big block
 */

static MIRD_RES mird_big_new(struct mird_transaction *mtr,
			     UINT32 table_id,
			     UINT32 next,
			     UINT32 *no,
			     unsigned char **data)
{
   MIRD_RES res;
   unsigned char *bdata;

   if ( (res=mird_tr_new_block(mtr,no,&bdata)) )
      return res;

   WRITE_BLOCK_LONG(bdata,0,mtr->no.msb);
   WRITE_BLOCK_LONG(bdata,1,mtr->no.lsb);
   WRITE_BLOCK_LONG(bdata,2,BLOCK_BIG);
   WRITE_BLOCK_LONG(bdata,3,table_id);
   WRITE_BLOCK_LONG(bdata,4,next);
   data[0]=bdata+20;

   return MIRD_OK;
}

/* 
 * writes a cell
 */

MIRD_RES mird_cell_write(struct mird_transaction *mtr,
			 UINT32 table_id,
			 UINT32 key,
			 UINT32 len,
			 unsigned char *srcdata,
			 UINT32 *chunk_id)
{
   MIRD_RES res;
   unsigned char *data;

   if (len+CELL_OVERHEAD<BIG_THRESHOLD(mtr->db))
   {
      if ( (res=mird_frag_new(mtr,table_id,
			      len+CELL_OVERHEAD,
			      chunk_id,&data)) )
	 return res;
      WRITE_BLOCK_LONG(data,0,CHUNK_CELL);
      WRITE_BLOCK_LONG(data,1,key);
      WRITE_BLOCK_LONG(data,2,len);
      memcpy(data+CELL_OVERHEAD,srcdata,len);
      return MIRD_OK;
   }
   else
   {
      UINT32 dib=DATA_IN_BIG(mtr->db)-CONT_OVERHEAD;
      UINT32 k=(len+CELL_OVERHEAD-CONT_OVERHEAD-1)/dib;
      UINT32 n;
      UINT32 id0,id1=0;
      UINT32 to_write;

      for (;;)
      {
	 n=k*dib-CELL_OVERHEAD+CONT_OVERHEAD;
	 if (!k) to_write=dib-CELL_OVERHEAD+CONT_OVERHEAD;
	 else to_write=dib;
	 if (to_write>len-n) to_write=len-n;

	 if (to_write<BIG_THRESHOLD(mtr->db))
	 {
	    if ( (res=mird_frag_new(mtr,table_id,
				    to_write+CELL_OVERHEAD,
				    &id0,&data)) )
	       return res;
	 }
	 else
	 {
	    if ( (res=mird_big_new(mtr,table_id,id1,&id0,&data)) )
	       return res;
	    id0=BLOCK_FRAG_2_CHUNK_ID(mtr->db,id0,0);
	 }

	 if (!k--)
	 {
	    WRITE_BLOCK_LONG(data,0,CHUNK_CELL);
	    WRITE_BLOCK_LONG(data,1,key);
	    WRITE_BLOCK_LONG(data,2,len);

	    memcpy(data+CELL_OVERHEAD,srcdata+n+CELL_OVERHEAD-CONT_OVERHEAD,to_write);

	    break;
	 }
	 else
	 {
	    WRITE_BLOCK_LONG(data,0,CHUNK_CONT);
	    WRITE_BLOCK_LONG(data,1,key);
	    memcpy(data+CONT_OVERHEAD,srcdata+n,to_write);
	 }

	 id1=id0;
      }
      chunk_id[0]=id0;
      return MIRD_OK;
   }
}

/*
 * determines cell size & key
 */

MIRD_RES mird_cell_get_info(struct mird *db,
			    UINT32 chunk_id,
			    UINT32 *key,
			    UINT32 *len)
{
   UINT32 b,n;
   unsigned char *data;
   MIRD_RES res;

   b=CHUNK_ID_2_BLOCK(db,chunk_id);
   n=CHUNK_ID_2_FRAG(db,chunk_id);
   if (n)
   {
      if ( (res=mird_frag_get(db,chunk_id,&data,len)) )
	 return res;
   }
   else
   {
      if ( (res=mird_block_get(db,b,&data)) )
	 return res;
      if (READ_BLOCK_LONG(data,2)!=BLOCK_BIG)
	 return mird_generate_error(MIRDE_WRONG_BLOCK,b,
				     BLOCK_BIG,READ_BLOCK_LONG(data,2));
      data+=20; /* big block stuff */
   }

   if (READ_BLOCK_LONG(data,0)!=CHUNK_CELL)
      return mird_generate_error(MIRDE_WRONG_CHUNK,chunk_id,
				  CHUNK_CELL,READ_BLOCK_LONG(data,0));

   if (key) key[0]=READ_BLOCK_LONG(data,1);
   len[0]=READ_BLOCK_LONG(data,2);

   return MIRD_OK;
}

/*
 * reads a cell to the given buffer
 */

MIRD_RES mird_cell_read(struct mird *db,
			UINT32 chunk_id,
			unsigned char *dest,
			UINT32 len)
{
   UINT32 b,n;
   unsigned char *data;
   MIRD_RES res;
   UINT32 flen;
   int first=1;
   UINT32 id=chunk_id,nid;

   for (;;)
   {
      b=CHUNK_ID_2_BLOCK(db,id);
      n=CHUNK_ID_2_FRAG(db,id);
      if (n)
      {
	 if ( (res=mird_frag_get(db,id,&data,&flen)) )
	    return res;
	 nid=0;
      }
      else
      {
	 if ( (res=mird_block_get(db,b,&data)) )
	    return res;

	 if (READ_BLOCK_LONG(data,2)!=BLOCK_BIG)
	    return mird_generate_error(MIRDE_WRONG_BLOCK,b,
					BLOCK_BIG,READ_BLOCK_LONG(data,2));

	 nid=READ_BLOCK_LONG(data,4);
	 data+=20; /* big block stuff */
	 flen=DATA_IN_BIG(db);
      }

      if (first)
      {
	 if (READ_BLOCK_LONG(data,0)!=CHUNK_CELL)
	    return mird_generate_error(MIRDE_WRONG_CHUNK,id,
					CHUNK_CELL,READ_BLOCK_LONG(data,0));
  
	 data+=CELL_OVERHEAD;
	 flen-=CELL_OVERHEAD;
	 first=0;
      }
      else
      {
	 if (READ_BLOCK_LONG(data,0)!=CHUNK_CONT)
	    return mird_generate_error(MIRDE_WRONG_CHUNK,id,
					CHUNK_CONT,READ_BLOCK_LONG(data,0));
  
	 data+=CONT_OVERHEAD;
	 flen-=CONT_OVERHEAD;
      }

      if (flen>len) flen=len;

      memcpy(dest,data,flen);
      dest+=flen;
      len-=flen;

      if (!len) 
	 return MIRD_OK;

      if (!(id=nid))
	 return mird_generate_error(MIRDE_CELL_SHORT,chunk_id,0,0);
   }

}

/*
 * reads a cell
 * allocates buffer, free that after use
 */

MIRD_RES mird_cell_get(struct mird *db,
		       UINT32 chunk_id,
		       UINT32 *key,
		       UINT32 *len,
		       unsigned char **data)
{
   MIRD_RES res;

   if (!chunk_id)
   {
      data=NULL;
      return MIRD_OK;
   }

   if ( (res=mird_cell_get_info(db,chunk_id,key,len)) )
      return res;

   if (!len[0])
   {
      if ( (res=MIRD_MALLOC(1,data)) )
         return res;
   }
   else
      if ( (res=MIRD_MALLOC(len[0],data)) )
         return res;

   if ( (res=mird_cell_read(db,chunk_id,data[0],len[0])) )
      return res;

   return MIRD_OK;
}
