#include "global.h"
#include "config.h"

#include "program.h"
#include "module_support.h"
#include "interpret.h"
#include "stralloc.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"
#include "operators.h"
#include "object.h"
#include "threads.h"

#ifdef HAVE_MIRD_H
#ifdef HAVE_LIBMIRD
#define HAVE_MIRD
#endif
#endif

#include "module_magic.h"

#define sp Pike_sp
#define fp Pike_fp

#ifdef HAVE_MIRD

#include <mird.h>

struct program *mird_program;
struct program *mird_transaction_program;
struct program *mird_scanner_program;

#define THISOBJ (fp->current_object)

#define TRY(X) \
   do { MIRD_RES res; if ( (res=(X)) ) pmird_exception(res); } while (0)

#ifndef PIKE_THREADS

#define LOCK(PMIRD)							\
   do									\
   {									\
      struct pmird_storage *me=(PMIRD);					\

#define UNLOCK(PMIRD)							\
   }									\
   while (0);

#else

#define LOCK(PMIRD)							\
   do									\
   {									\
      struct pmird_storage *me=(PMIRD);					\
      ONERROR err;							\
      SET_ONERROR(err,pmird_unlock,&(me->mutex));			\
      THREADS_ALLOW();							\
      mt_lock(&(me->mutex));

#define UNLOCK(PMIRD)							\
      mt_unlock(&(me->mutex));						\
      THREADS_DISALLOW();						\
      UNSET_ONERROR(err);						\
   }									\
   while (0);

/**** utilities ************************************/

static void pmird_unlock(PIKE_MUTEX_T *mutex)
{
   mt_unlock(mutex);
}

#endif

static void pmird_exception(MIRD_RES res)
{
   char *s,*d;
   mird_describe_error(res,&s);
   d=alloca(strlen(s)+1);
   memcpy(d,s,strlen(s)+1);
   mird_free((unsigned char *)s);
   mird_free_error(res);
   Pike_error("[mird] %s\n",d);
}

static void pmird_no_database(char *func)
{
   Pike_error("%s: database is not open\n",func);
}

static void pmird_tr_no_database(char *func)
{
   Pike_error("%s: no database connected to the transaction\n",func);
}

static void pmird_no_transaction(void)
{
   Pike_error("transaction is already closed\n");
}

/**** main program *********************************/

struct pmird_storage
{
   struct mird *db;
#ifdef PIKE_THREADS
   PIKE_MUTEX_T mutex;
#endif
};

#define THIS ((struct pmird_storage*)(fp->current_storage))

static void init_pmird(struct object *o)
{
   THIS->db=NULL;
#ifdef PIKE_THREADS
   mt_init(&THIS->mutex);
#endif
}

static void exit_pmird(struct object *o)
{
   if (THIS->db)
   {
      mird_free_structure(THIS->db);
      THIS->db=NULL;
   }
#ifdef PIKE_THREADS
   mt_destroy(&THIS->mutex);
#endif
}

/*
**! module Mird
**! submodule Glue
**! class Mird
**! method void create(string filename)
**! method void create(string filename,mapping options)
**!
**!	<data_description type=mapping>
**!
**!	<elem type=string name=flags>database flags, see below</elem>
**!
**!     <elem type=int name=block_size>
**!           database block size; must be even 2^n
**!           database size is limited to 2^32 blocks, ie block_size*2^32 
**!           (8192*2^(32-5) is approximately 1Tb)
**!           (2048)
**!     </elem>
**!        
**!     <elem type=int name=frag_bits>
**!           this sets the number of frags in a fragmented block,
**!           and address bits to locate them.
**!           the number of frags in a fragmented block is equal
**!           to 2^frag_bits-1, per default 2^5-1=32-1=31
**!          
**!           This steals bits from the 32 bit address used to point out
**!           a chunk.
**!           (5)
**!     </elem>
**!     
**!     <elem type=int name=hashtrie_bits>
**!           this is the number of bits in each hash-trie hash; 
**!           4*2^n must be a lot less then block_size
**!           (5)
**!     </elem>
**!     
**!     <elem type=int name=cache_size>
**!           this is the number of blocks cached from the database;
**!           this is used for both read- and write cache.
**!           Note that the memory usage is constant, 
**!           approximately block_size*cache_size bytes.
**!           (32)
**!     </elem>
**!     
**!     <elem type=int name=cache_search_length>
**!           this is the closed hash maximum search length to find
**!           a block in the cache; note that this will result in 
**!           linear time usage.
**!           (8)
**!     </elem>
**!     
**!     <elem type=int name=max_free_frag_blocks>
**!           this is how many blocks with free space that is 
**!           kept in a list to be used when a transaction
**!           is running. The closest fit of usage of these blocks are
**!           where the data will be stored.
**!           (10)
**!     </elem>
**!     
**!     <elem type=int name=file_mode>
**!           this is the default mode the files are opened with;
**!           although affected by UMASK
**!           (0666)
**!     </elem>
**!     
**!     <elem type=int name=journal_readback_n>
**!           this is how many journal entries are read back
**!           at once at transaction finish, cancel or copy time
**!           bytes needed is approximately 24*this number
**!           (200)
**!     </elem>
**!	</data_description>
**! 
**! 	<pre>flags is a string with any of these characters:
**! 	"r" - readonly
**! 	"R" - readonly in a live system (one process writes, many reads)
**! 	"n" - don't create if it doesn't exist
**! 	"x" - exclusive (always create)
**! 	"s" - call fsync when finishing transactions
**! 	"S" - call sync(2) after fsync
**! 	"j" - complain if journal file is gone missing
**! 	</pre>
**!
*/

