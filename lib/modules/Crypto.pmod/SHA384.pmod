#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA384)

//! SHA384 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 384 bits, or 48 octets.

inherit Nettle.SHA384;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.sha384_id;
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
  constant jwa = "HS384";

//! @ignore
}
//! @endignore

//! @endmodule HMAC
