struct buffer
{
  int allocated_size;
  int size;
  int rpos;
  int read_only;

  char *data;
  struct pike_string *str;
};


void wf_buffer_make_space( struct buffer *b, int n );
void wf_buffer_wbyte( struct buffer *b,  unsigned char s );
void wf_buffer_wshort( struct buffer *b, unsigned short s );
void wf_buffer_wint( struct buffer *b,   unsigned int s );

int wf_buffer_rbyte( struct buffer *b );
int wf_buffer_rint( struct buffer *b );
int wf_buffer_rshort( struct buffer *b );
int wf_buffer_eof( struct buffer *b );

void wf_buffer_seek( struct buffer *b, int pos );

struct buffer *wf_buffer_new( );
void wf_buffer_free( struct buffer *b );

void wf_buffer_clear( struct buffer *b );
void wf_buffer_set_empty( struct buffer *b );
void wf_buffer_set_pike_string( struct buffer *b, struct pike_string *data,
				int read_only );

void wf_buffer_append( struct buffer *b, char *data, int size );




