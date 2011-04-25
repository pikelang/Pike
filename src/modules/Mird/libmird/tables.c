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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "dmalloc.h"

static const char RCSID[]=
   "$Id$";

/*
 * looks up a table id in the master table
 */

INLINE MIRD_RES mird_table_get_root(struct mird *db,
				    UINT32 tables,
				    UINT32 table,
				    UINT32 *root,
				    UINT32 *type)
{
   UINT32 cell,len;
   unsigned char *data;
   MIRD_RES res;

   if (!table) 
      return mird_generate_error(MIRDE_NO_TABLE,table,0,0);

   if ( (res=mird_hashtrie_find(db,tables,table,&cell)) )
      return res;

   if (!cell) 
      return mird_generate_error(MIRDE_NO_TABLE,table,0,0);

   if ( (res=mird_frag_get(db,cell,&data,&len)) )
      return res;

   if (len<16)
      return mird_generate_error(MIRDE_SMALL_CHUNK,cell,16,CHUNK_ROOT);

   if (READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
      return mird_generate_error(MIRDE_WRONG_CHUNK,cell,
				 CHUNK_ROOT,READ_BLOCK_LONG(data,0));

#ifdef MIRD_TABLE_DEBUG   
   fprintf(stderr,"get; root=%lxh type=%lxh\n",
	   READ_BLOCK_LONG(data,2),
	   READ_BLOCK_LONG(data,3));
#endif

   if (root) root[0]=READ_BLOCK_LONG(data,2);
   if (type) type[0]=READ_BLOCK_LONG(data,3);

   return MIRD_OK;
}

MIRD_RES mird_db_table_get_root(struct mird *db,
				UINT32 table_id,
				UINT32 *root,
				UINT32 *type)
{
   MIRD_RES res;

   if ( (db->flags & MIRD_LIVE) )
      if ( (res=mird_readonly_refresh(db)) )
	 return res;

   if (db->cache_table_id!=table_id)
   {
      UINT32 lroot,ltype;

      if ( (res=mird_table_get_root(db,db->tables,table_id,
				    &lroot,&ltype)) )
	 return res;

      db->cache_table_id=table_id;
      db->cache_table_root=lroot;
      db->cache_table_type=ltype;
   }
   if (root) root[0]=db->cache_table_root;
   if (type) type[0]=db->cache_table_type;

#ifdef MIRD_TABLE_DEBUG
   fprintf(stderr,"db table %lxh root=%lxh\n",table_id,db->cache_table_root);
#endif

   return MIRD_OK;
}

MIRD_RES mird_tr_table_get_root(struct mird_transaction *mtr,
				UINT32 table_id,
				UINT32 *root,
				UINT32 *type)
{
   MIRD_RES res;

   if (mtr->cache_last_commited.lsb!=mtr->db->last_commited.lsb ||
       mtr->cache_last_commited.msb!=mtr->db->last_commited.msb ||
       mtr->cache_table_id!=table_id)
   {
      UINT32 lroot,ltype;

      if ( (res=mird_table_get_root(mtr->db,mtr->tables,table_id,
				    &lroot,&ltype)) )
	 return res;

      mtr->cache_last_commited=mtr->db->last_commited;
      mtr->cache_table_id=table_id;
      mtr->cache_table_root=lroot;
      mtr->cache_table_type=ltype;
   }
   if (root) root[0]=mtr->cache_table_root;
   if (type) type[0]=mtr->cache_table_type;

#ifdef MIRD_TABLE_DEBUG
   fprintf(stderr,"tr table %lxh root=%lxh\n",table_id,mtr->cache_table_root);
#endif

   return MIRD_OK;
}

/*
 * saves a table root
 */

MIRD_RES mird_table_write_root(struct mird_transaction *mtr,
			       UINT32 table_id,
			       UINT32 root)
{
   UINT32 cell,len;
   unsigned char *data,*bdata;
   MIRD_RES res;
   UINT32 table_type;

   if ( (res=mird_hashtrie_find_b(mtr->db,mtr->tables,table_id,&cell,&bdata)) )
      return res;

   if (!cell)
      return mird_generate_error(MIRDE_NO_TABLE,table_id,0,0);

   if (READ_BLOCK_LONG(bdata,1)==mtr->no.lsb &&
       READ_BLOCK_LONG(bdata,0)==mtr->no.msb)
   {
      /* it's ours, reuse it */
      if ( (res=mird_frag_get_w(mtr,cell,&data,&len)) )
	 return res;
      if (len<16)
	 return mird_generate_error(MIRDE_SMALL_CHUNK,cell,16,CHUNK_ROOT);
      if (READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
	 return mird_generate_error(MIRDE_WRONG_CHUNK,cell,
				    CHUNK_ROOT,READ_BLOCK_LONG(data,0));
      WRITE_BLOCK_LONG(data,2,root);
      return MIRD_OK;
   }
   else
   {
      if ( (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,cell))) )
	 return res;

      if ( (res=mird_frag_get(mtr->db,cell,&data,&len)) )
	 return res;

      table_type=READ_BLOCK_LONG(data,3);
#ifdef MIRD_TABLE_DEBUG
      fprintf(stderr,"copy; type=%lxh\n",table_type);
#endif

      if ( (res=mird_frag_new(mtr,MASTER_MIRD_TABLE_ID,4*4,&cell,&data)) )
	 return res;

      WRITE_BLOCK_LONG(data,0,CHUNK_ROOT);
      WRITE_BLOCK_LONG(data,1,table_id);
      WRITE_BLOCK_LONG(data,2,root);
      WRITE_BLOCK_LONG(data,3,table_type);

      if ( (res=mird_hashtrie_write(mtr,MASTER_MIRD_TABLE_ID,mtr->tables,
				    table_id,cell,&(mtr->tables),NULL,NULL)) )
	 return res;
      return MIRD_OK;
   }
}

