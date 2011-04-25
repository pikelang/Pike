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

#include "mird.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "dmalloc.h"
#include <setjmp.h>

#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef DO_TRACE
#define TRACE(X) fprintf(stderr,"\n%s   ",X);
#else
#define TRACE(X)
#endif

int mird_check_mem(void);

#define BEGIN_TEST(NAME)						\
   do									\
   {									\
      struct mird *db=NULL;						\
      char dots[80]="..........................."			\
"...................................";					\
      jmp_buf cancel;							\
      if (!setjmp(cancel))						\
      {									\
         dots[59-strlen(NAME)]=0;					\
         fprintf(stderr,"%s%s",NAME,dots);

#define END_TEST()							\
         fprintf(stderr,"ok\n");					\
         test_ok++;							\
         if (mird_check_mem())						\
         {								\
	    fprintf(stderr,"memory leak; cancelling tests\n");		\
	    exit(1);							\
         }								\
         continue;							\
      }									\
      /* error; */							\
      /* we need to close the file(s) */				\
      if (db) mird_free_structure(db);					\
   } while (0)

#define COMPLAIN(REASON)						\
   {									\
      fprintf(stderr,"fail\n   %s:%d:\n   ",__FILE__,__LINE__);		\
      fprintf REASON;							\
      fprintf(stderr,"\n\n");						\
      test_fail++;							\
      longjmp(cancel,1);						\
   }

#define TRY(WHAT) 							\
   do {									\
      MIRD_RES res;							\
      TRACE("try " #WHAT);						\
      res=(WHAT);							\
      if (res)								\
      {									\
	 char *s;							\
	 mird_describe_error(res,&s);					\
	 fprintf(stderr,"fail\n   %s:%d:\n   %s:\n   %s\n\n",		\
                 __FILE__,__LINE__,#WHAT,s);				\
	 mird_free(s);							\
	 mird_free_error(res);						\
         test_fail++;							\
	 longjmp(cancel,1);						\
      }									\
   } while(0)
 
#define FAIL(WHAT,KIND)							\
   do {									\
      MIRD_RES res;							\
      									\
      TRACE("fail: " #WHAT);						\
      res=(WHAT);							\
      if (res)								\
      {									\
	 char *s;							\
	 int kind[]=KIND;						\
	 int i;								\
	 for (i=0; i<sizeof(kind)/sizeof(*kind); i++)			\
	    if (res->error_no==kind[i]) break;				\
	 if (i==sizeof(kind)/sizeof(*kind))				\
	 {								\
            test_fail++;						\
	    mird_describe_error(res,&s);				\
	    fprintf(stderr,"fail\n   %s:%d:\n   %s:\n      %s"		\
		    "\n   expected %s\n",				\
		    __FILE__,__LINE__,#WHAT,s,#KIND);			\
	    mird_free(s);						\
	    mird_free_error(res);					\
  	    longjmp(cancel,1);						\
	 }								\
	 mird_free_error(res);						\
      }									\
      else								\
      {									\
         test_fail++;							\
	 fprintf(stderr,"fail\n   %s:%d:\n   %s:\n      %s"		\
		 "\n   expected %s\n\n",				\
                 __FILE__,__LINE__,#WHAT,"no error",#KIND);		\
         longjmp(cancel,1);						\
      }									\
   } while(0)
     

#define CHAPTER(X) \
 	fprintf(stderr,"\n%s:\n",X);

static volatile int test_ok=0,test_fail=0;

/* ----------------------------------------------------------------------- */

void test1(void)
{
   CHAPTER("empty database tests");

   BEGIN_TEST("create");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_close(db)); db=NULL;
   
   }
   END_TEST();

   BEGIN_TEST("open of old database");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_close(db)); db=NULL;
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_close(db)); db=NULL;
   
   }
   END_TEST();

   BEGIN_TEST("empty transaction");
   {
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;
   
   }
   END_TEST();

   BEGIN_TEST("empty transaction cancelled");
   {
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_cancel(mtr));
      TRY(mird_close(db)); db=NULL;
   
   }
   END_TEST();

   BEGIN_TEST("empty transactions (10)");
   {
      struct mird_transaction *amtr[10];		
      int i;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      for (i=0; i<10; i++) TRY(mird_transaction_new(db,amtr+i));
      for (i=0; i<=5; i++) TRY(mird_transaction_close(amtr[i]));
      for (i=9; i>5; i--) TRY(mird_transaction_close(amtr[i]));
      TRY(mird_close(db)); db=NULL;
   
   }
   END_TEST();

   BEGIN_TEST("empty transactions cancelled (10)");
   {
      struct mird_transaction *amtr[10];		
      int i;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      for (i=0; i<10; i++) TRY(mird_transaction_new(db,amtr+i));
      for (i=0; i<=5; i++) TRY(mird_transaction_cancel(amtr[i]));
      for (i=9; i>5; i--) TRY(mird_transaction_cancel(amtr[i]));
      TRY(mird_close(db)); db=NULL;
   
   }
   END_TEST();

   BEGIN_TEST("empty transaction closed twice");
   {
      struct mird_transaction *mtr=NULL;				
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_tr_resolve(mtr));
      TRY(mird_tr_finish(mtr));
      FAIL(mird_transaction_close(mtr),{MIRDE_TR_CLOSED});
      mird_tr_free(mtr);
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();


   BEGIN_TEST("empty transaction closed and then cancelled");
   {
      struct mird_transaction *mtr=NULL;				
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_tr_resolve(mtr));
      TRY(mird_tr_finish(mtr));
      FAIL(mird_transaction_cancel(mtr),{MIRDE_TR_CLOSED});
      mird_tr_free(mtr);
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("empty transaction cancelled and then closed");
   {
      struct mird_transaction *mtr=NULL;				
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_tr_rewind(mtr));
      FAIL(mird_transaction_close(mtr),{MIRDE_TR_CLOSED});
      mird_tr_free(mtr);
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("empty transaction cancelled twice");
   {
      struct mird_transaction *mtr=NULL;				
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_tr_rewind(mtr));
      FAIL(mird_transaction_cancel(mtr),{MIRDE_TR_CLOSED});
      mird_tr_free(mtr);
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("forgotten transactions");
   {
      struct mird_transaction *mtr=NULL;				
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_close(db)); db=NULL;
      if (mtr->db) COMPLAIN((stderr,"mtr->db not NULL but %p",mtr->db));
      mird_tr_free(mtr);
   }
   END_TEST();

}

