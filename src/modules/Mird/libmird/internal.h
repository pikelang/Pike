/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: internal.h,v 1.5 2002/10/09 13:28:39 nilsson Exp $
\*/

/*
** libMird by Mirar <mirar@mirar.org>
** please submit bug reports and patches to the author
**
** also see http://www.mirar.org/mird/
*/

/* #define MASSIVE_DEBUG */
/* #define SUPERMASSIVE_DEBUG */

#include "mird.h"
#include "config.h"

#ifdef MIRD_WONT_WORK
#error configure failed to find settings that would work on this system
#endif

/* what's UINT32? */

#define UINT32 unsigned INT32
#define UINT16 unsigned INT16

#ifdef INT64 
#define UINT64 unsigned INT64
#endif

/* what method to sync? */

#ifdef HAVE_FDATASYNC
#define FDATASYNC fdatasync
#else
#define FDATASYNC fsync
#endif

/* what lseek? */

#define LSEEK lseek

#include "physical.h"

/* find ntohl & htonl */
/* netinet/in.h seems to need sys/types.h on some systems */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

/* find memset, memcpy */

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* count syscalls */

#ifdef SYSCALL_STATISTICS
#define MIRD_SYSCALL_COUNT(DB,n) ( (DB)->syscalls_counter[n]++ )
#else
#define MIRD_SYSCALL_COUNT(DB,n) do {} while (0)
#endif
/*
  0 misc
  1 lseek
  2 read
  3 write
 */

/* internal structure */

#define POOL_ENTRIES 1024
struct mird_status_list 
{
   long size;
   long n;
   long pool_pos;
   int lock;

   struct mul_entry 
   {
      struct mul_entry *next;
      UINT32 x,y;
      UINT32 status;
   } **entries;

   struct mul_pool
   {
      struct mul_pool *next;
      struct mul_entry entries[POOL_ENTRIES];
   } *first_pool;

#ifdef MSL_COUNTER
   long c1;
   long c2;
   long c3;
   struct timeval tv;
#endif
};

#define FOURC(X) ((X)>>24),((X)>>16),((X)>>8),(X)

/*****************************************************************/
/* declarations of internal use                                  */
/*****************************************************************/

MIRD_RES mird_generate_error(int error_no,int x,int y,int z);
MIRD_RES mird_generate_error_s(int error_no,char *s,int x,int y,int z);
void mird_free_error(MIRD_RES error);

void mird_describe_error(MIRD_RES error,char **dest);
void mird_perror(char *prefix,MIRD_RES error);


/*** block.c *************************************************/

MIRD_RES mird_low_block_read(struct mird *db,UINT32 block,
			     unsigned char *dest,UINT32 n);
MIRD_RES mird_low_block_write(struct mird *db,UINT32 block,
			      unsigned char *data,UINT32 n);
MIRD_RES mird_block_write(struct mird *db,UINT32 block,
			  unsigned char *data);
MIRD_RES mird_block_read(struct mird *db,UINT32 block,
			 unsigned char *data);
MIRD_RES mird_cache_initiate(struct mird *db);
void mird_cache_free(struct mird *db);
void mird_cache_reset(struct mird *db);
MIRD_RES mird_cache_flush(struct mird *db);
MIRD_RES mird_cache_flush_transaction(struct mird_transaction *mtr);
MIRD_RES mird_cache_cancel_transaction(struct mird_transaction *mtr);
MIRD_RES mird_block_zot(struct mird *db,
			UINT32 block_no,
			unsigned char **dest);
MIRD_RES mird_block_get(struct mird *db,
			UINT32 block_no,
			unsigned char **dest);
MIRD_RES mird_block_get_w(struct mird *db,
			  UINT32 block_no,
			  unsigned char **dest);

/*** database.c **********************************************/

MIRD_RES mird_get_superblock(struct mird *db);
MIRD_RES mird_save_state(struct mird *db,int is_clean);

/* utility */

#ifdef MEM_DEBUG
#define mird_malloc(X,Y) _mird_malloc(X,Y,__FILE__,__LINE__)
MIRD_RES _mird_malloc(int bytes,void **dest,char *f,int l);
#else
MIRD_RES mird_malloc(int bytes,void **dest);
#endif

#define MIRD_MALLOC(N,D) mird_malloc((N),(void**)(D))

UINT32 mird_checksum(UINT32*data,UINT32 len);
UINT32 mird_random(UINT32 modulo);
void mird_fatal(char *why);

/*** freelist.c **********************************************/

MIRD_RES mird_freelist_push(struct mird *db,UINT32 no);
MIRD_RES mird_freelist_pop(struct mird *db,UINT32 *no);

