#include <math.h>

#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: whitefish.c,v 1.16 2001/05/25 15:55:20 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "array.h"
#include "module_support.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"

/* must be included last */
#include "module_magic.h"

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
  free( t->tmp );
  free( t );
}

#define OFFSET(X) \
 (X.type == HIT_BODY?X.u.body.pos:X.type==HIT_FIELD?(X.u.field.pos):(X.u.anchor.pos))

#define DOFF(X)  MINIMUM((int)X,7)
#define MOFF(X)  (X.type==HIT_BODY?0:X.type==HIT_FIELD?X.u.field.type+2:1)

static void handle_hit( Blob **blobs,
			int nblobs,
			struct object *res,
			int docid,
			double *field_c[66],
			double *prox_c[8],
			double mc, double mp)
{
  int i, j, k, end = 0;
  Hit *hits = malloc( nblobs * sizeof(Hit) );
  unsigned char *nhits = malloc( nblobs );
  unsigned char *pos = malloc( nblobs );

  int matrix[66][8];

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
      matrix[MOFF(hits[i])][0]++;
/*       printf("Absolute hit %d -> %d\n", hits[i].raw, MOFF(hits[i]) ); */
      /* forward the other positions */
      for( k = 0; k<nblobs; k++ )
	if( k != i &&  pos[ k ] < nhits[ k ] )
	{
	  while( (hits[k].raw < hits[i].raw) && (pos[ k ] < nhits[ k ]))
	    hits[k] = wf_blob_hit( blobs[k], pos[k]++ );
	  if( (pos[ k ] < nhits[ k ]) && hits[k].type == hits[i].type )
	  {
/* 	    printf("Pair hit %d-%d: %x -> [%d][%d]\n", */
/* 		   OFFSET(hits[i]),  OFFSET(hits[k]), */
/* 		   hits[i].raw, */
/* 		   MOFF(hits[i]), */

	    matrix[MOFF(hits[i])][DOFF(OFFSET(hits[k])-OFFSET(hits[i]))]+=2;
	  }
	}
    }
  }
  

  /* Now we have our nice matrix. Time to do some multiplication */

/*   printf("matrix:\n"); */
/*   for( i = 0; i<3; i++ ) */
/*   { */
/*     for( j = 0; j<8; j++ ) */
/*       printf( "%4d ", matrix[i][j] ); */
/*     printf("\n"); */
/*   } */
  
  {
    double accum = 0.0, fc, pc;
    int accum_i;
    for( i = 0; i<66; i++ )
      if( (fc = (*field_c)[i]) != 0.0 )
	for( j = 0; j<8; j++ )
	  if( (pc = (*prox_c)[j]) != 0.0 )
	    accum += (matrix[i][j]*fc*pc) / (mc*mp);

    /* Limit */
    if( accum > 32000.0 )
      accum = 32000.0;
    accum_i = (int)(accum *100 ); 
    if( accum_i > 0 )
      wf_resultset_add( res, docid, accum_i );
  }
}