/* ----------------------------------------------------------------------- */

void test2(void)
{
   CHAPTER("low level storage tests");

   BEGIN_TEST("creating tables");
   {
      struct mird_transaction *mtr=NULL;				
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("reading from non-existing table");
   {
      unsigned char *data;
      mird_size_t len;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      FAIL(mird_key_lookup(db,17,42,&data,&len),
	   {MIRDE_NO_TABLE});
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("create and read from empty table");
   {
      struct mird_transaction *mtr=NULL;				
      unsigned char *data;
      mird_size_t len;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_key_lookup(db,17,42,&data,&len));
      if (data) COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("create, close and read from empty table");
   {
      struct mird_transaction *mtr=NULL;				
      unsigned char *data;
      mird_size_t len;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_key_lookup(db,17,42,&data,&len));
      if (data) COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();
      
   BEGIN_TEST("create, write and read from table");
   {
      struct mird_transaction *mtr=NULL;				
      unsigned char *data;
      mird_size_t len;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
      TRY(mird_transaction_key_lookup(mtr,17,42,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      mird_free(data);
      TRY(mird_transaction_close(mtr));

      TRY(mird_key_lookup(db,17,42,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      mird_free(data);
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("removing tables");
   {
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_delete_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr));
      FAIL(mird_key_store(mtr,17,42,"hej",strlen("hej")),
           {MIRDE_NO_TABLE});
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("removing tables 2");
   {
      struct mird_transaction *mtr=NULL;				
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17|0x1000));
      TRY(mird_key_new_table(mtr,17|0x0100));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_delete_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr));
      FAIL(mird_key_store(mtr,17,42,"hej",strlen("hej")),
	   {MIRDE_NO_TABLE});
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;
   
   }
   END_TEST();

   BEGIN_TEST("single keys of different sizes");
   {
      mird_size_t i,j;
      char loop[]="-/|\\";
      unsigned char *buf,*data;
      mird_size_t len;
      struct mird_transaction *mtr=NULL;				

      buf=malloc(200000);
      if (!buf) COMPLAIN((stderr,"out of memory"));
      for (i=0; i<200000; i++)
	 buf[i]=(unsigned char)i;
      for (i=j=0; i<200000; i=(i*5)/3+1,j=(j+1)%4)
      {
	 fprintf(stderr,"%c\010",loop[j]);

	 unlink("test.mird");
	 TRY(mird_initialize("test.mird",&db));
	 TRY(mird_open(db));
   
	 TRY(mird_transaction_new(db,&mtr));
	 TRY(mird_key_new_table(mtr,17));
	 TRY(mird_key_store(mtr,17,42,buf,i));

	 TRY(mird_transaction_key_lookup(mtr,17,42,&data,&len));
	 if (!data || len!=i || memcmp(data,buf,i))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",data,len,i));

	 mird_free(data);
	 TRY(mird_transaction_close(mtr));

	 TRY(mird_key_lookup(db,17,42,&data,&len));
	 if (!data || len!=i || memcmp(data,buf,i))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",data,len,i));

	 mird_free(data);
	 TRY(mird_close(db)); db=NULL;

	 TRY(mird_initialize("test.mird",&db));
	 TRY(mird_open(db));
   
	 TRY(mird_key_lookup(db,17,42,&data,&len));
	 if (!data || len!=i || memcmp(data,buf,i))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",data,len,i));
	 mird_free(data);

	 TRY(mird_close(db)); db=NULL;
      }
   }
   END_TEST();

   BEGIN_TEST("keys of size 0..2048");
   {
      mird_size_t i;
      unsigned char *buf,*data;
      mird_size_t len;
      struct mird_transaction *mtr=NULL;				

      buf=malloc(2048);
      if (!buf) COMPLAIN((stderr,"out of memory"));

      for (i=0; i<2048; i++)
	 buf[i]=(unsigned char)i;

      fprintf(stderr,"-\010");

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));

      for (i=0; i<2048; i++)
	 TRY(mird_key_store(mtr,17,i,buf,i));

      fprintf(stderr,"/\010");

      for (i=0; i<2048; i++)
      {
	 TRY(mird_transaction_key_lookup(mtr,17,i,&data,&len));
	 if (!data || len!=i || memcmp(data,buf,i))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",data,len,i));
	 mird_free(data);
      }

      fprintf(stderr,"|\010");

      TRY(mird_transaction_close(mtr));

      for (i=0; i<2048; i++)
      {
	 TRY(mird_key_lookup(db,17,i,&data,&len));
	 if (!data || len!=i || memcmp(data,buf,i))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",data,len,i));
	 mird_free(data);
      }

      fprintf(stderr,"\\\010");

      TRY(mird_close(db)); db=NULL;

      fprintf(stderr,"-\010");

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      for (i=0; i<2048; i++)
      {
	 TRY(mird_key_lookup(db,17,i,&data,&len));
	 if (!data || len!=i || memcmp(data,buf,i))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",data,len,i));
	 mird_free(data);
      }

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();


   BEGIN_TEST("storing int key in string table (should fail)");
   {
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_s_key_new_table(mtr,17));
      TRY(mird_s_key_store(mtr,17,"42",2,"test data",strlen("test data")));
      FAIL(mird_key_store(mtr,17,42,"hej",3),
	   {MIRDE_WRONG_TABLE});
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_s_key_store(mtr,17,"42",2,"test data",strlen("test data")));
      FAIL(mird_key_store(mtr,17,42,"hej",3),
	   {MIRDE_WRONG_TABLE});
      TRY(mird_transaction_cancel(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("storing string key in int table (should fail)");
   {
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"hej",3));
      FAIL(mird_s_key_store(mtr,17,"42",2,"test data",strlen("test data")),
	   {MIRDE_WRONG_TABLE});
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_store(mtr,17,42,"hej",3));
      FAIL(mird_s_key_store(mtr,17,"42",2,"test data",strlen("test data")),
	   {MIRDE_WRONG_TABLE});
      TRY(mird_transaction_cancel(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("looking up int key in string table (should fail)");
   {
      struct mird_transaction *mtr=NULL;				
      unsigned char *data;
      mird_size_t len;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_s_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));

      TRY(mird_s_key_lookup(db,17,"42",2,&data,&len));
      if (data) COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      FAIL(mird_key_lookup(db,17,42,&data,&len),
           {MIRDE_WRONG_TABLE});

      TRY(mird_transaction_new(db,&mtr));

      TRY(mird_transaction_s_key_lookup(mtr,17,"42",2,&data,&len));
      if (data) COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      FAIL(mird_transaction_key_lookup(mtr,17,42,&data,&len),
           {MIRDE_WRONG_TABLE});
      TRY(mird_transaction_cancel(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("looking up string key in int table (should fail)");
   {
      struct mird_transaction *mtr=NULL;				
      unsigned char *data;
      mird_size_t len;

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));

      TRY(mird_key_lookup(db,17,42,&data,&len));
      if (data) COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      FAIL(mird_s_key_lookup(db,17,"42",2,&data,&len),
           {MIRDE_WRONG_TABLE});

      TRY(mird_transaction_new(db,&mtr));

      TRY(mird_transaction_key_lookup(mtr,17,42,&data,&len));
      if (data) COMPLAIN((stderr,"illegal result data=%p len=%lu",data,len));
      FAIL(mird_transaction_s_key_lookup(mtr,17,"42",2,&data,&len),
           {MIRDE_WRONG_TABLE});
      TRY(mird_transaction_cancel(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

}

/* ----------------------------------------------------------------------- */

void test3(void)
{
   CHAPTER("recover (journal file readback) tests");

   BEGIN_TEST("lost journal recover");
   {
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_s_key_new_table(mtr,17));
      TRY(mird_s_key_store(mtr,17,"42",2,"test data",strlen("test data")));
      TRY(mird_transaction_close(mtr));
      unlink("test.mird.journal");
      mird_free_structure(db); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("lost journal no recover (MIRD_JO_COMPLAIN)");
   {
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_s_key_new_table(mtr,17));
      TRY(mird_s_key_store(mtr,17,"42",2,"test data",strlen("test data")));
      TRY(mird_transaction_close(mtr));
      unlink("test.mird.journal");
      mird_free_structure(db); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_JO_COMPLAIN;
      FAIL(mird_open(db),{MIRDE_MISSING_JOURNAL});
      mird_free_structure(db); db=NULL;

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_s_key_new_table(mtr,17));
      TRY(mird_s_key_store(mtr,17,"42",2,"test data",strlen("test data")));
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("serial transactions 1");
   {
      mird_size_t i;
      unsigned char *data;
      mird_size_t len;
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_new_table(mtr,18));
      TRY(mird_transaction_close(mtr));

      for (i=0; i<20; i++)
      {
	 TRY(mird_transaction_new(db,&mtr));
	 TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
	 TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
	 TRY(mird_transaction_close(mtr));
      }
      mird_free_structure(db); db=NULL;
      
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));   
      for (i=0; i<20; i++)
      {
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (!data || len!=strlen("test data") ||
	     memcmp(data,"test data",strlen("test data")))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
		      data,len,i));
	 mird_free(data);
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (!data || len!=strlen("test data") ||
	     memcmp(data,"test data",strlen("test data")))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
		      data,len,i));
	 mird_free(data);
      }
      TRY(mird_close(db)); db=NULL;
   }   
   END_TEST();

   BEGIN_TEST("parallell transactions 1");
   {
      struct mird_transaction *amtr[20];		
      unsigned char *data;
      mird_size_t len,i;
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_new_table(mtr,18));
      TRY(mird_transaction_close(mtr));

      for (i=0; i<20; i++) 
	 TRY(mird_transaction_new(db,amtr+i));

      for (i=0; i<20; i++)
      {
	 TRY(mird_key_store(amtr[i],17,i*4711,
			    "test data",strlen("test data")));
	 TRY(mird_key_store(amtr[i],18,i*13,
			    "test data",strlen("test data")));
      }

      for (i=0; i<20; i++) 
	 TRY(mird_transaction_close(amtr[i]));

      mird_free_structure(db); db=NULL;
      
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));   
      for (i=0; i<20; i++)
      {
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (!data || len!=strlen("test data") ||
	     memcmp(data,"test data",strlen("test data")))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
		      data,len,i));
	 mird_free(data);
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (!data || len!=strlen("test data") ||
	     memcmp(data,"test data",strlen("test data")))
	    COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
		      data,len,i));
	 mird_free(data);
      }
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("serial transactions 2");
   {
      mird_size_t i;
      unsigned char *data;
      mird_size_t len;
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_new_table(mtr,18));
      TRY(mird_transaction_close(mtr));

      for (i=0; i<20; i++)
      {
	 TRY(mird_transaction_new(db,&mtr));
	 TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
	 TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
	 if (i&1) 
	    TRY(mird_transaction_close(mtr));
	 else
	    TRY(mird_transaction_cancel(mtr));
      }
      mird_free_structure(db); db=NULL;
      
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));   
      for (i=0; i<20; i++)
      {
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (i&1)
	 {
	    if (!data || len!=strlen("test data") ||
		memcmp(data,"test data",strlen("test data")))
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	    mird_free(data);
	 }
	 else
	 {
	    if (data)
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	 }
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (i&1)
	 {
	    if (!data || len!=strlen("test data") ||
		memcmp(data,"test data",strlen("test data")))
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	    mird_free(data);
	 }
	 else
	 {
	    if (data)
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	 }
      }
      TRY(mird_close(db)); db=NULL;
   }   
   END_TEST();

   BEGIN_TEST("parallell transactions 2");
   {
      struct mird_transaction *amtr[20];		
      unsigned char *data;
      mird_size_t len,i;
      struct mird_transaction *mtr=NULL;				

      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_new_table(mtr,18));
      TRY(mird_transaction_close(mtr));

      for (i=0; i<20; i++) 
	 TRY(mird_transaction_new(db,amtr+i));

      for (i=0; i<20; i++)
      {
	 TRY(mird_key_store(amtr[i],17,i*4711,
			    "test data",strlen("test data")));
	 TRY(mird_key_store(amtr[i],18,i*13,
			    "test data",strlen("test data")));
      }

      for (i=0; i<20; i++) 
	 if (i&1) 
	    TRY(mird_transaction_close(amtr[i]));
	 else
	    TRY(mird_transaction_cancel(amtr[i]));

      mird_free_structure(db); db=NULL;
      
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));   
      for (i=0; i<20; i++)
      {
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (i&1)
	 {
	    if (!data || len!=strlen("test data") ||
		memcmp(data,"test data",strlen("test data")))
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	    mird_free(data);
	 }
	 else
	 {
	    if (data)
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	 }
	 TRY(mird_key_lookup(db,17,i*4711,&data,&len));
	 if (i&1)
	 {
	    if (!data || len!=strlen("test data") ||
		memcmp(data,"test data",strlen("test data")))
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	    mird_free(data);
	 }
	 else
	 {
	    if (data)
	       COMPLAIN((stderr,"illegal result data=%p len=%lu i=%lu",
			 data,len,i));
	 }
      }
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

}

