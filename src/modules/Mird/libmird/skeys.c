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

#include <stdlib.h>
#include <unistd.h>

#include "internal.h"

#include "dmalloc.h"

static const char RCSID[]=
   "$Id$";

#define SKEY_OVERHEAD 16

#define SKEY_STEP(S,STEP) \
   STEP=skey_step(READ_BLOCK_LONG(S,2),READ_BLOCK_LONG(S,3));

INLINE UINT32 mird_s_key_hash(unsigned char *data,
			      UINT32 len)
{
   UINT32 z=len;
   
   while (len--) z+=(z<<5)^*(data++);

   return z; /* for better lsb performance */
}

/*

int hash(string data)
{
   int x,z=0;
   while (sscanf(data,"%4c%s",x,data))
      z=(z*137912897+x+0x01020304)&0xffffffff;
   if (strlen(data))
   {
      sscanf(data+"\0\0\0","%4c",x);
      if (strlen(data)>2) x+=0x0101;
      if (strlen(data)>1) x+=0x010101;
      x+=0x01010101;
      z=(z*137912897+x)&0xffffffff;
   }
   return (z%37493+z)&0xffffffff;   
}

*/

INLINE UINT32 skey_step(UINT32 key_len,UINT32 value_len)
{
   UINT32 step;
   step=key_len+value_len+SKEY_OVERHEAD;
   if (step&3) return step+4-(step&3);
   return step;
}

static MIRD_RES mird_s_key_pack(struct mird_transaction *mtr,
				struct transaction_id *id,
				unsigned char *old_data,UINT32 old_len,
				unsigned char *key_data,UINT32 key_len,
				unsigned char *value_data,UINT32 value_len,
				unsigned char **new_data,UINT32 *new_len)
{
   unsigned char *s,*d;
   UINT32 n,len;
   MIRD_RES res;

   /* get the new size */

   len=0;
   if (old_data) /* ok, count all except this key */
   {
      n=old_len;
      s=old_data;
      while (n)
      {
	 UINT32 step;
	 SKEY_STEP(s,step);

	 if (READ_BLOCK_LONG(s,2)!=key_len ||
	     memcmp(s+SKEY_OVERHEAD,key_data,key_len))
	    len+=step;

	 if (n<step)
	    return mird_generate_error(MIRDE_DATA_MISSING,0,0,0);

	 n-=step;
	 s+=step;
      }
   }

   if (value_data)
      len+=skey_step(key_len,value_len);

   if (!len) /* remove the cell */
   {
      new_data[0]=NULL;
      new_len[0]=0;
      return MIRD_OK; 
   }

   /* allocate a buffer */

   if ( (res=MIRD_MALLOC(len,&d)) )
      return res;

   new_len[0]=len;
   new_data[0]=d;
   
   /* insert my data */

   if (value_data)
   {
      WRITE_BLOCK_LONG(d,0,id->msb);
      WRITE_BLOCK_LONG(d,1,id->lsb);
      WRITE_BLOCK_LONG(d,2,key_len);
      WRITE_BLOCK_LONG(d,3,value_len);
      memcpy(d+SKEY_OVERHEAD,key_data,key_len);
      memcpy(d+SKEY_OVERHEAD+key_len,value_data,value_len);
      
      d+=skey_step(key_len,value_len);
   }

   /* copy the old data, if needed */

   if (old_data)
   {
      n=old_len;
      len=0;
      s=old_data;
      while (n)
      {
	 UINT32 step;
	 SKEY_STEP(s,step);

	 if (READ_BLOCK_LONG(s,2)!=key_len ||
	     memcmp(s+SKEY_OVERHEAD,key_data,key_len))
	 {
	    memcpy(d,s,step);
	    d+=step;
	 }

	 /* we now know that n+step+...+step is even */
	 n-=step;
	 s+=step;
      }
   }

   return MIRD_OK;
}

