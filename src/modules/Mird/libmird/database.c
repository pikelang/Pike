/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: database.c,v 1.7 2003/01/03 20:52:48 grubba Exp $
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
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

static const char RCSID[]=
   "$Id: database.c,v 1.7 2003/01/03 20:52:48 grubba Exp $";

/* forward declarations */

static MIRD_RES mird_open_file(struct mird *db,
			       int flags,int lock_file,int *dfd);
static MIRD_RES mird_open_old_database(struct mird *db);

static int mird_valid_sizes(struct mird *db);

/* #define TIMER_DEBUG */
#ifdef TIMER_DEBUG
#include <unistd.h>
#include <sys/time.h>

#define M_TIMER(X) struct timeval X
#define M_SET_TIMER(X) gettimeofday(&(X),NULL)
#define TDIFF(X,Y) ((Y).tv_usec-(X).tv_usec)*1e-6+((Y).tv_sec-(X).tv_sec)
#else
#define M_TIMER(X) 
#define M_SET_TIMER(X) 
#endif

/* 
 * open and close of the database
 * ------------------------------
 */

/*
 * creates a database structure with the default setup
 */

MIRD_RES mird_initialize(char *filename,
			   struct mird **dest)
{
   struct mird *db;

   dest[0]=NULL;

   db=smalloc(sizeof(struct mird));
   if (!db)
      return mird_generate_error(MIRDE_RESOURCE_MEM,
				  sizeof(struct mird),0,0);

   db->filename=sstrdup(filename);
   if (!db->filename)
   {
      sfree(db);
      return mird_generate_error(MIRDE_RESOURCE_MEM,
				  sizeof(strlen(filename)),0,0);
   }

   db->flags=MIRD_DEFAULT_FLAGS;
  
   db->block_size=MIRD_DEFAULT_BLOCK_SIZE;
   db->frag_bits=MIRD_DEFAULT_FRAG_BITS;
   db->hashtrie_bits=MIRD_DEFAULT_HASHTRIE_BITS;
   db->cache_size=MIRD_DEFAULT_CACHE_SIZE;
   db->cache_search_length=MIRD_DEFAULT_CACHE_SEARCH_LENGTH;
   db->max_free_frag_blocks=MIRD_DEFAULT_MAX_FREE_FRAG_BLOCKS;
   db->file_mode=MIRD_DEFAULT_FILE_MODE;
   db->journal_readback_n=MIRD_DEFAULT_JOURNAL_READBACK_N;
   db->journal_writecache_n=MIRD_DEFAULT_JOURNAL_WRITECACHE_N;

   db->db_fd=-1;
   db->jo_fd=-1;
   db->cache=NULL;
   db->journal_cache=NULL;
   db->buffer=NULL;

   db->last_commited.msb=0;
   db->last_commited.lsb=0;
   db->cache_table_id=0;

   db->free_list.blocks=NULL;
   db->new_free_list.blocks=NULL;

   db->first_transaction=NULL;

   db->syscalls_counter[0]=0;
   db->syscalls_counter[1]=0;
   db->syscalls_counter[2]=0;
   db->syscalls_counter[3]=0;
   db->syscalls_counter[4]=0;
   db->syscalls_counter[5]=0;
   db->syscalls_counter[6]=0;
   db->syscalls_counter[7]=0;

   dest[0]=db;

   return MIRD_OK;
}

/*
 * opens or creates a database 
 */