/* ----------------------------------------------------------------------- */

void test4(void)
{
   struct mird_transaction *mtr=NULL; 
   struct mird_transaction *mtr1=NULL; 
   struct mird_transaction *mtr2=NULL; 

   CHAPTER("conflicts tests");
   
   BEGIN_TEST("same key (both new)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same key (change)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"first value",strlen("first value")));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same key (first delete)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"first value",strlen("first value")));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr1,17,42,NULL,0));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same key (second delete)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"first value",strlen("first value")));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_key_store(mtr2,17,42,NULL,0));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same key (both delete)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"first value",strlen("first value")));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr1,17,42,NULL,0));
      TRY(mird_key_store(mtr2,17,42,NULL,0));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

#if 0
   /* this test fails, and there is no plans to fix it */
   BEGIN_TEST("same key (both new + delete)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr1,17,42,"first value",strlen("first value")));
      TRY(mird_key_store(mtr2,17,42,"first value",strlen("first value")));
      TRY(mird_key_store(mtr1,17,42,NULL,0));
      TRY(mird_key_store(mtr2,17,42,NULL,0));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();
#endif

   BEGIN_TEST("same key (both new, delete, create in first)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_key_store(mtr1,17,42,NULL,0));
      TRY(mird_key_store(mtr2,17,42,NULL,0));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same table (both new)");
   {
      struct mird_transaction *mtr1=NULL; 
      struct mird_transaction *mtr2=NULL; 
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same table (first delete, second delete + new)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same table (second delete, first delete + new)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same table (both delete)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same table (both new, delete)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same table (both new, delete, new)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("same table (both new, delete, second +n+d)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_transaction_close(mtr1));
      FAIL(mird_transaction_close(mtr2),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr2));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("if conflicts stays");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_CONFLICT});
      FAIL(mird_transaction_close(mtr1),{MIRDE_CONFLICT});
      FAIL(mird_transaction_close(mtr1),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("if conflicts stays after resolve()");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_tr_resolve(mtr1),{MIRDE_CONFLICT});
      FAIL(mird_tr_resolve(mtr1),{MIRDE_CONFLICT});
      FAIL(mird_transaction_close(mtr1),{MIRDE_CONFLICT});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();
}

void test5(void)
{
   struct mird_transaction *mtr=NULL; 
   struct mird_transaction *mtr1=NULL; 
   struct mird_transaction *mtr2=NULL; 

   BEGIN_TEST("table write / delete");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));

      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("table new + write / new + delete");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("table new + delete / new + write ");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("table new + write + delete / new + delete");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("table new + delete / new + write + delete");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_new_table(mtr2,17));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_delete_table(mtr2,17));
      TRY(mird_key_new_table(mtr1,17));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("table depend / write");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_depend_table(mtr1,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("table depend / depend");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_depend_table(mtr2,17));
      TRY(mird_depend_table(mtr1,17));
      TRY(mird_transaction_close(mtr2));
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();


   BEGIN_TEST("table write / depend");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr1,17,42,"value",strlen("value")));
      TRY(mird_depend_table(mtr2,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("table delete / write");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_transaction_close(mtr));
      TRY(mird_transaction_new(db,&mtr1));
      TRY(mird_transaction_new(db,&mtr2));
      TRY(mird_key_store(mtr2,17,42,"value",strlen("value")));
      TRY(mird_delete_table(mtr1,17));
      TRY(mird_transaction_close(mtr2));
      FAIL(mird_transaction_close(mtr1),{MIRDE_MIRD_TABLE_DEPEND_BROKEN});
      TRY(mird_transaction_cancel(mtr1));

      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();
}

