#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: blob.c,v 1.34 2004/10/26 20:40:38 grubba Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "fsort.h"
#include "array.h"
#include "module_support.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"
#include "buffer.h"

#define sp Pike_sp

static void exit_blob_struct( );

/*
  +-----------+----------+---------+---------+---------+
  | docid: 32 | nhits: 8 | hit: 16 | hit: 16 | hit: 16 |...
  +-----------+----------+---------+---------+---------+
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
    apply_svalue( b->feed, 2 );
    if( sp[-1].type != T_STRING )
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
      apply_svalue( b->feed, 2 );
      if( sp[-1].type != T_STRING )
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
  Blob *b = malloc( sizeof( Blob ) );
  MEMSET(b, 0, sizeof(Blob) );
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

#define HSIZE 13

struct blob_data
{
  int size;
  size_t memsize;
  struct hash *hash[HSIZE];
};

struct program *blob_program;
static struct hash *low_new_hash( int doc_id )
{
  struct hash *res = xalloc( sizeof( struct hash ) );
  res->doc_id = doc_id;
  res->next = 0;
  res->data = wf_buffer_new();
  wf_buffer_set_empty( res->data );
  return res;
}

static struct hash *new_hash( int doc_id )
{
  struct hash *res = low_new_hash( doc_id );
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
    struct hash *h = find_hash( d, docid );
    /* Make use of the fact that this dochash should be empty, and
     * assume that the incoming data is valid
     */
    wf_buffer_rewind_r( b, 5 );
    wf_buffer_rewind_w( h->data, -1 );
    wf_buffer_memcpy( h->data, b, nhits*2+5 );    
  }
  wf_buffer_free( b );
}

static void f_blob_create( INT32 args )
{
  if( args )
  {
    struct pike_string *s = sp[-1].u.string;
    if( sp[-1].type != PIKE_T_STRING )
      Pike_error("Expected a string\n");
    _append_blob( THIS, s );
  }
}

static void f_blob_merge( INT32 args )
{
  if(!args || sp[-1].type != PIKE_T_STRING )
    Pike_error("Expected a string\n");
  _append_blob( THIS, sp[-1].u.string );
}

static void f_blob_remove( INT32 args )
{
  int doc_id;
  unsigned int r;
  struct hash *h;
  struct hash *p = NULL;

  get_all_args("remove", args, "%d", &doc_id);
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

static void f_blob_remove_list( INT32 args )
{
  struct array *docs;
  int i;

  get_all_args("remove_list", args, "%a", &docs);

  for( i = 0; i<docs->size; i++ )
  {
    int doc_id;
    unsigned int r;
    struct hash *h;
    struct hash *p = NULL;

    if (docs->item[i].type != T_INT)
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
  push_int( 0 );
}

void wf_blob_low_add( struct object *o,
		      int docid, int field, int off )
{
  unsigned short s;
  switch( field )
  {
    case 0:
      s = off>((1<<14)-1)?((1<<14)-1):off;
      break;
    default:
      s = (3<<14) | ((field-1)<<8) | (off>255?255:off);
      break;
  }
  _append_hit( ((struct blob_data *)o->storage), docid, s );
}

static void f_blob_add( INT32 args )
{
  int docid;
  int field;
  int off;
  get_all_args("add", args, "%d%d%d", &docid, &field, &off);
  wf_blob_low_add( Pike_fp->current_object, docid, field, off );
  pop_n_elems( args );
  push_int( 0 );
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

static void f_blob_memsize( INT32 args )
{
  pop_n_elems(args);
  push_int( wf_blob_low_memsize( Pike_fp->current_object ) );
}

static void f_blob__cast( INT32 args )
{
  struct zipp {
    int id;
    struct buffer *b;
  } *zipp;
  int i, zp=0;
  struct hash *h;
  struct buffer *res;

  zipp = xalloc( THIS->size * sizeof( zipp[0] ) );

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
    int nh;
#ifdef PIKE_DEBUG
    if( zipp[i].b->size < 7 )
      fatal( "Expected at least 7 bytes! (1 hit)\n");
    else
#endif
    if((nh = zipp[i].b->data[4]) > 1 )
    {
#ifdef HANDLES_UNALIGNED_READ
      fsort( zipp[i].b->data+5, nh, 2, (void *)cmp_hit );
#else
      short *data = (short *)malloc( nh * 2 );
      MEMCPY( data,  zipp[i].b->data+5, nh * 2 );
      fsort( data, nh, 2, (void *)cmp_hit );
      MEMCPY( zipp[i].b->data+5, data, nh * 2 );
      free( data );
#endif
    }
  }
  res = wf_buffer_new();

  wf_buffer_set_empty( res );

  for( i = 0; i<zp; i++ )
    wf_buffer_append( res, zipp[i].b->data, zipp[i].b->size );

  free( zipp );

  exit_blob_struct(); /* Clear this buffer */
  pop_n_elems( args );
  push_string( make_shared_binary_string( res->data, res->size ) );
  wf_buffer_free( res );
}

static void init_blob_struct(struct object *o)
{
  MEMSET( THIS, 0, sizeof( struct blob_data ) );
}

static void exit_blob_struct(struct object *o)
{
  int i;
  for( i = 0; i<HSIZE; i++ )
    if( THIS->hash[i] )
      free_hash( THIS->hash[i] );
  init_blob_struct();
}

void init_blob_program()
{
  start_new_program();
  ADD_STORAGE( struct blob_data );
  add_function( "create", f_blob_create, "function(string|void:void)", 0 );
  add_function( "merge", f_blob_merge, "function(string:void)", 0 );
  add_function( "add", f_blob_add, "function(int,int,int:void)",0 );
  add_function( "remove", f_blob_remove, "function(int:void)",0 );
  add_function( "remove_list", f_blob_remove_list,
		"function(array(int):void)",0 );
  add_function( "data", f_blob__cast, "function(void:string)", 0 );
  add_function( "memsize", f_blob_memsize, "function(void:int)", 0 );
  set_init_callback( init_blob_struct );
  set_exit_callback( exit_blob_struct );
  blob_program = end_program( );
  add_program_constant( "Blob", blob_program, 0 );
}

void exit_blob_program()
{
  free_program( blob_program );
}
