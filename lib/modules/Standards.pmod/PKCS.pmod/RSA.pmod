/* RSA.pmod
 *
 * rsa operations and types as described in PKCS-1 */

#pike __REAL_VERSION__

#if 0
#define WERROR werror
#else
#define WERROR(x)
#endif

#if constant(Standards.ASN1.Decode.simple_der_decode)

import Standards.ASN1.Types;

//! Create a DER-coded RSAPublicKey structure
//! @param rsa
//!   <ref to="Crypto.RSA">Crypto.RSA</ref> object
//! @returns
//!   ASN1 coded RSAPublicKey structure
string public_key(object rsa)
{
  return asn1_sequence(Array.map(
    ({ rsa->get_n(), rsa->get_e() }),
    asn1_integer))->get_der();
}

//! Create a DER-coded RSAPrivateKey structure
//! @param rsa
//!   <ref to="Crypto.RSA">Crypto.RSA</ref> object
//! @returns
//!   ASN1 coded RSAPrivateKey structure
string private_key(object rsa)
{
  object n = rsa->get_n();
  object e = rsa->get_e();
  object d = rsa->get_d();
  object p = rsa->get_p();
  object q = rsa->get_q();

  return asn1_sequence(Array.map(
    ({ 0, n, e, d,
       p, q,
       d % (p - 1), d % (q - 1),
       q->invert(p) % p
    }),
    asn1_integer))->get_der();
}

/* Backwards compatibility */
//! @deprecated public_key
string rsa_public_key(object rsa) { return public_key(rsa); }

//! @deprecated private_key
string rsa_private_key(object rsa) { return private_key(rsa); }

//! Decode a DER-coded RSAPublicKey structure
//! @param key
//!   RSAPublicKey provided in ASN1 encoded format
//! @returns
//!   <ref to="Crypto.RSA">Crypto.RSA</ref> object
object parse_public_key(string key)
{
  // WERROR(sprintf("rsa->parse_public_key: '%s'\n", key));
  object a = Standards.ASN1.Decode.simple_der_decode(key);

  // WERROR(sprintf("rsa->parse_public_key: asn1 = %O\n", a));
  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 2)
      || (sizeof(a->elements->type_name - ({ "INTEGER" }))) )
  {
    //  WERROR("Not a Valid Key!\n");
    return 0;
  }
  object rsa = Crypto.rsa();
  rsa->set_public_key(a->elements[0]->value, a->elements[1]->value);
  return rsa;
}

//! Decode a DER-coded RSAPrivateKey structure
//! @param key
//!   RSAPrivateKey provided in ASN1 encoded format
//! @returns
//!   <ref to="Crypto.RSA">Crypto.RSA</ref> object
object parse_private_key(string key)
{
  WERROR(sprintf("rsa->parse_private_key: '%s'\n", key));
  object a = Standards.ASN1.Decode.simple_der_decode(key);
  
  WERROR(sprintf("rsa->parse_private_key: asn1 = %O\n", a));
  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 9)
      || (sizeof(a->elements->type_name - ({ "INTEGER" })))
      || a->elements[0]->value)
    return 0;
  
  object rsa = Crypto.rsa();
  rsa->set_public_key(a->elements[1]->value, a->elements[2]->value);
  rsa->set_private_key(a->elements[3]->value, a->elements[4..]->value);
  return rsa;
}

object build_rsa_public_key(object rsa)
{
  return asn1_sequence( ({
    asn1_sequence(
      ({ .Identifiers.rsa_id, asn1_null() }) ),
    asn1_bit_string(asn1_sequence(
      ({ asn1_integer(rsa->n), asn1_integer(rsa->e) }) )->get_der()) }) );
}

#endif