/* ----------------------------------------------------------------------- */

void test6(void)
{
   struct mird_transaction *mtr=NULL; 
   int i;

   CHAPTER("usage tests");

   BEGIN_TEST("cancelled parallell transactions");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
      TRY(mird_key_new_table(mtr,18));
      TRY(mird_key_store(mtr,18,42,"test data",strlen("test data")));
      TRY(mird_transaction_cancel(mtr));
      TRY(mird_sync(db));
      mird_debug_check_free(db,1);
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("cancelled serial transactions");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      for (i=0; i<10; i++)
      {
	 TRY(mird_transaction_new(db,&mtr));
	 TRY(mird_key_new_table(mtr,17));
	 TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
	 TRY(mird_key_new_table(mtr,18));
	 TRY(mird_key_store(mtr,18,42,"test data",strlen("test data")));
	 TRY(mird_transaction_cancel(mtr));
      }
      TRY(mird_sync(db));
      mird_debug_check_free(db,1);
      TRY(mird_close(db)); db=NULL;
   }  
   END_TEST();

#if 0

   BEGIN_TEST("closed serial transactions");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   int i;

   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));

   for (i=0; i<10; i++)
   {
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,42,"test data",strlen("test data")));
      TRY(mird_transaction_close(mtr));
   }
   for (i=0; i<10; i++)
   {
      TRY(mird_transaction_new(db,&mtr));
      (i&1)?TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
	 :TRY(mird_key_store(mtr,18,42,"test data",strlen("test data")));
      TRY(mird_transaction_close(mtr));
   }
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();
   
   BEGIN_TEST("closed parallel transactions (conflicts)");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   int i;

   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));

   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
   TRY(mird_key_store(mtr,18,42,"test data",strlen("test data")));

   for (i=0; i<10; i++)
      TRY(mird_transaction_close(catch { mtr[i]));
   TRY(mird_transaction_desmtroy(mtr));

   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();
#endif 
}

