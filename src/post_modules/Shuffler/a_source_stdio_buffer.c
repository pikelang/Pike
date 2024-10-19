/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "bignum.h"
#include "object.h"
#include "interpret.h"

#include "fdlib.h"
#include "fd_control.h"

#include <sys/stat.h>

#include "shuffler.h"
#include "modules/_Stdio/buffer.h"


/* Source: Stdio.Buffer object
 * Argument: Stdio.Buffer lookalike with read_buffer and consume
 */

struct sb_source
{
  struct source s;

  struct object *obj;
};

static struct data get_data( struct source *src, off_t PIKE_UNUSED(len) )
{
  struct sb_source *s = (struct sb_source *)src;
  struct data res;
  Buffer *io = io_buffer_from_object(s->obj);

  res.data = (char*)io_read_pointer(io);
  res.len = io_len(io);
  s->s.eof = 1;
  return res;
}

static void free_source( struct source *src )
{
  struct sb_source *s = (struct sb_source *)src;
  if (s->obj)
    free_object(s->obj);
  debug_malloc_touch(s);
  free(s);
}

struct source *source_stdio_buffer_make( struct svalue *s,
					     INT64 start, INT64 len )
{
  struct sb_source *res;
  struct object *o;

  if (TYPEOF(*s) != PIKE_T_OBJECT
   || find_identifier("read_buffer", s->u.object->prog) < 0)
    return 0;

  if (!(res = calloc(1, sizeof(struct sb_source))))
    return 0;

  o = s->u.object;

  {
    Buffer *io = io_buffer_from_object(o);
    INT64 slen = io_len(io);

    if (start) {
      if (start > slen)
	start = slen;
      io_consume(io, start);
      slen -= start;
    }

    if (len < 0)
      len = slen;
    if (len > slen)
      len = slen;

    push_int64(len);
    apply(o, "read_buffer", 1);
  }

  if (TYPEOF(Pike_sp[-1]) != PIKE_T_OBJECT)
    Pike_error("Some data went missing.\n");

  o = Pike_sp[-1].u.object;
  Pike_sp--;

  res->s.get_data = get_data;
  res->s.free_source = free_source;
  res->obj = o;
  return (struct source *)res;
}

void source_stdio_buffer_exit( )
{
}

void source_stdio_buffer_init( )
{
}
