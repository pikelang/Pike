#pike __REAL_VERSION__
#pragma strict_types

//! MD4 is a message digest function constructed by Ronald Rivest, and
//! is described in RFC 1320. It outputs message digests of 128 bits,
//! or 16 octets.

#if constant(Nettle) && constant(Nettle.MD4)

inherit Nettle.MD4;

// urn:oid:1.2.840.113549.2.5
string asn1_id() { return "*\206H\206\367\r\2\5"; }

#else
constant this_program_does_not_exist=1;
#endif
