#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: resultset.c,v 1.3 2001/05/22 08:17:20 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "array.h"
#include "fsort.h"

#include "config.h"

#include "resultset.h"
#include "whitefish.h"

/* must be included last */
#include "module_magic.h"

/*
 *! @class ResultSet
 *!
 *! A resultset is basically an array of hits from the search.
 */

/* The resultset class abstractions. */

struct program *resultset_program;
struct result_set_p {  int allocated_size; ResultSet *d; };

/* Note: inheriting this class is _not_ supported (for performance
 * reasons) 
*/
#define THIS ((struct result_set_p*)Pike_fp->current_object->storage)
#define T(o) ((struct result_set_p*)o->storage)

#ifdef DEBUG
struct object *wf_not_resultset( struct object *o )
{
  Pike_fatal("%p is not a resultset!\n", o );
  return 0; /* not reached */
}
#endif

void wf_resultset_free( struct object *o )
{
  free_object( WF_RESULTSET( o ) );
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
  T(o)->d->hits[ind].doc_id = document;
  T(o)->d->hits[ind].ranking = weight;
  T(o)->d->num_docs = ind+1;
}

void wf_resultset_clear( struct object *o )
{
  if( T(o)->allocated_size ) free( T(o)->d );
  T(o)->allocated_size = 256;
  T(o)->d = malloc( 4 + 8*256 );
  T(o)->d->num_docs = 0;
}

struct object *wf_resultset_new( )
{
  struct object *o;
  o = clone_object( resultset_program, 0 );
  wf_resultset_clear( WF_RESULTSET( o ) );
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




/* Pike functions */

static void f_resultset_cast( INT32 args )
/*
*! @decl string cast( string type )
*! Only works when type == "string". Returns the resultset data as a string.
*/
{
  pop_n_elems( args );
  if( THIS->d )
    push_string(make_shared_binary_string((char*)THIS->d,
					  THIS->d->num_docs*8+4));
  else
    push_int( 0 );
}

static void f_resultset_test( INT32 args )
/*
 *! @decl ResultSet test( int nelems, int start, int incr )
 *! 
 *! Fills the resulttest with nelems entries, the document IDs are
 *! strictly raising, starting with @[start], ending with
 *! @[start]+@[nelems]*@[incr].
 *!
 *! Used for debug and testing.
 */
{
  int i, j;
  struct object *o = Pike_fp->current_object;
  j = Pike_sp[-1].u.integer;
  wf_resultset_clear( o );
  for( i = 0; i<j; i++ )
    wf_resultset_add( o, i, rand()&0xffff );
}

static void f_resultset_slice( INT32 args )
/*
 *! @decl array(array(int)) slice( int first, int nelems )
 *!
 *! Return nelems entries from the result-set, starting with first.
 *! If 'first' is outside the resultset, or nelems is 0, 0 is returned.
 */
{
  int first, nelems, i;
  struct array *res;

  nelems = Pike_sp[-1].u.integer;
  first = Pike_sp[-2].u.integer;

  if( THIS->d->num_docs - first < nelems )
    nelems = THIS->d->num_docs-first;
  if( nelems < 1 )
  {
    pop_n_elems( args );
    push_int( 0 );
    return;
  }
  res = allocate_array( nelems );
  for( i = 0; i<nelems; i++ )
  {
    struct array *a2 = allocate_array(2);
    res->item[i].type = PIKE_T_ARRAY;
    res->item[i].u.array = a2;
    a2->item[0].type = PIKE_T_INT;
    a2->item[1].type = PIKE_T_INT;
    a2->item[0].u.integer = THIS->d->hits[i+first].doc_id;
    a2->item[1].u.integer = THIS->d->hits[i+first].ranking;
  }
  pop_n_elems( args );
  push_array( res );
}


static int cmp_hits( void *a, void *b )
{
  int ai = *(int *)((int *)a+1);
  int bi = *(int *)((int *)b+1);
  return ai < bi ? -1 : ai == bi ? 0 : 1 ;
}

static void f_resultset_sort( INT32 args )
/*
*! @decl void sort()
*!   Sort this ResultSet according to ranking.
*/
{
  fsort( THIS->d->hits, THIS->d->num_docs, 8, (void *)cmp_hits );
  RETURN_THIS();
}

static void f_resultset_dup( INT32 args )
/*
*! @decl ResultSet dup()
*!   Return a new resultset with the same contents as this one
*/
{
  struct object *o = clone_object( resultset_program, 0 );
  ResultSet *d = malloc( THIS->d->num_docs * 8 + 4 );
  MEMCPY( d, THIS->d, THIS->d->num_docs * 8 + 4 );
  T(o)->d = d;
  T(o)->allocated_size = T(o)->d->num_docs;
  pop_n_elems( args );
  push_object( o );
}


static void f_resultset__sizeof( INT32 args )
/*
*! @decl int _sizeof()
*! @decl int size()
*!   Return the size of this resultset, in entries. 
*/
{
  pop_n_elems( args );
  push_int( THIS->d->num_docs );
}

static void f_resultset_memsize( INT32 args )
/*
*! @decl int memsize()
*!   Return the size of this resultset, in bytes. 
*/
{
  pop_n_elems( args );
  push_int( THIS->allocated_size*8 + sizeof(struct object) + 8 );
}

static void f_resultset_overhead( INT32 args )
/*
*! @decl int overhead()
*!   Return the size of the memory overhead, in bytes.
*!
*!   You can minimize the overhead by calling dup(), which will create
*!   a new resultset with the exact size needed.
*/
{
  pop_n_elems( args );
  push_int( (THIS->allocated_size-THIS->d->num_docs)*8
	    + sizeof(struct object) + 4 );
}

void exit_resultset_program(void)
{
  free_program( resultset_program );
}


void init_resultset_program(void)
{
  start_new_program();
  {  
    ADD_STORAGE( ResultSet );
    add_function("cast", f_resultset_cast, "function(string:mixed)", 0 );
    add_function("test", f_resultset_test, "function(int:int)", 0 );
    add_function("sort",f_resultset_sort,"function(void:object)",0);
    add_function("dup",f_resultset_dup,"function(void:object)",0);
    add_function("slice",f_resultset_slice,
		 "function(int,int:array(array(int)))",0);


    add_function("_sizeof",f_resultset__sizeof,"function(void:int)", 0 );
    add_function("size",f_resultset__sizeof,"function(void:int)", 0 );

    /* debug related stuff */
    add_function("memsize",f_resultset_memsize,"function(void:int)", 0 );
    add_function("overhead",f_resultset_overhead,"function(void:int)", 0 );
    set_init_callback( init_rs );
    set_exit_callback( free_rs );
  }
  resultset_program = end_program( );
  add_program_constant( "ResultSet", resultset_program,0 );
}

/*
 * @endclass ResultSet
 */
