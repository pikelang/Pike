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

/* handles the journal file */

#include <unistd.h>
#include <stdlib.h>

#include "internal.h"

#include "dmalloc.h"
#include <stdio.h> 

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

static const char RCSID[]=
   "$Id$";


/*
 * creates a new journal file
 */

MIRD_RES mird_journal_new(struct mird *db)
{
   char *filename;
   MIRD_RES res;
   int fd;

   if ( (db->flags & MIRD_READONLY) )
      return mird_generate_error_s(MIRDE_READONLY,
				   sstrdup("mird_journal_new"),0,0,0);

   if (db->jo_fd!=-1) 
   {
      close(db->jo_fd);
      db->jo_fd=-1;
   }

   if ( (res=MIRD_MALLOC(strlen(db->filename)+20,&filename)) )
      return res; /* out of memory */
   sprintf(filename,"%s.journal",db->filename);

   if (unlink(filename)==-1)
      if (errno!=ENOENT && errno)
	 return mird_generate_error_s(MIRDE_RM,filename,0,errno,0);

   fd=open(filename,
#ifdef HAVE_O_BINARY
	   O_BINARY|
#endif
	   O_CREAT|O_RDWR|O_EXCL|O_APPEND,0666);
   if (fd==-1)
      return mird_generate_error_s(MIRDE_OPEN_CREATE,filename,0,errno,0);

   db->jo_fd=fd;

   sfree(filename);

   return MIRD_OK;
}

/*
 * removes the journal file
 */

MIRD_RES mird_journal_kill(struct mird *db)
{
   char *filename;
   MIRD_RES res;

   if (db->jo_fd!=-1) 
   {
      close(db->jo_fd);
      db->jo_fd=-1;
   }

   if ( (res=MIRD_MALLOC(strlen(db->filename)+20,&filename)) )
      return res; /* out of memory */
   sprintf(filename,"%s.journal",db->filename);

   if (unlink(filename)==-1)
      if (errno!=ENOENT && errno)
	 return mird_generate_error_s(MIRDE_RM,filename,0,errno,0);

   sfree(filename);

   return MIRD_OK;
}

/*
 * opens in read-only mode
 */

MIRD_RES mird_journal_open_read(struct mird *db)
{
   char *filename;
   MIRD_RES res;
   int fd;

   if (db->jo_fd!=-1) 
   {
      close(db->jo_fd);
      db->jo_fd=-1;
   }

   if ( (res=MIRD_MALLOC(strlen(db->filename)+20,&filename)) )
      return res; /* out of memory */
   sprintf(filename,"%s.journal",db->filename);

   fd=open(filename,O_RDWR|O_APPEND);
   if (fd==-1)
      return mird_generate_error_s(MIRDE_MISSING_JOURNAL,filename,0,errno,0);
   db->jo_fd=fd;

   sfree(filename);

   return MIRD_OK;
}

/*
 * logs an entry to the journal file
 */

MIRD_RES mird_journal_log_flush(struct mird *db)
{
   off_t n;

   if (!db->journal_cache_n) return MIRD_OK; /* noop */

#ifndef WORKING_O_APPEND
   MIRD_SYSCALL_COUNT(db,1);
   n=lseek(db->jo_fd,0,SEEK_END);
   if (n==-1)
      return mird_generate_error(MIRDE_JO_LSEEK,0,errno,0);
#endif

   for (;;)
   {
      MIRD_SYSCALL_COUNT(db,3);
      n=(off_t)write(db->jo_fd,db->journal_cache,(6*4)*db->journal_cache_n);
      if (n==-1)
	 if (errno==EINTR) 
	    continue; /* try again */
	 else 
	    return mird_generate_error(MIRDE_JO_WRITE,0,errno,0);

      if (n!=(off_t)((6*4)*db->journal_cache_n))
      {
	 /* journal is destroyed at this point, make sure it is closed */
	 close(db->jo_fd);
	 db->jo_fd=-1;
	 return mird_generate_error(MIRDE_JO_WRITE_SHORT,
				    0,n,6*4);
      }

      db->journal_cache_n=0;

      return MIRD_OK;
   }
}

MIRD_RES mird_journal_log_low(struct mird *db,
			      UINT32 type,
			      struct transaction_id trid,
			      UINT32 a,
			      UINT32 b,
			      UINT32 c,
			      UINT32 *checksum)
{
   MIRD_RES res;
   unsigned char *buf;

   if (db->journal_cache_n==db->journal_writecache_n)
   {
      if ( (res=mird_journal_log_flush(db)) )
	 return res;
   }

   buf=db->journal_cache+(6*4)*db->journal_cache_n++;

   WRITE_BLOCK_LONG(buf,0,type);
   WRITE_BLOCK_LONG(buf,1,trid.msb);
   WRITE_BLOCK_LONG(buf,2,trid.lsb);
   WRITE_BLOCK_LONG(buf,3,a);
   WRITE_BLOCK_LONG(buf,4,b);
   WRITE_BLOCK_LONG(buf,5,c);

   if (checksum)
      checksum[0]+=mird_checksum((UINT32*)buf,6);

/*        if ( (res=mird_journal_log_flush(db)) ) */
/*  	 return res; */

   return MIRD_OK;
}

