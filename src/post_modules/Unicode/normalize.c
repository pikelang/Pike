/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: normalize.c,v 1.14 2004/10/07 22:19:12 nilsson Exp $
*/

#include "global.h"
#include "stralloc.h"
#include "global.h"
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "module_support.h"

#include "config.h"
#include "buffer.h"
#include "normalize.h"

struct comp
{
  const int c1;
  const int c2;
  const int c;
}; /* 12 bytes/entry */

struct decomp
{
  const int c;
  const int compat;
  const int data[2];
}; /* 12 bytes/entry */

struct comp_h
{
  const struct comp   *v;
  struct comp_h *next;
};

struct decomp_h
{
  const struct decomp   *v;
  struct decomp_h *next;
};

struct canonic_h
{
  const struct canonical_cl   *v;
  struct canonic_h *next;
};

struct canonical_cl
{
  const int c;
  const int cl;
}; /* 8 bytes/entry */

#include "hsize.h"
/* generated from .txt */
#include "decompositions.h"
#include "canonicals.h"

static struct   comp_h   comp_h[sizeof(_c)/sizeof(_c[0])];
static struct   comp_h  *comp_hash[HSIZE];

static struct decomp_h   decomp_h[sizeof(_d)/sizeof(_d[0])];
static struct decomp_h  *decomp_hash[HSIZE];

static struct canonic_h   canonic_h[sizeof(_ca)/sizeof(_ca[0])];
static struct canonic_h  *canonic_hash[HSIZE];


#ifdef PIKE_DEBUG
static int hashes_inited = 0;
#endif

static void init_hashes()
{
  unsigned int i;

#ifdef PIKE_DEBUG
  if (hashes_inited) Pike_fatal ("init_hashes called twice\n");
  hashes_inited = 1;
#endif

  for( i = 0; i<sizeof(_d)/sizeof(_d[0]); i++ )
  {
    int h = _d[i].c%HSIZE;
    decomp_h[i].v = _d+i;
    decomp_h[i].next = decomp_hash[h];
    decomp_hash[h] = decomp_h+i;
  }
  for( i = 0; i<sizeof(_c)/sizeof(_c[0]); i++ )
  {
    int h = ((_c[i].c1<<16)|_c[i].c2)%HSIZE;
    comp_h[i].v = _c+i;
    comp_h[i].next = comp_hash[h];
    comp_hash[h] = comp_h+i;
  }
  for( i = 0; i<sizeof(_ca)/sizeof(_ca[0]); i++ )
  {
    int h = _ca[i].c % HSIZE;
    canonic_h[i].v = _ca+i;
    canonic_h[i].next = canonic_hash[h];
    canonic_hash[h] = canonic_h+i;
  }
}


void unicode_normalize_init()
{
  init_hashes();
}

const struct decomp *get_decomposition( int c )
{
  int hv = c % HSIZE;
  const struct decomp_h *r = decomp_hash[hv];
  while( r )
  {
    if( r->v->c == c )
      return r->v;
    r = r->next;
  }
  return 0;
}

int get_canonical_class( int c )
{
  int hv = c % HSIZE;
  const struct canonic_h *r = canonic_hash[hv];
  while( r )
  {
    if( r->v->c == c )
      return r->v->cl;
    r = r->next;
  }
  return 0;
}

#define SBase 0xAC00
#define LBase 0x1100
#define VBase 0x1161
#define TBase 0x11A7
#define LCount 19
#define VCount 21
#define TCount 28
#define NCount (VCount * TCount)
#define SCount (LCount * NCount)

