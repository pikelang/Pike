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

#include <shuffler.h>


/* Source: System.Memory
 * Argument: An initialized instance of the System.Memory class
 */
static struct program *shm_program = NULL;

struct sm_source
{
  struct source s;

  struct object *obj;
  struct {
    unsigned char *data;
    size_t len;
  } *mem;

  int offset, len;
};

static struct data get_data( struct source *_s, int len )
{
  struct sm_source *s = (struct sm_source *)_s;
  struct data res;
  
  res.do_free = 0;
  res.off = 0;
  res.data = s->mem->data + s->offset;
  
  if( len > s->len )
  {
    len = s->len;
    s->s.eof = 1; /* next read will be done from the next source */
  }

  res.len = len;

  s->len -= len;
  s->offset += len;

  return res;
}

static void free_source( struct source *_s )
{
  free_object(((struct sm_source *)_s)->obj);
}

struct source *source_system_memory_make( struct svalue *s,
					  INT64 start, INT64 len )
{
  struct sm_source *res;

  if( s->type != PIKE_T_OBJECT )
    return 0;

  if (!shm_program) {
    push_text("System.Memory");
    push_int(0);
    SAFE_APPLY_MASTER("resolv", 2);
    shm_program = program_from_svalue(Pike_sp - 1);
    if (!shm_program) {
      pop_stack();
      return 0;
    }
    add_ref(shm_program);
    pop_stack();
  }

  res = malloc( sizeof( struct sm_source ) );
  MEMSET( res, 0, sizeof( struct sm_source ) );

  if( !(res->mem = (void*)get_storage( s->u.object, shm_program ) ) )
  {
    free(res);
    return 0;
  }

  if( !res->mem->data || !res->mem->len )
  {
    free(res);
    return 0;
  }
  
  res->s.free_source = free_source;
  res->s.get_data = get_data;
  res->obj = s->u.object;
  res->obj->refs++;
  res->offset = start;

  if( len != -1 )
    if( len > res->mem->len-start )
    {
      res->obj->refs--;
      free(res);
      return 0;
    }
    else
      res->len = len;
  else
    res->len = len;

  if( res->len <= 0 )
  {
    res->obj->refs--;
    free(res);
    return 0;
  }
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
