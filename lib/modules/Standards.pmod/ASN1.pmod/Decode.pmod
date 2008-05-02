//
// $Id: Decode.pmod,v 1.23 2008/05/02 16:41:57 nilsson Exp $
//

#pike __REAL_VERSION__
#pragma strict_types
#define COMPATIBILITY

//! Decodes a DER object.

#if constant(Gmp.mpz)

//! Primitive unconstructed ASN1 data type.
class Primitive
{
  //! @decl inherit Types.Object
  inherit .Types.Object;

  constant constructed = 0;
  int combined_tag;

  string raw;

  //! get raw encoded contents of object
  string get_der() { return raw; }
  int get_combined_tag() { return combined_tag; }

  //! get tag
  int get_tag() { return .Types.extract_tag(combined_tag); }

  //! get class
  int get_cls() { return .Types.extract_cls(combined_tag); }

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

//! Constructed type
class Constructed
{
  //! @decl inherit Types.Object
  inherit .Types.Object;

  constant constructed = 1;
  int combined_tag;

  //! raw encoded  contents
  string raw;

  //! elements of object
  array elements;

  string get_der() { return raw; }
  int get_combined_tag() { return combined_tag; }

  //! get tag
  int get_tag() { return .Types.extract_tag(combined_tag); }

  //! get class
  int get_cls() { return .Types.extract_cls(combined_tag); }

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
//!   either @[Standards.ASN1.Decode.Primitive] or
//!   @[Standards.ASN1.Decode.constructed] if the type is unknown.
//!   Throws an exception if the data could not be decoded.
//!
//! @fixme
//!   Handling of implicit and explicit ASN.1 tagging, as well as
//!   other context dependence, is next to non_existant.
.Types.Object der_decode(ADT.struct data,
                         mapping(int:program(.Types.Object)) types)
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

  int tag = .Types.make_combined_tag(raw_tag >> 6, raw_tag & 0x1f);

  program(.Types.Object) p = types[tag];

  if (raw_tag & 0x20)
  {
    /* Constructed encoding */

#ifdef ASN1_DEBUG
    werror("Decoding constructed\n");
#endif

    array(.Types.Object) elements = ({ });
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

    .Types.Object res = p();
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
  werror("Decoding Primitive\n");
#endif
  // Primitive encoding
  return p ? p()->decode_primitive(contents, this_object(), types)
    : Primitive(tag, contents);
}

#define U(x) .Types.make_combined_tag(0, (x))

mapping(int:program(.Types.Object)) universal_types =
([ U(1) : .Types.Boolean,
   U(2) : .Types.Integer,
   U(3) : .Types.BitString,
   U(4) : .Types.OctetString,
   U(5) : .Types.Null,
   U(6) : .Types.Identifier,
   // U(9) : .Types.Real,
   // U(10) : .Types.Enumerated,
   U(12) : .Types.UTF8String,
   U(16) : .Types.Sequence,
   U(17) : .Types.Set,
   U(19) : .Types.PrintableString,
   U(20) : .Types.TeletexString,	// or broken_teletexString?
   U(22) : .Types.IA5String,
   U(23) : .Types.UTC,
   U(28) : .Types.UniversalString,
   U(30) : .Types.BMPString,
  ]);

//! decode a DER encoded object using universal data types
//!
//! @param data
//!   a DER encoded object
//! @returns
//!   an object from @[Standards.ASN1.Types] or
//!   either @[Standards.ASN1.Decode.Primitive] or
//!   @[Standards.ASN1.Decode.constructed] if the type is unknown.
.Types.Object simple_der_decode(string data)
{
  return der_decode(ADT.struct(data), universal_types);
}

#ifdef COMPATIBILITY
constant primitive = Primitive;
constant constructed = Constructed;
#endif

#else
constant this_program_does_not_exist=1;
#endif
