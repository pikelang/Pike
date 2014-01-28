//! ECDSA operations.

#pike __REAL_VERSION__
#pragma strict_types

#if constant(Crypto.ECC.Curve)

import Standards.ASN1.Types;

//! Lookup from ASN.1 DER encoded ECC named curve identifier to
//! the corresponding @[Crypto.ECC.Curve].
protected mapping(string:Crypto.ECC.Curve) curve_lookup = ([]);

protected void create()
{
  // Initialize the curve_lookup table.
  foreach(values(Crypto.ECC), mixed c) {
    if (!objectp(c) || !functionp(c->pkcs_named_curve_id)) continue;
    curve_lookup[c->pkcs_named_curve_id()->get_der()] = c;
  }
}

//! Get the ECC curve corresponding to an ASN.1 DER encoded
//! named curve identifier.
//!
//! @returns
//!   Returns @[UNDEFINED] if the curve is unsupported.
Crypto.ECC.Curve parse_ec_parameters(string ec_parameters)
{
  return curve_lookup[ec_parameters];
}

//! Create a DER-coded ECPrivateKey structure
//! @param ecdsa
//!   @[Crypto.ECC.Curve()->ECDSA] object.
//! @returns
//!   ASN.1 coded ECPrivateKey structure as specified in RFC 5915 section 3.
string(8bit) private_key(Crypto.ECC.SECP_521R1.ECDSA ecdsa)
{
  // ECPrivateKey ::= SEQUENCE {
  return Sequence(({
		    // version        INTEGER,
		    1,
		    // privateKey     OCTET STRING,
		    OctetString(sprintf("%*c",
					(ecdsa->size() + 7)>>3,
					ecdsa->get_private_key())),
		    // parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
		    ContextTag0(ecdsa->curve()->pkcs_ec_parameters()),
		    // publicKey  [1] BIT STRING OPTIONAL
		    ContextTag1(BitString(ecdsa->get_public_key())),
		  }));
  // }
}

//! Get an initialized ECDSA object from an ECC curve and
//! an ASN.1 DER encoded ec private key.
//!
//! As specified in RFC 5915 section 3.
Crypto.ECC.SECP_521R1.ECDSA parse_ec_private_key(string ec_private_key)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(ec_private_key);
  if (!a || a->type_name != "SEQUENCE" || (sizeof(a->elements) < 2) ||
      (a->elements[0]->type_name != "INTEGER") ||
      (a->elements[0]->value != 1) ||
      (a->elements[1]->type_name != "OCTET STRING")) {
    return UNDEFINED;
  }

  Crypto.ECC.SECP_521R1.ECDSA res = c->ECDSA();
  res->set_private_key(Gmp.mpz(a->elements[1]->value, 256));
  return res;
}
#else
constant this_program_does_not_exist=1;
#endif