MIRD_RES mird_open(struct mird *db)
{
   MIRD_RES res;
   int flags;

   /* open the file according to flags */

   if ( (db->flags & MIRD_READONLY) ) flags=O_RDONLY;
   else if ( (db->flags & MIRD_NOCREATE) ) flags=O_RDWR;
   else flags=O_RDWR|O_CREAT;
   if ( (db->flags & MIRD_EXCL) ) flags|=O_EXCL;

   res=mird_open_file(db,flags,!(db->flags&MIRD_READONLY),&(db->db_fd));
   if (res) 
      if (res->error_no!=MIRDE_CREATED_FILE)
	 return res;
      else
      {
	 mird_free_error(res);

	 /* check our parameters */
	 if (!mird_valid_sizes(db))
	    return mird_generate_error(MIRDE_INVALID_SETUP,0,0,0);
	 /* database file created; initiate the file */

	 db->free_list.next=0;
	 db->new_free_list.next=0;
	 db->last_used=0;
	 db->next_transaction.lsb=1;
	 db->next_transaction.msb=0;

	 db->tables=0; 

	 if ( (res=mird_save_state(db,1)) ) /* new == clean */
	    return res; 

	 /* always open the new file as an old anyway */
      }

   db->buffer=smalloc(db->block_size);
   if (!db->buffer)
   {
      sfree(db->filename);
      sfree(db);
      return mird_generate_error(MIRDE_RESOURCE_MEM,db->block_size,0,0);
   }

   if ( (res=mird_open_old_database(db)) )
      return res;

   return MIRD_OK; /* all done! */
}

/*
 * writes a state to the superblocks
 */

MIRD_RES mird_save_state(struct mird *db,int is_clean)
{
   UINT32 i;
   MIRD_RES res;
   unsigned char *data;
   
   /* make a superblock */
   if ( (res=MIRD_MALLOC(db->block_size,&data)) )
      return res; /* out of memory */

   memset((void*)data, 0, db->block_size);

   if (is_clean)
   {
      /* update those fields */
      db->last_last_used=db->last_used;
      db->last_tables=db->tables;
      db->last_free_list=db->free_list.next;
      db->last_next_transaction=db->next_transaction;
   }

   WRITE_BLOCK_LONG(data, 0, SUPERBLOCK_MIRD);
   WRITE_BLOCK_LONG(data, 1, DB_VERSION); /* version */
   WRITE_BLOCK_LONG(data, 2, BLOCK_SUPER);
   WRITE_BLOCK_LONG(data, 3, is_clean);
   WRITE_BLOCK_LONG(data, 4, db->block_size);
   WRITE_BLOCK_LONG(data, 5, db->frag_bits);
   WRITE_BLOCK_LONG(data, 6, db->hashtrie_bits);

   WRITE_BLOCK_LONG(data, 9, db->last_used);
   WRITE_BLOCK_LONG(data,10, db->last_last_used);
   WRITE_BLOCK_LONG(data,11, db->tables);
   WRITE_BLOCK_LONG(data,12, db->last_tables);
   WRITE_BLOCK_LONG(data,13, db->free_list.next);
   WRITE_BLOCK_LONG(data,14, db->last_free_list);

   WRITE_BLOCK_LONG(data,20, db->next_transaction.msb);
   WRITE_BLOCK_LONG(data,21, db->next_transaction.lsb);
   WRITE_BLOCK_LONG(data,22, db->last_next_transaction.msb);
   WRITE_BLOCK_LONG(data,23, db->last_next_transaction.lsb);
   
   /* write them */
   for (i=1; i&&i-1<=db->last_used; i<<=2)
   {
      WRITE_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-2,mird_random(0xffffffff));
      if ( (res=mird_block_write(db,i-1,data)) )
      {
	 sfree(data);
	 return res; /* failed to save all superblocks */
      }
   }
   sfree(data);
   return MIRD_OK;
}

/*
 * syncs a database (except running transactions)
 */

static MIRD_RES mird_clean(struct mird *db)
{
   MIRD_RES res;

   /* flush the journal (if needed) */

   if ( (res=mird_journal_log_flush(db)) )
      return res;
   
   /* reread the journal file and free all that needs to be freed */
   
   if ( (res=mird_check_usage(db)) ) return res;

   /* sync the free lists and the cache */

   if ( (res=mird_freelist_sync(db)) ) return res;
   if ( (res=mird_cache_flush(db)) ) return res;

   if ( (res=mird_journal_log_flush(db)) )
      return res;

/*    mird_debug_check_free(db); */

   if ( (res=mird_save_state(db,0)) ) return res; /* still dirty */

   MIRD_SYSCALL_COUNT(db,0);
   if ( FDATASYNC(db->jo_fd)==-1 )
      return mird_generate_error(MIRDE_JO_SYNC,0,errno,0);

   MIRD_SYSCALL_COUNT(db,0);
   if ( FDATASYNC(db->db_fd)==-1 )
      return mird_generate_error(MIRDE_DB_SYNC,0,errno,0);

   if ( ( db->flags & MIRD_CALL_SYNC ) )
   {
      MIRD_SYSCALL_COUNT(db,0);
      sync(); /* may not fail */
   }

   if ( (res=mird_save_state(db,1)) ) return res; /* clean now */

   MIRD_SYSCALL_COUNT(db,0);
   if ( FDATASYNC(db->db_fd)==-1 )
      return mird_generate_error(MIRDE_DB_SYNC,0,errno,0);

   if ( ( db->flags & MIRD_CALL_SYNC ) )
   {
      MIRD_SYSCALL_COUNT(db,0);
      sync(); /* may not fail */
   }

   return MIRD_OK;
}

