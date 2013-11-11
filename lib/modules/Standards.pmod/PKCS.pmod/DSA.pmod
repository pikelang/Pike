//! DSA operations as defined in RFC-2459.

/* NOTE: Unlike the functions in RSA.pmod, this function returns
 * an object rather than a string. */

#pike __REAL_VERSION__
// #pragma strict_types

#if constant(Crypto.DSA)

import Standards.ASN1.Types;

//!
Sequence algorithm_identifier(Crypto.DSA|void dsa)
{
  return
    dsa ? Sequence( ({ .Identifiers.dsa_id,
		       Sequence( ({ Integer(dsa->get_p()),
				    Integer(dsa->get_q()),
				    Integer(dsa->get_g()) }) ) }) )
    : Sequence( ({ .Identifiers.dsa_id }) );
}

//! Generates the DSAPublicKey value, as specified in RFC2459.
string public_key(Crypto.DSA dsa)
{
  return Integer(dsa->get_y())->get_der();
}

/* I don't know if this format interoperates with anything else */
//!
string private_key(Crypto.DSA dsa)
{
  return Sequence(map( ({ dsa->get_p(), dsa->get_q(), dsa->get_g(),
			  dsa->get_y(), dsa->get_x() }),
		       Integer))->get_der();
}

//!
Crypto.DSA parse_private_key(string key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);

  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 5)
      || (sizeof(a->elements->type_name - ({ "INTEGER" }))) )
    return 0;

  Crypto.DSA dsa = Crypto.DSA();
  dsa->set_public_key(@ a->elements[..3]->value);
  dsa->set_private_key(a->elements[4]->value);

  return dsa;
}

//! Returns the PKCS-1 algorithm identifier for DSA and the provided
//! hash algorithm. Only @[SHA1] supported.
Sequence signature_algorithm_id(Crypto.Hash hash)
{
  switch(hash->name())
  {
  case "sha1":
    return Sequence( ({ .Identifiers.dsa_sha_id }) );
    break;
  }
  return 0;
}

#else
constant this_program_does_not_exist=1;
#endif