static struct object *low_do_query_merge( Blob **blobs,
					  int nblobs,
					  double field_c[66],
					  double prox_c[8] )
{
  struct object *res = wf_resultset_new();
  struct tofree *__f = malloc( sizeof( struct tofree ) );
  double max_c=0.0, max_p=0.0;
  ONERROR e;
  int i, j, end=0;
  Blob **tmp;
  
  tmp = malloc( nblobs * sizeof( Blob *) );

  __f->res = res;
  __f->blobs = blobs;
  __f->nblobs = nblobs;
  __f->tmp    = tmp;
  SET_ONERROR( e, free_stuff, __f );


  for( i = 0; i<66; i++ )
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
    while( !end )
    {
      unsigned int min = 0x7ffffff;
    
      for( i = 0; i<nblobs; i++ )
	if( !blobs[i]->eof && ((unsigned int)blobs[i]->docid) < min )
	  min = blobs[i]->docid;

      if( min == 0x7ffffff )
	break;

/*       printf( "hit in %d: ", min ); */

      for( j = 0, i = 0; i < nblobs; i++ )
	if( blobs[i]->docid == min && !blobs[i]->eof )
	{
	  tmp[j++] = blobs[i];
/* 	  printf( "%8x ", blobs[i]->word ); */
	}
/*       printf( "\n"); */
      handle_hit( tmp, j, res, min, &field_c, &prox_c, max_c, max_p );
    
      /* Step the 'min' blobs */
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
				

static void f_do_query_merge( INT32 args )
/*! @decl ResultSet do_query_merge( array(int) words,          @
 *!                          array(int) field_coefficients,       @
 *!                          array(int) proximity_coefficients,   @
 *!                          function(int:string) blobfeeder)   
 *!       @[words]
 *!       
 *!          Arrays of word ids. Note that the order is significant
 *!          for the ranking.
 *!
 *!       @[field_coefficients]
 *!
 *!       An array of ranking coefficients for the different fields. 
 *!       In the range of [0x0000-0xffff]. The array (always) has 66
 *!       elements:
 *!
 *!	  Index        Coefficient for field
 *!	  -----        ---------------------
 *!	  0            body
 *!	  1            anchor
 *!	  2..65        Special field 0..63
 *!
 *!       @[proximity_coefficients]
 *!
 *!         An array of ranking coefficients for the different
 *!         proximity categories. Always has 8 elements, in the range
 *!         of [0x0000-0xffff].
 *!	 
 *!	 Index       Meaning
 *!	 -----       -------
 *!      0           spread: 0 (Perfect hit)
 *!	 1           spread: 1-5
 *!	 2           spread: 6-10                                 
 *!	 3           spread: 11-20
 *!	 4           spread: 21-40
 *!	 5           spread: 41-80
 *!	 6           spread: 81-160
 *!	 7           spread: 161-
 *!
 *!	 The 'spread' value should be defined somehow.
 *!	 
 *!     @[blobfeeder]
 *!     
 *!      This function returns a Pike string containing the word hits
 *!	 for a certain word_id. Call repeatedly until it returns 0.
 */
{
  double proximity_coefficients[8];
  double field_coefficients[66];
  int numblobs, i;
  Blob **blobs;

  struct svalue *cb;
  struct object *res;
  struct array *_words, *_field, *_prox;

  /* 1: Get all arguments. */
  get_all_args( "do_query_merge", args, "%a%a%a%*",
		&_words, &_field, &_prox, &cb);

  if( _field->size != 66 )
    Pike_error("Illegal size of field_coefficients array (expected 66)\n" );
  if( _prox->size != 8 )
    Pike_error("Illegal size of proximity_coefficients array (expected 8)\n" );

  numblobs = _words->size;
  if( !numblobs )
  {
    struct object *o = wf_resultset_new( );
    pop_n_elems( args );
    push_object( o );
    return;
  }

  blobs = malloc( sizeof(Blob *) * numblobs );

  for( i = 0; i<numblobs; i++ )
    blobs[i] = wf_blob_new( cb, _words->item[i].u.integer );

  for( i = 0; i<8; i++ )
    proximity_coefficients[i] = (double)_prox->item[i].u.integer;

  for( i = 0; i<66; i++ )
    field_coefficients[i] = (double)_field->item[i].u.integer;

  res = low_do_query_merge(blobs,numblobs,
			   field_coefficients,
			   proximity_coefficients );
  pop_n_elems( args );
  push_object( res );
}

void pike_module_init(void)
{
  init_resultset_program();
  init_blob_program();

  add_function( "do_query_merge", f_do_query_merge,
		"function(array(int),array(int),array(int)"
		",function(int:string):object)",
		0 );
}

void pike_module_exit(void)
{
  exit_resultset_program();
  exit_blob_program();
}