MIRD_RES mird_readonly_refresh(struct mird *db)
{
   MIRD_RES res;

   if ( (res=mird_block_read(db,0,db->buffer)) )
      return res;


   db->tables=READ_BLOCK_LONG(db->buffer,11);
   if (db->last_tables!=READ_BLOCK_LONG(db->buffer,12))
   {
/* there has been a sync, reset cache */
      mird_cache_reset(db);
   }

   return MIRD_OK;
}

MIRD_RES mird_sync(struct mird *db)
{
   MIRD_RES res;
   
   db->flags&=~MIRD_PLEASE_SYNC;

   if ( (db->flags & MIRD_READONLY) )
   {
      return mird_readonly_refresh(db); /* we're read only */
   }

   if (db->first_transaction)
      return mird_generate_error(MIRDE_TR_RUNNING,0,0,0);

   if ( (res=mird_clean(db)) )
      return res;

   /* ok, reinit states */
   if ( (res=mird_save_state(db,0)) ) return res;
   
   if ( (res=mird_journal_new(db)) ) return res; 
   
   /* done */
   return MIRD_OK;
}

MIRD_RES mird_sync_please(struct mird *db)
{
   if (!db->first_transaction)
   {
      if (!(db->flags & MIRD_READONLY) &&
	  db->next_transaction.msb==db->last_next_transaction.msb &&
	  db->next_transaction.lsb==db->last_next_transaction.lsb)
	 return MIRD_OK; /* don't do anything if not needed */
	  
      return mird_sync(db);
   }
   db->flags|=MIRD_PLEASE_SYNC;
   return MIRD_OK;
}

/*
 * closes a database and frees the structure
 */

MIRD_RES mird_close(struct mird *db)
{
   MIRD_RES res;

   if ( !(db->flags & MIRD_READONLY) )
   {
      while ( (db->first_transaction) )
      {
	 if ( !(db->first_transaction->flags & MIRDT_CLOSED) &&
	      (res=mird_tr_rewind(db->first_transaction)) )
	    return res;
	 db->first_transaction->db=NULL;
	 db->first_transaction=db->first_transaction->next;
      }

      if ( (res=mird_clean(db)) )
	 return res;

      MIRD_SYSCALL_COUNT(db,0);
      if ( close(db->db_fd)==-1 )
	 return mird_generate_error(MIRDE_DB_CLOSE,0,errno,0);
      db->db_fd=-1;
      if ( ( db->flags & MIRD_CALL_SYNC ) )
	 sync(); /* may not fail */

      if ( (res=mird_journal_kill(db)) )
	 return res;
   }

   /* closed, free structure */

   mird_free_structure(db);
   return MIRD_OK;
}


/*
 * frees a database structure
 */

void mird_free_structure(struct mird *db)
{
   struct mird_transaction *mtr;

   if (db->free_list.blocks) sfree(db->free_list.blocks);
   if (db->new_free_list.blocks) sfree(db->new_free_list.blocks);

   mird_cache_free(db);
   if (db->buffer) sfree(db->buffer);
   
   sfree(db->filename);

   /* free these if they are open */
   if (db->db_fd!=-1) close(db->db_fd);
   if (db->jo_fd!=-1) close(db->jo_fd);

   /* destroy for reuse */
   db->db_fd=-1; 
   db->jo_fd=-1; 
   db->filename=NULL;

   /* destroy all transactions */
   mtr=db->first_transaction;
   while (mtr) { mtr->db=NULL; mtr=mtr->next; }

   /* all done */
   sfree(db);
}

