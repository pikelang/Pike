// -*- Pike -*-
//
// RFC1521 functionality for Pike
//
// Marcus Comstedt 1996-1999


//! @rfc{1521@}, the @b{Multipurpose Internet Mail Extensions@} memo, defines a
//! structure which is the base for all messages read and written by
//! modern mail and news programs.  It is also partly the base for the
//! HTTP protocol.  Just like @rfc{822@}, MIME declares that a message should
//! consist of two entities, the headers and the body.  In addition, the
//! following properties are given to these two entities:
//!
//! @dl
//!  @item Headers
//!  @ul
//!   @item
//!     A MIME-Version header must be present to signal MIME compatibility
//!   @item
//!     A Content-Type header should be present to describe the nature of
//!     the data in the message body.  Seven major types are defined, and an
//!     extensive number of subtypes are available.  The header can also
//!     contain attributes specific to the type and subtype.
//!   @item
//!     A Content-Transfer-Encoding may be present to notify that the data
//!     of the body is encoded in some particular encoding.
//!  @endul
//!  @item Body
//!  @ul
//!   @item
//!     Raw data to be interpreted according to the Content-Type header
//!   @item
//!     Can be encoded using one of several Content-Transfer-Encodings to
//!     allow transport over non 8bit clean channels
//!  @endul
//! @enddl
//!
//! The MIME module can extract and analyze these two entities from a stream
//! of bytes.  It can also recreate such a stream from these entities.
//! To encapsulate the headers and body entities, the class @[MIME.Message] is
//! used.  An object of this class holds all the headers as a mapping from
//! string to string, and it is possible to obtain the body data in either
//! raw or encoded form as a string.  Common attributes such as message type
//! and text char set are also extracted into separate variables for easy
//! access.
//!
//! The Message class does not make any interpretation of the body
//! data, unless the content type is @tt{multipart@}.  A multipart
//! message contains several individual messages separated by boundary
//! strings.  The @[Message->create] method of the Message class will
//! divide a multipart body on these boundaries, and then create
//! individual Message objects for each part.  These objects will be
//! collected in the array @[Message->body_parts] within the original
//! Message object. If any of the new @[Message] objects have a body of
//! type multipart, the process is of course repeated recursively.


#pike __REAL_VERSION__
inherit ___MIME;

//! Class representing a substring of a larger string.
//!
//! This class is used to reduce the number of string copies
//! during parsing of @[MIME.Message]s.
protected class StringRange
{
  string data;
  int start;	// Inclusive.
  int end;	// Exclusive.
  protected void create(string|StringRange s, int start, int end)
  {
    if (start == end) {
      data = "";
      this::start = this::end = 0;
      return;
    }
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (objectp(s)) {
      start += s->start;
      if (start > s->end) start = s->end;
      end += s->start;
      if (end > s->end) end = s->end;
      s = s->data;
    }
    if ((end - start)*16 < sizeof(s)) {
      s = s[start..end-1];
      end -= start;
      start = 0;
    }
    data = s;
    this::start = start;
    this::end = end;
  }
  protected int _sizeof()
  {
    return end-start;
  }
  protected string|StringRange `[..](int low, int ltype, int high, int htype)
  {
    int len = end - start;
    if (ltype == Pike.INDEX_FROM_END) {
      low = len - (low + 1);
    }
    high += 1;
    if (htype == Pike.INDEX_FROM_END) {
      high = len - high;
    } else if (htype == Pike.OPEN_BOUND) {
      high = len;
    }
    if (low < 0) low = 0;
    if (high < 0) high = 0;
    if (low > len) low = len;
    if (high > len) high = len;
    if (!low && (high == len)) return this;
    if ((high - low) < 65536) return data[start+low..start+high-1];
    return StringRange(this, low, high);
  }
  protected int `[](int pos)
  {
    int npos = pos;
    if (npos < 0) {
      npos += end;
      if (npos < start) {
	error("Index out of range [-%d..%d]\n", 1 + end-start, end-start);
      }
    } else {
      npos += start;
      if (npos >= end) {
	error("Index out of range [-%d..%d]\n", 1 + end-start, end-start);
      }
    }
    return data[npos];
  }
  protected mixed cast(string type)
  {
    if( type == "string" )
      return data[start..end-1];
    return UNDEFINED;
  }
  protected int _search(string frag, int|void pos)
  {
    if (pos < 0)
      error("Start must be greater or equal to zero.\n");
    int npos = pos + start;
    if (npos > end)
      error("Start must not be greater than the length of the string.\n");
    if ((npos + sizeof(frag)) > end) return -1;
    npos = search(data, frag, npos);
    if (npos < 0) return npos;
    if ((npos + sizeof(frag)) > end) return -1;
    return npos - start;
  }
  protected string _sprintf(int c)
  {
    if (c == 'O')
      return sprintf("StringRange(%d bytes[%d..%d] %O)",
		     data && sizeof(data), start, end-1, data && data[..40]);
    return (string)this;
  }
}

private string|zero boundary_prefix;

//! Set a message boundary prefix. The @[MIME.generate_boundary()] will use this
//! prefix when creating a boundary string.
//!
//! @throws
//!  An error is thrown if @[boundary_prefix] doesn't adhere to @rfc{1521@}.
//!
//! @seealso
//!  @[MIME.generate_boundary()], @[MIME.get_boundary_prefix()]
//!
//! @param boundary_prefix
//!  This must adhere to @rfc{1521@} and can not be longer than 56 characters.
void set_boundary_prefix(string(8bit) boundary_prefix)
{
  // 5 upto 14 chars is randomly added to the boundary so the prefix must not
  // risking overflowing the max-length of 70 chars
  if (boundary_prefix && (sizeof(boundary_prefix) + 14) > 70) {
    error("Too long boundary prefix. The boundary prefix can not be longer "
          "than 56 characters.\n");
  }

  sscanf(boundary_prefix, "%*s%[^0-9a-zA-Z'()+_,./:=?-]", string illegal);

  if (illegal && sizeof(illegal)) {
    error("An illegal character (%q) was found in the boundary prefix.\n",
          illegal);
  }

  this::boundary_prefix = boundary_prefix;
}

//! Returns the @tt{boundary_prefix@} set via @[set_boundary_prefix()].
//!
//! @seealso
//!  @[MIME.set_boundary_prefix()], @[MIME.Message.setboundary()]
string(8bit) get_boundary_prefix()
{
  return boundary_prefix;
}

//! This function will create a string that can be used as a separator string
//! for multipart messages. If a boundary prefix has been set
//! using @[MIME.set_boundary_prefix()], the generated string will be prefixed
//! with the boundary prefix.
//!
//! The generated string is guaranteed not to appear
//! in @tt{base64@}, @tt{quoted-printable@}, or @tt{x-uue@} encoded data.
//! It is also unlikely to appear in normal text.  This function is used by
//! the cast method of the @tt{Message@} class if no boundary string is
//! specified.
//!
//! @seealso
//! @[MIME.set_boundary_prefix()]
//!
string generate_boundary( )
{
  if (boundary_prefix) {
    return boundary_prefix + random( 1000000000 );
  }
  return "'ThIs-RaNdOm-StRiNg-/=_."+random( 1000000000 )+":";
}

