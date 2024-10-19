#pike __REAL_VERSION__
#pragma strict_types

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

  array _encode()
  {
    return ({ cls, tag, raw });
  }

  void _decode(array(int|string(8bit)) x)
  {
    [ cls, tag, raw ] = x;
  }
}

//! Constructed type
class Constructed (int cls, int tag, string(8bit) raw, array(.Types.Object) elements)
{
  inherit .Types.Compound;
  constant type_name = "CONSTRUCTED";

  //! Get raw encoded contents of object
  string(8bit) get_der_content() { return raw; }
}

protected int read_varint(Stdio.Buffer data,
                          void|int(0..1) secure)
{
  int ret, byte;
  do {
    ret <<= 7;
    byte = data->read_int8();
    ret |= (byte & 0x7f);
    if (secure && !ret) {
      error("Non-canonical encoding of integer.\n");
    }
  } while (byte & 0x80);
  return ret;
}

// @returns
// @array
//   @elem int 1
//     Class
//   @elem bool 2
//     Constructed
//   @elem int 3
//     Tag
// @endarray
protected array(int) read_identifier(Stdio.Buffer data,
                                     void|int(0..1) secure)
{
  int byte = data->read_int8();

  int cls = byte >> 6;
  int constructed = (byte >> 5) & 1;
  int tag = byte & 0x1f;

  if( tag == 0x1f ) {
    tag = read_varint(data, secure);
    if (secure && (tag == (tag & 0x1f)) && (tag != 0x1f)) {
      error("Non-canonical encoding of tag 0x%02x.\n", tag);
    }
  }

  return ({ cls, constructed, tag });
}

//! @param data
//!   An instance of Stdio.Buffer containing the DER encoded data.
//!
//! @param types
//!   A mapping from combined tag numbers to classes from or derived
//!   from @[Standards.ASN1.Types]. Combined tag numbers may be
//!   generated using @[Standards.ASN1.Types.make_combined_tag].
//!
//! @param secure
//!   Will fail if the encoded ASN.1 isn't in its canonical encoding.
//!
//! @returns
//!   An object from @[Standards.ASN1.Types] or, either
//!   @[Standards.ASN1.Decode.Primitive] or
//!   @[Standards.ASN1.Decode.constructed], if the type is unknown.
//!   Throws an exception if the data could not be decoded.
//!
//! @fixme
//!   Handling of implicit and explicit ASN.1 tagging, as well as
//!   other context dependence, is next to non_existant.
.Types.Object der_decode(Stdio.Buffer data,
                         mapping(int:program(.Types.Object)) types,
                         void|int(0..1) secure)
{
  [int(0..3) cls, int constructed, int(0..) tag] =
    read_identifier(data, secure);

  int(0..) len = data->read_int8();
  // if( !cls && !constructed && !tag && !len )
  //   error("End-of-contents not supported.\n");
  if( !cls && !tag )
    error("Tag 0 reserved for BER encoding.\n");
  if (len & 0x80)
  {
    if (len == 0xff)
      error("Illegal size.\n");
    if (len == 0x80)
      error("Indefinite length form not supported.\n");
    if (secure) {
      if (!data->read_int8())
        error("Non-canonical length encoding.\n");
      data->unread(1);
    }
    len = data->read_int(len & 0x7f);
    if (secure && (len == (len & 0x7f)))
      error("Non-canonical length encoding.\n");
  }
  DBG("class %O, constructed=%d, tag=%d, length=%d\n",
      ({"universal","application","context","private"})[cls],
      constructed, tag, len);

  data = [object(Stdio.Buffer)]data->read_buffer(len);
  data->set_error_mode(1);

  program(.Types.Object)|zero p = types[ .Types.make_combined_tag(cls, tag) ];

  if (constructed)
  {
    DBG("Decoding constructed %O\n", p);

    if (!p)
    {
      array(.Types.Object) elements = ({ });

      while (sizeof(data))
        elements += ({ der_decode(data, types, secure) });

      if (cls==2 && sizeof(elements)==1)
      {
	// Context-specific constructed compound with a single element.
	// ==> Probably a TaggedType.
	DBG("Probable tagged type.\n");
	return .Types.MetaExplicit(2, tag)(elements[0]);
      }

      DBG("Unknown constructed type.\n");
      return Constructed(cls, tag, (string(8bit))data, elements);
    }

    .Types.Object res = ([program]p)();
    res->cls = cls;
    res->tag = tag;
    res->begin_decode_constructed((string(8bit))data);

    // Ask object which types it expects for field i, decode it, and
    // record the decoded object
    for(int i = 0; sizeof(data); i++)
    {
      DBG("Element %d\n", i);
      res->decode_constructed_element
        (i, der_decode(data, res->element_types(i, types), secure));
    }
    return res;
  }

  DBG("Decoding Primitive\n");

  if (p)
  {
    object(.Types.Object) res = ([program]p)();
    res->cls = cls;
    res->tag = tag;
    if (!res->decode_primitive((string(8bit))data, this_function,
                              types, secure) &&
        secure) {
      error("Invalid DER encoding.\n");
    }
    return res;
  }

  return Primitive(cls, tag, (string(8bit))data);
}

