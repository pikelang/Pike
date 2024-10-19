//! Encodes various asn.1 objects according to the Distinguished
//! Encoding Rules (DER)

#charset iso-8859-1
#pike 8.0
#pragma no_deprecation_warnings

#if 0
#define WERROR werror
#else
#define WERROR(x ...)
#endif

// Helper functions

//! Combines tag and class to a single integer, for internal uses.
//!
//! @param cls
//!   ASN1 type class (0..3).
//! @param tag
//!   ASN1 type tag (1..).
//! @returns
//!   The combined tag.
//! @seealso
//!  @[extract_tag], @[extract_cls]
int make_combined_tag(int cls, int tag)
{ return tag << 2 | cls; }

//! Extract ASN1 type tag from a combined tag.
//! @seealso
//!  @[make_combined_tag]
int extract_tag(int i) { return i >> 2; }

//! Extract ASN1 type class from a combined tag.
//! @seealso
//!  @[make_combined_tag]
int(0..3) extract_cls(int i) { return [int(0..3)](i & 3); }


// Class definitions

//! Generic, abstract base class for ASN1 data types.
class Object
{
  int cls = 0;
  int tag = 0;
  constant constructed = 0;

  constant type_name = "";

  //! Return the DER payload.
  string(8bit) get_der_content()
  {
    return "";
  }

  string(0..255) der_encode()
  {
    return build_der(get_der_content());
  }

  //! Get the class of this object.
  //!
  //! @returns
  //!   The class of this object.
  int get_cls() { return cls; }

  //! Get the tag for this object.
  //!
  //! @returns
  //!   The tag for this object.
  int get_tag() { return tag; }

  //! Get the combined tag (tag + class) for this object.
  //!
  //! @returns
  //!   the combined tag header
  int get_combined_tag() {
    return make_combined_tag(get_cls(), get_tag());
  }

  string(0..255) der;

  // Should be overridden by subclasses
  this_program decode_primitive(string contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object) decoder,
				mapping(int:program(Object)) types);
  void begin_decode_constructed(string raw);
  void decode_constructed_element(int i, object e);
  this_program end_decode_constructed(int length);

  mapping(int:program(Object)) element_types(int i,
      mapping(int:program(Object)) types) {
    return types; i;
  }
  this_program init(mixed ... args) { return this; args; }

  protected string(0..255) to_base_128(int n)
  {
    string(0..255) ret = [string(0..127)]sprintf("%c", n&0x7f);
    n >>= 7;
    while( n )
    {
      ret = [string(0..255)]sprintf("%c", (n&0x7f) | 0x80) + ret;
      n >>= 7;
    }
    return ret;
  }

  protected string(0..255) encode_tag()
  {
    int tag = get_tag();
    int cls = get_cls();
    if (tag < 31)
      return [string(0..255)]sprintf("%c",
				     (cls << 6) | (constructed << 5) | tag);

    return [string(0..255)]sprintf("%c%s",
				   (cls << 6) | (constructed << 5) | 0x1f,
				   to_base_128(tag) );
  }

  protected string(0..255) encode_length(int|object len)
  {
    if (len < 0x80)
      return [string(0..255)]sprintf("%c", len);
    string s = Gmp.mpz(len)->digits(256);
    if (sizeof(s) >= 0x80)
      error("Max length exceeded.\n" );
    return [string(0..255)]sprintf("%c%s", sizeof(s) | 0x80, s);
  }

  protected string(0..255) build_der(string(0..255) contents)
  {
    string(0..255) data =
      encode_tag() + encode_length(sizeof(contents)) + contents;
    WERROR("build_der: %O\n", data);
    return data;
  }

  string(0..255) record_der(string(0..255) s)
  {
    return (der = s);
  }

  void record_der_contents(string(0..255) s)
  {
    der = build_der(s);
  }


  //! Get the DER encoded version of this object.
  //!
  //! @returns
  //!   DER encoded representation of this object.
  string(0..255) get_der() {
    return der || (der = der_encode());
  }

  protected void create(mixed ...args) {
    WERROR("asn1_object[%s]->create\n", type_name);
    if (sizeof(args))
      init(@args);
  }
}

//! Compound object primitive
class Compound
{
  inherit Object;

  constant constructed = 1;

  //! Contents of compound object.
  array(Object) elements = ({ });

  this_program init(array args) {
    WERROR("asn1_compound[%s]->init(%O)\n", type_name, args);
    foreach(args, mixed o)
      if (!objectp(o))
	error( "Non-object argument %O\n", o );
    elements = [array(Object)]args;
    WERROR("asn1_compound: %O\n", elements);
    return this;
  }

  void begin_decode_constructed(string(0..255) raw) {
    WERROR("asn1_compound[%s]->begin_decode_constructed\n", type_name);
    record_der_contents(raw);
  }

  void decode_constructed_element(int i, object e) {
    WERROR("asn1_compound[%s]->decode_constructed_element(%O)\n",
	   type_name, e);
    if (i != sizeof(elements))
      error("Unexpected index!\n");
    elements += ({ e });
  }

  this_program end_decode_constructed(int length) {
    if (length != sizeof(elements))
      error("Invalid length!\n");
    return this;
  }

  protected mixed `[](mixed index)
  {
    if( intp(index) )
      return elements[index];
    return ::`[]([string]index);
  }

  protected int _sizeof()
  {
    return sizeof(elements);
  }

  protected string _sprintf(int t,mapping(string:int)|void params) {
    if (params) ++params->indent; else params=([]);
    return t=='O' && sprintf("%O(%*O)", this_program, params, elements);
  }

  string debug_string() {
    WERROR("asn1_compound[%s]->debug_string(), elements = %O\n",
	   type_name, elements);
    return _sprintf('O');
  }
}

