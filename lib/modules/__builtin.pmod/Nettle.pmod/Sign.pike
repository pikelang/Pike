#pike __REAL_VERSION__
#pragma strict_types

//!
//! Base class for cryptographic signature algorithms.
//!
//! Typical classes implementing this API are @[Crypto.RSA], @[Crypto.DSA] and
//! @[Crypto.ECC.Curve.ECDSA].
//!

//! Returns the printable name of the signing algorithm.
string(7bit) name();

//! Returns the number of bits in the private key.
int(0..) key_size();

//! Check whether the public key is the same in
//! two objects.
//!
//! @note
//!   This function differs from @[_equal()] in that only the
//!   public key is regarded, and that it only needs to regard
//!   objects implementing @[Crypto.Sign].
//!
//! @seealso
//!   @[_equal()]
int(0..1) public_key_equal(this_program other);

//! Check whether two objects are equivalent.
//!
//! This includes checking both the public and private keys.
//!
//! @seealso
//!   @[public_key_equal()]
protected int(0..1) _equal(mixed x);

// PKCS-1
#define Sequence Standards.ASN1.Types.Sequence

//! Signs the @[message] with a PKCS-1 signature using hash algorithm
//! @[h].
string(8bit) pkcs_sign(string(8bit) message, .Hash h);

//! Verify PKCS-1 signature @[sign] of message @[message] using hash
//! algorithm @[h].
int(0..1) pkcs_verify(string(8bit) message, .Hash h, string(8bit) sign);

//! Returns the PKCS-1 algorithm identifier for the signing algorithm with
//! the provided hash algorithm.
Sequence pkcs_signature_algorithm_id(.Hash hash);

//! Returns the PKCS-1 AlgorithmIdentifier.
Sequence pkcs_algorithm_identifier();

//! Creates a SubjectPublicKeyInfo ASN.1 sequence for the object.
//! See @rfc{5280:4.1.2.7@}.
Sequence pkcs_public_key();

// JOSE JWS (RFC 7515).

//! Generate a JOSE JWK mapping representation of the object.
//!
//! @param private_key
//!   If true, include private fields in the result.
//!
//! @returns
//!   Returns a mapping as per @rfc{7517:4@} on success,
//!   and @expr{0@} (zero) on failure (typically that
//!   the object isn't initialized properly, or that
//!   it isn't supported by JWK).
//!
//! @seealso
//!   @[Web.encode_jwk()], @[Web.decode_jwk()], @rfc{7517:4@}
mapping(string(7bit):string(7bit)) jwk(int(0..1)|void private_key)
{
  return 0;
}

//! Generate a JOSE JWK Thumbprint of the object.
//!
//! @param h
//!   Hash algorithm to use.
//!
//! @returns
//!   Returns the thumbprint (ie hash of the public fields) on success,
//!   and @expr{0@} (zero) on failure (typically that the object isn't
//!   initialized properly, or that it isn't supported by JWK).
//!
//! A typical use for this function is to generate a kid (key ID)
//! value (cf @rfc{7638:1@}.
//!
//! @seealso
//!   @[jwk()], @rfc{7638@}
string(8bit) jwk_thumbprint(.Hash h)
{
  mapping(string(7bit):string(7bit)) public_jwk = jwk();
  if (!public_jwk) return 0;
  // NB: For the fields used in JWK, the Standards.JSON.PIKE_CANONICAL
  //     behavior is the same as the one specified in RFC 7638 3.3.
  return h->hash([string(7bit)]Standards.JSON.encode(public_jwk,
						     Standards.JSON.PIKE_CANONICAL));
}

//! Signs the @[message] with a JOSE JWS signature using hash
//! algorithm @[h] and JOSE headers @[headers].
//!
//! @param message
//!   Message to sign.
//!
//! @param headers
//!   JOSE headers to use. Typically a mapping with a single element
//!   @expr{"typ"@}.
//!
//! @param h
//!   Hash algorithm to use. Valid hashes depend on the signature
//!   algorithm. The default value depends on the signature algorithm.
//!
//! @returns
//!   Returns the signature on success, and @expr{0@} (zero)
//!   on failure (typically that either the hash algorithm
//!   is invalid for this signature algorithm),
//!
//! @note
//!   The default implementation returns @expr{0@} for all parameters,
//!   and can thus serve as a fallback for signature algorithms that
//!   don't support or aren't supported by JWS (eg @[Crypto.DSA]).
//!
//! @seealso
//!   @[jose_decode()], @[pkcs_sign()], @rfc{7515@}
string(7bit) jose_sign(string(8bit) message,
		       mapping(string(7bit):string(7bit)|int)|void headers,
		       .Hash|void h)
{
  return 0;
}

//! Verify and decode a JOSE JWS signed value.
//!
//! @param jws
//!   A JSON Web Signature as returned by @[jose_sign()].
//!
//! @returns
//!   Returns @expr{0@} (zero) on failure, and an array
//!   @array
//!     @elem mapping(string(7bit):string(7bit)|int) 0
//!       The JOSE header.
//!     @elem string(8bit) 1
//!       The signed message.
//!   @endarray
//!
//! @note
//!   The default implementation returns @expr{0@} for all parameters,
//!   and can thus serve as a fallback for signature algorithms that
//!   don't support or aren't supported by JWS (eg @[Crypto.DSA]).
//!
//! @seealso
//!   @[jose_sign()], @[pkcs_verify()], @rfc{7515@}
array(mapping(string(7bit):string(7bit)|int)|
      string(8bit)) jose_decode(string(7bit) jws)
{
  return 0;
}

#undef Sequence
