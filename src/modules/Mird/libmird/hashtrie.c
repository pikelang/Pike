/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: hashtrie.c,v 1.8 2004/03/14 16:15:53 mast Exp $
*/

/*
** libMird by Mirar <mirar@mirar.org>
** please submit bug reports and patches to the author
**
** also see http://www.mirar.org/mird/
*/

/* handles hashtrie nodes
 */

#include <stdlib.h>
#include <unistd.h>

#include "internal.h"

/* AIX requires this to be the first thing in the file.  */
#if HAVE_ALLOCA_H
# include <alloca.h>
# ifdef __GNUC__
#  ifdef alloca
#   undef alloca
#  endif
#  define alloca __builtin_alloca
# endif 
#else
# ifdef __GNUC__
#  ifdef alloca
#   undef alloca
#  endif
#  define alloca __builtin_alloca
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
void *alloca();
#   endif
#  endif
# endif
#endif

static const char RCSID[]=
   "$Id: hashtrie.c,v 1.8 2004/03/14 16:15:53 mast Exp $";

#define TOO_DEEP_RECURSION 31 /* we can only shift down that */

/* 
 * finds a cell from any hashtrie node
 */

#ifdef MASSIVE_DEBUG
#define HASHTRIE_DEBUG
#endif
#ifdef HASHTRIE_DEBUG
#include <stdio.h>
#endif

static MIRD_RES mird_ht_find_b(struct mird *db,
			       UINT32 orig_root,
			       UINT32 orig_key,
			       UINT32 chunk_id,
			       UINT32 recur_level,
			       UINT32 key_left,
			       UINT32 *cell_chunk,
			       unsigned char **bdata_p)
{
   MIRD_RES res;
   UINT32 b,n,len,v;
   unsigned char *data;
   unsigned char *bdata;
   
   if (!chunk_id) /* end of chain */
   {
      cell_chunk[0]=0;
      return MIRD_OK;
   }

   n=CHUNK_ID_2_FRAG(db,chunk_id);
   if (!n) 
      goto check_big_cell;

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"find %08lxh %*s %lxh%ld, %lxh>>\n",
	   orig_key,(int)recur_level,"",
	   CHUNK_ID_2_BLOCK(db,chunk_id),n,key_left);
#endif


   if ( (res=mird_frag_get_b(db,chunk_id,&data,&bdata,&len)) )
      return res;

   if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE)
      goto check_cell;

   if (recur_level>TOO_DEEP_RECURSION)
      return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				 orig_root,orig_key,chunk_id);

   v=key_left&((1<<db->hashtrie_bits)-1);
   key_left>>=db->hashtrie_bits;
   
   chunk_id=READ_BLOCK_LONG(data,2+v);

   return 
      mird_ht_find_b(db,orig_root,orig_key,
		     chunk_id,recur_level+db->hashtrie_bits,key_left,cell_chunk,bdata_p);

check_big_cell:
   b=CHUNK_ID_2_BLOCK(db,chunk_id);
   if ( (res=mird_block_get(db,b,&bdata)) )
      return res;

   if (READ_BLOCK_LONG(bdata,2)!=BLOCK_BIG)
      return mird_generate_error(MIRDE_WRONG_BLOCK,b,
				  BLOCK_BIG,READ_BLOCK_LONG(bdata,2));
   
   data=bdata+20;

check_cell:
   if (READ_BLOCK_LONG(data,0)!=CHUNK_CELL &&
       READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
      return mird_generate_error(MIRDE_WRONG_CHUNK,chunk_id,
				  CHUNK_CELL,READ_BLOCK_LONG(data,0));

   if (READ_BLOCK_LONG(data,1)==orig_key) /* we got the right cell */
      cell_chunk[0]=chunk_id;
   else
      cell_chunk[0]=0;

   if (bdata_p) bdata_p[0]=bdata;

   return MIRD_OK;
}

/* 
 * finds a cell in a hashtrie
 */

MIRD_RES mird_hashtrie_find_b(struct mird *db,
			      UINT32 root, /* chunk id */
			      UINT32 key,
			      UINT32 *cell_chunk,
			      unsigned char **bdata)
{
   return mird_ht_find_b(db,root,key,root,0,key,cell_chunk,bdata);
}

/* 
 * writes a key to a hashtrie
 * creates new nodes if needed
 * performs splits if needed
 * make a new root if needed (not our or root=zero)
 *
 */

INLINE MIRD_RES mird_hashtrie_new_node(struct mird_transaction *mtr,
				       UINT32 table_id,
				       UINT32 *id,
				       unsigned char **data)
{
   MIRD_RES res;
   if ( (res=mird_frag_new(mtr,table_id,
			   8+(4<<mtr->db->hashtrie_bits),
			   id,data)) )
      return res;

   /* new frag -> always zero (memset otherwise) */
   WRITE_BLOCK_LONG(*data,0,CHUNK_HASHTRIE);

   return MIRD_OK;
}

INLINE MIRD_RES mird_hashtrie_copy_node(struct mird_transaction *mtr,
					UINT32 table_id,
					UINT32 *id,
					unsigned char *old_data,
					unsigned char **data)
{
   MIRD_RES res;

   memcpy(mtr->db->buffer,old_data+4,4+(4<<mtr->db->hashtrie_bits));

   if ( (res=mird_hashtrie_new_node(mtr,table_id,id,data)) )
      return res;
   
   memcpy(data[0]+4,mtr->db->buffer,4+(4<<mtr->db->hashtrie_bits));

   return MIRD_OK;
}