//! string object primitive
class String
{
  inherit Object;

  //! value of object
  string value;

  this_program init(string(0..255) s) {
    value = s;
    return this;
  }

  string(0..255) get_der_content() {
    return [string(0..255)]value;
  }

  this_program decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    record_der_contents(contents);
    value = contents;
    return this;
  }

  void begin_decode_constructed(string raw)
  {
    value = "";
  }

  void decode_constructed_element(int i, object(this_program) e)
  {
    value += e->value;
  }

  this_program end_decode_constructed(int length)
  {
    return this;
  }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }

  string debug_string() {
    WERROR("asn1_string[%s]->debug_string(), value = %O\n", type_name, value);
    return _sprintf('O');
  }
}

//! boolean object
class Boolean
{
  inherit Object;
  int tag = 1;
  constant type_name = "BOOLEAN";

  //! value of object
  int value;

  this_program init(int x) {
    value = x;
    return this;
  }

  // While every non-zero value is true, the canonical true is 0xff.
  string(0..255) get_der_content()
  {
    return value ? "\377" : "\0";
  }

  this_program decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    record_der_contents(contents);
    if( contents=="" ) error("Illegal boolean value.\n");
    value = (contents != "\0");
    return this;
  }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s)", this_program, (value?"TRUE":"FALSE"));
  }

  string debug_string() {
    return value ? "TRUE" : "FALSE";
  }
}

//! Integer object
//! All integers are represented as bignums, for simplicity
class Integer
{
  inherit Object;
  int tag = 2;
  constant type_name = "INTEGER";

  //! value of object
  Gmp.mpz value;

  this_object init(int|object n) {
    value = Gmp.mpz(n);
    WERROR("i = %s\n", value->digits());
    return this;
  }

  string(0..255) get_der_content()
  {
    string(0..255) s;

    if (value < 0)
    {
      Gmp.mpz n = [object(Gmp.mpz)](value +
				   pow(256, ([object(Gmp.mpz)](- value))->
				       size(256)));
      s = n->digits(256);
      if (!(s[0] & 0x80))
	s = "\377" + s;
    } else {
      s = value->digits(256);
      if (s[0] & 0x80)
	s = "\0" + s;
    }

    return s;
  }

  this_object decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    record_der_contents(contents);
    value = Gmp.mpz(contents, 256);
    if (contents[0] & 0x80)  /* Negative */
      value -= pow(256, sizeof(contents));
    return this;
  }

  protected string _sprintf(int t) {
    if(t!='O') return UNDEFINED;
    if(!value) return sprintf("%O(0)", this_program);
    return sprintf("%O(%d %s)", this_program,
                   value->size(), value->digits());
  }

  string debug_string() {
    return sprintf("INTEGER (%d) %s", value->size(), value->digits());
  }
}

//! Enumerated object
class Enumerated
{
  inherit Integer;
  int tag = 10;
  constant type_name = "ENUMERATED";
}

class Real
{
  inherit Object;
  int tag = 9;
  constant type_name = "REAL";

  float value;

  string(0..255) get_der_content()
  {
    string v = sprintf("%F", value);
    switch(v)
    {
    case "\0\0\0\0" : return ""; // 0.0
    case "\200\0\0\0" : return "\x43"; // -0.0
    case "\177\200\0\0" : return "\x40"; // inf
    case "\377\200\0\0" : return "\x41"; // -inf

    case "\177\300\0\0" : // nan
    case "\377\300\0\0" : // -nan
      return "\x42";
    }

    error("Encoding Real values not supported.\n");
  }

  this_object decode_primitive(string(0..255) contents,
                               function(ADT.struct,
                                        mapping(int:program(Object)):
                                        Object) decoder,
                               mapping(int:program(Object))|void types) {
    if( contents=="" ) { value = 0.0; return this; }
    int first = contents[0];
    switch( first )
    {
      // SpecialRealValues
    case 0b01000000: value = Math.inf; return this;
    case 0b01000001: value = -Math.inf; return this;
    case 0b01000010: value = Math.nan; return this;
    case 0b01000011: value = -0.0; return this;

      // ISO 6093
    case 0b00000001: error("ISO 6093 NR1 not supported.\n");
    case 0b00000010: error("ISO 6093 NR2 not supported.\n");
    case 0b00000011: error("ISO 6093 NR3 not supported.\n");
    }
    switch( first & 0xc0 )
    {
    case 0x00:
      error("Unknown real coding.\n");
    case 0x40:
      error("Unknown SpecialRealValues code.\n");
    }

    int neg = first & 0b01000000;
    int base;
    switch(first & 0b00110000)
    {
    case 0b00000000: base = 2; break;
    case 0b00010000: base = 8; break;
    case 0b00100000: base = 16; break;
    default: error("Unknown real base.\n");
    }

    int scaling = (first & 0b00001100) >> 2;

    int exp;
    int num;
    switch(first & 0b00000011)
    {
    case 0b00:
      sscanf(contents, "%*1c%+1c%+"+(sizeof(contents)-2)+"c", exp, num);
      break;
    case 0b01:
      sscanf(contents, "%*1c%+2c%+"+(sizeof(contents)-3)+"c", exp, num);
      break;
    case 0b10:
      sscanf(contents, "%*1c%+3c%+"+(sizeof(contents)-4)+"c", exp, num);
      break;
    case 0b11:
      int e_size = contents[1];
      int n_size = sizeof(contents)-2-e_size;
      sscanf(contents, "%*2c%+"+e_size+"c%+"+n_size+"c", exp, num);
      break;
    }

    int mantissa = num * (1<<scaling);
    if( neg ) mantissa = -mantissa;
    value = mantissa * pow((float)base, exp);

    return this;
  }

