struct buffer
{
  unsigned int size;
  /* Write pos */

  unsigned int rpos;
  /* Read pos */

  int read_only;
  /* If 1, the buffer cannot be written to (data points to str->str) */
  
  unsigned int allocated_size;
  /* The actual allocated size */

  unsigned char *data;

  struct pike_string *str;
  /* Used when read_only==1, to save memory. Data points to str->str,
   * and str has an extra reference. When the buffer is freed, the
   * reference is freed.
   */
};


/* Note: All w* and r* functions write and read to the buffer in NBO
 * (most significant byte first )
 *
 * They also handle unaligned data
 */

struct buffer *wf_buffer_new( );
/* Create a new (uninitalized) buffer. Use wf_buffer_set_empty or
 * wf_buffer_set_pike_string to initialize it.
 */

void wf_buffer_free( struct buffer *b );
/* Free the contents of the buffer and then the buffer */

void wf_buffer_clear( struct buffer *b );
/* Free the contents of the buffer, Use wf_buffer_set_empty or
 * wf_buffer_set_pike_string to initialize it again.
 */

void wf_buffer_set_empty( struct buffer *b );
/* Sets the contents of the buffer to an empty string, and read_only to 0.
 * This creates a writable buffer.
 */

void wf_buffer_set_pike_string( struct buffer *b, struct pike_string *data,
				int read_only );
/* Sets the contents of the buffer to the supplied pikestring, if
 * read_only is 1, the string is not copied, instead a reference is
 * added to it.
 *
 * To free the data, wf_buffer_clear must be called.
 */

void wf_buffer_wbyte( struct buffer *b,  unsigned char s );
/* Write a byte to the buffer.
 * read_only must be 0 
 */

void wf_buffer_wshort( struct buffer *b, unsigned short s );
/* Write a (16 bit) short to the buffer. 
 * read_only must be 0 
 */

void wf_buffer_wint( struct buffer *b,   unsigned int s );
/* Write a (32 bit) int to the buffer.
 * read_only must be 0 
 */

int wf_buffer_rbyte( struct buffer *b );
/* Read a byte from the buffer.
 * If wf_buffer_eof() is true, 0 is returned.
 */

unsigned int wf_buffer_rint( struct buffer *b );
/* Read an integer from the buffer.
 * If wf_buffer_eof() is true, 0 is returned.
 */

int wf_buffer_rshort( struct buffer *b );
/* Read a short from the buffer
 * If wf_buffer_eof() is true, 0 is returned.
 */

int wf_buffer_eof( struct buffer *b );
/* Returns 1 if the buffer does not have any more data to _read_ */

void wf_buffer_seek( struct buffer *b, unsigned int pos );
/* Move the read position to pos. if pos is greater than or equal the
 * size of the buffer, read_pos is moved to the end of the buffer
 * instead. 
 */

void wf_buffer_seek_w( struct buffer *b, unsigned int pos );
/* Set the write position to pos. If pos is greater than the current
 * size of the buffer, the data segment created is zeroed.
 */
  
void wf_buffer_append( struct buffer *b, char *data, int size );
/* Append the specified data to the buffer. 
 * read_only must be 0.
 */

int wf_buffer_memcpy( struct buffer *d,  struct buffer *s, int nelems );
/* Write nelems elements of data from s to d.
 * read_only must be 0.
 */

void wf_buffer_rewind_r( struct buffer *b, int n );
/* Subtract n from read_pos.
 * If n == -1, the position is set to 0.
 */

void wf_buffer_rewind_w( struct buffer *b, int n );
/* Subtract n from size.
 * If n == -1, the size is set to 0.
 */


