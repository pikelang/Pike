#include <math.h>

#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: whitefish.c,v 1.35 2004/08/07 15:26:57 js Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "object.h"
#include "array.h"
#include "module_support.h"
#include "module.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"
#include "blobs.h"
#include "linkfarm.h"

/* 7.2 compatibility stuff. */

#ifndef PIKE_MODULE_INIT
/* must be included last */
#include "module_magic.h"
#define PIKE_MODULE_INIT void pike_module_init(void)
#define PIKE_MODULE_EXIT void pike_module_exit(void)
#endif

struct  tofree
{
  Blob **blobs;
  Blob **tmp;
  int nblobs;
  struct object *res;
};

static void free_stuff( void *_t )
{
  struct tofree *t= (struct tofree *)_t;
  int i;
  if( t->res ) free_object( t->res );
  for( i = 0; i<t->nblobs; i++ )
    wf_blob_free( t->blobs[i] );
  free(t->blobs);
  free( t->tmp );
  free( t );
}

#define OFFSET(X) \
 (X.type == HIT_BODY?X.u.body.pos:X.u.field.pos)

#define DOFF(X)  _distance_f(X)
#define MOFF(X)  (X.type==HIT_BODY?0:X.u.field.type+1)

static int _distance_f( int distance )
{
  if( distance < 2 )   return 0;
  if( distance < 6 )   return 1;
  if( distance < 11 )  return 2;
  if( distance < 22 )  return 3;
  if( distance < 42 )  return 4;
  if( distance < 82 )  return 5;
  if( distance < 161 ) return 6;
  return 7;
}


static void handle_hit( Blob **blobs,
			int nblobs,
			struct object *res,
			int docid,
			double *field_c[65],
			double *prox_c[8],
			double mc, double mp,
			int cutoff )
{
  int i, j, k, end = 0;
  Hit *hits = malloc( nblobs * sizeof(Hit) );
  unsigned char *nhits = malloc( nblobs );
  unsigned char *pos = malloc( nblobs );

  int matrix[65][8];

  MEMSET(matrix, 0, sizeof(matrix) );
  MEMSET(hits, 0, nblobs * sizeof(Hit) );
  MEMSET(pos, 0, nblobs );

  for( i = 0; i<nblobs; i++ )
    nhits[i] = wf_blob_nhits( blobs[i] );


  for( i = 0; i<nblobs; i++ )
  {
    MEMSET( pos, 0, nblobs );
    for( j = 0; j<nhits[i]; j++ )
    {
      hits[i] = wf_blob_hit( blobs[i], j );
      matrix[MOFF(hits[i])][3]++;

      /* forward the other positions */
      for( k = 0; k<nblobs; k++ )
	if( k != i &&  pos[ k ] < nhits[ k ] )
	{
	  while( (hits[k].raw < hits[i].raw) && (pos[ k ] < nhits[ k ]))
	    hits[k] = wf_blob_hit( blobs[k], pos[k]++ );
	  if( (pos[ k ] < nhits[ k ]) && hits[k].type == hits[i].type )
	    matrix[MOFF(hits[i])][DOFF(OFFSET(hits[k])-OFFSET(hits[i]))]+=4;
	}
    }
  }
  
  free( pos );
  free( nhits );
  free( hits );
  /* Now we have our nice matrix. Time to do some multiplication */

  {
    double accum = 0.0, fc, pc;
    int accum_i;
    for( i = 0; i<65; i++ )
      if( (fc = (*field_c)[i]) != 0.0 )
	for( j = 0; j<8; j++ )
	  if( (pc = (*prox_c)[j]) != 0.0 )
	    accum += (MINIMUM(matrix[i][j],cutoff)*fc*pc) / (mc*mp);

    /* Limit */
    if( accum > 32000.0 )
      accum = 32000.0;
    accum_i = (int)(accum *100 ) + 1;
    if( accum > 0.0 )
      wf_resultset_add( res, docid, accum_i );
  }
}

