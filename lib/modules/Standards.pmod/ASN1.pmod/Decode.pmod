#pike __REAL_VERSION__
#pragma strict_types
#define COMPATIBILITY

#ifdef ASN1_DEBUG
#define DBG werror
#else
#define DBG(X ...)
#endif

//! Decodes a DER object.

//! Primitive unconstructed ASN1 data type.
class Primitive (int cls, int tag, string(8bit) raw)
{
  //! @decl inherit Types.Object
  inherit .Types.Object;

  //! Get raw encoded contents of object
  string(8bit) get_der_content() { return raw; }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d)", this_program, get_combined_tag());
  }

#ifdef COMPATIBILITY
  __deprecated__ string debug_string() {
    return sprintf("primitive(%d)", get_combined_tag());
  }
#endif
}

//! Constructed type
class Constructed (int cls, int tag, string(8bit) raw, array(.Types.Object) elements)
{
  inherit .Types.Compound;
  constant type_name = "CONSTRUCTED";

  //! Get raw encoded contents of object
  string(8bit) get_der_content() { return raw; }
}

protected int read_varint(ADT.struct data)
{
  int ret, byte;
  do {
    ret <<= 7;
    byte = data->get_uint(1);
    ret |= (byte & 0x7f);
  } while (byte & 0x80);
  return ret;
}

protected array(int) read_identifier(ADT.struct data)
{
  int byte = data->get_uint(1);

  int cls = byte >> 6;
  int const = (byte >> 5) & 1;
  int tag = byte & 0x1f;

  if( tag == 0x1f )
    tag = read_varint(data);

  return ({ cls, const, tag });
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
  [int cls, int const, int tag] = read_identifier(data);
  DBG("class %O, construced %d, tag %d\n",
      ({"universal","application","context","private"})[cls], const, tag);

  int len = data->get_uint(1);
  if (len & 0x80)
  {
    if (len == 0xff)
      error("Illegal size.\n");
    len = data->get_uint(len & 0x7f);
  }
  DBG("len : %d\n", len);

  string(0..255) contents = data->get_fix_string(len);
  DBG("contents: %O\n", contents);

  program(.Types.Object) p = types[ .Types.make_combined_tag(cls, tag) ];

  if (const)
  {
    /* Constructed encoding */

    DBG("Decoding constructed\n");
    array(.Types.Object) elements = ({ });
    ADT.struct struct = ADT.struct(contents);

    if (!p)
    {
      while (!struct->is_empty())
	elements += ({ der_decode(struct, types) });

      if (cls==2 && sizeof(elements)==1)
      {
	// Context-specific constructed compound with a single element.
	// ==> Probably a TaggedType.
	DBG("Probable tagged type.\n");
	return .Types.MetaExplicit(2, tag)(elements[0]);
      }

      DBG("Unknown constructed type.\n");
      return constructed(cls, tag, contents, elements);
    }

    .Types.Object res = p();
    res->cls = cls;
    res->tag = tag;
    res->begin_decode_constructed(contents);

    int i;

    // Ask object which types it expects for field i, decode it, and
    // record the decoded object
    for(i = 0; !struct->is_empty(); i++)
    {
      DBG("Element %d\n", i);
      res->decode_constructed_element
	(i, der_decode(struct,
		       res->element_types(i, types)));
    }
    return res->end_decode_constructed(i);
  }

  DBG("Decoding Primitive\n");

  if (p)
  {
    .Types.Object res = p();
    res->cls = cls;
    res->tag = tag;
    return res->decode_primitive(contents, this_object(), types);
  }

  return Primitive(cls, tag, contents);
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
//!
//! @param types
//!   An optional set of application-specific types.
//!   This set is used to extend @[universal_types].
//!
//! @returns
//!   an object from @[Standards.ASN1.Types] or
//!   either @[Standards.ASN1.Decode.Primitive] or
//!   @[Standards.ASN1.Decode.constructed] if the type is unknown.
.Types.Object simple_der_decode(string(0..255) data,
				mapping(int:program(.Types.Object))|void types)
{
  if (types) {
    return der_decode(ADT.struct(data), universal_types + types);
  }
  return der_decode(ADT.struct(data), universal_types);
}

#ifdef COMPATIBILITY
constant primitive = Primitive;
constant constructed = Constructed;
#endif
