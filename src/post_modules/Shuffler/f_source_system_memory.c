/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "bignum.h"
#include "object.h"
#include "interpret.h"

#include <shuffler.h>


/* Source: System.Memory
 * Argument: An initialized instance of the System.Memory class
 */
static struct program *shm_program = NULL;

struct sm_source
{
  struct source s;

  struct object *obj;

  char *data;
  size_t len;
};

static struct data get_data( struct source *src, off_t len )
{
  struct sm_source *s = (struct sm_source *)src;
  struct data res;

  if (len > s->len) {
    len = s->len;
    s->s.eof = 1; /* next read will be done from the next source */
  }

  s->len -= res.len = len;
  res.data = s->data;
  s->data += len;

  return res;
}

static void free_source( struct source *src )
{
  free_object(((struct sm_source *)src)->obj);
}

struct source *source_system_memory_make( struct svalue *s,
					  INT64 start, INT64 len )
{
  struct sm_source *res;
  struct {
    char *data;
    size_t len;
  } *mem;
  INT64 slen;

  if (TYPEOF(*s) != PIKE_T_OBJECT)
    return 0;

  if (!shm_program) {
    push_static_text("System.Memory");
    SAFE_APPLY_MASTER("resolv", 1);
    shm_program = program_from_svalue(Pike_sp - 1);
    if (!shm_program) {
      pop_stack();
      return 0;
    }
    add_ref(shm_program);
    pop_stack();
  }

  if (!(mem = get_storage(s->u.object, shm_program)))
    return 0;

  if (!(res = calloc(1, sizeof(struct sm_source))))
    return 0;

  res->s.free_source = free_source;
  res->s.get_data = get_data;
  res->obj = s->u.object;
  add_ref(res->obj);

  res->data = mem->data + start;
  slen = mem->len;

  slen -= start;
  if (slen < 0)
    slen = 0;
  if (len < 0)
    len = slen;
  if (len > slen)
    len = slen;

  res->len = len;
  return (struct source *)res;
}

void source_system_memory_exit( )
{
  if (shm_program) {
    free_program( shm_program );
  }
}

void source_system_memory_init( )
{
}
