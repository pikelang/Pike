#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.GOST94)

//! The GOST94 or GOST R 34.11-94 hash algorithm is a Soviet-era
//! algorithm used in Russian government standards, defined in RFC
//! 4357.

inherit Nettle.GOST94;

string name() { return "gost94"; }
