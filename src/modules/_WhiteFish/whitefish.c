#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: whitefish.c,v 1.7 2001/05/23 10:58:00 per Exp $");
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
  free( t );
}

static struct object *low_do_query_merge( Blob **blobs,
					  int nblobs,
					  int field_c[68],
					  int prox_c[8])
{
  struct object *res = wf_resultset_new();
  struct tofree *__f = malloc( sizeof( struct tofree ) );
  ONERROR e;

  __f->res = res;
  __f->blobs = blobs;
  __f->nblobs = nblobs;
  SET_ONERROR( e, free_stuff, __f );


  /* Time to do the real work. :-) */


  UNSET_ONERROR( e );
  __f->res = 0;
  free_stuff( __f );
  return res;
}
				

static void f_do_query_merge( INT32 args )
/*! @decl ResultSet do_query_merge( array(int) musthave,          @
 *!                          array(int) field_coefficients,       @
 *!                          array(int) proximity_coefficients,   @
 *!                          function(int:string) blobfeeder,     @
 *!			     int field)
 *!       @[musthave]
 *!       
 *!          Arrays of word ids. Note that the order is significant
 *!          for the ranking.
 *!
 *!       @[field_coefficients]
 *!
 *!       An array of ranking coefficients for the different fields. 
 *!       In the range of [0x0000-0xffff]. The array (always) has 68
 *!       elements:
 *!
 *!	  Index        Coefficient for field
 *!	  -----        ---------------------
 *!	  0..63        Special field 0..63
 *!	  64           Large body text
 *!	  65           Medium body text
 *!	  66           Small body text
 *!	  67           Anchor
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
  int proximity_coefficients[8];
  int field_coefficients[68];
  int numblobs, i;
  Blob **blobs;

  struct svalue *cb;
  struct array *_words, *_field, *_prox;

  /* 1: Get all arguments. */
  get_all_args( "do_query_merge", args, "%a%a%a%O",
		&_words, &_field, &_prox, &cb);

  if( _field->size != 68 )
    Pike_error("Illegal size of field_coefficients array (expected 68)\n" );
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
    proximity_coefficients[i] = _prox->item[i].u.integer;

  for( i = 0; i<48; i++ )
    field_coefficients[i] = _field->item[i].u.integer;

  push_object(low_do_query_merge(blobs,numblobs,
				 field_coefficients,
			 proximity_coefficients ));
}

void pike_module_init(void)
{
  init_resultset_program();

  add_function( "do_query_merge", f_do_query_merge,
		"function(array(int),array(int),array(int)"
		",function(int:string):object)",
		0 );
}

void pike_module_exit(void)
{
  exit_resultset_program();
}