MIRD_RES mird_s_key_store(struct mird_transaction *mtr,
			  UINT32 table_id,
			  unsigned char *skey,
			  UINT32 skey_len,
			  unsigned char *value,
			  UINT32 len)
{
   MIRD_RES res;
   unsigned char *old_data,*new_data;
   UINT32 old_len,new_len;
   UINT32 root,type,hashkey;

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
      return res;

   if (type!=MIRD_TABLE_STRINGKEY) 
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_STRINGKEY);

   hashkey=mird_s_key_hash(skey,skey_len);

   if ( (res=mird_low_key_lookup(mtr->db,root,hashkey,&old_data,&old_len)) )
      return res;

   if ( (res=mird_s_key_pack(mtr,&(mtr->no),
			     old_data,old_len,skey,skey_len,value,len,
			     &new_data,&new_len)) )
   {
      sfree(old_data);
      return res;
   }

   res=mird_low_key_store(mtr,table_id,hashkey,new_data,new_len,
			  MIRD_TABLE_STRINGKEY);

   if (new_data) sfree(new_data);
   if (old_data) sfree(old_data);

   return res;
}

/*
 * get data for a key in stringkey cell data
 */

static MIRD_RES mird_low_s_key_find(unsigned char *cell_data,
				    UINT32 cell_len,
				    unsigned char *key,
				    UINT32 key_len,
				    unsigned char **data,
				    UINT32 *len,
				    struct transaction_id *tr_id,
				    int *exists)
{
   unsigned char *s;
   UINT32 n;
   MIRD_RES res;

   if (!cell_data)
   {
      if (data) data[0]=NULL;
      if (exists) exists[0]=0;
      return MIRD_OK;
   }

   n=cell_len;
   s=cell_data;

   while (n)
   {
      UINT32 step;
      SKEY_STEP(s,step);

      if (READ_BLOCK_LONG(s,2)==key_len &&
	  !memcmp(s+SKEY_OVERHEAD,key,key_len))
      {
	 if (data)
	 {
	    if ( (res=MIRD_MALLOC(READ_BLOCK_LONG(s,3)+1,data)) )
	       return res;

	    memcpy(data[0],s+SKEY_OVERHEAD+key_len,
		   READ_BLOCK_LONG(s,3));
	    data[0][READ_BLOCK_LONG(s,3)]=0; /* make c string */
	    len[0]=READ_BLOCK_LONG(s,3);
	 }
	 if (tr_id)
	 {
	    tr_id->msb=READ_BLOCK_LONG(s,0);
	    tr_id->lsb=READ_BLOCK_LONG(s,1);
	 }
	 if (exists) exists[0]=1;
	 return MIRD_OK; /* found */
      }

      if (n<step)
	 return mird_generate_error(MIRDE_DATA_MISSING,0,0,0);

      n-=step;
      s+=step;
   }
	
   if (data) data[0]=NULL;
   if (exists) exists[0]=0;
   return MIRD_OK;  /* not found */
}

/*
 * low level look up a string key
 */

MIRD_RES mird_low_s_key_lookup(struct mird *db,
			       UINT32 root,
			       UINT32 hashkey,
			       unsigned char *key,
			       UINT32 key_len,
			       unsigned char **data,
			       UINT32 *len)
{
   unsigned char *cell_data;
   UINT32 cell_len;
   MIRD_RES res;
   
   if ( (res=mird_low_key_lookup(db,root,hashkey,&cell_data,&cell_len)) )
      return res;

   if (cell_data)
   {
      res=mird_low_s_key_find(cell_data,cell_len,
			      key,key_len,data,len,NULL,NULL);

      sfree(cell_data);
   }
   else
      data[0]=NULL;

   return res;
}

/*
 * look up a string key in a transaction
 */

MIRD_RES mird_transaction_s_key_lookup(struct mird_transaction *mtr,
				       UINT32 table_id,
				       unsigned char *key,
				       UINT32 key_len,
				       unsigned char **data,
				       UINT32 *len)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
      return res;

   if (type!=MIRD_TABLE_STRINGKEY) 
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_STRINGKEY);

   return mird_low_s_key_lookup(mtr->db,root,mird_s_key_hash(key,key_len),
				key,key_len,data,len);
}

/*
 * look up a string key in the database 
 */

