#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: blob.c,v 1.2 2001/05/22 13:52:27 per Exp $");
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

/* must be included last */
#include "module_magic.h"

/*
  +-----------+----------+---------+---------+---------+
  | docid: 32 | nhits: 8 | hit: 16 | hit: 16 | hit: 16 |...
  +-----------+----------+---------+---------+---------+
*/

int wf_blob_next( Blob *b )
{
  /* Find the next document ID */
  if( b->eof )
    return -1;
  b->docid = -1;
  if( b->pos >= b->len )
  {
    if( b->str )
      free_string( b->str );
    b->str = 0;
    push_int( b->word );
    apply_svalue( b->feed, 1 );
    if( sp[-1].type != T_STRING )
    {
      b->eof = 1;
      return -1;
    }
    b->blob = sp[-1].u.string->str;
    b->len = sp[-1].u.string->len;
    b->str = sp[-1].u.string;
    b->str->refs++;
    b->pos = 0;
    
  }
  else
  {
    /* FF past current docid */
    b->pos += 4 + 1 + 2*wf_blob_nhits( b);
    if( b->pos >= b->len )
    {
      if( b->str )
	free_string( b->str );
      b->str = 0;
      push_int( b->word );
      apply_svalue( b->feed, 1 );
      if( sp[-1].type != T_STRING )
      {
	b->eof = 1;
	return -1;
      }
      b->blob = sp[-1].u.string->str;
      b->len = sp[-1].u.string->len;
      b->str = sp[-1].u.string;
      b->str->refs++;
      b->pos = 0;
    }
  }
  return wf_blob_docid( b );
}

int wf_blob_eof( Blob *b )
{
  if( b->eof )
    return 1;
  return 0;
}

int wf_blob_nhits( Blob *b )
{
  if( b->eof ) return 0;
  return b->blob[b->pos+4];
}

Hit wf_blob_hit( Blob *b, int n )
{
  int off = b->pos + 5 + n*2;
  unsigned char h =  b->blob[ off ];
  unsigned char l = b->blob[ off + 1 ];
  Hit hit;
  hit.raw = (h<<8) | l;
  return hit;
}

int wf_blob_docid( Blob *b )
{
  if( b->eof ) return -1;
  if( b->docid >= 0 )
    return b->docid;
  else
  {
    int off = b->pos;
    unsigned char hh =  b->blob[ off ];
    unsigned char hl = b->blob[ off+1 ];
    unsigned char lh =  b->blob[ off+2 ];
    unsigned char ll = b->blob[ off+3 ];
    return b->docid = ((((((hh<<8) | hl)<<8) | lh)<<8) | ll);
  }
}


Blob *wf_blob_new( struct svalue *feed, int word )
{
  Blob *b = malloc( sizeof( Blob ) );
  MEMSET(b, 0, sizeof(b) );
  b->word = word;
  b->feed = feed;
  return b;
}


void wf_blob_free( Blob *b )
{
  if( b->str )  free_string( b->str );
  free( b );
}
