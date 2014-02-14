//! DSA operations as defined in RFC-2459.

/* NOTE: Unlike the functions in RSA.pmod, this function returns
 * an object rather than a string. */

#pike __REAL_VERSION__
// #pragma strict_types
#require constant(Crypto.DSA)

import Standards.ASN1.Types;

//! Returns the AlgorithmIdentifier as defined in RFC5280 section
//! 4.1.1.2. Optionally the DSA parameters are included, if a DSA
//! object is given as argument.
Sequence algorithm_identifier(Crypto.DSA|void dsa)
{
  return
    dsa ? dsa->pkcs_algorithm_identifier()
    : Sequence( ({ .Identifiers.dsa_id }) ); // FIXME: Shouldn't there be a Null() here?
}

//! Generates the DSAPublicKey value, as specified in RFC2459.
string public_key(Crypto.DSA dsa)
{
  return Integer(dsa->get_y())->get_der();
}


// FIXME
// Niels: I don't know if this format interoperates with anything else.
// Nilsson: PKCS#11 v2.01 section 11.9 encodes private key as the ASN.1
// Integers p, q and g. x is however not included. Why does that work?

//!
string private_key(Crypto.DSA dsa)
{
  return Sequence(map( ({ dsa->get_p(), dsa->get_q(), dsa->get_g(),
			  dsa->get_y(), dsa->get_x() }),
		       Integer))->get_der();
}

//! Decodes a DER-encoded DSAPublicKey structure.
//! @param key
//!   DSAPublicKey provided in ASN.1 DER-encoded format
//! @param p
//!   Public parameter p, usually transmitted in the algoritm identifier.
//! @param q
//!   Public parameter q, usually transmitted in the algoritm identifier.
//! @param g
//!   Public parameter g, usually transmitted in the algoritm identifier.
//! @returns
//!   @[Crypto.DSA] object
Crypto.DSA parse_public_key(string key, Gmp.mpz p, Gmp.mpz q, Gmp.mpz g)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);
  if(!a || a->type_name!="INTEGER" ) return 0;

  Crypto.DSA dsa = Crypto.DSA();
  dsa->set_public_key(p, q, g, a->value);
  return dsa;
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

//! Creates a SubjectPublicKeyInfo ASN.1 sequence for the given @[dsa]
//! object. See RFC 5280 section 4.1.2.7.
Sequence build_public_key(Crypto.DSA dsa)
{
  return Sequence(({
                    algorithm_identifier(dsa),
                    BitString(public_key(dsa)),
                  }));
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
  case "sha224":
    return Sequence( ({ .Identifiers.dsa_sha224_id }) );
    break;
  case "sha256":
    return Sequence( ({ .Identifiers.dsa_sha256_id }) );
    break;
  }
  return 0;
}
