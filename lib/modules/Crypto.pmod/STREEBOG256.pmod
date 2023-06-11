#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.STREEBOG256)

//! The STREEBOG256 algorithm is a member of the Streebog (GOST R 34.11-2012)
//! family of hash algorithms, and is used in Russian government standards.
//!
//! @seealso
//!   @[STREEBOG512]

inherit Nettle.STREEBOG256;

string name() { return "streebog256"; }
