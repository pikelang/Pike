/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: transaction.c,v 1.5 2003/01/03 20:52:49 grubba Exp $
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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef MASSIVE_DEBUG
#define RESOLVE_DEBUG
#endif

#ifdef RESOLVE_DEBUG
#include <stdio.h>
#endif


static const char RCSID[]=
   "$Id: transaction.c,v 1.5 2003/01/03 20:52:49 grubba Exp $";

/*
 * start a new transaction
 */

MIRD_RES mird_transaction_new(struct mird *db,
			       struct mird_transaction **mtr)
{
   MIRD_RES res;

   if ( (res=MIRD_MALLOC(sizeof(struct mird_transaction),mtr)) )
      return res;

   if ( (res=MIRD_MALLOC(sizeof(struct transaction_free_frags)*
			 db->max_free_frag_blocks,&(mtr[0]->free_frags))) )
   {
      sfree(*mtr);
      return res;
   }

   mtr[0]->db=db;
   mtr[0]->next=db->first_transaction;
   mtr[0]->tables=db->tables;
   mtr[0]->n_free_frags=0;
   mtr[0]->flags=0;
   mtr[0]->checksum=0;

   /* remember this to see if we need resolve */
   mtr[0]->last_commited=db->last_commited;

   /* don't use cache (transaction 0:0 doesn't exist) */
   mtr[0]->cache_last_commited.msb=
      mtr[0]->cache_last_commited.lsb=0;
   mtr[0]->cache_table_id=0;
   mtr[0]->cache_table_root=0;
   mtr[0]->cache_table_type=0;

   if (!(db->flags&MIRD_READONLY))
   {
      if ( (res=mird_journal_get_offset(db,&(mtr[0]->offset))) )
      {
	 sfree(mtr[0]->free_frags);
	 sfree(mtr[0]);
	 return res; 
      }

      mtr[0]->no=db->next_transaction;
      if ( (res=mird_journal_log(mtr[0],MIRDJ_NEW_TRANSACTION,0,0,0)) )
      {
	 sfree(mtr[0]->free_frags);
	 sfree(mtr[0]);
	 return res;
      }

/* all ok, increase count */
      if (++db->next_transaction.lsb==0)
	 ++db->next_transaction.msb;
   }

   /* link it in */
   db->first_transaction=mtr[0];
   return MIRD_OK;
}

/*
 * finish a transaction
 * frees the structure
 */

MIRD_RES mird_transaction_close(struct mird_transaction *mtr)
{
   MIRD_RES res;
   struct mird *db;

   if ( (mtr->db->flags & MIRD_READONLY) )
   {
      mird_tr_free(mtr);
      return MIRD_OK;
   }

   if ((mtr->flags & (MIRDT_REWOUND_START|MIRDT_CLOSED)))
      return mird_generate_error(MIRDE_TR_CLOSED,0,0,0);

   if ( (res=mird_tr_resolve(mtr)) ) return res;

   if ( (res=mird_tr_finish(mtr)) ) return res;

   db=mtr->db;
   mird_tr_free(mtr);
   if (db &&
       (db->flags&MIRD_PLEASE_SYNC) &&
       !(db->first_transaction))
      return mird_sync(db);

   return MIRD_OK;
}

/*
 * cancel a transaction
 * frees the structure
 */

MIRD_RES mird_transaction_cancel(struct mird_transaction *mtr)
{
   MIRD_RES res;
   struct mird *db;

   if ( (mtr->db->flags & MIRD_READONLY) )
   {
      mird_tr_free(mtr);
      return MIRD_OK;
   }

   if ( (res=mird_tr_rewind(mtr)) ) return res;

   db=mtr->db;
   mird_tr_free(mtr);
   if (db &&
       (db->flags&MIRD_PLEASE_SYNC) &&
       !(db->first_transaction))
      return mird_sync(db);

   return MIRD_OK;
}

/*
 * frees the transaction structure
 */