//! Extract raw data from an encoded string suitable for transport between
//! systems.
//!
//! The encoding can be any of
//! @string
//!   @value "7bit"
//!   @value "8bit"
//!   @value "base64"
//!   @value "binary"
//!   @value "quoted-printable"
//!   @value "x-uue"
//!   @value "x-uuencode"
//! @endstring
//!
//! The encoding string is not case sensitive.
//!
//! @seealso
//! @[MIME.encode()]
//!
string|StringRange decode( string|StringRange data, void|string encoding )
{
  switch (lower_case( encoding || "binary" )) {
  case "base64":
    return decode_base64( (string)data );
  case "quoted-printable":
    return decode_qp( (string)data );
  case "x-uue":
  case "x-uuencode":
    return decode_uue( (string)data );
  case "7bit":
  case "8bit":
  case "binary":
    return data;
  default:
    error("Unknown transfer encoding %s.\n", encoding);
  }
}

//! Encode raw data into something suitable for transport to other systems.
//!
//! The encoding can be any of
//! @string
//!   @value "7bit"
//!   @value "8bit"
//!   @value "base64"
//!   @value "binary"
//!   @value "quoted-printable"
//!   @value "x-uue"
//!   @value "x-uuencode"
//! @endstring
//!
//! The encoding string is not case sensitive.  For the @tt{x-uue@} encoding,
//! an optional @[filename] string may be supplied.
//!
//! If a nonzero value is passed as @[no_linebreaks], the result string
//! will not contain any linebreaks (@tt{base64@} and @tt{quoted-printable@}
//! only).
//!
//! @seealso
//! @[MIME.decode()]
//!
string encode( string data, void|string encoding, void|string filename,
	       void|int no_linebreaks )
{
  switch (lower_case( encoding || "binary" )) {
  case "base64":
    return encode_base64( data, no_linebreaks );
  case "quoted-printable":
    return encode_qp( data, no_linebreaks );
  case "x-uue":
  case "x-uuencode":
    return encode_uue( data, filename );
  case "7bit":
  case "8bit":
  case "binary":
    return data;
  default:
    error("Unknown transfer encoding %s.\n", encoding);
  }
}

//! Extracts the textual content and character set from an @i{encoded word@}
//! as specified by @rfc{1522@}/@rfc{2047@}.  The result is an array where the
//! first element is the raw text, and the second element the name of the
//! character set. If the input string is not an encoded word, the result is
//! still an array, but the char set element will be set to 0.
//!
//! @note
//! Note that this function can only be applied to individual encoded words.
//!
//! @seealso
//! @[MIME.encode_word()]
//!
array(string) decode_word( string word )
{
  string charset, encoding, encoded_text;
  if (sscanf( word,
	      "=?%[^][ \t()<>@,;:\"\\/?.=]?%[^][ \t()<>@,;:\"\\/?.=]?%s?=",
	      charset, encoding, encoded_text) == 3 ) {
    switch (lower_case( encoding )) {
    case "b":
      encoding = "base64";
      break;
    case "q":
      encoding = "quoted-printable";
      break;
    default:
      error( "Invalid rfc1522 encoding %s.\n", encoding );
    }
    return ({ decode( replace( encoded_text, "_", " " ), encoding ),
	      lower_case( charset ) });
  } else
    return ({ word, 0 });
}

//! Create an @i{encoded word@} as specified in @rfc{1522@} from an array
//! containing a raw text string and a char set name.
//!
//! The text will be transfer encoded according to the encoding argument,
//! which can be either @expr{"base64"@} or @expr{"quoted-printable"@}
//! (or either @expr{"b"@} or @expr{"q"@} for short).
//!
//! If either the second element of the array (the char set name), or
//! the encoding argument is 0, the raw text is returned as is.
//!
//! @seealso
//! @[MIME.decode_word()]
//!
string encode_word( string|array(string|zero) word, string|zero encoding )
{
  if (stringp(word))
    return word;
  if (!encoding || !word[1])
    return word[0];
  switch (lower_case(encoding)) {
  case "b":
  case "base64":
    encoding = "base64";
    break;
  case "q":
  case "quoted-printable":
    encoding = "quoted-printable";
    break;
  default:
    error( "Invalid rfc1522 encoding %s.\n", encoding);
  }
  string enc = encode( word[0], encoding, 0, 1 );
  if (encoding == "quoted-printable")
    enc = replace( enc, ({ "?", "_", "(", ")", "\\", "\"" }),
		   ({ "=3F", "=5F", "=28", "=29", "=5C", "=22" }) );
  return "=?"+word[1]+"?"+encoding[0..0]+"?"+ enc +"?=";
}

protected string remap(array(string|zero) item)
{
  if (sizeof(item)>1 && item[1])
    return Charset.decoder(item[1])->feed(item[0])->drain();
  else
    return item[0];
}

protected array(string) reremap(string word, string|function(string:string) selector,
			     string|void replacement,function(string:string)|void repcb)
{
  if(max(@values(word))<128)
    return ({ word,0 });
  string s = stringp(selector)? selector : ([function]selector)(word);
  return s?
    ({ Charset.encoder(s,replacement,repcb)->feed(word)->drain(), s }) :
    ({ word,0 });
}

//! Separates a header value containing @i{text@} into units and calls
//! @[MIME.decode_word()] on them.  The result is an array where each element
//! is a result from @[decode_word()].
//!
//! @seealso
//! @[MIME.decode_words_tokenized]
//! @[MIME.decode_words_text_remapped]
//!
array(array(string)) decode_words_text( string txt )
{
  object r = Regexp("^(.*[ \t\n\r]|)(=\\?[^\1- ?]*\\?[^\1- ?]*\\?"
		    "[^\1- ?]*\\?=)(([ \t\n\r]+)(.*)|)$");
  array a, res = ({});
  while ((a = r->split(txt)))
  {
    if(!sizeof(a[2])) a = a[..2]+({"",""});
    txt = a[0]||"";
    if(!sizeof(res) || sizeof(a[4])) a[4]=a[3]+a[4];
    array w = decode_word(a[1]);
    if (sizeof(a[4]))
      res = ({ w, ({ a[4], 0 }) }) + res;
    else
      res = ({ w }) + res;
  }
  a = res;
  res = ({});
  if (sizeof(txt)) res = ({ ({ txt, 0 }) });
  foreach(a, array(string) word) {
    if (sizeof(res) && res[-1][1] && (res[-1][1] == word[1])) {
      // Same character set as previous word -- Join the fragments.
      // This is a workaround for MUA's that split
      // the text in the middle of encoded characters.
      // eg PHPMailer [version 1.73]
      res[-1][0] += word[0];
    } else {
      res += ({ word });
    }
  }
  return res;
}

