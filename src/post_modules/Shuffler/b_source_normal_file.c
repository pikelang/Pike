/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: b_source_normal_file.c,v 1.12 2004/08/25 23:04:24 vida Exp $
*/

#include "global.h"
#include "bignum.h"
#include "object.h"
#include "interpret.h"
#include "threads.h"

#include "fdlib.h"
#include "fd_control.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "shuffler.h"

#define CHUNK 8192


/* Source: Normal file
 * Argument: Stdio.File instance pointing to a normal file
 */

static struct program *Fd_ref_program = NULL;

struct fd_source
{
  struct source s;
  struct object *obj;
  char buffer[CHUNK];
  int fd;
  size_t len;
};

static struct data get_data( struct source *_s, ptrdiff_t len )
{
  struct fd_source *s = (struct fd_source *)_s;
  struct data res;
  int rr;
  len = CHUNK; /* It's safe to ignore the 'len' argument */

  res.do_free = 0;
  res.off = 0;
  res.data = s->buffer;
  
  if( len > (ptrdiff_t)s->len )
  {
    len = (ptrdiff_t)s->len;
    s->s.eof = 1;
  }
  THREADS_ALLOW();
  rr = fd_read( s->fd, res.data, len );
  THREADS_DISALLOW();
/*    printf("B[normal file]: get_data( %d / %d ) --> %d\n", len, */
/*  	 s->len, rr); */

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
  PIKE_STAT_T st;
  if(s->type != PIKE_T_OBJECT)
    return 0;

  if (!Fd_ref_program)
  {
    push_text("files.Fd_ref");
    SAFE_APPLY_MASTER("resolv",1);
    Fd_ref_program = program_from_svalue(Pike_sp-1);
    if (!Fd_ref_program) {
      pop_stack();
      return 0;
    }
    add_ref(Fd_ref_program);
    pop_stack( );
  }

  if (!get_storage( s->u.object, Fd_ref_program ) )
    return 0;

  if (find_identifier("query_fd", s->u.object->prog) < 0)
    return 0;

  res = malloc( sizeof( struct fd_source ) );
  MEMSET( res, 0, sizeof( struct fd_source ) );

  apply( s->u.object, "query_fd", 0 );
  res->fd = Pike_sp[-1].u.integer;
  pop_stack();
  res->s.get_data = get_data;
  res->s.free_source = free_source;
  res->obj = s->u.object;
  add_ref(res->obj);

  if( fd_fstat( res->fd, &st ) < 0 )
  {
    goto fail;
  }
  if( !S_ISREG(st.st_mode) )
  {
    goto fail;
  }  
  if( len > 0 )
  {
    if( len > st.st_size-start )
    {
      goto fail;
    }
    else
      res->len = len;
  }
  else
    res->len = st.st_size-start;

  if( fd_lseek( res->fd, (off_t)start, SEEK_SET ) < 0 )
  {
    goto fail;
  }
  return (struct source *)res;

fail:
  free_source((void *)res);
  free(res);
  return 0;
}

void source_normal_file_exit( )
{
  if (Fd_ref_program) {
    free_program( Fd_ref_program );
  }
}

void source_normal_file_init( )
{
}
