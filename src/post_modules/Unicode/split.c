/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id$");
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

struct words *uc_words_mkspace( struct words *d, int n )
{
  while( d->size+n > d->allocated_size )
  {
    d->allocated_size += 256;
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

int unicode_is_wordchar( int c )
{
  /* Ideographs */
  unsigned int i;
  if( c>=0x5000 && c<= 0x9fff ) /* CJK */
    return 2;
  
  for( i = 0; i<sizeof(ranges)/sizeof(ranges[0]); i++ )
    if( c <= ranges[i].end )
      return c>=ranges[i].start;
  return 0;
}

struct words *unicode_split_words_buffer( struct buffer *data )
{
  unsigned int i;
  unsigned int in_word = 0;
  unsigned int last_start = 0;
  struct words *res = uc_words_new();

  for( i=0; i<data->size; i++ )
  {
    switch( unicode_is_wordchar( data->data[i] ) )
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