  string debug_string()
  {
    return sprintf("REAL %O", value);
  }
}

//! Bit string object
class BitString
{
  inherit Object;
  int tag = 3;
  constant type_name = "BIT STRING";

  //! value of object
  string(0..255) value;

  int unused = 0;

  this_program init(string(0..255) s)
  {
    value = s;
    return this;
  }

  string(0..255) get_der_content()
  {
    return [string(0..255)]sprintf("%c%s", unused, value);
  }

  //! Set the bitstring value as a string with @expr{"1"@} and
  //! @expr{"0"@}.
  this_program set_from_ascii(string(0..255) s)
  {
    array v = array_sscanf(s, "%8b"*(sizeof(s)/8)+"%b");
    v[-1] = v[-1]<<((-sizeof(s))%8);
    value = (string(0..255))v;
    set_length(sizeof(s));
    return this;
  }

  //! Sets the length of the bit string to @[len] number of bits. Will
  //! only work correctly on strings longer than @[len] bits.
  this_program set_length(int len) {
    if (len)
    {
      value = value[..(len + 7)/8];
      unused = (- len) % 8;
      value[-1] &= 256-(1<<unused);
    } else {
      unused = 0;
      value = "";
    }
    return this;
  }

  this_program decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    record_der_contents(contents);
    if (!sizeof(contents))
      return 0;
    unused = contents[0];

    if (unused >= 8)
      return 0;
    value = contents[1..];
    return this;
  }

  void begin_decode_constructed(string raw)
  {
    unused = 0;
    value = "";
  }

  void decode_constructed_element(int i, object(this_program) e)
  {
    if( unused ) error("Adding to a non-aligned bit stream.\n");
    value += e->value;
    unused = e->unused;
  }

  this_program end_decode_constructed(int length)
  {
    return this;
  }

  protected string _sprintf(int t) {
    if(t!='O') return UNDEFINED;
    if(!value) return sprintf("%O(0)", this_program);
    int size = sizeof(value)*8-unused;
    if(!unused) return sprintf("%O(%d %O)", this_program, size, value);
    return sprintf("%O(%d %0"+size+"s)", this_program, size,
                   ([object(Gmp.mpz)](Gmp.mpz(value, 256) >> unused))
                   ->digits(2));
  }

  string debug_string() {
    return sprintf("BIT STRING (%d) %s",
		   sizeof(value) * 8 - unused,
		   ([object(Gmp.mpz)](Gmp.mpz(value, 256) >> unused))
		   ->digits(2));
  }
}

//! Octet string object
class OctetString
{
  inherit String;
  int tag = 4;
  constant type_name = "OCTET STRING";
}

//! Null object
class Null
{
  inherit Object;
  int tag = 5;
  constant type_name = "NULL";

  string(0..255) get_der_content() { return ""; }

  this_program decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    record_der_contents(contents);
    return !sizeof(contents) && this;
  }

  string debug_string() { return "NULL"; }
}

//! Object identifier object
class Identifier
{
  inherit Object;
  int tag = 6;
  constant type_name = "OBJECT IDENTIFIER";

  //! value of object
  array(int) id;

  this_program init(int ... args) {
    if ( (sizeof(args) < 2)
	 || (args[0] > 2)
	 || (args[1] >= ( (args[0] < 2) ? 40 : 176) ))
      error( "Invalid object identifier.\n" );
    id = args;
    return this;
  }

  mixed _encode() { return id; }
  void _decode(array(int) data) { id=data; }

  //! Returns a new @[Identifier] object with @[args] appended to the
  //! ID path.
  this_program append(int ... args) {
    return this_program(@id, @args);
  }

  string(0..255) get_der_content()
  {
    return [string(0..255)]sprintf("%s%@s",
				   to_base_128(40 * id[0] + id[1]),
				   map(id[2..], to_base_128));
  }

  this_program decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    record_der_contents(contents);

    if (contents[0] < 120)
      id = ({ contents[0] / 40, contents[0] % 40 });
    else
      id = ({ 2, contents[0] - 80 });
    int index = 1;
    while(index < sizeof(contents))
    {
      int element = 0;
      do {
	element = element << 7 | (contents[index] & 0x7f);
      } while(contents[index++] & 0x80);
      id += ({ element });
    }
    return this;
  }

  protected string _sprintf(int t) {
    if(t!='O') return UNDEFINED;
    if(!id) return sprintf("%O(0)", this_program);
    return sprintf("%O(%s)", this_program, (array(string))id*".");
  }

  string debug_string() {
    return "IDENTIFIER " + (array(string)) id * ".";
  }

  protected int __hash()
  {
    return hash(get_der());
  }

  protected int(0..1) `==(mixed other) {
    return (objectp(other) &&
	    (this_program == object_program(other)) &&
	    equal(id, ([object(Identifier)]other)->id));
  }

  protected int(0..1) `<(mixed other) {
    if( !objectp(other) ||
        (this_program != object_program(other)) )
      return 0;
    array oid = ([object(Identifier)]other)->id;
    for( int i; i<min(sizeof(id),sizeof(oid)); i++ )
    {
      if( id[i] < oid[i] ) return 1;
      if( id[i] > oid[i] ) return 0;
    }
    return sizeof(id) < sizeof(oid);
  }
}

//! Checks if a Pike string can be encoded with UTF8. That is
//! always the case...
int(1..1) asn1_utf8_valid (string s)
{
  return 1;
}

//! UTF8 string object
//!
//! Character set: ISO/IEC 10646-1 (compatible with Unicode).
//!
//! Variable width encoding, see @rfc{2279@}.
class UTF8String
{
  inherit String;
  int tag = 12;
  constant type_name = "UTF8String";

