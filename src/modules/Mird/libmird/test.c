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
#include <stdlib.h>

#include "mird.h"

void err(char *when,MIRD_RES res)
{
   mird_perror(when,res);
   mird_free_error(res);
   exit(1);
}

#define TRY(S,X) do { MIRD_RES res; if ( (res=(X)) ) err(S,res); } while (0)
#define WARN(S,X) do { MIRD_RES res; if ( (res=(X)) ) { mird_perror(S,res); mird_free_error(res); } } while (0)

#define Z 9

int main()
{
   struct mird *db;
   struct mird_transaction *tr[Z];
   unsigned long i,l;

   unsigned char *data,*re;

   remove("test.mird");

   TRY("init",mird_initialize("test.mird",&db));

/*    db->block_size=2048; */
/*    db->frag_bits=4; */
/*    db->hashtrie_bits=4; */
   db->cache_size=64; 

   re=malloc(20000);

   TRY("open",mird_open(db));
   TRY("newt",mird_transaction_new(db,tr+0));
   TRY("newtbl",mird_key_new_table(tr[0],17));
   TRY("newtbl",mird_key_new_table(tr[0],18));
   TRY("endt",mird_transaction_close(tr[0]));
   
   for (i=0; i<Z; i++)
   {
      TRY("newt2",mird_transaction_new(db,tr+i));
#define PARALLELL
#ifdef PARALLELL
   }
   for (i=0; i<Z; i++)
   {
#endif
      TRY("stor",mird_key_store(tr[i],17,i*4711,"test data",9));
      TRY("stor",mird_key_store(tr[i],18,i*13,re,9000));
#ifdef PARALLELL
   }

   for (i=0; i<Z; i++)
   {
#endif
      if (i&1)
	 TRY("clos",mird_transaction_close(tr[i]));
      else
	 TRY("canc",mird_transaction_cancel(tr[i]));
   }


/*    TRY("flush",mird_cache_flush(db)); */

/*    for (i=0; i<=db->last_used+5; i++) */
/*       mird_describe_block(db,i); */

   mird_free_structure(db);

   fprintf(stderr,"---\n");

   TRY("init",mird_initialize("test.mird",&db));
   TRY("open",mird_open(db));
   for (i=0; i<Z; i++)
   {
      TRY("fetch",mird_key_lookup(db,17,i*4711,&data,&l));
      if (i&1)
      {
	 if (!data) fprintf(stderr,"missing at %ld\n",i);
	 else if (memcmp(data,"test data",9))
	    fprintf(stderr,"mismatch at %ld\n",i);
      }
      else
      {
	 if (data) fprintf(stderr,"too much at %ld\n",i);
      }
   }
   TRY("close db",mird_close(db));

   return 0;
}
