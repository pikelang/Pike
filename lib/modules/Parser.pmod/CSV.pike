//! This is a parser for line oriented data that is either comma,
//! semi-colon or tab separated.  It extends the functionality
//! of the @[Parser.Tabular] with some specific functionality related
//! to a header and record oriented parsing of huge datasets.
//!
//! We document only the differences with the basic @[Parser.Tabular].
//!
//! @seealso
//!  @[Parser.Tabular]

#pike __REAL_VERSION__

inherit Parser.Tabular;

//! This function consumes the header-line preceding a typical comma,
//! semicolon or tab separated value list.
//!
//! @param delimiters
//! Explicitly specify a string containing all the characters that should
//! be considered field delimiters.  If not specified, the function will
//! try to autodetect the single delimiter in use.
//!
//! @returns
//! It returns true if a CSV head has successfully been parsed.
//!
//! @seealso
//!  @[fetchrecord()], @[compile()]
int parsehead(void|string delimiters)
{ if(skipemptylines())
    return 0;
  string line;
  line=_in->gets();
  if(!delimiters)
  { int countcomma,countsemicolon,counttab;
    countcomma=countsemicolon=counttab=0;
    foreach(line;;int c)
      switch(c)
      { case ',':countcomma++;
	  break;
        case ';':countsemicolon++;
	  break;
        case '\t':counttab++;
	  break;
      }
    delimiters=countcomma>countsemicolon?countcomma>counttab?",":"\t":
      countsemicolon>counttab?";":"\t";
  }
  multiset delim=(<>);
  foreach(delimiters;;int c)
    delim+=(<c>);
  array res=({ (["single":1]),0 });
  string format="%[^"+replace(delimiters,(["\\":"\\\\","-":"\\-","]":"\\]"]))
   +"]%*c%s";
  line+=delimiters[..0];
  do
  { string word;
    sscanf(line,format,word,line);
    res+=({(["delim":delim,"name":String.trim_whites(word)])});
  }
  while(sizeof(line));
  setformat(({res}));
  return 1;
}

//! This function consumes a single record from the input.
//! To be used in conjunction with @[parsehead()].
//!
//! @returns
//! It returns the mapping describing the record.
//!
//! @seealso
//!  @[parsehead()], @[fetch()]
mapping fetchrecord(void|array|mapping format)
{ mapping res=fetch(format);
  if(!res)
    return UNDEFINED;
  foreach(res;;mapping v)
     return v;
}
