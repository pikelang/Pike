/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: e_source_block_pikestream.c,v 1.4 2004/08/25 23:04:24 vida Exp $
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


static struct data get_data( struct source *_s, int len )
{
  struct pf_source *s = (struct pf_source *)_s;
  struct data res;

  res.len  = 0;
  res.off  = 0;
  res.do_free = 0;

  if( s->len>0 && len > s->len ) {
    len = s->len;
    s->s.eof = 1;
  }
    
  do {
    struct pike_string *st;

    push_int( len );
    apply( s->obj, "read", 1 );

    if(Pike_sp[-1].type != PIKE_T_STRING
     || !(st = Pike_sp[-1].u.string)->len) {
      pop_stack();
      break;
    }

    if( st->len < s->skip )
      s->skip -= st->len;
    else {
      res.data = malloc(st->len -= s->skip);
      memcpy(res.data, st->str+s->skip, res.len = st->len);
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

static void free_source( struct source *_s )
{
  free_object(((struct pf_source *)_s)->obj);
}

struct source *source_block_pikestream_make( struct svalue *s,
					     INT64 start, INT64 len )
{
  struct pf_source *res;

  if( (s->type != PIKE_T_OBJECT) ||
      (find_identifier("read",s->u.object->prog)==-1) )
    return 0;
  
  res = malloc( sizeof( struct pf_source ) );
  MEMSET( res, 0, sizeof( struct pf_source ) );

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