void mird_tr_free(struct mird_transaction *mtr)
{
   struct mird_transaction **prev;

   if (mtr->db)
   {
      /* unlink it from the transaction */

      prev=&(mtr->db->first_transaction);
      while (prev[0]!=mtr)
	 if (!prev[0]) 
	    mird_fatal("transaction not in list\n");
	 else
	    prev=&(prev[0]->next);
   
      prev[0]=mtr->next;

      mtr->db=NULL;
   }

   sfree(mtr->free_frags);
   sfree(mtr);
}

/*
 * rewinds a transaction's actions
 */

MIRD_RES mird_tr_rewind(struct mird_transaction *mtr)
{
   MIRD_RES res;
   UINT32 *ent,*cur;
   MIRD_OFF_T pos;
   UINT32 n;

   const UINT32 msbn=htonl(mtr->no.msb);
   const UINT32 lsbn=htonl(mtr->no.lsb);
   const UINT32 allon=htonl(MIRDJ_ALLOCATED_BLOCK);

   if ( (mtr->flags & MIRDT_CLOSED) )
      return mird_generate_error(MIRDE_TR_CLOSED,0,0,0);

   if ( (mtr->flags & MIRDT_REWOUND_START) )
   {
      pos=mtr->cont_offset;
   }
   else
   {
      mtr->flags|=MIRDT_REWOUND_START;
      pos=mtr->cont_offset=mtr->offset;

      if ( (res=mird_cache_cancel_transaction(mtr)) )
	 return res;
   }

   /* flush journal for readback */
   if ( (res=mird_journal_log_flush(mtr->db)) )
      return res;

   /* loop through transaction entries 
    * free allocated blocks 
    */


   if ( (res=MIRD_MALLOC(MIRD_JOURNAL_ENTRY_SIZE*mtr->db->journal_readback_n,
			 &ent)) )
      return res;

   for (;;)
   {
      if ( (res=mird_journal_get(mtr->db,pos,
				 mtr->db->journal_readback_n,
				 (unsigned char*)ent,&n)) )
      {
	 sfree(ent);
	 return res;
      }
      
      if (!n)
      {
	 sfree(ent);
	 break;
      }

      cur=ent;
      while (n--)
      {
	 if (cur[2]==lsbn &&
	     cur[1]==msbn) /* us */
	    if (cur[0]==allon)
	    {
	       mtr->cont_offset=pos;
	       /* please free our allocation */
	       if ( (res=mird_tr_unused(mtr,READ_BLOCK_LONG(cur,3))) )
	       {
		  sfree(ent);
		  return res;
	       }
	    }
	 cur+=MIRD_JOURNAL_ENTRY_SIZE/4;
	 pos+=MIRD_JOURNAL_ENTRY_SIZE;
      }
   }
   if ( (res=mird_journal_log(mtr,MIRDJ_TRANSACTION_CANCEL,
			      0,0,mtr->checksum)) )
      return res;
   mtr->flags|=MIRDT_CLOSED; /* all done! */

   return MIRD_OK;
}

/*
 * finishes a transaction's actions
 * resolve conflicts first
 */

