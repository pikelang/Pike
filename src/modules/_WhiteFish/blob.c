#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: blob.c,v 1.4 2001/05/23 10:58:00 per Exp $");
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
  if( b->b->rpos >= b->b->size )
  {
    if( !b->feed )
    {
      wf_buffer_clear( b->b );
      b->eof = 1;
      return -1;
    }
    push_int( b->word );
    apply_svalue( b->feed, 1 );
    if( sp[-1].type != T_STRING )
    {
      b->eof = 1;
      return -1;
    }
    wf_buffer_set_pike_string( b->b, sp[-1].u.string, 1 );
  }
  else
  {
    /* FF past current docid */
    b->b->rpos += 4 + 1 + 2*wf_blob_nhits( b );
    if( b->b->rpos >= b->b->size )
    {
      if( !b->feed )
      {
	wf_buffer_clear( b->b );
	b->eof = 1;
	return -1;
      }
      push_int( b->word );
      apply_svalue( b->feed, 1 );
      if( sp[-1].type != T_STRING )
      {
	b->eof = 1;
	return -1;
      }
      wf_buffer_set_pike_string( b->b, sp[-1].u.string, 1 );
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
  return b->b->data[b->b->rpos+4];
}

Hit wf_blob_hit( Blob *b, int n )
{
  Hit hit;
  if( b->eof )
  {
    hit.type = HIT_NOTHING;
    hit.u.raw = -1;
    return hit;
  }
  else
  {
    int off = b->b->rpos + 5 + n*2;
    unsigned char h =  b->b->data[ off ];
    unsigned char l = b->b->data[ off + 1 ];
    hit.u.raw = (h<<8) | l;


    hit.type = HIT_BODY;
    if( hit.u.body.id == 3 )
    {
      hit.type = HIT_FIELD;
      if( hit.u.field.type == 63 )
	hit.type = HIT_ANCHOR;
    }
    return hit;
  }
}
int wf_blob_docid( Blob *b )
{
  if( b->eof )
    return -1;
  if( b->docid >= 0 )
    return b->docid;
  else
  {
    int off = b->b->rpos;
    unsigned char hh =  b->b->data[ off ];
    unsigned char hl = b->b->data[ off+1 ];
    unsigned char lh =  b->b->data[ off+2 ];
    unsigned char ll = b->b->data[ off+3 ];
    return b->docid = ((((((hh<<8) | hl)<<8) | lh)<<8) | ll);
  }
}


Blob *wf_blob_new( struct svalue *feed, int word )
{
  Blob *b = malloc( sizeof( Blob ) );
  MEMSET(b, 0, sizeof(b) );
  b->word = word;
  b->feed = feed;
  b->b = wf_buffer_new();
  return b;
}

void wf_blob_free( Blob *b )
{
  if( b->b )  wf_buffer_free( b->b );
  free( b );
}








/* Pike interface to build blobs. */

struct hash
{
  int doc_id;
  struct hash *next;
  struct buffer data;
};

struct
{
  struct hash hash[4711];
};



