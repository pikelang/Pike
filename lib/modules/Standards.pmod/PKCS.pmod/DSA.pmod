//
// $Id: DSA.pmod,v 1.6 2003/01/27 02:54:02 nilsson Exp $
//

//! DSA operations as defined in RFC-2459.

/* NOTE: Unlike the functions in RSA.pmod, this function returns
 * an object rather than a string. */

#pike __REAL_VERSION__
// #pragma strict_types

#if constant(Gmp.mpz)

import Standards.ASN1.Types;

//!
Sequence algorithm_identifier(Crypto.dsa|void dsa)
{
  return
    dsa ? Sequence( ({ .Identifiers.dsa_id,
		       Sequence( ({ Integer(dsa->p),
				    Integer(dsa->q),
				    Integer(dsa->g) }) ) }) )
    : Sequence( ({ .Identifiers.dsa_id }) );
}

//!
string public_key(Crypto.dsa dsa)
{
  return Integer(dsa->y)->get_der();
}

/* I don't know if this format interoperates with anything else */
//!
string private_key(Crypto.dsa dsa)
{
  return Sequence(map( ({ dsa->p, dsa->q, dsa->g, dsa->y, dsa->x }),
		       Integer))->get_der();
}

//!
Crypto.dsa parse_private_key(string key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);

  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 5)
      || (sizeof(a->elements->type_name - ({ "INTEGER" }))) )
    return 0;

  Crypto.dsa dsa = Crypto.dsa();
  dsa->set_public_key(@ a->elements[..3]->value);
  dsa->set_private_key(a->elements[4]->value);

  return dsa;
}

#endif