MIRD_RES mird_tr_finish(struct mird_transaction *mtr)
{
   MIRD_RES res;

   if ( (mtr->flags & MIRDT_CLOSED) )
      return mird_generate_error(MIRDE_TR_CLOSED,0,0,0);

   /* confirm close */

   if ( (res=mird_frag_close(mtr)) )
      return res;

   /* flush cache */
   if ( (res=mird_cache_flush_transaction(mtr)) )
      return res;

   /* if we have dependencies, 
    * and there is other transactions,
    */
   if ( (mtr->flags & MIRDT_DEPENDENCIES) &&
	(mtr->db->first_transaction!=mtr || mtr->next) )
   {
      /* convert dependencies to lock entries in the journal file */

      const UINT32 dependn=htonl(MIRDJ_DEPEND_KEY);
      const UINT32 msbn=htonl(mtr->no.msb);
      const UINT32 lsbn=htonl(mtr->no.lsb);
      UINT32 *ent,*cur;
      MIRD_OFF_T pos=mtr->offset;
      UINT32 n;

      /* flush for readback */
      if ( (res=mird_journal_log_flush(mtr->db)) )
	 return res;


      if ( (res=MIRD_MALLOC(MIRD_JOURNAL_ENTRY_SIZE*
			    mtr->db->journal_readback_n, &ent)) )
	 return res;
      
#ifdef RESOLVE_DEBUG
      fprintf(stderr,"resolve pos=%ld\n",pos);
#endif

      if ( (res=mird_journal_get(mtr->db,pos,
				 mtr->db->journal_readback_n,
				 (unsigned char*)ent,&n)) )
	 goto clean;

      if (!n)
      {
	 res=MIRD_OK;
	 goto clean;
      }

      cur=ent;
      while (n--)
      {
	 if (cur[0]==dependn && cur[2]==lsbn && cur[1]==msbn) 
	 {
#ifdef RESOLVE_DEBUG
	    fprintf(stderr,"dependn %lxh %lxh\n",
		    READ_BLOCK_LONG(cur,3),
		    READ_BLOCK_LONG(cur,4));
#endif
		    
	    if ( (res=mird_journal_log(mtr,MIRDJ_KEY_LOCK,
				       READ_BLOCK_LONG(cur,3),
				       READ_BLOCK_LONG(cur,4), 0)) )
	       goto clean;
	 }
	 cur+=MIRD_JOURNAL_ENTRY_SIZE/4;
	 pos+=MIRD_JOURNAL_ENTRY_SIZE;
      }
clean:
      sfree(ent);
      if (res) return res;
   }

   /* log this */
   if ( (res=mird_journal_log(mtr,MIRDJ_TRANSACTION_FINISHED,
			      mtr->tables,0,mtr->checksum)) )
      return res;

   if ( (res=mird_journal_log_flush(mtr->db)) )
      return res;

   /* save our tables as the new tables */

   mtr->db->last_commited=mtr->no;
   mtr->db->cache_table_id=0; /* destroy */
   mtr->db->tables=mtr->tables;
   mtr->flags|=MIRDT_CLOSED;

   /* write the new superblocks */

   if ( (res=mird_save_state(mtr->db,0)) )
      return res;

   if ( (mtr->db->flags & MIRD_SYNC_END) )
   {
      MIRD_SYSCALL_COUNT(mtr->db,0);
      if ( FDATASYNC(mtr->db->db_fd)==-1 )
	 return mird_generate_error(MIRDE_DB_SYNC,0,errno,0);
      MIRD_SYSCALL_COUNT(mtr->db,0);
      if ( FDATASYNC(mtr->db->jo_fd)==-1 )
	 return mird_generate_error(MIRDE_JO_SYNC,0,errno,0);
      if ( ( mtr->db->flags & MIRD_CALL_SYNC ) )
      {
	 MIRD_SYSCALL_COUNT(mtr->db,0);
	 sync(); /* may not fail */
      }
   }

   return MIRD_OK;
}

/*
 * resolves conflicts
 */