//! Like @[MIME.decode_words_text()], but the extracted strings are
//! also remapped from their specified character encoding into UNICODE,
//! and then pasted together.  The result is thus a string in the original
//! text format, without @rfc{1522@} escapes, and with all characters in UNICODE
//! encoding.
//!
//! @seealso
//! @[MIME.decode_words_tokenized_remapped]
//!
string decode_words_text_remapped( string txt )
{
  return map(decode_words_text(txt), remap)*"";
}

//! Tokenizes a header value just like @[MIME.tokenize()], but also
//! converts encoded words using @[MIME.decode_word()].  The result is
//! an array where each element is either an @expr{int@} representing
//! a special character, or an @expr{array@} as returned by
//! @[decode_word()] representing an atom or a quoted string.
//!
//! @seealso
//! @[MIME.decode_words_tokenized_labled]
//! @[MIME.decode_words_tokenized_remapped]
//! @[MIME.decode_words_text]
//!
array(array(string)|int) decode_words_tokenized( string phrase, int|void flags )
{
  return map(tokenize(phrase, flags),
		   lambda(string|int item) {
		     return intp(item)? item : decode_word(item);
		   });
}

//! Like @[MIME.decode_words_tokenized()], but the extracted atoms are
//! also remapped from their specified character encoding into UNICODE.
//! The result is thus identical to that of @[MIME.tokenize()], but
//! without @rfc{1522@} escapes, and with all characters in UNICODE encoding.
//!
//! @seealso
//! @[MIME.decode_words_tokenized_labled_remapped]
//! @[MIME.decode_words_text_remapped]
//!
array(string|int) decode_words_tokenized_remapped( string phrase,
						   int|void flags )
{
  return map(decode_words_tokenized(phrase, flags),
		   lambda(array(string)|int item) {
		     return intp(item)? item : remap(item);
		   });
}

//! Tokenizes and labels a header value just like @[MIME.tokenize_labled()],
//! but also converts encoded words using @[MIME.decode_word()].  The result
//! is an array where each element is an array of two or more elements, the
//! first being the label.  The rest of the array depends on the label:
//!
//! @string
//!   @value "special"
//!     One additional element, containing the character code for the special
//!     character as an @expr{int@}.
//!   @value "word"
//!     Two additional elements, the first being the word, and the second
//!     being the character set of this word (or 0 if it did not originate
//!     from an encoded word).
//!   @value "domain-literal"
//!     One additional element, containing the domain literal as a string.
//!   @value "comment"
//!     One additional element, containing an array as returned by
//!     @[MIME.decode_words_text()].
//! @endstring
//!
//! @seealso
//! @[MIME.decode_words_tokenized_labled_remapped]
//!
array(array(string|int|array(array(string))))
decode_words_tokenized_labled( string phrase, int|void flags )
{
  return map( tokenize_labled( phrase, flags ),
		    lambda(array(string|int) item) {
		      switch(item[0]) {
		      case "encoded-word":
			return ({ "word", @decode_word(item[1]) });
		      case "word":
			return item + ({ 0 });
		      case "comment":
			return ({ "comment", decode_words_text(item[1]) });
		      default:
			return item;
		      }
		    });
}

//! Like @[MIME.decode_words_tokenized_labled()], but the extracted words are
//! also remapped from their specified character encoding into UNICODE.
//! The result is identical to that of @[MIME.tokenize_labled()], but
//! without @rfc{1522@} escapes, and with all characters in UNICODE encoding.
//!
array(array(string|int))
decode_words_tokenized_labled_remapped(string phrase, int|void flags)
{
  return map(decode_words_tokenized_labled(phrase, flags),
		   lambda(array(string|int|array(array(string|int))) item) {
		     switch(item[0]) {
		     case "word":
		       return ({ "word", remap(item[1..]) });
		     case "comment":
		       return ({ "comment", map(item[1], remap)*"" });
		     default:
		       return item;
		     }
		   });
}

//! Decodes the given string as a key-value parameter cascade
//! according to e.g. @rfc{7239:4@}.
//!
//! @note
//! This function will decode all conforming inputs, but it will also
//! be forgiving when presented with non-conforming inputs.
//!
//! @seealso
//! @[encode_headerfield_params]
array(ADT.OrderedMapping) decode_headerfield_params (string s)
{
  array(mapping(string:string)|ADT.OrderedMapping) totres = ({});
  ADT.OrderedMapping mapres = ADT.OrderedMapping();
  string key, goteq;
  int nextset;	   // Fake a terminating ',' to ensure proper post-processing
  foreach (MIME.tokenize(s) + ({ ',' }); ; int|string token) {
    switch (token) {
      case ',':
        nextset = 1;
      case ';':
        token = goteq;
        goteq = 0;
        break;
      case '=':
        goteq = "";
        continue;
      default:
        if (key)
          break;
        key = token; goteq = 0;
        continue;
    }
    if (key)
      mapres[key] = token, key = 0;
    if (nextset && sizeof(mapres))
      totres += ({ mapres }), mapres = ADT.OrderedMapping();
    nextset = 0;
  }
  return totres;
}

//! The inverse of @[decode_words_text()], this function accepts
//! an array of strings or pairs of strings which will each be encoded
//! by @[encode_word()], after which they are all pasted together.
//!
//! @param encoding
//!   Either @expr{"base64"@} or @expr{"quoted-printable"@}
//!  (or either @expr{"b"@} or @expr{"q"@} for short).
//!
//! @seealso
//! @[MIME.encode_words_text_remapped]
//!
string encode_words_text(array(string|array(string)) phrase, string encoding)
{
  phrase = filter(phrase, lambda(string|array(string) w) {
			    return stringp(w)? sizeof(w) :
			      sizeof(w[0]) || w[1];
			  });
  array(string) ephrase = map(phrase, encode_word, encoding);
  if(!encoding) return ephrase*"";
  string res="";
  for(int i=0; i<sizeof(ephrase); i++)
    if(ephrase[i] != (stringp(phrase[i])? phrase[i] : phrase[i][0])) {
      if(sizeof(res) && !(<' ','\t','\n','\r'>)[res[-1]])
	res += " ";
      res += ephrase[i];
      if(i+1<sizeof(ephrase) && !(<' ','\t','\n','\r'>)[ephrase[i+1][0]])
	res += " ";
    } else
      res += ephrase[i];
  return res;
}

