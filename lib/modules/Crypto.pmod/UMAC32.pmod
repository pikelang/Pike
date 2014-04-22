#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.UMAC32)

//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC32] outputs a digest of 32 bits or 4 octets.
//!
//! @seealso
//!   @[UMAC64], @[UMAC96], @[UMAC128]

inherit Nettle.UMAC32;