static MIRD_RES mird_ht_write(struct mird_transaction *mtr,
			      UINT32 table_id,
			      UINT32 orig_root, 
			      UINT32 orig_key,
			      UINT32 cell_chunk,
			      UINT32 node_id, /* current */
			      UINT32 key_left,
			      UINT32 recur_level,
			      UINT32 *new_node,
			      UINT32 *old_cell,
			      UINT32 *old_is_mine)
{
   MIRD_RES res;
   UINT32 b,n,len,v,id,id1,i;
   unsigned char *data,*bdata;
   UINT32 filter=((1<<recur_level)-1);

   if (!node_id) /* end of search */
   {
      new_node[0]=cell_chunk;
      if (old_cell) old_cell[0]=0;
      if (old_is_mine) old_is_mine[0]=0;
      return MIRD_OK;
   }

   n=CHUNK_ID_2_FRAG(mtr->db,node_id);
   if (!n) 
      goto check_big_cell;

   if ( (res=mird_frag_get_b(mtr->db,node_id,&data,&bdata,&len)) )
      return res;

   if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE)
      goto check_cell;

   if ( (READ_BLOCK_LONG(data,1)&filter) != (orig_key&filter) )
      return mird_generate_error(MIRDE_HASHTRIE_AWAY,
				 orig_root,orig_key,node_id);

#ifdef HASHTRIE_DEBUG
   b=0;
#endif

   if (recur_level>TOO_DEEP_RECURSION)
      return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				 orig_root,orig_key,node_id);

   
   if (READ_BLOCK_LONG(bdata,1)!=mtr->no.lsb ||
       READ_BLOCK_LONG(bdata,0)!=mtr->no.msb)
   {
      i=1<<mtr->db->hashtrie_bits;
      while (i) if (((UINT32*)data)[1+i]) break; else i--;
      if (!i) /* that node is empty, we won't copy that */
      {
	 if (old_is_mine) old_is_mine[0]=0;
	 if (old_cell) old_cell[0]=0;
	 new_node[0]=cell_chunk;
	 if ( (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,node_id))) ) 
	    return res; /* free old */
	 return MIRD_OK;
      }

      if ( (res=mird_hashtrie_copy_node(mtr,table_id,new_node,data,&data)) )
	 return res;
      if ( (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,node_id))) ) 
	 return res; /* free old */

#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"writ %08lx %*snew node    %lxh%ld copied from %lxh%ld\n",
	      orig_key,(int)recur_level,"",
	      CHUNK_ID_2_BLOCK(mtr->db,*new_node),
	      CHUNK_ID_2_FRAG(mtr->db,*new_node),
	      CHUNK_ID_2_BLOCK(mtr->db,node_id),
	      CHUNK_ID_2_FRAG(mtr->db,node_id));
      b=1;
#endif
   }
   else
   {
      new_node[0]=node_id;
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"writ %08lx %*skeep node   %lxh%ld\n",
	      orig_key,(int)recur_level,"",
	      CHUNK_ID_2_BLOCK(mtr->db,*new_node),
	      CHUNK_ID_2_FRAG(mtr->db,*new_node));
#endif
   }

   if (key_left!=(orig_key>>recur_level))
      mird_fatal("er, ops\n");

   v=key_left&((1<<mtr->db->hashtrie_bits)-1);
   key_left>>=mtr->db->hashtrie_bits;
   


   id=READ_BLOCK_LONG(data,2+v);

   if ( (res=mird_ht_write(mtr,table_id,orig_root,orig_key,cell_chunk,
			   id,key_left,
			   recur_level+mtr->db->hashtrie_bits,&id1,
			   old_cell,old_is_mine)) )
      return res;

   if (id!=id1) /* write new node */
   {
      if ( (res=mird_frag_get_w(mtr,new_node[0],&data,&len)) )
	 return res;
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"writ %08lx %*supdate node %lxh%ld branch %lxh [%lxh>>] to %lxh%ld\n",
	      orig_key,(int)recur_level,"",
	      CHUNK_ID_2_BLOCK(mtr->db,new_node[0]),
	      CHUNK_ID_2_FRAG(mtr->db,new_node[0]),v,orig_key>>recur_level,
	      CHUNK_ID_2_BLOCK(mtr->db,id1),
	      CHUNK_ID_2_FRAG(mtr->db,id1));
#endif
      WRITE_BLOCK_LONG(data,2+v,id1); /* new branch */

#if 0
      if (!id1) /* maybe we can delete this node */
      {
	 for (v=1<<mtr->db->hashtrie_bits; v;)
	 {
	    /* 0 is always 0, skip READ_BLOCK_LONG */
	    if (((unsigned long*)data)[1+v]) break;
	    v--;
	 }
	 if (!v) /* all zero */
	    new_node[0]=0;
      }
#endif
   }
#ifdef HASHTRIE_DEBUG
   else if (b)
   {
      fprintf(stderr,"writ %08lx %*snew node    %lxh%ld (above) but no new branch (?) id=%lxh id1=%lxh v=%lxh\n",
	      orig_key,(int)recur_level,"",
	      CHUNK_ID_2_BLOCK(mtr->db,new_node[0]),
	      CHUNK_ID_2_FRAG(mtr->db,new_node[0]),
	      id,id1,v);
	      
   }
#endif

   return MIRD_OK;

check_big_cell:
   b=CHUNK_ID_2_BLOCK(mtr->db,node_id);
   if ( (res=mird_block_get(mtr->db,b,&bdata)) )
      return res;

   if (READ_BLOCK_LONG(bdata,2)!=BLOCK_BIG)
      return mird_generate_error(MIRDE_WRONG_BLOCK,b,
				  BLOCK_BIG,READ_BLOCK_LONG(bdata,2));
   
   data=bdata+20;

check_cell:
   if (READ_BLOCK_LONG(data,0)!=CHUNK_CELL &&
       READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
      return mird_generate_error(MIRDE_WRONG_CHUNK,node_id,
				  CHUNK_CELL,READ_BLOCK_LONG(data,0));

   /* if our cell -> just return new_node as cell_chunk */
   /* if another cell -> */
   /*   if cell_chunk is zero -> return new_node as this */
   /*   otherwise split */

   if (READ_BLOCK_LONG(data,1)==orig_key) 
   {
      if (old_is_mine)
	 if (READ_BLOCK_LONG(bdata,1)==mtr->no.lsb &&
	     READ_BLOCK_LONG(bdata,0)==mtr->no.msb)
	    old_is_mine[0]=1;
	 else
	    old_is_mine[0]=0;
      
      if (old_cell) old_cell[0]=node_id;

      new_node[0]=cell_chunk;
      if ( (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,node_id))) ) 
	 return res; /* free old */
      return MIRD_OK;
   }
   else if (!cell_chunk)
   {
      /* we didn't exist anyway */
      if (old_cell) old_cell[0]=0;
      if (old_is_mine) old_is_mine[0]=0;
      new_node[0]=node_id;
      return MIRD_OK;
   }
   else 
   {
      if (recur_level>TOO_DEEP_RECURSION)
	 return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				    orig_root,orig_key,node_id);

      v=(READ_BLOCK_LONG(data,1)>>recur_level)&
	 ((1<<mtr->db->hashtrie_bits)-1);

      /* split */
      if ( (res=mird_hashtrie_new_node(mtr,table_id,&id,&data)) )
	 return res;