#if 0
   
   /* -------------------------------------------------------------------- */

   CHAPTER("usage and recover tests");
   BEGIN_TEST("serial transactions 2");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
   for (i=0; i<20; i++)
   {
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
      if (i&1) TRY(mird_transaction_close(mtr)); else TRY(mird_transaction_cancel(mtr));
   }
   TRY(mird__debug_cut(db));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();

   BEGIN_TEST("parallell transactions 2");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
   TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr[i],17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr[i],18,i*13,"test data",strlen("test data")))1000);
   }
   for (i=0; i<20; i++)
      TRY(mird_transaction_close(if (i&1) mtr[i]));
   TRY(mird__debug_cut(db));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();

   BEGIN_TEST("desmtruction of tables");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   int i;
   for (i=0; i<2000; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }
   TRY(mird_transaction_close(mtr));
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_transaction_delete_table(mtr)17);
   TRY(mird_transaction_close(mtr));
   TRY(mird__debug_cut(db));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_transaction_delete_table(mtr)18);
   TRY(mird_transaction_close(mtr));
   TRY(mird__debug_cut(db));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();

   BEGIN_TEST("table conflicts 1");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   

TRY(mird_transaction_new(db,&mtr));

   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   int i;
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }

   for (i=0; i<10; i++)
      TRY(mird_transaction_close(catch { mtr[i]));
   TRY(mird_transaction_desmtroy(mtr));

   TRY(mird__debug_cut(db));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();

   BEGIN_TEST("table conflicts 2");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   

   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   int i,j;
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }
   TRY(mird_transaction_close(mtr));

TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<24; i++)
   {
      for (j=0; j<20; j++)
      {
	 TRY(mird_key_store(mtr[i],17,j,"test data",strlen("test data")));
	 TRY(mird_key_store(mtr[i],18,j,"test data",strlen("test data")));
      }
      mtTRY(mird_delete_table(r[i],17));
      mtTRY(mird_key_store(r[i],18,NULL,0));
      if (!(i%2))
      {
	 mtTRY(mird_key_new_table(r[i],17));
	 for (j=0; j<20; j++)
	    TRY(mird_key_store(mtr[i],17,j*13,"test data",strlen("test data")));
      }
      if (!(i%3))
      {
	 mtTRY(mird_key_new_table(r[i],18));
	 for (j=0; j<20; j++)
	    TRY(mird_key_store(mtr[i],18,j*4711,"test data",strlen("test data")));
      }
      if (!(i%12))
      {
	 mtTRY(mird_delete_table(r[i],17));
	 mtTRY(mird_key_store(r[i],18,NULL,0));
      }
      if (!i)
      {
	 mtTRY(mird_key_new_table(r[i],17));
	 mtTRY(mird_key_new_table(r[i],18));
      }
      catch 
      {
	 for (j=0; j<20; j++)
	 {
	    TRY(mird_key_store(mtr[i],17,j*4711,"test data",strlen("test data")));
	    TRY(mird_key_store(mtr[i],18,j*13,"test data",strlen("test data")));
	 }
      };
   }

   for (i=0; i<10; i++)
   TRY(mird_transaction_close(catch { mtr[i]));
   TRY(mird_transaction_desmtroy(mtr));

   TRY(mird__debug_cut(db));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();

   BEGIN_TEST("table conflicts 2 / broken journal");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   

   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   int i,j;
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }
   TRY(mird_transaction_close(mtr));

TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<24; i++)
   {
      for (j=0; j<20; j++)
      {
	 TRY(mird_key_store(mtr[i],17,j,"test data",strlen("test data")));
	 TRY(mird_key_store(mtr[i],18,j,"test data",strlen("test data")));
      }
      mtTRY(mird_delete_table(r[i],17));
      mtTRY(mird_key_store(r[i],18,NULL,0));
      if (!(i%2))
      {
	 mtTRY(mird_key_new_table(r[i],17));
	 for (j=0; j<20; j++)
	    TRY(mird_key_store(mtr[i],17,j*13,"test data",strlen("test data")));
      }
      if (!(i%3))
      {
	 mtTRY(mird_key_new_table(r[i],18));
	 for (j=0; j<20; j++)
	    TRY(mird_key_store(mtr[i],18,j*4711,"test data",strlen("test data")));
      }
      if (!(i%12))
      {
	 mtTRY(mird_delete_table(r[i],17));
	 mtTRY(mird_key_store(r[i],18,NULL,0));
      }
      if (!i)
      {
	 mtTRY(mird_key_new_table(r[i],17));
	 mtTRY(mird_key_new_table(r[i],18));
      }
      catch 
      {
	 for (j=0; j<20; j++)
	 {
	    TRY(mird_key_store(mtr[i],17,j*4711,"test data",strlen("test data")));
	    TRY(mird_key_store(mtr[i],18,j*13,"test data",strlen("test data")));
	 }
      };
   }

   for (i=0; i<10; i++)
   TRY(mird_transaction_close(catch { mtr[i]));
   TRY(mird_transaction_desmtroy(mtr));

   TRY(mird__debug_cut(db));
   object f=Stdio.File("test.mird.journal","wr");
   f->mtruncate(40960);
   f->seek(40960);
   TRY(mird_transaction_close(f));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL
   
   END_TEST();


   BEGIN_TEST("parallel transactions / broken database file");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   

   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr[i],17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr[i],18,i*13,"test data",strlen("test data")))1000);
   }
   for (i=0; i<20; i++)
   TRY(mird_transaction_close(if (i&1) mtr[i]));

   TRY(mird__debug_cut(db));
   object f=Stdio.File("test.mird","wr");
   /*
     f->seek(8192);
     f->write("blaha blaha");
   */
   for (i=0; i<100; i++)
   {
      f->seek(10000+i*1000); 
      f->write("blaha blaha");
   }
   TRY(mird_transaction_close(f));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db)); db=NULL;
   
   END_TEST();

   BEGIN_TEST("cancelled transactions / cut journal file");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   

   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
   TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<2000; i++)
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
   TRY(mird_transaction_cancel(mtr));
   TRY(mird__debug_cut(db));
   object f=Stdio.File("test.mird.journal","wr");
   f->mtruncate(Stdio.file_size("test.mird.journal")-70);
   TRY(mird_transaction_close(f));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db));
   
   END_TEST();


   BEGIN_TEST("litter in journal file");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
   TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }
   TRY(mird_transaction_close(mtr));
   TRY(mird__debug_cut(db));
   object f=Stdio.File("test.mird.journal","wr");
   smtring s=f->read(0x7ffffff);
   s=s[..24*6-1]+replace(s[24*6..],"allo","ltmtr");
   f->seek(0);
   f->write(s);
   TRY(mird_transaction_close(f));
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db));
   
   END_TEST();

   BEGIN_TEST("litter in journal file 2");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
   TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }
   TRY(mird_transaction_close(mtr));
   TRY(mird__debug_cut(db));
   object f=Stdio.File("test.mird.journal","wr");
   smtring s=f->read(0x7ffffff);
   s=s[..24*6-1]+replace(s[24*6..],"free","ltmtr");
   f->seek(0);
   f->write(s);
   f->close();
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db));
   
   END_TEST();

   BEGIN_TEST("litter in journal file 3");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
   TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }
   TRY(mird_transaction_cancel(mtr));
   TRY(mird__debug_cut(db));
   object f=Stdio.File("test.mird.journal","wr");
   smtring s=f->read(0x7ffffff);
   s=s[..24*6-1]+replace(s[24*6..],"allo","ltmtr");
   f->seek(0);
   f->write(s);
   f->close();
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db));
   
   END_TEST();

   BEGIN_TEST("litter in journal file 4");

   unlink("test.mird");
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));
   TRY(mird_key_new_table(mtr,17));
   TRY(mird_key_new_table(mtr,18));
   TRY(mird_transaction_close(mtr));
   int i;
   TRY(mird_transaction_new(db,&mtr));
   for (i=0; i<20; i++)
   {
      TRY(mird_key_store(mtr,17,i*4711,"test data",strlen("test data")));
      TRY(mird_key_store(mtr,18,i*13,"test data",strlen("test data")));
   }
   TRY(mird_transaction_cancel(mtr));
   TRY(mird__debug_cut(db));
   object f=Stdio.File("test.mird.journal","wr");
   smtring s=f->read(0x7ffffff);
   s=s[..24*6-1]+replace(s[24*6..],"free","ltmtr");
   f->seek(0);
   f->write(s);
   f->close();
   TRY(mird_initialize("test.mird",&db));
   TRY(mird_open(db));
   TRY(mird__debug_check_free(db)1);
   TRY(mird_close(db));
   
   END_TEST();