MIRD_RES mird_freelist_initiate(struct mird *db);
MIRD_RES mird_freelist_sync(struct mird *db);

/*** journal.c ***********************************************/

MIRD_RES mird_journal_read(struct mird *db);
MIRD_RES mird_journal_new(struct mird *db);
MIRD_RES mird_journal_kill(struct mird *db);
MIRD_RES mird_journal_close(struct mird *db);

MIRD_RES mird_journal_log_low(struct mird *db, UINT32 type,
			      struct transaction_id trans_id,
			      UINT32 a,UINT32 b,UINT32 c,
			      UINT32 *checksum);
MIRD_RES mird_journal_log(struct mird_transaction *mtr,
			  UINT32 type, UINT32 a, UINT32 b, UINT32 c);
MIRD_RES mird_journal_log_flush(struct mird *db);

MIRD_RES mird_journal_get_offset(struct mird *db,MIRD_OFF_T *off);

MIRD_RES mird_journal_get(struct mird *db,
			  MIRD_OFF_T offset,
			  UINT32 n,
			  unsigned char *dest,
			  UINT32 *dest_n);

MIRD_RES mird_journal_write_pos(struct mird *db,
				MIRD_OFF_T *pos,
				UINT32 type,
				struct transaction_id trid,
				UINT32 a,UINT32 b,UINT32 c);

/*** transaction.c *******************************************/

MIRD_RES mird_transaction_new(struct mird *db,
			      struct mird_transaction **mtr);
MIRD_RES mird_transaction_finish(struct mird_transaction *mtr);
MIRD_RES mird_transaction_cancel(struct mird_transaction *mtr);

MIRD_RES mird_tr_resolve(struct mird_transaction *mtr);

void mird_tr_free(struct mird_transaction *mtr);
MIRD_RES mird_tr_rewind(struct mird_transaction *mtr);
MIRD_RES mird_tr_finish(struct mird_transaction *mtr);

MIRD_RES mird_tr_new_block(struct mird_transaction *mtr,
			   UINT32 *no,
			   unsigned char **data);
MIRD_RES mird_tr_new_block_no(struct mird_transaction *mtr,
			      UINT32 *no);
MIRD_RES mird_tr_unused(struct mird_transaction *mtr,
			UINT32 chunk_id);

void mird_simul_tr_free(struct mird *db,
			UINT32 no_msb,
			UINT32 no_lsb);
MIRD_RES mird_simul_tr_find(struct mird *db,
			    UINT32 no_msb,
			    UINT32 no_lsb,
			    struct mird_transaction **mtr_p);
MIRD_RES mird_simul_tr_new(struct mird *db,
			   UINT32 no_msb,
			   UINT32 no_lsb,
			   MIRD_OFF_T offset);
MIRD_RES mird_simul_tr_verify(struct mird_transaction *mtr);
MIRD_RES mird_simul_tr_rewind(struct mird_transaction *mtr,
			      MIRD_OFF_T stop,MIRD_OFF_T *wpos);

/*** tables.c ************************************************/

MIRD_RES mird_tables_resolve(struct mird_transaction *mtr);

MIRD_RES mird_table_write_root(struct mird_transaction *mtr,
			       UINT32 table_id,
			       UINT32 root);

MIRD_RES mird_tables_mark_use(struct mird *db,
			      UINT32 tables,
			      UINT32 table_id,
			      UINT32 key,
			      struct mird_status_list *mul);

MIRD_RES mird_low_table_new(struct mird_transaction *mtr,
			    UINT32 table_id,
			    UINT32 table_type);

MIRD_RES mird_db_table_get_root(struct mird *db,
				UINT32 table_id,
				UINT32 *root,
				UINT32 *type);

MIRD_RES mird_tr_table_get_root(struct mird_transaction *mtr,
				UINT32 table_id,
				UINT32 *root,
				UINT32 *type);

MIRD_RES mird_low_key_store(struct mird_transaction *mtr,
			    UINT32 table_id,
			    UINT32 key,
			    unsigned char *value,
			    UINT32 len,
			    UINT32 table_type);

MIRD_RES mird_low_key_lookup(struct mird *db,
			     UINT32 root,
			     UINT32 key,
			     unsigned char **data,
			     UINT32 *len);

MIRD_RES mird_low_table_scan(struct mird *db,
			     UINT32 root,
			     UINT32 n,
			     struct mird_scan_result *prev,
			     struct mird_scan_result **dest);

MIRD_RES mird_low_scan_continued(UINT32 key,
				 struct mird_scan_result **dest);

/*** skeys.c *************************************************/

MIRD_RES mird_s_key_resolve(struct mird_transaction *mtr,
			    UINT32 table_id,
			    UINT32 hashkey,
			    UINT32 old_cell,
			    UINT32 new_cell);

