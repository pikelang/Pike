#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.UMAC128)

//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC128] outputs a digest of 128 bits or 16 octets.
//!
//! @seealso
//!   @[UMAC32], @[UMAC64], @[UMAC96]

inherit Nettle.UMAC128;