MIRD_RES mird_s_key_lookup(struct mird *db,
			   UINT32 table_id,
			   unsigned char *key,
			   UINT32 key_len,
			   unsigned char **data,
			   UINT32 *len)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_db_table_get_root(db,table_id,&root,&type)) )
      return res;

   if (type!=MIRD_TABLE_STRINGKEY) 
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_STRINGKEY);

   return mird_low_s_key_lookup(db,root,mird_s_key_hash(key,key_len),
				key,key_len,data,len);

}

/*
 * make a new string-key table
 */			

MIRD_RES mird_s_key_new_table(struct mird_transaction *mtr,
			      UINT32 table_id)
{
   return mird_low_table_new(mtr,table_id,MIRD_TABLE_STRINGKEY);
}

/*
 * resolve a string key cell
 */

MIRD_RES mird_s_key_resolve(struct mird_transaction *mtr,
			    UINT32 table_id,
			    UINT32 hashkey,
			    UINT32 old_cell,
			    UINT32 new_cell)
{
   /* what we wish to know: has any of our keys
    * changed in between old and new? */

   /* what we need to do: 
    * copy any of the new keys that's not ours into our cell
    */

   unsigned char *old_cell_data=NULL,*new_cell_data=NULL,*our_cell_data=NULL;
   UINT32 old_cell_len,new_cell_len,our_cell_len;
   UINT32 root,type;
   MIRD_RES res;
   unsigned char *key,*s;
   UINT32 key_len;
   UINT32 n;
   int need_save=0;

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
      return res;

   if (type!=MIRD_TABLE_STRINGKEY) 
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_STRINGKEY);

   if ( (res=mird_low_key_lookup(mtr->db,root,hashkey,
				 &our_cell_data,&our_cell_len)) )
      goto clean;

   if ( (res=mird_cell_get(mtr->db,old_cell,NULL,
			   &old_cell_len,&old_cell_data)) )
      goto clean;

   if ( (res=mird_cell_get(mtr->db,new_cell,NULL,
			   &new_cell_len,&new_cell_data)) )
      goto clean;

   /* for every key that has changed between old_cell and new_cell,
    * copy (or delete) that key in our cell,
    * if it's not ours too (if it is, conflict)
    */

   /* loop over the keys in new_cell */
   /* check if any of them has changed since old_cell */

   n=new_cell_len;
   s=new_cell_data;

   if (new_cell_data) while (n)
   {
      UINT32 step;
      struct transaction_id tr_id,mtr_id;
      int exists;
      unsigned char *tmp;
      UINT32 tmp_len;

      SKEY_STEP(s,step);

      key=s+SKEY_OVERHEAD;
      key_len=READ_BLOCK_LONG(s,2);
      
      if ( (res=mird_low_s_key_find(old_cell_data,old_cell_len,
				    key,key_len,
				    NULL,NULL,
				    &tr_id,&exists)) )
	 goto clean;

      if (!exists ||
	  tr_id.msb!=READ_BLOCK_LONG(s,0) ||
	  tr_id.lsb!=READ_BLOCK_LONG(s,1)) 
      {
	 /* they diff */

	 if ( (res=mird_low_s_key_find(our_cell_data,our_cell_len,
				       key,key_len,
				       NULL,NULL,
				       &mtr_id,&exists)) )
	    goto clean;

	 if (exists && 
	     (mtr_id.msb==mtr->no.msb &&
	      mtr_id.lsb==mtr->no.lsb))
	     goto conflict;
	 
	 /* copy new data into the cell, if we doesn't have it */
	 if ( !exists ||
	      mtr_id.msb != READ_BLOCK_LONG(s,0) ||
	      mtr_id.lsb != READ_BLOCK_LONG(s,1)) 
	 {
#if 0
	    fprintf(stderr,"change key (len %ld) %ld:%ld was %ld:%ld (old %ld:%ld) (exists:%d)\n",
		    key_len,
		    mtr_id.msb,mtr_id.lsb,
		    READ_BLOCK_LONG(s,0),
		    READ_BLOCK_LONG(s,1),
		    tr_id.msb,tr_id.lsb,
		    exists);
#endif

	    tr_id.msb=READ_BLOCK_LONG(s,0);
	    tr_id.lsb=READ_BLOCK_LONG(s,1);

	    if ( (res=mird_s_key_pack(mtr,&tr_id,
				      our_cell_data,our_cell_len,
				      key,key_len,
				      key+READ_BLOCK_LONG(s,2),
				      READ_BLOCK_LONG(s,3),
				      &tmp,&tmp_len)) )
	       goto clean;

	    if (our_cell_data) sfree(our_cell_data);

	    our_cell_data=tmp;
	    our_cell_len=tmp_len;
	    need_save=1;
	 }
      }

      if (n<step)
      {
	 res=mird_generate_error(MIRDE_DATA_MISSING,0,0,0);
	 goto clean;
      }

      n-=step;
      s+=step;
   }

   /* loop over the keys in old_cell */
   /* check if any of them has been deleted in new_cell */

   n=old_cell_len;
   s=old_cell_data;

   if (old_cell_data) while (n)
   while (n)
   {
      UINT32 step;
      struct transaction_id tr_id;
      int exists;
      unsigned char *tmp;
      UINT32 tmp_len;

      SKEY_STEP(s,step);

      key=s+SKEY_OVERHEAD;
      key_len=READ_BLOCK_LONG(s,2);
      
      if ( (res=mird_low_s_key_find(new_cell_data,new_cell_len,
				    key,key_len,
				    NULL,NULL,
				    &tr_id,&exists)) )
	 goto clean;

      if (!exists) /* it has been deleted */
      {
	 /* they diff */

	 if ( (res=mird_low_s_key_find(our_cell_data,our_cell_len,
				       key,key_len,
				       NULL,NULL,
				       &tr_id,&exists)) )
	    goto clean;

	 if (!exists ||
	     (tr_id.msb==mtr->no.msb &&
	      tr_id.lsb==mtr->no.lsb))
	    goto conflict; /* we've deleted too, or changed it */

	 /* remove it in our cell too, if it's still there */
	 if (exists)
	 {
	    if ( (res=mird_s_key_pack(mtr,&tr_id,
				      our_cell_data,our_cell_len,
				      key,key_len,
				      NULL,0,
				      &tmp,&tmp_len)) )
	       goto clean;

	    if (our_cell_data) sfree(our_cell_data);

	    our_cell_data=tmp;
	    our_cell_len=tmp_len;
	    need_save=1;
	 }
      }

      if (n<step)
      {
	 res=mird_generate_error(MIRDE_DATA_MISSING,0,0,0);
	 goto clean;
      }

      n-=step;
      s+=step;
   }
	
   /* ok, conflict(s) resolved - good */

   if (need_save)
   {
      /* write our new cell */
      if ( (res=mird_low_key_store(mtr,table_id,hashkey,
				   our_cell_data,our_cell_len,
				   MIRD_TABLE_STRINGKEY)) )
	 goto clean;
   }

   res=MIRD_OK; 