#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"writ %08lx %*snew node    %lxh%ld old branch %lxh to %lxh%ld\n",
	      orig_key,(int)recur_level,"",
	      CHUNK_ID_2_BLOCK(mtr->db,id),
	      CHUNK_ID_2_FRAG(mtr->db,id),v,
	      CHUNK_ID_2_BLOCK(mtr->db,node_id),
	      CHUNK_ID_2_FRAG(mtr->db,node_id));
#endif

      WRITE_BLOCK_LONG(data,1,orig_key);

      WRITE_BLOCK_LONG(data,2+v,node_id); /* old node */

      /* repeat (by tail) */
      return mird_ht_write(mtr,table_id,orig_root,orig_key,cell_chunk,
			   id,key_left,recur_level,new_node,
			   old_cell,old_is_mine);
   }
}

MIRD_RES mird_hashtrie_write(struct mird_transaction *mtr,
			     UINT32 table_id,
			     UINT32 root, /* chunk id */
			     UINT32 key,
			     UINT32 cell_chunk,
			     UINT32 *new_root,
			     UINT32 *old_cell, /* chunk id */
			     UINT32 *old_is_mine) 
{
   return
      mird_ht_write(mtr,table_id,root,key,cell_chunk,
		    root,key,0,new_root,old_cell,old_is_mine);
}

/*
 * used to loop over a hashtrie
 */

MIRD_RES mird_ht_scan(struct mird *db, UINT32 orig_root,UINT32 orig_key,
		      UINT32 node_id,
		      struct transaction_id *restrict_,
		      UINT32 recur_level, UINT32 key_left,
		      UINT32 read_n,
		      UINT32 *dest_key, UINT32 *dest_cell, UINT32 *dest_n,
		      int first) /* take first instead of next */
{
   MIRD_RES res;
   UINT32 b,n,len,v;
   UINT32 id;
   unsigned char *data;
   unsigned char *bdata;
   unsigned char *buf;
   
   if (!node_id) /* end of chain */
      return MIRD_OK;

   if (recur_level>TOO_DEEP_RECURSION)
      return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				 orig_root,orig_key,0);
   
   n=CHUNK_ID_2_FRAG(db,node_id);
   if (!n) 
      goto check_big_cell;

   if ( (res=mird_frag_get_b(db,node_id,&data,&bdata,&len)) )
      return res;

   /* check if we're on the right track */
   if (restrict_ &&
       (READ_BLOCK_LONG(bdata,1)!=restrict_->lsb ||
	READ_BLOCK_LONG(bdata,0)!=restrict_->msb))
      return MIRD_OK;

   if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE)
      goto check_cell;

   v=key_left&((1<<db->hashtrie_bits)-1);
   key_left>>=db->hashtrie_bits;

   buf=alloca( 4<<db->hashtrie_bits );
   memcpy(buf,data+8,4<<db->hashtrie_bits);

   for (;;)
   {
      id=READ_BLOCK_LONG(buf,v);

      if (id)
      {
#ifdef HASHTRIE_DEBUG
	 fprintf(stderr,"--- %*s%lxh in %lxh%ld (to %lxh%ld)\n",
		 (int)recur_level,"",v,
		 CHUNK_ID_2_BLOCK(db,node_id),
		 CHUNK_ID_2_FRAG(db,node_id),
		 CHUNK_ID_2_BLOCK(db,id),
		 CHUNK_ID_2_FRAG(db,id));
#endif

	 if ( (res = mird_ht_scan(db,orig_root,orig_key,id,restrict_,
				  recur_level+db->hashtrie_bits,key_left,
				  read_n,dest_key,dest_cell,dest_n,first)) )
	    return res;

	 if (dest_n[0]==read_n) 
	    return MIRD_OK; /* all done */
      }

      v++;
      key_left=0;
      if (v==(UINT32)(1<<db->hashtrie_bits))
	 return MIRD_OK; /* branch complete */
   }

check_big_cell:
   b=CHUNK_ID_2_BLOCK(db,node_id);
   if ( (res=mird_block_get(db,b,&bdata)) )
      return res;

   if (READ_BLOCK_LONG(bdata,2)!=BLOCK_BIG)
      return mird_generate_error(MIRDE_WRONG_BLOCK,b,
				  BLOCK_BIG,READ_BLOCK_LONG(bdata,2));

   /* check if we're on the right track */
   if (restrict_ &&
       (READ_BLOCK_LONG(bdata,1)!=restrict_->lsb ||
	READ_BLOCK_LONG(bdata,0)!=restrict_->msb))
      return MIRD_OK;
   
   data=bdata+20;

check_cell:
   if (READ_BLOCK_LONG(data,0)!=CHUNK_CELL &&
       READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
      return mird_generate_error(MIRDE_WRONG_CHUNK,node_id,
				  CHUNK_CELL,READ_BLOCK_LONG(data,0));

   if (dest_key) dest_key[dest_n[0]]=READ_BLOCK_LONG(data,1);
   if (dest_cell) dest_cell[dest_n[0]]=node_id;

   if (first || dest_key[0]!=orig_key) /* yeah, keep this*/
      dest_n[0]++;

   return MIRD_OK;
}