INLINE MIRD_RES mird_tr_resolve_cont(struct mird_transaction *mtr)
{
   /* go through all our journal entries */
   /* check if our "old id" per key is still the same */
   /* if not, report conflict */
   
   MIRD_RES res;
   UINT32 *ent,*cur;
   MIRD_OFF_T pos;
   UINT32 n;

   UINT32 last_root=0,last_table_id=0,last_type=0;
   UINT32 my_last_root=0,my_last_table_id=0;

   const UINT32 msbn=htonl(mtr->no.msb);
   const UINT32 lsbn=htonl(mtr->no.lsb);
   const UINT32 wroten=htonl(MIRDJ_WROTE_KEY);
   const UINT32 dependn=htonl(MIRDJ_DEPEND_KEY);
   const UINT32 deleten=htonl(MIRDJ_DELETE_KEY);
   const UINT32 rewroten=htonl(MIRDJ_REWROTE_KEY);
   const UINT32 redeleten=htonl(MIRDJ_REDELETE_KEY);
   const UINT32 keylockn=htonl(MIRDJ_KEY_LOCK);

   struct mird_status_list *msl=NULL;

   pos=mtr->cont_offset;

   if ( (res=MIRD_MALLOC(MIRD_JOURNAL_ENTRY_SIZE*mtr->db->journal_readback_n,
			 &ent)) )
      return res;

   for (;;)
   {
#ifdef RESOLVE_DEBUG
      fprintf(stderr,"resolve pos=%ld\n",pos);
#endif

      if ( (res=mird_journal_get(mtr->db,pos,
				 mtr->db->journal_readback_n,
				 (unsigned char*)ent,&n)) )
	 goto clean;

      if (!n)
      {
	 /* all done */
	 res=MIRD_OK;
	 break;
      }

      cur=ent;
      while (n--)
      {
#ifdef RESOLVE_DEBUG
	 fprintf(stderr,"resolve run %c%c%c%c %ld:%ld %lxh %lxh %lxh\n",
		 FOURC(READ_BLOCK_LONG(cur,0)),
		 READ_BLOCK_LONG(cur,1),
		 READ_BLOCK_LONG(cur,2),
		 READ_BLOCK_LONG(cur,3),
		 READ_BLOCK_LONG(cur,4),
		 READ_BLOCK_LONG(cur,5));
#endif

	 if (cur[2]==lsbn &&
	     cur[1]==msbn) /* us */
	 {
	    if (cur[0]==wroten ||
		cur[0]==dependn ||
		cur[0]==deleten)
	    {
	       /* check if old chunk id still is active */
	       UINT32 table_id=READ_BLOCK_LONG(cur,3);
	       UINT32 key     =READ_BLOCK_LONG(cur,4);
	       UINT32 old_cell=READ_BLOCK_LONG(cur,5);

	       UINT32 root,cell,type;
	       
#ifdef RESOLVE_DEBUG
	       fprintf(stderr,"%s table %lxh key %lxh\n",
		       (cur[0]==wroten)?"WROT":
		       (cur[0]==deleten)?"DELE":"DEPE",
		       table_id,key);
#endif

	       if (msl && cur[0]!=dependn)
	       {
		  UINT32 status;
		  if ( (res=mird_status_get(msl,table_id,key,&status)) )
		     goto clean;

		  if (status) /* we've checked this already */
		  {
		     if ( (res=mird_status_set(msl,table_id,key,
					       (cur[0]==wroten)?1:2)) )
			goto clean;

		     goto cont; /* skip this one */
		  }
	       }

	       if (cur[0]==deleten)
	       {
		  if (!msl) 
		     if ( (res=mird_status_new(mtr->db,&msl)) )
			goto clean;
		  if ( (res=mird_status_set(msl,table_id,key,2)) )
		     goto clean;
	       }
	       
	       if (last_table_id==table_id && table_id)
	       {
		  root=last_root;
		  type=last_type;
	       }
	       else if (!table_id)
	       {
		  type=MIRD_TABLE_MASTER;
		  root=mtr->db->tables;
	       }
	       else
	       {
		  if ( (res=mird_db_table_get_root(mtr->db,table_id,
						   &root,&type)) )
		  {
		     if (res->error_no==MIRDE_NO_TABLE)
		     {
			mird_free_error(res);
			if ( (res=mird_tr_table_get_root(mtr,table_id,
							 NULL,&type)) )
			   goto clean;
			last_type=type;
			last_root=root=0;
		     }
		     else
			goto clean;
		  }
		  else
		  {
		     last_root=root;
		     last_type=type;
		  }
		  last_table_id=table_id;
	       }
	       
	       if ( (res=mird_hashtrie_find(mtr->db,root,key,&cell)) )
		  goto clean;

	       if (cell!=old_cell) /* conflict */
	       {
#ifdef RESOLVE_DEBUG
		  fprintf(stderr,"conflict; key=%lxh old=%lxh%ld new=%lxh%ld; root=%lxh\n",
			  key,
			  CHUNK_ID_2_BLOCK(mtr->db,old_cell),
			  CHUNK_ID_2_FRAG(mtr->db,old_cell),
			  CHUNK_ID_2_BLOCK(mtr->db,cell),
			  CHUNK_ID_2_FRAG(mtr->db,cell),
			  root);
#endif

		  switch (type)
		  {
		     case MIRD_TABLE_STRINGKEY:
			if ( (res=mird_s_key_resolve(mtr,
						     table_id,
						     key,
						     old_cell,
						     cell)) )
			{
			   if (res->error_no==MIRDE_CONFLICT_S &&
			       cur[0]==dependn)
			      res->error_no=MIRDE_DEPEND_BROKEN_S;
			   goto clean;
			}
			break;

		     case MIRD_TABLE_HASHKEY:
			res=mird_generate_error(
			   (cur[0]==dependn) 
			   ? MIRDE_DEPEND_BROKEN 
			   : MIRDE_CONFLICT,
			   table_id,key,0);
			goto clean;

		     case MIRD_TABLE_MASTER:
			res=mird_generate_error(
			   MIRDE_MIRD_TABLE_DEPEND_BROKEN,
			   0,key,0);
			goto clean;
			
		     default:
			res=mird_generate_error(MIRDE_WRONG_TABLE,
						table_id,
						type,
						0);
			goto clean;
		  }
	       }
#ifdef RESOLVE_DEBUG
	       else
		  fprintf(stderr,"no conflict; key=%lxh old=%lxh%ld new=%lxh%ld; root=%lxh\n",
			  key,
			  CHUNK_ID_2_BLOCK(mtr->db,old_cell),
			  CHUNK_ID_2_FRAG(mtr->db,old_cell),
			  CHUNK_ID_2_BLOCK(mtr->db,cell),
			  CHUNK_ID_2_FRAG(mtr->db,cell),
			  root);
#endif

	    }
	    else if (cur[0]==rewroten ||
		     cur[0]==redeleten)
	    {
	       /* check if old chunk id still is active */
	       UINT32 table_id=READ_BLOCK_LONG(cur,3);
	       UINT32 key     =READ_BLOCK_LONG(cur,4);

#ifdef RESOLVE_DEBUG
	       fprintf(stderr,"%s table %lxh key %lxh\n",
		       (cur[0]==rewroten)?"RWRO":"RDEL",
		       table_id,key);
#endif

	       if (msl)
	       {
		  UINT32 status;
		  if ( (res=mird_status_get(msl,table_id,key,&status)) )
		     goto clean;

		  if (status || cur[0]==redeleten) 
		  {
		     if ( (res=mird_status_set(msl,table_id,key,
					       (cur[0]==rewroten)?1:2)) )
			goto clean;
		  }
	       }
	       else if (cur[0]==redeleten)
	       {
		  if (!msl) 
		     if ( (res=mird_status_new(mtr->db,&msl)) )
			goto clean;
		  if ( (res=mird_status_set(msl,table_id,key,2)) )
		     goto clean;
	       }
	    }
	 }
	 else if (cur[0]==keylockn)
	 {
	    /* other transaction has locked an entry;
	     * check so we didn't change it
	     */

	    UINT32 table_id=READ_BLOCK_LONG(cur,3);
	    UINT32 key     =READ_BLOCK_LONG(cur,4);
	    unsigned char *data;
	    UINT32 root,type,cell;

#ifdef RESOLVE_DEBUG
	    fprintf(stderr,"key lock table %lxh key %lxh\n",table_id,key);
#endif

	    if (my_last_table_id==table_id && table_id)
	       root=my_last_root;
	    else if (!table_id)
	       root=mtr->tables;
	    else
	    {
	       if ( (res=mird_tr_table_get_root(mtr,table_id,
						&root,&type)) )
	       {
		  if (res->error_no!=MIRDE_NO_TABLE)
		     goto clean;
		  mird_free_error(res);
		  root=0;
	       }
	       else
		  my_last_root=root;
	       my_last_table_id=table_id;
	    }

	    if ( (res=mird_hashtrie_find_b(mtr->db,root,key,&cell,&data)) )
	       goto clean;

#ifdef RESOLVE_DEBUG
	    fprintf(stderr,"   root=%lxh cell=%lxh\n",root,cell);

	    if (cell)
	       fprintf(stderr,"   owner: %ld:%ld\n",
		       READ_BLOCK_LONG(data,0),
		       READ_BLOCK_LONG(data,1));
#endif

	    if (cell && 
		READ_BLOCK_LONG(data,1)==mtr->no.lsb &&
		READ_BLOCK_LONG(data,0)==mtr->no.msb)
	    {
	       if (table_id) 
		  res=mird_generate_error(MIRDE_CONFLICT,table_id,key,0);
	       else
		  res=mird_generate_error(MIRDE_MIRD_TABLE_DEPEND_BROKEN,0,key,0);

	       goto clean;
	    }
	    else
	    {
	       UINT32 z;
	       
	       if (!msl) 
	       {
		  if ( (res=mird_status_new(mtr->db,&msl)) )
		     goto clean;
	       }
	       else
	       {
		  if ( (res=mird_status_get(msl,table_id,key,&z)) )
		     goto clean;

		  if (z)
		     if (table_id) 
			res=mird_generate_error(MIRDE_CONFLICT,table_id,key,0);
		     else
			res=mird_generate_error(MIRDE_MIRD_TABLE_DEPEND_BROKEN,
						0,key,0);
	       }
	       if ( (res=mird_status_set(msl,table_id,key,3)) )
		  goto clean;
	    }
	 }
cont:
	 cur+=MIRD_JOURNAL_ENTRY_SIZE/4;
	 pos+=MIRD_JOURNAL_ENTRY_SIZE;
      }
   }

/*    fprintf(stderr,"uh\n"); */

   if (msl) /* there may be cells to be redeleted,
	     * or lock journal entries
	     */
   {
      struct mul_entry **entp,*ent;
      long n;

#ifdef RESOLVE_DEBUG      
      fprintf(stderr,"msl\n");
#endif

      n=msl->size;
      entp=msl->entries;
      while (n--)
      {
	 ent=*(entp++);
	 while (ent)
	 {
#ifdef RESOLVE_DEBUG
	    fprintf(stderr,"table %lxh key %lxh status %ld\n",
		    ent->x,ent->y,ent->status);
#endif

	    if (ent->status==2)
	    {
	       UINT32 table_id=ent->x;
	       UINT32 key=ent->y;
	       UINT32 root;

#ifdef RESOLVE_DEBUG
	       fprintf(stderr,"redelete table %lxh key %lxh\n",table_id,key);
#endif

	       if (my_last_table_id==table_id && table_id)
		  root=my_last_root;
	       else if (table_id)
	       {
		  if ( (res=mird_tr_table_get_root(mtr,table_id,
						   &root,NULL)) )
		     goto clean;

		  my_last_table_id=table_id;
		  my_last_root=root;
	       }
	       else
		  root=mtr->tables;
	       
	       if ( (res=mird_hashtrie_redelete(mtr,table_id,
						root,key,&root)) )
		  goto clean;

	       if (!table_id)
		  mtr->tables=root;
	       else if (root!=my_last_root)
	       {
		  if ( (res=mird_table_write_root(mtr,
						  table_id,
						  root)) )
		     goto clean;
	       }
	    }
	    ent=ent->next;
	 }
      }
   }

clean:
#ifdef RESOLVE_DEBUG
   fprintf(stderr,"clean %p\n",res);
#endif

   if (msl) mird_status_free(msl);

   sfree(ent);
   return res;
}


