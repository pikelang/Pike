#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.UMAC64)

//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC64] outputs a digest of 64 bits or 8 octets.
//!
//! @seealso
//!   @[UMAC32], @[UMAC96], @[UMAC128]

inherit Nettle.UMAC64;
