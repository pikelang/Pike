/* Decode.pmod
 *
 */

#pike __REAL_VERSION__

/* Decodes a DER object. DATA is an instance of ADT.struct, and types
 * is a mapping from tag numbers to classes.
 *
 * Returns either an object, or a mapping of tag and contents.
 * Throws an exception if the data could not be decoded.
 *
 * FIXME: Handling of implicit and explicit ASN.1 tagging, as well as
 * other context dependence, is next to non_existant. */

#if constant(Gmp.mpz)

import .Types;

class primitive
{
  import .Types;

  constant constructed = 0;  
  int combined_tag;
  string raw;
  
  string get_der() { return raw; }
  int get_combined_tag() { return combined_tag; }
  int get_tag() { return extract_tag(combined_tag); }
  int get_cls() { return extract_cls(combined_tag); }
  
  void create(int t, string r)
    {
      combined_tag = t;
      raw = r;
    }

  string debug_string() 
    {
      return sprintf("primitive(%d)", combined_tag);
    }
}

class constructed
{
  import .Types;
  
  constant constructed = 1;
  int combined_tag;
  
  string raw;
  array elements;
  
  string get_der() { return raw; }
  int get_combined_tag() { return combined_tag; }
  int get_tag() { return extract_tag(combined_tag); }
  int get_cls() { return extract_cls(combined_tag); }

  void create(int t, string r, array e)
    {
      combined_tag = t;
      raw = r;
      elements = e;
    }
}

object|mapping der_decode(object data, mapping types)
{
  int raw_tag = data->get_uint(1);
  int len;
  string contents;

#ifdef ASN1_DEBUG
  werror(sprintf("decoding raw_tag %x\n", raw_tag));
#endif

  if ( (raw_tag & 0x1f) == 0x1f)
    error("ASN1.Decode: High tag numbers is not supported\n");

  len = data->get_uint(1);
  if (len & 0x80)
    len = data->get_uint(len & 0x7f);
    
#ifdef ASN1_DEBUG
  werror(sprintf("len : %d\n", len));
#endif
  contents = data->get_fix_string(len);
  
#ifdef ASN1_DEBUG
  werror(sprintf("contents: %O\n", contents));
#endif

  int tag = make_combined_tag(raw_tag >> 6, raw_tag & 0x1f);

  program p = types[tag];
  
  if (raw_tag & 0x20)
  {
    /* Constructed encoding */

#ifdef ASN1_DEBUG
    werror("Decoding constructed\n");
#endif

    array elements = ({ });
    object struct = ADT.struct(contents);
    
    if (!p)
    {
#ifdef ASN1_DEBUG
      werror("Unknown constructed type\n");
#endif
      while (!struct->is_empty())
	elements += ({ der_decode(struct, types) });

      return constructed(tag, contents, elements);
    }

    object res = p();
    res->begin_decode_constructed(contents);
    
    int i;

    /* Ask object which types it expects for field i, decode it, and
     * record the decoded object */
    for(i = 0; !struct->is_empty(); i++)
    {
#ifdef ASN1_DEBUG
      werror(sprintf("Element %d\n", i));
#endif
      res->decode_constructed_element
	(i, der_decode(struct,
		       res->element_types(i, types)));
    }
    return res->end_decode_constructed(i);
  }
  else
  {
#ifdef ASN1_DEBUG
    werror("Decoding primitive\n");
#endif
    /* Primitive encoding */
    return p ? p()->decode_primitive(contents)
      : primitive(tag, contents);
  }
}

#define U(x) make_combined_tag(0, (x))

mapping universal_types =
([ U(1) : asn1_boolean,
   U(2) : asn1_integer,
   U(3) : asn1_bit_string,
   U(4) : asn1_octet_string,
   U(5) : asn1_null,
   U(6) : asn1_identifier,
   // U(9) : asn1_real,
   // U(10) : asn1_enumerated,
   U(12) : asn1_utf8_string,
   U(16) : asn1_sequence,
   U(17) : asn1_set,
   U(19) : asn1_printable_string,
   U(20) : asn1_teletex_string,	// or asn1_broken_teletex_string?
   U(22) : asn1_IA5_string,
   U(23) : asn1_utc,
   U(28) : asn1_universal_string,
   U(30) : asn1_bmp_string,
  ]);

object|mapping simple_der_decode(string data)
{
  return der_decode(ADT.struct(data), universal_types);
}

#endif
