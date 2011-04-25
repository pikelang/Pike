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
#include <unistd.h>
#include <stdio.h>

/* for strerror */
#ifdef HAVE_STRING_H
#include <string.h> 
#endif

#include "dmalloc.h"

static const char RCSID[]=
   "$Id$";

static struct mird_error failed_to_allocate_error =
{ MIRDE_RESOURCE_ERR,NULL,sizeof(struct mird_error),0,0 };

struct mird_error* mird_generate_error(int error_no,int x,int y,int z)
{
   struct mird_error *me;

/*     if (error_no!=MIRDE_CREATED_FILE) */
/*        mird_fatal("bang"); */

   me=smalloc(sizeof(struct mird_error));
   if (!me) return &failed_to_allocate_error;
   me->error_no=error_no;
   me->x=x;
   me->y=y;
   me->z=z;
   me->s=NULL;
   return me;
}

struct mird_error* mird_generate_error_s(int error_no,char *s,
					   int x,int y,int z)
{
   struct mird_error *me;

/*     if (error_no!=MIRDE_CREATED_FILE) */
/*        mird_fatal("bang"); */

   me=smalloc(sizeof(struct mird_error));
   if (!me) return &failed_to_allocate_error;
   me->error_no=error_no;
   me->x=x;
   me->y=y;
   me->z=z;
   me->s=s;
   return me;
}

void mird_free_error(MIRD_RES error)
{
   if (error==&failed_to_allocate_error) return;
   if (error->s) sfree(error->s);
   error->error_no=0;
   error->s=NULL; /* don't reuse */
   sfree(error);
}

void mird_fatal(char *why)
{
   char buf[200];
   sprintf(buf,"MIRD FATAL: %s\n",why);
   write(2,buf,strlen(buf));
   abort();
}

