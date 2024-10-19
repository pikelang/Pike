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

/* Source: Pike-Stream
 * Argument: Stdio.File lookalike with read
 *           callback support (set_read_callback)
 */
static struct program *source_program;

#ifdef THIS
#undef THIS
#endif

#define THIS ((struct pf_source *)(Pike_fp->current_storage))

struct pf_source
{
  struct source s;

  struct object *self;
  struct object *obj;
  struct pike_string *str;
  char *data;
  size_t available;
  int eof;

  void (*when_data_cb)( void *a );
  struct object *when_data_cb_arg;
  size_t len, skip;
};

static void setup_callbacks( struct source *src )
{
  struct pf_source *s = (struct pf_source *)src;
  ref_push_object(s->self);
  apply(s->obj, "set_close_callback", 1);	/* Set this first */
  pop_stack();
  ref_push_object(s->self);
  apply(s->obj, "set_read_callback", 1);	/* Set this second */
  pop_stack();					/* Ordering is significant */
}

static void remove_callbacks( struct source *src )
{
  struct pf_source *s = (struct pf_source *)src;
  if (s->obj && s->obj->prog) {
    push_int(0);
    apply(s->obj, "set_close_callback", 1);
    pop_stack();
    push_int(0);
    apply(s->obj, "set_read_callback", 1);
    pop_stack();
  }
}

static void frees(struct pf_source*s) {
  if (s->str) {
    free_string(s->str);
    s->str = 0;
  }
}

static struct data get_data( struct source *src, off_t PIKE_UNUSED(len) )
{
  struct pf_source *s = (struct pf_source *)src;
  struct data res;

  res.data = NULL;
  res.len = s->available;
  if (s->eof)
    s->s.eof = 1;
  if (s->available) {
    res.data = s->data;
    s->available = 0;
  } else if (!s->obj->prog) {	/* Object imploded before we were done */
    s->s.eof = 1;	       /* FIXME should we throw an error here? */
  } else {
  /* No data available, but there should be in the future (no EOF, nor
   * out of the range of data to send as specified by the arguments to
   * source_stream_make)
   */
    res.len = -2;
    setup_callbacks(src);
  }
  return res;
}

static void f_got_data( INT32 args )
{
  struct pf_source *s = (struct pf_source*)Pike_fp->current_object->storage;

  if (!s->obj)	   /* Already finished */
    return;	  /* FIXME Should we throw an error here? */

  remove_callbacks( (struct source *)s );

  if (args < 2
   || TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) {	/* Called as close_cb? */
    s->eof = 1;		    /* signal EOF */
    pop_n_elems(args);
    if (!s->available)
      goto cb;
  } else {					/* Called as read_cb */
    size_t slen;
    frees(s);
    s->str = Pike_sp[-1].u.string;
    slen = s->str->len;
    if (!slen)
      Pike_error("Shuffler: Read callback passing a zero size string\n");
    if (s->str->size_shift)
      Pike_error("Shuffler: Wide strings are not supported\n");
    if (slen > s->skip) {
      s->available = slen -= s->skip;
      s->data = s->str->str + s->skip;
      s->skip = 0;
      Pike_sp--;
      args--;
    } else {
      s->skip -= slen;
      s->str = 0;
    }
    pop_n_elems(args);
cb: if (s->when_data_cb)
      s->when_data_cb(s->when_data_cb_arg);
  }

  push_int(0);
}

static void
 set_callback(struct source *src, void (*cb)( void *a ), struct object *a) {
  struct pf_source *s = (struct pf_source *)src;
  s->when_data_cb = cb;
  if (!s->when_data_cb_arg)
    add_ref(s->when_data_cb_arg = a);
}

static void free_source( struct source *src ) {
  struct pf_source *s = (struct pf_source *)src;
  remove_callbacks(src);
  frees(s);
  if (s->when_data_cb_arg)
    free_object(s->when_data_cb_arg);
  if (s->obj) {
    free_object(s->obj);
    s->obj = 0;
  }
  if (s->self)
    free_object(s->self);
}

struct source *source_pikestream_make(struct svalue *sv,
				      INT64 start, INT64 len) {
  struct pf_source *s;

  if (TYPEOF(*sv) != PIKE_T_OBJECT
   || find_identifier("set_read_callback", sv->u.object->prog) < 0)
    return 0;

  {
    struct object *p;
    if (!(p = clone_object(source_program, 0)))
      return 0;

    s = (struct pf_source*)p->storage;
    s->self = p;
  }

  add_ref(s->obj = sv->u.object);

  s->len = len;
  s->skip = start;
  s->s.get_data = get_data;
  s->s.free_source = free_source;
  s->s.set_callback = set_callback;
  s->s.setup_callbacks = setup_callbacks;
  s->s.remove_callbacks = remove_callbacks;

  return (struct source *)s;
}

void source_pikestream_exit() {
  free_program(source_program);
}

void source_pikestream_init() {
  start_new_program();
  ADD_STORAGE(struct pf_source);
  ADD_FUNCTION("`()", f_got_data, tFunc(tInt tStr,tVoid),0);
  source_program = end_program();
}