MIRD_RES mird_tr_resolve(struct mird_transaction *mtr)
{
   MIRD_RES res;

#if 1
   if (mtr->last_commited.msb == mtr->db->last_commited.msb &&
       mtr->last_commited.lsb == mtr->db->last_commited.lsb)
   {
      /* nothing to resolve against */
      return MIRD_OK;
   }
#endif

   if ( (res=mird_tables_resolve(mtr)) )
      return res;

   mtr->cont_offset=mtr->offset;

   if ( (res=mird_tr_resolve_cont(mtr)) )
      return res;

   mtr->last_commited=mtr->db->last_commited;
   return MIRD_OK;
}

/* 
 * continues to resolve conflicts
 */

/*
 * allocates a block for a transaction
 */

MIRD_RES mird_tr_new_block(struct mird_transaction *mtr,
			   UINT32 *no,
			   unsigned char **data)
{
   MIRD_RES res;
   if ( (res=mird_freelist_pop(mtr->db,no)) )
      return res;

   if ( (res=mird_journal_log(mtr,MIRDJ_ALLOCATED_BLOCK,
			      *no,0,0)) )
      return res;
   
   if ( (res=mird_block_zot(mtr->db,*no,data)) )
      return res; /* error */

   return MIRD_OK;
}

MIRD_RES mird_tr_new_block_no(struct mird_transaction *mtr,
			      UINT32 *no)
{
   MIRD_RES res;
   if ( (res=mird_freelist_pop(mtr->db,no)) )
      return res;

   if ( (res=mird_journal_log(mtr,MIRDJ_ALLOCATED_BLOCK,
			      *no,0,0)) )
      return res;

   return MIRD_OK;
}

