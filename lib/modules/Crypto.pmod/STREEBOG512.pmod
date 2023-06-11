#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.STREEBOG512)

//! The STREEBOG512 algorithm is a member of the Streebog (GOST R 34.11-2012)
//! family of hash algorithms, and is used in Russian government standards.
//!
//! @seealso
//!   @[STREEBOG256]

inherit Nettle.STREEBOG512;

string name() { return "streebog512"; }