//! This is the reverse of @[MIME.decode_words_text_remapped()].  A
//! single UNICODE string is provided, which is separated into
//! fragments and transcoded to selected character sets by this
//! function as needed.
//!
//! @param encoding
//!   Either @expr{"base64"@} or @expr{"quoted-printable"@}
//!  (or either @expr{"b"@} or @expr{"q"@} for short).
//! @param charset
//!   Either the name of a character set to use, or a function returning
//!   a character set to use given a text fragment as input.
//! @param replacement
//!   The @[replacement] argument to use when calling @[Charset.encoder]
//! @param repcb
//!   The @[repcb] argument to use when calling @[Charset.encoder]
//!
//! @seealso
//! @[MIME.encode_words_tokenized_remapped]
//!
string encode_words_text_remapped(string text, string encoding,
				  string|function(string:string) charset,
				  string|void replacement,
				  function(string:string)|void repcb)
{
  array(array(string)) out = ({});
  string lastword = "";
  while(sizeof(text)) {
    sscanf(text, "%[ \t\n\r]%[^ \t\n\r]%s", string ws, string word, text);
    array(string) ww = reremap(word, charset, replacement, repcb);
    if(sizeof(ws))
      if(!ww[1])
	ww[0] = ws + ww[0];
      else if(!sizeof(out))
	out = ({({ws,0})});
      else if(!out[-1][1])
	out[-1][0] += ws;
      else {
	/* Two encoded words joined by whitespace - not possible */
	word = lastword+ws+word;
	ww = reremap(word, charset, replacement, repcb);
	out = out[..<1];
      }
    lastword = word;
    out += ({ ww });
  }
  return encode_words_text(out, encoding);
}

//! The inverse of @[decode_words_tokenized()], this functions accepts
//! an array like the argument to @[quote()], but instead of simple strings
//! for atoms and quoted-strings, it will also accept pairs of strings to
//! be passed to @[encode_word()].
//!
//! @param encoding
//!   Either @expr{"base64"@} or @expr{"quoted-printable"@}
//!  (or either @expr{"b"@} or @expr{"q"@} for short).
//!
//! @seealso
//!   @[MIME.encode_words_quoted_remapped()]
//!   @[MIME.encode_words_quoted_labled()]
//!
string encode_words_quoted(array(array(string)|int) phrase, string encoding)
{
  return quote(map(phrase, lambda(array(string)|int item) {
				   return intp(item)? item :
				     encode_word(item, encoding);
				 }));
}

//! The inverse of @[decode_words_tokenized_remapped()], this functions
//! accepts an array equivalent to the argument of @[quote()], but also
//! performs on demand word encoding in the same way as
//! @[encode_words_text_remapped()].
//!
//! @seealso
//!   @[MIME.encode_words_text_remapped()]
//!   @[MIME.encode_words_quoted_labled_remapped()]
//!
string encode_words_quoted_remapped(array(string|int) phrase, string encoding,
				    string|function(string:string) charset,
				    string|void replacement,
				    function(string:string)|void repcb)
{
  return encode_words_quoted(map(phrase, lambda(string|int item) {
					   return intp(item)? item :
					     reremap(item, charset,
						     replacement, repcb);
					 }), encoding);
}

//! The inverse of @[decode_words_tokenized_labled()], this functions accepts
//! an array like the argument to @[quote_labled()], but "word" labled
//! elements can optionally contain an additional string element specifying
//! a character set, in which case an encoded-word will be used.  Also, the
//! format for "comment" labled elements is entirely different; instead of
//! a single string, an array of strings or pairs like the first argument to
//! @[encode_words_text()] is expected.
//!
//! @param encoding
//!   Either @expr{"base64"@} or @expr{"quoted-printable"@}
//!  (or either @expr{"b"@} or @expr{"q"@} for short).
//!
//! @seealso
//!   @[MIME.encode_words_quoted()]
//!   @[MIME.encode_words_quoted_labled_remapped()]
//!
string encode_words_quoted_labled(array(array(string|int|array(string|array(string)))) phrase, string encoding)
{
  return
    quote_labled(map(phrase,
			   lambda(array(string|int|array(string)) item) {
			     switch(item[0]) {
			     case "word":
			       if(sizeof(item)>2 && item[2])
				 return ({
				   "encoded-word",
				   encode_word(item[1..], encoding) });
			       else
				 return item;
			     case "comment":
			       return ({
				 "comment",
				 encode_words_text(item[1], encoding) });
			     default:
			       return item;
			     }
			   }));
}

//! The inverse of @[decode_words_tokenized_labled_remapped()], this function
//! accepts an array equivalent to the argument of @[quote_labled()], but also
//! performs on demand word encoding in the same way as
//! @[encode_words_text_remapped()].
//!
string encode_words_quoted_labled_remapped(array(array(string|int)) phrase,
					   string encoding,
					   string|function(string:string) charset,
					   string|void replacement,
					   function(string:string)|void repcb)
{
  return quote_labled(map(phrase, lambda(array(string|int) item) {
				    switch(item[0]) {
				    case "word":
				      item = item[..0]+reremap(item[1],
							       charset,
							       replacement,
							       repcb);
				      if(sizeof(item)>2 && item[2])
					return ({
					  "encoded-word",
					  encode_word(item[1..], encoding) });
				      else
					return item;
				    case "comment":
				      return ({
					"comment",
					encode_words_text_remapped(item[1],
								   encoding,
								   charset,
								   replacement,
								   repcb) });
				    default:
				      return item;
				    }
				  }));
}

//! Encodes the given key-value parameters as a string
//! according to e.g. @rfc{7239:4@}.
//!
//! @seealso
//! @[decode_headerfield_params]
string encode_headerfield_params(array(mapping|ADT.OrderedMapping) params)
{
  array res = ({});
  int sep;
  foreach (params; ; mapping(string:string)|ADT.OrderedMapping m) {
    foreach (m; mixed key; mixed value) {
      if (sep)
        res += ({ sep });
      res += ({ (string)key });
      if (value)
        res += ({ '=', (string)value });
      sep = ';';
    }
    sep = ',';
  }
  return MIME.quote(res);
}

//! Provide a reasonable default for the subtype field.
//!
//! Some pre-@rfc{1521@} mailers provide only a type and no subtype in the
//! Content-Type header field.  This function can be used to obtain a
//! reasonable default subtype given the type of a message.  (This is done
//! automatically by the @[MIME.Message] class.)
//!
//! Currently, the function uses the following guesses:
//! @string
//!   @value "text"
//!     @expr{"plain"@}
//!   @value "message"
//!     @expr{"rfc822"@}
//!   @value "multipart"
//!     @expr{"mixed"@}
//! @endstring
//!
string|zero guess_subtype( string type )
{
  switch (type) {
  case "text":
    return "plain";
  case "message":
    return "rfc822";
  case "multipart":
    return "mixed";
  }
  return 0;
}

