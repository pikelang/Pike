#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: resultset.c,v 1.1 2001/05/22 06:41:57 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"

#include "parser.h"

/* must be included last */
#include "module_magic.h"


// The resultset class abstractions.
static struct program resultset_program;
struct result_set_p {  int allocated_size; ResultSet *d; }

/* Note: inheriting this class is _not_ supported (for performance
 * reasons) 
*/
#define THIS ((struct result_set_p*)Pike_fp->current_object->storage)
#define T(o) ((struct result_set_p*)o->storage)


void wf_resultset_free( struct object *o )
{
  free_object( o );
}

void wf_resultset_add( struct object *o, int document, int weight )
{
  int ind = T(o)->d->num_docs;
  if( T(o)->allocated_size == ind )
  {
    T(o)->allocated_size *= 2;
    T(o)->d = realloc( T(o)->d,
		       4 + /* num_docs */
		       4*T(o)->allocated_size*2 ); /* hits */
    if( !T(o)->d )
      Pike_error( "Out of memory" );
  }
  T(o)->d.hits[ind].doc_id = document;
  T(o)->d.hits[ind].doc_id = weight;
  T(o)->d.num_docs = ind+1;
}

struct object *wf_resultset_new( )
{
  struct object *o;
  o = clone_object( resultset_program, 0 );
  T(o)->alloocated_size = 256;
  T(o)->d = malloc( 4 + 4*512 );
  T(o)->d->num_docs = 0;
  return o;
}

static void init_rs( )
{
  THIS->d = 0;
  THIS->allocated_size = 0;
}

static void free_rs()
{
  THIS->allocated_size = 0;
  if( THIS->d )  free( THIS->d );
  THIS->d = 0;
}

static void f_resultset_cast( INT32 args )
{
  pop_n_elems( args );
  if( THIS->d )
    push_string(make_shared_binary_string(THIS->d,THIS->d->num_docs*8+4));
  else
    push_int( 0 );
}

static void f_resultset_test( INT32 args )
{
  int i, j;
  struct object *o = Pike_fp->current_object;
  j = Pike_sp[-1].u.integer;
  for( i = 0; i<j; i++ )
    wf_resultset_add( o, i, i );
}

void init_resultset_program(void)
{
  start_new_program();
  {  
    ADD_STORAGE( ResultSet );
    add_function( "cast", f_resultset_cast, "function(string:mixed)", 0 );
    add_function( "test", f_resultset_test, "function(int:int)", 0 );
    set_init_callback( init_rs );
    set_exit_callback( free_rs );
  }
  resultset_program = end_program( );
  add_program_constant( "ResultSet", resultset_program );
}
