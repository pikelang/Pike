/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stralloc.h"
#include "bignum.h"
#include "interpret.h"

#include "shuffler.h"


/* Source: Int
 * Argument: 8-bit byte
 */

struct i8_source
{
  struct source s;

  char datum;
};

static struct data get_data( struct source *src, off_t PIKE_UNUSED(len) )
{
  struct i8_source *s = (struct i8_source *)src;
  struct data res;

  s->s.eof = 1; /* next read will be done from the next source */
  res.len = 1; /* ignore len parameter and deliver what we have */
  res.data = &s->datum;

  return res;
}

static void free_source( struct source *src )
{
  struct i8_source *s = (struct i8_source *)src;
  debug_malloc_touch(s);
  free(s);
}

struct source *source_int8_make( struct svalue *s,
				       INT64 start, INT64 len )
{
  struct i8_source *res;

  if (TYPEOF(*s) != PIKE_T_INT)
    return 0;

  if (!(res = calloc(1, sizeof(struct i8_source))))
    return 0;

  debug_malloc_touch(res);

  res->datum = s->u.integer;
  res->s.free_source = free_source;
  res->s.get_data = get_data;

  if (start || !len)
    res->s.eof = 1; /* already consumed */

  return (struct source *)res;
}

void source_int8_init( )
{
  /* nothing to do here */
}

void source_int8_exit( )
{
  /* nothing to do here */
}
