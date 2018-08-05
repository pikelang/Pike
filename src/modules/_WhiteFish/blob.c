/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stralloc.h"
#include "interpret.h"
#include "program.h"
#include "object.h"
#include "array.h"
#include "module_support.h"
#include "fsort.h"
#include "bignum.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"
#include "buffer.h"

#define sp Pike_sp

static void exit_blob_struct(struct object *o);

/*
  +-----------+----------+---------+---------+---------+
  | docid: 32 | nhits: 8 | hit: 16 | hit: 16 | hit: 16 |...
  +-----------+----------+---------+---------+---------+

  For hit < 0xc000, hit in body (HIT_BODY).
  Note: Old wf_buf_low_add() had a bug where the max was 0x3fff.

             +----+---------------+--------+
  Otherwise: | 11 | field_type: 6 | hit: 8 | (HIT_FIELD).
             +----+---------------+--------+

  cf wf_blob_hit() and wf_buf_low_add().
*/

int wf_blob_next( Blob *b )
{
  /* Find the next document ID */
  if( b->eof )
    return 0;

  b->docid = 0;
  if( b->b->rpos >= b->b->size )
  {
    if( !b->feed )
    {
      wf_buffer_clear( b->b );
      b->eof = 1;
      return -1;
    }
    ref_push_string( b->word );
    push_int( b->docid );
    push_int64( (INT64) b );
    apply_svalue( b->feed, 3 );
    if( TYPEOF(sp[-1]) != T_STRING )
    {
      b->eof = 1;
      return -1;
    }
    wf_buffer_set_pike_string( b->b, sp[-1].u.string, 1 );
  }
  else
  {
    /* FF past current docid */
    b->b->rpos += 4 + 1 + 2*wf_blob_nhits( b );
    if( b->b->rpos >= b->b->size )
    {
      if( !b->feed )
      {
	wf_buffer_clear( b->b );
	b->eof = 1;
	return -1;
      }
      ref_push_string( b->word );
      push_int( b->docid );
      push_int64( (INT64) b );
      apply_svalue( b->feed, 3 );
      if( TYPEOF(sp[-1]) != T_STRING )
      {
	b->eof = 1;
	return -1;
      }
      wf_buffer_set_pike_string( b->b, sp[-1].u.string, 1 );
    }
  }
  return wf_blob_docid( b );
}

int wf_blob_eof( Blob *b )
{
  if( b->eof )
    return 1;
  return 0;
}

int wf_blob_nhits( Blob *b )
{
  if( b->eof ) return 0;
  return ((unsigned char *)b->b->data)[b->b->rpos+4];
}

int wf_blob_hit_raw( Blob *b, int n )
{
  if( b->eof )
    return 0;
  else
  {
    int off = b->b->rpos + 5 + n*2;
    return (b->b->data[ off ]<<8) | b->b->data[ off + 1 ];
  }
}

Hit wf_blob_hit( Blob *b, int n )
{
  Hit hit;
  if( b->eof )
  {
    hit.type = HIT_NOTHING;
    hit.raw = 0;
    return hit;
  }
  else
  {
    int off = b->b->rpos + 5 + n*2;
    unsigned char h =  b->b->data[ off ];
    unsigned char l = b->b->data[ off + 1 ];
    unsigned short ht= (h<<8) | l;
    hit.raw = ht;
    if( (ht>>14) == 3 )
    {
      hit.type = HIT_FIELD;
      hit.u.field._pad = 3;
      hit.u.field.type = (ht>>8) & 63;
      hit.u.field.pos = ht&255;
    }
    else
    {
      hit.type = HIT_BODY;
      hit.u.body.pos = ht;
      hit.u.body.id = 0;
    }
    return hit;
  }
}
int wf_blob_docid( Blob *b )
{
  if( b->eof )
    return -1;
  if( b->docid > 0 )
    return b->docid;
  else
  {
    int off = b->b->rpos;
    unsigned char hh =  b->b->data[ off ];
    unsigned char hl = b->b->data[ off+1 ];
    unsigned char lh =  b->b->data[ off+2 ];
    unsigned char ll = b->b->data[ off+3 ];
    return b->docid = ((((((hh<<8) | hl)<<8) | lh)<<8) | ll);
  }
}


Blob *wf_blob_new( struct svalue *feed, struct pike_string *word )
{
  Blob *b = calloc( 1, sizeof( Blob ) );
  b->word = word;
  if( word )
    add_ref(word);
  b->feed = feed;
  b->b = wf_buffer_new();
  return b;
}

void wf_blob_free( Blob *b )
{
  if( b->b )
    wf_buffer_free( b->b );
  if( b->word )
    free_string( b->word );
  free( b );
}