  string(0..255) get_der_content()
  {
    return string_to_utf8(value);
  }

  this_program decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    der = contents;
    if (catch {
      value = utf8_to_string(contents);
    })
      return 0;

    return this;
  }
}

//! Sequence object
class Sequence
{
  inherit Compound;
  int tag = 16;
  constant type_name = "SEQUENCE";

  string(0..255) get_der_content()
  {
    WERROR("ASN1.Sequence: elements = '%O\n", elements);
    array(string) a = elements->get_der();
    WERROR("ASN1.Sequence: der_encode(elements) = '%O\n", a);
    return [string(0..255)]`+("", @ a);
  }

  this_program decode_primitive(string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object) decoder,
				mapping(int:program(Object)) types) {
    der = contents;
    elements = ({});
    ADT.struct struct = ADT.struct(contents);
    while (!struct->is_empty()) {
      elements += ({ decoder(struct, types) });
    }
    return this;
  }
}

//! Set object
class Set
{
  inherit Compound;
  int tag = 17;
  constant type_name = "SET";

  int(-1..1) compare_octet_strings(string r, string s)
  {
    if (r == s) return 0;

    for(int i = 0;; i++) {
      if (i == sizeof(r))
	return (i = sizeof(s)) ? 0 : -1;
      if (i == sizeof(s))
	return 1;
      if (r[i] < s[i])
	return -1;
      else if (r[i] > s[i])
	return 1;
    }
  }

  string get_der_content() {
    WERROR("asn1_set->der: elements = '%O\n", elements);
    array(string) a = elements->get_der();
    WERROR("asn1_set->der: der_encode(elements) = '%O\n", a);
    return [string(0..255)]
      `+("", @[array(string)]
	 Array.sort_array(a, compare_octet_strings));
  }
}

Regexp asn1_printable_invalid_chars = Regexp("([^-A-Za-z0-9 '()+,./:=?])");

//! Checks if a Pike string can be encoded as a @[PrintableString].
int(0..1) asn1_printable_valid (string s) {
  if (global.String.width(s)!=8) return 0;
  return !asn1_printable_invalid_chars->match(s);
}

//! PrintableString object
class PrintableString
{
  inherit String;
  int tag = 19;
  constant type_name = "PrintableString";
}

Regexp asn1_teletex_invalid_chars = Regexp ("([\\\\{}\240����\255�])");

//!
int(0..1) asn1_teletex_valid (string s)
{
  if (global.String.width(s)!=8)
    // T.61 encoding of wide strings not implemented.
    return 0;
  return !asn1_teletex_invalid_chars->match (s);
}

//! TeletexString object
//!
//! Avoid this one; it seems to be common that this type is used to
//! label strings encoded with the ISO 8859-1 character set (use
//! asn1_broken_teletex_string for that). From
//! http://www.mindspring.com/~asn1/nlsasn.htm:
//!
//! /.../ Types GeneralString, VideotexString, TeletexString
//! (T61String), and GraphicString exist in earlier versions
//! [pre-1994] of ASN.1. They are considered difficult to use
//! correctly by applications providing national language support.
//! Varying degrees of application support for T61String values seems
//! to be most common in older applications. Correct support is made
//! more difficult, as character values available in type T61String
//! have changed with the addition of new register entries from the
//! 1984 through 1997 versions.
//!
//! This implementation is based on the description of T.61 and T.51
//! in "Some functions for representing T.61 characters from the
//! X.500 Directory Service in ISO 8859 terminals (Version 0.2. July
//! 1994.)" by Enrique Silvestre Mora (mora@@si.uji.es), Universitat
//! Jaume I, Spain, found in the package
//! ftp://pereiii.uji.es/pub/uji-ftp/unix/ldap/iso-t61.translation.tar.Z
//!
//! The translation is only complete for 8-bit latin 1 strings. It
//! encodes strictly to T.61, but decodes from the superset T.51.
//!
//! @note
//!   CCITT Recommendation T.61 is also known as ISO-IR 103:1985
//!   (graphic characters) and ISO-IR 106:1985 and ISO-IR 107:1985
//!   (control characters).
//!
//! @seealso
//!   @[Charset]
class TeletexString
{
  inherit String;
  int tag = 20;
  constant type_name = "TeletexString";	// Alias: T61String

#define ENC_ERR(char) char
#define DEC_ERR(str) str
#define DEC_COMB_MARK "\300"

#define GR(char) "\301" char	/* Combining grave accent */
#define AC(char) "\302" char	/* Combining acute accent */
#define CI(char) "\303" char	/* Combining circumflex accent */
#define TI(char) "\304" char	/* Combining tilde */
#define MA(char) "\305" char	/* Combining macron */
#define BR(char) "\306" char	/* Combining breve */
#define DA(char) "\307" char	/* Combining dot above */
#define DI(char) "\310" char	/* Combining diaeresis */
#define RA(char) "\312" char	/* Combining ring above */
#define CE(char) "\313" char	/* Combining cedilla */
#define UN(char) "\314" char	/* Combining underscore (note 6) */
#define DO(char) "\315" char	/* Combining double acute accent */
#define OG(char) "\316" char	/* Combining ogonek */
#define CA(char) "\317" char	/* Combining caron */

