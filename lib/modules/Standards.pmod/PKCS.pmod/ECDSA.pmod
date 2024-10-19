//! ECDSA operations.

#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.ECC)

import Standards.ASN1.Types;

//! Lookup from ASN.1 DER encoded ECC named curve identifier to
//! the corresponding @[Crypto.ECC.Curve].
protected mapping(string:Crypto.ECC.Curve) curve_lookup = ([]);

protected void create()
{
  // Initialize the curve_lookup table.
  foreach(values(Crypto.ECC), mixed c) {
    if (!objectp(c) || !functionp(([object]c)->pkcs_named_curve_id)) continue;
    object(Crypto.ECC.Curve) curve = [object(Crypto.ECC.Curve)]c;
    object(Standards.ASN1.Types.Identifier)|zero id = curve->pkcs_named_curve_id();
    if (!id) continue;
    curve_lookup[id->get_der()] = curve;
  }
}

//! Get the ECC curve corresponding to an ASN.1 DER encoded
//! named curve identifier.
//!
//! @returns
//!   Returns @[UNDEFINED] if the curve is unsupported.
object(Crypto.ECC.Curve)|zero parse_ec_parameters(string ec_parameters)
{
  return curve_lookup[ec_parameters];
}

//!
variant object(Crypto.ECC.Curve)|zero
  parse_ec_parameters(Standards.ASN1.Types.Identifier|void named_curve)
{
  if (!named_curve) return UNDEFINED;
  return parse_ec_parameters(named_curve->get_der());
}

//! Create a DER-coded ECPrivateKey structure
//! @param ecdsa
//!   @[Crypto.ECC.Curve()->ECDSA] object.
//! @returns
//!   ASN.1 coded ECPrivateKey structure as specified in
//!   @rfc{5915:3@}.
string(8bit) private_key(Crypto.ECC.SECP_521R1.ECDSA ecdsa)
{
  // ECPrivateKey ::= SEQUENCE {
  return Sequence(({
		    // version        INTEGER,
		    1,
		    // privateKey     OCTET STRING,
		    OctetString(sprintf("%*c",
					[int(0..)]((ecdsa->size() + 7)>>3),
					ecdsa->get_private_key())),
		    // parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
		    TaggedType0(ecdsa->get_curve()->pkcs_ec_parameters()),
		    // publicKey  [1] BIT STRING OPTIONAL
		    TaggedType1(BitString(ecdsa->get_public_key())),
		  }))->get_der();
  // }
}

//! Get an initialized ECDSA object from an ECC curve and
//! an ASN.1 ec private key sequence.
//!
//! As specified in @rfc{5915:3@}.
object(Crypto.ECC.SECP_521R1.ECDSA)|zero
  parse_private_key(Sequence a, Crypto.ECC.Curve|void c)
{
  if ((sizeof(a->elements) < 2) ||
      (a->elements[0]->type_name != "INTEGER") ||
      (a->elements[0]->value != 1) ||
      (a->elements[1]->type_name != "OCTET STRING")) {
    return UNDEFINED;
  }

  if ((sizeof(a->elements) > 2) &&
      (a->elements[2]->type_name == "EXPLICIT") &&
      !a->elements[2]->get_tag()) {
    Sequence b = [object(Sequence)]a->elements[2];
    if ((sizeof(b->elements) == 1)) {
      c = parse_ec_parameters([string(8bit)]b->elements[0]->get_der());
    }
  }

  if (!c) return UNDEFINED;

  Crypto.ECC.SECP_521R1.ECDSA res = c->ECDSA();
  res->set_private_key(Gmp.mpz([string(8bit)]a->elements[1]->value, 256));
  return res;
}

//! Get an initialized ECDSA object from an ECC curve and
//! an ASN.1 DER encoded ec private key.
//!
//! As specified in @rfc{5915:3@}.
variant object(Crypto.ECC.SECP_521R1.ECDSA)|zero
  parse_private_key(string(8bit) ec_private_key, Crypto.ECC.Curve|void c)
{
  Object o =
    Standards.ASN1.Decode.simple_der_decode(ec_private_key);
  if (!o || o->type_name != "SEQUENCE") return UNDEFINED;
  return parse_private_key([object(Sequence)]o, c);
}

//! Create a DER-coded ECPublicKey structure
//! @param ecdsa
//!   @[Crypto.ECC.Curve()->ECDSA] object.
//! @returns
//!   ASN.1 coded ECPublicKey structure as specified in
//!   @rfc{5480:2@}.
string(8bit) public_key(Crypto.ECC.SECP_521R1.ECDSA ecdsa)
{
  return ecdsa->pkcs_public_key()->get_der();
}

//! Get an initialized ECDSA object from an ECC curve and an ec public key.
//!
//! See @rfc{5280:4.1.2.7@} and @rfc{5480:2.2@}.
Crypto.ECC.SECP_521R1.ECDSA parse_public_key(string(8bit) key, Crypto.ECC.Curve c)
{
  Crypto.ECC.SECP_521R1.ECDSA res = c->ECDSA();
  res->set_public_key(key);
  return res;
}