/*
 * creates a new table
 */

MIRD_RES mird_low_table_new(struct mird_transaction *mtr,
			    UINT32 table_id,
			    UINT32 table_type)
{
   unsigned char *data;
   MIRD_RES res;
   UINT32 cell;

   if ( (mtr->db->flags & MIRD_READONLY) )
      return mird_generate_error_s(MIRDE_READONLY,
				   sstrdup("mird_low_table_new"),0,0,0);

   if (table_id==0) /* illegal to create master table */
      return mird_generate_error(MIRDE_MIRD_TABLE_EXISTS,table_id,0,0);

   if ( (res=mird_hashtrie_find_b(mtr->db,mtr->tables,table_id,&cell,NULL)) )
      return res;

   if (cell)
      return mird_generate_error(MIRDE_MIRD_TABLE_EXISTS,table_id,0,0);

   if ( (res=mird_frag_new(mtr,MASTER_MIRD_TABLE_ID,4*4,&cell,&data)) )
      return res;

   WRITE_BLOCK_LONG(data,0,CHUNK_ROOT);
   WRITE_BLOCK_LONG(data,1,table_id);
   WRITE_BLOCK_LONG(data,2,0);
   WRITE_BLOCK_LONG(data,3,table_type);

#ifdef MIRD_TABLE_DEBUG
   fprintf(stderr,"make; type=%lxh\n",table_type);
#endif

   if ( (res=mird_hashtrie_write(mtr,MASTER_MIRD_TABLE_ID,mtr->tables,table_id,cell,
				 &(mtr->tables),NULL,NULL)) )
      return res;

   mtr->flags|=MIRDT_DEPENDENCIES;
   if ( (res=mird_journal_log(mtr,MIRDJ_DEPEND_KEY,
			      0,table_id,cell)) )
      return res;

   return MIRD_OK;
}

/*
 * removes a table root
 */

