struct words
{
  unsigned int size;
  unsigned int allocated_size;
  struct word
  {
    unsigned int start;
    unsigned int size;
  } words[1];
};

void uc_words_free( struct words *w );
struct words *unicode_split_words_buffer( struct buffer *data );
int unicode_is_wordchar( int c );
