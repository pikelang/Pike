#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.GOST94CP)

//! The GOST94 or GOST R 34.11-94 hash algorithm is a Soviet-era
//! algorithm used in Russian government standards. This is the
//! "CryptoPro" variant.
//!
//! @seealso
//!   @[GOST94]

inherit Nettle.GOST94CP;

string name() { return "gost94cp"; }