MIRD_RES mird_journal_log(struct mird_transaction *mtr,
			  UINT32 type,
			  UINT32 a,
			  UINT32 b,
			  UINT32 c)
{
   return mird_journal_log_low(mtr->db,type,mtr->no,a,b,c,
			       &(mtr->checksum));
}

/*
 * for simul: write at position 
 */

MIRD_RES mird_journal_write_pos(struct mird *db,
				MIRD_OFF_T *pos,
				UINT32 type,
				struct transaction_id trid,
				UINT32 a,UINT32 b,UINT32 c)
{
   UINT32 buf[6];
   off_t n;

   WRITE_BLOCK_LONG(buf,0,type);
   WRITE_BLOCK_LONG(buf,1,trid.msb);
   WRITE_BLOCK_LONG(buf,2,trid.lsb);
   WRITE_BLOCK_LONG(buf,3,a);
   WRITE_BLOCK_LONG(buf,4,b);
   WRITE_BLOCK_LONG(buf,5,c);

#if ! WORKING_O_APPEND || ! WORKING_F_TRUNCATE
   MIRD_SYSCALL_COUNT(db,0);
   n=LSEEK(db->jo_fd,*pos,SEEK_SET);

   if (n==-1)
      return mird_generate_error(MIRDE_JO_LSEEK,0,errno,0);
#endif

   for (;;)
   {
      MIRD_SYSCALL_COUNT(db,3);
      n=write(db->jo_fd,(void*)buf,6*4);
      if (n==-1)
	 if (errno==EINTR) 
	    continue; /* try again */
	 else 
	    return mird_generate_error(MIRDE_JO_WRITE,0,errno,0);

      if (n!=6*4)
	 return mird_generate_error(MIRDE_JO_WRITE_SHORT,
				    0,n,6*4);

      pos[0]+=6*4;
      return MIRD_OK;
   }
}


/*
 * gets current offset in the journal file
 */

MIRD_RES mird_journal_get_offset(struct mird *db,
				 MIRD_OFF_T *offset)
{
   MIRD_OFF_T n;

   MIRD_SYSCALL_COUNT(db,1);
   n=lseek(db->jo_fd,0,SEEK_END);
   if (n==-1)
      return mird_generate_error(MIRDE_JO_LSEEK,0,errno,0);

   if (sizeof(MIRD_OFF_T)>4 && n>(MIRD_OFF_T)0xffffffff) 
      return mird_generate_error(MIRDE_JO_TOO_BIG,0,0,0);

   offset[0]=n+(6*4)*db->journal_cache_n;

   return MIRD_OK;
}

/*
 * reads a piece of journal
 */

MIRD_RES mird_journal_get(struct mird *db,
			  MIRD_OFF_T offset,
			  UINT32 n,
			  unsigned char *dest,
			  UINT32 *dest_n)
{
   int fd=db->jo_fd;
   int res;

   MIRD_SYSCALL_COUNT(db,1);
   res=LSEEK(fd,offset,SEEK_SET);

   if (res==-1)
      return mird_generate_error(MIRDE_JO_LSEEK,offset,errno,0);
   
   for (;;)
   {
      MIRD_SYSCALL_COUNT(db,2);
      res=read(fd,(void*)dest,n*MIRD_JOURNAL_ENTRY_SIZE);

      if (res==-1)
	 if (errno==EINTR) 
	    continue; /* try again */
	 else 
	    return mird_generate_error(MIRDE_JO_READ,offset,errno,0);

      dest_n[0]=res/MIRD_JOURNAL_ENTRY_SIZE;

      return MIRD_OK;
   }
}

/*
 * syncs the system with the old and dirty journal file 
 */

