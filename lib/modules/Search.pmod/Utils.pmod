public array(string) tokenize_and_normalize( string what )
//! This can be optimized quite significantly when compared to
//! tokenize( normalize( x ) ) in the future, currently it's not all
//! that much faster, but still faster.
{
  return Unicode.split_words_and_normalize( lower_case(what) );
}

public array(string) tokenize(string in)
//! Tokenize the input string (Note: You should first call normalize
//! on it)
{
  return Unicode.split_words( in );
}


public string normalize(string in)
//! Normalize the input string. Performs unicode NFKD normalization
//! and then lowercases the whole string
{
  return Unicode.normalize( lower_case(in), "KD" );
}