/* Pike interface to build blobs. */

#define THIS ((struct blob_data *)Pike_fp->current_storage)

struct hash
{
  int doc_id;
  struct hash *next;
  struct buffer *data;
};

#define HSIZE 101

struct blob_data
{
  int size;
  size_t memsize;
  struct hash *hash[HSIZE];
};

struct program *blob_program;
static struct hash *low_new_hash( int doc_id )
{
  struct hash *res = ALLOC_STRUCT( hash );
  res->doc_id = doc_id;
  res->next = 0;
  res->data = wf_buffer_new();
  wf_buffer_set_empty( res->data );
  return res;
}

static struct hash *new_hash( int doc_id )
{
  struct hash *res =  low_new_hash( doc_id );
  wf_buffer_wint( res->data, doc_id );
  wf_buffer_wbyte( res->data, 0 );
  return res;
}

static void insert_hash( struct blob_data *d, struct hash *h )
{
  unsigned int r = ((unsigned int)h->doc_id) % HSIZE;
  h->next = d->hash[r];
  d->hash[r] = h;
}

static struct hash *find_hash( struct blob_data *d, int doc_id )
{
  unsigned int r = ((unsigned int)doc_id) % HSIZE;
  struct hash *h = d->hash[ r ];

  while( h )
  {
    if( h->doc_id == doc_id )
      return h;
    h = h->next;
  }
  d->size++;
  h = new_hash( doc_id );
  if( d->memsize )
    d->memsize+=sizeof( struct hash )+32;
  insert_hash( d, h );
  return h;
}

static void free_hash( struct hash *h )
{
  while( h )
  {
    struct hash *n = h->next;
    if( h->data )
      wf_buffer_free( h->data );
    free( h );
    h = n;
  }
}

static void _append_hit( struct blob_data *d, int doc_id, int hit )
{
  struct hash *h = find_hash( d, doc_id );

  int nhits = ((unsigned char *)h->data->data)[4];

  /* Max 255 hits */
  if( nhits < 255 )
  {
    if( d->memsize ) d->memsize+=8;
    wf_buffer_wshort( h->data, hit );
    ((unsigned char *)h->data->data)[4] = nhits+1;
  }
}

static void _append_blob( struct blob_data *d, struct pike_string *s )
{
  struct buffer *b = wf_buffer_new();
  wf_buffer_set_pike_string( b, s, 1 );
  while( !wf_buffer_eof( b ) )
  {
    int docid = wf_buffer_rint( b );
    int nhits = wf_buffer_rbyte( b );
    int remaining = b->size - b->rpos;
    int orig_rpos = b->rpos;
    int i;
    int prev;
    struct hash *h;
    if (nhits > (remaining>>1)) {
      fprintf(stderr, "Invalid blob entry for doc 0x%08x: %d hits of %d missing.\n",
	      (unsigned INT32)docid, nhits - (remaining>>1), nhits);
      nhits = remaining>>1;
      remaining = -1;
    }
    if (!nhits) {
      fprintf(stderr, "Invalid blob entry for document 0x%08x (0 hits!).\n",
	      (unsigned INT32)docid);
      break;
    }

    /* Perform some minimal validation of the entry. */
    prev = -1;
    for (i = 0; i < nhits; i++) {
      int val = wf_buffer_rshort(b);
      if (prev == val) {
	/* Check if we're at max hit for the field. */
	if ((val < 0xbfff) || ((val & 0xff) != 0xff)) {
	  /* Probably some junk like 2020202020202020202020... */
	  /* Skip this entry and remainder of blob. */
	  if (val == 0x3fff) {
	    /* Special case for common broken max due to broken
	     * wf_buf_low_add().
	     */
	    continue;
	  }
	  fprintf(stderr,
		  "Duplicate hits in blob entry for document 0x%08x: 0x%04x.\n",
		  (unsigned INT32)docid, val);
	  remaining = -1;
	  nhits = 0;
	  break;
	}
      }
      prev = val;
    }

    /* Restore read position. */
    b->rpos = orig_rpos;

    if (nhits) {
      h = find_hash( d, docid );
      /* Make use of the fact that this dochash should be empty, and
       * assume that the incoming data is valid
       */
      wf_buffer_rewind_w( h->data, 1 );
      wf_buffer_wbyte(h->data, nhits);
      wf_buffer_memcpy( h->data, b, nhits*2 );
    }
    if (remaining < 0) break;
  }
  wf_buffer_free( b );
}

/*! @module Search
 */

/*! @class Blob
 */

/*! @decl void create(void|string initial)
 */

