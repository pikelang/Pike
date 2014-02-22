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

//! Check whether two objects are equvivalent.
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
//! See RFC 5280 section 4.1.2.7.
Sequence pkcs_public_key();
#undef Sequence
