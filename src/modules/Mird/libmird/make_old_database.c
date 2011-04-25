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

#include <stdio.h>
#include "mird.h"

#define TRY(WHAT) 							\
   do {									\
      MIRD_RES res;							\
      res=(WHAT);							\
      if (res)								\
      {									\
	 char *s;							\
	 mird_describe_error(res,&s);					\
	 fprintf(stderr,"fail\n   %s:%d:\n   %s:\n   %s\n\n",		\
                 __FILE__,__LINE__,#WHAT,s);				\
	 mird_free(s);							\
         mird_free_error(res);						\
	 exit(1);							\
      }									\
   } while(0)

int main()
{
   struct mird *db;
   struct mird_transaction *mtr;

   TRY(mird_initialize("old-database.mird",&db));
   db->block_size=128;
   db->frag_bits=2;
   db->hashtrie_bits=2;
   db->flags|=MIRD_EXCL;
   TRY(mird_open(db));
   
   TRY(mird_transaction_new(db,&mtr));

   TRY(mird_key_new_table(mtr,17));
   TRY(mird_s_key_new_table(mtr,18));

   TRY(mird_key_store(mtr,17,1001,"\0\0\3\351",4));
   TRY(mird_key_store(mtr,17,1,"\0\0\0\1",4));

   TRY(mird_s_key_store(mtr,18,"eva",3,"\0\0\0\1",4));
   TRY(mird_s_key_store(mtr,18,"lena",4,"\0\0\3\351",4));
   TRY(mird_s_key_store(mtr,18,"sofia",5,"\0\17F)",4));

   TRY(mird_transaction_close(mtr));
   TRY(mird_close(db));

   return 0;
}
