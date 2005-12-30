/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: buffer.h,v 1.5 2005/12/30 22:20:30 nilsson Exp $
*/

#include "stralloc.h"

struct buffer
{
  unsigned int allocated_size;
  unsigned int size, rpos;
  p_wchar2 *data;
};

void uc_buffer_write( struct buffer *d, p_wchar2 data );
INT32 uc_buffer_read( struct buffer *d );
struct buffer *uc_buffer_new( );
struct buffer *uc_buffer_write_pikestring( struct buffer *d,
					   struct pike_string *s );
void uc_buffer_free( struct buffer *d);
struct pike_string *uc_buffer_to_pikestring( struct buffer *d );
void uc_buffer_insert( struct buffer *b, unsigned int pos, p_wchar2 c );
struct buffer *uc_buffer_from_pikestring( struct pike_string *s );
struct buffer *uc_buffer_new_size( int s );
