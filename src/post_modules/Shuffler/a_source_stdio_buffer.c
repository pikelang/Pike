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

  off_t lastlen;
  struct object *obj;
};

static struct data get_data( struct source *src, off_t len )
{
  struct sb_source *s = (struct sb_source *)src;
  struct data res;

  Buffer *io = io_buffer_from_object(s->obj);

  io_consume(io, s->lastlen);
  res.data = io_read_pointer(io);
  s->lastlen = res.len = len = len > io_len(io) ? io_len(io) : len;

  s->s.eof = !len;
  return res;
}

static void free_source( struct source *src )
{
  struct sb_source *s = (struct sb_source *)src;
  free_object(s->obj);
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
    INT64 slen;

    if (start)
      io_consume(io, start);

    slen = io_len(io);
    if (len >= 0 && slen < len)
      slen = len;
    if (slen < 0)
      slen = 0;
    push_int64(slen);
    apply(o, "read_buffer", 1);
  }

  if (TYPEOF(Pike_sp[-1]) != PIKE_T_OBJECT)
    Pike_error("Some data went missing.\n");

  o = Pike_sp[-1].u.object;
  Pike_sp--;

  res->lastlen = 0;
  res->s.get_data = get_data;
  res->s.free_source = free_source;
  add_ref(res->obj = o);
  return (struct source *)res;
}

void source_stdio_buffer_exit( )
{
}

void source_stdio_buffer_init( )
{
}
