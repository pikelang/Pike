#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: buffer.c,v 1.1 2001/05/23 10:58:00 per Exp $");
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

void wf_buffer_make_space( struct buffer *b, int n )
{
  if( b->read_only )  fatal("Oops\n");
  while( b->allocated_size-n >= b->size )
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
  s = htons(s);
  wf_buffer_make_space( b, 2 );
  MEMCPY( b->data+b->size, &s, 2 );
  b->size += 2;
}

void wf_buffer_wint( struct buffer *b,
		      unsigned int s )
{
  s = htonl(s);
  wf_buffer_make_space( b, 4 );
  MEMCPY( b->data+b->size, &s, 4 );
  b->size += 4;
}


int wf_buffer_rbyte( struct buffer *b )
{
  if( b->rpos < b->size )
    return b->data[ b->rpos++ ];
  return 0;
}

int wf_buffer_rint( struct buffer *b )
{
  int res=0, i;
  for( i=0; i<4; i++ )
  {
    res <<= 8;
    res |= wf_buffer_rbyte( b );
  }
  return res;
}

int wf_buffer_rshort( struct buffer *b )
{
  int res=0, i;
  for( i=0; i<2; i++ )
  {
    res <<= 8;
    res |= wf_buffer_rbyte( b );
  }
  return res;
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
  b->data = malloc( 256 );
  b->allocated_size = 256;
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
  MEMSET(b, 0, sizeof(struct buffer));
  return b;
}

void wf_buffer_append( struct buffer *b,
		       char *data,
		       int size )
{
  wf_buffer_make_space( b, size );
  MEMCPY( b->data+b->size, data, size );
  b->data+=size;
}
