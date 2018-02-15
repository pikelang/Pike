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
//! semicolon or tab separated value list and autocompiles a format
//! description from that.  After this function has
//! successfully parsed a header-line, you can proceed with
//! either @[fetchrecord()] or @[fetch()] to get the remaining records.
//!
//! @param delimiters
//! Explicitly specify a string containing all the characters that should
//! be considered field delimiters.  If not specified or empty, the function
//! will try to autodetect the single delimiter in use.
//!
//! @param matchfieldname
//! A string containing a regular expression, using @[Regexp.SimpleRegexp]
//! syntax, or an object providing a @[Regexp.SimpleRegexp.match()]
//! single string argument compatible method, that must match all the
//! individual fieldnames before the header will be considered valid.
//!
//! @returns
//! It returns true if a CSV head has successfully been parsed.
//!
//! @seealso
//!  @[fetchrecord()], @[fetch()], @[compile()]
int parsehead(void|string delimiters,void|string|object matchfieldname)
{ if(skipemptylines())
    return 0;
  { string line=_in->gets();
    if(!delimiters||!sizeof(delimiters))
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
    _in->unread(line+"\n");
  }

  multiset delim=(<>);
  foreach(delimiters;;int c)
    delim+=(<c>);

  array res=({ (["single":1]),0 });
  mapping m=(["delim":delim]);

  if(!objectp(matchfieldname))
    matchfieldname=Regexp(matchfieldname||"");
  _eol=0;
  if(mixed err = catch
    { _checkpoint checkp=_checkpoint();
      do
      { string field=_getdelimword(m);
        res+=({ m+(["name":field]) });
	if(String.width(field)>8)
	  field=string_to_utf8(field);	  // FIXME dumbing it down for Regexp()
        if(!matchfieldname->match(field))
	  throw(1);
      }
      while(!_eol);
    })
    switch(err)
    { default:
	throw(err);
      case 1:
	return 0;
    }
  setformat( ({res}) );
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
