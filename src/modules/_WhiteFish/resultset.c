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
#include "operators.h"
#include "array.h"
#include "bignum.h"
#include "module_support.h"
#include "fsort.h"
#include "pike_types.h"

#include "config.h"

#include "resultset.h"
#include "whitefish.h"

#define sp Pike_sp

/*! @module Search
 */

/*! @class ResultSet
 *!
 *! A resultset is basically an array of hits from the search.
 *!
 *! Note: inheriting this class is _not_ supported (for performance
 *! reasons)
 */

/* The resultset class abstractions. */

struct result_set_p {  int allocated_size; ResultSet *d; };
static struct program *dateset_program;

#define THIS ((struct result_set_p*)Pike_fp->current_storage)
#define T(o) ((struct result_set_p*)o->storage)

#define RETURN_THIS() pop_n_elems(args); ref_push_object(Pike_fp->current_object)

#ifdef PIKE_DEBUG
struct program *resultset_program;
struct object *wf_not_resultset( struct object *o )
{
  Pike_fatal("%p is not a resultset!\n", o );
  UNREACHABLE(return 0);
}
#else
static struct program *resultset_program;
#endif

void wf_resultset_free( struct object *o )
{
  free_object( WF_RESULTSET( o ) );
}

void wf_resultset_add( struct object *o,
		       unsigned int document,
		       unsigned int weight )
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
    d = T(o)->d = xrealloc( d, 4 + /* num_docs */
                            4*T(o)->allocated_size*2 ); /* hits */
  }
  d->hits[ind].doc_id = document;
  d->hits[ind].ranking = weight;
  d->num_docs = ind+1;
}

void wf_resultset_avg_ranking( struct object *o, int ind, int weight )
{
#ifdef PIKE_DEBUG
  if( !T(o)->d )
    Pike_fatal("Odd, indeed. Encountered empty resultset\n");
#endif
  if( ind < 0 )
    ind = T(o)->d->num_docs-1;
#ifdef PIKE_DEBUG
  if( ind < 0 || ind > T(o)->d->num_docs-1)
    Pike_fatal( "Indexing resultset with -1\n");
#endif
  T(o)->d->hits[ind].ranking=(T(o)->d->hits[ind].ranking>>1)+(weight>>1);
}

