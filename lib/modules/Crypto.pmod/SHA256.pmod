#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA256)

//! SHA256 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 256 bits, or 32 octets.

inherit Nettle.SHA256;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.sha256_id;
}

//! @module HMAC

//! @ignore
protected class _HMAC
{
//! @endignore

  inherit ::this_program;

  //! JWS algorithm identifier.
  //!
  //! @seealso
  //!   @rfc{7518:3.1@}
  constant jwa = "HS256";

//! @ignore
}
//! @endignore

//! @endmodule HMAC