static void pmird_create(INT32 args)
{
   struct pmird_storage * this=THIS;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Mird",1);
   if (sp[-args].type!=T_STRING ||
       sp[-args].u.string->size_shift)
      SIMPLE_BAD_ARG_ERROR("Mird",1,"8 bit string");

   if (args>1)
      if (sp[1-args].type!=T_MAPPING)
	 SIMPLE_BAD_ARG_ERROR("Mird",2,"mapping of options");

   if (this->db)
      mird_free_structure(this->db);

   TRY(mird_initialize(sp[-args].u.string->str,&(this->db)));

   if (args>1)
   {
#define LOOKUP(SVALUE,TEXT)						\
      push_svalue(&(SVALUE));						\
      push_text(TEXT);							\
      f_index(2);

#define LOOKUP_INT(SVALUE,TEXT,WHAT)					\
      LOOKUP(SVALUE,TEXT);						\
      if (!IS_UNDEFINED(sp-1))						\
      {									\
	 if (sp[-1].type!=T_INT)					\
	    SIMPLE_BAD_ARG_ERROR("Mird",2,"option \""TEXT"\":int");	\
	 this->db->WHAT=sp[-1].u.integer;				\
      }									\
      pop_stack();

      LOOKUP_INT(sp[1-args],"block_size",block_size);
      LOOKUP_INT(sp[1-args],"frag_bits",frag_bits);
      LOOKUP_INT(sp[1-args],"hashtrie_bits",hashtrie_bits);
      LOOKUP_INT(sp[1-args],"cache_size",cache_size);
      LOOKUP_INT(sp[1-args],"cache_search_length",cache_search_length);
      LOOKUP_INT(sp[1-args],"max_free_frag_blocks",max_free_frag_blocks);
      LOOKUP_INT(sp[1-args],"file_mode",file_mode);
      LOOKUP_INT(sp[1-args],"journal_readback_n",journal_readback_n);

      LOOKUP(sp[1-args],"flags");
      if (!IS_UNDEFINED(sp-1))
      {
	 ptrdiff_t i;
	 if (sp[-1].type!=T_STRING ||
	     sp[-1].u.string->size_shift)
	    SIMPLE_BAD_ARG_ERROR("Mird",2,"\"flags\":8-bit string");
	 for (i=0; i<sp[-1].u.string->len; i++)
	    switch (sp[-1].u.string->str[i])
	    {
	       case 'r': this->db->flags|=MIRD_READONLY; break;
	       case 'R': this->db->flags|=MIRD_READONLY|MIRD_LIVE; break;
	       case 'n': this->db->flags|=MIRD_NOCREATE; break;
	       case 'x': this->db->flags|=MIRD_EXCL; break;
	       case 's': this->db->flags|=MIRD_SYNC_END; break;
	       case 'S': this->db->flags|=MIRD_CALL_SYNC; break;
	       case 'j': this->db->flags|=MIRD_JO_COMPLAIN; break;
	       default:
		  SIMPLE_BAD_ARG_ERROR("Mird",2,"\"flags\":[rRnxsSj]");
	    }
      }
      pop_stack();
   }

/*     this->db->cache_size=6144; */
/*     this->db->journal_readback_n=10000; */
/*    this->db->hashtrie_bits=4; */
   
   /* options here */

LOCK(this);
   do 
   { 
      MIRD_RES res; 
      if ( (res=mird_open(this->db)) ) 
      {
	 mird_free_structure(this->db);
	 this->db=NULL;
	 pmird_exception(res); 
      }
   } while (0);
UNLOCK(this);

   pop_n_elems(args);
   push_int(0);
}

/*
**! method void close()
**! method void destroy()
**!	This closes the database, ie
**!	<ol>
**!	<li>cancels all ongoing transactions</li>
**!	<li>syncs the database (flushes caches,
**!	and frees all unused blocks)</li>
**!	<li>destroys the database object</li>
**!	</ol>
**!	Call this or destroy the database object
**!	when you want the database closed.
*/

static void pmird_close(INT32 args)
{
   struct pmird_storage * this=THIS;

   MIRD_RES res;
   pop_n_elems(args);

   if (this->db)
   {
LOCK(this);
      res=mird_close(this->db);
      if (res) mird_free_structure(this->db);
      this->db=NULL;
      if (res) pmird_exception(res);
UNLOCK(this);
   }

   push_int(0);
}

