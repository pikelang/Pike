/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: split.c,v 1.5 2006/02/28 22:39:32 marcus Exp $
*/

#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: split.c,v 1.5 2006/02/28 22:39:32 marcus Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "module_support.h"

#include "config.h"
#include "buffer.h"
#include "split.h"

static inline struct words *uc_words_mkspace( struct words *d, int n )
{
  while( d->size+n > d->allocated_size )
  {
    d->allocated_size *= 2;
    d = realloc( d, d->allocated_size*8+(sizeof(struct words)-8) );
  }
  return d;
}

struct words *uc_words_write( struct words *d,
			      unsigned int start,
			      unsigned int len )
{
  d = uc_words_mkspace( d,1 );
  d->words[d->size].start = start;
  d->words[d->size].size = len;
  d->size++;
  return d;
}

struct words *uc_words_new( )
{
  struct words *s = malloc( sizeof( struct words ) + 31*8 );
  s->allocated_size = 32;
  s->size = 0;
  return s;
}

void uc_words_free( struct words *w )
{
  free( w );
}

#include "wordbits.h"

static inline int _unicode_is_wordchar( int c )
{
  unsigned int i;
  for( i = 0; i<sizeof(ranges)/sizeof(ranges[0]); i++ )
    if( c <= ranges[i].end )
      return (c>=ranges[i].start?
	      ( (c >= 0x3400 && c <= 0x9fff) ||
		(c >= 0x20000 && c <= 0x2ffff) ? /* CJK */ 2 : 1 )
	      : 0);
  return 0;
}

int unicode_is_wordchar( int c )
{
  return _unicode_is_wordchar(c);
}

struct words *unicode_split_words_pikestr0( struct pike_string *data )
{
  unsigned int i;
  unsigned int in_word = 0;
  unsigned int last_start = 0;
  struct words *res = uc_words_new();
  unsigned char *ptr = data->str;
  unsigned int sz = data->len;
  
  for( i=0; i<sz; i++, ptr++ )
  {
    switch( _unicode_is_wordchar( *ptr ) )
    {
      case 1: /* normal  */
	if( *ptr > 127 )
	{
	  uc_words_free( res );
	  return NULL;
	}
	if( !in_word )
	{
	  last_start = i;
	  in_word=1;
	}
	break;
      case 0: /* not */
	if( in_word )
	{
	  in_word=0;
	  res = uc_words_write( res, last_start, i-last_start );
	}
	break;
    }
  }
  if( in_word )
    return uc_words_write( res, last_start, i-last_start );
  return res;
}

struct words *unicode_split_words_buffer( struct buffer *data )
{
  unsigned int i;
  unsigned int in_word = 0;
  unsigned int last_start = 0;
  struct words *res = uc_words_new();
  unsigned int *ptr = data->data;
  unsigned int sz = data->size;
  for( i=0; i<sz; i++ )
  {
    switch( _unicode_is_wordchar( *ptr++ ) )
    {
      case 1: /* normal  */
	if( !in_word )
	{
	  last_start = i;
	  in_word=1;
	}
	break;
      case 0: /* not */
	if( in_word )
	{
	  in_word=0;
	  res = uc_words_write( res, last_start, i-last_start );
	}
	break;
      case 2: /* single character word */
	if( in_word )
	{
	  in_word=0;
	  res = uc_words_write( res, last_start, i-last_start );
	}
	res = uc_words_write( res, i, 1 );
    }
  }
  if( in_word )
    return uc_words_write( res, last_start, i-last_start );
  return res;
}