  constant encode_from = ({
    /*"#", "$",*/ "�",		// Note 3
    "\\", "{", "}",		// Note 7
    "\240",			// No-break space (note 7)
    "�",			// Multiplication sign
    "�",			// Division sign
    "�",			// Superscript one
    "�",			// Registered sign (note 7)
    "�",			// Copyright sign (note 7)
    "�",			// Not sign (note 7)
    "�",			// Broken bar (note 7)
    "�",			// Latin capital ligature ae
    "�",			// Feminine ordinal indicator
    "�",			// Latin capital letter o with stroke
    "�",			// Masculine ordinal indicator
    "�",			// Latin capital letter thorn
    "�",			// Latin small ligature ae
    "�",			// Latin small letter eth
    "�",			// Latin small letter o with stroke
    "�",			// Latin small letter sharp s
    "�",			// Latin small letter thorn
    "\255",			// Soft hyphen (note 7)
    "�",			// Latin capital letter eth (no equivalent)
    // Combinations
    "^", "`", "~",		// Note 4
    "�", "�", "�", "�",
    "�", "�", "�", "�", "�", "�",
    "�", "�", "�", "�", "�", "�",
    "�",
    "�",
    "�", "�", "�", "�",
    "�", "�", "�", "�",
    "�", "�", "�", "�",
    "�", "�", "�", "�",
    "�",
    "�",
    "�", "�", "�", "�", "�",
    "�", "�", "�", "�", "�",
    "�", "�", "�", "�",
    "�", "�", "�", "�",
    "�",
    "�",
  });

  constant encode_to = ({
    /*"#", "$",*/ "\250",	// Note 3
    ENC_ERR("\\"), ENC_ERR("{"), ENC_ERR("}"), // Note 7
    ENC_ERR("\240"),		// No-break space (note 7)
    "\264",			// Multiplication sign
    "\270",			// Division sign
    "\321",			// Superscript one
    ENC_ERR("�"),		// Registered sign (note 7)
    ENC_ERR("�"),		// Copyright sign (note 7)
    ENC_ERR("�"),		// Not sign (note 7)
    ENC_ERR("�"),		// Broken bar (note 7)
    "\341",			// Latin capital ligature ae
    "\343",			// Feminine ordinal indicator
    "\351",			// Latin capital letter o with stroke
    "\353",			// Masculine ordinal indicator
    "\354",			// Latin capital letter thorn
    "\361",			// Latin small ligature ae
    "\363",			// Latin small letter eth
    "\371",			// Latin small letter o with stroke
    "\373",			// Latin small letter sharp s
    "\374",			// Latin small letter thorn
    ENC_ERR("\255"),		// Soft hyphen (note 7)
    ENC_ERR("�"),		// Latin capital letter eth (no equivalent)
    // Combinations
    CI(" "), GR(" "), TI(" "),	// Note 4
    AC(" "), DI(" "), MA(" "), CE(" "),
    GR("A"), AC("A"), CI("A"), TI("A"), DI("A"), RA("A"),
    GR("a"), AC("a"), CI("a"), TI("a"), DI("a"), RA("a"),
    CE("C"),
    CE("c"),
    GR("E"), AC("E"), CI("E"), DI("E"),
    GR("e"), AC("e"), CI("e"), DI("e"),
    GR("I"), AC("I"), CI("I"), DI("I"),
    GR("i"), AC("i"), CI("i"), DI("i"),
    TI("N"),
    TI("n"),
    GR("O"), AC("O"), CI("O"), TI("O"), DI("O"),
    GR("o"), AC("o"), CI("o"), TI("o"), DI("o"),
    GR("U"), AC("U"), CI("U"), DI("U"),
    GR("u"), AC("u"), CI("u"), DI("u"),
    GR("Y"),
    GR("y"),
  });

  constant decode_from = ({
    /*"#", "$",*/ "\244", "\246", "\250", // Note 3
    /*"^", "`", "~",*/		// Note 4
    "\251",			// Left single quotation mark (note 7)
    "\252",			// Left double quotation mark (note 7)
    "\254",			// Leftwards arrow (note 7)
    "\255",			// Upwards arrow (note 7)
    "\256",			// Rightwards arrow (note 7)
    "\257",			// Downwards arrow (note 7)
    "\264",			// Multiplication sign
    "\270",			// Division sign
    "\271",			// Right single quotation mark (note 7)
    "\272",			// Right double quotation mark (note 7)
    "\300",			// Note 5
    GR(""),			// Combining grave accent
    AC(""),			// Combining acute accent
    CI(""),			// Combining circumflex accent
    TI(""),			// Combining tilde
    MA(""),			// Combining macron
    BR(""),			// Combining breve
    DA(""),			// Combining dot above
    DI(""),			// Combining diaeresis
    "\311",			// Note 5
    RA(""),			// Combining ring above
    CE(""),			// Combining cedilla
    UN(""),			// Combining underscore (note 6)
    DO(""),			// Combining double acute accent
    OG(""),			// Combining ogonek
    CA(""),			// Combining caron
    "\320",			// Em dash (note 7)
    "\321",			// Superscript one
    "\322",			// Registered sign (note 7)
    "\323",			// Copyright sign (note 7)
    "\324",			// Trade mark sign (note 7)
    "\325",			// Eighth note (note 7)
    "\326",			// Not sign (note 7)
    "\327",			// Broken bar (note 7)
    "\330", "\331", "\332", "\333", // Note 2
    "\334",			// Vulgar fraction one eighth (note 7)
    "\335",			// Vulgar fraction three eighths (note 7)
    "\336",			// Vulgar fraction five eighths (note 7)
    "\337",			// Vulgar fraction seven eighths (note 7)
    "\340",			// Ohm sign
    "\341",			// Latin capital ligature ae
    "\342",			// Latin capital letter d with stroke
    "\343",			// Feminine ordinal indicator
    "\344",			// Latin capital letter h with stroke
    "\345",			// Note 2
    "\346",			// Latin capital ligature ij
    "\347",			// Latin capital letter l with middle dot
    "\350",			// Latin capital letter l with stroke
    "\351",			// Latin capital letter o with stroke
    "\352",			// Latin capital ligature oe
    "\353",			// Masculine ordinal indicator
    "\354",			// Latin capital letter thorn
    "\355",			// Latin capital letter t with stroke
    "\356",			// Latin capital letter eng
    "\357",			// Latin small letter n preceded by apostrophe
    "\360",			// Latin small letter kra
    "\361",			// Latin small ligature ae
    "\362",			// Latin small letter d with stroke
    "\363",			// Latin small letter eth
    "\364",			// Latin small letter h with stroke
    "\365",			// Latin small letter dotless i
    "\366",			// Latin small ligature ij
    "\367",			// Latin small letter l with middle dot
    "\370",			// Latin small letter l with stroke
    "\371",			// Latin small letter o with stroke
    "\372",			// Latin small ligature oe
    "\373",			// Latin small letter sharp s
    "\374",			// Latin small letter thorn
    "\375",			// Latin small letter t with stroke
    "\376",			// Latin small letter eng
    "\377",			// Soft hyphen (note 7)
  });