/* 
**! method object sync()
**! method object sync_please()
**!	Syncs the database; this flushes all
**!	eventual caches and frees all unused blocks.
**!
**!	sync() can only be called when there is no
**!	ongoing transactions. sync_please() sets
**!	a marker that the database should be synced
**!	when the last transaction is finished or cancelled,
**!	which will eventually sync the database.
**!	
**!	The usage could be in a call_out-loop,
**!	<pre>
**!	   [...]
**!	   call_out(sync_the_database,5*60);	
**!	   [...]
**!	void sync_the_database()
**!	{
**!	   call_out(sync_please,5*60);
**!	   my_mird->sync_please();
**!	}
**!	</pre>
**!
**! returns the called object
*/

static void pmird_sync(INT32 args)
{
   struct pmird_storage * this=THIS;

   pop_n_elems(args);

   if (!this->db) pmird_no_database("sync");

   LOCK(this);
   TRY(mird_sync(this->db));
   UNLOCK(this);

   ref_push_object(THISOBJ);
}

static void pmird_sync_please(INT32 args)
{
   struct pmird_storage * this=THIS;

   pop_n_elems(args);

   if (!this->db) pmird_no_database("sync_please");

   LOCK(this);
   TRY(mird_sync_please(this->db));
   UNLOCK(this);

   ref_push_object(THISOBJ);
}

/*
**! method zero|string fetch(int table_id,int|string key)
**! returns the data value or zero_type if key wasn't found
*/

static void pmird_fetch(INT32 args)
{
   struct pmird_storage * this=THIS;

   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct mird *db=THIS->db;
   unsigned char *data;
   mird_size_t len;

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",2);

   if (!this->db) {
     pmird_no_transaction();
     return;
   }

   if (sp[1-args].type==T_INT)
   {
      get_all_args("fetch",args,"%i%i",&table_id,&hashkey);
LOCK(this);
      TRY(mird_key_lookup(db,(mird_key_t)table_id,
			  (mird_key_t)hashkey,
			  &data,
			  &len));
UNLOCK(this);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("fetch",args,"%i%S",&table_id,&stringkey);
LOCK(this);
      TRY(mird_s_key_lookup(db,(mird_key_t)table_id,
			    (unsigned char*)(stringkey->str),
			    DO_NOT_WARN((mird_size_t)(stringkey->len)),
			    &data,
			    &len));
UNLOCK(this);
   }
   else
      SIMPLE_BAD_ARG_ERROR("fetch",2,"int|string");
   
   pop_n_elems(args);

   if (data)
   {
      push_string(make_shared_binary_string((char *)data,len));
      mird_free(data);
   }
   else
   {
      push_int(0);
      sp[-1].subtype=NUMBER_UNDEFINED;
   }
}

/*
**! method int first_unused_key(int table_id)
**! method int first_unused_key(int table_id,int start_key)
**! method int first_unused_table()
**! method int first_unused_table(int start_table_id)
*/

static void pmird_first_unused_key(INT32 args)
{
   struct pmird_storage * this=THIS;

   INT_TYPE table_id=0,start_key=0;
   mird_key_t dest_key;

   if (args>1)
      get_all_args("first_unused_key",args,"%i%i",&table_id,&start_key);
   else
      get_all_args("first_unused_key",args,"%i",&table_id);

   if (!this->db) {
     pmird_no_transaction();
     return;
   }

LOCK(this);
   TRY(mird_find_first_unused(this->db,(mird_key_t)table_id,
			      (mird_key_t)start_key,&dest_key));
UNLOCK(this);
   pop_n_elems(args);
   push_int( (INT_TYPE)dest_key );
}

static void pmird_first_unused_table(INT32 args)
{
   struct pmird_storage * this=THIS;

   INT_TYPE table_id=0;
   mird_key_t dest_key;

   if (args)
      get_all_args("first_unused_table",args,"%i",&table_id);

   if (!this->db) {
     pmird_no_transaction();
     return;
   }

LOCK(this);
   TRY(mird_find_first_unused_table(this->db,(mird_key_t)table_id,
				    &dest_key));
UNLOCK(this);
   pop_n_elems(args);
   push_int( (INT_TYPE)dest_key );
}

/*
**! method void _debug_cut()
**!	This closes the database without
**!	flushing or syncing. This should never be used and
**!	exist only for testing purposes.
*/

static void pmird__debug_cut(INT32 args)
{
   struct pmird_storage * this=THIS;

   if (this->db)
   {
      mird_free_structure(this->db);
      this->db=NULL;
   }
   
   pop_n_elems(args);
   push_int(0);
}

/*
**! method void _debug_check_free()
**! method void _debug_check_free(int(0..1) silent)
**!	this syncs the database and verifies
**!	the database free list. It prints stuff
**!	on stderr. It exists only for debug
**!	purpose and has no other use.
*/

static void pmird__debug_check_free(INT32 args)
{
   struct pmird_storage * this=THIS;

   int silent=0;
   if (sp[-args].type==T_INT &&
       sp[-args].u.integer) silent=1;
   if (!this->db) pmird_no_database("_debug_check_free");
   TRY(mird_sync(this->db));
   mird_debug_check_free(this->db,silent);
   
   pop_n_elems(args);
   push_int(0);
}

