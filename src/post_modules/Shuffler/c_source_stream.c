#include "global.h"
#include "bignum.h"
#include "object.h"
#include "interpret.h"

#include "fdlib.h"
#include "fd_control.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "shuffler.h"

#define CHUNK 8192
/* $Id: c_source_stream.c,v 1.2 2002/05/30 01:53:46 per Exp $ */


/* Source: Stream
 * Argument: Stdio.File instance pointing to a stream
 *           of some kind (network socket, named pipes, stdin etc)
 */


static struct program *Fd_ref_program;

struct fd_source
{
  struct source s;

  struct object *obj;
  char _read_buffer[CHUNK], _buffer[CHUNK];
  int available;
  int fd;

  void (*when_data_cb)( void *a );
  void *when_data_cb_arg;
  size_t len, skip;
};


static void read_callback( int fd, struct fd_source *s );
static void setup_callbacks( struct source *_s )
{
  struct fd_source *s = (struct fd_source *)_s;
  if( !s->available )
    set_read_callback( s->fd, (void*)read_callback, s );
}

static void remove_callbacks( struct source *_s )
{
  struct fd_source *s = (struct fd_source *)_s;
  set_read_callback( s->fd, 0, 0 );
}


static struct data get_data( struct source *_s, int len )
{
  struct fd_source *s = (struct fd_source *)_s;
  struct data res;
  res.off = res.do_free = 0;
  res.len = s->available;

  if( s->available ) /* There is data in the buffer. Return it. */
  {
    res.data = s->_buffer;
    MEMCPY( res.data, s->_read_buffer, res.len );
    s->available = 0;
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
  free_object(((struct fd_source *)_s)->obj);
}

static void read_callback( int fd, struct fd_source *s )
{
  int l;
  remove_callbacks( (struct source *)s );

  if( s->s.eof )
  {
    if( s->when_data_cb )
      s->when_data_cb( s->when_data_cb_arg );
    return;
  }

  l = fd_read( s->fd, s->_read_buffer, CHUNK );

  if( l <= 0 )
  {
    s->s.eof = 1;
    s->available = 0;
  }
  else if( s->skip )
  {
    if( s->skip >= l )
    {
      s->skip -= l;
      return;
    }
    MEMCPY( s->_read_buffer, s->_read_buffer+s->skip, l-s->skip );
    l-=s->skip;
    s->skip = 0;
  }
  if( s->len > 0 )
  {
    if( s->len < l )
      l = s->len;
    s->len -= l;
    if( !s->len )
      s->s.eof = 1;
  }
  s->available = l;
  if( s->when_data_cb )
    s->when_data_cb( s->when_data_cb_arg );
}

static void set_callback( struct source *_s, void (*cb)( void *a ), void *a )
{
  struct fd_source *s = (struct fd_source *)_s;
  s->when_data_cb = cb;
  s->when_data_cb_arg = a;;
}

struct source *source_stream_make( struct svalue *s,
				   INT64 start, INT64 len )
{
  struct fd_source *res;
/*    struct stat st; */
  if( (s->type != PIKE_T_OBJECT) ||
      !get_storage( s->u.object, Fd_ref_program ) )
    return 0;

  res = malloc( sizeof( struct fd_source ) );
  MEMSET( res, 0, sizeof( struct fd_source ) );

  apply( s->u.object, "query_fd", 0 );
  res->fd = Pike_sp[-1].u.integer;
  pop_stack();

/*    if( fd_fstat( res->fd, &st ) < 0 )   goto fail; */
/*    if( S_ISREG(st.st_mode) )            goto fail; */
  
  if( len != -1 )
    res->len = len;
  else
    res->len = -1;

  if( start )
    res->skip = start;

  res->s.get_data = get_data;
  res->s.free_source = free_source;
  res->s.set_callback = set_callback;
  res->s.setup_callbacks = setup_callbacks;
  res->s.remove_callbacks = remove_callbacks;
  res->obj = s->u.object;
  res->obj->refs++;
  return (struct source *)res;

fail:
  free(res);
  return 0;
}

void source_stream_exit( )
{
  free_program( Fd_ref_program );
}

void source_stream_init( )
{
  push_text("files.Fd_ref"); push_int(0);
  SAFE_APPLY_MASTER("resolv",2);
  Fd_ref_program = program_from_svalue(Pike_sp-1);
  Fd_ref_program->refs++;
  pop_stack( );
}
