#include "global.h"
#include "config.h"

#include "program.h"
#include "module_support.h"
#include "interpret.h"
#include "stralloc.h"
#include "mapping.h"
#include "array.h"
#include "object.h"

#ifdef HAVE_MIRD_H
#ifdef HAVE_LIBMIRD
#define HAVE_MIRD
#endif
#endif

#ifdef HAVE_MIRD

#include "mird.h"

struct program *mird_program;
struct program *mird_transaction_program;
struct program *mird_scanner_program;

#define THISOBJ (fp->current_object)

#define TRY(X) \
   do { MIRD_RES res; if ( (res=(X)) ) pmird_exception(res); } while (0)

#define LOCK(PMIRD)
#define UNLOCK(PMIRD)

/**** utilities ************************************/

static void pmird_exception(MIRD_RES res)
{
   char *s,*d;
   mird_describe_error(res,&s);
   d=alloca(strlen(s)+1);
   memcpy(d,s,strlen(s)+1);
   error("[mird] %s\n",s);
}

static void pmird_no_database(void)
{
   error("database is not open\n");
}

static void pmird_tr_no_database(void)
{
   error("no database connected to the transaction\n");
}

static void pmird_no_transaction(void)
{
   error("transaction is already closed\n");
}

/**** main program *********************************/

struct pmird_storage
{
   struct mird *db;
};

#define THIS ((struct pmird_storage*)(fp->current_storage))

static void init_pmird(struct object *o)
{
   THIS->db=NULL;
}

