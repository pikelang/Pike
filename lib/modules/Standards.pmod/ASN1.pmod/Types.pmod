//! Encodes various asn.1 objects according to the Distinguished
//! Encoding Rules (DER)

#pike __REAL_VERSION__
#pragma strict_types

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

#define CODEC(X) \
  array _encode() { return ({ cls, tag, value }); } \
  void _decode(array(X) x) { [ cls, tag, value ] = x; }

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
				function(Stdio.IOBuffer,
					 mapping(int:program(Object)):
					 Object) decoder,
				mapping(int:program(Object)) types);
  void begin_decode_constructed(string raw);
  void decode_constructed_element(int i, object e);
  __deprecated__ void end_decode_constructed(int length) { }

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

  __deprecated__ string(0..255) record_der(string(0..255) s)
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

  array _encode()
  {
    return ({ cls, tag, elements });
  }

  void _decode(array(int|array(Object)) x)
  {
    [ cls, tag, elements ] = x;
  }

  __deprecated__ string debug_string() {
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

  //! @ignore
  CODEC(string);
  //! @endignore

  this_program init(string(0..255) s) {
    value = s;
    return this;
  }

  string(0..255) get_der_content() {
    return [string(0..255)]value;
  }

  this_program decode_primitive(string(0..255) contents,
				function(Stdio.IOBuffer,
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

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }

  __deprecated__ string debug_string() {
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

  //! @ignore
  CODEC(int);
  //! @endignore

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
				function(Stdio.IOBuffer,
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

  __deprecated__ string debug_string() {
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

  //! @ignore
  CODEC(Gmp.mpz);
  //! @endignore

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
				function(Stdio.IOBuffer,
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

  __deprecated__ string debug_string() {
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

  //! @ignore
  CODEC(float);
  //! @endignore

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
                               function(Stdio.IOBuffer,
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

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O)", this_program, value);
  }

  __deprecated__ string debug_string()
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
				function(Stdio.IOBuffer,
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

  protected string _sprintf(int t) {
    if(t!='O') return UNDEFINED;
    if(!value) return sprintf("%O(0)", this_program);
    int size = sizeof(value)*8-unused;
    if(!unused) return sprintf("%O(%d %O)", this_program, size, value);
    return sprintf("%O(%d %0"+size+"s)", this_program, size,
                   ([object(Gmp.mpz)](Gmp.mpz(value, 256) >> unused))
                   ->digits(2));
  }

  array _encode()
  {
    return ({ cls, tag, value, unused });
  }

  void _decode(array(int|string(8bit)) x)
  {
    [ cls, tag, value, unused ] = x;
  }

  __deprecated__ string debug_string() {
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
				function(Stdio.IOBuffer,
					 mapping(int:program(Object)):
					 Object)|void decoder,
				mapping(int:program(Object))|void types) {
    record_der_contents(contents);
    return !sizeof(contents) && this;
  }

  array _encode()
  {
    return ({ cls, tag });
  }

  void _decode(array(int) x)
  {
    [ cls, tag ] = x;
  }

  __deprecated__ string debug_string() { return "NULL"; }
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
				function(Stdio.IOBuffer,
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

  protected mixed cast(string t)
  {
    switch(t)
    {
    case "string": return (array(string))id * ".";
    case "array" : return id+({});
    default: return UNDEFINED;
    }
  }

  protected string _sprintf(int t) {
    if(t!='O') return UNDEFINED;
    if(!id) return sprintf("%O(0)", this_program);
    return sprintf("%O(%s)", this_program, (array(string))id*".");
  }

  array _encode()
  {
    return ({ cls, tag, id });
  }

  void _decode(array(int|array(int)) x)
  {
    if( sizeof(x)!=3 || intp(x[2]) )
      id = [array(int)]x; // Compat with old codec that didn't save cls/tag.
    else
      [ cls, tag, id ] = x;
  }

  __deprecated__ string debug_string() {
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
//! Variable width encoding, see rfc2279.
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
				function(Stdio.IOBuffer,
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
				function(Stdio.IOBuffer,
					 mapping(int:program(Object)):
					 Object) decoder,
				mapping(int:program(Object)) types) {
    der = contents;
    elements = ({});
    Stdio.IOBuffer data = Stdio.IOBuffer(contents);
    while (sizeof(data)) {
      elements += ({ decoder(data, types) });
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

protected Regexp asn1_printable_invalid_chars = Regexp("([^-A-Za-z0-9 '()+,./:=?])");

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
  int(0..) tag = 20;
  constant type_name = "TeletexString";	// Alias: T61String
}

//! @class TeletexString
//!
//! This used to be a string encoded with the T.61 character coding,
//! and has been obsolete since the early 1990:s.
//!
//! @seealso
//!   @[BrokenTeletexString]

//! @decl inherit BrokenTeletexString

//! @ignore
__deprecated__(program(BrokenTeletexString)) TeletexString =
  (__deprecated__(program(BrokenTeletexString))) BrokenTeletexString;
//! @endignore

//! @endclass

protected Regexp asn1_IA5_invalid_chars = Regexp ("([\200-\377])");

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
//! RFC 2459 4.1.2.5.1
class UTC
{
  inherit String;
  int tag = 23;
  constant type_name = "UTCTime";

  this_program init(int|string|Calendar.ISO_UTC.Second t)
  {
    if(intp(t))
      set_posix([int]t);
    else if(objectp(t))
      set_posix([object(Calendar.ISO_UTC.Second)]t);
    else
      value = [string]t;
  }

  this_program set_posix(int t)
  {
    return set_posix(Calendar.ISO_UTC.Second(t));
  }

  //!
  variant this_program set_posix(Calendar.ISO_UTC.Second second)
  {

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

class GeneralizedTime
{
  inherit UTC;
  int tag = 24;
  constant type_name = "GeneralizedTime";

  // We are currently not doing any management of fractions. X690
  // states that fractions shouldn't have trailing zeroes, and should
  // be completely removed int the ".0" case.

  this_program set_posix(int t)
  {
    return set_posix(Calendar.ISO_UTC.Second(t));
  }

  //!
  variant this_program set_posix(Calendar.ISO_UTC.Second second)
  {
    value = sprintf("%04d%02d%02d%02d%02d%02dZ",
                    [int]second->year_no(),
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
    if( !value || sizeof(value) < 15 )
      error("Data not GeneralizedTime date string.\n");
    array(int) t = array_sscanf(value, "%4d%2d%2d%2d%2d%2d");
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
				function(Stdio.IOBuffer,
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
				function(Stdio.IOBuffer,
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

    array _encode()
    {
      return ({ real_cls, real_tag, contents });
    }

    void _decode(array(int|Object) x)
    {
      [ real_cls, real_tag, contents ] = x;
    }
    __deprecated__ string debug_string() {
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

  array _encode()
  {
    return ({ real_cls, real_tag, valid_types });
  }

  void _decode(array(int|mapping(int:program(Object))) x)
  {
    [ real_cls, real_tag, valid_types ] = x;
  }
}

//! Some common explicit tags for convenience.
//!
//! These are typically used to indicate which
//! of several optional fields are present.
//!
//! @example
//!   Eg RFC 5915 section 3:
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