int get_compose_pair( int c1, int c2 )
{
  const struct comp_h *r;
  if( c1 >= LBase )
  {
    /* Perhaps hangul */
    int LIndex = c1-LBase, SIndex;
    if( LIndex < LCount )
    {
      int VIndex = c2-VBase;
      if( 0 <= VIndex && VIndex < VCount )
	return SBase + (LIndex*VCount + VIndex)*TCount;
    }

    if( c1 >= SBase )
    {
      SIndex = c1-SBase;
      if( SIndex < SCount && (SIndex % TCount)== 0 )
      {
	int TIndex = c2-TBase;
	if( 0 <= TIndex && TIndex <= TCount )
	  /* LVT */
	  return c1+TIndex;
      }
    }
  }

  /* Nope. Not hangul. */
  for( r=comp_hash[ ((unsigned int)((c1<<16) | (c2))) % HSIZE ]; r; r=r->next )
    if( (r->v->c1 == c1) && (r->v->c2 == c2) )
      return r->v->c;

  return 0;
}

static void rec_get_decomposition( int canonical, int c, struct buffer *tmp )
{
  const struct decomp *decomp;
  if( (decomp = get_decomposition( c )) && !(canonical && decomp->compat) )
  {
    if( decomp->data[0] )
      rec_get_decomposition( canonical, decomp->data[0], tmp );
    if( decomp->data[1] )
      rec_get_decomposition( canonical, decomp->data[1], tmp );
  }
  else
  {
    if( (c >= SBase) && c < (SBase+SCount) )
      /* Hangul */
    {
      int l, v, t;
      c-=SBase;
      l= LBase + c / NCount;
      v = VBase + (c % NCount) / TCount;
      t = TBase + (c % TCount);
      uc_buffer_write( tmp, l );
      uc_buffer_write( tmp, v );
      if( t != TBase )
	uc_buffer_write( tmp, t );
    }
    else
      uc_buffer_write( tmp, c );
  }
}

struct buffer *unicode_decompose_buffer( struct buffer *source,	int how )
{
  unsigned int i, j;
  struct buffer *res = uc_buffer_new();
  struct buffer *tmp = uc_buffer_new();
  int canonical = !(how & COMPAT_BIT);

  for( i = 0; i<source->size; i++ )
  {
    if( source->data[i] < 160 )
    {
      uc_buffer_write( res, source->data[i] );
    }
    else
    {
      tmp->size = 0;
      rec_get_decomposition( canonical, source->data[i], tmp );
      for( j = 0; j<tmp->size; j++ )
      {
	int c = tmp->data[j];
	int cl = get_canonical_class( c );
	int k = res->size;
	/* Sort combining marks */
	if( cl != 0 )
	{
	  for( ; k > 0; k-- )
	    if( get_canonical_class( res->data[k-1] ) <= cl )
	      break;
	}
	uc_buffer_insert( res, k, c );
      }
    }
  }
  uc_buffer_free( tmp );
  uc_buffer_free( source );
  return res;
}

struct buffer *unicode_compose_buffer( struct buffer *source, int how )
{
  int startch = source->data[0];
  int lastclass = get_canonical_class( startch )?256:0;
  unsigned int startpos = 0, comppos=1;
  unsigned int pos;
  
  for( pos = 1; pos < source->size; pos++ )
  {
    int ch = source->data[ pos ];
    int cl = get_canonical_class( ch );
    int co = get_compose_pair( startch, ch );

    if( co && ((lastclass < cl) || (lastclass == 0)) )
      source->data[ startpos ] = startch = co;
    else
    {
      if( cl == 0 )
      {
	startpos = comppos;
	startch = ch;
      }
      lastclass = cl;
      source->data[comppos++] = ch;
    }
  }
  source->size = comppos;
  return source;
}


struct pike_string *unicode_normalize( struct pike_string *source,
				       int how )
{
  /* Special case for the empty string. */
  if (!source->len) {
    add_ref(source);
    return source;
  }
  /* What, me lisp? */
  if( how & COMPOSE_BIT )
    return
      uc_buffer_to_pikestring(
	unicode_compose_buffer(
	  unicode_decompose_buffer(
	    uc_buffer_write_pikestring(
	      uc_buffer_new(),
	      source ),
	    how ),
	  how ) );
  return
    uc_buffer_to_pikestring(
      unicode_decompose_buffer(
	uc_buffer_write_pikestring(
	  uc_buffer_new(),
	  source ),
	how ) );
}