static MIRD_RES mird_table_delete_root(struct mird_transaction *mtr,
				       UINT32 table_id)
{
   MIRD_RES res;
   UINT32 cell;

   if ( (res=mird_hashtrie_find_b(mtr->db,mtr->tables,table_id,&cell,NULL)) )
      return res;

   if (!cell)
      return mird_generate_error(MIRDE_NO_TABLE,table_id,0,0);

   if ( (res=mird_hashtrie_write(mtr,MASTER_MIRD_TABLE_ID,mtr->tables,table_id,0,
				 &(mtr->tables),NULL,NULL)) )
      return res;

   mtr->flags|=MIRDT_DEPENDENCIES;
   if ( (res=mird_journal_log(mtr,MIRDJ_DEPEND_KEY,
			      0,table_id,cell)) )
      return res;

   return MIRD_OK;
}


/* 
 * delete a whole table
 */

MIRD_RES mird_delete_table(struct mird_transaction *mtr,
			   UINT32 table_id)
{
   MIRD_RES res;
   UINT32 root;

   if ( (mtr->db->flags & MIRD_READONLY) )
      return mird_generate_error_s(MIRDE_READONLY,
				   sstrdup("mird_delete_table"),0,0,0);

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,NULL)) )
      return res;

   if ( (res=mird_hashtrie_free_all(mtr,root)) )
      return res;

   if ( (res=mird_table_delete_root(mtr,table_id)) )
      return res;

   /* don't care to write cache if we don't need to destroy it */
   if (mtr->cache_last_commited.lsb==mtr->db->last_commited.lsb &&
       mtr->cache_last_commited.msb==mtr->db->last_commited.msb &&
       mtr->cache_table_id==table_id)
      mtr->cache_last_commited.lsb=
	 mtr->cache_last_commited.msb=0; /* destroy */
   
   return MIRD_OK;
}


/*
 * adds a dependency for a whole table
 */

MIRD_RES mird_depend_table(struct mird_transaction *mtr,
			   UINT32 table_id)
{
   UINT32 cell;
   MIRD_RES res;

   if ( (mtr->db->flags & MIRD_READONLY) )
      return mird_generate_error_s(MIRDE_READONLY,
				   sstrdup("mird_depend_table"),0,0,0);

   if ( (res=mird_hashtrie_find_b(mtr->db,mtr->tables,table_id,&cell,NULL)) )
      return res;

   mtr->flags|=MIRDT_DEPENDENCIES;
   return mird_journal_log(mtr,MIRDJ_DEPEND_KEY,
			   0,table_id,cell);
}

/* 
 * creates a new hash key table
 */

MIRD_RES mird_key_new_table(struct mird_transaction *mtr,
			    UINT32 table_id)
{
   return mird_low_table_new(mtr,table_id,MIRD_TABLE_HASHKEY);
}

/*
 * saves or deletes a key/value pair in a table
 * if value is a NULL pointer, key is deleted
 */

#include <stdio.h>

MIRD_RES mird_low_key_store(struct mird_transaction *mtr,
			    UINT32 table_id,
			    UINT32 key,
			    unsigned char *value,
			    UINT32 len,
			    UINT32 table_type)
{
   MIRD_RES res;
   UINT32 root,new_root,type;
   UINT32 cell,old_cell=0,old_is_mine;

   if ( (mtr->db->flags & MIRD_READONLY) )
      return mird_generate_error_s(MIRDE_READONLY,
				   sstrdup("mird_low_key_store"),0,0,0);


   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
      return res;
   if (type!=table_type)
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,table_type);

   if (!value)
      cell=0;
   else if ( (res=mird_cell_write(mtr,table_id,key,len,value,&cell)) )
      return res;

   if ( (res=mird_hashtrie_write(mtr,table_id,root,key,cell,
				 &new_root,&old_cell,&old_is_mine)) )
      return res;


   if (new_root!=root &&
       (res=mird_table_write_root(mtr,table_id,new_root)))
      return res;
   
   mtr->cache_table_root=new_root;

   if ( !old_is_mine )
   {
      if ( (res=mird_journal_log(mtr,
				 cell?MIRDJ_WROTE_KEY:MIRDJ_DELETE_KEY,
				 table_id,key,old_cell)) )
	 return res;
   }
   else if (!old_cell)
      if ( (res=mird_journal_log(mtr,
				 cell?MIRDJ_REWROTE_KEY:MIRDJ_REDELETE_KEY,
				 table_id,key,old_cell)) )
	 return res;

   return MIRD_OK;
}