static void f_blob_create( INT32 args )
{
  if( args )
  {
    struct pike_string *s = sp[-1].u.string;
    if( TYPEOF(sp[-1]) != PIKE_T_STRING )
      Pike_error("Expected a string\n");
    _append_blob( THIS, s );
    pop_n_elems(args);
  }
}

/*! @decl void merge(string data)
 */

static void f_blob_merge( INT32 args )
{
  if(!args || TYPEOF(sp[-1]) != PIKE_T_STRING )
    Pike_error("Expected a string\n");
  _append_blob( THIS, sp[-1].u.string );
}

/*! @decl void remove(int doc_id)
 */

static void f_blob_remove( INT32 args )
{
  int doc_id;
  unsigned int r;
  struct hash *h;
  struct hash *p = NULL;

  get_all_args(NULL, args, "%d", &doc_id);
  r = ((unsigned int)doc_id) % HSIZE;
  h = THIS->hash[r];

  pop_n_elems(args);

  while( h )
  {
    if( h->doc_id == doc_id )
    {
      if( p )
	p->next = h->next;
      else
	THIS->hash[ r ] = h->next;
      if( h->data )
	wf_buffer_free( h->data );
      free( h );
      THIS->size--;
      push_int(1);
      return;
    }
    p = h;
    h = h->next;
  }

  push_int(0);
}

/*! @decl void remove_list(array(int) docs)
 */

static void f_blob_remove_list( INT32 args )
{
  struct array *docs;
  int i;

  get_all_args(NULL, args, "%a", &docs);

  for( i = 0; i<docs->size; i++ )
  {
    int doc_id;
    unsigned int r;
    struct hash *h;
    struct hash *p = NULL;

    if (TYPEOF(docs->item[i]) != T_INT)
      Pike_error("Bad argument 1 to remove_list, expected array(int).\n");

    doc_id = docs->item[i].u.integer;
    r = ((unsigned int)doc_id) % HSIZE;
    h = THIS->hash[r];

    while( h )
    {
      if( h->doc_id == doc_id )
      {
	if( p )
	  p->next = h->next;
	else
	  THIS->hash[ r ] = h->next;
	h->next = 0;
	free_hash( h );
	THIS->size--;
	break;
      }
      p = h;
      h = h->next;
    }
  }
  pop_n_elems(args);
}

void wf_blob_low_add( struct object *o,
		      int docid, int field, int off )
{
  unsigned short s;
  switch( field )
  {
    case 0:
      /* Body. 0 <= hits <= 0xbfff */
      s = off>((3<<14)-1)?((3<<14)-1):off;
      break;
    default:
      /* Field. 0b11 | (field-1):6 | off:8 */
      s = (3<<14) | ((field-1)<<8) | (off>255?255:off);
      break;
  }
  _append_hit( ((struct blob_data *)o->storage), docid, s );
}

/*! @decl void add(int docid, int field, int offset)
 */

static void f_blob_add( INT32 args )
{
  int docid;
  int field;
  int off;
  get_all_args(NULL, args, "%d%d%d", &docid, &field, &off);
  wf_blob_low_add( Pike_fp->current_object, docid, field, off );
  pop_n_elems( args );
}

int cmp_zipp( void *a, void *b )
{
  /* a and b in HBO, and aligned */
  int ai = *(int *)((int *)a);
  int bi = *(int *)((int *)b);
  return ai < bi ? -1 : ai == bi ? 0 : 1 ;
}

int cmp_hit( unsigned char *a, unsigned char *b )
{
  /* a and b in NBO, and not aligned */
  unsigned short ai = (a[0]<<8) | a[1], bi = (b[0]<<8) | b[1];
  return ai < bi ? -1 : ai == bi ? 0 : 1 ;
}

#define CMP(X,Y) cmp_hit( X, Y )
#define STEP(X,Y)  X+(Y*sizeof(short))
#define SWAP(X,Y)  do {\
  /* swap 2 bytes */\
  tmp = (X)[0];  (X)[0] = (Y)[0];  (Y)[0] = tmp;\
  tmp = (X)[1];  (X)[1] = (Y)[1];  (Y)[1] = tmp;\
} while(0)

size_t wf_blob_low_memsize( struct object *o )
{
  struct blob_data *tt = ((struct blob_data *)o->storage);

  if( tt->memsize )
    return tt->memsize;
  else
  {
    struct hash *h;
    size_t size = HSIZE*sizeof(void *);
    int i;
    for( i = 0; i<HSIZE; i++ )
    {
      h = tt->hash[i];
      while( h )
      {
	size+=sizeof(struct hash)+sizeof(struct buffer )+
	  h->data->allocated_size;
	h = h->next;
      }
    }
    return tt->memsize = size;
  }
}