static struct object *low_do_query_or( Blob **blobs,
					  int nblobs,
					  double field_c[65],
					  double prox_c[8],
					  int cutoff)
{
  struct object *res = wf_resultset_new();
  struct tofree *__f = malloc( sizeof( struct tofree ) );
  double max_c=0.0, max_p=0.0;
  ONERROR e;
  int i, j;
  Blob **tmp;
  tmp = malloc( nblobs * sizeof( Blob *) );

  __f->res = res;
  __f->blobs = blobs;
  __f->nblobs = nblobs;
  __f->tmp    = tmp;
  SET_ONERROR( e, free_stuff, __f );


  for( i = 0; i<65; i++ )
    if( field_c[i] > max_c )
      max_c = field_c[i];
  
  for( i = 0; i<8; i++ )
    if( prox_c[i] > max_p )
      max_p = prox_c[i];

  if( max_p != 0.0 && max_c != 0.0 )
  {
    /* Time to do the real work. :-) */
    for( i = 0; i<nblobs; i++ ) /* Forward to first element */
      wf_blob_next( blobs[i] );  

    /* Main loop: Find the smallest element in the blob array. */
    while( 1 )
    {
      unsigned int min = 0x7fffffff;
    
      for( i = 0; i<nblobs; i++ )
	if( !blobs[i]->eof && ((unsigned int)blobs[i]->docid) < min )
	  min = blobs[i]->docid;

      if( min == 0x7fffffff )
	break;

      for( j = 0, i = 0; i < nblobs; i++ )
	if( blobs[i]->docid == min && !blobs[i]->eof )
	  tmp[j++] = blobs[i];

      handle_hit( tmp, j, res, min, &field_c, &prox_c, max_c, max_p, cutoff );
    
      for( i = 0; i<j; i++ )
	wf_blob_next( tmp[i] );
    }
  }
  /* Free workarea and return the result. */

  UNSET_ONERROR( e );
  __f->res = 0;
  free_stuff( __f );
  return res;
}
				
static void handle_phrase_hit( Blob **blobs,
			       int nblobs,
			       struct object *res,
			       int docid,
			       double *field_c[65],
			       double mc )
{
  int i, j, k;
  unsigned char *nhits = malloc( nblobs*2 );
  unsigned char *first = nhits+nblobs;
  int matrix[65];
  double accum = 0.0;
  
  MEMSET(matrix, 0, sizeof(matrix) );


  for( i = 0; i<nblobs; i++ )
  {
    nhits[i] = wf_blob_nhits( blobs[i] );
    first[i] = 0;
  }


  for( i = 0; i<nhits[0]; i++)
  {
    double add;
    int hit = 1;
    Hit m = wf_blob_hit( blobs[0], i );
    int h = m.raw;
    if( (add = (*field_c)[ MOFF(m) ]) == 0.0 )
      continue;

    for( j = 1; j<nblobs; j++)
      for( k = first[j]; k<nhits[j]; k++ )
      {
	int h2 = wf_blob_hit_raw( blobs[j], k );
	if( h2 > h )
	{
	  first[j]=k;
	  if( h2-j == h )
	    hit++;
	  break;
	}
      }

    if( hit == nblobs )
      accum += add/mc;
  }

  free( nhits );  

  if( accum > 0.0 )
    wf_resultset_add( res, docid, (int)(accum*100) );
}