/*
**! method int _debug_syscalls()
**! returns the number of syscalls the database has done so far
*/

static void pmird__debug_syscalls(INT32 args)
{
   struct pmird_storage * this=THIS;

   if (!this->db) pmird_no_database("_debug_syscalls");

   pop_n_elems(args);
   push_int( (INT_TYPE)(this->db->syscalls_counter[0]) );
   push_int( (INT_TYPE)(this->db->syscalls_counter[1]) );
   push_int( (INT_TYPE)(this->db->syscalls_counter[2]) );
   push_int( (INT_TYPE)(this->db->syscalls_counter[3]) );
   push_int( (INT_TYPE)(this->db->syscalls_counter[4]) );
   push_int( (INT_TYPE)(this->db->syscalls_counter[5]) );
   push_int( (INT_TYPE)(this->db->syscalls_counter[6]) );
   push_int( (INT_TYPE)(this->db->last_used) );
   push_int( (INT_TYPE)(this->db->last_used*this->db->block_size) );
   f_aggregate(9);
}

/**** transaction program ***************************/

struct pmtr_storage
{
   struct mird_transaction *mtr;
   struct object *dbobj;
   struct pmird_storage *parent;
};

#undef THIS
#define THIS ((struct pmtr_storage*)(fp->current_storage))

static void init_pmtr(struct object *o)
{
   THIS->mtr=NULL;
   THIS->dbobj=NULL;
}

static void exit_pmtr(struct object *o)
{
   if (THIS->mtr)
   {
      mird_tr_free(THIS->mtr);
      THIS->mtr=NULL;
   }
   if (THIS->dbobj)
   {
      free_object(THIS->dbobj);
      THIS->dbobj=NULL;
   }
}

/*
**! class Transaction
**!	A transaction object is enclosing a change in the database.
**!	It can either succeed in full or fail in full.
**!	If some other transaction has changed the same data as
**!	the current, the transaction closed last will fail (conflict).
**!	
**! method void create(Mird parent)
**!	Creates a new transaction within the given database.
*/

static void pmtr_create(INT32 args)
{
   struct pmtr_storage *this=THIS;

   struct pmird_storage *pmird=NULL;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Transaction",1);

   if ( !sp[-1].type==T_OBJECT ||
	!(pmird=(struct pmird_storage*)
	  get_storage(sp[-args].u.object,mird_program)) )
      SIMPLE_BAD_ARG_ERROR("Transaction",1,"Mird object");

   add_ref(sp[-args].u.object);
   this->dbobj=sp[-args].u.object;
   this->parent=pmird;

   if (!pmird->db) pmird_no_database("Transaction");

   this->mtr=NULL;
LOCK(this->parent);
   TRY(mird_transaction_new(pmird->db,&(this->mtr)));
UNLOCK(this->parent);

   pop_n_elems(args);
   push_int(0);
}

/*
**! method void close()
**!	closes a transaction; may cast exceptions
**!	if there are conflicts
*/

static void pmtr_close(INT32 args)
{
   struct pmtr_storage *this=THIS;

   struct mird_transaction *mtr;

   pop_n_elems(args);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("close");
     return;
   }

   mtr=this->mtr;
LOCK(this->parent);
   TRY(mird_transaction_close(mtr));
UNLOCK(this->parent);
   this->mtr=NULL;

   ref_push_object(THISOBJ);
}

/*
**! method void cancel()
**! method void destroy()
**!	cancels (rewinds) a transaction
*/

static void pmtr_cancel(INT32 args)
{
   struct pmtr_storage *this=THIS;

   pop_n_elems(args);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("cancel");
     return;
   }

LOCK(this->parent);
   TRY(mird_transaction_cancel(this->mtr));
UNLOCK(this->parent);
   this->mtr=NULL;

   push_int(0);
}

static void pmtr_destroy(INT32 args)
{
   struct pmtr_storage *this=THIS;

   pop_n_elems(args);

   if (this->mtr &&
       this->mtr->db)
   {
LOCK(this->parent);
      TRY(mird_transaction_cancel(this->mtr));
UNLOCK(this->parent);
      this->mtr=NULL;
   }
   else if (this->mtr)
   {
      mird_tr_free(this->mtr);
      this->mtr=NULL;
   }

   push_int(0);
}

/*
**! method object resolve()
**!	Tries to resolve a transaction; 
**!	casts an exception if there is a conflict.
**!	May be called more then once.
**! returns the called object
*/

static void pmtr_resolve(INT32 args)
{
   struct pmtr_storage *this=THIS;

   pop_n_elems(args);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("resolve");
     return;
   }

LOCK(this->parent);
   TRY(mird_tr_resolve(this->mtr));
UNLOCK(this->parent);

   ref_push_object(THISOBJ);
}

/*
**! method object store(int table_id,int|string key,string data)
*/