MIRD_RES mird_hashtrie_next(struct mird *db,
			    UINT32 root,
			    UINT32 key,
			    UINT32 n,
			    UINT32 *dest_key,
			    UINT32 *dest_cell,
			    UINT32 *dest_n)
{
   dest_n[0]=0;
   return mird_ht_scan(db,root,key,root,NULL,
		       0,key,n,dest_key,dest_cell,dest_n,0);
}

MIRD_RES mird_hashtrie_first(struct mird *db,
			     UINT32 root,
			     UINT32 n,
			     UINT32 *dest_key,
			     UINT32 *dest_cell,
			     UINT32 *dest_n)
{
   dest_n[0]=0;
   return mird_ht_scan(db,root,0,root,NULL,
		       0,0,n,dest_key,dest_cell,dest_n,1);
}

MIRD_RES mird_tr_hashtrie_next(struct mird_transaction *mtr,
			       UINT32 root,
			       UINT32 key,
			       UINT32 n,
			       UINT32 *dest_key,
			       UINT32 *dest_cell,
			       UINT32 *dest_n)
{
   dest_n[0]=0;
   return mird_ht_scan(mtr->db,root,key,root,&(mtr->no),
		       0,key,n,dest_key,dest_cell,dest_n,0);
}

MIRD_RES mird_tr_hashtrie_first(struct mird_transaction *mtr,
				UINT32 root,
				UINT32 n,
				UINT32 *dest_key,
				UINT32 *dest_cell,
				UINT32 *dest_n)
{
   dest_n[0]=0;
   return mird_ht_scan(mtr->db,root,0,root,&(mtr->no),
		       0,0,n,dest_key,dest_cell,dest_n,1);
}

/*
 * used to find an unused key in a hashtrie
 */

static MIRD_RES mird_ht_find_no_key(struct mird *db, 
				    UINT32 orig_root,
				    UINT32 orig_key,
				    UINT32 node_id,
				    UINT32 recur_level, 
				    UINT32 key_left,
				    UINT32 *dest_key)
{
   MIRD_RES res;
   UINT32 b,n,len,v,i,m;
   UINT32 id;
   UINT32 min;
   unsigned char *data;
   unsigned char *bdata;
   unsigned char *buf;
   
   if (!node_id) /* end of chain */
   {
      dest_key[0]=orig_key;
      return MIRD_OK;
   }

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"-n- %*s%lxh%ld find start=%lxh left=%lxh>> (%lxh)\n",
	   (int)recur_level,"",
	   CHUNK_ID_2_BLOCK(db,node_id),
	   CHUNK_ID_2_FRAG(db,node_id),
	   orig_key,key_left,
	   key_left&((1<<db->hashtrie_bits)-1));
#endif

   if (recur_level>TOO_DEEP_RECURSION)
      return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				 orig_root,orig_key,0);
   
   n=CHUNK_ID_2_FRAG(db,node_id);
   if (!n) 
      goto check_big_cell;

   if ( (res=mird_frag_get_b(db,node_id,&data,&bdata,&len)) )
      return res;

   if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE)
      goto check_cell;

   v=key_left&((1<<db->hashtrie_bits)-1);
   key_left>>=db->hashtrie_bits;

   buf=alloca( 4<<db->hashtrie_bits );
   memcpy(buf,data+8,4<<db->hashtrie_bits);

   m=(1<<db->hashtrie_bits);
   for (i=v; i<m; i++)
   {
      if (!READ_BLOCK_LONG(buf,i))
      {
	 dest_key[0]=orig_key&(((1<<recur_level)-1)|(i<<recur_level));
#ifdef HASHTRIE_DEBUG
	 fprintf(stderr,"-n- %*s%lxh%ld %lxh is unused -> %lxh\n",
		 (int)recur_level,"",
		 CHUNK_ID_2_BLOCK(db,node_id),
		 CHUNK_ID_2_FRAG(db,node_id),
		 i,dest_key[0]);
#endif
	 return MIRD_OK;
      }
   }

   min=0xffffffff;
   for (;;)
   {
      id=READ_BLOCK_LONG(buf,v);

      if (id)
      {
#ifdef HASHTRIE_DEBUG
	 fprintf(stderr,"-n- %*s%lxh in %lxh%ld (to %lxh%ld)\n",
		 (int)recur_level,"",v,
		 CHUNK_ID_2_BLOCK(db,node_id),
		 CHUNK_ID_2_FRAG(db,node_id),
		 CHUNK_ID_2_BLOCK(db,id),
		 CHUNK_ID_2_FRAG(db,id));
#endif

	 if ( (res = mird_ht_find_no_key(db,orig_root,
					 orig_key|(v<<recur_level),
					 id,
					 recur_level+db->hashtrie_bits,
					 key_left,
					 dest_key)) )
	    return res;

#ifdef HASHTRIE_DEBUG
	 fprintf(stderr,"-n- %*s%lxh gives %lx\n",
		 (int)recur_level,"",v,dest_key[0]);
#endif

	 if (dest_key[0]<min) min=dest_key[0];
      }

      v++;
      key_left=0;
      orig_key&=((1<<recur_level)-1);
      if (v==m)
      {
	 dest_key[0]=min;
	 return MIRD_OK; /* branch complete */
      }
   }

check_big_cell:
   b=CHUNK_ID_2_BLOCK(db,node_id);
   if ( (res=mird_block_get(db,b,&bdata)) )
      return res;

   if (READ_BLOCK_LONG(bdata,2)!=BLOCK_BIG)
      return mird_generate_error(MIRDE_WRONG_BLOCK,b,
				 BLOCK_BIG,READ_BLOCK_LONG(bdata,2));

   data=bdata+20;

check_cell:
   if (READ_BLOCK_LONG(data,0)!=CHUNK_CELL &&
       READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
      return mird_generate_error(MIRDE_WRONG_CHUNK,node_id,
				 CHUNK_CELL,READ_BLOCK_LONG(data,0));

   min=READ_BLOCK_LONG(data,1);
   if (orig_key==min) 
   {
      dest_key[0]=orig_key&(((1<<recur_level)-1)|((key_left+1)<<recur_level));
      if (dest_key[0]==0) dest_key[0]=0xffffffff;
   }
   else
      dest_key[0]=orig_key;

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"-n- %*s%lxh%ld is cell (%lxh!=%lxh?) -> %lxh\n",
	   (int)recur_level,"",
	   CHUNK_ID_2_BLOCK(db,node_id),
	   CHUNK_ID_2_FRAG(db,node_id),
	   min,orig_key,dest_key[0]);