clean:
   if (old_cell_data) sfree(old_cell_data);
   if (new_cell_data) sfree(new_cell_data);
   if (our_cell_data) sfree(our_cell_data);

   return res;

conflict:
   if ( (res=MIRD_MALLOC(key_len,&s)) )
      goto clean;

   memcpy(s,key,key_len);

   res=mird_generate_error_s(MIRDE_CONFLICT_S,s,table_id,hashkey,key_len);
   goto clean;
}

/*
 * scan through a string-key table
 * 
 * if prev is NULL, scan will start from first key,
 * otherwise prev will be freed (or reused)
 *
 * n is not definite, it might be changed depending on
 * how many keys there are in the cells read
 *
 * note: it will be freed upon error too.
 */

static MIRD_RES mird_low_s_table_scan(struct mird *db,
				      UINT32 root,
				      UINT32 nr,
				      struct mird_s_scan_result *prev,
				      struct mird_s_scan_result **dest)
{
   struct mird_scan_result *msr;
   MIRD_RES res;
   UINT32 i,j;
   UINT32 tupels;

   if (prev)
   {
      msr=prev->msr;
      sfree(prev);
   }
   else
      msr=NULL;

   if ( (res=mird_low_table_scan(db,root,nr,msr,&msr)) )
      return res;