static void pmtr_store(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct pike_string *data;
   struct mird_transaction *mtr=THIS->mtr;

   if (args<3)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",3);

   if (!THIS->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!THIS->mtr->db) {
     pmird_tr_no_database("store");
     return;
   }

   if (sp[1-args].type==T_INT)
   {
      get_all_args("store",args,"%i%i%S",&table_id,&hashkey,&data);
LOCK(this->parent);
      TRY(mird_key_store(mtr,(mird_key_t)table_id,
			 (mird_key_t)hashkey,
			 (unsigned char*)(data->str),
			 DO_NOT_WARN((mird_size_t)(data->len))));
UNLOCK(this->parent);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("store",args,"%i%S%S",&table_id,&stringkey,&data);
LOCK(this->parent);
      TRY(mird_s_key_store(mtr,(mird_key_t)table_id,
			   (unsigned char*)(stringkey->str),
			   DO_NOT_WARN((mird_size_t)(stringkey->len)),
			   (unsigned char*)(data->str),
			   DO_NOT_WARN((mird_size_t)(data->len))));
UNLOCK(this->parent);
   }
   else
      SIMPLE_BAD_ARG_ERROR("store",2,"int|string");
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object delete(int table_id,int|string key)
*/

static void pmtr_delete(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct mird_transaction *mtr=THIS->mtr;

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",2);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("delete");
     return;
   }

   if (sp[1-args].type==T_INT)
   {
      get_all_args("delete",args,"%i%i",&table_id,&hashkey);
LOCK(this->parent);
      TRY(mird_key_store(mtr,(mird_key_t)table_id,
			 (mird_key_t)hashkey,
			 NULL,0));
UNLOCK(this->parent);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("delete",args,"%i%S",&table_id,&stringkey);
LOCK(this->parent);
      TRY(mird_s_key_store(mtr,(mird_key_t)table_id,
			   (unsigned char*)(stringkey->str),
			   DO_NOT_WARN((mird_size_t)(stringkey->len)),
			   NULL,0));
UNLOCK(this->parent);
   }
   else
      SIMPLE_BAD_ARG_ERROR("delete",2,"int|string");
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method zero|string fetch(int table_id,int|string key)
**! returns the data value or zero_type if key wasn't found
*/

static void pmtr_fetch(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct mird_transaction *mtr=this->mtr;
   unsigned char *data;
   mird_size_t len;

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",2);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("fetch");
     return;
   }

   if (sp[1-args].type==T_INT)
   {
      get_all_args("fetch",args,"%i%i",&table_id,&hashkey);
LOCK(this->parent);
      TRY(mird_transaction_key_lookup(mtr,(mird_key_t)table_id,
				      (mird_key_t)hashkey,
				      &data,
				      &len));
UNLOCK(this->parent);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("fetch",args,"%i%S",&table_id,&stringkey);
LOCK(this->parent);
      TRY(mird_transaction_s_key_lookup(mtr,(mird_key_t)table_id,
					(unsigned char*)(stringkey->str),
					DO_NOT_WARN((mird_size_t)(stringkey->len)),
					&data,
					&len));
UNLOCK(this->parent);
   }
   else
      SIMPLE_BAD_ARG_ERROR("fetch",2,"int|string");
   
   pop_n_elems(args);

   if (data)
   {
      push_string(make_shared_binary_string((char *)data,len));
      mird_free(data);
   }
   else
   {
      push_int(0);
      sp[-1].subtype=NUMBER_UNDEFINED;
   }
}

/*
**! method object new_hashkey_table(int table_id);
**! method object new_stringkey_table(int table_id);
**!	creates a table in the database
*/

static void pmtr_new_hashkey_table(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE table_id;

   get_all_args("new_hashkey_table",args,"%i",&table_id);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("new_hashkey_table");
     return;
   }

LOCK(this->parent);
   TRY(mird_key_new_table(this->mtr,(mird_key_t)table_id));
UNLOCK(this->parent);

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void pmtr_new_stringkey_table(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE table_id;

   get_all_args("new_hashkey_table",args,"%i",&table_id);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("new_stringkey_table");
     return;
   }

LOCK(this->parent);
   TRY(mird_s_key_new_table(this->mtr,(mird_key_t)table_id));
UNLOCK(this->parent);

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object delete_table(int table_id)
**!	delets a table from the database
**! note 
**!	this can take some time, depending on how
**!	much data that is in that table
*/

static void pmtr_delete_table(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE table_id;

   get_all_args("delete_table",args,"%i",&table_id);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("delete_table");
     return;
   }
  
   LOCK(this->parent);
   TRY(mird_delete_table(this->mtr,(mird_key_t)table_id));
   UNLOCK(this->parent);
}

/*
**! method object depend_table(int table_id)
*/

static void pmtr_depend_table(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE table_id;

   get_all_args("depend_table",args,"%i",&table_id);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("depend_table");
     return;
   }
  
   LOCK(this->parent);
   TRY(mird_depend_table(this->mtr,(mird_key_t)table_id));
   UNLOCK(this->parent);
}

