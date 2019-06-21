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
static struct program *callback_program;

struct pf_source
{
  struct source s;

  struct object *obj;
  struct object *cb_obj;
  struct pike_string *str;
  char *data;
  size_t available;
  int eof;

  void (*when_data_cb)( void *a );
  void *when_data_cb_arg;
  size_t len, skip;
};


struct callback_prog
{
  struct pf_source *s;
};

static void setup_callbacks( struct source *src )
{
  struct pf_source *s = (struct pf_source *)src;
  ref_push_object( s->cb_obj );
  apply( s->obj, "set_read_callback", 1 );
  pop_stack();
  ref_push_object( s->cb_obj );
  apply( s->obj, "set_close_callback", 1 );
  pop_stack();
}

static void remove_callbacks( struct source *src )
{
  struct pf_source *s = (struct pf_source *)src;
  if (s->obj->prog) {
    push_int(0);
    apply( s->obj, "set_read_callback", 1 );
    pop_stack();
    push_int(0);
    apply( s->obj, "set_close_callback", 1 );
    pop_stack();
  }
}

static void frees(struct pf_source*s) {
  if (s->str) {
    free_string(s->str);
    s->str = 0;
  }
}

static struct data get_data( struct source *src, off_t len )
{
  struct pf_source *s = (struct pf_source *)src;
  struct data res;

  res.len = s->available;
  if (s->eof)
    s->s.eof = 1;
  if (s->available) {
    res.data = s->data;
    s->available = 0;
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

static void free_source( struct source *src )
{
  struct pf_source *s = (struct pf_source *)src;
  remove_callbacks( src );
  frees(s);
  free_object(s->cb_obj);
  free_object(s->obj);
}

static void f_got_data( INT32 args )
{
  struct pf_source *s =
    ((struct callback_prog *)Pike_fp->current_object->storage)->s;

  remove_callbacks( (struct source *)s );

  if (args < 2
   || TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) {
    s->eof = 1;		    /* signal EOF */
    pop_n_elems(args);
    if (!s->available)
      goto cb;
  } else {
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
cb: if( s->when_data_cb )
      s->when_data_cb( s->when_data_cb_arg );
  }

  push_int(0);
}

static void set_callback( struct source *src, void (*cb)( void *a ), void *a )
{
  struct pf_source *s = (struct pf_source *)src;
  s->when_data_cb = cb;
  s->when_data_cb_arg = a;
}

struct source *source_pikestream_make( struct svalue *s,
				       INT64 start, INT64 len )
{
  struct pf_source *res;

  if( (TYPEOF(*s) != PIKE_T_OBJECT) ||
      (find_identifier("set_read_callback",s->u.object->prog)==-1) )
    return 0;

  if (!(res = calloc( 1, sizeof( struct pf_source))))
    return 0;

  res->len = len;
  res->skip = start;

  res->s.get_data = get_data;
  res->s.free_source = free_source;
  res->s.set_callback = set_callback;
  res->s.setup_callbacks = setup_callbacks;
  res->s.remove_callbacks = remove_callbacks;
  res->obj = s->u.object;
  add_ref(res->obj);

  res->cb_obj = clone_object( callback_program, 0 );
  ((struct callback_prog *)res->cb_obj->storage)->s = res;

  return (struct source *)res;
}

void source_pikestream_exit( )
{
  free_program( callback_program );
}

void source_pikestream_init( )
{
  start_new_program();
  ADD_STORAGE( struct callback_prog );
  ADD_FUNCTION("`()", f_got_data, tFunc(tInt tStr,tVoid),0);
  callback_program = end_program();
}
