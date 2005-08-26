// -*- Pike -*-
//
// RFC1521 functionality for Pike
//
// Marcus Comstedt 1996-1999
// $Id: module.pmod,v 1.7 2005/08/26 11:48:36 jonasw Exp $


//! RFC1521, the @b{Multipurpose Internet Mail Extensions@} memo, defines a
//! structure which is the base for all messages read and written by
//! modern mail and news programs.  It is also partly the base for the
//! HTTP protocol.  Just like RFC822, MIME declares that a message should
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
//! The Message class does not make any interpretation of the body data,
//! unless the content type is @tt{multipart@}.  A multipart message
//! contains several individual messages separated by boundary strings.
//! The @tt{create@} method of the Message class will divide a multipart
//! body on these boundaries, and then create individual Message objects
//! for each part.  These objects will be collected in the array
//! @tt{body_parts@} within the original Message object.  If any of the new
//! Message objects have a body of type multipart, the process is of course
//! repeated recursively.


#pike __REAL_VERSION__
inherit ___MIME;

//! This function will create a string that can be used as a separator string
//! for multipart messages.  The generated string is guaranteed not to appear
//! in @tt{base64@}, @tt{quoted-printable@}, or @tt{x-uue@} encoded data.
//! It is also unlikely to appear in normal text.  This function is used by
//! the cast method of the @tt{Message@} class if no boundary string is
//! specified.
//!
string generate_boundary( )
{
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
string decode( string data, string encoding )
{
  switch (lower_case( encoding || "binary" )) {
  case "base64":
    return decode_base64( data );
  case "quoted-printable":
    return decode_qp( data );
  case "x-uue":
  case "x-uuencode":
    return decode_uue( data );
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
string encode( string data, string encoding, void|string filename,
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
//! as specified by RFC1522.  The result is an array where the first element
//! is the raw text, and the second element the name of the character set.
//! If the input string is not an encoded word, the result is still an array,
//! but the char set element will be set to 0.
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

//! Create an @i{encoded word@} as specified in RFC1522 from an array
//! containing a raw text string and a char set name.
//!
//! The text will be transfer encoded according to the encoding argument,
//! which can be either @tt{"base64"@} or @tt{"quoted-printable"@}
//! (or either @tt{"b"@} or @tt{"q"@} for short).
//!
//! If either the second element of the array (the char set name), or
//! the encoding argument is 0, the raw text is returned as is.
//!
//! @seealso
//! @[MIME.decode_word()]
//!
string encode_word( string|array(string) word, string encoding )
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

static string remap(array(string) item)
{
  if (sizeof(item)>1 && item[1])
    return master()->resolv("Locale")["Charset"]
      ->decoder(item[1])->feed(item[0])->drain();
  else
    return item[0];
}

//! Separates a header value containing @i{text@} into units and calls
//! @[MIME.decode_word()] on them.  The result is an array where each element
//! is a result from @tt{decode_word()@}.
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
  return (sizeof(txt)? ({ ({ txt, 0 }) }) : ({ })) + res;
}

//! Like @[MIME.decode_words_text()], but the extracted strings are
//! also remapped from their specified character encoding into UNICODE,
//! and then pasted together.  The result is thus a string in the original
//! text format, without RFC1522 escapes, and with all characters in UNICODE
//! encoding.
//!
//! @seealso
//! @[MIME.decode_words_tokenized_remapped]
//!
string decode_words_text_remapped( string txt )
{
  return Array.map(decode_words_text(txt), remap)*"";
}

//! Tokenizes a header value just like @[MIME.tokenize()], but also converts
//! encoded words using @[MIME.decode_word()].  The result is an array where
//! each element is either an @tt{int@} representing a special character,
//! or an array as returned by @tt{decode_word()@} representing an atom or
//! a quoted string.
//!
//! @seealso
//! @[MIME.decode_words_tokenized_labled]
//! @[MIME.decode_words_tokenized_remapped]
//! @[MIME.decode_words_text]
//!
array(array(string)|int) decode_words_tokenized( string phrase )
{
  return Array.map(tokenize(phrase),
		   lambda(string|int item) {
		     return intp(item)? item : decode_word(item);
		   });
}

//! Like @[MIME.decode_words_tokenized()], but the extracted atoms are
//! also remapped from their specified character encoding into UNICODE.
//! The result is thus identical to that of @[MIME.tokenize()], but
//! without RFC1522 escapes, and with all characters in UNICODE encoding.
//!
//! @seealso
//! @[MIME.decode_words_tokenized_labled_remapped]
//! @[MIME.decode_words_text_remapped]
//!
array(string|int) decode_words_tokenized_remapped( string phrase )
{
  return Array.map(decode_words_tokenized(phrase),
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
//!     character as an @tt{int@}.
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
  decode_words_tokenized_labled( string phrase )
{
  return Array.map( tokenize_labled( phrase ),
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
//! without RFC1522 escapes, and with all characters in UNICODE encoding.
//!
array(array(string|int))
  decode_words_tokenized_labled_remapped(string phrase)
{
  return Array.map(decode_words_tokenized_labled(phrase),
		   lambda(array(string|int|array(array(string|int))) item) {
		     switch(item[0]) {
		     case "word":
		       return ({ "word", remap(item[1..]) });
		     case "comment":
		       return ({ "comment", Array.map(item[1], remap)*"" });
		     default:
		       return item;
		     }
		   });
}

//! The inverse of @[decode_words_text()], this function accepts
//! an array of strings or pairs of strings which will each be encoded
//! by @[encode_word()], after which they are all pasted together.
//!
//! @param encoding
//!   Either @tt{"base64"@} or @tt{"quoted-printable"@}
//!  (or either @tt{"b"@} or @tt{"q"@} for short).
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

//! The inverse of @[decode_words_tokenized()], this functions accepts
//! an array like the argument to @[quote()], but instead of simple strings
//! for atoms and quoted-strings, it will also accept pairs of strings to
//! be passed to @[encode_word()].
//!
//! @param encoding
//!   Either @tt{"base64"@} or @tt{"quoted-printable"@}
//!  (or either @tt{"b"@} or @tt{"q"@} for short).
//!
//! @seealso
//!   @[MIME.encode_words_quoted_labled()]
//!
string encode_words_quoted(array(array(string)|int) phrase, string encoding)
{
  return quote(Array.map(phrase, lambda(array(string)|int item) {
				   return intp(item)? item :
				     encode_word(item, encoding);
				 }));
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
//!   Either @tt{"base64"@} or @tt{"quoted-printable"@}
//!  (or either @tt{"b"@} or @tt{"q"@} for short).
//!
//! @seealso
//!   @[MIME.encode_words_quoted()]
//!
string encode_words_quoted_labled(array(array(string|int|array(string|array(string)))) phrase, string encoding)
{
  return
    quote_labled(Array.map(phrase,
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

//! Provide a reasonable default for the subtype field.
//!
//! Some pre-RFC1521 mailers provide only a type and no subtype in the
//! Content-Type header field.  This function can be used to obtain a
//! reasonable default subtype given the type of a message.  (This is done
//! automatically by the @[MIME.Message] class.)
//!
//! Currently, the function uses the following guesses:
//! @string
//!   @value "text"
//!     @tt{"plain"@}
//!   @value "message"
//!     @tt{"rfc822"@}
//!   @value "multipart"
//!     @tt{"mixed"@}
//! @endstring
//!
string guess_subtype( string type )
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
//! Headers that occurr multiple times will have their contents NUL separated,
//! unless @[use_multiple] has been specified, in which case the contents will
//! be arrays.
//!
array(mapping(string:string|array(string))|string) 
  parse_headers(string message, void|int(1..1) use_multiple)
{
  string head, body, header, hname, hcontents;
  if (has_prefix(message, "\r\n") || has_prefix(message, "\n")) {
    // No headers.
    return ({ ([]), message[1 + (message[0] == '\r')..] });
  } else {
    int mesgsep1 = search(message, "\r\n\r\n");
    int mesgsep2 = search(message, "\n\n");
    int mesgsep = (mesgsep1<0? mesgsep2 :
		   (mesgsep2<0? mesgsep1 :
		    (mesgsep1<mesgsep2? mesgsep1 : mesgsep2)));
    if (mesgsep<0) {
      // No body.
      head = message;
      body = "";
    } else if (mesgsep) {
      head = (mesgsep>0? message[..mesgsep-1]:"");
      body = message[mesgsep+(message[mesgsep]=='\r'? 4:2)..];
    }
  }
  mapping(string:string|array) headers = ([ ]);
  foreach( replace(head, ({"\r", "\n ", "\n\t"}),
		   ({"", " ", " "}))/"\n", header ) 
  {
    if(4==sscanf(header, "%[!-9;-~]%*[ \t]:%*[ \t]%s", hname, hcontents))
    {
      if (use_multiple)
	headers[hname=lower_case(hname)]
	  = (headers[hname]||({}))+({hcontents});
      else
	if(headers[lower_case(hname)])
	  headers[lower_case(hname)] += "\0"+hcontents;
	else
	  headers[lower_case(hname)] = hcontents;
    }
  }
  return ({ headers, body });
}


//! This class is used to hold a decoded MIME message.
class Message {

  import Array;

  string encoded_data;
  string decoded_data;

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
  mapping(string:string) headers;

  //! If the message is of type @tt{multipart@}, this is an array
  //! containing one Message object for each part of the message.
  //! If the message is not a multipart, this field is @tt{0@} (zero).
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
  //! is @tt{"us-ascii"@}.
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
  //! is @tt{"text"@}.
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
  //! is @tt{"plain"@}.
  //!
  //! @seealso
  //! @[type], @[params]
  //!
  string subtype;

  //! The contents of the @tt{Content-Transfer-Encoding@} header.
  //!
  //! If no @tt{Content-Transfer-Encoding@} header is given, this field
  //! is @tt{0@} (zero).
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
  //! is @tt{0@}.
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
  array(string|int) is_partial( )
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
  //! @[getdata()]
  //!
  void setdata( string data )
  {
    if (data != decoded_data) {
      decoded_data = data;
      encoded_data = 0;
    }
  }

  //! This method returns the raw data of the message body entity.
  //!
  //! The @[type] and @[subtype] attributes indicate how this data should
  //! be interpreted.
  //!
  //! @seealso
  //! @[getencoded()]
  //!
  string getdata( )
  {
    if (encoded_data && !decoded_data)
      decoded_data = decode( encoded_data, transfer_encoding );
    return decoded_data;
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
      encoded_data = encode( decoded_data, transfer_encoding, get_filename() );
    return encoded_data;
  }

  //! Select a new transfer encoding for this message.
  //!
  //! The @tt{Content-Transfer-Encoding@} header will be modified accordingly,
  //! and subsequent calls to @tt{getencoded@} will produce data encoded using
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
  //! This is equivalent of calling @code{setparam("charset", @[charset])@}.
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
  //! This is equivalent of calling @code{setparam("boundary", @[boundary])@}.
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
  string cast( string dest_type )
  {
    string data;
    object body_part;
    
    if (dest_type != "string")
      error( "Can't cast Message to %s.\n", dest_type);
    
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
    
    headers["content-length"] = ""+strlen(data);

    return map( indices(headers),
		lambda(string hname){
		  return map(headers[hname]/"\0",
			     lambda(string header,string hname) {
			       return hname+": "+header;
			     },
			     replace(map(hname/"-",
					 String.capitalize)*"-",
				     "Mime","MIME"))*"\r\n";
		} )*"\r\n" + "\r\n\r\n" + data;
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
  void create(void | string message,
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
    if (hdrs || parts) {
      string|array(string) hname;
      decoded_data = message;
      if (hdrs)
	foreach( indices(hdrs), hname )
	  headers[lower_case(hname)] = hdrs[hname];
      body_parts = parts;
    } else if (message) {
      array(mapping(string:string)|string) h = parse_headers(message);
      headers = h[0];
      encoded_data = h[1];
    }
    if (headers["content-type"]) {
      array(array(string|int)) arr =
	tokenize(headers["content-type"]) / ({';'});
      array(string|int) p;
      if(sizeof(arr[0])!=3 || arr[0][1]!='/' ||
	 !stringp(arr[0][0]) || !stringp(arr[0][2]))
	if(sizeof(arr[0])==1 && stringp(arr[0][0]) &&
	   (subtype = guess_subtype(lower_case(type = arr[0][0]))))
	  arr = ({ ({ type, '/', subtype }) }) + arr[1..];
	else if(!guess)
	  error("invalid Content-Type %O\n", headers["content-type"]);
	else
	  arr = ({ ({ "text", '/', "plain" }) }) + arr[1..];
      type = lower_case(arr[0][0]);
      subtype = lower_case(arr[0][2]);
      foreach( arr[1..], p )
	if(sizeof(p)) {
	  if(sizeof(p)<3 || p[1]!='=' || !stringp(p[0]))
	    if(guess)
	      continue; // just ignore the broken data we failed to parse
	    else
	      error("invalid parameter %O in Content-Type %O (%O)\n",
		    p[0], headers["content-type"], guess);
	  params[ lower_case(p[0]) ] = p[2..]*"";
	}
      charset = lower_case(params["charset"] || charset);
      boundary = params["boundary"];
    }
    if (headers["content-disposition"]) {
      array(array(string|int)) arr =
	tokenize(headers["content-disposition"]) / ({';'});
      array(string|int) p;
      if(sizeof(arr[0])!=1 || !stringp(arr[0][0]))
      {
	if(!guess)
	  error("invalid Content-Disposition in message\n");
      } else
      {
	disposition = lower_case(arr[0][0]);
	foreach( arr[1..], p )
	  if(sizeof(p))
	  {
	    if(sizeof(p)<3 || p[1]!='=' || !stringp(p[0]))
	      if(guess)
		break;
	      else
		error("invalid parameter %O in Content-Disposition\n", p[0]);
	    disp_params[ lower_case(p[0]) ] = p[2..]*"";
	  }
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
      array(string) parts = ("\n"+getdata())/("\n--"+boundary);
      if (sizeof(parts)<2)
	if (guess)
	  parts = ({parts[0], "--"});
	else
	  error("boundary missing from multipart-body\n");
      string epilogue = parts[-1];
      if ((sscanf(epilogue, "--%*[ \t]%s", epilogue)<2 ||
	   (epilogue != "" && epilogue[0] != '\n' &&
	    epilogue[0..1] != "\r\n")) && !guess) {
	error("multipart message improperly terminated (%O%s)\n",
	      epilogue[..200],
	      sizeof(epilogue) > 201 ? "[...]" : "");
      }
      encoded_data = 0;
      decoded_data = parts[0][1..];
      if(sizeof(decoded_data) && decoded_data[-1]=='\r')
	decoded_data = decoded_data[..sizeof(decoded_data)-2];
      body_parts = map(parts[1..sizeof(parts)-2], lambda(string part){
	if(sizeof(part) && part[-1]=='\r')
	  part = part[..sizeof(part)-2];
	sscanf(part, "%*[ \t]%s", part);
	if(has_prefix(part, "\r\n"))
	  part = part[2..];
	else if(has_prefix(part, "\n"))
	  part = part[1..];
	else if(!guess)
	  error("newline missing after multipart boundary\n");
	return object_program(this_object())(part, 0, 0, guess);
      });
    }
    if((hdrs || parts) && !decoded_data) {
      decoded_data = (parts?
		      "This is a multi-part message in MIME format.\r\n":
		      "");
    }
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
      Message(`+(@Array.map(sort(indices(parts)),
			    lambda(int i, mapping(int:object) parts){
	return parts[i]->getencoded();
      }, parts)));
    foreach(indices(reconstructed->headers), string h) {
      if(h != "message-id" && h != "encrypted" && h != "mime-version" &&
	 h != "subject" && (strlen(h)<8 || h[0..7] != "content-"))
	m_delete(reconstructed->headers, h);
    }
    foreach(indices(enclosing_headers), string h) {
      if(h != "message-id" && h != "encrypted" && h != "mime-version" &&
	 h != "subject" && (strlen(h)<8 || h[0..7] != "content-"))
	reconstructed->headers[h] = enclosing_headers[h];
    }
    return reconstructed;
  } else return (maxgot>total? -1 : total-got);
}
