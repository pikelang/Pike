#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: blobs.c,v 1.3 2001/05/26 14:11:35 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "array.h"
#include "operators.h"
#include "module_support.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"
#include "buffer.h"
#include "blobs.h"

/* must be included last */
#include "module_magic.h"
static void exit_blobs_struct( );

#define HSIZE 4711

extern struct program *blob_program;

struct hash
{
  struct object *bl;
  struct hash *next;
  int id;
};

struct blobs
{
  int next_ind;
  struct hash *next_h;
  struct hash *hash[HSIZE];
};

#define THIS ((struct blobs *)Pike_fp->current_object->storage)

static struct hash *new_hash( int id )
{
  struct hash *res =  malloc( sizeof( struct hash ) );
  res->id = id;
  res->next = 0;
  res->bl = clone_object( blob_program,0 );
  return res;
}

static void insert_hash( struct blobs *d, struct hash *h )
{
  int r = h->id % HSIZE;
  h->next = d->hash[ r ];
  d->hash[ r ] = h;
}

static void free_hash( struct hash *h )
{
  while( h )
  {
    struct hash *n = h->next;
    if( h->bl ) free_object( h->bl );
    free( h );
    h = n;
  }
}

static struct hash *find_hash( struct blobs *d, int id )
{
  int r = id % HSIZE;
  struct hash *h = d->hash[ r ];
  while( h )
  {
    if( h->id == id )
      return h;
    h = h->next;
  }
  h = new_hash( id );
  insert_hash( d, h );
  return h;
}

static void f_blobs_add_words( INT32 args )
/*
 *! @decl void add_words( int docid, array(int) words, int field_id,
 *!                       int link_hash )
 *!
 *! Add all the words in the 'words' array to the blobs
 */ 
{
  INT_TYPE docid;
  struct array *words;
  INT_TYPE field_id;
  INT_TYPE link_hash;

  int i;
  struct blobs *blbl = THIS;
  
  get_all_args( "add_words", args, "%d%a%d%d",
		&docid, &words, &field_id, &link_hash );

  for( i = 0; i<words->size; i++ )
  {
    struct hash *h= find_hash( blbl, words->item[i].u.integer );
    wf_blob_low_add( h->bl, docid, field_id, link_hash, i );
  }
  pop_n_elems( args );
  push_int(0);
}

static void f_blobs_add_words_hash( INT32 args )
/*
 *! @decl void add_words_hash( int docid, array(string) words, int field_id, @
 *!                             int link_hash )
 *!
 *! Like add_words, but also hash the words using the normal hash()
 *! function in pike.
 */ 
{
  INT_TYPE docid;
  struct array *words;
  INT_TYPE field_id;
  INT_TYPE link_hash;

  int i;
  struct blobs *blbl = THIS;
  
  get_all_args( "add_words", args, "%d%a%d%d",
		&docid, &words, &field_id, &link_hash );


  for( i = 0; i<words->size; i++ )
  {
    struct pike_string *s = words->item[i].u.string;
    int w = simple_hashmem( s->str, s->len<<s->size_shift, 100<<s->size_shift );
    struct hash *h = find_hash( blbl, w );
    wf_blob_low_add( h->bl, docid, field_id, link_hash, i );
  }

  pop_n_elems( args );
  push_int(0);
}

static void f_blobs_memsize( INT32 args )
/*
 *! @decl int memsize()
 *!
 *! Returns the in-memory size of the blobs
 */
{
  int size = HSIZE*sizeof(void *);
  int i;
  struct hash *h;
  struct blobs *bl = THIS;
  
  for( i = 0; i<HSIZE; i++ )
  {
    h = bl->hash[i];
    while( h )
    {
      size += wf_blob_low_memsize( h->bl );
      h = h->next;
    }
  }
  pop_n_elems(args);
  push_int( size );
}


static void f_blobs_read( INT32 args )
/*
 *! @decl array read();
 *!
 *! returns ({ int word_id, @[Blob] b }) or 0
 */
{
  struct blobs *t = THIS;
  while( !t->next_h )
  {
    if( t->next_ind >= HSIZE )
    {
      pop_n_elems( args );

      push_int( 0 );
      push_int( 0 );
      push_array(aggregate_array( 2 ));
      return;
    }
    t->next_h = t->hash[ t->next_ind ];
    t->next_ind++;
  }
  pop_n_elems( args );

  push_int( t->next_h->id );
  ref_push_object( t->next_h->bl );
  push_array(aggregate_array( 2 ));

  t->next_h = THIS->next_h->next;
}

static void init_blobs_struct( )
{
  MEMSET( THIS, 0, sizeof( struct blobs ) );
}

static void exit_blobs_struct( )
{
  int i;
  for( i = 0; i<HSIZE; i++ )
    if( THIS->hash[i] )
      free_hash( THIS->hash[i] );
  init_blobs_struct();
}

static struct program *blobs_program;
void init_blobs_program()
{
  start_new_program();
  ADD_STORAGE( struct blobs );
  add_function("add_words",f_blobs_add_words,
	       "function(int,array,int,int:void)",0 );
  add_function("add_words_hash",f_blobs_add_words_hash,
	       "function(int,array,int,int:void)",0 );
  add_function("memsize", f_blobs_memsize, "function(void:int)", 0 );
  add_function("read", f_blobs_read, "function(void:array(int|object))", 0 );
  set_init_callback( init_blobs_struct );
  set_exit_callback( exit_blobs_struct );
  blobs_program = end_program( );
  add_program_constant( "Blobs", blobs_program, 0 );
}

void exit_blobs_program()
{
  free_program( blobs_program );
}
