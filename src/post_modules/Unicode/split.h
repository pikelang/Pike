/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: split.h,v 1.4 2004/04/11 18:51:10 per Exp $
*/

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
struct words *unicode_split_words_pikestr0( struct pike_string *data );
int unicode_is_wordchar( int c );
