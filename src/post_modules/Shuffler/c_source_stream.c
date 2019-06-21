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
#include "backend.h"

#include <sys/stat.h>

#include "shuffler.h"

#define CHUNK 8192


/* Source: Stream
 * Argument: Stdio.File instance pointing to a stream
 *           of some kind (network socket, named pipes, stdin etc)
 */


struct fd_source
{
  struct source s;

  struct object *obj;
  char buffer[CHUNK];
  char *data;
  size_t available;
  int readwanted;
  int fd;

  void (*when_data_cb)( void *a );
  void *when_data_cb_arg;
  INT64 len, skip;
};


static void read_callback( int fd, struct fd_source *s );
static void setup_callbacks( struct source *src )
{
  struct fd_source *s = (struct fd_source *)src;
  set_read_callback( s->fd, (void*)read_callback, s );
}

static void remove_callbacks( struct source *src )
{
  struct fd_source *s = (struct fd_source *)src;
  if (s->obj->prog)
    set_read_callback( s->fd, 0, 0 );
}

static int doread(struct fd_source *s) {
  int l = fd_read(s->fd, s->buffer, CHUNK);
  if (l > 0) {
    if (s->skip >= l) {
      s->skip -= l;
      return 0;
    }
    s->data = s->buffer + s->skip;
    s->available = l - s->skip;
    s->skip = 0;
    s->readwanted = 0;
  } else
    s->readwanted = -1;
  return 1;
}

static struct data get_data(struct source *src, off_t len)
{
  struct fd_source *s = (struct fd_source *)src;
  struct data res;

  for (;;) {
    if (s->readwanted < 0)
      s->s.eof = 1;
    if (len = s->available) { /* There is data in the buffer. Return it. */
      res.data = s->data;
      res.len = len;
      s->available = 0;
    } else if (s->readwanted > 0) {
      doread(s);
      continue;
    } else if (s->readwanted < 0) {
      res.len = 0;
    } else {
     /* No data available, but there should be in the future (no EOF, nor
      * out of the range of data to send as specified by the arguments to
      * source_stream_make)
      */
      res.len = -2;
      setup_callbacks(src);
    }
    break;
  }

  return res;
}


static void free_source( struct source *src )
{
  remove_callbacks( src );
  free_object(((struct fd_source *)src)->obj);
}

static void read_callback( int UNUSED(fd), struct fd_source *s )
{
  remove_callbacks( (struct source *)s );

  if (s->available > 0)
    s->readwanted = 1;	 /* Remember to do a read when the buffer is empty */
  else if (doread(s) && s->when_data_cb)
    s->when_data_cb (s->when_data_cb_arg);
}

static void set_callback( struct source *src, void (*cb)( void *a ), void *a )
{
  struct fd_source *s = (struct fd_source *)src;
  s->when_data_cb = cb;
  s->when_data_cb_arg = a;;
}

static int is_stdio_file(struct object *o)
{
  struct program *p = o->prog;
  INT32 i = p->num_inherits;
  while( i-- )
  {
    if( p->inherits[i].prog->id == PROG_STDIO_FD_ID ||
        p->inherits[i].prog->id == PROG_STDIO_FD_REF_ID )
      return 1;
  }
  return 0;
}

struct source *source_stream_make( struct svalue *s,
				   INT64 start, INT64 len )
{
  struct fd_source *res;
  if (TYPEOF(*s) != PIKE_T_OBJECT
   || !is_stdio_file(s->u.object)
   || find_identifier("query_fd", s->u.object->prog) < 0)
    return 0;

  if (!(res = calloc(1, sizeof(struct fd_source))))
    return 0;

  apply(s->u.object, "query_fd", 0);
  res->fd = Pike_sp[-1].u.integer;
  pop_stack();

  res->len = len;
  res->skip = start;

  res->s.get_data = get_data;
  res->s.free_source = free_source;
  res->s.set_callback = set_callback;
  res->s.setup_callbacks = setup_callbacks;
  res->s.remove_callbacks = remove_callbacks;
  res->obj = s->u.object;
  add_ref(res->obj);
  return (struct source *)res;
}

void source_stream_exit( )
{
}

void source_stream_init( )
{
}
