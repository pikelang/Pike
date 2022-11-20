#pike __REAL_VERSION__
#if constant(Nettle.ECDSA)

//! Elliptic Curve Digital Signing Algorithm
//!

inherit Nettle.ECDSA;

//! Set the elliptic curve to use.
//!
//! @note
//!   Changing this variable clears all other state in the object as well.
this_program set_curve(Crypto.ECC.Curve curve)
{
  ::set_curve(curve);
  return this;
}

//! Set the private key.
//!
//! @note
//!   Throws errors if the key isn't valid for the curve.
this_program set_private_key(object(Gmp.mpz)|int k)
{
  ::set_private_key(k);
  return this;
}

//! Change to the selected point on the curve as public key.
//!
//! @note
//!   Throws errors if the point isn't on the curve.
this_program set_public_key(object(Gmp.mpz)|int x, object(Gmp.mpz)|int y)
{
  ::set_public_key(x, y);
  return this;
}

//! Set the random function, used to generate keys and parameters,
//! to the function @[r].
this_program set_random(function(int:string(8bit)) r)
{
  ::set_random(r);
  return this;
}

//! Generate a new set of private and public keys on the current curve.
this_program generate_key()
{
  ::generate_key();
  return this;
}

//! Signs the @[message] with a PKCS-1 signature using hash algorithm
//! @[h].
string(8bit) pkcs_sign(string(8bit) message, .Hash h)
{
  array sign = map(raw_sign(h->hash(message)), Standards.ASN1.Types.Integer);
  return Standards.ASN1.Types.Sequence(sign)->get_der();
}

#define Object Standards.ASN1.Types.Object

//! Verify PKCS-1 signature @[sign] of message @[message] using hash
//! algorithm @[h].
int(0..1) pkcs_verify(string(8bit) message, .Hash h, string(8bit) sign)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(sign);

  // The signature is the DER-encoded ASN.1 sequence Ecdsa-Sig-Value
  // with the two integers r and s. See RFC 4492 section 5.4.
  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof([array]a->elements) != 2)
      || (sizeof( ([array(object(Object))]a->elements)->type_name -
		  ({ "INTEGER" }))))
    return 0;

  return raw_verify(h->hash(message),
		    [object(Gmp.mpz)]([array(object(Object))]a->elements)[0]->
		      value,
		    [object(Gmp.mpz)]([array(object(Object))]a->elements)[1]->
		      value);
}

#undef Object

#endif