/*
 * journals that some chunk is left unused 
 */

MIRD_RES mird_tr_unused(struct mird_transaction *mtr,
			UINT32 block_id)
{
   MIRD_RES res;

   if ( (res=mird_journal_log(mtr,MIRDJ_BLOCK_UNUSED,
			      block_id,0,0)) )
      return res;

   return MIRD_OK;
}

/*
 * journal readback simulation: new transaction 
 */

MIRD_RES mird_simul_tr_new(struct mird *db,
			   UINT32 msb,
			   UINT32 lsb,
			   MIRD_OFF_T offset)
{
   MIRD_RES res;
   struct mird_transaction *mtr;

   if ( (res=MIRD_MALLOC(sizeof(struct mird_transaction),&mtr)) )
      return res;

   mtr->db=db;
   mtr->next=db->first_transaction;
   mtr->tables=db->tables;
   mtr->flags=0;

   mtr->no.msb=msb;
   mtr->no.lsb=lsb;
   mtr->offset=offset;
   mtr->checksum=0;

   /* link it in */
   db->first_transaction=mtr;
   return MIRD_OK;
}

/*
 * journal readback simulation: find transaction 
 */

MIRD_RES mird_simul_tr_find(struct mird *db,
			    UINT32 no_msb,
			    UINT32 no_lsb,
			    struct mird_transaction **mtr_p)
{
   struct mird_transaction *mtr;   

