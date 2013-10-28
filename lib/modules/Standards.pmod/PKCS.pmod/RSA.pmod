
//! RSA operations and types as described in PKCS-1.

#pike __REAL_VERSION__
// #pragma strict_types

#if constant(Standards.ASN1.Decode) && constant(Crypto.RSA)

import Standards.ASN1.Types;

//! Create a DER-coded RSAPublicKey structure
//! @param rsa
//!   @[Crypto.RSA] object
//! @returns
//!   ASN1 coded RSAPublicKey structure
string public_key(Crypto.RSA rsa)
{
  return Sequence(({ Integer(rsa->get_n()), Integer(rsa->get_e()) }))
    ->get_der();
}

//! Create a DER-coded RSAPrivateKey structure
//! @param rsa
//!   @[Crypto.RSA] object
//! @returns
//!   ASN1 coded RSAPrivateKey structure
string private_key(Crypto.RSA rsa)
{
  Gmp.mpz n = rsa->get_n();
  Gmp.mpz e = rsa->get_e();
  Gmp.mpz d = rsa->get_d();
  Gmp.mpz p = rsa->get_p();
  Gmp.mpz q = rsa->get_q();

  return Sequence(map(
    ({ 0, n, e, d,
       p, q,
       d % (p - 1), d % (q - 1),
       q->invert(p) % p
    }),
    Integer))->get_der();
}

//! Decode a DER-coded RSAPublicKey structure
//! @param key
//!   RSAPublicKey provided in ASN1 encoded format
//! @returns
//!   @[Crypto.RSA] object
Crypto.RSA parse_public_key(string key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);

  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 2)
      || (sizeof(a->elements->type_name - ({ "INTEGER" }))) )
    return 0;

  Crypto.RSA rsa = Crypto.RSA();
  rsa->set_public_key(a->elements[0]->value, a->elements[1]->value);
  return rsa;
}

//! Decode a DER-coded RSAPrivateKey structure
//! @param key
//!   RSAPrivateKey provided in ASN1 encoded format
//! @returns
//!   @[Crypto.RSA] object
Crypto.RSA parse_private_key(string key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);
  
  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 9)
      || (sizeof(a->elements->type_name - ({ "INTEGER" })))
      || a->elements[0]->value)
    return 0;
  
  Crypto.RSA rsa = Crypto.RSA();
  rsa->set_public_key(a->elements[1]->value, a->elements[2]->value);
  rsa->set_private_key(a->elements[3]->value, a->elements[4..]->value);
  return rsa;
}

Sequence build_rsa_public_key(Crypto.RSA rsa)
{
  return Sequence(({
                    Sequence(({ .Identifiers.rsa_id, Null() })),
                    BitString( public_key(rsa) ),
                  }));
}

//! Returns the PKCS-1 algorithm identifier for RSA and the provided
//! hash algorithm. One of @[MD2], @[MD5] or @[SHA1].
Sequence signature_algorithm_id(Crypto.Hash hash)
{
  switch(hash)
  {
  case Crypto.MD2:
    return Sequence( ({ .Identifiers.rsa_md2_id, Null() }) );
    break;
  case Crypto.MD5:
    return Sequence( ({ .Identifiers.rsa_md5_id, Null() }) );
    break;
  case Crypto.SHA1:
    return Sequence( ({ .Identifiers.rsa_sha1_id, Null() }) );
    break;
  }
  return 0;
}


#else
constant this_program_does_not_exist=1;
#endif
