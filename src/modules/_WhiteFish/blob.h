typedef struct _Blob
{
  int word;
  struct svalue *feed;

  char *blob;
  int len;
  int pos;
} Blob;


typedef union
{
  unsigned short raw;
  struct
  {
    char id:2;
    int position:14;
  } body;
  struct
  {
    char id:2;
    char type:6;
    char pos:8;
  } field;
  struct
  {
    char id:2;
    char type:6;
    char hash:4;
    char pos:4;
  } anchor;
} Hit;

Blob *wf_blob_new( struct svalue *feed, int word );
int   wf_blob_next( Blob *b );
int   wf_blob_nhits( Blob *b );
Hit   wf_blob_hit( Blob *b, int n );
int   wf_blob_docid( Blob *b );

