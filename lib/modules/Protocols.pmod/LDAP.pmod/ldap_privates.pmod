#pike __REAL_VERSION__

// LDAP client protocol implementation for Pike.
//
// Honza Petrous, hop@unibase.cz
//
// ----------------------------------------------------------------------
//
// ToDo List:
//
//	- v2 operations: 
//		modify
//
// History:
//
//	v2.0  1999-02-19 Create separate file. Implementation the following
//			 classes:
//			  - asn1_enumerated
//			  - asn1_boolean
//			  - asn1_application_sequence
//			  - asn1_application_octet_string
//			  - asn1_context_integer
//			  - asn1_context_octet_string
//			  - asn1_context_sequence
//			  - asn1_context_set
//			  - ldap_der_decode
//



// --------------- Standards.ASN1.Types private add-on --------------------
// This is very poor defined own ASN.1 objects (not enough time to clean it!)
//import Standards.ASN1.Encode;

#include "ldap_globals.h"

class asn1_application_sequence
{
  inherit Standards.ASN1.Types.Sequence;
  int cls = 1;
  constant type_name = "APPLICATION SEQUENCE";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, array args) {
    ::init(args);
    tagx = tagid;
  }

  protected string _sprintf(mixed ... args)
  {
    return sprintf("[%d]%s", tagx, ::_sprintf(@args));
  }
}

class asn1_application_octet_string
{
  inherit Standards.ASN1.Types.OctetString;
  int cls = 1;
  constant type_name = "APPLICATION OCTET_STRING";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, string arg) {
    ::value = arg;
    tagx = tagid;
  }

  protected string _sprintf(mixed ... args)
  {
    return sprintf("[%d]%s", tagx, ::_sprintf(@args));
  }
}

class asn1_context_boolean
{
  inherit Standards.ASN1.Types.Boolean;
  int cls = 2;
  constant type_name = "CONTEXT BOOLEAN";
  int tagx;

  int get_tag() {return tagx;}

  void init(int tagid, int arg) {
    ::init(arg);
    tagx = tagid;
  }

  protected string _sprintf(mixed ... args)
  {
    return sprintf("[%d]%s", tagx, ::_sprintf(@args));
  }
}

class asn1_context_integer
{
  inherit Standards.ASN1.Types.Integer;
  int cls = 2;
  constant type_name = "CONTEXT INTEGER";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, int arg) {
    ::init(arg);
    tagx = tagid;
  }

  protected string _sprintf(mixed ... args)
  {
    return sprintf("[%d]%s", tagx, ::_sprintf(@args));
  }
}

class asn1_context_octet_string
{
  inherit Standards.ASN1.Types.OctetString;
  int cls = 2;
  constant type_name = "CONTEXT OCTET_STRING";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, string arg) {
    ::init(arg);
    tagx = tagid;
  }

  protected string _sprintf(mixed ... args)
  {
    return sprintf("[%d]%s", tagx, ::_sprintf(@args));
  }
}

class asn1_context_sequence
{
  inherit Standards.ASN1.Types.Sequence;
  int cls = 2;
  constant type_name = "CONTEXT SEQUENCE";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, array arg) {
    ::init(arg);
    tagx = tagid;
  }

  protected string _sprintf(mixed ... args)
  {
    return sprintf("[%d]%s", tagx, ::_sprintf(@args));
  }
}

class asn1_context_set
{
  inherit Standards.ASN1.Types.Set;
  int cls = 2;
  constant type_name = "CONTEXT SET";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, array arg) {
    ::init(arg);
    tagx = tagid;
  }

  protected string _sprintf(mixed ... args)
  {
    return sprintf("[%d]%s", tagx, ::_sprintf(@args));
  }
}

class asn1_sequence
{
  inherit Standards.ASN1.Types.Sequence;
  constant type_name = "SEQUENCE";
  int cls = 2;
  int tag = 0;

  void init(int tagid, array arg)
  {
    ::init(arg);
  }

#if 0
  string encode_length(int|object len)
  {
    if (len < 0x80000000) {
      // Microsofts ldap APIs use this encoding.
      return sprintf("\204%4c", len);
    }
    return ::encode_length(len);
  }
#endif /* 0 */
}