MIRD_RES mird_key_store(struct mird_transaction *mtr,
			UINT32 table_id,
			UINT32 key,
			unsigned char *value,
			UINT32 len)
{
   return mird_low_key_store(mtr,table_id,key,value,len,MIRD_TABLE_HASHKEY);
}

/*
 * gets a key from a table
 * "data" is allocated (can be freed by mird_cell_free_buffer),
 * or zero (NULL) if the key isn't found
 */

MIRD_RES mird_low_key_lookup(struct mird *db,
			     UINT32 root,
			     UINT32 key,
			     unsigned char **data,
			     UINT32 *len)
{
   UINT32 cell;
   MIRD_RES res;

   if ( (res=mird_hashtrie_find(db,root,key,&cell)) )
      return res;

   if (!cell) 
   {
      data[0]=NULL;
      len[0]=0;
      return MIRD_OK;
   }

   if ( (res=mird_cell_get(db,cell,&key,len,data)) )
      return res;

   return MIRD_OK;
}

MIRD_RES mird_transaction_key_lookup(struct mird_transaction *mtr,
				     UINT32 table_id,
				     UINT32 key,
				     unsigned char **data,
				     UINT32 *len)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
      return res;

   if (type!=MIRD_TABLE_HASHKEY) 
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_HASHKEY);

   return mird_low_key_lookup(mtr->db,root,key,data,len);
}
			

MIRD_RES mird_key_lookup(struct mird *db,
			 UINT32 table_id,
			 UINT32 key,
			 unsigned char **data,
			 UINT32 *len)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_db_table_get_root(db,table_id,&root,&type)) )
      return res;

   if (type!=MIRD_TABLE_HASHKEY) 
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_HASHKEY);

   return mird_low_key_lookup(db,root,key,data,len);
}
		

/*
 * find the first unused key in a table
 */

MIRD_RES mird_find_first_unused(struct mird *db,
				UINT32 table_id,
				UINT32 start_key,
				UINT32 *dest_key)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_db_table_get_root(db,table_id,&root,&type)) )
      return res;

   return mird_hashtrie_find_no_key(db,root,start_key,dest_key);
}

MIRD_RES mird_transaction_find_first_unused(struct mird_transaction *mtr,
					    UINT32 table_id,
					    UINT32 start_key,
					    UINT32 *dest_key)
{
   UINT32 root,type;
   MIRD_RES res;

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
      return res;

   return mird_hashtrie_find_no_key(mtr->db,root,start_key,dest_key);
}

MIRD_RES mird_find_first_unused_table(struct mird *db,
				      UINT32 start_no,
				      UINT32 *dest_table_id)
{
   if (start_no==0) start_no=1;
   return mird_hashtrie_find_no_key(db,db->tables,start_no,dest_table_id);
}

MIRD_RES mird_transaction_find_first_unused_table(struct mird_transaction *mtr,
						  UINT32 start_no,
						  UINT32 *dest_table_id)
{
   if (start_no==0) start_no=1;
   return mird_hashtrie_find_no_key(mtr->db,mtr->tables,start_no,
				    dest_table_id);
}


/*
 * resolve tables
 */

