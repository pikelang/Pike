/* $Id: buffer.c,v 1.12 2004/07/20 16:37:11 grubba Exp $
 */
#include "global.h"

#include "config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "stralloc.h"
RCSID("$Id: buffer.c,v 1.12 2004/07/20 16:37:11 grubba Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"
#include "buffer.h"

static INLINE int range( int n, int m )
{
  if( n < (m>>3) )    return (m>>3);
  else if( n < 32 )   return 32;
  else if( n < 64 )   return 64;
  else if( n < 128 )  return 128;
  else if( n < 256 )  return 256;
  else if( n < 512 )  return 512;
  else if( n < 1024 ) return 1024;
  else if( n < 2048 ) return 2048;
  else return n;
}

static INLINE void wf_buffer_make_space( struct buffer *b, unsigned int n )
{
#ifdef PIKE_DEBUG
  if( b->read_only )
    fatal("Oops\n");
#endif
  if( b->allocated_size-b->size < n )
  {
    b->allocated_size += range(n,b->allocated_size);
    b->data =realloc(b->data,b->allocated_size);
  }
}

void wf_buffer_wbyte( struct buffer *b,
		      unsigned char s )
{
  if( b->allocated_size == b->size )
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
  else if( b->rpos > (unsigned int)n )
    b->rpos -= n;
  else
    b->rpos = 0;
}

void wf_buffer_rewind_w( struct buffer *b, int n )
{
  if( n == -1 )
     b->size = 0;
  else if( b->size > (unsigned int)n )
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

unsigned int wf_buffer_rint( struct buffer *b )
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


void wf_buffer_seek( struct buffer *b, unsigned int pos )
{
  b->rpos = pos;
}

void wf_buffer_seek_w( struct buffer *b, unsigned int pos )
{
#ifdef PIKE_DEBUG
  if( b->read_only )
    fatal( "Oops, read_only\n");
#endif
  if( pos > b->size )
  {
    wf_buffer_make_space( b, (unsigned int)(pos-b->size) );
    MEMSET( b->data+b->size, 0, (unsigned int)(pos-b->size) );
  }
  b->size = pos;
}
  
void wf_buffer_clear( struct buffer *b )
{
  if( !b->read_only && b->data )
    free( b->data );
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
  b->data = xalloc( 16 );
  b->allocated_size = 16;
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
    add_ref(data);
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
  struct buffer *b = xalloc( sizeof( struct buffer ) );
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