#endif


   return MIRD_OK;
}

MIRD_RES mird_hashtrie_find_no_key(struct mird *db, 
				   UINT32 root,
				   UINT32 minimum_key,
				   UINT32 *dest_key)
{
   return mird_ht_find_no_key(db,root,minimum_key,root,0,minimum_key,dest_key);
}

/*
 * resolving 
 */

static MIRD_RES mird_ht_resolve(struct mird_transaction *mtr,
				UINT32 table_id,
				UINT32 root_m,
				UINT32 root_o,
				UINT32 node_m, /* must be valid */
				UINT32 node_o, /* might be zero */
				UINT32 recur_level,
				UINT32 *new_node_m)
{
   /* check this node;
    *
    * if node_o is a cell
    *   if node_m is a cell
    *     if node_m and node_o keys are the same
    *       if node_m is mine, return new_node = node_m (it's mine)
    *       else return new_node = node_o (it's newer)
    *     else (node_m and node_o keys are not equal)
    *       split node_m, recurse
    *   else (node_m is a hashtrie)
    *     resolve node_o in node_m
    *
    * if node_m is a cell (node_o isn't)
    *   split node_m,recurse
    *
    * if node_m isn't our's,
    *   it has been updated;
    *   return new_node_m=node_o
    
    * loop over branches
    *   if the branch differs in node_m and node_o, 
    *     recurse
    */

   UINT32 b_m,b_o,n_m,n_o;
   unsigned char *data,*bdata;
   int need_save;
   int node_o_cell=0,node_m_cell=0;
   UINT32 len,v,key_m,id_m,id_o,key_o;
   unsigned char *bufm=NULL;
   MIRD_RES res;
   UINT32 filter=((1<<recur_level)-1);

   b_m=CHUNK_ID_2_BLOCK(mtr->db,node_m);
   n_m=CHUNK_ID_2_FRAG(mtr->db,node_m);
   b_o=CHUNK_ID_2_BLOCK(mtr->db,node_o);
   n_o=CHUNK_ID_2_FRAG(mtr->db,node_o);

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"%*sm:%lxh%ld o:%lxh%ld ",
	   (int)recur_level,"",b_m,n_m,b_o,n_o);
#endif

   if (!node_m)
   {
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"we don't have anything; they win\n");
#endif
      new_node_m[0]=node_o;
      return MIRD_OK;
   }

   if (!n_m)
   {
      if ( (res=mird_block_get(mtr->db,b_m,&bdata)) )
	 return res;
      data=bdata+20;
      node_m_cell=1;
   }
   else
   {
      if ( (res=mird_frag_get_b(mtr->db,node_m,&data,&bdata,&len)) )
	 return res;
   }
   key_m=READ_BLOCK_LONG(data,1);

   if ( ! (READ_BLOCK_LONG(bdata,1)==mtr->no.lsb &&
	   READ_BLOCK_LONG(bdata,0)==mtr->no.msb ) )
   {
      /* we're on the old track */
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"mine isn't mine, give away\n");
#endif

      new_node_m[0]=node_o;
      return MIRD_OK;
   }
   else if (!node_o) 
   {
      /* it's a new track, and we win */
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"mine is mine and old doesn't exist\n");
#endif
      new_node_m[0]=node_m;
      return MIRD_OK;
   }

   if (!n_o) 
      node_o_cell=1;

   if (!n_m ||
       READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE) 
      node_m_cell=1;

   if (!(node_m_cell))
   {
      bufm=alloca( 4<<mtr->db->hashtrie_bits );
   
      memcpy(bufm,data+8,4<<mtr->db->hashtrie_bits);
   }

   if (n_o)
   {
      if ( (res=mird_frag_get_b(mtr->db,node_o,&data,&bdata,&len)) )
	 return res;

      if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE) 
	 node_o_cell=1;
   }
   else
   {
      /* we need that data anyway */
      /* and we know b_o is zero here */
      if ( (res=mird_block_get(mtr->db,b_o,&bdata)) )
	 return res;
      if (READ_BLOCK_LONG(bdata,2)!=BLOCK_BIG)
	 return mird_generate_error(MIRDE_WRONG_BLOCK,b_o,
				    BLOCK_BIG,READ_BLOCK_LONG(bdata,2));
      data=bdata+20;
   }
   key_o=READ_BLOCK_LONG(data,1);

   if ( (key_m&filter)!=(key_o&filter) )
   {
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"we're away from the track! m:%lxh o:%lxh (full is %lxh/%lxh)\n",
	      key_m&filter,key_o&filter,key_m,key_o);
#endif
      return mird_generate_error(MIRDE_HASHTRIE_AWAY,
				 key_m,key_o,node_m);
   }

   if (node_m_cell) 
   {
      if (node_o_cell)
      {
	 key_o=READ_BLOCK_LONG(data,1);
	 if (key_m==key_o)
	 {
	    /* it is mine, I win */
	    new_node_m[0]=node_m;
#ifdef HASHTRIE_DEBUG
	    fprintf(stderr,"both cells; it is mine, I win\n");
#endif
	    return MIRD_OK;
	 }
#ifdef HASHTRIE_DEBUG
	 else fprintf(stderr,"both cells; keys differ (o:%08lxh m:%08lxh)...",
		      key_o,key_m);
#endif
      }
#ifdef HASHTRIE_DEBUG
      else fprintf(stderr,"other is node ");
#endif
      goto split_node_m;
   }

   /* ok, compare hashtrie nodes */

   need_save=0;

   if (node_o_cell)
   {
      if (recur_level>TOO_DEEP_RECURSION)
	 return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				    root_o,0,node_o);

      v=(READ_BLOCK_LONG(data,1)>>recur_level)&((1<<mtr->db->hashtrie_bits)-1);
      id_m=READ_BLOCK_LONG(bufm,v);

