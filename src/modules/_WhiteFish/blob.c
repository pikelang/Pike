#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: blob.c,v 1.8 2001/05/24 17:09:01 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "fsort.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"
#include "buffer.h"

/* must be included last */
#include "module_magic.h"
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
    return -1;
  b->docid = -1;
  if( b->b->rpos >= b->b->size )
  {
    if( !b->feed )
    {
      wf_buffer_clear( b->b );
      b->eof = 1;
      return -1;
    }
    push_int( b->word );
    apply_svalue( b->feed, 1 );
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
      push_int( b->word );
      apply_svalue( b->feed, 1 );
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
  return b->b->data[b->b->rpos+4];
}

Hit wf_blob_hit( Blob *b, int n )
{
  Hit hit;
  if( b->eof )
  {
    hit.type = HIT_NOTHING;
    hit.u.raw = -1;
    return hit;
  }
  else
  {
    int off = b->b->rpos + 5 + n*2;
    unsigned char h =  b->b->data[ off ];
    unsigned char l = b->b->data[ off + 1 ];
    hit.u.raw = (h<<8) | l;


    hit.type = HIT_BODY;
    if( hit.u.body.id == 3 )
    {
      hit.type = HIT_FIELD;
      if( hit.u.field.type == 63 )
	hit.type = HIT_ANCHOR;
    }
    return hit;
  }
}
int wf_blob_docid( Blob *b )
{
  if( b->eof )
    return -1;
  if( b->docid >= 0 )
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


Blob *wf_blob_new( struct svalue *feed, int word )
{
  Blob *b = malloc( sizeof( Blob ) );
  MEMSET(b, 0, sizeof(b) );
  b->word = word;
  b->feed = feed;
  b->b = wf_buffer_new();
  return b;
}

void wf_blob_free( Blob *b )
{
  if( b->b )  wf_buffer_free( b->b );
  free( b );
}








/* Pike interface to build blobs. */

#define THIS ((struct blob_data *)Pike_fp->current_object->storage)

struct hash
{
  int doc_id;
  struct hash *next;
  struct buffer *data;
};

#define HSIZE 4711

struct blob_data
{
  int size;
  struct hash *hash[HSIZE];
};

static struct program *blob_program;
static struct hash *low_new_hash( int doc_id )
{
  struct hash *res =  malloc( sizeof( struct hash ) );
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
  int r = h->doc_id % HSIZE;
  h->next = d->hash[r];
  d->hash[r] = h;
}

static struct hash *find_hash( struct blob_data *d, int doc_id )
{
  int r = doc_id % HSIZE;
  struct hash *h = d->hash[ r ];
  
  while( h )
  {
    if( h->doc_id == doc_id )
      return h;
    h = h->next;
  }
  d->size++;
  h = new_hash( doc_id );
  insert_hash( d, h );
  return h;
}

static void free_hash( struct hash *h )
{
  while( h )
  {
    struct hash *n = h->next;
    if( h->data ) wf_buffer_free( h->data );
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
    int i;
    struct hash *h = find_hash( d, docid );
    /* Make use of the fact that this blob is currently empty, and
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

static void f_blob_remove( INT32 args )
{
  int doc_id = sp[-1].u.integer;

  pop_n_elems(args);

  int r = doc_id % HSIZE;
  struct hash *h = THIS->hash[r], *p = 0;
  
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

static void f_blob_add( INT32 args )
{
  int docid = sp[-2].u.integer;
  int hit = sp[-1].u.integer;

  _append_hit( THIS, docid, hit );

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

int cmp_hit( char *a, char *b )
{
  /* a and b in NBO, and not aligned */
  unsigned short ai = a[0]<<8 | a[1],
                 bi = b[0]<<8 | b[1];
  return ai < bi ? -1 : ai == bi ? 0 : 1 ;
}

#define CMP(X,Y) cmp_hit( X, Y )
#define STEP(X,Y)  X+(Y*sizeof(short))
#define SWAP(X,Y)  do {\
  /* swap 2 bytes */\
  tmp = (X)[0];  (X)[0] = (Y)[0];  (Y)[0] = tmp;\
  tmp = (X)[1];  (X)[1] = (Y)[1];  (Y)[1] = tmp;\
} while(0)

static void f_blob__cast( INT32 args )
{
  struct zipp {
    int id;
    struct buffer *b;
  } *zipp;
  int i, zp=0;
  struct hash *h;
  struct buffer *res;

  zipp = malloc( THIS->size * sizeof( zipp[0] ) );

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
      short *data = (short *)malloc( nh * 2 );
      MEMCPY( data,  zipp[i].b->data+5, nh * 2 );
      fsort( data, nh, 2, (void *)cmp_hit );
      MEMCPY( zipp[i].b->data+5, data, nh * 2 );
      free( data );
    }
  }
  res = wf_buffer_new();

  wf_buffer_set_empty( res );

  for( i = 0; i<zp; i++ )
  {
    int j;
    wf_buffer_append( res, zipp[i].b->data, zipp[i].b->size );
/*     for( j = 0; j<4; j++ ) */
/*       printf( "%02x", ((unsigned char *)zipp[i].b->data)[j] ); */
/*     printf( " %02x ", ((unsigned char *)zipp[i].b->data)[4] ); */
/*     for( j = 5; j<zipp[i].b->size; j+=2 ) */
/*       printf( " %02x%02x", ((unsigned char*)zipp[i].b->data)[j], */
/* 	      ((unsigned char *)zipp[i].b->data)[j+1] ); */
/*     printf("\n"); */
  }
  free( zipp );

  /*exit_blob_struct();*/


  pop_n_elems( args );
  push_string( make_shared_binary_string( res->data, res->size ) );
  wf_buffer_free( res );
}

static void init_blob_struct( )
{
  MEMSET( THIS, 0, sizeof( struct blob_data ) );
}

static void exit_blob_struct( )
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
  add_function( "add", f_blob_add, "function(int,int:void)",0 );
  add_function( "remove", f_blob_remove, "function(int:void)",0 );
  add_function( "data", f_blob__cast, "function(void:string)", 0 );
  set_init_callback( init_blob_struct );
  set_exit_callback( exit_blob_struct );
  blob_program = end_program( );
  add_program_constant( "Blob", blob_program, 0 );
}

void exit_blob_program()
{
  free_program( blob_program );
}
