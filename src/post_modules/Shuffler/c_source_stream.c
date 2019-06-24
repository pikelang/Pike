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

struct program *source_program;

#ifdef THIS
#undef THIS
#endif

#define THIS ((struct fd_source *)(Pike_fp->current_storage))

struct fd_source
{
  struct source s;

  struct object *self;
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

static void remove_callbacks(struct source *src) {
  struct fd_source *s = (struct fd_source *)src;
  if (s->obj->prog)
    set_read_callback(s->fd, 0, 0);
}

static void read_callback( int UNUSED(fd), struct fd_source *s )
{
  if (!s->obj)     /* Already finished */
    return;       /* FIXME Should we throw an error here? */

  remove_callbacks( (struct source *)s );

  if (s->available > 0)
    s->readwanted = 1;	 /* Remember to do a read when the buffer is empty */
  else if (doread(s) && s->when_data_cb)
    s->when_data_cb (s->when_data_cb_arg);
}

static void setup_callbacks( struct source *src ) {
  struct fd_source *s = (struct fd_source *)src;
  set_read_callback( s->fd, (void*)read_callback, s );
}

static struct data get_data(struct source *src, off_t len)
{
  struct fd_source *s = (struct fd_source *)src;
  struct data res;

reload:
  if (s->readwanted < 0)
    s->s.eof = 1;
  if (res.len = len = s->available) {
    res.data = s->data;
    s->available = 0;
  } else if (s->readwanted > 0) {
    doread(s);
    goto reload;
  } else if (!s->obj->prog) {      /* Object imploded before we were done */
    s->s.eof = 1;	          /* FIXME should we throw an error here? */
  } else if (!s->readwanted) {
   /* No data available, but there should be in the future (no EOF, nor
    * out of the range of data to send as specified by the arguments to
    * source_stream_make)
    */
    res.len = -2;
    setup_callbacks(src);
  }

  return res;
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

static void free_source(struct source *src) {
  free_object(((struct fd_source*)src)->self);
}

struct source *source_stream_make(struct svalue *sv,
				  INT64 start, INT64 len) {
  struct fd_source *s;

  if (TYPEOF(*sv) != PIKE_T_OBJECT
   || !is_stdio_file(sv->u.object)
   || find_identifier("query_fd", sv->u.object->prog) < 0)
    return 0;

  {
    struct object *p;

    if (!(p = clone_object(source_program, 0)))
      return 0;

    s = (struct fd_source*)p->storage;
    s->self = p;
  }

  add_ref(s->obj = sv->u.object);

  apply(s->obj, "query_fd", 0);
  s->fd = Pike_sp[-1].u.integer;
  pop_stack();

  s->len = len;
  s->skip = start;
  s->s.get_data = get_data;
  s->s.free_source = free_source;
  s->s.set_callback = set_callback;
  s->s.setup_callbacks = setup_callbacks;
  s->s.remove_callbacks = remove_callbacks;
  return (struct source *)s;
}

static void source_destruct(struct object *UNUSED(o)) {
  remove_callbacks((struct source*)THIS);
  free_object(THIS->obj);
  THIS->obj = 0;
}

void source_stream_exit() {
  free_program(source_program);
}

void source_stream_init() {
  start_new_program();
  ADD_STORAGE(struct fd_source);
  set_exit_callback(source_destruct);
  source_program = end_program();
}
