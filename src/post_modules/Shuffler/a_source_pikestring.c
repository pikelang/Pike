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


/* Source: String
 * Argument: 8-bit string
 */

struct ps_source
{
  struct source s;

  struct pike_string*str;
  char *data;
  size_t len;
};

static struct data get_data( struct source *src, off_t len )
{
  struct ps_source *s = (struct ps_source *)src;
  struct data res;

  if (len > s->len) {
    len = s->len;
    s->s.eof = 1; /* next read will be done from the next source */
  }

  res.data = s->data;
  s->data += len;
  s->len -= res.len = len;

  return res;
}

static void free_source( struct source *src )
{
  free_string(((struct ps_source *)src)->str);
}

struct source *source_pikestring_make( struct svalue *s,
				       INT64 start, INT64 len )
{
  struct ps_source *res;
  INT64 slen;

  if (TYPEOF(*s) != PIKE_T_STRING
   || s->u.string->size_shift)
    return 0;

  if (!(res = calloc(1, sizeof(struct ps_source))))
    return 0;

  debug_malloc_touch(res);

  res->s.free_source = free_source;
  res->s.get_data = get_data;

  copy_shared_string(res->str, s->u.string);
  slen = res->str->len;
  if (start > slen)
    start = slen;
  res->data = res->str->str + start;
  slen -= start;

  if (len < 0)
    len = slen;
  if (len > slen)
    len = slen;

  res->len = len;
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
