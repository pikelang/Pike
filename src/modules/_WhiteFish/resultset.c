#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: resultset.c,v 1.24 2004/10/26 20:41:33 grubba Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "array.h"
#include "fsort.h"
#include "module_support.h"

#include "config.h"

#include "resultset.h"
#include "whitefish.h"

#define sp Pike_sp

/*
 *! @class ResultSet
 *!
 *! A resultset is basically an array of hits from the search.
 *! 
 *! Note: inheriting this class is _not_ supported (for performance
 *! reasons) 
 */

/* The resultset class abstractions. */

struct result_set_p {  int allocated_size; ResultSet *d; };
struct program *resultset_program;

#define THIS ((struct result_set_p*)Pike_fp->current_storage)
#define T(o) ((struct result_set_p*)o->storage)

#define RETURN_THIS() pop_n_elems(args); ref_push_object(Pike_fp->current_object)

#ifdef PIKE_DEBUG
struct object *wf_not_resultset( struct object *o )
{
  fatal("%p is not a resultset!\n", o );
  return 0; /* not reached */
}
#endif

void wf_resultset_free( struct object *o )
{
  free_object( WF_RESULTSET( o ) );
}

void wf_resultset_add( struct object *o, int document, int weight )
{
  int ind;
  ResultSet *d;

  if( !(d=T(o)->d) )
  {
    wf_resultset_clear( o );
    d = T(o)->d;
  }
  ind = d->num_docs;
  
  if( T(o)->allocated_size == ind )
  {
    T(o)->allocated_size += 2048;
    d = T(o)->d = realloc( d, 4 + /* num_docs */
			   4*T(o)->allocated_size*2 ); /* hits */
    if( !d )
      Pike_error( "Out of memory" );
  }
  d->hits[ind].doc_id = document;
  d->hits[ind].ranking = weight;
  d->num_docs = ind+1;
}

void wf_resultset_avg_ranking( struct object *o, int ind, int weight )
{
#ifdef PIKE_DEBUG
  if( !T(o)->d )
    fatal("Odd, indeed. Encountered empty resultset\n");
#endif
  if( ind < 0 )
    ind = T(o)->d->num_docs-1;
#ifdef PIKE_DEBUG
  if( ind < 0 || ind > T(o)->d->num_docs-1)
    fatal( "Indexing resultset with -1\n");
#endif
  T(o)->d->hits[ind].ranking=(T(o)->d->hits[ind].ranking>>1)+(weight>>1);
}

void wf_resultset_add_ranking( struct object *o, int ind, int weight )
{
#ifdef PIKE_DEBUG
  if( !T(o)->d )
    fatal("Odd, indeed. Encountered empty resultset\n");
#endif
  if( ind < 0 )
    ind = T(o)->d->num_docs-1;
#ifdef PIKE_DEBUG
  if( ind < 0 || ind > T(o)->d->num_docs-1)
    fatal( "Indexing resultset with -1\n");
#endif
  T(o)->d->hits[ind].ranking=(T(o)->d->hits[ind].ranking)+(weight);
}


void wf_resultset_empty( struct object *o )
{
  if( T(o)->d ) free( T(o)->d );
  T(o)->allocated_size = 0;
  T(o)->d = 0;
}

void wf_resultset_push( struct object *o )
{
  if( T(o)->d )
  {
    if( T(o)->d->num_docs == 0 )
      wf_resultset_empty( o );
  }
  push_object( o );
}

void wf_resultset_clear( struct object *o )
{
  if( T(o)->d ) free( T(o)->d );
  T(o)->allocated_size = 256;
  T(o)->d = malloc( 4 + 8*256 );
  T(o)->d->num_docs = 0;
}

struct object *wf_resultset_new( )
{
  struct object *o;
  o = clone_object( resultset_program, 0 );
  wf_resultset_empty( WF_RESULTSET( o ) );
  return o;
}

static void init_rs(struct object *o)
{
  THIS->d = 0;
  THIS->allocated_size = 0;
}

static void free_rs(struct object *o)
{
  THIS->allocated_size = 0;
  if( THIS->d )  free( THIS->d );
  THIS->d = 0;
}