MIRD_RES mird_journal_read(struct mird *db)
{
   MIRD_RES res;
   UINT32 *ent,*cur;
   MIRD_OFF_T pos=0,wpos,cpos;
   UINT32 n;
   struct mird_transaction *mtr;

   const UINT32 newn=htonl(MIRDJ_NEW_TRANSACTION);
   const UINT32 canceln=htonl(MIRDJ_TRANSACTION_CANCEL);
   const UINT32 finishn=htonl(MIRDJ_TRANSACTION_FINISHED);
   const UINT32 allon=htonl(MIRDJ_ALLOCATED_BLOCK);

   if ( (res=mird_journal_open_read(db)) )
   {
      /* no journal file */

      if ( (db->flags & MIRD_JO_COMPLAIN) )
      {
	 res->error_no=MIRDE_MISSING_JOURNAL;
	 return res;
      }

      /* if we can't complain, run as everything is ok */

      mird_free_error(res);
      return mird_journal_new(db);
   }

   /* ok, read the file;
    * create transactions after need
    * verify at transaction finish
    */

   if ( (res=MIRD_MALLOC(MIRD_JOURNAL_ENTRY_SIZE*db->journal_readback_n,
			 &ent)) )
      return res;

   for (;;)
   {
      if ( (res=mird_journal_get(db,pos,
				 db->journal_readback_n,
				 (unsigned char*)ent,&n)) )
	 break;
      
      if (!n)
      {
	 res=MIRD_OK;
	 break;
      }

      cur=ent;
      while (n--)
      {
	 struct mird_transaction *mtr;

#ifdef RECOVER_DEBUG
	 fprintf(stderr,"recover %c%c%c%c %ld:%ld %lxh %lxh %lxh\n",
		 FOURC(READ_BLOCK_LONG(cur,0)),
		 READ_BLOCK_LONG(cur,1),
		 READ_BLOCK_LONG(cur,2),
		 READ_BLOCK_LONG(cur,3),
		 READ_BLOCK_LONG(cur,4),
		 READ_BLOCK_LONG(cur,5));
#endif

	 if (cur[0]==newn)
	 {
	    /* new transaction */
	    if ( (res=mird_simul_tr_new(db,READ_BLOCK_LONG(cur,1),
					READ_BLOCK_LONG(cur,2),pos)) )
	       goto complete;

	    if (READ_BLOCK_LONG(cur,1)>db->next_transaction.msb ||
		(READ_BLOCK_LONG(cur,1)==db->next_transaction.msb &&
		 READ_BLOCK_LONG(cur,2)>=db->next_transaction.lsb))
	    {
	       db->next_transaction.msb=READ_BLOCK_LONG(cur,1),
		  db->next_transaction.lsb=READ_BLOCK_LONG(cur,2);
	       if (++db->next_transaction.lsb==0)
		  ++db->next_transaction.msb;
	    }
	 }

	 if ( (res=mird_simul_tr_find(db,READ_BLOCK_LONG(cur,1),
				      READ_BLOCK_LONG(cur,2),
				      &mtr)) )
	    goto complete;

	 if (!mtr) /* er, ops, well, lets finish here */
	    goto complete;

	 if (cur[0]==canceln)
	 {
	    if (READ_BLOCK_LONG(cur,5)!=mtr->checksum)
	    {
#ifdef RECOVER_DEBUG
	       fprintf(stderr,"checksum error got: %lxh should be: %lxh\n",
		       READ_BLOCK_LONG(cur,5),mtr->checksum);
#endif
	       res=MIRD_OK;
	       goto complete;
	    }

	    /* cancel a transaction */
	    mird_simul_tr_free(db,READ_BLOCK_LONG(cur,1),
			       READ_BLOCK_LONG(cur,2));
	 }
	 else if (cur[0]==finishn)
	 {
	    if (READ_BLOCK_LONG(cur,5)!=mtr->checksum)
	    {
#ifdef RECOVER_DEBUG
	       fprintf(stderr,"checksum error got: %lxh should be: %lxh\n",
		       READ_BLOCK_LONG(cur,5),mtr->checksum);
#endif
	       res=MIRD_OK;
	       goto complete;
	    }

	    if ( (res=mird_simul_tr_verify(mtr)) )
	    {
	       /* let's finish here */
#ifdef RECOVER_DEBUG
	       fprintf(stderr,"verify failed\n");
#endif
	       mird_free_error(res);
	       res=MIRD_OK;
	       goto complete;
	    }

	    db->tables=READ_BLOCK_LONG(cur,3);

	    mird_simul_tr_free(db,READ_BLOCK_LONG(cur,1),
			       READ_BLOCK_LONG(cur,2));
	 }
	 else if (cur[0]==allon)
	 {
	    UINT32 b;

	    /* allocated block, remove it from the freelists */

	    if ( (res=mird_freelist_pop(db,&b)) )
	       goto complete;

	    if (b!=READ_BLOCK_LONG(cur,3))
	    {
	       /* should never happen */
	       res=MIRD_OK;
	       goto complete;
	    }
	 }
	 mtr->checksum+=mird_checksum(cur,6);

	 cur+=MIRD_JOURNAL_ENTRY_SIZE/4;
	 pos+=MIRD_JOURNAL_ENTRY_SIZE;
      }
   }

complete:

#ifdef RECOVER_DEBUG
   fprintf(stderr,"give up / finished\n");
#endif

   sfree(ent);

   if (db->first_transaction)
   {
      /* ok, free the ones we ought to free */
      mtr=db->first_transaction;
      wpos=cpos=pos;

#if WORKING_FTRUNCATE && WORKING_O_APPEND
      /* then use the add-at-end method */
      ftruncate(db->jo_fd,wpos);
#endif

      while (mtr)
      {
	 if ( (res=mird_simul_tr_rewind(mtr,cpos,&wpos)) )
	    return res;
	 mtr=mtr->next;
      }
#if WORKING_FTRUNCATE && ! WORKING_O_APPEND
      /* just short it so the usage checker doesn't have to check all */
      ftruncate(db->jo_fd,wpos);
#endif
   }

   while (db->first_transaction)
   {
      mird_simul_tr_free(db,db->first_transaction->no.msb,
			 db->first_transaction->no.lsb);
   }

   mird_sync(db);
   /*    mird_debug_check_free(db); */
   
   return res;
}
