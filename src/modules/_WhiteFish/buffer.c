#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: buffer.c,v 1.4 2001/05/25 18:39:32 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"
#include "buffer.h"

/* must be included last */
#include "module_magic.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

static INLINE void wf_buffer_make_space( struct buffer *b, int n )
{
#ifdef PIKE_DEBUG
  if( b->read_only )  fatal("Oops\n");
#endif
  while( b->allocated_size-n < b->size )
  {
    b->allocated_size++;
    b->allocated_size *= 2;
    b->data = realloc( b->data, b->allocated_size );
  }
}

void wf_buffer_wbyte( struct buffer *b,
		      unsigned char s )
{
  wf_buffer_make_space( b, 1 );
  b->data[b->size++] = s;
}

void wf_buffer_wshort( struct buffer *b,
		       unsigned short s )
{
  wf_buffer_make_space( b, 2 );
  b->data[b->size++] = (s>>8);
  b->data[b->size++] = (s)&255;
}

void wf_buffer_wint( struct buffer *b,
		      unsigned int s )
{
  s = htonl(s);
  wf_buffer_make_space( b, 4 );
  MEMCPY( b->data+b->size, &s, 4 );
  b->size += 4;
}

void wf_buffer_rewind_r( struct buffer *b, int n )
{
  if( n == -1 )
    b->rpos = 0;
  else if( b->rpos > n )
    b->rpos -= n;
  else
    b->rpos = 0;
}

void wf_buffer_rewind_w( struct buffer *b, int n )
{
  if( n == -1 )
     b->size = 0;
  else if( b->size > n )
    b->size -= n;
  else
    b->size = 0;
}

int wf_buffer_rbyte( struct buffer *b )
{
  if( b->rpos < b->size )
    return ((unsigned char *)b->data)[ b->rpos++ ];
  return 0;
}

int wf_buffer_rint( struct buffer *b )
{
  return (((((wf_buffer_rbyte( b ) << 8) |
	     wf_buffer_rbyte( b ))<<8)   |
	   wf_buffer_rbyte( b )) << 8)   |
         wf_buffer_rbyte( b );
}

int wf_buffer_memcpy( struct buffer *d,
		      struct buffer *s,
		      int nelems )
{
  if( (s->rpos+nelems) > s->size )
    nelems = s->size-s->rpos;
  if( nelems <= 0 )
    return 0;
  wf_buffer_make_space( d, nelems );
  MEMCPY( d->data+d->size, s->data+s->rpos, nelems );
  s->rpos += nelems;
  d->size += nelems;
  return nelems;
}

int wf_buffer_rshort( struct buffer *b )
{
  return (wf_buffer_rbyte( b ) << 8) | wf_buffer_rbyte( b );
}

int wf_buffer_eof( struct buffer *b )
{
  return b->rpos >= b->size;
}


void wf_buffer_seek( struct buffer *b, int pos )
{
  b->rpos = pos;
}
  
void wf_buffer_clear( struct buffer *b )
{
  if( !b->read_only && b->data )  free( b->data );
  if( b->read_only && b->str )    free_string( b->str );
  MEMSET(b, 0, sizeof(struct buffer));
}

void wf_buffer_free( struct buffer *b )
{
  wf_buffer_clear( b );
  free( b );
}


void wf_buffer_set_empty( struct buffer *b )
{
  wf_buffer_clear( b );
  b->data = malloc( 32 );
  b->allocated_size = 32;
}

void wf_buffer_set_pike_string( struct buffer *b,
				struct pike_string *data,
				int read_only )
{
  wf_buffer_clear( b );
  if( read_only )
  {
    b->read_only = 1;
    b->str = data;
    data->refs++;
    b->size = data->len;
    b->data = data->str;
  }
  else
  {
    b->size = data->len;
    b->data = malloc( b->size );
    b->allocated_size = b->size;
    MEMCPY( b->data, data->str, b->size );
  }
}

struct buffer *wf_buffer_new( )
{
  struct buffer *b = malloc( sizeof( struct buffer ) );
  MEMSET( b, 0, sizeof(struct buffer) );
  return b;
}

void wf_buffer_append( struct buffer *b,
		       char *data,
		       int size )
{
  wf_buffer_make_space( b, size );
  MEMCPY( b->data+b->size, data, size );
  b->size+=size;
}
