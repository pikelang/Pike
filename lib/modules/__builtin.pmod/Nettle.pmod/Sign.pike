//!
//! Base class for cryptographic signature algorithms.
//!
//! Typical classes implementing this API are @[Crypto.RSA], @[Crypto.DSA] and
//! @[Crypto.ECC.Curve.ECDSA].
//!

// PKCS-1
#define Sequence Standards.ASN1.Types.Sequence

//! Returns the printable name of the signing algorithm.
string(7bit) name();

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
