/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: a_source_pikestring.c,v 1.10 2004/10/16 07:27:29 agehall Exp $
*/

#include "global.h"
#include "stralloc.h"
#include "bignum.h"
#include "interpret.h"

#include "shuffler.h"


/* Source: String
 * Argument: 8-bit string
 */

struct ps_source
{
  struct source s;

  struct pike_string *str;
  int offset, len;
};

static struct data get_data( struct source *_s, off_t len )
{
  struct ps_source *s = (struct ps_source *)_s;
  struct data res;
  
  res.do_free = 0;
  res.off = 0;
  res.data = s->str->str + s->offset;
  
  if( len > s->len )
  {
    len = s->len;
    s->s.eof = 1; /* next read will be done from the next source */
  }

  res.len = len;

  s->len -= len;
  s->offset += len;

  return res;
}

static void free_source( struct source *_s )
{
  free_string(((struct ps_source *)_s)->str);
}

struct source *source_pikestring_make( struct svalue *s,
				       INT64 start, INT64 len )
{
  struct ps_source *res;

  if( s->type != PIKE_T_STRING )   return 0;
  if( s->u.string->size_shift )    return 0;

  res = malloc( sizeof( struct ps_source ) );
  debug_malloc_touch( res );
  debug_malloc_touch( s );
  MEMSET( res, 0, sizeof( struct ps_source ) );

  res->s.free_source = free_source;
  res->s.get_data = get_data;

  copy_shared_string(res->str, s->u.string);
  res->offset = start;

  if( len != -1 )
  {
    if( len > res->str->len-start )
    {
      sub_ref(res->str);
      free(res);
      return 0;
    }
    else
      res->len = len;
  }
  else
    res->len = res->str->len-start;

  if( res->len <= 0 )
  {
    sub_ref(res->str);
    free(res);
    return 0;
  }
  return (struct source *)res;
}

void source_pikestring_init( )
{
  /* nothing to do here */
}

void source_pikestring_exit( )
{
  /* nothing to do here */
}