//! @decl array(mapping(string:string)|string) parse_headers(string message)
//! @decl array(mapping(string:array(string))|string) parse_headers( @
//!                                                     string message, @
//!                                                     int(1..1) use_multiple)
//!
//! This is a low level function that will separate the headers from the body
//! of an encoded message.  It will also translate the headers into a mapping.
//! It will however not try to analyze the meaning of any particular header.
//! This means that the body is returned as is, with any transfer-encoding
//! intact.
//!
//! It is possible to call this function with just the header part
//! of a message, in which case an empty body will be returned.
//!
//! The result is returned in the form of an array containing two elements.
//! The first element is a mapping containing the headers found.  The second
//! element is a string containing the body.
//!
//! Headers that occur multiple times will have their contents NUL separated,
//! unless @[use_multiple] has been specified, in which case the contents will
//! be arrays.
//!
//! @note
//! Some headers (eg Subject) may include @rfc{1522@}/@rfc{2047@} encoded words. To
//! decode these, see @[decode_words_text] and @[decode_words_tokenized] and
//! their friends.
//!
array(mapping(string:string|array(string))|string|StringRange)
  parse_headers(string|StringRange message, void|int(1..1) use_multiple)
{
  string head, header, hname, hcontents;
  string|StringRange body;
  int mesgsep;
  if (has_prefix(message, "\r\n") || has_prefix(message, "\n")) {
    // No headers.
    return ({ ([]), message[1 + (message[0] == '\r')..] });
  } else {
    int mesgsep1 = search(message, "\r\n\r\n");
    int mesgsep2 = search(message, "\n\n");
    mesgsep = (mesgsep1<0? mesgsep2 :
               (mesgsep2<0? mesgsep1 :
                (mesgsep1<mesgsep2? mesgsep1 : mesgsep2)));
    if (mesgsep<0) {
      // No body, or only body.
      head = (string)message;
      body = "";
    } else if (mesgsep) {
      head = (string)(mesgsep>0? message[..mesgsep-1]:"");
      body = message[mesgsep+(message[mesgsep]=='\r'? 4:2)..];
    }
  }
  mapping(string:string|array) headers = ([ ]);
  foreach( replace(head, ({"\r", "\n ", "\n\t"}),
		   ({"", " ", " "}))/"\n", header )
  {
    if(4==sscanf(header, "%[!-9;-~]%*[ \t]:%*[ \t]%s", hname, hcontents))
    {
      hname = lower_case(hname);
      if (use_multiple)
	headers[hname] += ({hcontents});
      else
	if(headers[hname])
	  headers[hname] += "\0"+hcontents;
	else
	  headers[hname] = hcontents;
    }
  }

  if( mesgsep<0 && !sizeof(headers) )
    return ({ ([]), (string)message });
  return ({ headers, body });
}


//! This class is used to hold a decoded MIME message.
class Message {

  import Array;

  protected string|StringRange encoded_data;
  protected string|StringRange decoded_data;

  //! This mapping contains all the headers of the message.
  //!
  //! The key is the header name (in lower case) and the value is
  //! the header value.
  //!
  //! Although the mapping contains all headers, some particular headers get
  //! special treatment by the module and should @b{not@} be accessed through
  //! this mapping. These fields are currently:
  //! @string
  //!   @value "content-type"
  //!   @value "content-disposition"
  //!   @value "content-length"
  //!   @value "content-transfer-encoding"
  //! @endstring
  //! The contents of these fields can be accessed and/or modified through
  //! a set of variables and methods available for this purpose.
  //!
  //! @seealso
  //! @[type], @[subtype], @[charset], @[boundary], @[transfer_encoding],
  //! @[params], @[disposition], @[disp_params], @[setencoding()],
  //! @[setparam()], @[setdisp_param()], @[setcharset()], @[setboundary()]
  //!
  //! @note
  //! Some headers (eg Subject) may include @rfc{1522@}/@rfc{2047@} encoded words. To
  //! decode these, see @[decode_words_text] and @[decode_words_tokenized] and
  //! their friends.
  //!
  mapping(string:string) headers;

  //! If the message is of type @tt{multipart@}, this is an array
  //! containing one Message object for each part of the message.
  //! If the message is not a multipart, this field is @expr{0@} (zero).
  //!
  //! @seealso
  //! @[type], @[boundary]
  //!
  array(object) body_parts;

  //! For multipart messages, this @tt{Content-Type@} parameter gives a
  //! delimiter string for separating the individual messages.  As multiparts
  //! are handled internally by the module, you should not need to access this
  //! field.
  //!
  //! @seealso
  //! @[setboundary()]
  //!
  string boundary;

  //! One of the possible parameters of the @tt{Content-Type@} header is the
  //! charset attribute. It determines the character encoding used in bodies of
  //! type @tt{text@}.
  //!
  //! If there is no @tt{Content-Type@} header, the value of this field
  //! is @expr{"us-ascii"@}.
  //!
  //! @seealso
  //! @[type]
  //!
  string charset;

  //! The @tt{Content-Type@} header contains a type, a subtype, and optionally
  //! some parameters. This field contains the type attribute extracted
  //! from the header.
  //!
  //! If there is no @tt{Content-Type@} header, the value of this field
  //! is @expr{"text"@}.
  //!
  //! @seealso
  //! @[subtype], @[params]
  //!
  string type;

  //! The @tt{Content-Type@} header contains a type, a subtype, and optionally
  //! some parameters. This field contains the subtype attribute extracted
  //! from the header.
  //!
  //! If there is no @tt{Content-Type@} header, the value of this field
  //! is @expr{"plain"@}.
  //!
  //! @seealso
  //! @[type], @[params]
  //!
  string subtype;

  //! The contents of the @tt{Content-Transfer-Encoding@} header.
  //!
  //! If no @tt{Content-Transfer-Encoding@} header is given, this field
  //! is @expr{0@} (zero).
  //!
  //! Transfer encoding and decoding is done transparently by the module,
  //! so this field should be interesting only to applications wishing to
  //! do auto conversion of certain transfer encodings.
  //!
  //! @seealso
  //! @[setencoding()]
  //!
  string transfer_encoding;

  //! A mapping containing all the additional parameters to the
  //! @tt{Content-Type@} header.
  //!
  //! Some of these parameters have fields of their own, which should
  //! be accessed instead of this mapping wherever applicable.
  //!
  //! @seealso
  //! @[charset], @[boundary], @[setparam()]
  //!
  mapping (string:string) params;

  //! The first part of the @tt{Content-Disposition@} header, hinting on how
  //! this part of a multipart message should be presented in an interactive
  //! application.
  //!
  //! If there is no @tt{Content-Disposition@} header, this field
  //! is @expr{0@}.
  //!
  string disposition;

  //! A mapping containing all the additional parameters to the
  //! @tt{Content-Disposition@} header.
  //!
  //! @seealso
  //! @[setdisp_param()], @[get_filename()]
  //!
  mapping (string:string) disp_params;


  //! This method tries to find a suitable filename should you want to save the
  //! body data to disk.
  //!
  //! It will examine the @tt{filename@} attribute of the
  //! @tt{Content-Disposition@} header, and failing that the @tt{name@}
  //! attribute of the @tt{Content-Type@} header. If neither attribute is set,
  //! the method returns 0.
  //!
  //! @note
  //! An interactive application should always query the user for the actual
  //! filename to use.  This method may provide a reasonable default though.
  //!
  string get_filename( )
  {
    string fn = disp_params["filename"] || params["name"];
    return fn && decode_words_text_remapped(fn);
  }

