#pike __REAL_VERSION__
#pragma strict_types

//! MD5 is a message digest function constructed by Ronald Rivest, and
//! is described in RFC 1321. It outputs message digests of 128 bits,
//! or 16 octets.

#if constant(Nettle) && constant(Nettle.MD5_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.MD5_Info;
inherit .Hash;

.HashState `()() { return Nettle.MD5_State(); }

// urn:oid:1.2.840.113549.2.5
string asn1_id() { return "*\206H\206\367\r\2\5"; }

//! This is a convenience alias for @[Nettle.crypt_md5()],
//! that uses the same API as the other hashes.
//!
//! @note
//!   The @[rounds] parameter is currently ignored.
//!   For forward compatibility, either leave out,
//!   or specify as @expr{1000@}.
//!
//! @seealso
//!   @[Nettle.HashInfo()->crypt_hash()], @[crypt_md5()]
string crypt_hash(string password, string salt, int|void rounds)
{
  return Nettle.crypt_md5(password, salt);
}

#else
constant this_program_does_not_exist=1;
#endif