/* Pike functions */

static void f_resultset_create( INT32 args )
{
  if( args && sp[-1].type == PIKE_T_ARRAY )
  {
    int i;
    wf_resultset_clear(  Pike_fp->current_object );
    for(i = 0; i<sp[-1].u.array->size; i++ )
      if( sp[-1].u.array->item[i].type == PIKE_T_ARRAY )
	wf_resultset_add( Pike_fp->current_object,
			  sp[-1].u.array->item[i].u.array->item[0].u.integer,
			  sp[-1].u.array->item[i].u.array->item[1].u.integer );
      else
	wf_resultset_add( Pike_fp->current_object,
			  sp[-1].u.array->item[i].u.integer, 1 );

  }
}

static void f_resultset_cast( INT32 args )
/*
*! @decl string cast( string type )
*! Only works when type == "array". Returns the resultset data as a array.
*/
{
  static void f_resultset_slice( INT32 args );
  pop_n_elems( args );
  push_int(0);
  push_int( 0x7fffffff );
  f_resultset_slice(2);
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
  int i, j, s, b;
  struct object *o = Pike_fp->current_object;
  get_all_args("test", args, "%d%d%d", &j, &b, &s);
  wf_resultset_clear( o );
  for( i = 0; i<j; i++ )
    wf_resultset_add( o, b+i*s, rand()&0xffff );
  pop_n_elems(args);
  f_resultset_memsize(0);
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

  if( !THIS->d )
  {
    pop_n_elems( args );
    push_array( allocate_array(0) );
    return;
  }

  get_all_args("slice", args, "%d%d", &first, &nelems);

  if( THIS->d->num_docs - first < nelems )
    nelems = THIS->d->num_docs-first;
  if( nelems < 1 )
  {
    pop_n_elems( args );
    push_array( allocate_array(0) );
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
  return ai > bi ? -1 : ai == bi ? 0 : 1 ;
}

static int cmp_docid( void *a, void *b )
{
  int ai = *(int *)((int *)a);
  int bi = *(int *)((int *)b);
  return ai < bi ? -1 : ai == bi ? 0 : 1 ;
}

static void f_resultset_sort( INT32 args )
/*
*! @decl void sort()
*!   Sort this ResultSet according to ranking.
*/
{
  if(THIS->d)
    fsort( THIS->d->hits, THIS->d->num_docs, 8, (void *)cmp_hits );
  RETURN_THIS();
}

static void f_resultset_sort_docid( INT32 args )
/*
*! @decl void sort_docid()
*!   Sort this ResultSet according to document id
*/
{
  if(THIS->d)
    fsort( THIS->d->hits, THIS->d->num_docs, 8, (void *)cmp_docid );
  RETURN_THIS();
}

static void f_resultset_dup( INT32 args )
/*
*! @decl ResultSet dup()
*!   Return a new resultset with the same contents as this one
*/
{
  struct object *o = clone_object( resultset_program, 0 );
  if(THIS->d)
  {
    ResultSet *d = malloc( THIS->d->num_docs * 8 + 4 );
    MEMCPY( d, THIS->d, THIS->d->num_docs * 8 + 4 );
    T(o)->d = d;
    T(o)->allocated_size = T(o)->d->num_docs;
  }
  pop_n_elems( args );
  wf_resultset_push( o );
}


static void f_resultset__sizeof( INT32 args )
/*
*! @decl int _sizeof()
*! @decl int size()
*!   Return the size of this resultset, in entries. 
*/
{
  pop_n_elems( args );
  if( THIS->d )
    push_int( THIS->d->num_docs );
  else
    push_int( 0 );
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
  if( !THIS->d )
    f_resultset_memsize( 0 );
  else
    push_int( (THIS->allocated_size-THIS->d->num_docs)*8
	      + sizeof(struct object) + 8 );
}

static void duplicate_resultset( struct object *dest,
				 struct object *src )
{
  if( src->refs == 1 )
  {
    /* Destructively move the data. */
    T(dest)->d = T(src)->d;
    T(dest)->allocated_size = T(src)->allocated_size;
    T(src)->d = 0;
    T(src)->allocated_size = 0;
  }
  else
  {
    /* Ok, we have to copy it. */
    int size = 4+4*T(src)->allocated_size*2;
    T(dest)->allocated_size = T(src)->allocated_size;
    T(dest)->d              = malloc( size );
    MEMCPY( (char *)T(dest)->d, (char *)T(src)->d, size );
  }
}

static void f_resultset_or( INT32 args )
/*
*! @decl ResultSet `|( ResultSet a )
*! @decl ResultSet `+( ResultSet a )
*! @decl ResultSet or( ResultSet a )
*!
*! Add the given resultsets together, to generate a resultset with
*! both sets included. The ranking will be averaged if a document
*! exists in both resultsets.
*/
{
  struct object *res = wf_resultset_new();
  struct object *left = Pike_fp->current_object;
  struct object *right;
  int lp=-1, rp=-1;

  int left_used=1, right_used=1;
  int left_left=1, right_left=1;
  int right_size, left_size;

  int left_doc=0, left_rank=0, right_doc=0, right_rank=0, last=-1;
  ResultSet *set_r, *set_l = T(left)->d;

  get_all_args( "or", args, "%o", &right );

  set_r = T( WF_RESULTSET(right) )->d;


  if( !set_l )
  {
    if( set_r )
      duplicate_resultset( res, right );
    pop_n_elems(args);
    wf_resultset_push( res );
    return;
  }

  if( !set_r )
  {
    duplicate_resultset( res, left );
    pop_n_elems(args);
    wf_resultset_push( res );
    return;
  }

  left_size = set_l->num_docs;
  right_size = set_r->num_docs;
  
  while( left_left || right_left )
  {
    if( left_left && left_used ) /* New from left */
    {
      if( ++lp == left_size )
      {
	left_left = 0;
	left_rank = 0;
	if( !right_left )
	  continue;
      }
      else
      {
	left_doc = set_l->hits[lp].doc_id;
	left_rank = set_l->hits[lp].ranking;
	left_used = 0;
      }
    }

    if( right_left && right_used ) /* New from right */
    {
      if( ++rp == right_size )
      {
	right_left = 0;
	right_rank = 0;
	if( !left_left )
	  continue;
      }
      else
      {
	right_doc = set_r->hits[rp].doc_id;
	right_rank = set_r->hits[rp].ranking;
	right_used = 0;
      }
    }


    if(!right_left || (left_doc <= right_doc))
    {
      if(left_doc>last)
	wf_resultset_add( res, (last = left_doc), left_rank );
      else if( left_doc == last )
	wf_resultset_add_ranking( res, -1, left_rank );
      left_used=1;
    }

    if(!left_left || (right_doc <= left_doc ) )
    {
      if(right_doc>last)
	wf_resultset_add( res, (last = right_doc), right_rank );
      else if( right_doc == last )
	wf_resultset_add_ranking( res, -1, right_rank );
      right_used=1;
    }
  }
  if( !left_used )
    if(left_doc!=last)
      wf_resultset_add( res, (last = left_doc), left_rank );
  if( !right_used )
    if(right_doc!=last)
      wf_resultset_add( res, (last = right_doc), right_rank );
  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_resultset_intersect( INT32 args )
/*
 *! @decl ResultSet intersect( ResultSet a )
 *! @decl ResultSet `&( ResultSet a )
 *!
 *!
 *! Return a new resultset with all entries that are present in _both_
 *! sets. Only the document_id is checked, the resulting ranking is
 *! the lowest ranking of the two sets..
 */
{
  struct object *res = wf_resultset_new();
  struct object *left = Pike_fp->current_object;
  struct object *right;
  int lp=-1, rp=-1;

  int left_used=1, right_used=1;
  int left_left=1, right_left=1;
  int right_size, left_size;

  int left_doc=0, left_rank=0, right_doc=0, right_rank=0, last=-1;
  ResultSet *set_r, *set_l = T(left)->d;

  get_all_args( "intersect", args, "%o", &right );

  right = WF_RESULTSET( right );
  set_r = T(right)->d;

  if( !set_l || !set_r ) /* ({}) & X == ({}) && X & ({}) == ({}) */
  {
    pop_n_elems(args);
    wf_resultset_push(res);
    return;
  }

  left_size = set_l->num_docs;
  right_size = set_r->num_docs;

  while( left_left && right_left )
  {
    if( left_left && left_used ) /* New from left */
    {
      if( ++lp == left_size )
      {
	left_left = 0;
	continue;
      }
      else
      {
	left_doc = set_l->hits[lp].doc_id;
	left_rank = set_l->hits[lp].ranking;
	left_used = 0;
      }
    }

    if( right_left && right_used ) /* New from right */
    {
      if( ++rp == right_size )
      {
	right_left = 0;
	continue;
      }
      else
      {
	right_doc = set_r->hits[rp].doc_id;
	right_rank = set_r->hits[rp].ranking;
	right_used = 0;
      }
    }

    if(!right_left || (left_doc <= right_doc))
    {
      if( left_doc == right_doc )
      {	
	if(left_doc>last)
	  wf_resultset_add( res, (last = left_doc),
			    MINIMUM(left_rank,right_rank) );
      }
      left_used=1;
    }

    if(!left_left || (right_doc <= left_doc))
    {
      if( right_doc == left_doc )
      {	
	if(right_doc>last)
	  wf_resultset_add( res, (last = right_doc),
			    MINIMUM(left_rank,right_rank) );
      }
      right_used=1;
    }
  }
  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_resultset_add_ranking( INT32 args )
/*
 *! @decl ResultSet add_ranking( ResultSet a )
 *!
 *! Return a new resultset. All entries are the same as in this set, 
 *! but if an entry exists in @[a], the ranking from @[a] is added to
 *! the ranking of the entry
 */
{
  struct object *res = wf_resultset_new();
  struct object *left = Pike_fp->current_object;
  struct object *right;
  int lp=-1, rp=-1;

  int left_used=1, right_used=1;
  int left_left=1, right_left=1;
  int right_size, left_size;

  int left_doc=0, left_rank=0, right_rank=0, right_doc=0, last=-1;
  ResultSet *set_r, *set_l = T(left)->d;

  get_all_args( "sub", args, "%o", &right );

  right = WF_RESULTSET( right );
  set_r = T(right)->d;

  if( !set_l ) /* add_ranking( ({}), X ) == ({}) */
  {
    pop_n_elems(args);
    wf_resultset_push( res );
    return;
  }

  if( !set_r ) /* add_ranking( X,({}) ) == X */
  {
    duplicate_resultset( res, left );
    pop_n_elems(args);
    wf_resultset_push(res);
    return;
  }
  
  left_size = set_l->num_docs;
  right_size = set_r->num_docs;
  
  while( left_left )
  {
    if( left_left && left_used ) /* New from left */
    {
      if( ++lp == left_size )
      {
	left_left = 0;
	continue;
      }
      else
      {
	left_doc = set_l->hits[lp].doc_id;
	left_rank = set_l->hits[lp].ranking;
	left_used = 0;
      }
    }

    if( right_left && right_used ) /* New from right */
    {
      if( ++rp == right_size )
      {
	right_left = 0;
	if( !left_left )
	  continue;
      }
      else
      {
	right_doc = set_r->hits[rp].doc_id;
	right_rank = set_r->hits[rp].ranking;
	right_used = 0;
      }
    }


    if(!right_left || (left_doc <= right_doc))
    {
      if( left_doc != right_doc )
      {	
	if( left_doc > last )
	  wf_resultset_add( res, (last = left_doc), left_rank );
      } else
	wf_resultset_add( res, (last = left_doc), left_rank+right_rank );

      left_used=1;
    }

    if( right_doc <= left_doc )
      right_used=1;
  }
  if( !left_used )
    if(left_doc!=last)
      wf_resultset_add( res, (last = left_doc), left_rank );
  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_resultset_sub( INT32 args )
/*
 *! @decl ResultSet sub( ResultSet a )
 *! @decl ResultSet `-( ResultSet a )
 *!
 *! Return a new resultset with all entries in a removed from the
 *! current ResultSet.
 *!
 *! Only the document_id is checked, the ranking is irrelevalt.
 */
{
  struct object *res = wf_resultset_new();
  struct object *left = Pike_fp->current_object;
  struct object *right;
  int lp=-1, rp=-1;

  int left_used=1, right_used=1;
  int left_left=1, right_left=1;
  int right_size, left_size;

  int left_doc=0, left_rank=0, right_doc=0, last=-1;
  ResultSet *set_r, *set_l = T(left)->d;

  get_all_args( "sub", args, "%o", &right );

  right = WF_RESULTSET( right );
  set_r = T(right)->d;


  if( !set_l  ) /* ({}) - X == ({}) */
  {
    pop_n_elems(args);
    wf_resultset_push(res);
    return;
  }

  if( !set_r ) /*  X-({}) == X */
  {
    duplicate_resultset( res, left );
    pop_n_elems(args);
    wf_resultset_push(res);
    return;
  }
  
  left_size = set_l->num_docs;
  right_size = set_r->num_docs;
  
  while( left_left )
  {
    if( left_left && left_used ) /* New from left */
    {
      if( ++lp == left_size )
      {
	left_left = 0;
	continue;
      }
      else
      {
	left_doc = set_l->hits[lp].doc_id;
	left_rank = set_l->hits[lp].ranking;
	left_used = 0;
      }
    }

    if( right_left && right_used ) /* New from right */
    {
      if( ++rp == right_size )
      {
	right_left = 0;
	if( !left_left )
	  continue;
      }
      else
      {
	right_doc = set_r->hits[rp].doc_id;
	right_used = 0;
      }
    }


    if(!right_left || (left_doc <= right_doc))
    {
      if( left_doc != right_doc )
      {	
	if(left_doc>last)
	  wf_resultset_add( res, (last = left_doc), left_rank );
      }
      left_used=1;
    }

    if( right_doc <= left_doc )
      right_used=1;
  }
  if( !left_used )
    if(left_doc!=last)
      wf_resultset_add( res, (last = left_doc), left_rank );
  pop_n_elems( args );
  wf_resultset_push( res );
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
    add_function("create",f_resultset_create,
		 "function(void|array(int|array(int)):void)",0);

    add_function("sort",f_resultset_sort,"function(void:object)",0);
    add_function("sort_docid",f_resultset_sort_docid,
		 "function(void:object)",0);

    add_function("dup",f_resultset_dup,"function(void:object)",0);

    add_function("slice",f_resultset_slice,
		 "function(int,int:array(array(int)))",0);

    add_function( "or", f_resultset_or, "function(object:object)", 0 );
    add_function( "`|", f_resultset_or, "function(object:object)", 0 );
    add_function( "`+", f_resultset_or, "function(object:object)", 0 );

    add_function( "sub", f_resultset_sub, "function(object:object)", 0 );
    add_function( "`-", f_resultset_sub, "function(object:object)", 0 );

    add_function( "add_ranking", f_resultset_add_ranking,
		  "function(object:object)", 0 );

    add_function( "intersect", f_resultset_intersect,
		  "function(object:object)", 0 );
    add_function( "`&", f_resultset_intersect, "function(object:object)", 0 );

    add_function("_sizeof",f_resultset__sizeof,"function(void:int)", 0 );
    add_function("size",f_resultset__sizeof,"function(void:int)", 0 );

    /* debug related functions */
    add_function("memsize",f_resultset_memsize,"function(void:int)", 0 );
    add_function("overhead",f_resultset_overhead,"function(void:int)", 0 );
    add_function("test", f_resultset_test, "function(int,int,int:int)", 0 );

    set_init_callback( init_rs );
    set_exit_callback( free_rs );
  }
  resultset_program = end_program( );
  add_program_constant( "ResultSet", resultset_program,0 );
}

/*
 *! @endclass ResultSet
 */