   mtr=db->first_transaction;
   while (mtr)
      if (mtr->no.msb==no_msb &&
	  mtr->no.lsb==no_lsb)
      {
	 mtr_p[0]=mtr;
	 return MIRD_OK;
      }
      else 
	 mtr=mtr->next;

   mtr_p[0]=NULL;

   return MIRD_OK;
}

/*
 * journal readback simulation: free transaction 
 */

void mird_simul_tr_free(struct mird *db,
			UINT32 msb,UINT32 lsb)
{
   struct mird_transaction **prev;
   struct mird_transaction *mtr;

   /* unlink it from the transaction list */

   prev=&(db->first_transaction);
   while (prev[0]->no.msb!=msb ||
	  prev[0]->no.lsb!=lsb)
      if (!prev[0]) 
	 return; /* not in list */
      else
	 prev=&(prev[0]->next);
   
   mtr=prev[0];
   prev[0]=mtr->next;

   sfree(mtr);
}

/*
 * journal readback simulation: verify a transaction's completion
 */

MIRD_RES mird_simul_tr_verify(struct mird_transaction *mtr)
{
   MIRD_RES res;
   UINT32 *ent,*cur;
   MIRD_OFF_T pos;
   UINT32 n;

   UINT32 msbn=htonl(mtr->no.msb);
   UINT32 lsbn=htonl(mtr->no.lsb);
   UINT32 allon=htonl(MIRDJ_ALLOCATED_BLOCK);
   UINT32 finishn=htonl(MIRDJ_TRANSACTION_FINISHED);
   UINT32 frag_progress_n=htonl(BLOCK_FRAG_PROGRESS);

   if ( (mtr->flags & MIRDT_CLOSED) )
      return mird_generate_error(MIRDE_TR_CLOSED,0,0,0);

   /* loop through transaction entries 
    * check all allocated blocks 
    */

   if ( (res=MIRD_MALLOC(MIRD_JOURNAL_ENTRY_SIZE*mtr->db->journal_readback_n,
			 &ent)) )
      return res;

   pos=mtr->offset;

   for (;;)
   {
      if ( (res=mird_journal_get(mtr->db,pos,
				 mtr->db->journal_readback_n,
				 (unsigned char*)ent,&n)) )
      {
	 sfree(ent);
	 return res;
      }
      
      if (!n)
      {
	 /* this shouldn't happen, where's the finish marker? */
	 sfree(ent);
	 return mird_generate_error(MIRDE_VERIFY_FAILED,0,0,0);
      }

      cur=ent;
      while (n--)
      {
	 if (cur[2]==lsbn &&
	     cur[1]==msbn) /* us */
	    if (cur[0]==allon)
	    {
	       unsigned char *bdata;
	       UINT32 b=READ_BLOCK_LONG(cur,3);

	       if ( (res=mird_block_get(mtr->db,b,&bdata)) )
	       {
		  sfree(ent);
		  return res; /* error */
	       }

	       if ( ((UINT32*)bdata)[0]!=msbn ||
		    ((UINT32*)bdata)[1]!=lsbn ||
		    ((UINT32*)bdata)[2]==frag_progress_n )
	       {
		  sfree(ent);
		  return mird_generate_error(MIRDE_VERIFY_FAILED,0,0,0);
	       }
	    }
	    else if (cur[0]==finishn)
	    {
	       /* we're done! */
	       sfree(ent);
	       return MIRD_OK; 
	    }
	 cur+=MIRD_JOURNAL_ENTRY_SIZE/4;
	 pos+=MIRD_JOURNAL_ENTRY_SIZE;
      }
   }
}

