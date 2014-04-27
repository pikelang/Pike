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

#include <sys/types.h>
#include <sys/stat.h>

#include "shuffler.h"

/* Source: blocking pike-stream
 * Argument: Stdio.File lookalike without read
 *           callback support (read only needed)
 */

struct pf_source
{
  struct source s;

  struct object *obj;
  INT64 len, skip;
};


static struct data get_data( struct source *src, off_t len )
{
  struct pf_source *s = (struct pf_source *)src;
  struct data res = { 0, 0, 0, NULL };

  if( s->len>0 && len > s->len ) {
    len = s->len;
    s->s.eof = 1;
  }
    
  do {
    struct pike_string *st;

    push_int( len );
    apply( s->obj, "read", 1 );

    if(TYPEOF(Pike_sp[-1]) != PIKE_T_STRING
     || !(st = Pike_sp[-1].u.string)->len) {
      pop_stack();
      break;
    }

    if( st->len < s->skip )
      s->skip -= st->len;
    else {
      res.len = st->len - s->skip;
      res.data = xalloc(res.len);
      memcpy(res.data, st->str+s->skip, res.len);
      res.do_free = 1;
      s->skip = 0;
    }
    pop_stack();
  }
  while(s->skip || !res.len);
  if(res.len < len)
    s->s.eof = 1;
  if(s->len > 0)
    s->len -= res.len;
  return res;
}

static void free_source( struct source *src )
{
  free_object(((struct pf_source *)src)->obj);
}

struct source *source_block_pikestream_make( struct svalue *s,
					     INT64 start, INT64 len )
{
  struct pf_source *res;

  if( (TYPEOF(*s) != PIKE_T_OBJECT) ||
      (find_identifier("read",s->u.object->prog)==-1) )
    return 0;
  
  res = calloc( 1, sizeof( struct pf_source ) );
  if( !res ) return NULL;

  res->len = len;
  res->skip = start;

  res->s.get_data = get_data;
  res->s.free_source = free_source;
  res->obj = s->u.object;
  add_ref(res->obj);
  return (struct source *)res;
}

void source_block_pikestream_exit( )
{
}

void source_block_pikestream_init( )
{
}
