/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "bignum.h"
#include "object.h"
#include "interpret.h"

#include "fdlib.h"
#include "fd_control.h"

#include <sys/types.h>
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

  void (*when_data_cb)( void *a );
  void *when_data_cb_arg;
  size_t len, skip;
};


struct callback_prog
{
  struct pf_source *s;
};

static void setup_callbacks( struct source *_s )
{
  struct pf_source *s = (struct pf_source *)_s;
  if( !s->str )
  {
    ref_push_object( s->cb_obj );
    apply( s->obj, "set_read_callback", 1 );
    pop_stack();
    ref_push_object( s->cb_obj );
    apply( s->obj, "set_close_callback", 1 );
    pop_stack();
  }
}

static void remove_callbacks( struct source *_s )
{
  struct pf_source *s = (struct pf_source *)_s;
  push_int(0);
  apply( s->obj, "set_read_callback", 1 );
  pop_stack();
  push_int(0);
  apply( s->obj, "set_close_callback", 1 );
  pop_stack();
}


static struct data get_data( struct source *_s, int len )
{
  struct pf_source *s = (struct pf_source *)_s;
  struct data res;
  char *buffer = NULL;

  if( s->str )
  {
    len = s->str->len;
    if( s->skip )
    {
      if( s->skip >= (size_t)s->str->len )
      {
	s->skip -= (size_t)s->str->len;
	res.do_free = 0;
	res.data = 0;
	res.len = -2;
	res.off = 0;
	return res;
      }
      len -= s->skip;
      buffer = malloc( len );
      MEMCPY( buffer, s->str->str+s->skip, len);
    }
    else
    {
      if( s->len > 0 )
      {
	if( s->len < (size_t)len )
	  len = s->len;
	s->len -= len;
	if( !s->len )
	  s->s.eof = 1;
      }
      buffer = malloc( len );
      MEMCPY( buffer, s->str->str, len );
    }
    res.data = buffer;
    res.len = len;
    res.do_free = 1;
    res.off = 0;
    free_string( s->str );
    s->str = 0;
    setup_callbacks( _s );
  }
  else if( !s->len )
    s->s.eof = 1;
  else
  /* No data available, but there should be in the future (no EOF, nor
   * out of the range of data to send as specified by the arguments to
   * source_stream_make)
   */
    res.len = -2;

  return res;
}

static void free_source( struct source *_s )
{
  remove_callbacks( _s );
  free_object(((struct pf_source *)_s)->cb_obj);
  free_object(((struct pf_source *)_s)->obj);
}

static void f_got_data( INT32 args )
{
  struct pf_source *s =
    ((struct callback_prog *)Pike_fp->current_object->storage)->s;

  remove_callbacks( (struct source *)s );

  if( s->str ||
      Pike_sp[-1].type != PIKE_T_STRING ||
      Pike_sp[-1].u.string->size_shift ||
      Pike_sp[-1].u.string->len == 0)
  {
    s->s.eof = 1;
    pop_n_elems(args);
    push_int(0);
    return;
  }
  s->str = Pike_sp[-1].u.string;
  Pike_sp--;
  pop_n_elems(args-1);
  push_int(0);

  if( s->when_data_cb )
    s->when_data_cb( s->when_data_cb_arg );
}

static void set_callback( struct source *_s, void (*cb)( void *a ), void *a )
{
  struct pf_source *s = (struct pf_source *)_s;
  s->when_data_cb = cb;
  s->when_data_cb_arg = a;
}

struct source *source_pikestream_make( struct svalue *s,
				       INT64 start, INT64 len )
{
  struct pf_source *res;

  if( (s->type != PIKE_T_OBJECT) ||
      (find_identifier("set_read_callback",s->u.object->prog)==-1) )
    return 0;
  
  res = malloc( sizeof( struct pf_source ) );
  MEMSET( res, 0, sizeof( struct pf_source ) );

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
  add_function("`()", f_got_data, "function(int,string:void)",0);
  callback_program = end_program();
}