#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"o is cell; resolve only branch %lx "
	      "(is m:%lxh%ld o:%lxh%ld) [%lxh>>/%lxh]\n",v,
	      CHUNK_ID_2_BLOCK(mtr->db,id_m),
	      CHUNK_ID_2_FRAG(mtr->db,id_m),
	      CHUNK_ID_2_BLOCK(mtr->db,node_o),
	      CHUNK_ID_2_FRAG(mtr->db,node_o),
	      (READ_BLOCK_LONG(data,1)>>recur_level),
	      READ_BLOCK_LONG(data,1));
#endif

      if (node_o!=id_m)
      {
	 if (!id_m)
	 {
	    WRITE_BLOCK_LONG(bufm,v,node_o);
	    need_save=1;
	 }
	 else
	 {
	    if ( (res = mird_ht_resolve(mtr,table_id,root_m,root_o,
					id_m,node_o,
					recur_level+mtr->db->hashtrie_bits,
					new_node_m)) )
	       return res;
	 
	    if (new_node_m[0]!=id_m) /* we have to update this node */
	    {
	       WRITE_BLOCK_LONG(bufm,v,new_node_m[0]);
	       need_save=1;
	    }
	 }
      }
   }
   else 
   {
      unsigned char *bufo;
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"resolve loop\n");
#endif
      bufo=alloca( 4<<mtr->db->hashtrie_bits );
      memcpy(bufo,data+8,4<<mtr->db->hashtrie_bits);

#ifdef HASHTRIE_DEBUG
      {
	 UINT32 i;
	 fprintf(stderr,"%*so [",(int)recur_level,"");
	 for (i=0; i<(1<<mtr->db->hashtrie_bits); i++)
	    if (READ_BLOCK_LONG(bufo,i))
	       fprintf(stderr,"%ld:%lxh%ld ",
		       i,
		       CHUNK_ID_2_BLOCK(mtr->db,READ_BLOCK_LONG(bufo,i)),
		       CHUNK_ID_2_FRAG(mtr->db,READ_BLOCK_LONG(bufo,i)));
	 fprintf(stderr,"]\n%*sn [",(int)recur_level,"");
	 for (i=0; i<(1<<mtr->db->hashtrie_bits); i++)
	    if (READ_BLOCK_LONG(bufm,i))
	       fprintf(stderr,"%ld:%lxh%ld ",
		       i,
		       CHUNK_ID_2_BLOCK(mtr->db,READ_BLOCK_LONG(bufm,i)),
		       CHUNK_ID_2_FRAG(mtr->db,READ_BLOCK_LONG(bufm,i)));
	 fprintf(stderr,"]\n");
      }
#endif
      for (v=(1<<mtr->db->hashtrie_bits);v--;)
      {
	 id_m=READ_BLOCK_LONG(bufm,v);
	 id_o=READ_BLOCK_LONG(bufo,v);

	 /* if they differ, we need to resolve */
	 if (id_m!=id_o)
	 {
	    if (!id_m)
	    {
	       /* doesn't exist in this branch */
	       WRITE_BLOCK_LONG(bufm,v,id_o);
	       need_save=1;
	    }
	    else
	    {
	       if ( (res = mird_ht_resolve(mtr,table_id,root_m,root_o,
					   id_m,id_o,
					   recur_level+mtr->db->hashtrie_bits,
					   new_node_m)) )
		  return res;
	 
	       if (new_node_m[0]!=id_m) /* we have to update this node */
	       {
		  WRITE_BLOCK_LONG(bufm,v,new_node_m[0]);
		  need_save=1;
	       }
	    }
	 }
      }
#ifdef HASHTRIE_DEBUG
      {
	 UINT32 i;
	 fprintf(stderr,"%*s= [",(int)recur_level,"");
	 for (i=0; i<(1<<mtr->db->hashtrie_bits); i++)
	    if (READ_BLOCK_LONG(bufm,i))
	       fprintf(stderr,"%ld:%lxh%ld ",
		       i,
		       CHUNK_ID_2_BLOCK(mtr->db,READ_BLOCK_LONG(bufm,i)),
		       CHUNK_ID_2_FRAG(mtr->db,READ_BLOCK_LONG(bufm,i)));
	 fprintf(stderr,"]\n");
      }
#endif
   }
   if (need_save)
   {
      if ( (res=mird_frag_get_w(mtr,node_m,&data,&len)) )
	 return res;
      memcpy(data+8,bufm,4<<mtr->db->hashtrie_bits);
      /* done, will be synced later */
   }

   /* all resolved, and keep this node */
   new_node_m[0]=node_m;
   return MIRD_OK;

split_node_m:
   if (!node_o_cell) 
   {
      /* free other */
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"(free other) ");
#endif
      if ( (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,node_o))) )
	 return res;  /* free old */
   }

   v=(key_m>>recur_level)&((1<<mtr->db->hashtrie_bits)-1);

   if ( (res=mird_hashtrie_new_node(mtr,table_id,&id_m,&data)) )
      return res;

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"split mine; new node is %lxh%ld, key is %lxh, branch %lxh%ld at %lxh\n",
	   CHUNK_ID_2_BLOCK(mtr->db,id_m),
	   CHUNK_ID_2_FRAG(mtr->db,id_m),
	   key_m,
	   CHUNK_ID_2_BLOCK(mtr->db,node_m),
	   CHUNK_ID_2_FRAG(mtr->db,node_m),
	   v);
#endif

   WRITE_BLOCK_LONG(data,1,key_m);

   WRITE_BLOCK_LONG(data,2+v,node_m); /* old node */

   return mird_ht_resolve(mtr,table_id,root_m,root_o,
			  id_m,node_o,
			  recur_level,
			  new_node_m);
}

MIRD_RES mird_hashtrie_resolve(struct mird_transaction *mtr,
			       UINT32 table_id,
			       UINT32 root_m,
			       UINT32 root_o,
			       UINT32* new_root_m)
{
   return mird_ht_resolve(mtr,table_id,root_m,root_o,
			  root_m,root_o,0,new_root_m);
}