MIRD_RES mird_tables_resolve(struct mird_transaction *mtr)
{
   MIRD_RES res;
#define RESOLVE_NUMBER 16
   UINT32 id[RESOLVE_NUMBER],cell[RESOLVE_NUMBER];
   UINT32 n,i,new_root,len,my_root,old_root;
   unsigned char *data;

   /* resolve master table */

   if ( (res=mird_hashtrie_resolve(mtr,
				   0,  /* master table id */
				   mtr->tables,
				   mtr->db->tables,
				   &(mtr->tables))) )
      return res;

   /* for each table we should sync,
    * call mird_hashtrie_resolve
    */

   if ( (res=mird_tr_hashtrie_first(mtr,
				    mtr->tables,
				    RESOLVE_NUMBER,
				    id,cell,&n)) )
      return res;

   while (n)
   {
      for (i=0; i<n; i++)
      {
	 UINT32 root_cell;
	 /* we need to free the old root */
	 if ( (res=mird_hashtrie_find(mtr->db,mtr->db->tables,
				      id[i],&root_cell)) )
	    return res;
	 if ( root_cell &&
	      (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,root_cell))) )
	    return res;

	 if ( (res=mird_db_table_get_root(mtr->db,id[i],&old_root,NULL)) )
	 {
	    if (res->error_no==MIRDE_NO_TABLE)
	    {
	       mird_free_error(res);
	       old_root=0;
	    }
	    else
	       return res;
	 }

	 if ( (res=mird_frag_get(mtr->db,cell[i],&data,&len)) )
	    return res;
	 
	 if (READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
	    return mird_generate_error(MIRDE_WRONG_CHUNK,cell[i],
				       CHUNK_ROOT,READ_BLOCK_LONG(data,0));
   
	 my_root=READ_BLOCK_LONG(data,2);

	 if ( (res=mird_hashtrie_resolve(mtr,
					 id[i],
					 my_root,
					 old_root,
					 &new_root)) )
	    return res;

	 if (new_root!=my_root)
	 {
	    if ( (res=mird_frag_get_w(mtr,cell[i],&data,&len)) )
	       return res;
	    WRITE_BLOCK_LONG(data,2,new_root);
	 }
      }

      if ( (res=mird_tr_hashtrie_next(mtr,
				      mtr->tables,
				      id[n-1],
				      RESOLVE_NUMBER,
				      id,cell,&n)) )
	 return res;
   }

   return MIRD_OK;
}
				    
MIRD_RES mird_tables_mark_use(struct mird *db,
			      UINT32 tables,
			      UINT32 table_id,
			      UINT32 key,
			      struct mird_status_list *mul)
{
   UINT32 root;
   MIRD_RES res;

   if (table_id)
   {
      if ( (res=mird_db_table_get_root(db,table_id,&root,NULL)) )
      {
	 if (res->error_no==MIRDE_NO_TABLE)
	 {
	    mird_free_error(res);
	    return MIRD_OK; 
	 }
	 return res;
      }

      if ( (res=mird_hashtrie_mark_use(db,root,key,mul)) )
	 return res;
   }
   else
   {
      if ( (res=mird_hashtrie_mark_use(db,db->tables,key,mul)) )
	 return res;
   }

   return MIRD_OK;
}

/*
 * scan through a table
 *
 * if prev is NULL, scan will start from first key,
 * otherwise prev will be freed (or reused)
 *
 * note: it will be freed upon error too.
 */

MIRD_RES mird_low_table_scan(struct mird *db,
			     UINT32 root,
			     UINT32 n,
			     struct mird_scan_result *prev,
			     struct mird_scan_result **dest)
{
   UINT32 *dest_key;
   UINT32 *dest_cell;
   MIRD_RES res;
   UINT32 i;

   dest[0]=NULL;

   if ( (res=MIRD_MALLOC(sizeof(UINT32)*n,&dest_key)) ) goto clean;
   if ( (res=MIRD_MALLOC(sizeof(UINT32)*n,&dest_cell)) ) goto clean;

   if ( (res=MIRD_MALLOC(sizeof(struct mird_scan_result)+
			 sizeof(struct mird_scan_tupel)*n,dest)) )
      goto clean;

   dest[0]->n=0;

   if (prev)
   {
      UINT32 key=prev->tupel[prev->n-1].key;
      
      if ( (res=mird_hashtrie_next(db,root,key,n,dest_key,dest_cell,&n)) )
	 goto clean;
   }
   else
   {
      if ( (res=mird_hashtrie_first(db,root,n,dest_key,dest_cell,&n)) )
	 goto clean;
   }