#endif

   /* --------------------------------------------------------------------- */

void test7(void)
{
   struct mird_transaction *mtr=NULL; 
   unsigned char *data;
   mird_size_t len;

   CHAPTER("database flag tests");

   BEGIN_TEST("read-only database");
   {
/* create database to test */
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
   
      TRY(mird_transaction_new(db,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
      TRY(mird_s_key_new_table(mtr,18));
      TRY(mird_s_key_store(mtr,18,"42",2,"test data",strlen("test data")));
      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;

/* check if read-only is possible */
      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_READONLY|MIRD_LIVE;
      TRY(mird_open(db));
      
      TRY(mird_key_lookup(db,17,42,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:42",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"42",2,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:42",
		   data,len));
      mird_free(data);

      TRY(mird_transaction_new(db,&mtr));

      TRY(mird_transaction_key_lookup(mtr,17,42,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:42",
		   data,len));
      mird_free(data);

      TRY(mird_transaction_s_key_lookup(mtr,18,"42",2,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:42",
		   data,len));
      mird_free(data);
      
      FAIL(mird_key_new_table(mtr,19),{MIRDE_READONLY});
      FAIL(mird_key_new_table(mtr,17),{MIRDE_READONLY});
      FAIL(mird_key_store(mtr,17,42,"test data",strlen("test data")),
	   {MIRDE_READONLY});
      FAIL(mird_key_store(mtr,17,43,"test data",strlen("test data")),
	   {MIRDE_READONLY});
      FAIL(mird_s_key_new_table(mtr,18),{MIRDE_READONLY});
      FAIL(mird_s_key_store(mtr,18,"42",2,"test data",strlen("test data")),
	   {MIRDE_READONLY});
      FAIL(mird_s_key_store(mtr,18,"43",2,"test data",strlen("test data")),
	   {MIRDE_READONLY});
      FAIL(mird_key_store(mtr,17,42,"qwpoek",strlen("qwpoek")),
	   {MIRDE_READONLY});
      FAIL(mird_s_key_store(mtr,18,"42",2,"qpwoek",strlen("qpwoek")),
	   {MIRDE_READONLY});
      FAIL(mird_key_store(mtr,17,42,NULL,0),
	   {MIRDE_READONLY});
      FAIL(mird_s_key_store(mtr,18,"42",2,NULL,0),
	   {MIRDE_READONLY});

      TRY(mird_transaction_close(mtr));
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("read-many write-one");
   {
      struct mird *dbw;

/* create database to test */
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&dbw));
      TRY(mird_open(dbw));
   
      TRY(mird_transaction_new(dbw,&mtr));
      TRY(mird_key_new_table(mtr,17));
      TRY(mird_s_key_new_table(mtr,18));
      TRY(mird_transaction_close(mtr));

/* open read-only database */
      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_READONLY|MIRD_LIVE;
      TRY(mird_open(db));

      TRY(mird_transaction_new(dbw,&mtr));
      TRY(mird_key_store(mtr,17,42,"test data",strlen("test data")));
      TRY(mird_s_key_store(mtr,18,"42",2,"test data",strlen("test data")));
      TRY(mird_transaction_close(mtr));

      TRY(mird_key_lookup(db,17,42,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:42 1st",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"42",2,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"test data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 18:\"42\" 1st",
		   data,len));
      mird_free(data);

      TRY(mird_transaction_new(dbw,&mtr));
      TRY(mird_key_store(mtr,17,43,"more data",strlen("test data")));
      TRY(mird_s_key_store(mtr,18,"43",2,"more data",strlen("test data")));
      TRY(mird_transaction_close(mtr));

      TRY(mird_key_lookup(db,17,43,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"more data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:43 2nd",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"43",2,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"more data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 18:\"43\" 2nd",
		   data,len));
      mird_free(data);

      TRY(mird_transaction_new(dbw,&mtr));
      TRY(mird_key_store(mtr,17,42,"some data",strlen("test data")));
      TRY(mird_s_key_store(mtr,18,"42",2,"some data",strlen("test data")));
      TRY(mird_transaction_close(mtr));

      TRY(mird_sync(dbw));

      TRY(mird_transaction_new(dbw,&mtr));
      TRY(mird_key_store(mtr,17,44,"extradata",strlen("test data")));
      TRY(mird_s_key_store(mtr,18,"44",2,"extradata",strlen("test data")));
      TRY(mird_transaction_close(mtr));

      TRY(mird_key_lookup(db,17,42,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"some data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:42 3rd",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"42",2,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"some data",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 18:\"42\" 3rd",
		   data,len));
      mird_free(data);

      TRY(mird_key_lookup(db,17,44,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"extradata",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 17:42 4th",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"44",2,&data,&len));
      if (!data || len!=strlen("test data") ||
	  memcmp(data,"extradata",strlen("test data")))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu 18:\"42\" 4th",
		   data,len));
      mird_free(data);

      TRY(mird_close(dbw)); dbw=NULL;
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("create read-only database (should fail)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_READONLY;
      FAIL(mird_open(db),{MIRDE_OPEN});
      mird_free_structure(db); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("create no-create marked database (should fail)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_NOCREATE;
      FAIL(mird_open(db),{MIRDE_OPEN});
      mird_free_structure(db); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("open no-create marked existing database");
   {
      unlink("test.mird");

      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_close(db));

      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_NOCREATE;
      TRY(mird_open(db));
      TRY(mird_close(db));
   }
   END_TEST();

   BEGIN_TEST("create existing exclusive database (should fail)");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      TRY(mird_close(db));
      
      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_EXCL;
      FAIL(mird_open(db),{MIRDE_OPEN_CREATE});
      mird_free_structure(db); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("create non-existant exclusive database");
   {
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      db->flags|=MIRD_EXCL;
      TRY(mird_open(db));
      TRY(mird_close(db));
   }
   END_TEST();

#if 0
   BEGIN_TEST("two open on the same file");
   {
      int pid;
      struct mird *db2;
      unlink("test.mird");
      TRY(mird_initialize("test.mird",&db));
      TRY(mird_open(db));
      if (!(pid=fork()))
      {
	 fprintf(stderr,"forked\n");
	 TRY(mird_initialize("test.mird",&db2));
	 FAIL(mird_open(db2),{MIRDE_OPEN});
	 _exit(0);
      }
      waitpid(pid,&pid,0);
      TRY(mird_close(db));
   }
   END_TEST();
#endif
}

/* ----------------------------------------------------------------------- */

void testa(void)
{
   CHAPTER("reading old database");

   BEGIN_TEST("open");
   {
      TRY(mird_initialize("old-database.mird",&db));
      db->flags|=MIRD_READONLY;
      TRY(mird_open(db));
   
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();

   BEGIN_TEST("values");
   {
      unsigned char *data;
      mird_size_t len;

      TRY(mird_initialize("old-database.mird",&db));
      db->flags|=MIRD_READONLY;
      TRY(mird_open(db));

      TRY(mird_key_lookup(db,17,1,&data,&len));
      if (!data || len!=4 ||
	  memcmp(data,"\0\0\0\1",4))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu from 17:1",
		   data,len));
      mird_free(data);

      TRY(mird_key_lookup(db,17,1001,&data,&len));
      if (!data || len!=4 ||
	  memcmp(data,"\0\0\3\351",4))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu from 17:1001",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"lena",4,&data,&len));
      if (!data || len!=4 ||
	  memcmp(data,"\0\0\3\351",4))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu from 18:\"lena\"",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"eva",3,&data,&len));
      if (!data || len!=4 ||
	  memcmp(data,"\0\0\0\1",4))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu from 18:\"eva\"",
		   data,len));
      mird_free(data);

      TRY(mird_s_key_lookup(db,18,"sofia",5,&data,&len));
      if (!data || len!=4 ||
	  memcmp(data,"\0\17F)",4))
	 COMPLAIN((stderr,"illegal result data=%p len=%lu from 18:\"sofia\"",
		   data,len));
      mird_free(data);
   
      TRY(mird_close(db)); db=NULL;
   }
   END_TEST();
}

int main()
{
   test1();
   test2();
   test7();
   test3();
   test4();
   test5();
   test6();

   testa();

   fprintf(stderr,"\ntotal: %d tests",test_ok+test_fail);
   if (!test_fail) fprintf(stderr,", all ok\n");
   else fprintf(stderr,", %d ok, %d failed\n",test_ok,test_fail);

   return 0;
}