/*
**! method int first_unused_key(int table_id)
**! method int first_unused_key(int table_id,int start_key)
**! method int first_unused_table()
**! method int first_unused_table(int start_table_id)
*/

static void pmtr_first_unused_key(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE table_id=0,start_key=0;
   mird_key_t dest_key;

   if (args>1)
      get_all_args("first_unused_key",args,"%i%i",&table_id,&start_key);
   else
      get_all_args("first_unused_key",args,"%i",&table_id);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("first_unused_key");
     return;
   }

LOCK(this->parent);
   TRY(mird_transaction_find_first_unused(this->mtr,(mird_key_t)table_id,
					  (mird_key_t)start_key,&dest_key));
UNLOCK(this->parent);
   pop_n_elems(args);
   push_int( (INT_TYPE)dest_key );
}

static void pmtr_first_unused_table(INT32 args)
{
   struct pmtr_storage *this=THIS;

   INT_TYPE table_id=0;
   mird_key_t dest_key;

   if (args)
      get_all_args("first_unused_table",args,"%i",&table_id);

   if (!this->mtr) {
     pmird_no_transaction();
     return;
   }
   if (!this->mtr->db) {
     pmird_tr_no_database("first_unused_table");
     return;
   }

LOCK(this->parent);
   TRY(mird_transaction_find_first_unused_table(this->mtr,(mird_key_t)table_id,
						&dest_key));
UNLOCK(this->parent);
   pop_n_elems(args);
   push_int( (INT_TYPE)dest_key );
}

/**** hashkey table scanner ************************/

#undef THIS
#define THIS ((struct pmts_storage*)(fp->current_storage))

struct pmts_storage
{
   enum { PMTS_UNKNOWN, PMTS_HASHKEY, PMTS_STRINGKEY } type;
   struct mird_scan_result *msr;
   struct mird_s_scan_result *mssr;
   struct object *obj;
   struct pmird_storage *pmird;
   struct pmtr_storage *pmtr;
   mird_key_t table_id;
};

static void init_pmts(struct object *o)
{
   THIS->msr=NULL;
   THIS->mssr=NULL;
   THIS->obj=NULL;
   THIS->type=PMTS_UNKNOWN;
}

static void exit_pmts(struct object *o)
{
   if (THIS->msr) mird_free_scan_result(THIS->msr);
   if (THIS->mssr) mird_free_s_scan_result(THIS->mssr);
   THIS->msr=NULL;
   THIS->mssr=NULL;
   if (THIS->obj) free_object(THIS->obj);
   THIS->obj=0;
}

/*
**! class Scanner
**!	Objects of this class is used to read off all
**!	contents of a table in the database.
**!
**! method void create(Mird database,int table_id)
**! method void create(Transaction transaction,int table_id)
**!	Creates a new scanner object, tuned to the
**!	given table and database or transaction.
*/

static void pmts_create(INT32 args)
{
   struct pmird_storage *pmird;
   struct pmtr_storage *pmtr;
   struct pmts_storage *this=THIS;
   mird_key_t type;

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("Scanner",2);

   exit_pmts(THISOBJ);

   pmird=(struct pmird_storage*)
      get_storage(sp[-args].u.object,mird_program);
   pmtr=(struct pmtr_storage*)
      get_storage(sp[-args].u.object,mird_transaction_program);

   if (!pmird && !pmtr)
      SIMPLE_BAD_ARG_ERROR("Scanner",1,"Mird|Transaction");

   if (sp[1-args].type!=T_INT)
      SIMPLE_BAD_ARG_ERROR("Scanner",2,"int");

   add_ref(sp[-args].u.object);
   this->obj=sp[-args].u.object;
   this->pmird=pmird;
   this->pmtr=pmtr;

   this->table_id=(mird_key_t)sp[1-args].u.integer;

   if (!this->pmird)
      this->pmird=this->pmtr->parent;

LOCK(this->pmird);
   if (this->pmtr)
      TRY(mird_transaction_get_table_type(this->pmtr->mtr,
					  this->table_id,&type));
   else
      TRY(mird_get_table_type(this->pmird->db,this->table_id,&type));
UNLOCK(this->pmird);

   switch (type)
   {
      case MIRD_TABLE_HASHKEY: this->type=PMTS_HASHKEY; break;
      case MIRD_TABLE_STRINGKEY: this->type=PMTS_STRINGKEY; break;
      default:
	 Pike_error("Scanner: Unknown table %08lx\n",(unsigned long)type);
   }

   if (args>2) {
      if (sp[2-args].type!=T_INT)
	 SIMPLE_BAD_ARG_ERROR("Scanner",3,"int");
      else
      {
	 mird_key_t key=(mird_key_t)sp[2-args].u.integer;
	 switch (this->type)
	 {
	    case PMTS_HASHKEY:
	       TRY(mird_scan_continued(key,&(this->msr)));
	       break;
	    case PMTS_STRINGKEY:
	       TRY(mird_s_scan_continued(key,&(this->mssr)));
	       break;
	    case PMTS_UNKNOWN: Pike_error("illegal scanner type\n"); break;
	 }
      }
   }

   pop_n_elems(args);
   push_int(0);
}