  constant decode_to = ({
    /*"#", "$",*/ "$", "#", "\244", // Note 3
    /*"^", "`", "~",*/		// Note 4
    DEC_ERR("\251"),		// Left single quotation mark (note 7)
    DEC_ERR("\252"),		// Left double quotation mark (note 7)
    DEC_ERR("\254"),		// Leftwards arrow (note 7)
    DEC_ERR("\255"),		// Upwards arrow (note 7)
    DEC_ERR("\256"),		// Rightwards arrow (note 7)
    DEC_ERR("\257"),		// Downwards arrow (note 7)
    "�",			// Multiplication sign
    "�",			// Division sign
    DEC_ERR("\271"),		// Right single quotation mark (note 7)
    DEC_ERR("\272"),		// Right double quotation mark (note 7)
    DEC_ERR("\300"),		// Note 5
    DEC_COMB_MARK GR(""),	// Combining grave accent
    DEC_COMB_MARK AC(""),	// Combining acute accent
    DEC_COMB_MARK CI(""),	// Combining circumflex accent
    DEC_COMB_MARK TI(""),	// Combining tilde
    DEC_COMB_MARK MA(""),	// Combining macron
    DEC_COMB_MARK BR(""),	// Combining breve
    DEC_COMB_MARK DA(""),	// Combining dot above
    DEC_COMB_MARK DI(""),	// Combining diaeresis
    DEC_ERR("\311"),		// Note 5
    DEC_COMB_MARK RA(""),	// Combining ring above
    DEC_COMB_MARK CE(""),	// Combining cedilla
    DEC_COMB_MARK UN(""),	// Combining underscore (note 6)
    DEC_COMB_MARK DO(""),	// Combining double acute accent
    DEC_COMB_MARK OG(""),	// Combining ogonek
    DEC_COMB_MARK CA(""),	// Combining caron
    DEC_ERR("\320"),		// Em dash (note 7)
    "�",			// Superscript one
    "�",			// Registered sign (note 7)
    "�",			// Copyright sign (note 7)
    DEC_ERR("\324"),		// Trade mark sign (note 7)
    DEC_ERR("\325"),		// Eighth note (note 7)
    "�",			// Not sign (note 7)
    "�",			// Broken bar (note 7)
    DEC_ERR("\330"), DEC_ERR("\331"), DEC_ERR("\332"), DEC_ERR("\333"), // Note 2
    DEC_ERR("\334"),		// Vulgar fraction one eighth (note 7)
    DEC_ERR("\335"),		// Vulgar fraction three eighths (note 7)
    DEC_ERR("\336"),		// Vulgar fraction five eighths (note 7)
    DEC_ERR("\337"),		// Vulgar fraction seven eighths (note 7)
    DEC_ERR("\340"),		// Ohm sign
    "�",			// Latin capital ligature ae
    DEC_ERR("\342"),		// Latin capital letter d with stroke
    "�",			// Feminine ordinal indicator
    DEC_ERR("\344"),		// Latin capital letter h with stroke
    DEC_ERR("\345"),		// Note 2
    DEC_ERR("\346"),		// Latin capital ligature ij
    DEC_ERR("\347"),		// Latin capital letter l with middle dot
    DEC_ERR("\350"),		// Latin capital letter l with stroke
    "�",			// Latin capital letter o with stroke
    DEC_ERR("\352"),		// Latin capital ligature oe
    "�",			// Masculine ordinal indicator
    "�",			// Latin capital letter thorn
    DEC_ERR("\355"),		// Latin capital letter t with stroke
    DEC_ERR("\356"),		// Latin capital letter eng
    DEC_ERR("\357"),		// Latin small letter n preceded by apostrophe
    DEC_ERR("\360"),		// Latin small letter kra
    "�",			// Latin small ligature ae
    DEC_ERR("\362"),		// Latin small letter d with stroke
    "�",			// Latin small letter eth
    DEC_ERR("\364"),		// Latin small letter h with stroke
    DEC_ERR("\365"),		// Latin small letter dotless i
    DEC_ERR("\366"),		// Latin small ligature ij
    DEC_ERR("\367"),		// Latin small letter l with middle dot
    DEC_ERR("\370"),		// Latin small letter l with stroke
    "�",			// Latin small letter o with stroke
    DEC_ERR("\372"),		// Latin small ligature oe
    "�",			// Latin small letter sharp s
    "�",			// Latin small letter thorn
    DEC_ERR("\375"),		// Latin small letter t with stroke
    DEC_ERR("\376"),		// Latin small letter eng
    "\255",			// Soft hyphen (note 7)
  });