static struct object *low_do_query_phrase( Blob **blobs, int nblobs,
					   double field_c[65])
{
  struct object *res = wf_resultset_new();
  struct tofree *__f = malloc( sizeof( struct tofree ) );
  double max_c=0.0;
  ONERROR e;
  int i, j;
  __f->blobs = blobs;
  __f->nblobs = nblobs;
  __f->res = res;
  __f->tmp    = 0;
  SET_ONERROR( e, free_stuff, __f );


  for( i = 0; i<65; i++ )
    if( field_c[i] > max_c )
      max_c = field_c[i];

  if(  max_c != 0.0 )
  {
    /* Time to do the real work. :-) */
    for( i = 0; i<nblobs; i++ ) /* Forward to first element */
      wf_blob_next( blobs[i] );

    /* Main loop: Find the smallest element in the blob array. */
    while( 1 )
    {
      unsigned int min = 0x7fffffff;
    
      for( i = 0; i<nblobs; i++ )
	if( blobs[i]->eof )
	  goto end;
	else if( ((unsigned int)blobs[i]->docid) < min )
	  min = blobs[i]->docid;

      if( min == 0x7fffffff )
	goto end;

      for( j = 0, i = 0; i < nblobs; i++ )
	if( blobs[i]->docid != min )
	  goto next;

      handle_phrase_hit( blobs, nblobs, res, min, &field_c, max_c );
    
    next:
      for( i = 0; i<nblobs; i++ )
	if( blobs[i]->docid == min )
	  wf_blob_next( blobs[i] );
    }
  }
end:
  /* Free workarea and return the result. */

  UNSET_ONERROR( e );
  __f->res = 0;
  free_stuff( __f );
  return res;
}
				
static struct object *low_do_query_and( Blob **blobs, int nblobs,
					double field_c[65],
					double prox_c[8],
					int cutoff)
{
  struct object *res = wf_resultset_new();
  struct tofree *__f = malloc( sizeof( struct tofree ) );
  double max_c=0.0, max_p=0.0;
  ONERROR e;
  int i, j;
  __f->blobs = blobs;
  __f->nblobs = nblobs;
  __f->res = res;
  __f->tmp    = 0;
  SET_ONERROR( e, free_stuff, __f );


  for( i = 0; i<65; i++ )
    if( field_c[i] > max_c )
      max_c = field_c[i];

  for( i = 0; i<8; i++ )
    if( prox_c[i] > max_p )
      max_p = prox_c[i];

  if(  max_c != 0.0 )
  {
    /* Time to do the real work. :-) */
    for( i = 0; i<nblobs; i++ ) /* Forward to first element */
      wf_blob_next( blobs[i] );

    /* Main loop: Find the smallest element in the blob array. */
    while( 1 )
    {
      unsigned int min = 0x7fffffff;
    
      for( i = 0; i<nblobs; i++ )
	if( blobs[i]->eof )
	  goto end;
	else if( ((unsigned int)blobs[i]->docid) < min )
	  min = blobs[i]->docid;

      if( min == 0x7fffffff )
	goto end;

      for( j = 0, i = 0; i < nblobs; i++ )
	if( blobs[i]->docid != min )
	  goto next;

      handle_hit( blobs, nblobs, res, min, &field_c,&prox_c, max_c,max_p,
		  cutoff );
    
    next:
      for( i = 0; i<nblobs; i++ )
	if( blobs[i]->docid == min )
	  wf_blob_next( blobs[i] );
    }
  }
end:
  /* Free workarea and return the result. */

  UNSET_ONERROR( e );
  __f->res = 0;
  free_stuff( __f );
  return res;
}

/*! @module Search
 */

