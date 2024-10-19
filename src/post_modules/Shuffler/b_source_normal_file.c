/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "bignum.h"
#include "object.h"
#include "interpret.h"
#include "threads.h"

#include "fdlib.h"
#include "fd_control.h"

#include <sys/stat.h>

#include "shuffler.h"

#define CHUNK (64*1024)

/* Source: Normal file
 * Argument: Stdio.File instance pointing to a normal file
 */

struct fd_source
{
  struct source s;
  struct object *obj;
  char buffer[CHUNK];
  int fd;
  off_t len;
};

static struct data get_data( struct source *src, off_t len )
{
  struct fd_source *s = (struct fd_source *)src;
  struct data res;
  int rr;
  len = CHUNK; /* It's safe to ignore the 'len' argument */

  if (s->len > 0 && len > s->len)
    len = s->len;

  THREADS_ALLOW();
  while(0 > (rr = fd_read(s->fd, s->buffer, len)) && errno == EINTR);
  THREADS_DISALLOW();

  res.len = rr;
  if (rr <= 0 || (s->len > 0 && !(s->len -= rr)))
    s->s.eof = 1;
  res.data = s->buffer;
  return res;
}


static void free_source( struct source *src )
{
  struct fd_source *s = (struct fd_source *)src;
  if (s->obj)
    free_object(s->obj);
  debug_malloc_touch(s);
  free(s);
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

struct source *source_normal_file_make( struct svalue *s,
					INT64 start, INT64 len )
{
  struct fd_source *res;
  PIKE_STAT_T st;
  int fd;

  if (TYPEOF(*s) != PIKE_T_OBJECT
   || !is_stdio_file(s->u.object)
   || find_identifier("query_fd", s->u.object->prog) < 0)
    return 0;

  apply(s->u.object, "query_fd", 0);
  fd = Pike_sp[-1].u.integer;
  pop_stack();

  if (fd_fstat(fd, &st) < 0 || !S_ISREG(st.st_mode))
    return 0;

  if (start && (fd_lseek(fd, (off_t)start, SEEK_CUR) < 0))
    return NULL;

  if (!(res = calloc(1, sizeof(struct fd_source))))
    return 0;

  res->fd = fd;
  res->s.get_data = get_data;
  res->s.free_source = free_source;
  res->obj = s->u.object;
  add_ref(res->obj);
  res->len = len;
  return (struct source *)res;
}

void source_normal_file_exit( )
{
}

void source_normal_file_init( )
{
}
