//!
//! Public-Key Cryptography Standards (PKCS).
//!
//! This is the Pike API for dealing with a set of standards
//! initially published by RSA Security Inc, and later by IETF
//! and others in various RFCs.
//!
//! @seealso
//!   @[Standards.ASN1], @[Crypto],
//!   @rfc{2314@}, @rfc{2459@}, @rfc{2986@}, @rfc{3279@}, @rfc{3280@},
//!   @rfc{4055@}, @rfc{4985@}, @rfc{5208@}, @rfc{5280@}, @rfc{5480@},
//!   @rfc{5639@}, @rfc{5915@}, @rfc{7292@}

import Standards.ASN1.Types;

private object RSA;
private object DSA;
#if constant(Crypto.ECC.Curve)
private object ECDSA;
#endif /* Crypto.ECC.Curve */

//! Parse a PKCS#8 PrivateKeyInfo (cf @rfc{5208:5@}).
//!
//! @seealso
//!   @[RSA.parse_private_key()], @[DSA.parse_private_key()]
Crypto.Sign.State parse_private_key(Sequence seq)
{
  if ((sizeof(seq->elements) < 3) ||
      (sizeof(seq->elements) > 4) ||
      (seq->elements[1]->type_name != "SEQUENCE") ||
      (seq->elements[2]->type_name != "OCTET STRING") ||
      (seq->elements[0]->type_name != "INTEGER") ||
      (seq->elements[0]->value) ||	// Version 0.
      (sizeof(seq->elements[1]->elements) != 2) ||
      (seq->elements[1]->elements[0]->type_name != "OBJECT IDENTIFIER"))
    return UNDEFINED;

  // FIXME: Care about the attribute set (element 4)?
  Identifier alg_id = seq->elements[1]->elements[0];
  if (alg_id == .Identifiers.rsa_id) {
    if(!RSA)
      RSA = master()->resolv("Standards.PKCS.RSA");
    return RSA.parse_private_key(seq->elements[2]->value);
  }
  if (alg_id == .Identifiers.dsa_id) {
    if(!DSA)
      DSA = master()->resolv("Standards.PKCS.DSA");
    return DSA.parse_private_key(seq->elements[2]->value);
  }
#if constant(Crypto.ECC.Curve)
  // RFC 5915:1a.
  if ((alg_id == .Identifiers.ec_id) ||
      (alg_id == .Identifiers.ec_dh_id) ||
      (alg_id == .Identifiers.ec_mqw_id)) {
    if(!ECDSA)
      ECDSA = master()->resolv("Standards.PKCS.ECDSA");
    Crypto.ECC.Curve curve =
      ECDSA.parse_ec_parameters(seq->elements[1]->elements[1]);
    return ECDSA.parse_private_key(seq->elements[2]->value, curve);
  }
#endif /* Crypto.ECC.Curve */
  return UNDEFINED;
}

//!
variant Crypto.Sign.State parse_private_key(string key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(key);

  if (!a || (a->type_name != "SEQUENCE"))
    return UNDEFINED;
  return parse_private_key([object(Sequence)]a);
}