  constant decode_comb = ([
    GR(" "): "`",
    AC(" "): "�",
    CI(" "): "^",
    TI(" "): "~",
    DI(" "): "�",
    // RA(" "): DEC_ERR(RA(" ")),
    MA(" "): "�",
    // BR(" "): DEC_ERR(BR(" ")),
    // DA(" "): DEC_ERR(DA(" ")),
    CE(" "): "�",
    // DO(" "): DEC_ERR(DO(" ")),
    // OG(" "): DEC_ERR(OG(" ")),
    // CA(" "): DEC_ERR(CA(" ")),
    GR("A"): "�", AC("A"): "�", CI("A"): "�", TI("A"): "�", DI("A"): "�", RA("A"): "�",
    GR("a"): "�", AC("a"): "�", CI("a"): "�", TI("a"): "�", DI("a"): "�", RA("a"): "�",
    CE("C"): "�",
    CE("c"): "�",
    GR("E"): "�", AC("E"): "�", CI("E"): "�", DI("E"): "�",
    GR("e"): "�", AC("e"): "�", CI("e"): "�", DI("e"): "�",
    GR("I"): "�", AC("I"): "�", CI("I"): "�", DI("I"): "�",
    GR("i"): "�", AC("i"): "�", CI("i"): "�", DI("i"): "�",
    TI("N"): "�",
    TI("n"): "�",
    GR("O"): "�", AC("O"): "�", CI("O"): "�", TI("O"): "�", DI("O"): "�",
    GR("o"): "�", AC("o"): "�", CI("o"): "�", TI("o"): "�", DI("o"): "�",
    GR("U"): "�", AC("U"): "�", CI("U"): "�", DI("U"): "�",
    GR("u"): "�", AC("u"): "�", CI("u"): "�", DI("u"): "�",
    GR("Y"): "�",
    GR("y"): "�",
  ]);

  /* Notes from Moras paper:

     (1) All characters in 0xC0-0xCF are non-spacing characters.  They are
     all diacritical marks.  To be represented stand-alone, they need to
     be followed by a SPACE (0x20).  They can appear, also, before
     letters if the couple is one of the defined combinations.

     (2) Reserved for future standardization.

     (3) Current terminals may send and receive 0xA6 and 0xA4 for the NUMBER
     SIGN and DOLLAR SIGN, respectively.  When receiving codes 0x23 and
     0x24, they may interpret them as NUMBER SIGN and CURRENCY SIGN,
     respectively.  Future applications should code the NUMBER SIGN,
     DOLLAR SIGN and CURRENCY SIGN as 0x23, 0x24 and 0xA8, respectively.

     (4) Terminals should send only the codes 0xC1, 0xC3 and 0xC4, followed
     by SPACE (0x20) for stand-alone GRAVE ACCENT, CIRCUMFLEX ACCENT and
     TILDE, respectively.  Nevertheless the terminal shall interpret the
     codes 0x60, 0x5E and 0x7E as GRAVE, CIRCUMFLEX and TILDE,
     respectively.

     (5) This code position is reserved and shall not be used.

     (6) It is recommended to implement the "underline" function by means of
     the control function SGR(4) instead of the "non-spacing underline"
     graphic character.

     (7) Not used in current teletex service (Recommendation T.61).
  */

#undef GR
#undef AC
#undef CI
#undef TI
#undef MA
#undef BR
#undef DA
#undef DI
#undef RA
#undef CE
#undef UN
#undef DO
#undef OG
#undef CA

  string(0..255) get_der_content()
  {
    return [string(0..255)]replace(value, encode_from,
				   [array(string(0..255))]encode_to);
  }

  this_program decode_primitive (string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    der = contents;

    array(string) parts =
      replace (contents, decode_from, decode_to) / DEC_COMB_MARK;
    value = parts[0];
    foreach (parts[1..], string part)
      value += (decode_comb[part[..1]] || DEC_ERR(part[..1])) + part[2..];

    return this;
  }

#undef ENC_ERR
#undef DEC_ERR
#undef DEC_COMB_MARK
}

//!
int(0..1) asn1_broken_teletex_valid (string s)
{
  return global.String.width(s)==8;
}

//! (broken) TeletexString object
//!
//! Encodes and decodes latin1, but labels it TeletexString, as is
//! common in many broken programs (e.g. Netscape 4.0X).
class BrokenTeletexString
{
  inherit String;
  int tag = 20;
  constant type_name = "TeletexString";	// Alias: T61String
}

Regexp asn1_IA5_invalid_chars = Regexp ("([\200-\377])");

//!
int(0..1) asn1_IA5_valid (string s)
{
  if (global.String.width(s)!=8) return 0;
  return !asn1_printable_invalid_chars->match (s);
}

//! IA5 String object
//!
//! Character set: ASCII. Fixed width encoding with 1 octet per
//! character.
class IA5String
{
  inherit String;
  int tag = 22;
  constant type_name = "IA5STRING";
}

//!
class VisibleString {
  inherit String;
  int tag = 26;
  constant type_name = "VisibleString";
}

//! UTCTime
//!
//! @rfc{2459:4.1.2.5.1@}.
class UTC
{
  inherit String;
  int tag = 23;
  constant type_name = "UTCTime";

  //!
  this_program set_posix(int t)
  {
    object second = Calendar.ISO_UTC.Second(t);

    // RFC 2459 4.1.2.5.1:
    //
    // Where YY is greater than or equal to 50, the year shall be
    // interpreted as 19YY; and
    //
    // Where YY is less than 50, the year shall be interpreted as 20YY.
    if (second->year_no() >= 2050)
      error( "Times later than 2049 not supported.\n" );
    if (second->year_no() < 1950)
      error( "Times earlier than 1950 not supported.\n" );

    value = sprintf("%02d%02d%02d%02d%02d%02dZ",
                    [int]second->year_no() % 100,
                    [int]second->month_no(),
                    [int]second->month_day(),
                    [int]second->hour_no(),
                    [int]second->minute_no(),
                    [int]second->second_no());
    return this;
  }