/*
**! method zero|mapping(string|int:string) read(int n)
**!	Reads some tupels from the table the scanner
**!	is directed against; the size of the resulting
**!	mapping is close to this number.
**!
**! returns a mapping of the next (about) n tupels in the table, or zero if there is no more tupels in the table
**!
**! note
**!	a buffer directly depending on this size is allocated;
**!	it's not recommended doing a "read(0x7fffffff)".
*/

#define SCAN_MAP 0
#define SCAN_TUPELS 1
#define SCAN_INDICES 2
#define SCAN_VALUES 3
#define SCAN_COUNT 4

static void _pmts_read(INT32 args,int do_what)
{
   struct pmts_storage *this=THIS;

   INT_TYPE n;
   MIRD_RES res=NULL;
   mird_size_t i,nn;

   get_all_args("read",args,"%+",&n);

   if (this->pmird && !this->pmird->db) pmird_no_database("read");
   if (this->pmtr && !this->pmtr->mtr) pmird_no_transaction();
   if (this->pmtr && !this->pmtr->parent->db) pmird_tr_no_database("read");


LOCK(this->pmird);
   if (this->pmird)
   {
      switch (this->type)
      {
	 case PMTS_HASHKEY:
	    res=mird_table_scan(this->pmird->db,this->table_id,(mird_size_t)n,
				this->msr,&(this->msr));
	    break;
	 case PMTS_STRINGKEY:
	    res=mird_s_table_scan(this->pmird->db,this->table_id,
				  (mird_size_t)n,this->mssr,&(this->mssr));
	    break;
	 default: Pike_error("illegal scanner type\n"); break;
      }
   }
   else /* pmtr */
   {
      switch (this->type)
      {
	 case PMTS_HASHKEY:
	    res=mird_transaction_table_scan(this->pmtr->mtr,
					    this->table_id,(mird_size_t)n,
					    this->msr,&(this->msr));
	    break;
	 case PMTS_STRINGKEY:
	    res=mird_transaction_s_table_scan(this->pmtr->mtr,
					      this->table_id,(mird_size_t)n,
					      this->mssr,&(this->mssr));
	    break;
	 default: Pike_error("illegal scanner type\n"); break;
      }
   }
UNLOCK(this->pmird);
   if (res) pmird_exception(res);

   pop_n_elems(args);

   if (this->msr)
   {
      if (do_what!=SCAN_COUNT) 
	 for (i=0; i<this->msr->n; i++)
	 {
	    if (do_what!=SCAN_VALUES)
	       push_int((INT_TYPE)(this->msr->tupel[i].key));
	    if (do_what!=SCAN_INDICES)
	       push_string(make_shared_binary_string(
		  (char *)this->msr->tupel[i].value,
		  this->msr->tupel[i].value_len));
	    if (do_what==SCAN_TUPELS)
	       f_aggregate(2);
	 }
      nn=this->msr->n;
   }
   else if (this->mssr)
   {
      if (do_what!=SCAN_COUNT)
	 for (i=0; i<this->mssr->n; i++)
	 {
	    if (do_what!=SCAN_VALUES)
	       push_string(make_shared_binary_string(
		  (char *)this->mssr->tupel[i].key,
		  this->mssr->tupel[i].key_len));
	    if (do_what!=SCAN_INDICES)
	       push_string(make_shared_binary_string(
		  (char *)this->mssr->tupel[i].value,
		  this->mssr->tupel[i].value_len));
	    if (do_what==SCAN_TUPELS)
	       f_aggregate(2);
	 }
      nn=this->mssr->n;
   }
   else /* eod */
   {
      push_int(0);
      return;
   }
   if (do_what==SCAN_COUNT)
      push_int(nn);
   else if (do_what==SCAN_MAP)
      f_aggregate_mapping(nn*2);
   else
      f_aggregate(nn);
}

static void pmts_read(INT32 args)
{
   _pmts_read(args,SCAN_MAP);
}

static void pmts_read_tupels(INT32 args)
{
   _pmts_read(args,SCAN_TUPELS);
}

static void pmts_read_indices(INT32 args)
{
   _pmts_read(args,SCAN_INDICES);
}

static void pmts_read_values(INT32 args)
{
   _pmts_read(args,SCAN_VALUES);
}

static void pmts_read_count(INT32 args)
{
   _pmts_read(args,SCAN_COUNT);
}


/*
**! method int next_key()
**!	Gives back a possible argument to the constructor
**!	of <ref>Scanner</ref>; allows the possibility
**!	to continue to scan even if the Scanner object is lost.
*/

static void pmts_next_key(INT32 args)
{
   mird_key_t key;
   struct pmts_storage *this=THIS;
   switch (this->type)
   {
      case PMTS_HASHKEY:
	 TRY(mird_scan_continuation(this->msr,&key));
	 break;
      case PMTS_STRINGKEY:
	 TRY(mird_s_scan_continuation(this->mssr,&key));
	 break;
      case PMTS_UNKNOWN: Pike_error("illegal scanner type\n"); break;
   }
   pop_n_elems(args);
   push_int( (INT_TYPE)key );
}