/*** frags.c *************************************************/

MIRD_RES mird_frag_new(struct mird_transaction *mtr,
		       UINT32 table_id,
		       UINT32 len,
		       UINT32 *chunk_id,
		       unsigned char **fdata);

MIRD_RES mird_frag_get_b(struct mird *db,
			 UINT32 chunk_id,
			 unsigned char **fdata,
			 unsigned char **bdata,
			 UINT32 *len);

MIRD_RES mird_frag_get_w(struct mird_transaction *mtr,
			 UINT32 chunk_id,
			 unsigned char **fdata,
			 UINT32 *len);

MIRD_RES mird_frag_close(struct mird_transaction *mtr);

#define mird_frag_get(DB,ID,DA,LE) mird_frag_get_b(DB,ID,DA,NULL,LE)

/*** cells.c *************************************************/

MIRD_RES mird_cell_write(struct mird_transaction *mtr,
			 UINT32 table_id,
			 UINT32 key,
			 UINT32 len,
			 unsigned char *srcdata,
			 UINT32 *chunk_id);

MIRD_RES mird_cell_get(struct mird *db,
		       UINT32 chunk_id,
		       UINT32 *key,
		       UINT32 *len,
		       unsigned char **data);

MIRD_RES mird_cell_read(struct mird *db,
			UINT32 chunk_id,
			unsigned char *dest,
			UINT32 len);

MIRD_RES mird_cell_get_info(struct mird *db,
			    UINT32 chunk_id,
			    UINT32 *key,
			    UINT32 *len);


/*** hashtrie.c **********************************************/

MIRD_RES mird_hashtrie_find_b(struct mird *db,
			      UINT32 root, /* chunk id */
			      UINT32 key,
			      UINT32 *cell_chunk,
			      unsigned char **bdata);

#define mird_hashtrie_find(DB,ID,KE,CE) \
	mird_hashtrie_find_b(DB,ID,KE,CE,NULL)

MIRD_RES mird_hashtrie_write(struct mird_transaction *mtr,
			     UINT32 table_id,
			     UINT32 root, /* chunk id */
			     UINT32 key,
			     UINT32 cell_chunk,
			     UINT32 *new_root,
			     UINT32 *old_cell,
			     UINT32 *old_is_mine);

MIRD_RES mird_hashtrie_resolve(struct mird_transaction *mtr,
			       UINT32 table_id,
			       UINT32 root_m,
			       UINT32 root_o,
			       UINT32* new_root_m);

MIRD_RES mird_hashtrie_next(struct mird *db,
			    UINT32 root,UINT32 key,
			    UINT32 n,UINT32 *dest_key,
			    UINT32 *dest_cell,UINT32 *dest_n);
MIRD_RES mird_hashtrie_first(struct mird *db,
			     UINT32 root,
			     UINT32 n,UINT32 *dest_key,
			     UINT32 *dest_cell,UINT32 *dest_n);
MIRD_RES mird_tr_hashtrie_next(struct mird_transaction *mtr,
			       UINT32 root,UINT32 key, 
			       UINT32 n,UINT32 *dest_key,
			       UINT32 *dest_cell, UINT32 *dest_n);
MIRD_RES mird_tr_hashtrie_first(struct mird_transaction *mtr,
				UINT32 root,
				UINT32 n,UINT32 *dest_key,
				UINT32 *dest_cell,UINT32 *dest_n);

MIRD_RES mird_hashtrie_free_all(struct mird_transaction *mtr,
				UINT32 root);

MIRD_RES mird_hashtrie_redelete(struct mird_transaction *mtr,
				UINT32 table_id,
				UINT32 root,UINT32 key,
				UINT32 *new_root);

MIRD_RES mird_hashtrie_mark_use(struct mird *db,
				UINT32 root,
				UINT32 key,
				struct mird_status_list *mul);

MIRD_RES mird_hashtrie_find_no_key(struct mird *db, 
				   UINT32 root,
				   UINT32 minimum_key,
				   UINT32 *dest_key);

/*** usage.c *************************************************/

MIRD_RES mird_mark_used(struct mird_status_list *mul,
			UINT32 block_no);

MIRD_RES mird_check_usage(struct mird *db);


MIRD_RES mird_status_new(struct mird *db,struct mird_status_list **mul);
void mird_status_free(struct mird_status_list *mul);
MIRD_RES mird_status_set(struct mird_status_list *mul,
			 UINT32 x,UINT32 y,UINT32 status);
MIRD_RES mird_status_get(struct mird_status_list *mul,
			 UINT32 x,UINT32 y,UINT32 *status);

/*** debug.c *************************************************/

void mird_describe_block(struct mird *db,
			 UINT32 block_no);