/*
 * finds the last valid superblock from the database
 * cleans the database or returns error if dirty & no clean
 */

/*
 * superblocks are blocks no 4^n-1,
 * 0,3,15,63,...,up to the highest used block
 */

static MIRD_RES mird_open_old_database(struct mird *db)
{
   struct stat st;
   UINT32 i;
   int fd=db->db_fd;
   unsigned char sbuf[40];
   unsigned char *data;
   MIRD_RES res;
   int is_clean=0;
   UINT32 version;

   db->free_list.n=0;
   db->new_free_list.n=0;
   db->new_free_list.next=0;
   db->new_free_list.last=0;

   if (fstat(fd,&st)==-1)
      return mird_generate_error(MIRDE_DB_STAT,errno,0,0);
   
   /* first block tells us the size, if valid */

   MIRD_SYSCALL_COUNT(db,1);
   if (lseek(fd,0,SEEK_SET)==-1)
      return mird_generate_error(MIRDE_DB_LSEEK,0,errno,0);

   for (;;)
   {
      MIRD_OFF_T z;
      MIRD_SYSCALL_COUNT(db,2);
      z=(MIRD_OFF_T)read(fd,sbuf,40);
      if (z==-1)
	 if (errno==EINTR)
	    continue;
	 else
	    return mird_generate_error(MIRDE_DB_READ,0,errno,0);
      else if (z!=40)
	 return mird_generate_error(MIRDE_INVAL_HEADER,0,0,0);
      break;
   }
   
   if (READ_BLOCK_LONG(sbuf,0)!=SUPERBLOCK_MIRD ||
       READ_BLOCK_LONG(sbuf,2)!=BLOCK_SUPER)
      return mird_generate_error(MIRDE_INVAL_HEADER,0,0,0);

   if ((version=READ_BLOCK_LONG(sbuf,1))!=DB_VERSION) 
      return mird_generate_error(MIRDE_UNKNOWN_VERSION,0,0,0);

   db->block_size=READ_BLOCK_LONG(sbuf,4);
   db->frag_bits=READ_BLOCK_LONG(sbuf,5);
   db->hashtrie_bits=READ_BLOCK_LONG(sbuf,6);

   if (!mird_valid_sizes(db))
      return mird_generate_error(MIRDE_INVAL_HEADER,0,0,0);

   /* ok, now we know the facts */

   if ( (res=MIRD_MALLOC(db->block_size,&data)) )
      return res; /* out of memory */

   /* find the most beautiful superblock: */
   /* all superblock checks & agrees -> fine */
   /* otherwise, report f*ckup */

   for (i=1; i && i<=st.st_size/db->block_size; i<<=2)
   {
      res=mird_low_block_read(db,i-1,data,1);
      if (res) 
      {
	 mird_free_error(res);
	 if (i==1)
	 {
	    /* bad; we expecteded a superblock here */
	    sfree(data);
	    return mird_generate_error_s(MIRDE_GARBLED,
					 "first superblock bad",0,0,0);
	 }
	 else /* it might be a hole or something */
	    continue;
      }
      if (i!=1 && 
	  mird_checksum((UINT32*)data,(db->block_size>>2)-1) !=
	  READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1))
      {
	 /* it's a hole */
	 continue;
      }

      /* check if the block is valid */
      if (READ_BLOCK_LONG(data,0)!=SUPERBLOCK_MIRD ||
	  READ_BLOCK_LONG(data,2)!=BLOCK_SUPER 
	  ||
	  mird_checksum((UINT32*)data,(db->block_size>>2)-1) !=
	  READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1)
	  ||
	  version!=READ_BLOCK_LONG(data,1) ||
	  db->block_size!=READ_BLOCK_LONG(data,4) ||
	  db->frag_bits!=READ_BLOCK_LONG(data,5) ||
	  db->hashtrie_bits!=READ_BLOCK_LONG(data,6))
      {
	 sfree(data);
#if 0
	 fprintf(stderr,"a s%d b%d c%d e%d\n",
		 READ_BLOCK_LONG(data,0)!=SUPERBLOCK_MIRD,
		 READ_BLOCK_LONG(data,2)!=BLOCK_SUPER,
		 mird_checksum((UINT32*)data,(db->block_size>>2)-1) !=
		 READ_BLOCK_LONG(data,LONGS_IN_BLOCK(db)-1),
		 version!=READ_BLOCK_LONG(data,1) ||
		 db->block_size!=READ_BLOCK_LONG(data,4) ||
		 db->frag_bits!=READ_BLOCK_LONG(data,5) ||
		 db->hashtrie_bits!=READ_BLOCK_LONG(data,6));
#endif
	 return mird_generate_error_s(MIRDE_GARBLED,"superblocks disagree",
				      0,0,0);	 
      }

      if (i==1) /* read facts from first block */
      {
	 is_clean=READ_BLOCK_LONG(data,3);
	 db->free_list.next=
	    db->last_free_list=READ_BLOCK_LONG(data,14);
	 db->last_last_used=
	    db->last_used=READ_BLOCK_LONG(data,10);
	 db->last_tables=
	    db->tables=READ_BLOCK_LONG(data,12);
	 db->next_transaction.msb=READ_BLOCK_LONG(data,22);
	 db->next_transaction.lsb=READ_BLOCK_LONG(data,23);
	 db->last_next_transaction=db->next_transaction;
      }
      else /* check facts */
      {
	 if ( (db->last_used!=READ_BLOCK_LONG(data,10)) ||
	      (db->last_tables!=READ_BLOCK_LONG(data,12)) ||
	      (db->free_list.next!=READ_BLOCK_LONG(data,14)) ||
	      (db->next_transaction.msb!=READ_BLOCK_LONG(data,22)) ||
	      (db->next_transaction.lsb!=READ_BLOCK_LONG(data,23)) )
	 {
	    if ((db->next_transaction.msb==READ_BLOCK_LONG(data,22) &&
		 db->next_transaction.lsb>READ_BLOCK_LONG(data,23)) ||
		db->next_transaction.msb>READ_BLOCK_LONG(data,22))
	    {
	       /* old generation; ignore */
	       continue;
	    }
		
	    sfree(data);
	    return mird_generate_error_s(MIRDE_GARBLED,"superblocks mismatch",
					 0,0,0);	 
	 }

	 is_clean=READ_BLOCK_LONG(data,3)?is_clean:0;
      }
   }


   sfree(data);

   if ( (res=mird_cache_initiate(db)) ) 
      return res; /* error */

   if ( (db->flags & MIRD_READONLY) )
      return mird_readonly_refresh(db);

   if ( (res=mird_freelist_initiate(db)) )
      return res; 

   if (!is_clean)
   {
      if ( (res=mird_journal_read(db)) )
	 return res;

      if ( (res=mird_sync(db)) ) /* this opens the journal too */
	 return res;
   }
   else
      if ( (res=mird_journal_new(db)) )
	 return res;

   if ( (res=mird_save_state(db,0)) ) /* mark file as dirty */
      return res;
   
   return MIRD_OK; /* all done */
}