  //! If this message is a part of a fragmented message (i.e. has a
  //! Content-Type of @tt{message/partial@}), an array with three elements
  //! is returned.
  //!
  //! The first element is an identifier string. This string should be used to
  //! group this message with the other fragments of the message (which will
  //! have the same id string).
  //!
  //! The second element is the sequence number of this fragment. The first
  //! part will have number 1, the next number 2 etc.
  //!
  //! The third element of the array is either the total number of fragments
  //! that the original message has been split into, or 0 of this information
  //! is not available.
  //!
  //! If this method is called in a message that is not a part of a fragmented
  //! message, it will return 0.
  //!
  //! @seealso
  //! @[MIME.reconstruct_partial()]
  //!
  array(string|int)|zero is_partial( )
  {
    return (type == "message" && subtype == "partial") &&
      ({ params["id"], (int)params["number"], (int)(params["total"]||"0") });
  }

  //! Replaces the body entity of the data with a new piece of raw data.
  //!
  //! The new data should comply to the format indicated by the
  //! @[type] and @[subtype] attributes.
  //!
  //! @note
  //! Do not use this method unless you know what you are doing.
  //!
  //! @seealso
  //! @[getdata()], @[setencoded], @[data]
  //!
  void setdata( string data )
  {
    if (data != decoded_data) {
      decoded_data = data;
      encoded_data = 0;
    }
  }

  //! @decl string data
  //!
  //! This variable contains the raw data of the message body entity.
  //!
  //! The @[type] and @[subtype] attributes indicate how this data should
  //! be interpreted.
  //!
  //! @note
  //!   In Pike 7.6 and earlier you had to use @[getdata()] and
  //!   @[setdata()] to access this value.
  //!
  //! @seealso
  //!   @[getdata()], @[setdata()]

  void `->data=(string data)
  {
    setdata(data);
  }

  //! This method returns the raw data of the message body entity.
  //!
  //! The @[type] and @[subtype] attributes indicate how this data should
  //! be interpreted.
  //!
  //! @seealso
  //! @[setdata()], @[getencoded()], @[data]
  //!
  string getdata( )
  {
    if (encoded_data && !decoded_data)
      decoded_data = decode( encoded_data, transfer_encoding );
    return decoded_data = (string)decoded_data;
  }

  string `->data()
  {
    return getdata();
  }

  //! This method returns the data of the message body entity, encoded
  //! using the current transfer encoding.
  //!
  //! You should never have to call this function.
  //!
  //! @seealso
  //! @[getdata()]
  //!
  string getencoded( )
  {
    if (decoded_data && !encoded_data)
      encoded_data = encode( (string)decoded_data, transfer_encoding,
			     get_filename() );
    return (string)encoded_data;
  }

  //! Select a new transfer encoding for this message.
  //!
  //! The @tt{Content-Transfer-Encoding@} header will be modified accordingly,
  //! and subsequent calls to @[getencoded] will produce data encoded using
  //! the new encoding.
  //!
  //! See @[MIME.encode()] for a list of valid encodings.
  //!
  //! @seealso
  //! @[getencoded()], @[MIME.encode()]
  //!
  void setencoding( string encoding )
  {
    if(encoded_data && !decoded_data)
      decoded_data = getdata( );
    headers["content-transfer-encoding"] = transfer_encoding =
      lower_case( encoding );
    encoded_data = 0;
  }

  //! Set or modify the named parameter of the @tt{Content-Type@} header.
  //!
  //! Common parameters include @tt{charset@} for text messages, and
  //! @tt{boundary@} for multipart messages.
  //!
  //! @note
  //! It is not allowed to modify the @tt{Content-Type@} header directly,
  //! please use this function instead.
  //!
  //! @seealso
  //! @[setcharset()], @[setboundary()], @[setdisp_param()]
  //!
  void setparam( string param, string value )
  {
    param = lower_case(param);
    params[param] = value;
    switch(param) {
    case "charset":
      charset = value;
      break;
    case "boundary":
      boundary = value;
      break;
    case "name":
      if(transfer_encoding != "x-uue" && transfer_encoding != "x-uuencode")
	break;
      if(encoded_data && !decoded_data)
	decoded_data = getdata( );
      encoded_data = 0;
      break;
    }
    headers["content-type"] =
      quote(({ type, '/', subtype })+
	    `+(@map(indices(params), lambda(string param) {
	      return ({ ';', param, '=', params[param] });
	    })));
  }

  //! Set or modify the named parameter of the @tt{Content-Disposition@}
  //! header.
  //!
  //! A common parameters is e.g. @tt{filename@}.
  //!
  //! @note
  //! It is not allowed to modify the @tt{Content-Disposition@} header
  //! directly, please use this function instead.
  //!
  //! @seealso
  //! @[setparam()], @[get_filename()]
  //!
  void setdisp_param( string param, string value )
  {
    param = lower_case( param );
    disp_params[param] = value;
    switch (param) {
    case "filename":
      if (transfer_encoding != "x-uue" && transfer_encoding != "x-uuencode")
	break;
      if (encoded_data && !decoded_data)
	decoded_data = getdata( );
      encoded_data = 0;
      break;
    }
    headers["content-disposition"] =
      quote(({ disposition || "attachment" })+
	    `+(@map(indices(disp_params), lambda(string param) {
	      return ({ ';', param, '=', disp_params[param] });
	    })));
  }

  //! Sets the @tt{charset@} parameter of the @tt{Content-Type@} header.
  //!
  //! This is equivalent of calling @expr{setparam("charset", @[charset])@}.
  //!
  //! @seealso
  //! @[setparam()]
  //!
  void setcharset( string charset )
  {
    setparam( "charset", charset );
  }

  //! Sets the @tt{boundary@} parameter of the @tt{Content-Type@} header.
  //!
  //! This is equivalent of calling @expr{setparam("boundary", @[boundary])@}.
  //!
  //! @seealso
  //! @[setparam()]
  //!
  void setboundary( string boundary )
  {
    setparam( "boundary", boundary );
  }

  //! Casting the message object to a string will yield a byte stream suitable
  //! for transmitting the message over protocols such as ESMTP and NNTP.
  //!
  //! The body will be encoded using the current transfer encoding, and
  //! subparts of a multipart will be collected recursively. If the message
  //! is a multipart and no boundary string has been set, one will be
  //! generated using @[generate_boundary()].
  //!
  //! @seealso
  //! @[create()]
  protected string cast( string dest_type )
  {
    string data;
    object body_part;

    if (dest_type != "string")
      return UNDEFINED;

    data = getencoded( );

    if (body_parts) {

      if (!boundary) {
	if (type != "multipart") {
	  type = "multipart";
	  subtype = "mixed";
	}
	setboundary( generate_boundary( ) );
      }

      data += "\r\n";
      foreach( body_parts, body_part )
	data += "--"+boundary+"\r\n"+((string)body_part)+"\r\n";
      data += "--"+boundary+"--\r\n";
    }

    headers["content-length"] = ""+sizeof(data);

    return map( indices(headers),
		lambda(string hname){
		  return map(arrayp(headers[hname]) ? headers[hname] :
			     headers[hname]/"\0",
			     lambda(string header,string hname) {
			       return hname+": "+header;
			     },
			     replace(map(hname/"-",
					 String.capitalize)*"-",
				     "Mime","MIME"))*"\r\n";
		} )*"\r\n" + "\r\n\r\n" + data;
  }

