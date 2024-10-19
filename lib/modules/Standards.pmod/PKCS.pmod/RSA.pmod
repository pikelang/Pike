
//! RSA operations and types as described in PKCS-1.

#pike __REAL_VERSION__
// #pragma strict_types
#require constant(Nettle.Hash)

import Standards.ASN1.Types;

//! Returns the AlgorithmIdentifier as defined in @rfc{5280:4.1.1.2@}.
Sequence algorithm_identifier()
{
  return Sequence( ({ .Identifiers.rsa_id, Null() }) );
}

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
//!   RSAPublicKey provided in ASN.1 DER-encoded format
//! @returns
//!   @[Crypto.RSA] object
Crypto.RSA.State parse_public_key(string key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);

  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 2)
      || (sizeof(a->elements->type_name - ({ "INTEGER" }))) )
    return UNDEFINED;

  Crypto.RSA.State rsa = Crypto.RSA();
  rsa->set_public_key(a->elements[0]->value, a->elements[1]->value);
  return rsa;
}

//! Decode a RSAPrivateKey structure
//! @param key
//!   RSAPrivateKey provided in ASN.1 format
//! @returns
//!   @[Crypto.RSA] object
Crypto.RSA.State parse_private_key(Sequence seq)
{
  if ((sizeof(seq->elements) != 9)
      || (sizeof(seq->elements->type_name - ({ "INTEGER" })))
      || seq->elements[0]->value)
    return UNDEFINED;

  Crypto.RSA.State rsa = Crypto.RSA();
  rsa->set_public_key(seq->elements[1]->value, seq->elements[2]->value);
  rsa->set_private_key(seq->elements[3]->value, seq->elements[4..]->value);
  return rsa;
}

//! Decode a DER-coded RSAPrivateKey structure
//! @param key
//!   RSAPrivateKey provided in ASN.1 DER-encoded format
//! @returns
//!   @[Crypto.RSA] object
variant Crypto.RSA.State parse_private_key(string key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);

  if (!a || (a->type_name != "SEQUENCE"))
    return UNDEFINED;
  return parse_private_key([object(Sequence)]a);
}

//! Creates a SubjectPublicKeyInfo ASN.1 sequence for the given @[rsa]
//! object. See @rfc{5280:4.1.2.7@}.
Sequence build_public_key(Crypto.RSA rsa)
{
  return Sequence(({
                    algorithm_identifier(),
                    BitString( public_key(rsa) ),
                  }));
}

//! Creates a PrivateKeyInfo ASN.1 sequence for the given @[rsa]
//! object. See @rfc{5208:5@}.
Sequence build_private_key(Crypto.RSA rsa)
{
  return Sequence(({
                    Integer(0), // Version
                    algorithm_identifier(),
                    OctetString( private_key(rsa) ),
                  }));
}

//! Returns the PKCS-1 algorithm identifier for RSA and the provided
//! hash algorithm. One of @[MD2], @[MD5], @[SHA1], @[SHA256],
//! @[SHA384] or @[SHA512].
object(Sequence)|zero signature_algorithm_id(Crypto.Hash hash)
{
  switch(hash->name())
  {
  case "md2":
    return Sequence( ({ .Identifiers.rsa_md2_id, Null() }) );
    break;
  case "md5":
    return Sequence( ({ .Identifiers.rsa_md5_id, Null() }) );
    break;
  case "sha1":
    return Sequence( ({ .Identifiers.rsa_sha1_id, Null() }) );
    break;
  case "sha256":
    return Sequence( ({ .Identifiers.rsa_sha256_id, Null() }) );
    break;
  case "sha384":
    return Sequence( ({ .Identifiers.rsa_sha384_id, Null() }) );
    break;
  case "sha512":
    return Sequence( ({ .Identifiers.rsa_sha512_id, Null() }) );
    break;
  case "sha512-224":
    return Sequence( ({ .Identifiers.rsa_sha512_224_id, Null() }) );
    break;
  case "sha512-256":
    return Sequence( ({ .Identifiers.rsa_sha512_256_id, Null() }) );
    break;
  }
  return 0;
}

//! Returns the PKCS-1 algorithm identifier for RSASSA-PSS and the
//! provided hash algorithm and saltlen.
//!
//! @seealso
//!   @rfc{3447:A.2.3@}
Sequence pss_signature_algorithm_id(Crypto.Hash hash,
				    int(0..)|void saltlen)
{
  if (undefinedp(saltlen)) saltlen = 20;
  array params = ({});
  if (hash->name() != "sha1") {
    array hash_alg = ({ hash->pkcs_hash_id() });
    if (has_prefix(hash->name(), "md")) {
      // RFC 3447 Appendix C:
      //
      //   When id-md2 and id-md5 are used in an AlgorithmIdentifier the
      //   parameters MUST be present and MUST be NULL.
      //
      //   When id-sha1, id-sha256, id-sha384 and id-sha512 are used in an
      //   AlgorithmIdentifier the parameters (which are optional) SHOULD
      //   be omitted. However, an implementation MUST also accept
      //   AlgorithmIdentifier values where the parameters are NULL.
      hash_alg += ({ Null() });
    }
    params = ({ TaggedType0(Sequence(hash_alg)),
		TaggedType1(Sequence(({ .Identifiers.mgf1_id,
					Sequence(hash_alg), }))),
    });
  }
  if (saltlen != 20) {
    params += ({ TaggedType2(Integer(saltlen)) });
  }

  /* NB: We skip the last item since we only support the default anyway.
   *
   * params += ({ TaggedType3(Integer(1)) });
   */
  return Sequence(({ .Identifiers.rsassa_pss_id, Sequence(params), }));
}

//! Returns the PKCS-1 algorithm identifier for RSAES-OAEP and the
//! provided hash algorithm and label.
//!
//! @seealso
//!   @rfc{3447:A.2.1@}
Sequence oaep_algorithm_id(Crypto.Hash hash, string(8bit)|void label)
{
  if (!label) label = "";
  array params = ({});
  if (hash->name() != "sha1") {
    array hash_alg = ({ hash->pkcs_hash_id() });
    if (has_prefix(hash->name(), "md")) {
      // RFC 3447 Appendix C:
      //
      //   When id-md2 and id-md5 are used in an AlgorithmIdentifier the
      //   parameters MUST be present and MUST be NULL.
      //
      //   When id-sha1, id-sha256, id-sha384 and id-sha512 are used in an
      //   AlgorithmIdentifier the parameters (which are optional) SHOULD
      //   be omitted. However, an implementation MUST also accept
      //   AlgorithmIdentifier values where the parameters are NULL.
      hash_alg += ({ Null() });
    }
    params = ({ TaggedType0(Sequence(hash_alg)),
		TaggedType1(Sequence(({ .Identifiers.mgf1_id,
					Sequence(hash_alg), }))),
    });
  }
  if (label != "") {
    params += ({ TaggedType2(Sequence(({ .Identifiers.pspecified_id,
					 OctetString(label) }))),
    });
  }
  return Sequence(({ .Identifiers.rsaes_oaep_id, Sequence(params), }));
}