/*
 * utility functions
 */

static MIRD_RES mird_open_file(struct mird *db,
			       int flags,int lock_file,int *dfd)
{
   int fd=-1;

#ifdef HAVE_O_BINARY
   flags|=O_BINARY;
#endif

   MIRD_SYSCALL_COUNT(db,0);
   if (!(flags&O_EXCL))
      fd=open(db->filename,flags&~O_CREAT);
   if ((flags&O_EXCL) || fd==-1)
   {
      if ( (flags & O_CREAT) )
      {
	 MIRD_SYSCALL_COUNT(db,0);
	 fd=open(db->filename,flags,db->file_mode);
	 if (fd==-1)
	    return mird_generate_error_s(MIRDE_OPEN_CREATE,
					 sstrdup(db->filename),errno,0,0);

	 if (lock_file && 
#ifdef HAS_FLOCK
	     flock(fd,LOCK_EX|LOCK_NB)==-1
#else
#ifdef HAS_LOCKF
	     !!lockf(fd,F_LOCK,0)
#else
	     0
#endif
#endif
	     )
	 {
	    MIRD_SYSCALL_COUNT(db,0);
	    close(fd);
	    return mird_generate_error_s(MIRDE_LOCK_FILE,
					 sstrdup(db->filename),errno,0,0);
	 }

	 dfd[0]=fd;
	 return mird_generate_error(MIRDE_CREATED_FILE,0,0,0);
      }
      else
	 return mird_generate_error_s(MIRDE_OPEN,
				      sstrdup(db->filename),errno,0,0);
   }
   if (lock_file)
   {
      MIRD_SYSCALL_COUNT(db,0);
      if (
#ifdef HAS_FLOCK
	 flock(fd,LOCK_EX|LOCK_NB)==-1
#else
#ifdef HAS_LOCKF
	 !!lockf(fd,F_LOCK,0)
#else
	 0
#endif
#endif
	  )
      {
	 close(fd);
	 return mird_generate_error_s(MIRDE_LOCK_FILE,
				      sstrdup(db->filename),errno,0,0);
      }
   }
   dfd[0]=fd;
   return MIRD_OK;
}