void mird_describe_error(MIRD_RES error,char **dest)
{
   dest[0]=smalloc((error->s?strlen(error->s):0)+100);
   if (!dest[0]) return;

   switch (error->error_no)
   {
      /* invalid arguments */
      case MIRDE_INVALID_SETUP: 
	 sprintf(dest[0],"invalid database parameters");
	 break;

      case MIRDE_TR_CLOSED:
	 sprintf(dest[0],"transaction is closed already");
	 break;

	 /* file open/create/lock general errors, s=filename, x=errno */
      case MIRDE_OPEN	: 
	 sprintf(dest[0],"open failed; \"%s\", errno=%ld (%s)",
			 error->s,error->x,strerror(error->x));
	 break;
      case MIRDE_OPEN_CREATE: 
	 sprintf(dest[0],"open/create failed; \"%s\", errno=%ld (%s)",
		 error->s,error->x,strerror(error->x));
	 break;
      case MIRDE_LOCK_FILE: 
	 sprintf(dest[0],"didn't get lock on file; \"%s\", errno=%ld (%s)",
		 error->s,error->x,strerror(error->x));
	 break;
      case MIRDE_RM	: 
	 sprintf(dest[0],"rm failed; \"%s\", errno=%ld (%s)",
			 error->s,error->x,strerror(error->x));
	 break;

	 /* special case returned from mird_open_file, 
	  * meaning the file was created (no other info) */
      case MIRDE_CREATED_FILE: 
	 sprintf(dest[0],"file created");
	 break;

	 /* database file errors, x=block no */
      case MIRDE_DB_LSEEK: 
	 sprintf(dest[0],"lseek failure, block %lxh, errno=%ld (%s)",
		 error->x,error->y,strerror(error->y));
	 break;
      case MIRDE_DB_READ: 
	 sprintf(dest[0],"read failure, block %lxh, errno=%ld (%s)",
		 error->x,error->y,strerror(error->y));
	 break;
      case MIRDE_DB_READ_SHORT: 
	 sprintf(dest[0],"read too few bytes, block %lxh, "
		 "expected %ld, read %ld bytes",
		 error->x,error->z,error->y);
	 break;
      case MIRDE_DB_WRITE: 
	 sprintf(dest[0],"write failure, block %lxh, errno=%ld (%s)",
		 error->x,error->y,strerror(error->y));
	 break;
      case MIRDE_DB_WRITE_SHORT: 
	 sprintf(dest[0],"wrote too few bytes, block %lxh, "
		 "expected %ld, wrote %ld bytes",
		 error->x,error->z,error->y);
	 break;
      case MIRDE_DB_READ_CHECKSUM: 
	 sprintf(dest[0],"checksum error in block read, block %lxh",
		 error->x);
	 break;

      case MIRDE_DB_STAT: 
	 sprintf(dest[0],"call to fstat failed, errno=%ld (%s)",
		 error->y,strerror(error->y));
	 break;

      case MIRDE_DB_SYNC: 
	 sprintf(dest[0],"call to fsync failed, errno=%ld (%s)",
		 error->y,strerror(error->y));
	 break;

      case MIRDE_DB_CLOSE: 
	 sprintf(dest[0],"call to close failed, errno=%ld (%s)",
		 error->y,strerror(error->y));
	 break;

      case MIRDE_WRONG_BLOCK: 
	 sprintf(dest[0],"wrong block type; block %lxh, expected %lxh got %lxh",
		 error->x,error->y,error->z);
	 break;

      case MIRDE_ILLEGAL_FREE:
	 sprintf(dest[0],"linked freelist is not freelist; block %08lx",
		 error->x);
	 break;

      case MIRDE_WRONG_CHUNK: 
	 sprintf(dest[0],"wrong chunk type; chunk %lxh, expected %lxh got %lxh",
		 error->x,error->y,error->z);
	 break;

      case MIRDE_CELL_SHORT: 
	 sprintf(dest[0],"shortage of cell data, expected longer chain; chunk %lxh",
		 error->x);
	    break;

      case MIRDE_DATA_MISSING: 
	 sprintf(dest[0],"string cell is uneven (this is bad)");
	    break;

      case MIRDE_SMALL_CHUNK: 
	 sprintf(dest[0],"chunk is too small; chunk %lxh, needed %ld bytes, type %lxh",
		 error->x,error->y,error->z);
	    break;

      case MIRDE_HASHTRIE_RECURSIVE:
	 sprintf(dest[0],"hashtrie is recursive; hashtrie root %lxh, orig key: %lxh, at node: %lxh",
		 error->x,error->y,error->z);
	    break;

      case MIRDE_HASHTRIE_AWAY:
	 sprintf(dest[0],"hashtrie led to foreign key; hashtrie root %lxh, orig key: %lxh, at node: %lxh",
		 error->x,error->y,error->z);
	    break;
	 
      case MIRDE_ILLEGAL_FRAG: 
	 sprintf(dest[0],"try to read an illegal frag, block %lxh frag %ld",
		 error->x,error->y);
	 break;

      case MIRDE_INVAL_HEADER: 
	 sprintf(dest[0],"given file has an invalid header");
	 break;
      case MIRDE_UNKNOWN_VERSION: 
	 sprintf(dest[0],"can't read that version of database");
	 break;
      case MIRDE_GARBLED: 
	 sprintf(dest[0],"database garbled (%s); need resurrection",
		 error->s);
	 break;

/* journal file errors */

      case MIRDE_JO_LSEEK: 
	 sprintf(dest[0],"journal lseek failure, errno=%ld (%s)",
		 error->y,strerror(error->y));
	 break;

      case MIRDE_JO_WRITE: 
	 sprintf(dest[0],"journal write failure, errno=%ld (%s)",
		 error->y,strerror(error->y));
	 break;
      case MIRDE_JO_WRITE_SHORT: 
	 sprintf(dest[0],"journal write too bytes, "
		 "expected %ld, wrote %ld bytes",
		 error->z,error->y);
	 break;
      case MIRDE_JO_READ: 
	 sprintf(dest[0],"journal read failure, pos=%ld, errno=%ld (%s)",
		 error->x,error->y,strerror(error->y));
	 break;
      case MIRDE_JO_MISSING_CANCEL: 
	 sprintf(dest[0],"missing transaction cancel entry just written");
	 break;

      case MIRDE_JO_TOO_BIG:
	 sprintf(dest[0],"journal file has grown too big, sync it");
	 break;

	 /* resource errors */
      case MIRDE_RESOURCE_MEM: 
	 sprintf(dest[0],"out of memory; needed %ld bytes",
		 error->x);
	 break;
      case MIRDE_RESOURCE_ERR: 
	 sprintf(dest[0],"failed to allocate error; needed %ld bytes",
		 error->x);
	 break;

      case MIRDE_CONFLICT:
	 sprintf(dest[0],"conflict; table id=%lxh (%ld), key=%lxh (%ld)",
		 error->x,error->x,error->y,error->y);
	 break;

      case MIRDE_DEPEND_BROKEN:
	 sprintf(dest[0],"dependency broken; table id=%lxh (%ld), key=%lxh (%ld)",
		 error->x,error->x,error->y,error->y);
	 break;

      case MIRDE_MIRD_TABLE_DEPEND_BROKEN:
	 sprintf(dest[0],"table dependency broken; table id=%lxh (%ld)",
		 error->y,error->y);
	 break;

      case MIRDE_CONFLICT_S:
	 sprintf(dest[0],"conflict; table id=%lxh (%ld), hash key=%lxh (%ld), key len=%ld",
		 error->x,error->x,error->y,error->y,error->z);
	 break;

      case MIRDE_DEPEND_BROKEN_S:
	 sprintf(dest[0],"dependency broken; table id=%lxh (%ld), hash key=%lxh (%ld), key len=%ld",
		 error->x,error->x,error->y,error->y,error->z);
	 break;

      case MIRDE_NO_TABLE:
	 sprintf(dest[0],"no such table; table id %lxh (%ld)",
		 error->x,error->x);
	 break;

      case MIRDE_MIRD_TABLE_EXISTS:
	 sprintf(dest[0],"table already exists; table id %lxh (%ld)",
		 error->x,error->x);
	 break;

      case MIRDE_WRONG_TABLE:
	 sprintf(dest[0],"wrong table type; table id %lxh (%ld) got %lxh expected %lxh",
		 error->x,error->x,error->y,error->z);
	 break;

      case MIRDE_MISSING_JOURNAL:
	 sprintf(dest[0],"database is dirty but no journal file (%s)",
		 error->s);
	 break;

      case MIRDE_READONLY:
	 sprintf(dest[0],"database is read only");
	 break;

      case MIRDE_TR_RUNNING:
	 sprintf(dest[0],"there is ongoing transactions");
	 break;

      case MIRDE_VERIFY_FAILED:
	 sprintf(dest[0],"verification of transaction failed");
	 break;


      default:
	 sprintf(dest[0],"unknown error; no=%d, x=%ld, y=%ld, z=%ld",
		 error->error_no,error->x,error->y,error->z);
   }
}

void mird_perror(char *s,MIRD_RES error)
{
   char *err;
   mird_describe_error(error,&err);
   fprintf(stderr,"%s: %s\n",s,err);
}