  protected string token_to_string(string|int token)
  {
    return intp(token) ? sprintf("%c", token) : token;
  }

  //! Parse a Content-Type or Content-Disposition parameter.
  //!
  //! @param params
  //!   Mapping to add parameters to.
  //!
  //! @param entry
  //!   Array of tokens containing a parameter declaration.
  //!
  //! @param header
  //!   Name of the header from which @[entry] originated.
  //!   This is only used to report errors.
  //!
  //! @param guess
  //!   Make a best-effort attempt to parse broken entries.
  //!
  //! @param entry2
  //!   Same as @[entry], but tokenized with @[MIME.TOKENIZE_KEEP_ESCAPES].
  //!
  //! @seealso
  //!   @[create()]
  protected void parse_param(mapping(string:string) params,
			     array(string|int) entry,
			     string header,
			     int|void guess,
			     array(string|int)|void entry2)
  {
    if(sizeof(entry)) {
      if(sizeof(entry)<3 || entry[1]!='=' || !stringp(entry[0]))
	if(guess) {
	  if ((sizeof(entry) == 1) && stringp(entry[0])) {
	    if (sizeof(entry = (entry[0]/"-")) > 1) {
	      // Assume there's a typo where the '=' has been replaced
	      // with a '-'.
	      entry = ({ entry[0], '=', entry[1..]*"-" });
	    }
	    // Just use the entry as a param with "" as the value.
	  }
	  else
	    return; // just ignore the broken data we failed to parse
	} else
	  error("invalid parameter %O in %s %O (%O)\n",
		entry[0], header, headers[lower_case(header)], guess);
      string param = lower_case(entry[0]);
      string val;
      if (guess) {
	val = map(entry[2..], token_to_string) * "";
      } else if (sizeof(filter(entry[2..], intp))) {
	error("invalid quoting of parameter %O in %s %O (%O)\n",
	      entry[0], header, headers[lower_case(header)], guess);
      } else {
	val = entry[2..]*"";
      }

      params[param] = val;

      // Check for MSIE:
      //
      // MSIE insists on sending the full local path to the file as
      // the "filename" parameter, but forgets to quote the backslashes.
      //
      // Heuristic:
      //   * If there are forward slashes, or properly quoted backslashes,
      //     everything's alright.
      //   * Note that UNC-paths (\\host\dir\file) look like they have
      //     a properly quoted backslash as the first character, so
      //     we disregard the first character.
      if ((param == "filename") && guess && entry2 &&
	  !has_value(val, "/") && !has_value(val[1..], "\\") &&
	  (sizeof(entry2) >= 3) && (entry2[1] == '=') &&
	  (lower_case(entry2[0]) == param)) {
	val = map(entry2[2..], token_to_string) * "";
	if (has_value(val, "\\"))
	  params[param] = val;
      }
    }
  }

