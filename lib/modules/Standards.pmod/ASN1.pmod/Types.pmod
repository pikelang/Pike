/* encode.pmod
 *
 * Encodes various asn.1 objects according to the Distinguished
 * Encoding Rules (DER) */

#if 0
#define WERROR werror
#else
#define WERROR(x)
#endif

#define error(msg) throw( ({ msg, backtrace() }) )


/* Combines tag and class as a single integer, in a somewhat arbitrary
 * way. This works also for tags beyond 31 (although not for tags
 * beyond 2^30. */

#define MAKE_COMBINED_TAG(cls, tag) (((tag) << 2) | (cls))
int make_combined_tag(int cls, int tag)
{ return MAKE_COMBINED_TAG(cls, tag); }

int extract_tag(int i) { return i >> 2; }
int extract_cls(int i) { return i & 3; }

class asn1_object
{
  constant cls = 0;
  constant tag = 0;
  constant constructed = 0;

  constant type_name = 0;
  
  int get_cls() { return cls; }
  int get_tag() { return tag; }
  int get_combinded_tag()
    { return make_combined_tag(get_tag(), get_cls()); }
  
  string der;
  
  // Should be overridden by subclasses 
  object decode_primitive(string contents)
    { error("decode_primitive not implemented\n"); }

  object begin_decode_constructed(string raw) 
    { error("begin_decode_constructed not implemented\n"); }
  object decode_constructed_element(int i, object e) 
    { error("decode_constructed_element not implemented\n"); }
  object end_decode_constructed(int length) 
    { error("end_decode_constructed not implemented\n"); }

  string der_encode();
  mapping element_types(int i, mapping types) { return types; }
  object init(mixed ... args) { return this_object(); }

  string to_base_128(int n)
    {
      if (!n)
	return "\0";
      /* Convert tag number to base 128 */
      array(int) digits = ({ });

      /* Array is built in reverse order, least significant digit first */
      while(n)
      {
	digits += ({ (n & 0x7f) | 0x80 });
	n >>= 7;
      }
      digits[0] &= 0x7f;

      return sprintf("%@c", reverse(digits));
    }
  
  string encode_tag()
    {
      int tag = get_tag();
      int cls = get_cls();
      if (tag < 31)
	return sprintf("%c", (cls << 6) | (constructed << 5) | tag);

      return sprintf("%c%s", (cls << 6) | (constructed << 5) | 0x1f,
		     to_base_128(tag) );
    }

  string encode_length(int|object len)
    {
      if (len < 0x80)
	return sprintf("%c", len);
      string s = Gmp.mpz(len)->digits(256);
      if (strlen(s) >= 0x80)
	throw( ({ "asn1.encode.asn1_object->encode_length: Max length exceeded.\n",
		  backtrace() }) );
      return sprintf("%c%s", strlen(s) | 0x80, s);
    }
  
  string build_der(string contents)
    {
      string data = encode_tag() + encode_length(strlen(contents)) + contents;
      // WERROR(sprintf("build_der: '%s'\n", Crypto.string_to_hex(data)));
      WERROR(sprintf("build_der: '%s'\n", data));
      return data;
    }

  string record_der(string s)
    {
      return (der = s);
    }

  string record_der_contents(string s)
    {
      record_der(build_der(s));
    }
  
  string get_der()
    {
      return der || (record_der(der_encode()));
    }

  void create(mixed ...args)
    {
      WERROR(sprintf("asn1_object[%s]->create\n", type_name));
      if (sizeof(args))
	init(@args);
    }
}

class asn1_compound
{
  inherit asn1_object;

  constant constructed = 1;
  array elements = ({ });

  object init(array args)
    {
      WERROR(sprintf("asn1_compound[%s]->init(%O)\n", type_name, args));
      elements = args;
      foreach(elements, mixed o)
	if (!o || !objectp(o))
	  throw( ({ "asn1_compound: Non-object argument!\n",
		    backtrace() }) );
      WERROR(sprintf("asn1_compound: %O\n", elements));
      return this_object();
    }

  object begin_decode_constructed(string raw)
    {
      WERROR(sprintf("asn1_compound[%s]->begin_decode_constructed\n",
		     type_name));
      record_der_contents(raw);
      return this_object();
    }

  object decode_constructed_element(int i, object e)
    {
      WERROR(sprintf("asn1_compound[%s]->decode_constructed_element(%O)\n",
		     type_name, e));
      if (i != sizeof(elements))
	error("decode_constructed_element: Unexpected index!\n");
      elements += ({ e });
      return this_object();
    }
  
  object end_decode_constructed(int length)
    {
      if (length != sizeof(elements))
	error("end_decode_constructed: Invalid length!\n");

      return this_object();
    }