/*! @decl int memsize()
 */

static void f_blob_memsize( INT32 args )
{
  pop_n_elems(args);
  push_int( wf_blob_low_memsize( Pike_fp->current_object ) );
}

/*! @decl string data()
 */

static void f_blob__cast( INT32 args )
{
  struct zipp {
    int id;
    struct buffer *b;
  } *zipp;
  int i, zp=0;
  struct hash *h;
  struct buffer *res;

  zipp = xalloc( THIS->size * sizeof( zipp[0] ) + 1);

  for( i = 0; i<HSIZE; i++ )
  {
    h = THIS->hash[i];
    while( h )
    {
      zipp[ zp ].id = h->doc_id;
      zipp[ zp ].b = h->data;
      zp++;
      h = h->next;
    }
  }

  /* 1: Sort the blobs */
  if( zp > 1 )
    fsort( zipp, zp, sizeof( zipp[0] ), (void *)cmp_zipp );

  /* 2: Sort in the blobs */
  for( i = 0; i<zp; i++ )
  {
    unsigned int nh;
#ifdef PIKE_DEBUG
    /* NB: As new_hash() returns a zero-length hash struct (ie 5 bytes),
     *     requiring 7 bytes is a bit excessive.
     */
    if( zipp[i].b->size < 5 ) {
      fprintf(stderr, "zipp[%d]: doc_id: %d, b: %p (%d bytes)\n",
	      i, zipp[i].id, zipp[i].b->data, zipp[i].b->size);
      Pike_fatal( "Expected at least 5 bytes! (0 hits)\n");
    }
#endif
    if((nh = zipp[i].b->data[4]) > 1 )
    {
#ifdef HANDLES_UNALIGNED_READ
#ifdef PIKE_DEBUG
      if (nh<<1 != zipp[i].b->size-5) {
	Pike_fatal("Unexpected blob size: %d != %d\n",
		   nh<<1, zipp[i].b->size-5);
      }
#endif
      fsort( zipp[i].b->data+5, nh, 2, (void *)cmp_hit );
#else
      short *data = malloc( nh * 2 );
#ifdef PIKE_DEBUG
      if (nh<<1 != zipp[i].b->size-5) {
	Pike_fatal("Unexpected blob size: %d != %d\n",
		   nh<<1, zipp[i].b->size-5);
      }
#endif
      memcpy( data,  zipp[i].b->data+5, nh * 2 );
      fsort( data, nh, 2, (void *)cmp_hit );
      memcpy( zipp[i].b->data+5, data, nh * 2 );
      free( data );
#endif
    }
  }
  res = wf_buffer_new();

  wf_buffer_set_empty( res );

  for( i = 0; i<zp; i++ )
    wf_buffer_append( res, zipp[i].b->data, zipp[i].b->size );

  free( zipp );

  exit_blob_struct(NULL); /* Clear this buffer */
  pop_n_elems( args );
  push_string( make_shared_binary_string( (char *)res->data, res->size ) );
  wf_buffer_free( res );
}

/*! @endclass
 */

/*! @endmodule
 */

/* NB: This function is called directly from f_blob__cast() above,
 *     and thus needs to be prepared to be called again.
 */
static void exit_blob_struct(struct object * UNUSED(o))
{
  int i;
  for( i = 0; i<HSIZE; i++ )
    if( THIS->hash[i] )
      free_hash( THIS->hash[i] );
  /* Prepare for next use. */
  memset(THIS, 0, sizeof(struct blob_data));
}

void init_blob_program(void)
{
  start_new_program();
  ADD_STORAGE( struct blob_data );
  ADD_FUNCTION( "create", f_blob_create, tFunc(tOr(tStr,tVoid),tVoid), 0 );
  ADD_FUNCTION( "merge", f_blob_merge, tFunc(tStr,tVoid), 0 );
  ADD_FUNCTION( "add", f_blob_add, tFunc(tInt tInt tInt,tVoid),0 );
  ADD_FUNCTION( "remove", f_blob_remove, tFunc(tInt,tVoid),0 );
  ADD_FUNCTION( "remove_list", f_blob_remove_list, tFunc(tArr(tInt),tVoid), 0);
  ADD_FUNCTION( "data", f_blob__cast, tFunc(tVoid,tStr), 0 );
  ADD_FUNCTION( "memsize", f_blob_memsize, tFunc(tVoid,tInt), 0 );
  set_exit_callback( exit_blob_struct );
  blob_program = end_program( );
  add_program_constant( "Blob", blob_program, 0 );
}

void exit_blob_program(void)
{
  free_program( blob_program );
}