  //! @decl void create()
  //! @decl void create(string message)
  //! @decl void create(string message, @
  //!                   mapping(string:string|array(string)) headers, @
  //!                   array(object)|void parts)
  //! @decl void create(string message, @
  //!                   mapping(string:string|array(string)) headers, @
  //!                   array(object)|void parts, int(0..1) guess)
  //!
  //! There are several ways to call the constructor of the Message class:
  //!
  //! @ul
  //!   @item
  //!     With zero arguments, you will get a dummy message with neither
  //!     headers nor body. Not very useful.
  //!   @item
  //!     With one argument, the argument is taken to be a byte stream
  //!     containing a message in encoded form. The constructor will analyze
  //!     the string and extract headers and body.
  //!   @item
  //!     With two or three arguments, the first argument is taken to be the
  //!     raw body data, and the second argument a desired set of headers.
  //!     The keys of this mapping are not case-sensitive.  If the given
  //!     headers indicate that the message should be of type multipart,
  //!     an array of Message objects constituting the subparts should be
  //!     given as a third argument.
  //!   @item
  //!     With the @[guess] argument set to 1 (@[headers] and @[parts] may be 0
  //!     if you don't want to give any), you get a more forgiving MIME Message
  //!     that will do its best to guess what broken input data really meant. It
  //!     won't always guess right, but for applications like mail archives and
  //!     similar where you can't get away with throwing an error at the user,
  //!     this comes in handy. Only use the @[guess] mode only for situations
  //!     where you @i{need@} to process broken MIME messages silently; the
  //!     abuse of overly lax tools is what poisons standards.
  //! @endul
  //!
  //! @seealso
  //! @[cast()]
  protected void create(void | string|StringRange message,
			void | mapping(string:string|array(string)) hdrs,
			void | array(object) parts,
			void | int guess)
  {
    encoded_data = 0;
    decoded_data = 0;
    headers = ([ ]);
    params = ([ ]);
    disp_params = ([ ]);
    body_parts = 0;
    type = "text";
    subtype = "plain";
    charset = "us-ascii";
    boundary = 0;
    disposition = 0;
    if (message && stringp(message) && (sizeof(message) > 0x100000)) {
      // Message is larger than 1 MB.
      // Attempt to reduce memory use by using StringRange.
      message = StringRange(message, 0, sizeof(message));
    }
    if (hdrs || parts) {
      string|array(string) hname;
      decoded_data = message;
      if (hdrs)
	foreach( indices(hdrs), hname )
	  headers[lower_case(hname)] = hdrs[hname];
      body_parts = parts;
    } else if (message)
      [ headers, encoded_data ] = parse_headers(message);

    if (headers["content-type"]) {
      array(array(string|int)) arr =
	tokenize(headers["content-type"]) / ({';'});
      array(string|int) p;
      if (guess && sizeof(arr[0]) > 3) {
	// Workspace Webmail 5.6.17 is known to be able to
	// send attachments with the content-type header
	// "application/msword application; name=\"Foo.doc\";"
	// Strip the extraneous tokens.
	arr[0] = arr[0][..2];
      }
      if(sizeof(arr[0])!=3 || arr[0][1]!='/' ||
	 !stringp(arr[0][0]) || !stringp(arr[0][2]))
	if(sizeof(arr[0])==1 && stringp(arr[0][0]) &&
	   (subtype = guess_subtype(lower_case(type = arr[0][0]))))
	  arr = ({ ({ type, '/', subtype }) }) + arr[1..];
	else if(!guess)
	  error("invalid Content-Type %O\n", headers["content-type"]);
	else
	  arr = ({ ({ "application", '/', "octet-stream" }) }) + arr[1..];
      type = lower_case(arr[0][0]);
      subtype = lower_case(arr[0][2]);
      foreach( arr[1..], p )
	parse_param(params, p, "Content-Type", guess);
      charset = lower_case(params["charset"] || charset);
      boundary = params["boundary"];
    }
    if (headers["content-disposition"]) {
      array(array(string|int)) arr;
      array(array(string|int)) arr2;
      mixed err = catch {
	  arr = tokenize(headers["content-disposition"]) / ({';'});
	};
      mixed err2 = catch {
	  arr2 = tokenize(headers["content-disposition"],
			  MIME.TOKENIZE_KEEP_ESCAPES) / ({';'});
	};
      if (err) {
	if (!guess || err2) throw(err);
	// Known broken, probably MSIE.
	arr = arr2;
	arr2 = 0;
      }

      array(string|int) p;
      if(sizeof(arr[0])!=1 || !stringp(arr[0][0]))
      {
	if(!guess)
	  error("invalid Content-Disposition in message\n");
      } else
      {
	disposition = lower_case(arr[0][0]);
	foreach( arr[1..]; int i; p )
	  parse_param(disp_params, p, "Content-Disposition", guess,
		      arr2 && ((i+1) < sizeof(arr2)) && arr2[i+1]);
      }
    }
    if (headers["content-transfer-encoding"]) {
      array(string) arr=tokenize(headers["content-transfer-encoding"]);
      if(sizeof(arr)!=1 || !stringp(arr[0]))
      {
	if(!guess)
	  error("invalid Content-Transfer-Encoding %O\n",
		headers["content-transfer-encoding"]);
      } else
	transfer_encoding = lower_case(arr[0]);
    }
    if (boundary && type=="multipart" && !body_parts &&
       (encoded_data || decoded_data)) {

      string|StringRange data = decoded_data || getdata();
      string separator = "--" + boundary;
      array(string) parts = ({});
      int start = 0;
      int found = 0;
      encoded_data = 0;
      decoded_data = 0;
      while ((found = search(data, separator, found)) != -1) {
	if (found) {
	  if (data[found-1] != '\n') {
	    found += sizeof(separator);
	    continue;
	  }
	  string part;

	  // Strip the terminating LF or CRLF.
	  if ((found > 1) && (data[found - 2] == '\r')) {
	    part = data[start..found-3];
	  } else {
	    part = data[start..found-2];
	  }
	  if (start) {
	    parts += ({ part });
	  } else {
	    decoded_data = part;
	  }
	} else {
	  decoded_data = "";
	}

	// Skip past the separator and any white space after it.
	found += sizeof(separator);
	string|zero terminator = data[found..found+1];
	if (terminator == "--") {
	  found += 2;
	} else {
	  terminator = 0;
	}
	while ((found < sizeof(data)) &&
	       ((data[found] == ' ') || (data[found] == '\t'))) {
	  found++;
	}
	if ((found < sizeof(data)) && (data[found] == '\n')) {
	  found++;
	} else if ((found < sizeof(data)) &&
		   (data[found..found+1] == "\r\n")) {
	  found += 2;
	} else if (!guess && !terminator) {
	  error("newline missing after multipart boundary\n");
	}

	start = found;
	if (terminator) break;
      }
      string epilogue = data[start..];
      if (!decoded_data) {
	if (guess) {
	  decoded_data = epilogue;
	  epilogue = "";
	} else
	  error("boundary missing from multipart-body\n");
      }
      if (epilogue != "" && epilogue != "\n" && epilogue != "\r\n" && !guess) {
	error("multipart message improperly terminated (%O%s)\n",
	      epilogue[..200],
	      sizeof(epilogue) > 201 ? "[...]" : "");
      }
      body_parts = map(parts, this_program, 0, 0, guess);
    }
    if((hdrs || parts) && !decoded_data) {
      decoded_data = (parts?
		      "This is a multi-part message in MIME format.\r\n":
		      "");
    }
  }

  protected string _sprintf(int c)
  {
    if (c == 'O')
      return sprintf("Message(%O)", disp_params);
    return (string)this;
  }
}

//! This function will attempt to reassemble a fragmented message from its
//! parts.
//!
//! The array @[collection] should contain @[MIME.Message] objects forming
//! a complete set of parts for a single fragmented message.
//! The order of the messages in the array is not important, but every part
//! must exist at least once.
//!
//! Should the function succeed in reconstructing the original message, a
//! new @[MIME.Message] object will be returned.
//!
//! If the function fails to reconstruct an original message, an integer
//! indicating the reason for the failure will be returned:
//! @int
//!   @value 0
//!     Returned if empty @[collection] is passed in, or one that contains
//!     messages which are not of type @tt{message/partial@}, or parts of
//!     different fragmented messages.
//!   @value 1..
//!     If more fragments are needed to reconstruct the entire message, the
//!     number of additional messages needed is returned.
//!   @value -1
//!     If more fragments are needed, but the function can't determine
//!     exactly how many.
//! @endint
//!
//! @note
//! Note that the returned message may in turn be a part of another,
//! larger, fragmented message.
//!
//! @seealso
//! @[MIME.Message->is_partial()]
//!
int|object reconstruct_partial(array(object) collection)
{
  int got = 0, maxgot = 0, top = sizeof(collection), total = 0;
  mapping(int:object) parts = ([ ]);
  string id;

  if(!top)
    return 0;

  if(!(id = (collection[0]->is_partial()||({0}))[0]))
    return 0;

  foreach(collection, object m) {
    array(int|string) p = m->is_partial();
    if(!(p && p[0] == id))
      return 0;
    if((!total || p[1]==p[2]) && p[2])
      total = p[2];
    if(p[1]>maxgot)
      maxgot = p[1];
    if(p[1]>0 && !parts[p[1]]) {
      parts[p[1]] = m;
      got++;
    }
  }

  if(!total)
    return -1;

  if(got == total && maxgot == total) {
    mapping(string:string) enclosing_headers = parts[1]->headers;

    object reconstructed =
      Message(`+(@map(sort(indices(parts)),
			    lambda(int i, mapping(int:object) parts){
	return parts[i]->getencoded();
      }, parts)));
    foreach(indices(reconstructed->headers), string h) {
      if(h != "message-id" && h != "encrypted" && h != "mime-version" &&
	 h != "subject" && (sizeof(h)<8 || h[0..7] != "content-"))
	m_delete(reconstructed->headers, h);
    }
    foreach(indices(enclosing_headers), string h) {
      if(h != "message-id" && h != "encrypted" && h != "mime-version" &&
	 h != "subject" && (sizeof(h)<8 || h[0..7] != "content-"))
	reconstructed->headers[h] = enclosing_headers[h];
    }
    return reconstructed;
  } else return (maxgot>total? -1 : total-got);
}