   if (prev) { mird_free_scan_result(prev); prev=NULL; }

   if (!n) /* no more tupels */
   { 
      res=MIRD_OK; 
      goto clean; 
   }

   for (i=0; i<n; i++)
   {
      if ( (res=mird_cell_get(db,dest_cell[i],
			      &(dest[0]->tupel[i].key),
			      &(dest[0]->tupel[i].value_len),
			      &(dest[0]->tupel[i].value))) )
	 goto clean;
      dest[0]->n++;
   }
   res=MIRD_OK; 
   goto complete;

clean:
   if (dest[0]) sfree(dest[0]); 
   dest[0]=NULL;
complete:
   if (dest_key) sfree(dest_key);
   if (dest_cell) sfree(dest_cell);
   if (prev) mird_free_scan_result(prev);

   return res;
}
	
#include <stdio.h>

MIRD_RES mird_table_scan(struct mird *db,
			 UINT32 table_id,
			 UINT32 n,
			 struct mird_scan_result *prev,
			 struct mird_scan_result **dest)
{
   UINT32 root,type;
   MIRD_RES res;

   dest[0]=NULL;

   if ( (res=mird_db_table_get_root(db,table_id,&root,&type)) )
   {
      if (prev) mird_free_scan_result(prev);
      return res;
   }

   if (type!=MIRD_TABLE_HASHKEY) 
   {
      if (prev) mird_free_scan_result(prev);
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_HASHKEY);
   }

   return mird_low_table_scan(db,root,n,prev,dest);
}
		     
MIRD_RES mird_transaction_table_scan(struct mird_transaction *mtr,
				     UINT32 table_id,
				     UINT32 n,
				     struct mird_scan_result *prev,
				     struct mird_scan_result **dest)
{
   UINT32 root,type;
   MIRD_RES res;

   dest[0]=NULL;

   if ( (res=mird_tr_table_get_root(mtr,table_id,&root,&type)) )
   {
      if (prev) mird_free_scan_result(prev);
      return res;
   }

   if (type!=MIRD_TABLE_HASHKEY) 
   {
      if (prev) mird_free_scan_result(prev);
      return mird_generate_error(MIRDE_WRONG_TABLE,table_id,
				 type,MIRD_TABLE_HASHKEY);
   }

   return mird_low_table_scan(mtr->db,root,n,prev,dest);
}
		     
void mird_free_scan_result(struct mird_scan_result *msr)
{
   while (msr->n--)
      if (msr->tupel[msr->n].value) sfree(msr->tupel[msr->n].value);
   sfree(msr);
}

MIRD_RES mird_scan_continued(UINT32 key,struct mird_scan_result **dest)
{
   MIRD_RES res;
   if ( (res=MIRD_MALLOC(sizeof(struct mird_scan_result)+
			 sizeof(struct mird_scan_tupel)*1,dest)) )
   {
      dest[0]=NULL;
      return res;
   }
   dest[0]->n=1;
   dest[0]->tupel[0].key=key;
   dest[0]->tupel[0].value=NULL;
   return MIRD_OK;
}

MIRD_RES mird_scan_continuation(struct mird_scan_result *msr,UINT32 *key)
{
   if (!msr || !msr->n)
      return mird_generate_error(MIRDE_SCAN_END,0,0,0);
   key[0]=msr->tupel[msr->n-1].key;
   return MIRD_OK;
}



MIRD_RES mird_get_table_type(struct mird *db,
			     mird_key_t table_id,
			     mird_key_t *table_type)
{
   UINT32 root;

   return mird_db_table_get_root(db,table_id,&root,table_type);
}

MIRD_RES mird_transaction_get_table_type(struct mird_transaction *mtr,
					 mird_key_t table_id,
					 mird_key_t *table_type)
{
   UINT32 root;

   return mird_tr_table_get_root(mtr,table_id,&root,table_type);
}