/*
 * used to delete a whole hashtrie (and leaves)
 */

static MIRD_RES mird_ht_free(struct mird_transaction *mtr,
			     UINT32 orig_root,
			     UINT32 node_id,
			     UINT32 recur_level,
			     UINT32 last_unused)
{
   MIRD_RES res;
   UINT32 b,n,len,v;
   UINT32 id;
   unsigned char *data;
   unsigned char *buf;

   if (!node_id) /* end of chain */
      return MIRD_OK;

   if (recur_level>TOO_DEEP_RECURSION)
      return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				 orig_root,0,0);
   
   b=CHUNK_ID_2_BLOCK(mtr->db,node_id);
   n=CHUNK_ID_2_FRAG(mtr->db,node_id);
   if (!n) 
      return MIRD_OK; /* freed by parent */

   if ( (res=mird_frag_get(mtr->db,node_id,&data,&len)) )
      return res;

   if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE)
      return MIRD_OK; /* freed by parent */

   buf=alloca( 4<<mtr->db->hashtrie_bits );
   
   memcpy(buf,data+8,4<<mtr->db->hashtrie_bits);

   for (v=1<<mtr->db->hashtrie_bits;v--;)
   {
      id=READ_BLOCK_LONG(buf,v);

      if (id)
      {
#ifdef HASHTRIE_DEBUG
	 fprintf(stderr,"%*s delete %lxh%ld in %lxh%ld\n",(int)recur_level,"",
		 CHUNK_ID_2_BLOCK(mtr->db,id),
		 CHUNK_ID_2_FRAG(mtr->db,id),
		 CHUNK_ID_2_BLOCK(mtr->db,node_id),
		 CHUNK_ID_2_FRAG(mtr->db,node_id));
#endif

	 if (!CHUNK_ID_2_FRAG(mtr->db,id)) 
	 {
	    if ((res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,id))))
	       return res;
	 }
	 else
	 {
	    if ( (res=mird_ht_free(mtr,orig_root,id,
				   recur_level+mtr->db->hashtrie_bits,last_unused)) )
	       return res;

	    b=CHUNK_ID_2_BLOCK(mtr->db,id);
	    if (b!=last_unused)
	       last_unused=b;
	    if ((res=mird_tr_unused(mtr,b)))
	       return res;
	 }
      }
   }
   return MIRD_OK;
}

MIRD_RES mird_hashtrie_free_all(struct mird_transaction *mtr,
				UINT32 root)
{
   UINT32 b;
   MIRD_RES res;
   b=CHUNK_ID_2_BLOCK(mtr->db,root);
   if (b)
   {
#ifdef HASHTRIE_DEBUG
      fprintf(stderr,"delete tree %lxh%ld\n",
	      CHUNK_ID_2_BLOCK(mtr->db,root),
	      CHUNK_ID_2_FRAG(mtr->db,root));
#endif
      if ((res=mird_tr_unused(mtr,b)))
	 return res;
      return mird_ht_free(mtr,root,root,0,b);
   }
   return MIRD_OK; /* already free */
}

/*
 * delete a cell if it isn't owned by us already 
 */

static MIRD_RES mird_ht_redelete(struct mird_transaction *mtr,
				 UINT32 table_id,
				 UINT32 orig_root, 
				 UINT32 orig_key,
				 UINT32 node_id, /* current */
				 UINT32 key_left,
				 UINT32 recur_level,
				 UINT32 *new_node)
{
   MIRD_RES res;
   UINT32 b,n,len,v,id,id1;
   unsigned char *data,*bdata;
   UINT32 filter=((1<<recur_level)-1);

   if (!node_id) /* end of search */
   {
      new_node[0]=0;
      return MIRD_OK; /* already deleted */
   }

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"del %08lxh %*s %lxh%ld\n",orig_key,(int)recur_level,"",
	   CHUNK_ID_2_BLOCK(mtr->db,node_id),
	   CHUNK_ID_2_FRAG(mtr->db,node_id));
#endif
	   

   n=CHUNK_ID_2_FRAG(mtr->db,node_id);
   if (!n) 
      goto check_big_cell;

   if ( (res=mird_frag_get_b(mtr->db,node_id,&data,&bdata,&len)) )
      return res;

   if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE)
      goto check_cell;

   if (recur_level>TOO_DEEP_RECURSION)
      return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				 orig_root,orig_key,node_id);

   if ( (READ_BLOCK_LONG(data,1)&filter) != (orig_key&filter) )
      return mird_generate_error(MIRDE_HASHTRIE_AWAY,
				 orig_root,orig_key,node_id);
   
   if (READ_BLOCK_LONG(bdata,1)!=mtr->no.lsb ||
       READ_BLOCK_LONG(bdata,0)!=mtr->no.msb)
   {
      if ( (res=mird_hashtrie_copy_node(mtr,table_id,new_node,data,&data)) )
	 return res;
      if ( (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,node_id))) ) 
	 return res; /* free old */
   }
   else new_node[0]=node_id;

   v=key_left&((1<<mtr->db->hashtrie_bits)-1);
   key_left>>=mtr->db->hashtrie_bits;
   
   id=READ_BLOCK_LONG(data,2+v);

   if (id)
   {
      if ( (res=mird_ht_redelete(mtr,table_id,orig_root,orig_key,id,key_left,
				 recur_level+mtr->db->hashtrie_bits,&id1)) )
	 return res;
   }
   else
      id1=0;

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"del %08lxh %*s => %lxh%ld (was %lxh%ld)\n",
	   orig_key,(int)(recur_level-1),"",
	   CHUNK_ID_2_BLOCK(mtr->db,id1),
	   CHUNK_ID_2_FRAG(mtr->db,id1),
	   CHUNK_ID_2_BLOCK(mtr->db,id),
	   CHUNK_ID_2_FRAG(mtr->db,id));
