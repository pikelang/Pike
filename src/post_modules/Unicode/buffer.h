struct buffer
{
  unsigned int allocated_size;
  unsigned int size, rpos;
  INT32 *data;
};

void uc_buffer_write( struct buffer *d, INT32 data );
INT32 uc_buffer_read( struct buffer *d );
struct buffer *uc_buffer_new( );
struct buffer *uc_buffer_write_pikestring( struct buffer *d,
					   struct pike_string *s );
void uc_buffer_free( struct buffer *d);
struct pike_string *uc_buffer_to_pikestring( struct buffer *d );
void uc_buffer_insert( struct buffer *b, unsigned int pos, int c );
struct buffer *uc_buffer_from_pikestring( struct pike_string *s );
struct buffer *uc_buffer_new_size( int s );