static void f_do_query_phrase( INT32 args )
/*! @decl ResultSet do_query_phrase( array(string) words,          @
 *!                          array(int) field_coefficients,       @
 *!                          function(int:string) blobfeeder)   
 *! @param words
 *!       
 *! Arrays of word ids. Note that the order is significant for the
 *! ranking.
 *!
 *! @param field_coefficients
 *!
 *! An array of ranking coefficients for the different fields. In the
 *! range of [0x0000-0xffff]. The array (always) has 65 elements:
 *!
 *! @array
 *!   @elem int 0
 *!     body
 *!   @elem int 1..64
 *!     Special field 0..63.
 *! @endarray
 *!
 *! @param blobfeeder
 *!     
 *! This function returns a Pike string containing the word hits for a
 *! certain word_id. Call repeatedly until it returns @expr{0@}.
 */
{
  double proximity_coefficients[8];
  double field_coefficients[65];
  int numblobs, i;
  Blob **blobs;

  struct svalue *cb;
  struct object *res;
  struct array *_words, *_field;

  /* 1: Get all arguments. */
  get_all_args( "do_query_phrase", args, "%a%a%*",
		&_words, &_field, &cb);

  if( _field->size != 65 )
    Pike_error("Illegal size of field_coefficients array (expected 65)\n" );

  numblobs = _words->size;
  if( !numblobs )
  {
    struct object *o = wf_resultset_new( );
    pop_n_elems( args );
    wf_resultset_push( o );
    return;
  }

  blobs = malloc( sizeof(Blob *) * numblobs );

  for( i = 0; i<numblobs; i++ )
    blobs[i] = wf_blob_new( cb, _words->item[i].u.string );

  for( i = 0; i<65; i++ )
    field_coefficients[i] = (double)_field->item[i].u.integer;

  res = low_do_query_phrase(blobs,numblobs, field_coefficients );
  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_do_query_and( INT32 args )
/*! @decl ResultSet do_query_and( array(string) words,          @
 *!                          array(int) field_coefficients,       @
 *!                          array(int) proximity_coefficients,   @
 *!                          function(int:string) blobfeeder)   
 *! @param words
 *!       
 *! Arrays of word ids. Note that the order is significant for the
 *! ranking.
 *!
 *! @param field_coefficients
 *!
 *! An array of ranking coefficients for the different fields. In the
 *! range of [0x0000-0xffff]. The array (always) has 65 elements:
 *!
 *! @array
 *!   @elem int 0
 *!     body
 *!   @elem int 1..64
 *!     Special field 0..63.
 *! @endarray
 *!
 *! @param proximity_coefficients
 *!
 *! An array of ranking coefficients for the different proximity
 *! categories. Always has 8 elements, in the range of
 *! [0x0000-0xffff].
 *!
 *! @array
 *!   @elem int 0
 *!     spread: 0 (Perfect hit)
 *!   @elem int 1
 *!     spread: 1-5
 *!   @elem int 2
 *!     spread: 6-10
 *!   @elem int 3
 *!     spread: 11-20
 *!   @elem int 4
 *!     spread: 21-40
 *!   @elem int 5
 *!     spread: 41-80
 *!   @elem int 6
 *!     spread: 81-160
 *!   @elem int 7
 *!     spread: 161-
 *! @endarray
 *!
 *! @param blobfeeder
 *!     
 *! This function returns a Pike string containing the word hits for a
 *! certain word_id. Call repeatedly until it returns @expr{0@}.
 */
{
  double proximity_coefficients[8];
  double field_coefficients[65];
  int numblobs, i, cutoff;
  Blob **blobs;

  struct svalue *cb;
  struct object *res;
  struct array *_words, *_field, *_prox;

  /* 1: Get all arguments. */
  get_all_args( "do_query_and", args, "%a%a%a%d%*",
		&_words, &_field, &_prox, &cutoff, &cb);

  if( _field->size != 65 )
    Pike_error("Illegal size of field_coefficients array (expected 65)\n" );
  if( _prox->size != 8 )
    Pike_error("Illegal size of proximity_coefficients array (expected 8)\n" );

  numblobs = _words->size;
  if( !numblobs )
  {
    struct object *o = wf_resultset_new( );
    pop_n_elems( args );
    wf_resultset_push( o );
    return;
  }

  blobs = malloc( sizeof(Blob *) * numblobs );

  for( i = 0; i<numblobs; i++ )
    blobs[i] = wf_blob_new( cb, _words->item[i].u.string );

  for( i = 0; i<8; i++ )
    proximity_coefficients[i] = (double)_prox->item[i].u.integer;

  for( i = 0; i<65; i++ )
    field_coefficients[i] = (double)_field->item[i].u.integer;

  res = low_do_query_and(blobs,numblobs,
			 field_coefficients,
			 proximity_coefficients,
			 cutoff );

  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_do_query_or( INT32 args )
/*! @decl ResultSet do_query_or( array(string) words,          @
 *!                              array(int) field_coefficients,       @
 *!                              array(int) proximity_coefficients,   @
 *!                              function(int:string) blobfeeder)   
 *! @param words
 *!       
 *! Arrays of word ids. Note that the order is significant for the
 *! ranking.
 *!
 *! @param field_coefficients
 *!
 *! An array of ranking coefficients for the different fields. In the
 *! range of [0x0000-0xffff]. The array (always) has 65 elements:
 *!
 *! @array
 *!   @elem int 0
 *!     body
 *!   @elem int 1..64
 *!     Special field 0..63.
 *! @endarray
 *!
 *! @param proximity_coefficients
 *!
 *! An array of ranking coefficients for the different proximity
 *! categories. Always has 8 elements, in the range of
 *! [0x0000-0xffff].
 *!
 *! @array
 *!   @elem int 0
 *!     spread: 0 (Perfect hit)
 *!   @elem int 1
 *!     spread: 1-5
 *!   @elem int 2
 *!     spread: 6-10
 *!   @elem int 3
 *!     spread: 11-20
 *!   @elem int 4
 *!     spread: 21-40
 *!   @elem int 5
 *!     spread: 41-80
 *!   @elem int 6
 *!     spread: 81-160
 *!   @elem int 7
 *!     spread: 161-
 *! @endarray
 *!
 *! @param blobfeeder
 *!     
 *! This function returns a Pike string containing the word hits for a
 *! certain word_id. Call repeatedly until it returns @expr{0@}.
 */
{
  double proximity_coefficients[8];
  double field_coefficients[65];
  int numblobs, i, cutoff;
  Blob **blobs;

  struct svalue *cb;
  struct object *res;
  struct array *_words, *_field, *_prox;

  /* 1: Get all arguments. */
  get_all_args( "do_query_or", args, "%a%a%a%d%*",
		&_words, &_field, &_prox, &cutoff, &cb);

  if( _field->size != 65 )
    Pike_error("Illegal size of field_coefficients array (expected 65)\n" );
  if( _prox->size != 8 )
    Pike_error("Illegal size of proximity_coefficients array (expected 8)\n" );

  numblobs = _words->size;
  if( !numblobs )
  {
    struct object *o = wf_resultset_new( );
    pop_n_elems( args );
    wf_resultset_push( o );
    return;
  }

  blobs = malloc( sizeof(Blob *) * numblobs );

  for( i = 0; i<numblobs; i++ )
    blobs[i] = wf_blob_new( cb, _words->item[i].u.string );

  for( i = 0; i<8; i++ )
    proximity_coefficients[i] = (double)_prox->item[i].u.integer;

  for( i = 0; i<65; i++ )
    field_coefficients[i] = (double)_field->item[i].u.integer;

  res = low_do_query_or(blobs,numblobs,
			field_coefficients,
			proximity_coefficients,
			cutoff );
  pop_n_elems( args );
  wf_resultset_push( res );
}

/*! @endmodule
 */


PIKE_MODULE_INIT
{
  init_resultset_program();
  init_blob_program();
  init_blobs_program();
  init_linkfarm_program();

  add_function( "do_query_or", f_do_query_or,
		"function(array(string),array(int),array(int),int"
		",function(string,int:string):object)",
		0 );

  add_function( "do_query_and", f_do_query_and,
		"function(array(string),array(int),array(int),int"
		",function(string,int:string):object)",
		0 );

  add_function( "do_query_phrase", f_do_query_phrase,
		"function(array(string),array(int)"
		",function(string,int:string):object)",
		0 );
}

PIKE_MODULE_EXIT
{
  exit_resultset_program();
  exit_blob_program();
  exit_blobs_program();
  exit_linkfarm_program();
}