#endif

   if (id!=id1) /* write new node */
   {
      if ( (res=mird_frag_get_w(mtr,new_node[0],&data,&len)) )
	 return res;

#ifdef HASHTRIE_DEBUG
      {
	 UINT32 i;
	 fprintf(stderr,"del %08lxh %*s[",orig_key,(int)recur_level,"");
	 for (i=0; i<(1<<mtr->db->hashtrie_bits); i++)
	    if (READ_BLOCK_LONG(data,2+i)) 
	       fprintf(stderr,"%ld:%lxh%ld ",
		       i,
		       CHUNK_ID_2_BLOCK(mtr->db,READ_BLOCK_LONG(data,2+i)),
		       CHUNK_ID_2_FRAG(mtr->db,READ_BLOCK_LONG(data,2+i)));
	 fprintf(stderr,"]\n");
      }
#endif

      WRITE_BLOCK_LONG(data,2+v,id1); /* new branch */
   }

#if 1
   if (!id1) /* maybe we can delete this node */
   {
      for (v=1<<mtr->db->hashtrie_bits; v;)
      {
	 /* 0 is always 0, skip READ_BLOCK_LONG */
	 if (((unsigned long*)data)[1+v]) break;
	 v--;
      }
      if (!v) /* all zero */
      {
	 new_node[0]=0;
	 if ( (res=mird_tr_unused(mtr,CHUNK_ID_2_BLOCK(mtr->db,node_id))) ) 
	    return res; /* free old */
      }
   }
#endif

   return MIRD_OK;

check_big_cell:
   b=CHUNK_ID_2_BLOCK(mtr->db,node_id);
   if ( (res=mird_block_get(mtr->db,b,&bdata)) )
      return res;

   if (READ_BLOCK_LONG(bdata,2)!=BLOCK_BIG)
      return mird_generate_error(MIRDE_WRONG_BLOCK,b,
				  BLOCK_BIG,READ_BLOCK_LONG(bdata,2));
   
   data=bdata+20;

check_cell:
   if (READ_BLOCK_LONG(data,0)!=CHUNK_CELL &&
       READ_BLOCK_LONG(data,0)!=CHUNK_ROOT)
      return mird_generate_error(MIRDE_WRONG_CHUNK,node_id,
				  CHUNK_CELL,READ_BLOCK_LONG(data,0));

   /* if our cell -> just return new_node as cell_chunk */
   /* if another cell -> */
   /*   if cell_chunk is zero -> return new_node as this */
   /*   otherwise split */

   if (READ_BLOCK_LONG(data,1)==orig_key) 
   {
      if (READ_BLOCK_LONG(bdata,1)==mtr->no.lsb &&
	  READ_BLOCK_LONG(bdata,0)==mtr->no.msb)
	 return MIRD_OK; /* alread ours */
      
      new_node[0]=0;

      /* don't free old; we've already done that once */

      return MIRD_OK;
   }
   else
   {
      /* we didn't exist anyway */
      new_node[0]=0;
      return MIRD_OK;
   }
}

MIRD_RES mird_hashtrie_redelete(struct mird_transaction *mtr,
				UINT32 table_id,
				UINT32 root, /* chunk id */
				UINT32 key,
				UINT32 *new_root)
{
   return
      mird_ht_redelete(mtr,table_id,root,key,root,key,0,new_root);
}


/*
 * mark usage for a key 
 */

static MIRD_RES mird_ht_mark_use(struct mird *db,
				 UINT32 orig_root,
				 UINT32 orig_key,
				 UINT32 chunk_id,
				 UINT32 recur_level,
				 UINT32 key_left,
				 struct mird_status_list *mul)
{
   MIRD_RES res;
   UINT32 b,n,len,v;
   unsigned char *data;
   unsigned char *bdata;
   UINT32 filter=((1<<recur_level)-1);
   
   if (!chunk_id) /* end of chain */
      return MIRD_OK;

   b=CHUNK_ID_2_BLOCK(db,chunk_id);
   n=CHUNK_ID_2_FRAG(db,chunk_id);

#ifdef HASHTRIE_DEBUG
   fprintf(stderr,"  %*smark %lxh%ld key %lxh\n",
	   (int)recur_level,"",b,n,orig_key);
#endif
   
   if ( (res=mird_mark_used(mul,b)) )
      return res;

   if (!n) 
   {
      for (;;)
      {
	 if ( (res=mird_block_get(db,b,&bdata)) )
	    return res;
	 chunk_id=READ_BLOCK_LONG(bdata,4); /* next */
	 if (!chunk_id) return MIRD_OK;
	 b=CHUNK_ID_2_BLOCK(db,chunk_id);
	 n=CHUNK_ID_2_FRAG(db,chunk_id);
	 if ( (res=mird_mark_used(mul,b)) )
	    return res;
	 if (n) return MIRD_OK; /* not a big block */
      }
   }

   if ( (res=mird_frag_get_b(db,chunk_id,&data,&bdata,&len)) )
      return res;

   if (READ_BLOCK_LONG(data,0)!=CHUNK_HASHTRIE)
      return MIRD_OK; /* end of line */

   if (recur_level>TOO_DEEP_RECURSION)
      return mird_generate_error(MIRDE_HASHTRIE_RECURSIVE,
				 orig_root,orig_key,0);
   
   if ( (READ_BLOCK_LONG(data,1)&filter) != (orig_key&filter) )
      return mird_generate_error(MIRDE_HASHTRIE_AWAY,
				 orig_root,orig_key,chunk_id);

   v=key_left&((1<<db->hashtrie_bits)-1);
   key_left>>=db->hashtrie_bits;
   
   chunk_id=READ_BLOCK_LONG(data,2+v);

   return 
      mird_ht_mark_use(db,orig_root,orig_key,
		       chunk_id,recur_level+db->hashtrie_bits,key_left,mul);
}

/* 
 * finds a cell in a hashtrie
 */

MIRD_RES mird_hashtrie_mark_use(struct mird *db,
				UINT32 root, /* chunk id */
				UINT32 key,
				struct mird_status_list *mul)

{
   return mird_ht_mark_use(db,root,key,root,0,key,mul);
}
