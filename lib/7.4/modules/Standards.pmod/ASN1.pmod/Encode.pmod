/* Encode.pmod
 *
 * Encodes various asn.1 objects according to the Distinguished
 * Encoding Rules (DER) */

/* This file is obsolete. All code should use ASN1.Types instead. */

#pike __REAL_VERSION__

#if 0
#define WERROR werror
#else
#define WERROR(x)
#endif

#if constant(Gmp.mpz)

class asn1_object
{
  constant cls = 0;
  constant tag = 0;
  constant constructed = 0;
  
  int get_cls() { return cls; }
  int get_tag() { return tag; }

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
      if (sizeof(s) >= 0x80)
	error( "asn1.encode.asn1_object->encode_length: Max length exceeded.\n" );
      return sprintf("%c%s", sizeof(s) | 0x80, s);
    }
  
  string build_der(string contents)
    {
      string data = encode_tag() + encode_length(sizeof(contents)) + contents;
      // WERROR(sprintf("build_der: '%s'\n", Crypto.string_to_hex(data)));
      WERROR(sprintf("build_der: '%s'\n", data));
      return data;
    }
}

class asn1_compound
{
  inherit asn1_object;

  constant constructed = 1;
  array(object) elements;

  void create(object ...args)
    {
      elements = args;
      foreach(elements, mixed o)
	if (!o || !objectp(o))
	  error( "asn1_compound: Non-object argument!\n" );
      WERROR(sprintf("asn1_compound: %O\n", elements));
    }
}

class asn1_integer
{
  inherit asn1_object;
  constant tag = 2;
  
  object value;

  void create(int|object n) { value = Gmp.mpz(n); }

  string der()
    {
      string s;
      
      if (value < 0)
      {
	object n = value + pow(256, (- value)->size(256));
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
}

class asn1_bitstring
{
  inherit asn1_object;
  constant tag = 3;

  string value;
  int unused = 0;
  
  void create(string s) { value = s; }

  string der()
    {
      return build_der(sprintf("%c%s", unused, value));
    }

  int set_length(int len)
    {
      if (len)
      {
	value = value[..(len + 7)/8];
	unused = (- len) % 8;
	value = sprintf("%s%c", value[..sizeof(value)-2], value[-1]
		    & ({ 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 })[unused]);
      } else {
	unused = 0;
	value = "";
      }
    }  
}

class asn1_octet_string
{
  inherit asn1_object;
  constant tag = 4;
  string value;

  void create(string s) { value = s; }

  string der()
    {
      return build_der(value);
    }
}

class asn1_null
{
  inherit asn1_object;
  constant tag = 5;

  string der() { return build_der(""); }
}

class asn1_identifier
{
  inherit asn1_object;
  constant tag = 6;

  array(int) id;

  void create(int ...args)
    {
      if ( (sizeof(args) < 2)
	   || (args[0] > 2)
	   || (args[1] >= ( (args[0] < 2) ? 40 : 176) ))
	error( "asn1.encode.asn1_identifier->create: Invalid object identifier.\n" );
      id = args;
    }

  object append(int ...args)
    {
      return object_program(this_object())(@id, @args);
    }
  
  string der()
    {
      return build_der(sprintf("%c%@s", 40 * id[0] + id[1],
			       Array.map(id[2..], to_base_128)));
    }
}

class asn1_sequence
{
  inherit asn1_compound;
  constant tag = 16;

  string der()
    {
      WERROR(sprintf("asn1_sequence->der: elements = '%O\n",
		     elements));
      array(string) a = Array.map(elements, "der");
      WERROR(sprintf("asn1_sequence->der: der(elements) = '%O\n", a));
      return build_der(`+("", @ a));
    }
}

class asn1_set
{
  inherit asn1_compound;
  constant tag = 17;

  int compare_octet_strings(string r, string s)
    {
      for(int i = 0;; i++)
      {
	if (i == sizeof(r))
	  return (i = sizeof(s)) ? 0 : 1;
	if (i == sizeof(s))
	  return -1;
	if (r[i] < s[i])
	  return 1;
	else if (r[i] > s[i])
	  return -1;
      }
    }

  string der()
    {
      WERROR(sprintf("asn1_set->der: elements = '%O\n",
		     elements));
      array(string) a = Array.map(elements, "der");
      WERROR(sprintf("asn1_set->der: der(elements) = '%O\n", a));
      return build_der(`+("", @Array.sort_array(a, compare_octet_strings)));
    }
}

class asn1_printable_string
{
  inherit asn1_octet_string;
  constant tag = 19;
}

class asn1_T61_string
{
  inherit asn1_octet_string;
  constant tag = 20;
}

class asn1_IA5_string
{
  inherit asn1_octet_string;
  constant tag = 22;
}

class asn1_utc
{
  inherit asn1_octet_string;
  constant tag = 23;
}

#endif