#define U(x) .Types.make_combined_tag(0, (x))

mapping(int:program(.Types.Object)) universal_types =
([ U(1) : .Types.Boolean,
   U(2) : .Types.Integer,
   U(3) : .Types.BitString,
   U(4) : .Types.OctetString,
   U(5) : .Types.Null,
   U(6) : .Types.Identifier,
   // 7 : Object descriptor type
   // 8 : External type / instance-of
   U(9) : .Types.Real,
   U(10) : .Types.Enumerated,
   // 11 : Embedded-pdv
   U(12) : .Types.UTF8String,
   // 13 : Relative object identifier
   // 14 : Time
   // 15 : reserved
   U(16) : .Types.Sequence,
   U(17) : .Types.Set,
   U(19) : .Types.PrintableString,
   U(20) : .Types.BrokenTeletexString,
   U(22) : .Types.IA5String,
   U(23) : .Types.UTC,
   U(24) : .Types.GeneralizedTime,
   U(28) : .Types.UniversalString,
   // 29 : UnrestrictedCharacterString
   U(30) : .Types.BMPString,
   // 31 : DATE
   // 32 : TIME-OF-DAY
   // 33 : DATE-TIME
   // 34 : DURATION
   // 35 : OID internationalized resource identifier
   // 36 : Relative OID internationalized resource identifier
  ]);

//! decode a DER encoded object using universal data types
//!
//! @param data
//!   a DER encoded object
//!
//! @param types
//!   An optional set of application-specific types. Should map
//!   combined tag numbers to classes from or derived from
//!   @[Standards.ASN1.Types]. Combined tag numbers may be generated
//!   using @[Standards.ASN1.Types.make_combined_tag]. This set is
//!   used to extend @[universal_types].
//!
//! @returns
//!   an object from @[Standards.ASN1.Types] or
//!   either @[Standards.ASN1.Decode.Primitive] or
//!   @[Standards.ASN1.Decode.Constructed] if the type is unknown.
.Types.Object simple_der_decode(string(0..255) data,
				mapping(int:program(.Types.Object))|void types)
{
  types = types ? universal_types+[mapping]types : universal_types;
  return der_decode(Stdio.Buffer(data)->set_error_mode(1), [mapping]types);
}

//! Works just like @[simple_der_decode], except it will return
//! @expr{0@} on errors, including if there is leading or trailing
//! data in the provided ASN.1 @[data].
//!
//! @seealso
//!   @[simple_der_decode]
object(.Types.Object)|zero secure_der_decode(string(0..255) data,
				mapping(int:program(.Types.Object))|void types)
{
  types = types ? universal_types+[mapping]types : universal_types;
  Stdio.Buffer buf = Stdio.Buffer(data)->set_error_mode(1);
  catch {
    .Types.Object ret = der_decode(buf, [mapping]types, 1);
    if( sizeof(buf) ) return 0;
    return ret;
  };
  return 0;
}
