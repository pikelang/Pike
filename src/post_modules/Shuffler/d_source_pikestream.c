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
  int available;

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
  if( !s->available )
  {
    ref_push_object( s->cb_obj );
    apply( s->obj, "set_read_callback", 1 );
    pop_stack();
    ref_push_object( s->cb_obj );
    apply( s->obj, "set_close_callback", 1 );
    pop_stack();
  }
}

static void remove_callbacks( struct source *src )
{
  struct pf_source *s = (struct pf_source *)src;
  push_int(0);
  apply( s->obj, "set_read_callback", 1 );
  pop_stack();
  push_int(0);
  apply( s->obj, "set_close_callback", 1 );
  pop_stack();
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

  if (s->available) {
    if (!s->str) {
      s->s.eof = 1;
      res.len = 0;
      return res;
    }
    len = s->str->len;
    if (s->skip) {
      if (s->skip >= len) {
        frees(s);
	s->skip -= len;
	goto getmore;
      }
      len -= s->skip;
    }
    if (s->len) {
      if (s->len < (size_t)len)
        len = s->len;
      s->len -= len;
      if (!s->len)
        s->s.eof = 1;
    }
    res.data = s->str->str + s->skip;
    res.len = len;
    if (s->available < 0)
      s->s.eof = 1;
    s->available = 0;
  } else {
getmore:
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

  if (args < 2 ||
      TYPEOF(Pike_sp[-1]) != PIKE_T_STRING ||
      Pike_sp[-1].u.string->size_shift ||
      Pike_sp[-1].u.string->len == 0) {

    s->available = -1;

  } else {
    frees(s);
    s->available = 1;
    s->str = Pike_sp[-1].u.string;
    Pike_sp--;
    args--;
  }

  pop_n_elems(args);

  if( s->when_data_cb )
    s->when_data_cb( s->when_data_cb_arg );

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
  res->available = 0;

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