static void exit_pmird(struct object *o)
{
   if (THIS->db)
   {
      mird_free_structure(THIS->db);
      THIS->db=NULL;
   }
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
**! 	"n" - don't create if it doesn't exist
**! 	"x" - exclusive (always create)
**! 	"s" - call sync(2) after fsync
**! 	"j" - complain if journal file is gone missing
**! 	</pre>
**!
*/

static void pmird_create(INT32 args)
{
   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Mird",1);
   if (sp[-args].type!=T_STRING ||
       sp[-args].u.string->size_shift)
      SIMPLE_BAD_ARG_ERROR("Mird",1,"8 bit string");

LOCK(THIS);
   if (THIS->db)
      mird_free_structure(THIS->db);

   TRY(mird_initialize(sp[-args].u.string->str,&(THIS->db)));

/*    THIS->db->hashtrie_bits=4; */
   
   /* options here */

   TRY(mird_open(THIS->db));
UNLOCK(THIS);

   pop_n_elems(args);
   push_int(0);
}

/*
**! method void close()
**! method void destroy()
**!	This closes the database, ie
**!	<ol>
**!	<li>cancels all ongoing transactions
**!	<li>syncs the database (flushes caches,
**!	and frees all unused blocks)
**!	<li>destroys the database object
**!	</ol>
**!	Call this or destroy the database object
**!	when you want the database closed.
*/

static void pmird_close(INT32 args)
{
   MIRD_RES res;
   pop_n_elems(args);

   if (THIS->db)
   {
LOCK(THIS);
      res=mird_close(THIS->db);
      if (res) mird_free_structure(THIS->db);
      THIS->db=NULL;
      if (res) pmird_exception(res);
UNLOCK(THIS);
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
   pop_n_elems(args);

   if (!THIS->db) pmird_no_database();

   LOCK(THIS);
   TRY(mird_sync(THIS->db));
   UNLOCK(THIS);

   ref_push_object(THISOBJ);
}

static void pmird_sync_please(INT32 args)
{
   pop_n_elems(args);

   if (!THIS->db) pmird_no_database();

   LOCK(THIS);
   TRY(mird_sync_please(THIS->db));
   UNLOCK(THIS);

   ref_push_object(THISOBJ);
}

/*
**! method zero|string fetch(int table_id,int|string key)
**! returns the data value or zero_type if key wasn't found
*/

static void pmird_fetch(INT32 args)
{
   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct mird *db=THIS->db;
   unsigned char *data;
   mird_size_t len;

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",2);

   if (!THIS->db) return pmird_no_transaction();

   if (sp[1-args].type==T_INT)
   {
      get_all_args("fetch",args,"%i%i",&table_id,&hashkey);
LOCK(THIS);
      TRY(mird_key_lookup(db,(mird_key_t)table_id,
			  (mird_key_t)hashkey,
			  &data,
			  &len));
UNLOCK(THIS);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("fetch",args,"%i%S",&table_id,&stringkey);
LOCK(THIS);
      TRY(mird_s_key_lookup(db,(mird_key_t)table_id,
			    (unsigned char*)(stringkey->str),
			    (mird_size_t)(stringkey->len),
			    &data,
			    &len));
UNLOCK(THIS);
   }
   else
      SIMPLE_BAD_ARG_ERROR("fetch",2,"int|string");
   
   pop_n_elems(args);

   if (data)
      push_string(make_shared_binary_string(data,len));
   else
   {
      push_int(0);
      sp[-1].subtype=NUMBER_UNDEFINED;
   }
   mird_free(data);
}

/*
**! method void _debug_cut()
**!	This closes the database without
**!	flushing or syncing. This should never be used and
**!	exist only for testing purposes.
*/

static void pmird__debug_cut(INT32 args)
{
   if (THIS->db)
   {
      mird_free_structure(THIS->db);
      THIS->db=NULL;
   }
   
   pop_n_elems(args);
   push_int(0);
}

/*
**! method void _debug_check_free()
**! method void _debug_check_free(int(0..1) silent)
**!	This syncs the database and verifies
**!	the database free list. It prints stuff
**!	on stderr. It exists only for debug
**!	purpose and has no other use.
*/

static void pmird__debug_check_free(INT32 args)
{
   int silent=0;
   if (sp[-args].type==T_INT &&
       sp[-args].u.integer) silent=1;
   if (!THIS->db) pmird_no_database();
   TRY(mird_sync(THIS->db));
   mird_debug_check_free(THIS->db,silent);
   
   pop_n_elems(args);
   push_int(0);
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
   struct pmird_storage *pmird;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Transaction",1);
   
   if ( !(pmird=(struct pmird_storage*)
	  get_storage(sp[-args].u.object,mird_program)) )
      SIMPLE_BAD_ARG_ERROR("Transaction",1,"Mird object");

   add_ref(sp[-args].u.object);
   THIS->dbobj=sp[-args].u.object;
   THIS->parent=pmird;

   if (!pmird->db) pmird_no_database();

   THIS->mtr=NULL;
LOCK(THIS->parent);
   TRY(mird_transaction_new(pmird->db,&(THIS->mtr)));
UNLOCK(THIS->parent);

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
   struct mird_transaction *mtr;

   pop_n_elems(args);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

   mtr=THIS->mtr;
LOCK(THIS->parent);
   TRY(mird_transaction_close(mtr));
UNLOCK(THIS->parent);
   THIS->mtr=NULL;

   ref_push_object(THISOBJ);
}

/*
**! method void cancel()
**! method void destroy()
**!	cancels (rewinds) a transaction
*/

static void pmtr_cancel(INT32 args)
{
   pop_n_elems(args);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

LOCK(THIS->parent);
   TRY(mird_transaction_cancel(THIS->mtr));
UNLOCK(THIS->parent);
   THIS->mtr=NULL;

   ref_push_object(THISOBJ);
}

static void pmtr_destroy(INT32 args)
{
   pop_n_elems(args);

   if (THIS->mtr &&
       THIS->mtr->db)
   {
LOCK(THIS->parent);
      TRY(mird_transaction_cancel(THIS->mtr));
UNLOCK(THIS->parent);
      THIS->mtr=NULL;
   }
   else if (THIS->mtr)
   {
      mird_tr_free(THIS->mtr);
      THIS->mtr=NULL;
   }

   ref_push_object(THISOBJ);
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
   pop_n_elems(args);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

LOCK(THIS->parent);
   TRY(mird_tr_resolve(THIS->mtr));
UNLOCK(THIS->parent);

   ref_push_object(THISOBJ);
}

/*
**! method object store(int table_id,int|string key,string data)
*/

static void pmtr_store(INT32 args)
{
   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct pike_string *data;
   struct mird_transaction *mtr=THIS->mtr;

   if (args<3)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",3);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

   if (sp[1-args].type==T_INT)
   {
      get_all_args("store",args,"%i%i%S",&table_id,&hashkey,&data);
LOCK(THIS->parent);
      TRY(mird_key_store(mtr,(mird_key_t)table_id,
			 (mird_key_t)hashkey,
			 (unsigned char*)(data->str),
			 (mird_size_t)(data->len)));
UNLOCK(THIS->parent);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("store",args,"%i%S%S",&table_id,&stringkey,&data);
LOCK(THIS->parent);
      TRY(mird_s_key_store(mtr,(mird_key_t)table_id,
			   (unsigned char*)(stringkey->str),
			   (mird_size_t)(stringkey->len),
			   (unsigned char*)(data->str),
			   (mird_size_t)(data->len)));
UNLOCK(THIS->parent);
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
   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct mird_transaction *mtr=THIS->mtr;

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",2);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

   if (sp[1-args].type==T_INT)
   {
      get_all_args("delete",args,"%i%i",&table_id,&hashkey);
LOCK(THIS->parent);
      TRY(mird_key_store(mtr,(mird_key_t)table_id,
			 (mird_key_t)hashkey,
			 NULL,0));
UNLOCK(THIS->parent);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("delete",args,"%i%S",&table_id,&stringkey);
LOCK(THIS->parent);
      TRY(mird_s_key_store(mtr,(mird_key_t)table_id,
			   (unsigned char*)(stringkey->str),
			   (mird_size_t)(stringkey->len),
			   NULL,0));
UNLOCK(THIS->parent);
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
   INT_TYPE hashkey,table_id;
   struct pike_string *stringkey;
   struct mird_transaction *mtr=THIS->mtr;
   unsigned char *data;
   mird_size_t len;

   if (args<2)
      SIMPLE_TOO_FEW_ARGS_ERROR("store",2);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

   if (sp[1-args].type==T_INT)
   {
      get_all_args("fetch",args,"%i%i",&table_id,&hashkey);
LOCK(THIS->parent);
      TRY(mird_transaction_key_lookup(mtr,(mird_key_t)table_id,
				      (mird_key_t)hashkey,
				      &data,
				      &len));
UNLOCK(THIS->parent);
   }
   else if (sp[1-args].type==T_STRING)
   {
      get_all_args("fetch",args,"%i%S",&table_id,&stringkey);
LOCK(THIS->parent);
      TRY(mird_transaction_s_key_lookup(mtr,(mird_key_t)table_id,
					(unsigned char*)(stringkey->str),
					(mird_size_t)(stringkey->len),
					&data,
					&len));
UNLOCK(THIS->parent);
   }
   else
      SIMPLE_BAD_ARG_ERROR("fetch",2,"int|string");
   
   pop_n_elems(args);

   if (data)
      push_string(make_shared_binary_string(data,len));
   else
   {
      push_int(0);
      sp[-1].subtype=NUMBER_UNDEFINED;
   }
   mird_free(data);
}

/*
**! method object new_hashkey_table(int table_id);
**! method object new_stringkey_table(int table_id);
**!	creates a table in the database
*/

static void pmtr_new_hashkey_table(INT32 args)
{
   INT_TYPE table_id;

   get_all_args("new_hashkey_table",args,"%i",&table_id);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

LOCK(THIS->parent);
   TRY(mird_key_new_table(THIS->mtr,(mird_key_t)table_id));
UNLOCK(THIS->parent);

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void pmtr_new_stringkey_table(INT32 args)
{
   INT_TYPE table_id;

   get_all_args("new_hashkey_table",args,"%i",&table_id);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();

LOCK(THIS->parent);
   TRY(mird_s_key_new_table(THIS->mtr,(mird_key_t)table_id));
UNLOCK(THIS->parent);

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
   INT_TYPE table_id;

   get_all_args("delete_table",args,"%i",&table_id);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();
  
   LOCK(THIS->parent);
   TRY(mird_delete_table(THIS->mtr,(mird_key_t)table_id));
   UNLOCK(THIS->parent);
}

/*
**! method object depend_table(int table_id)
*/

static void pmtr_depend_table(INT32 args)
{
   INT_TYPE table_id;

   get_all_args("depend_table",args,"%i",&table_id);

   if (!THIS->mtr) return pmird_no_transaction();
   if (!THIS->mtr->db) return pmird_tr_no_database();
  
   LOCK(THIS->parent);
   TRY(mird_depend_table(THIS->mtr,(mird_key_t)table_id));
   UNLOCK(THIS->parent);
}

/**** hashkey table scanner ************************/

#undef THIS
#define THIS ((struct pmts_storage*)(fp->current_storage))

struct pmts_storage
{
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
   THIS->obj=sp[-args].u.object;
   THIS->pmird=pmird;
   THIS->pmtr=pmtr;

   THIS->table_id=(mird_key_t)sp[1-args].u.integer;

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

static void pmts_read(INT32 args)
{
   INT_TYPE n;
   MIRD_RES res;
   mird_size_t i;

   get_all_args("read",args,"%+",&n);

   if (THIS->pmird && !THIS->pmird->db) pmird_no_database();
   if (THIS->pmtr && !THIS->pmtr->mtr) pmird_no_transaction();
   if (THIS->pmtr && !THIS->pmtr->parent->db) pmird_tr_no_database();
   
   if (THIS->msr)
   {
      if (THIS->pmird)
	 res=mird_table_scan(THIS->pmird->db,THIS->table_id,(mird_size_t)n,
			     THIS->msr,&(THIS->msr));
      else /* pmtr */
	 res=mird_transaction_table_scan(THIS->pmtr->mtr,
					 THIS->table_id,(mird_size_t)n,
					 THIS->msr,&(THIS->msr));
   }
   else if (THIS->mssr)
   {
      if (THIS->pmird)
	 res=mird_s_table_scan(THIS->pmird->db,THIS->table_id,(mird_size_t)n,
			       THIS->mssr,&(THIS->mssr));
      else /* pmtr */
	 res=mird_transaction_s_table_scan(THIS->pmtr->mtr,
					   THIS->table_id,(mird_size_t)n,
					   THIS->mssr,&(THIS->mssr));
   }
   else /* chance */
   {
      if (THIS->pmird)
	 res=mird_table_scan(THIS->pmird->db,THIS->table_id,(mird_size_t)n,
			     NULL,&(THIS->msr));
      else /* pmtr */
	 res=mird_transaction_table_scan(THIS->pmtr->mtr,
					 THIS->table_id,(mird_size_t)n,
					 NULL,&(THIS->msr));
      if (res && res->error_no==MIRDE_WRONG_TABLE) 
      {
	 mird_free_error(res);
	 /* it's a string table, then */

	 if (THIS->pmird)
	    res=mird_s_table_scan(THIS->pmird->db,THIS->table_id,
				  (mird_size_t)n,THIS->mssr,&(THIS->mssr));
	 else /* pmtr */
	    res=mird_transaction_s_table_scan(THIS->pmtr->mtr,
					      THIS->table_id,(mird_size_t)n,
					      THIS->mssr,&(THIS->mssr));
      }
   }
   if (res) pmird_exception(res);

   pop_n_elems(args);

   if (THIS->msr)
   {
      for (i=0; i<THIS->msr->n; i++)
      {
	 push_int((INT_TYPE)(THIS->msr->tupel[i].key));
	 push_string(make_shared_binary_string(
	    THIS->msr->tupel[i].value,
	    THIS->msr->tupel[i].value_len));
      }
      f_aggregate_mapping(THIS->msr->n*2);
   }
   else if (THIS->mssr)
   {
      for (i=0; i<THIS->mssr->n; i++)
      {
	 push_string(make_shared_binary_string(
	    THIS->mssr->tupel[i].key,
	    THIS->mssr->tupel[i].key_len));
	 push_string(make_shared_binary_string(
	    THIS->mssr->tupel[i].value,
	    THIS->mssr->tupel[i].value_len));
      }
      f_aggregate_mapping(THIS->mssr->n*2);
   }
   else /* eod */
      push_int(0);
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

   ADD_FUNCTION("_debug_cut",pmird__debug_cut,tFunc(tNone,tVoid),0);
   ADD_FUNCTION("_debug_check_free",pmird__debug_check_free,
		tFunc(tNone,tVoid),0);

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
		tFunc(tNone,tObj),0);
   ADD_FUNCTION("new_hashkey_table",pmtr_new_hashkey_table,
		tFunc(tNone,tObj),0);

   mird_transaction_program=end_program();

   /* scanner */

   start_new_program();
   ADD_STORAGE(struct pmts_storage);
   set_init_callback(init_pmts);
   set_exit_callback(exit_pmts);

   ADD_FUNCTION("create",pmts_create,tFunc(tObj tInt,tVoid),0);
   ADD_FUNCTION("read",pmts_read,tFunc(tIntPos,tMap(tOr(tInt,tStr),tStr)),0);

   mird_scanner_program=end_program();

   /* all done */

#if 0
   start_new_program();
#endif

   add_program_constant("Mird",mird_program,0);
   add_program_constant("Transaction",mird_transaction_program,0);
   add_program_constant("Scanner",mird_scanner_program,0);

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
