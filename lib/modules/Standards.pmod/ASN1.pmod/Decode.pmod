//
// $Id: Decode.pmod,v 1.19 2004/04/14 20:19:09 nilsson Exp $
//

#pike __REAL_VERSION__
#pragma strict_types
#define COMPATIBILITY

//! Decodes a DER object.

#if constant(Gmp.mpz)

import .Types;

//! Primitive unconstructed ASN1 data type.
class primitive
{
  //! @decl inherit Types.Object
  inherit Object;

  constant constructed = 0;
  int combined_tag;

  string raw;

  //! get raw encoded contents of object
  string get_der() { return raw; }
  int get_combined_tag() { return combined_tag; }

  //! get tag
  int get_tag() { return extract_tag(combined_tag); }

  //! get class
  int get_cls() { return extract_cls(combined_tag); }

  void create(int t, string r) {
    combined_tag = t;
    raw = r;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d)", this_program, combined_tag);
  }

#ifdef COMPATIBILITY
  string debug_string() {
    return sprintf("primitive(%d)", combined_tag);
  }
#endif
}

//! constructed type
class constructed
{
  //! @decl inherit Types.Object
  inherit Object;

  constant constructed = 1;
  int combined_tag;

  //! raw encoded  contents
  string raw;

  //! elements of object
  array elements;

  string get_der() { return raw; }
  int get_combined_tag() { return combined_tag; }

  //! get tag
  int get_tag() { return extract_tag(combined_tag); }

  //! get class
  int get_cls() { return extract_cls(combined_tag); }

  void create(int t, string r, array e) {
    combined_tag = t;
    raw = r;
    elements = e;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d)", this_program, combined_tag);
  }
}

//! @param data
//!   an instance of ADT.struct
//! @param types
//!   a mapping from combined tag numbers to classes from or derived from
//!   @[Standards.ASN1.Types]. Combined tag numbers may be generated using
//!   @[Standards.ASN1.Types.make_combined_tag].
//!
//! @returns
//!   an object from @[Standards.ASN1.Types] or
//!   either @[Standards.ASN1.Decode.primitive] or
//!   @[Standards.ASN1.Decode.constructed] if the type is unknown.
//!   Throws an exception if the data could not be decoded.
//!
//! @fixme
//!   Handling of implicit and explicit ASN.1 tagging, as well as
//!   other context dependence, is next to non_existant.
Object der_decode(ADT.struct data, mapping(int:program(Object)) types)
{
  int raw_tag = data->get_uint(1);
  int len;
  string contents;

#ifdef ASN1_DEBUG
  werror("decoding raw_tag %x\n", raw_tag);
#endif

  if ( (raw_tag & 0x1f) == 0x1f)
    error("High tag numbers is not supported\n");

  len = data->get_uint(1);
  if (len & 0x80)
    len = data->get_uint(len & 0x7f);

#ifdef ASN1_DEBUG
  werror("len : %d\n", len);
#endif
  contents = data->get_fix_string(len);

#ifdef ASN1_DEBUG
  werror("contents: %O\n", contents);
#endif

  int tag = make_combined_tag(raw_tag >> 6, raw_tag & 0x1f);

  program(Object) p = types[tag];

  if (raw_tag & 0x20)
  {
    /* Constructed encoding */

#ifdef ASN1_DEBUG
    werror("Decoding constructed\n");
#endif

    array(Object) elements = ({ });
    ADT.struct struct = ADT.struct(contents);

    if (!p)
    {
#ifdef ASN1_DEBUG
      werror("Unknown constructed type\n");
#endif
      while (!struct->is_empty())
	elements += ({ der_decode(struct, types) });

      return constructed(tag, contents, elements);
    }

    Object res = p();
    res->begin_decode_constructed(contents);

    int i;

    // Ask object which types it expects for field i, decode it, and
    // record the decoded object
    for(i = 0; !struct->is_empty(); i++)
    {
#ifdef ASN1_DEBUG
      werror("Element %d\n", i);
#endif
      res->decode_constructed_element
	(i, der_decode(struct,
		       res->element_types(i, types)));
    }
    return res->end_decode_constructed(i);
  }

#ifdef ASN1_DEBUG
  werror("Decoding primitive\n");
#endif
  // Primitive encoding
  return p ? p()->decode_primitive(contents)
    : primitive(tag, contents);
}

#define U(x) make_combined_tag(0, (x))

mapping(int:program(Object)) universal_types =
([ U(1) : Boolean,
   U(2) : Integer,
   U(3) : BitString,
   U(4) : OctetString,
   U(5) : Null,
   U(6) : Identifier,
   // U(9) : Real,
   // U(10) : Enumerated,
   U(12) : UTF8String,
   U(16) : Sequence,
   U(17) : Set,
   U(19) : PrintableString,
   U(20) : TeletexString,	// or broken_teletexString?
   U(22) : IA5String,
   U(23) : UTC,
   U(28) : UniversalString,
   U(30) : BMPString,
  ]);

//! decode a DER encoded object using universal data types
//!
//! @param data
//!   a DER encoded object
//! @returns
//!   an object from @[Standards.ASN1.Types] or
//!   either @[Standards.ASN1.Decode.primitive] or
//!   @[Standards.ASN1.Decode.constructed] if the type is unknown.
Object simple_der_decode(string data)
{
  return der_decode(ADT.struct(data), universal_types);
}

#else
constant this_program_does_not_exist=1;
#endif