   if (!msr) /* no more tupels */
   {
      dest[0]=NULL;
      return MIRD_OK;
   }

   /* count the number of tupels */

   tupels=0;
   for (i=0; i<msr->n; i++)
   {
      unsigned char *s=msr->tupel[i].value;
      UINT32 n=msr->tupel[i].value_len;

      while (n)
      {
	 UINT32 step;
	 SKEY_STEP(s,step);

	 tupels++;
	 if (n<step)
	 {
	    mird_free_scan_result(msr);
	    return mird_generate_error(MIRDE_DATA_MISSING,0,0,0);
	 }

	 n-=step;
	 s+=step;
      }
   }

   /* allocate a new mssr and place and adjust the pointers */

   if ( (res=MIRD_MALLOC(sizeof(struct mird_s_scan_result)+
			 sizeof(struct mird_s_scan_tupel)*tupels,dest)) )
   {
      mird_free_scan_result(msr);
      return res;
   }
   dest[0]->n=tupels;
   dest[0]->msr=msr;

   j=0;
   for (i=0; i<msr->n; i++)
   {
      unsigned char *s=msr->tupel[i].value;
      UINT32 n=msr->tupel[i].value_len;

      while (n)
      {
	 UINT32 step;
	 SKEY_STEP(s,step);

	 dest[0]->tupel[j].key=s+SKEY_OVERHEAD;
	 dest[0]->tupel[j].key_len=READ_BLOCK_LONG(s,2);
	 dest[0]->tupel[j].value=s+SKEY_OVERHEAD+READ_BLOCK_LONG(s,2);
	 dest[0]->tupel[j].value_len=READ_BLOCK_LONG(s,3);
	 
	 j++;
	 n-=step;
	 s+=step;
      }
   }

   return MIRD_OK;
}

MIRD_RES mird_transaction_s_table_scan(struct mird_transaction *mtr,
				       UINT32 table_id,
				       UINT32 n,
				       struct mird_s_scan_result *prev,
				       struct mird_s_scan_result **dest)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
   {
      if (prev) mird_free_s_scan_result(prev);
      return res;
   }

   if (type!=MIRD_TABLE_STRINGKEY) 
   {
      if (prev) mird_free_s_scan_result(prev);
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_HASHKEY);
   }

   return mird_low_s_table_scan(mtr->db,root,n,prev,dest);
}

MIRD_RES mird_s_table_scan(struct mird *db,
			   UINT32 table_id,
			   UINT32 n,
			   struct mird_s_scan_result *prev,
			   struct mird_s_scan_result **dest)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_db_table_get_root(db,table_id,&root,&type)) )
   {
      if (prev) mird_free_s_scan_result(prev);
      return res;
   }

   if (type!=MIRD_TABLE_STRINGKEY) 
   {
      if (prev) mird_free_s_scan_result(prev);
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_HASHKEY);
   }

   return mird_low_s_table_scan(db,root,n,prev,dest);
}

void mird_free_s_scan_result(struct mird_s_scan_result *mssr)
{
   mird_free_scan_result(mssr->msr);
   sfree(mssr);
}

MIRD_RES mird_s_scan_continued(UINT32 key,struct mird_s_scan_result **dest)
{
   struct mird_scan_result *msr;
   MIRD_RES res;

   if ( (res=mird_scan_continued(key,&msr)) )
      return res;

   if ( (res=MIRD_MALLOC(sizeof(struct mird_s_scan_result)+
			 sizeof(struct mird_s_scan_tupel)*1,dest)) )
   {
      mird_free_scan_result(msr);
      return res;
   }
   dest[0]->msr=msr;
   dest[0]->n=0;
   return MIRD_OK;
}

MIRD_RES mird_s_scan_continuation(struct mird_s_scan_result *mssr,UINT32 *key)
{
   if (!mssr || !mssr->msr || !mssr->msr->n)
      return mird_generate_error(MIRDE_SCAN_END,0,0,0);
   key[0]=mssr->msr->tupel[mssr->msr->n-1].key;
   return MIRD_OK;
}
