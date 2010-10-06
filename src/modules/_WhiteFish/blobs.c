#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: blobs.c,v 1.14 2005/05/19 22:35:47 mast Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "object.h"
#include "array.h"
#include "module_support.h"

#include "config.h"

#include "whitefish.h"
#include "buffer.h"
#include "blobs.h"

static void exit_blobs_struct( );

#define HSIZE 10007

#define THIS ((struct blobs *)Pike_fp->current_storage)
#define HASH(X) (((unsigned int) (size_t)(X)) % HSIZE)

struct hash
{
  unsigned int word_data_offset;
  int current_document;
  struct buffer *buffer;
  struct hash *next;
  struct pike_string *id;
};

struct blobs
{
  int next_ind;
  int size;
  int nwords;
  struct hash *next_h;
  struct hash *hash[HSIZE];
};


static struct hash *new_hash( struct pike_string *id )
{
  struct hash *res = malloc( sizeof( struct hash ) );
  if( !res )
      Pike_error("Out of memory\n");
  res->id = id;
  add_ref(id);
  res->next = 0;
  res->buffer = wf_buffer_new();
  res->word_data_offset = 0;
  res->current_document = -1;
  return res;
}

static void insert_hash( struct blobs *d, struct hash *h )
{
  unsigned int r = HASH(h->id);
  h->next = d->hash[ r ];
  d->hash[ r ] = h;
}

static void free_hash( struct hash *h )
{
  while( h )
  {
    struct hash *n = h->next;
    if( h->buffer ) wf_buffer_free( h->buffer );
    if( h->id )    free_string( h->id );
    free( h );
    h = n;
  }
}

static struct hash *find_hash( struct blobs *d, struct pike_string *id )
{
  unsigned int r = HASH(id);
  struct hash *h = d->hash[ r ];
  while( h )
  {
    if( h->id == id )
      return h;
    h = h->next;
  }
  h = new_hash( id );
  insert_hash( d, h );
  d->nwords++;
  d->size+=sizeof( struct hash )+32;
  return h;
}

/*! @module Search
 */

/*! @class Blobs
 */

static void f_blobs_add_words( INT32 args )
/*! @decl void add_words( int docid, array(string) words, int field_id )
 *!
 *! Add all the words in the 'words' array to the blobs
 */ 
{
  INT_TYPE docid;
  struct array *words;
  INT_TYPE field_id;

  int i;
  struct blobs *blbl = THIS;
  
  get_all_args( "add_words", args, "%d%a%d",
		&docid, &words, &field_id);

  for( i = 0; i<words->size; i++ )
    if( words->item[i].type != PIKE_T_STRING )
      Pike_error("Illegal element %d in words array\n", i );
    else
    {
      struct hash *x = find_hash( blbl, words->item[i].u.string );
      if( !x->buffer )
	Pike_error("Read already called\n");
      blbl->size-=x->buffer->allocated_size;
      if( x->current_document != docid )
      {
	x->current_document = docid;
	wf_buffer_wint( x->buffer, docid );
	wf_buffer_wbyte( x->buffer, 0 );
	x->word_data_offset = x->buffer->size-1;
      }
      if( x->buffer->data[x->word_data_offset] < 255 )
      {
	unsigned short s;
	x->buffer->data[x->word_data_offset]++;
	blbl->size+=2;
	if( field_id )
	    s = (3<<14) | ((field_id-1)<<8) | (i>255?255:i);
	else
	    s = i>((1<<14)-1)?((1<<14)-1):i;
	wf_buffer_wshort( x->buffer, s );
      }
      blbl->size+=x->buffer->allocated_size;
    }
  pop_n_elems( args );
  push_int(0);
}

static void f_blobs_memsize( INT32 args )
/*! @decl int memsize()
 *!
 *! Returns the in-memory size of the blobs
 */
{
  pop_n_elems( args );
  push_int( THIS->size );
}
static void f_blobs_read( INT32 args )
/*! @decl array read();
 *!
 *! returns ({ string word_id, string blob  }) or ({0,0}) As a side-effect,
 *! this function frees the blob and the word_id, so you can only read
 *! the blobs struct once. Also, once you have called @[read],
 *! @[add_words] will no longer work as expected.
 */
{
  struct blobs *t = THIS;
  struct array *a = allocate_array( 2 );
  pop_n_elems(args);
  while( !t->next_h )
  {
    if( t->next_ind >= HSIZE )
    {
      a->item[0].type = PIKE_T_INT;
      a->item[0].u.integer = 0;
      a->item[1].type = PIKE_T_INT;
      a->item[1].u.integer = 0;
      push_array( a );
      return;
    }
    t->next_h = t->hash[ t->next_ind ];
    t->next_ind++;
  }

  a->item[0].type = PIKE_T_STRING;
  a->item[0].u.string = t->next_h->id;
  a->item[1].type = PIKE_T_STRING;
  a->item[1].u.string = make_shared_binary_string( t->next_h->buffer->data,
						   t->next_h->buffer->size );
  wf_buffer_free( t->next_h->buffer );
  t->next_h->buffer = 0;
  t->next_h->id = 0;

  push_array( a );

  t->next_h = THIS->next_h->next;
}


static int compare_wordarrays( const void *_a, const void *_b )
{
  const struct svalue *a = (struct svalue *)_a;
  const struct svalue *b = (struct svalue *)_b;
  return my_quick_strcmp( a->u.array->item[0].u.string,
			  b->u.array->item[0].u.string );
}

/*! @decl array(array(string)) read_all_sorted()
 *!
 *! returns ({({ string word1_id, string blob1  }),...}), sorted by word_id in octed order.
 *!
 *! As a side-effect,
 *! this function frees the blobs and the word_ids, so you can only read
 *! the blobs struct once. Also, once you have called @[read] or @[read_all_sorted],
 *! @[add_words] will no longer work as expected.
 */
static void f_blobs_read_all_sorted( INT32 args )
{
  struct array *g = allocate_array( THIS->nwords );
  int i;
  for( i = 0; i<THIS->nwords; i++ )
  {
    f_blobs_read(0);
    g->item[i]=Pike_sp[-1];
    Pike_sp--;
  }
  qsort( &g->item[0], THIS->nwords, sizeof(struct svalue), compare_wordarrays );
  push_array(g);
}



/*! @endclass
 */

/*! @endmodule
 */

static void init_blobs_struct( )
{
  MEMSET( THIS, 0, sizeof( struct blobs ) );
  THIS->size = sizeof( struct blobs ) + 128;
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
	       "function(int,array,int:void)",0 );
  add_function("memsize", f_blobs_memsize, "function(void:int)", 0 );
  add_function("read", f_blobs_read, "function(void:array(string))", 0);
  add_function("read_all_sorted", f_blobs_read_all_sorted, "function(void:array(array(string)))", 0);
  set_init_callback( init_blobs_struct );
  set_exit_callback( exit_blobs_struct );
  blobs_program = end_program( );
  add_program_constant( "Blobs", blobs_program, 0 );
}

void exit_blobs_program()
{
  free_program( blobs_program );
}