MIRD_RES mird_simul_tr_rewind(struct mird_transaction *mtr,
			      MIRD_OFF_T stop,MIRD_OFF_T *wpos)
{
   MIRD_RES res;
   UINT32 *ent,*cur;
   MIRD_OFF_T pos;
   UINT32 n;

   UINT32 msbn=htonl(mtr->no.msb);
   UINT32 lsbn=htonl(mtr->no.lsb);
   UINT32 allon=htonl(MIRDJ_ALLOCATED_BLOCK);

   /* loop through transaction entries 
    * free allocated blocks 
    */

   if ( (res=MIRD_MALLOC(MIRD_JOURNAL_ENTRY_SIZE*mtr->db->journal_readback_n,
			 &ent)) )
      return res;

   pos=mtr->offset;

   for (;;)
   {
      if ( (res=mird_journal_get(mtr->db,pos,
				 mtr->db->journal_readback_n,
				 (unsigned char*)ent,&n)) )
      {
	 sfree(ent);
	 return res;
      }
      
      if (!n) /* we didn't reach stop??! */
      {
	 sfree(ent); 
	 return MIRD_OK;  /* well... */
      }

      cur=ent;
      while (n--)
      {
	 if (pos==stop)
	 {
	    sfree(ent);
	    return MIRD_OK;
	 }

	 if (cur[2]==lsbn &&
	     cur[1]==msbn) /* us */
	    if (cur[0]==allon)
	    {
	       mtr->cont_offset=pos;
	       /* please free our allocation */
	       if ( (res=mird_journal_write_pos(mtr->db,wpos,
						MIRDJ_BLOCK_UNUSED,
						mtr->no,
						READ_BLOCK_LONG(cur,3),
						0,0)) )
	       {
		  sfree(ent);
		  return res;
	       }
	    }
	 cur+=MIRD_JOURNAL_ENTRY_SIZE/4;
	 pos+=MIRD_JOURNAL_ENTRY_SIZE;
      }
   }
}
