#pike __REAL_VERSION__

// LDAP client protocol implementation for Pike.
//
// Honza Petrous, hop@unibase.cz
//

#include "ldap_globals.h"

import Standards.ASN1.Types;

Object asn1_factory(program(Object) p, int cls, int tag, mixed arg)
{
  Object o = p(arg);
  o->cls = cls;
  o->tag = tag;
  return o;
}

#define U(C,T) make_combined_tag(C,T)

// Mapping from class+tag to program.
protected mapping(int:program) ldap_type_proc = ([

    U(1,1) : Sequence,	// [1] BindResponse
    U(1,2) : 0,		// [2] UnbindRequest
    U(1,3) : Sequence,	// [3] SearchRequest
    U(1,4) : Sequence,	// [4] SearchResultEntry
    U(1,5) : Sequence,	// [5] SearchResultDone
    U(1,6) : Sequence,	// [6] ModifyRequest
    U(1,7) : Sequence,	// [7] ModifyResponse
    U(1,8) : Sequence,	// [8] AddRequest
    U(1,9) : Sequence,	// [9] AddResponse
    U(1,11) : Sequence,	// [11] DelResponse
    U(1,13) : Sequence,	// [13] ModifyDNResponse
    U(1,15) : Sequence,	// [15] CompareResponse
    U(1,19) : Sequence,	// [19] SearchResultReference
    U(1,23) : Sequence,	// [23] ExtendedRequest
    U(1,24) : Sequence,	// [24] ExtendedResponse

    U(2,0) : Sequence,
]);

Object ldap_der_decode(string data)
{
  return Standards.ASN1.Decode.simple_der_decode(data, ldap_type_proc);
}
