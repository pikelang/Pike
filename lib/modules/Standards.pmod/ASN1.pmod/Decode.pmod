/* Decode.pmod
 *
 */

#define error(msg) throw( ({ msg, backtrace() }) )

/* Decodes a DER object. DATA is an instance of ADT.struct, and types
 * is a mapping from tag numbers to classes.
 *
 * Returns either an object, or a mapping of tag and contents.
 * Throws an exception if the data could not be decoded.
 *
 * FIXME: Handling of implicit and explicit ASN.1 tagging, as well as
 * other context dependence, is next to non_existant. */

class primitive
{
  int tag;
  string raw;

  string get_der() { return raw; }

  void create(int t, string r)
    {
      tag = t;
      raw = r;
    }

  string debug_string() 
    {
      return sprintf("primitive(%d)", tag);
    }
}

class constructed
{
  int tag;
  string raw;
  array contents;
  
  string get_der() { return raw; }

  void create(int t, string r, array c)
    {
      tag = t;
      raw = r;
      contents = c;
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

  int len = data->get_uint(1);
  if (len & 0x80)
    len = data->get_uint(len & 0x7f);
    
#ifdef ASN1_DEBUG
  werror(sprintf("len : %d\n", len));
#endif
  contents = data->get_fix_string(len);
  
#ifdef ASN1_DEBUG
  werror(sprintf("contents: %O\n", contents));
#endif

  int tag = raw_tag & 0xdf; // Class and tag bits
  
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

import .Types;

object|mapping simple_der_decode(string data)
{
  return der_decode(ADT.struct(data),
		    ([ // 1 : asn1_boolean,
		       2 : asn1_integer,
		       3 : asn1_bit_string,
		       4 : asn1_octet_string,
		       5 : asn1_null,
		       6 : asn1_identifier,
		       // 9 : asn1_real,
		       // 10 : asn1_enumerated,
		       16 : asn1_sequence,
		       17 : asn1_set,
		       19 : asn1_printable_string,
		       20 : asn1_T61_string,
		       23 : asn1_utc
		    ]) );
}
