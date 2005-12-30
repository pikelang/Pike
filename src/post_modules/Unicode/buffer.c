/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: buffer.c,v 1.7 2005/12/30 22:20:30 nilsson Exp $
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

static void buffer_mkspace( struct buffer *d, int n )
{
  while( d->size+n > d->allocated_size )
  {
    d->allocated_size += 512;
    d->data = realloc( d->data, d->allocated_size*4 );
  }
}

void uc_buffer_write( struct buffer *d, p_wchar2 data )
{
  buffer_mkspace( d, 1 );
  d->data[d->size++] = data;
}

INT32 uc_buffer_read( struct buffer *d )
{
  if( d->rpos < d->size )
    return d->data[d->rpos++];
  return 0;
}

struct buffer *uc_buffer_new_size( int s )
{
  struct buffer *res = malloc( sizeof( struct buffer ) );
  res->allocated_size = s;
  res->size = 0;
  res->data = malloc(s * 4);
  return res;
}

struct buffer *uc_buffer_new( )
{
  return uc_buffer_new_size( 32 );
}

struct buffer *uc_buffer_from_pikestring( struct pike_string *s )
{
  struct buffer *res = uc_buffer_new_size( s->len );
  uc_buffer_write_pikestring( res, s );
  return res;
}

struct buffer *uc_buffer_write_pikestring( struct buffer *d,
					   struct pike_string *s )
{
  switch( s->size_shift )
  {
    case 0:
      {
	p_wchar0 *p = STR0(s);
	int i;
	for( i = 0; i<s->len; i++ )
	  uc_buffer_write( d, p[i] );
      }
      break;
    case 1:
      {
	p_wchar1 *p = STR1(s);
	int i;
	for( i = 0; i<s->len; i++ )
	  uc_buffer_write( d, p[i] );
      }
      break;
    case 2:
      {
	p_wchar2 *p = STR2(s);
	int i;
	for( i = 0; i<s->len; i++ )
	  uc_buffer_write( d, p[i] );
      }
      break;
  }
  return d;
}

void uc_buffer_free( struct buffer *d)
{
  free( d->data );
  free( d );
}

struct pike_string *uc_buffer_to_pikestring( struct buffer *d )
{
  struct pike_string * s = make_shared_binary_string2( d->data, d->size );
  uc_buffer_free( d );
  return s;
}

void uc_buffer_insert( struct buffer *b, unsigned int pos, p_wchar2 c )
{
  unsigned int i;
  if( pos == b->size )
    uc_buffer_write( b, c );
  else
  {
    uc_buffer_write( b, 0 );
    for( i = b->size-1; i>pos; i-- )
      b->data[i] = b->data[i-1];
    b->data[pos] = c;
  }
}
