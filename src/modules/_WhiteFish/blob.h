typedef struct _Blob
{
  struct svalue *feed;

  int word;
  int docid;
  int eof;
  
  struct buffer *b;
} Blob;

typedef enum {
  HIT_NOTHING,
  HIT_BODY,
  HIT_FIELD,
  HIT_ANCHOR
} HitType;

typedef struct
{
  HitType type;
  union
  {
    unsigned short raw;
    struct
    {
      unsigned char id:2;
      unsigned int pos:14;
    } body;
    struct
    {
      unsigned char _pad:2;
      unsigned char type:6;
      unsigned char pos:8;
    } field;
    struct
    {
      unsigned char _pad;
      unsigned char hash:4;
      unsigned char pos:4;
    } anchor;
  } u;
} Hit;

Blob *wf_blob_new( struct svalue *feed, int word );
/* Create a new blob object. The 'feed' function gets 'word' as it's
 * only argument, and it should return more blob-data to be processed.
 */

void wf_blob_free( Blob *b );
/* Free a blob object */

int wf_blob_next( Blob *b );
/* Return the document-id of the next document in the blob, or -1 */

int wf_blob_nhits( Blob *b );
/* Return the number of hits for the current document in the blob */

Hit wf_blob_hit( Blob *b, int n );
/* Return the given hit. n must be smaller than the value returned by
 * nhits.
 */

int wf_blob_docid( Blob *b );
/* Return the current document-id of the blob, same as the value
 * returned from wf_blob_next()
 */

int wf_blob_eof( Blob *b );
/* Returns -1 if there are no more entries available */




void init_blob_program();
void exit_blob_program();
