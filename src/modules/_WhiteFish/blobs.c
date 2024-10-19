/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stralloc.h"
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
  int docid;
  struct array *words;
  int field_id;

  int i;
  struct blobs *blbl = THIS;

  get_all_args( NULL, args, "%d%a%d",
		&docid, &words, &field_id);

  for( i = 0; i<words->size; i++ )
    if( TYPEOF(words->item[i]) != PIKE_T_STRING )
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
      if( (unsigned char)x->buffer->data[x->word_data_offset] < 255 )
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
      SET_SVAL(a->item[0], PIKE_T_INT, NUMBER_NUMBER, integer, 0);
      SET_SVAL(a->item[1], PIKE_T_INT, NUMBER_NUMBER, integer, 0);
      push_array( a );
      return;
    }
    t->next_h = t->hash[ t->next_ind ];
    t->next_ind++;
  }

  SET_SVAL(a->item[0], PIKE_T_STRING, 0, string, t->next_h->id);
  SET_SVAL(a->item[1], PIKE_T_STRING, 0, string,
	   make_shared_binary_string0( t->next_h->buffer->data,
				       t->next_h->buffer->size ));
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
 *! Returns ({({ string word1_id, string blob1  }),...}),
 *! sorted by word_id in octet order.
 *!
 *! @note
 *! This function also frees the blobs and the word_ids, so you can only read
 *! the blobs struct once. Also, once you have called @[read] or
 *! @[read_all_sorted], @[add_words] will no longer work as expected.
 */
static void f_blobs_read_all_sorted( INT32 UNUSED(args) )
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

static void init_blobs_struct(struct object *UNUSED(o))
{
  THIS->size = sizeof( struct blobs ) + 128;
}

static void exit_blobs_struct(struct object *UNUSED(o))
{
  int i;
  for( i = 0; i<HSIZE; i++ )
    if( THIS->hash[i] )
      free_hash( THIS->hash[i] );
}

static struct program *blobs_program;
void init_blobs_program(void)
{
  start_new_program();
  ADD_STORAGE( struct blobs );
  ADD_FUNCTION("add_words",f_blobs_add_words,tFunc(tInt tArray tInt,tVoid),0 );
  ADD_FUNCTION("memsize", f_blobs_memsize, tFunc(tVoid,tInt), 0 );
  ADD_FUNCTION("read", f_blobs_read, tFunc(tVoid,tArr(tStr)), 0);
  ADD_FUNCTION("read_all_sorted", f_blobs_read_all_sorted, tFunc(tVoid,tArr(tArr(tStr))), 0);
  set_init_callback( init_blobs_struct );
  set_exit_callback( exit_blobs_struct );
  blobs_program = end_program( );
  add_program_constant( "Blobs", blobs_program, 0 );
}

void exit_blobs_program(void)
{
  free_program( blobs_program );
}