void wf_resultset_add_ranking( struct object *o, int ind, int weight )
{
#ifdef PIKE_DEBUG
  if( !T(o)->d )
    Pike_fatal("Odd, indeed. Encountered empty resultset\n");
#endif
  if( ind < 0 )
    ind = T(o)->d->num_docs-1;
#ifdef PIKE_DEBUG
  if( ind < 0 || ind > T(o)->d->num_docs-1)
    Pike_fatal( "Indexing resultset with -1\n");
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

struct object *wf_resultset_new(void)
{
  struct object *o;
  o = clone_object( resultset_program, 0 );
  wf_resultset_empty( WF_RESULTSET( o ) );
  return o;
}

static void free_rs(struct object *UNUSED(o))
{
  if( THIS->d )  free( THIS->d );
}


/* Pike functions */

static void f_resultset_create( INT32 args )
{
  wf_resultset_clear( Pike_fp->current_object );
  if( args && TYPEOF(sp[-1]) == PIKE_T_ARRAY )
  {
    int i;
    struct array *d = sp[-1].u.array;
    for(i = 0; i< d->size; i++ )
      if( TYPEOF(d->item[i]) == PIKE_T_ARRAY )
      {
	INT64 di, ri;
	struct array *a = d->item[i].u.array;
	if( a->size < 2 )
	  continue;
	if( TYPEOF(a->item[0]) == PIKE_T_OBJECT )
	{
	  push_object( a->item[0].u.object );
          get_all_args( NULL, 1, "%l", &di );
	  Pike_sp--;
	}
	else
	  di = a->item[0].u.integer;
	if( TYPEOF(a->item[1]) == PIKE_T_OBJECT )
	{
	  push_object( a->item[1].u.object );
          get_all_args( NULL, 1, "%l", &ri );
	  Pike_sp--;
	}
	else
	  ri = a->item[1].u.integer;
	wf_resultset_add( Pike_fp->current_object, di, ri );
      }
      else
      {
	INT64 ri;
	if( TYPEOF(d->item[i]) == PIKE_T_OBJECT )
	{
	  push_object( d->item[i].u.object );
          get_all_args( NULL, 1, "%l", &ri );
	  Pike_sp--;
	}
	else
	  ri = d->item[i].u.integer;
	wf_resultset_add( Pike_fp->current_object, ri, 1 );
      }
  }
  pop_n_elems(args);
}

static void f_resultset_slice( INT32 args );

static void f_resultset_cast( INT32 args )
/*! @decl array cast( string type )
 *! Only works when type == @expr{"array"@}. Returns the resultset
 *! data as a array.
 */
{
  struct pike_string *type = Pike_sp[-args].u.string;
  pop_stack(); /* type have at least one more reference. */

  if( type==literal_array_string )
  {
    push_int(0);
    push_int( 0x7fffffff );
    f_resultset_slice(2);
  }
  else
    push_undefined();
}

static void f_resultset_memsize( INT32 args )
/*! @decl int memsize()
 *!   Return the size of this resultset, in bytes.
 */
{
  pop_n_elems( args );
  push_int( THIS->allocated_size*8 + sizeof(struct object) + 8 );
}

static void f_resultset_test( INT32 args )
/*! @decl ResultSet test( int nelems, int start, int incr )
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
  get_all_args(NULL, args, "%d%d%d", &j, &b, &s);
  wf_resultset_clear( o );
  for( i = 0; i<j; i++ )
    wf_resultset_add( o, b+i*s, rand()&0xffff );
  pop_n_elems(args);
  f_resultset_memsize(0);
}

static void f_resultset_slice( INT32 args )
/*! @decl array(array(int)) slice( int first, int nelems )
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

  get_all_args(NULL, args, "%d%d", &first, &nelems);

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
    SET_SVAL(res->item[i], PIKE_T_ARRAY, 0, array, a2);
    SET_SVAL_TYPE(a2->item[0], PIKE_T_INT);
    SET_SVAL_TYPE(a2->item[1], PIKE_T_INT);
    if( THIS->d->hits[i+first].doc_id <= ((((unsigned int)1<<31)-1)) )
	a2->item[0].u.integer = THIS->d->hits[i+first].doc_id;
    else
    {
      push_int64( THIS->d->hits[i+first].doc_id );
      a2->item[0] = *(Pike_sp-1);
      Pike_sp--;
    }
    if( THIS->d->hits[i+first].ranking <= (((unsigned int)1<<31)-1) )
      a2->item[1].u.integer = THIS->d->hits[i+first].ranking;
    else
    {
      push_int64( THIS->d->hits[i+first].ranking );
      a2->item[1] = *(Pike_sp-1);
      Pike_sp--;
    }
    a2->item[1].u.integer = THIS->d->hits[i+first].ranking;
  }
  push_array( res );
}


static int cmp_hits( void *a, void *b )
{
  int ai = *(int *)((int *)a+1);
  int bi = *(int *)((int *)b+1);
  return ai > bi ? -1 : ai == bi ? 0 : 1 ;
}
static int cmp_hits_rev( void *a, void *b )
{
  int ai = *(int *)((int *)a+1);
  int bi = *(int *)((int *)b+1);
  return ai < bi ? -1 : ai == bi ? 0 : 1 ;
}

static int cmp_docid( void *a, void *b )
{
  int ai = *(int *)((int *)a);
  int bi = *(int *)((int *)b);
  return ai < bi ? -1 : ai == bi ? 0 : 1 ;
}

static void f_resultset_sort( INT32 args )
/*! @decl void sort()
 *!   Sort this ResultSet according to ranking.
 */
{
  if(THIS->d)
    fsort( THIS->d->hits, THIS->d->num_docs, 8, (void *)cmp_hits );
  RETURN_THIS();
}

static void f_resultset_sort_rev( INT32 args )
/*! @decl void sort()
 *!   Sort this ResultSet according to ranking.
 */
{
  if(THIS->d)
    fsort( THIS->d->hits, THIS->d->num_docs, 8, (void *)cmp_hits_rev );
  RETURN_THIS();
}

static void f_resultset_sort_docid( INT32 args )
/*! @decl void sort_docid()
 *!   Sort this ResultSet according to document id.
 */
{
  if(THIS->d)
    fsort( THIS->d->hits, THIS->d->num_docs, 8, (void *)cmp_docid );
  RETURN_THIS();
}

static void f_resultset_dup( INT32 args )
/*! @decl ResultSet dup()
 *!   Return a new resultset with the same contents as this one.
 */
{
  struct object *o = clone_object( resultset_program, 0 );
  if(THIS->d)
  {
    if (T(o)->allocated_size < THIS->d->num_docs) {
      ResultSet *d = xalloc( THIS->d->num_docs * 8 + 4 );
      if (T(o)->d) free(T(o)->d);
      T(o)->d = d;
      T(o)->allocated_size = T(o)->d->num_docs;
    }
    memcpy( T(o)->d, THIS->d, THIS->d->num_docs * 8 + 4 );
  }
  pop_n_elems( args );
  wf_resultset_push( o );
}


static void f_resultset__sizeof( INT32 args )
/*! @decl int _sizeof()
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
/*! @decl int overhead()
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
  if (T(dest)->d) {
    free(T(dest)->d);
    T(dest)->d = NULL;
  }
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
    T(dest)->d              = xalloc( size );
    memcpy( T(dest)->d, T(src)->d, size );
  }
}

static void f_resultset_or( INT32 args )
/*! @decl ResultSet `|( ResultSet a )
 *! @decl ResultSet `+( ResultSet a )
 *! @decl ResultSet or( ResultSet a )
 *!
 *! Add the given resultsets together, to generate a resultset with
 *! both sets included. The rankings will be added if a document
 *! exists in both resultsets.
 */
{
  struct object *res = wf_resultset_new();
  struct object *left = Pike_fp->current_object;
  struct object *right;
  int lp=-1, rp=-1;

  int left_valid = 0, right_valid = 0;
  int left_left=1, right_left=1;
  int right_size, left_size;

  int left_doc=0, left_rank=0, right_doc=0, right_rank=0, last=-1;
  ResultSet *set_r, *set_l = T(left)->d;

  get_all_args( NULL, args, "%o", &right );

  set_r = T( WF_RESULTSET(right) )->d;

  if( !set_l || !set_l->num_docs )
  {
    if( set_r )
      duplicate_resultset( res, right );
    pop_n_elems(args);
    wf_resultset_push( res );
    return;
  }

  if( !set_r || !set_r->num_docs )
  {
    duplicate_resultset( res, left );
    pop_n_elems(args);
    wf_resultset_push( res );
    return;
  }

  left_left = !!(left_size = set_l->num_docs);
  right_left = !!(right_size = set_r->num_docs);

  while( left_left || right_left )
  {
    if( left_left && !left_valid ) /* New from left */
    {
      if( ++lp >= left_size )
      {
	left_left = 0;
	left_rank = 0;
	lp = left_size;
	if( !right_left )
	  break;
      }
      else
      {
	left_doc = set_l->hits[lp].doc_id;
	left_rank = set_l->hits[lp].ranking;
	left_valid = 1;
      }
    }

    if( right_left && !right_valid ) /* New from right */
    {
      if( ++rp >= right_size )
      {
	right_left = 0;
	right_rank = 0;
	rp = right_size;
	if( !left_left )
	  break;
      }
      else
      {
	right_doc = set_r->hits[rp].doc_id;
	right_rank = set_r->hits[rp].ranking;
	right_valid = 1;
      }
    }

    if(left_valid && ((left_doc <= right_doc) || !right_valid))
    {
      if(left_doc>last)
	wf_resultset_add( res, (last = left_doc), left_rank );
      else if( left_doc == last )
	wf_resultset_add_ranking( res, -1, left_rank );
      left_valid = 0;
    } else if(right_valid && (right_doc <= left_doc || !left_valid ) )
    {
      if(right_doc>last)
	wf_resultset_add( res, (last = right_doc), right_rank );
      else if( right_doc == last )
	wf_resultset_add_ranking( res, -1, right_rank );
      right_valid = 0;
    }
  }
  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_resultset_intersect( INT32 args )
/*! @decl ResultSet intersect( ResultSet a )
 *! @decl ResultSet `&( ResultSet a )
 *!
 *! Return a new resultset with all entries that are present in _both_
 *! sets. Only the document_id is checked, the resulting ranking is
 *! the sum of the rankings if the two sets.
 */
{
  struct object *res = wf_resultset_new();
  struct object *left = Pike_fp->current_object;
  struct object *right;
  int lp=-1, rp=-1;

  int left_valid = 0, right_valid = 0;
  int left_left=1, right_left=1;
  int right_size, left_size;

  int left_doc=0, left_rank=0, right_doc=0, right_rank=2147483647, last=-1;
  ResultSet *set_r, *set_l = T(left)->d;

  get_all_args( NULL, args, "%o", &right );

  right = WF_RESULTSET( right );
  set_r = T(right)->d;

  if( !set_l || !set_l->num_docs || !set_r || !set_r->num_docs )
  {
    /* ({}) & X == ({}) && X & ({}) == ({}) */
    pop_n_elems(args);
    wf_resultset_push(res);
    return;
  }

  left_left = !!(left_size = set_l->num_docs);
  right_left = !!(right_size = set_r->num_docs);

  while( left_left && right_left )
  {
    if( !left_valid ) /* New from left */
    {
      if( ++lp >= left_size )
      {
	left_left = 0;
	break;
      }
      else
      {
	left_doc = set_l->hits[lp].doc_id;
	left_rank = set_l->hits[lp].ranking;
	left_valid = 1;
      }
    }

    if( !right_valid ) /* New from right */
    {
      if( ++rp >= right_size )
      {
	right_left = 0;
	break;
      }
      else
      {
	right_doc = set_r->hits[rp].doc_id;
	right_rank = set_r->hits[rp].ranking;
	right_valid = 1;
      }
    }

    if (left_doc < right_doc) {
      left_valid = 0;
      continue;
    }

    if (right_doc < left_doc) {
      right_valid = 0;
      continue;
    }

    if (right_rank < left_rank) left_rank = right_rank;
    wf_resultset_add(res, left_doc, left_rank);
    left_valid = right_valid = 0;
  }
  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_resultset_add_ranking( INT32 args )
/*! @decl ResultSet add_ranking( ResultSet a )
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

  get_all_args( NULL, args, "%o", &right );

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
    if( left_used ) /* New from left */
    {
      if( ++lp == left_size )
      {
	left_left = 0;
	break;
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
  pop_n_elems( args );
  wf_resultset_push( res );
}

static void f_resultset_sub( INT32 args )
/*! @decl ResultSet sub( ResultSet a )
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

  get_all_args( NULL, args, "%o", &right );

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
	break;
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
  pop_n_elems( args );
  wf_resultset_push( res );
}

void exit_resultset_program(void)
{
  if (resultset_program) {
    free_program (resultset_program);
    resultset_program = NULL;
  }
  if (dateset_program) {
    free_program (dateset_program);
    dateset_program = NULL;
  }
}

static struct object *dup_dateset()
{
  struct object *o = clone_object( dateset_program, 0 );
  if(THIS->d)
  {
    ResultSet *d;
    if (T(o)->allocated_size <= THIS->d->num_docs) {
      d = xalloc( THIS->d->num_docs * 8 + 4 );
      if (T(o)->d) free(T(o)->d);
      T(o)->d = d;
      T(o)->allocated_size = T(o)->d->num_docs;
    }
    T(o)->d->num_docs = 0;
  }
  else
  {
    wf_resultset_clear( o );
  }
  return o;
}

static void f_dateset_finalize( INT32 args )
{
  int i;
  ResultSet *source = THIS->d;
  if (source) {
    for( i = 0; i<source->num_docs; i++ )
      source->hits[i].ranking = 0;
  }
  RETURN_THIS();
}

#define DUP_DATESET()   do {			\
    pop_n_elems(args);				\
    o = dup_dateset();				\
    res = T(o)->d;				\
    push_object( o );				\
  } while(0)


static void f_dateset_before( INT32 args )
{
  int before, i;
  struct object *o;
  ResultSet *source = THIS->d;
  ResultSet *res;

  get_all_args( NULL, args, "%d", &before );
  DUP_DATESET();

  if (source) {
    for( i = 0; i<source->num_docs; i++ )
      if( (int) source->hits[i].ranking < before )
	res->hits[res->num_docs++] = source->hits[i];
  }
}

static void f_dateset_after( INT32 args )
{
  int after, i;
  struct object *o;
  ResultSet *source = THIS->d;
  ResultSet *res;

  get_all_args( NULL, args, "%d", &after );
  DUP_DATESET();

  if (source) {
    for( i = 0; i<source->num_docs; i++ )
      if( (int) source->hits[i].ranking > after )
	res->hits[res->num_docs++] = source->hits[i];
  }
}

static void f_dateset_between( INT32 args )
{
  int before, after, i;
  struct object *o;
  ResultSet *source = THIS->d;
  ResultSet *res;

  get_all_args( NULL, args, "%d%d", &after, &before );
  DUP_DATESET();

  if (source) {
    if( before <= after )
      return;

    for( i = 0; i<source->num_docs; i++ )
      if( ((int) source->hits[i].ranking > after) &&
	  ((int) source->hits[i].ranking < before) )
	res->hits[res->num_docs++] = source->hits[i];
  }
}

static void f_dateset_not_between( INT32 args )
{
  int before, after, i;
  struct object *o;
  ResultSet *source = THIS->d;
  ResultSet *res;

  get_all_args( NULL, args, "%d%d", &after, &before );
  DUP_DATESET();

  if (source) {
    if( before <= after )
      return;

    for( i = 0; i<source->num_docs; i++ )
      if( ((int) source->hits[i].ranking < after) ||
	  ((int) source->hits[i].ranking > before) )
	res->hits[res->num_docs++] = source->hits[i];
  }
}

static void f_resultset_add( INT32 args )
{
  INT64 d, h;
  get_all_args( NULL, args, "%l%l", &d, &h );
  wf_resultset_add( Pike_fp->current_object, d, h );
  pop_n_elems(args);
}

static void f_resultset_add_many( INT32 args )
{
  struct array *a, *b;
  int i;
  get_all_args( NULL, args, "%a%a", &a, &b );
  if( a->size != b->size )
    Pike_error("Expected equally sized arrays\n");
  for( i=0;i<a->size;i++ )
  {
    INT64 ri, di;
    if( TYPEOF(a->item[i]) == PIKE_T_OBJECT )
    {
      push_object( a->item[i].u.object );
      get_all_args( NULL, 1, "%l", &di );
      Pike_sp--;
    }
    else
      di = a->item[i].u.integer;
    if( TYPEOF(b->item[i]) == PIKE_T_OBJECT )
    {
      push_object( b->item[i].u.object );
      get_all_args( NULL, 1, "%l", &ri );
      Pike_sp--;
    }
    else
      ri = b->item[i].u.integer;
    wf_resultset_add( Pike_fp->current_object, di, ri );
  }
  pop_n_elems(args);
}

void init_resultset_program(void)
{
  start_new_program();
  {
    ADD_STORAGE( struct result_set_p );
    ADD_FUNCTION("cast", f_resultset_cast, tFunc( tStr, tMix ), ID_PRIVATE );
    ADD_FUNCTION("create",f_resultset_create,
                 tFunc( tOr(tArr(tOr(tInt,tArr(tInt))),tVoid), tVoid ), 0);

    ADD_FUNCTION("sort",f_resultset_sort,tFunc(tVoid,tObj),0);
    ADD_FUNCTION("sort_rev",f_resultset_sort_rev,tFunc(tVoid,tObj),0);
    ADD_FUNCTION("sort_docid",f_resultset_sort_docid,tFunc(tVoid,tObj),0);

    ADD_FUNCTION("dup",f_resultset_dup,tFunc(tVoid,tObj),0);

    ADD_FUNCTION("slice",f_resultset_slice,
                 tFunc(tInt tInt,tArr(tArr(tInt))),0);

    ADD_FUNCTION( "or", f_resultset_or, tFunc(tObj,tObj), 0 );
    ADD_FUNCTION( "`|", f_resultset_or, tFunc(tObj,tObj), 0 );
    ADD_FUNCTION( "`+", f_resultset_or, tFunc(tObj,tObj), 0 );

    ADD_FUNCTION( "sub", f_resultset_sub, tFunc(tObj,tObj), 0 );
    ADD_FUNCTION( "`-", f_resultset_sub, tFunc(tObj,tObj), 0 );

    ADD_FUNCTION( "add_ranking", f_resultset_add_ranking, tFunc(tObj,tObj), 0);

    ADD_FUNCTION( "intersect", f_resultset_intersect, tFunc(tObj,tObj), 0);
    ADD_FUNCTION( "`&", f_resultset_intersect, tFunc(tObj,tObj), 0 );

    ADD_FUNCTION("add", f_resultset_add, tFunc(tInt tInt,tVoid), 0 );
    ADD_FUNCTION("add_many", f_resultset_add_many,
                 tFunc(tArr(tInt) tArr(tInt),tVoid), 0 );
    ADD_FUNCTION("_sizeof",f_resultset__sizeof,tFunc(tVoid,tInt), 0 );
    ADD_FUNCTION("size",f_resultset__sizeof,tFunc(tVoid,tInt), 0 );

    /* debug related functions */
    ADD_FUNCTION("memsize",f_resultset_memsize,tFunc(tVoid,tInt), 0 );
    ADD_FUNCTION("overhead",f_resultset_overhead,tFunc(tVoid,tInt), 0 );
    ADD_FUNCTION("test", f_resultset_test, tFunc(tInt tInt tInt,tInt), 0 );
    ADD_FUNCTION( "finalize", f_dateset_finalize, tFunc(tVoid,tObj), 0 );

    set_exit_callback( free_rs );
  }
  resultset_program = end_program( );
  add_program_constant( "ResultSet", resultset_program,0 );

  start_new_program();
  {
    struct svalue x;
    SET_SVAL(x, PIKE_T_PROGRAM, 0, program, resultset_program);
    ADD_FUNCTION( "before", f_dateset_before,  tFunc(tInt,tObj), 0 );
    ADD_FUNCTION( "after", f_dateset_after,    tFunc(tInt,tObj), 0 );
    ADD_FUNCTION( "between", f_dateset_between,tFunc(tInt tInt,tObj), 0 );
    ADD_FUNCTION( "not_between", f_dateset_not_between,
                  tFunc(tInt tInt,tObj), 0 );
    do_inherit( &x, 0, NULL );
  }
  dateset_program = end_program( );
  add_program_constant( "DateSet", dateset_program,0 );
}

/*! @endclass ResultSet
 */

/*! @endmodule
 */