#ifdef MEM_DEBUG
MIRD_RES _mird_malloc(int bytes,void **dest,char *f,int l)
#else
MIRD_RES mird_malloc(int bytes,void **dest)
#endif
{
#ifdef MEM_DEBUG
   if (!(dest[0]=_smalloc(bytes,f,l)))
#else
   if (!(dest[0]=smalloc(bytes)))
#endif
      return mird_generate_error(MIRDE_RESOURCE_MEM,bytes,0,0);

   memset(dest[0],17,bytes);

   return MIRD_OK;
}

void mird_free(unsigned char *buffer)
{
   sfree(buffer);
}

UINT32 mird_checksum(UINT32 *ldata,UINT32 len)
{
   register UINT32 z=len*0x34879945;
   register unsigned char *data=(unsigned char*)ldata;

   register unsigned long q=len/8;

   while (q--)
   {
      z+=(z<<5)^((data[ 0]<<24)|(data[ 1]<<16)|(data[ 2]<<8)|data[ 3]);
      z+=(z<<5)^((data[ 4]<<24)|(data[ 5]<<16)|(data[ 6]<<8)|data[ 7]);
      z+=(z<<5)^((data[ 8]<<24)|(data[ 9]<<16)|(data[10]<<8)|data[11]);
      z+=(z<<5)^((data[12]<<24)|(data[13]<<16)|(data[14]<<8)|data[15]);
      z+=(z<<5)^((data[16]<<24)|(data[17]<<16)|(data[18]<<8)|data[19]);
      z+=(z<<5)^((data[20]<<24)|(data[21]<<16)|(data[22]<<8)|data[23]);
      z+=(z<<5)^((data[24]<<24)|(data[25]<<16)|(data[26]<<8)|data[27]);
      z+=(z<<5)^((data[28]<<24)|(data[29]<<16)|(data[30]<<8)|data[31]);
      data+=4*8;
   }

   len=len&7;

   while (len--)
      z+=(z<<5)+((data[0]<<23)|(data[1]<<16)|(data[2]<<7)|data[3]);

   return z;
}

static UINT32 mird_r1=0x12345678;
static UINT32 mird_r2=0x9abcdef0;

UINT32 mird_random(UINT32 modulo)
{
   mird_r2=((mird_r2<<17)*0x91283915+((mird_r2>>15)^mird_r2));
   mird_r1=mird_r2+mird_r1%modulo;
   return mird_r1%modulo;
}

/*
 * checks if block_size, frag_bits and hashtrie_bits 
 * are in valid ranges
 */

static int mird_valid_sizes(struct mird *db)
{
   /* check given sizes */
   UINT32 i;
   for (i=7; i<31; i++)
      if (db->block_size==(UINT32)(1<<i)) break;
   if (i==31) return 0;
   
   if ( (UINT32)(4<<db->frag_bits)>(db->block_size/4) ) return 0;
   if ( (UINT32)(4<<db->hashtrie_bits)>(db->block_size/4) ) return 0;
   
   if ( !db->cache_search_length ||
	!db->max_free_frag_blocks ||
	!db->journal_readback_n ||
	!db->cache_size) return 0;

   return 1;
}