// Low-level decoder.
// NOTE: API may change!
object|mapping der_decode(object data,
			  mapping(int:program) types)
{
  int raw_tag = data->get_uint(1);
  int len;
  string contents;

  // raw_tag is [cls:2][constr:1][tag:5].

  int tag = raw_tag & 0x1f;
  int cls = (raw_tag & 0xc0)>>6;

  if ( tag == 0x1f) {
    // Tag encoded in big-endian, base 128
    // msb signals continuation.
    tag = 0;
    int val;
    do {
      tag <<= 7;
      tag |= (val = data->get_uint(1)) & 0x7f;
    } while (val & 0x80);
  }

  len = data->get_uint(1);

  if (len & 0x80) {
    len = data->get_uint(len & 0x7f);
  }

  contents = data->get_fix_string(len);

  //int tag = raw_tag & 0xdf; // Class and tag bits

  program p = types[ Standards.ASN1.Types.make_combined_tag(cls,tag) ];

  if (raw_tag & 0x20)
  {
    /* Constructed encoding */

    array elements = ({ });
    object struct = ADT.struct(contents);

    if (!p)
    {
      while (!struct->is_empty())
        elements += ({ der_decode(struct, types) });

      return Standards.ASN1.Decode.constructed(tag, contents, elements);
    }

    object res = p();

    // hop: For non-universal classes we provide tag number
    if(cls)
      res = p(tag, ({}));

    res->begin_decode_constructed(contents);

    int i;

    /* Ask object which types it expects for field i, decode it, and
     * record the decoded object */
    for(i = 0; !struct->is_empty(); i++)
    {
      res->decode_constructed_element
        (i, der_decode(struct,
                       res->element_types(i, types)));
    }

    return res->end_decode_constructed(i);
  }
  else
  {
    /* Primitive encoding */
    return p ? p()->decode_primitive(contents)
      : Standards.ASN1.Decode.primitive(tag, contents);
  }
}

#define U(C,T) Standards.ASN1.Types.make_combined_tag(C,T)

// Mapping from class+tag to program.
protected mapping(int:program) ldap_type_proc = ([

    U(0,1) : Standards.ASN1.Types.Boolean,
    U(0,2) : Standards.ASN1.Types.Integer,
    U(0,3) : Standards.ASN1.Types.BitString,
    U(0,4) : Standards.ASN1.Types.OctetString,
    U(0,5) : Standards.ASN1.Types.Null,
    U(0,6) : Standards.ASN1.Types.Identifier,
    // 9 : asn1_real,
    U(0,10) : Standards.ASN1.Types.Enumerated,
    U(0,16) : Standards.ASN1.Types.Sequence,
    U(0,17) : Standards.ASN1.Types.Set,
    U(0,19) : Standards.ASN1.Types.PrintableString,
    U(0,20) : Standards.ASN1.Types.TeletexString,
    U(0,23) : Standards.ASN1.Types.UTC,

    U(1,1) : asn1_application_sequence,	// [1] BindResponse
    U(1,2) : 0,				// [2] UnbindRequest
    U(1,3) : asn1_application_sequence,	// [3] SearchRequest
    U(1,4) : asn1_application_sequence,	// [4] SearchResultEntry
    U(1,5) : asn1_application_sequence,	// [5] SearchResultDone
    U(1,6) : asn1_application_sequence,	// [6] ModifyRequest
    U(1,7) : asn1_application_sequence,	// [7] ModifyResponse
    U(1,8) : asn1_application_sequence,	// [8] AddRequest
    U(1,9) : asn1_application_sequence,	// [9] AddResponse
    U(1,11) : asn1_application_sequence,	// [11] DelResponse
    U(1,13) : asn1_application_sequence,	// [13] ModifyDNResponse
    U(1,15) : asn1_application_sequence,	// [15] CompareResponse
    U(1,19) : asn1_application_sequence,	// [19] SearchResultReference
    U(1,23) : asn1_application_sequence,	// [23] ExtendedRequest
    U(1,24) : asn1_application_sequence,	// [24] ExtendedResponse

    U(2,0) : asn1_sequence,
]);

object|mapping ldap_der_decode(string data)
{
  return der_decode(ADT.struct(data), ldap_type_proc);
}

// ------------- end of ASN.1 API hack -----------------------------