  //!
  int get_posix()
  {
    if( !value || sizeof(value)!=13 ) error("Data not UTC date string.\n");

    array t = (array(int))(value[..<1]/2);
    if(t[0]>49)
      t[0]+=1900;
    else
      t[0]+=2000;

    return [int]Calendar.ISO_UTC.Second(@t)->unix_time();
  }
}

//!
int(0..0) asn1_universal_valid (string s)
{
  return 0; s; // Return 0 since the UniversalString isn't implemented.
}

//! Universal String object
//!
//! Character set: ISO/IEC 10646-1 (compatible with Unicode).
//! Fixed width encoding with 4 octets per character.
//!
//! @fixme
//! The encoding is very likely UCS-4, but that's not yet verified.
class UniversalString
{
  inherit OctetString;
  int tag = 28;
  constant type_name = "UniversalString";

  string get_der_content() {
    error( "Encoding not implemented\n" );
  }

  this_program decode_primitive (string contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    error( "Decoding not implemented\n" ); contents;
  }
}

//!
int(0..1) asn1_bmp_valid (string s)
{
  return global.String.width(s)<32;
}

//! BMP String object
//!
//! Character set: ISO/IEC 10646-1 (compatible with Unicode).
//! Fixed width encoding with 2 octets per character.
//!
//! FIXME: The encoding is very likely UCS-2, but that's not yet verified.
class BMPString
{
  inherit OctetString;
  int tag = 30;
  constant type_name = "BMPString";

  string get_der_content() {
    return string_to_unicode (value);
  }

  this_program decode_primitive (string(0..255) contents,
				function(ADT.struct,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    der = contents;
    value = unicode_to_string (contents);
    return this;
  }
}

//! Meta-instances handle a particular explicit tag and set of types.
//! Once cloned this object works as a factory for Compound objects
//! with the cls and tag that the meta object was initialized with.
//!
//! @example
//!   MetaExplicit m = MetaExplicit(1,2);
//!   Compound c = m(Integer(3));
class MetaExplicit
{
  int real_tag;
  int real_cls;

  mapping(int:program(Object)) valid_types;

  class `() {
    inherit Compound;
    constant type_name = "EXPLICIT";
    constant constructed = 1;

    int get_tag() { return real_tag; }
    int get_cls() { return real_cls; }

    Object contents;

    array(Object) `elements() { return contents ? ({ contents }) : ({}); }
    void `elements=(array(Object) args)
    {
      if (sizeof(args) > 1) error("Invalid number of elements.\n");
      contents = sizeof(args) && args[0];
    }

    int `combined_tag() {
      return get_combined_tag();
    }

    this_program init(Object o) {
      contents = o;
      return this;
    }

    string get_der_content() {
      WERROR("asn1_explicit->der: contents = '%O\n", contents);
      return contents->get_der();
    }

    this_program decode_constructed_element(int i, Object e) {
      if (i)
	error("Unexpected index!\n");
      contents = e;
      return this;
    }

    this_program end_decode_constructed(int length) {
      if (length != 1)
	error("length != 1!\n");
      return this;
    }

    mapping(int:program(Object)) element_types(int i,
        mapping(int:program(Object)) types) {
      if (i)
	error("Unexpected index!\n");
      return valid_types || types;
    }

    protected string _sprintf(int t) {
      if (t != 'O') return UNDEFINED;
      if ((real_cls == 2) && (real_tag <= 3)) {
	// Special case for the convenience variants further below.
	return sprintf("%O.TaggedType%d(%O)",
		       global::this, real_tag, contents);
      }
      return sprintf("%O(%s %d %O)", this_program, type_name,
		     real_tag, contents);
    }

    string debug_string() {
      return type_name + "[" + (int) real_tag + "]";
    }
  }

  //!
  protected void create(int cls, int tag,
			mapping(int:program(Object))|void types) {
    real_cls = cls;
    real_tag = tag;
    valid_types = types;
  }
}

//! Some common explicit tags for convenience.
//!
//! These are typically used to indicate which
//! of several optional fields are present.
//!
//! @example
//!   Eg @rfc{5915:3@}:
//!   @code
//!     ECPrivateKey ::= SEQUENCE {
//!       version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
//!       privateKey     OCTET STRING,
//!       parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
//!       publicKey  [1] BIT STRING OPTIONAL
//!     }
//!   @endcode
//!   The presence of the fields @tt{parameters@} and @tt{publicKey@} above
//!   are indicated with @[TaggedType0] and @[TaggedType1] respectively.
MetaExplicit TaggedType0 = MetaExplicit(2, 0);
MetaExplicit TaggedType1 = MetaExplicit(2, 1);
MetaExplicit TaggedType2 = MetaExplicit(2, 2);
MetaExplicit TaggedType3 = MetaExplicit(2, 3);

constant meta_explicit = MetaExplicit;
constant asn1_object = Object;
constant asn1_compound = Compound;
constant asn1_string = String;
constant asn1_boolean = Boolean;
constant asn1_integer = Integer;
constant asn1_enumerated = Enumerated;
constant asn1_bit_string = BitString;
constant asn1_octet_string = OctetString;
constant asn1_null = Null;
constant asn1_identifier = Identifier;
constant asn1_utf8_string = UTF8String;
constant asn1_sequence = Sequence;
constant asn1_set = Set;
constant asn1_printable_string = PrintableString;
constant asn1_teletex_string = TeletexString;
constant asn1_broken_teletex_string = BrokenTeletexString;
constant asn1_IA5_string = IA5String;
constant asn1_utc = UTC;
constant asn1_universal_string = UniversalString;
constant asn1_bmp_string = BMPString;