/****              *********************************/

int mird_check_mem(void);

static void m_debug_check_mem(INT32 args)
{
   pop_n_elems(args);
   push_int(mird_check_mem());
}

/**** module stuff *********************************/

void pike_module_init(void)
{
#if 0
   struct program *p;
   struct object *o;
#endif

   /* main program */

   start_new_program();

   ADD_STORAGE(struct pmird_storage);
   set_init_callback(init_pmird);
   set_exit_callback(exit_pmird);

   ADD_FUNCTION("create",pmird_create,
		tFunc(tStr tOr(tVoid,tMapping),tVoid),0);
   ADD_FUNCTION("close",pmird_close,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("destroy",pmird_close,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("sync",pmird_sync,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("sync_please",pmird_sync_please,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("fetch",pmird_fetch,tFunc(tInt tOr(tInt,tStr),tOr(tStr,tInt0)),0);

   ADD_FUNCTION("first_unused_key",pmird_first_unused_key,
		tFunc(tInt tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("first_unused_table",pmird_first_unused_table,
		tFunc(tOr(tVoid,tInt),tInt),0);

   ADD_FUNCTION("_debug_cut",pmird__debug_cut,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("_debug_check_free",pmird__debug_check_free,
		tFunc(tNone,tVoid),0);
   ADD_FUNCTION("_debug_syscalls",pmird__debug_syscalls,
		tFunc(tNone,tArr(tInt)),0);

   mird_program=end_program();

   /* transaction program */

   start_new_program();
   ADD_STORAGE(struct pmtr_storage);
   set_init_callback(init_pmtr);
   set_exit_callback(exit_pmtr);

   ADD_FUNCTION("create",pmtr_create,tFunc(tObj,tVoid),0);
   ADD_FUNCTION("cancel",pmtr_cancel,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("destroy",pmtr_destroy,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("close",pmtr_close,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("resolve",pmtr_resolve,tFunc(tNone,tObj),0);

   ADD_FUNCTION("store",pmtr_store,tFunc(tInt tOr(tInt,tStr) tStr,tObj),0);
   ADD_FUNCTION("delete",pmtr_delete,tFunc(tInt tOr(tInt,tStr),tObj),0);
   ADD_FUNCTION("fetch",pmtr_fetch,tFunc(tInt tOr(tInt,tStr),
					 tOr(tStr,tInt0)),0);
   ADD_FUNCTION("delete_table",pmtr_delete_table,tFunc(tInt,tObj),0);
   ADD_FUNCTION("depend_table",pmtr_depend_table,tFunc(tInt,tObj),0);

   ADD_FUNCTION("new_stringkey_table",pmtr_new_stringkey_table,
		tFunc(tInt,tObj),0);
   ADD_FUNCTION("new_hashkey_table",pmtr_new_hashkey_table,
		tFunc(tInt,tObj),0);

   ADD_FUNCTION("first_unused_key",pmtr_first_unused_key,
		tFunc(tInt tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("first_unused_table",pmtr_first_unused_table,
		tFunc(tOr(tVoid,tInt),tInt),0);

   mird_transaction_program=end_program();

   /* scanner */

   start_new_program();
   ADD_STORAGE(struct pmts_storage);
   set_init_callback(init_pmts);
   set_exit_callback(exit_pmts);

   ADD_FUNCTION("create",pmts_create,tFunc(tObj tInt,tVoid),0);
   ADD_FUNCTION("read",pmts_read,tFunc(tIntPos,tMap(tOr(tInt,tStr),tStr)),0);
   ADD_FUNCTION("read_tupels",pmts_read_tupels,
		tFunc(tIntPos,tArr(tArr(tOr(tInt,tStr)))),0);
   ADD_FUNCTION("read_indices",pmts_read_indices,
		tFunc(tIntPos,tArr(tOr(tInt,tStr))),0);
   ADD_FUNCTION("read_values",pmts_read_values,
		tFunc(tIntPos,tArr(tStr)),0);
   ADD_FUNCTION("read_count",pmts_read_count,
		tFunc(tIntPos,tIntPos),0);
   ADD_FUNCTION("next_key",pmts_next_key,tFunc(tNone,tInt),0);

   mird_scanner_program=end_program();

   /* all done */

#if 0
   start_new_program();
#endif

   add_program_constant("Mird",mird_program,0);
   add_program_constant("Transaction",mird_transaction_program,0);
   add_program_constant("Scanner",mird_scanner_program,0);

   ADD_FUNCTION("_debug_check_mem",m_debug_check_mem,
		tFunc(tNone,tInt),0);

#if 0
   p=end_program();

   o=clone_object(p,0);
   add_object_constant("Glue",o,0);
   free_object(o);
   free_program(p);
#endif
}

void pike_module_exit(void)
{
   free_program(mird_program);
   free_program(mird_transaction_program);
   free_program(mird_scanner_program);
}

#else

void pike_module_init(void) {}
void pike_module_exit(void) {}

#endif
