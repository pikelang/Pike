#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.UMAC96)

//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC96] outputs a digest of 96 bits or 12 octets.
//!
//! @seealso
//!   @[UMAC32], @[UMAC64], @[UMAC128]

inherit Nettle.UMAC96;
