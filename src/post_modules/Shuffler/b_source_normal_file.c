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

/* $Id: b_source_normal_file.c,v 1.1 2002/05/29 07:40:11 per Exp $ */


/* Source: Normal file
 * Argument: Stdio.File instance pointing to a normal file
 */


static struct program *Fd_ref_program;

struct fd_source
{
  struct source s;

  struct object *obj;
  char buffer[CHUNK];
  int fd;
  size_t len;
};

static struct data get_data( struct source *_s, int len )
{
  struct fd_source *s = (struct fd_source *)_s;
  struct data res;
  int rr;

  len = CHUNK; /* It's safe to ignore the 'len' argument */

  res.do_free = 0;
  res.off = 0;
  res.data = s->buffer;
  
  if( len > s->len )
  {
    len = s->len;
    s->s.eof = 1;
  }

  rr = fd_read( s->fd, res.data, len );
  res.len = rr;
  if( rr < len )
    s->s.eof = 1;
  return res;
}


static void free_source( struct source *_s )
{
  free_object(((struct fd_source *)_s)->obj);
}

struct source *source_normal_file_make( struct svalue *s,
					INT64 start, INT64 len )
{
  struct fd_source *res;
  struct stat st;
  if( (s->type != PIKE_T_OBJECT) ||
      !get_storage( s->u.object, Fd_ref_program ) )
    return 0;

  res = malloc( sizeof( struct fd_source ) );
  MEMSET( res, 0, sizeof( struct fd_source ) );

  apply( s->u.object, "query_fd", 0 );
  res->fd = Pike_sp[-1].u.integer;
  pop_stack();

  if( fd_fstat( res->fd, &st ) < 0 )
    goto fail;

  if( !S_ISREG(st.st_mode) )
    goto fail;
  
  if( len != -1 )
  {
    if( len > st.st_size-start )
      goto fail;
    else
      res->len = len;
  }
  else
    res->len = st.st_size-start;

  if( fd_lseek( res->fd, (off_t)start, SEEK_SET ) < 0 )
    goto fail;

  res->s.get_data = get_data;
  res->s.free = free_source;
  res->obj = s->u.object;
  res->obj->refs++;
  return (struct source *)res;

fail:
  free(res);
  return 0;
  
}

void source_normal_file_exit( )
{
  free_program( Fd_ref_program );
}

void source_normal_file_init( )
{
  push_text("files.Fd_ref"); push_int(0);
  SAFE_APPLY_MASTER("resolv",2);
  Fd_ref_program = program_from_svalue(Pike_sp-1);
  Fd_ref_program->refs++;
  pop_stack( );
}