  string debug_string()
    {
      WERROR(sprintf("asn1_compound[%s]->debug_string(), elements = %O\n",
		     type_name, elements));
      return "(" + type_name + " " + elements->debug_string() * ",\n" + ")";
    }
}

class asn1_string
{
  inherit asn1_object;
  string value;

  object init(string s)
    {
      value = s;
      return this_object();
    }

  string der_encode()
    {
      return build_der(value);
    }

  object decode_primitive(string contents)
    {
      record_der_contents(contents);
      value = contents;
      return this_object();
    }
  
  string debug_string()
    {
      WERROR(sprintf("asn1_string[%s]->debug_string(), value = %O\n",
		     type_name, value));
      return sprintf("%s %O", type_name, value);
    }
}

// FIXME: What is the DER-encoding of TRUE???
// According to Jan Petrous, the LDAP spec says that 0xff is right.

class asn1_boolean
{
  inherit asn1_object;
  constant tag = 1;
  constant type_name = "BOOLEAN";

  int value;
  
  object init(int x) { value = x; return this_object(); }

  string der_encode() { return build_der(value ? "\377" : "\0"); }
  object decode_primitive(string contents)
    {
      if (strlen(contents) != 1)
      {
	WERROR("asn1_boolean->decode_primitive: Bad length.\n");
	return 0;
      }
      record_der_contents(contents);
      value = contents[0];
      return this_object();
    }
  string debug_string()
    {
      return value ? "TRUE" : "FALSE";
    }
}
    
// All integers are represented as bignums, for simplicity
class asn1_integer
{
  inherit asn1_object;
  constant tag = 2;
  constant type_name = "INTEGER";
  
  object value;

  object init(int|object n)
    {
      value = Gmp.mpz(n);
      WERROR(sprintf("i = %s\n", value->digits()));
      return this_object();
    }

  string der_encode()
    {
      string s;
      
      if (value < 0)
      {
	object n = value + Gmp.pow(256, (- value)->size(256));
	s = n->digits(256);
	if (!(s[0] & 0x80))
	  s = "\377" + s;
      } else {
	s = value->digits(256);
	if (s[0] & 0x80)
	  s = "\0" + s;
      }
      return build_der(s);
    }
  
  object decode_primitive(string contents)
    {
      record_der_contents(contents);
      value = Gmp.mpz(contents, 256);
      if (contents[0] & 0x80)  /* Negative */
	value -= Gmp.pow(256, strlen(contents));
      return this_object();
    }
  
  string debug_string()
    {
      return sprintf("INTEGER (%d) %s", value->size(), value->digits());
    }

}
class asn1_enumerated
{
  inherit asn1_integer;
  constant tag = 10;
  constant type_name ="ENUMERATED";
}

class asn1_bit_string
{
  inherit asn1_object;
  constant tag = 3;
  constant type_name = "BIT STRING";
  string value;
  int unused = 0;
  
  object init(string s) { value = s; return this_object(); }

  string der_encode()
    {
      return build_der(sprintf("%c%s", unused, value));
    }

  int set_length(int len)
    {
      if (len)
      {
	value = value[..(len + 7)/8];
	unused = (- len) % 8;
	value = sprintf("%s%c", value[..strlen(value)-2], value[-1]
		    & ({ 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 })[unused]);
      } else {
	unused = 0;
	value = "";
      }
    }
  
  object decode_primitive(string contents)
    {
      record_der_contents(contents);
      if (!sizeof(contents))
	return 0;
      unused = contents[0];

      if (unused >= 8)
	return 0;
      value = contents[1..];
      return this_object();
    }
  
  string debug_string()
    {
      return sprintf("BIT STRING (%d) %s",
		     sizeof(value) * 8 - unused,
		     (Gmp.mpz(value, 256) >> unused)->digits(2));
    }
}

class asn1_octet_string
{
  inherit asn1_string;
  constant tag = 4;
  constant type_name = "OCTET STRING";
}

#if constant(string_to_utf8)
class asn1_utf8_string
{
  inherit asn1_string;
  constant tag = 12;
  constant type_name = "UTF8String";

  string der_encode()
    {
      return build_der(string_to_utf8(value));
    }

  object decode_primitive(string contents)
    {
      record_der_contents(contents);
      if (catch {
	value = utf8_to_string(contents);
      })
	return 0;
      
      return this_object();
    }
}  
#endif /* constant(string_to_utf8) */

class asn1_null
{
  inherit asn1_object;
  constant tag = 5;
  constant type_name = "NULL";
  
  string der_encode() { return build_der(""); }

  object decode_primitive(string contents)
    {
      record_der_contents(contents);
      return !sizeof(contents) && this_object();
    }

  string debug_string() { return "NULL"; }
}

class asn1_identifier
{
  inherit asn1_object;
  constant tag = 6;
  constant type_name = "OBJECT IDENTIFIER";
  
  array(int) id;

  object init(int ...args)
    {
      if ( (sizeof(args) < 2)
	   || (args[0] > 2)
	   || (args[1] >= ( (args[0] < 2) ? 40 : 176) ))
	throw( ({ "asn1.encode.asn1_identifier->init: Invalid object identifier.\n",
		  backtrace() }) );
      id = args;
      return this_object();
    }

  object append(int ...args)
    {
      return object_program(this_object())(@id, @args);
    }
  
  string der_encode()
    {
      return build_der(sprintf("%c%@s", 40 * id[0] + id[1],
			       Array.map(id[2..], to_base_128)));
    }

  object decode_primitive(string contents)
    {
      record_der_contents(contents);

      if (contents[0] < 120)
	id = ({ contents[0] / 40, contents[0] % 40 });
      else
	id = ({ 2, contents[0] - 80 });
      int index = 1;
      while(index < strlen(contents))
      {
	int element = 0;
	do
	{
	  element = element << 7 | (contents[index] & 0x7f);
	} while(contents[index++] & 0x80);
	id += ({ element });
      }
      return this_object();
    }

  string debug_string()
    {
      return "IDENTIFIER " + (array(string)) id * ".";
    }

  int `==(object other)
    {
      return (object_program(this_object())
	      == object_program(other))
	&& equal(id, other->id);
    }
}

class asn1_sequence
{
  inherit asn1_compound;
  constant tag = 16;
  constant type_name = "SEQUENCE";
  
  string der_encode()
    {
      WERROR(sprintf("asn1_sequence->der: elements = '%O\n",
		     elements));
      array(string) a = Array.map(elements, "get_der");
      WERROR(sprintf("asn1_sequence->der: der_encode(elements) = '%O\n", a));
      return build_der(`+("", @ a));
    }
}

class asn1_set
{
  inherit asn1_compound;
  constant tag = 17;
  constant type_name = "SET";
  
  int compare_octet_strings(string r, string s)
    {
      for(int i = 0;; i++)
      {
	if (i == strlen(r))
	  return (i = strlen(s)) ? 0 : 1;
	if (i == strlen(s))
	  return -1;
	if (r[i] < s[i])
	  return 1;
	else if (r[i] > s[i])
	  return -1;
      }
    }

  string der_encode()
    {
      WERROR(sprintf("asn1_set->der: elements = '%O\n",
		     elements));
      array(string) a = Array.map(elements, "get_der");
      WERROR(sprintf("asn1_set->der: der_encode(elements) = '%O\n", a));
      return build_der(`+("", @Array.sort_array(a, compare_octet_strings)));
    }
}

class asn1_printable_string
{
  inherit asn1_string;
  constant tag = 19;
  constant type_name = "PrintableString";
}

/* Often used for latin1 */
/* Aka T61String */
class asn1_teletex_string
{
  inherit asn1_string;
  constant tag = 20;
  constant type_name = "TeletexString";
}

class asn1_IA5_string
{
  inherit asn1_string;
  constant tag = 22;
  constant type_name = "IA5STRING";
}

class asn1_utc
{
  inherit asn1_string;
  constant tag = 23;
  constant type_name = "UTCTime";
}

class meta_explicit
{
  /* meta-instances handle a particular explicit tag and set of types */
  int real_tag;
  int real_cls;
  
  mapping valid_types;

  class `()
    {
      inherit asn1_compound;
      constant type_name = "EXPLICIT";
      constant constructed = 1;
      
      int get_tag() { return real_tag; }
      int get_cls() { return real_cls; }
      
      object contents;

      object init(object o)
	{
	  contents = o;
	  return this_object();
	}
    
      string der_encode()
	{
	  WERROR(sprintf("asn1_explicit->der: contents = '%O\n",
			 contents));
	  return build_der(contents->get_der());
	}

      object decode_constructed_element(int i, object e)
	{
	  if (i)
	    error("decode_constructed_element: Unexpected index!\n");
	  contents = e;
	  return this_object();
	}

      object end_decode_constructed(int length)
      {
	if (length != 1)
	  error("end_decode_constructed: length != 1!\n");
	return this_object();
      }

      mapping element_types(int i, mapping types)
	{
	  if (i)
	    error("element_types: Unexpected index!\n");
	  return valid_types || types;
	}

      string debug_string()
	{
	  return type_name + "[" + (int) real_tag + "]"
	    + contents->debug_string();
	}
    }
  
  void create(int cls, int tag, mapping|void types)
    {
      real_cls = cls;
      real_tag = tag;
      valid_types = types;
    }
}
      
