#pike __REAL_VERSION__
#pragma strict_types

//! The GOST94 or GOST R 34.11-94 hash algorithm is a Soviet-era
//! algorithm used in Russian government standards, defined in RFC
//! 4357.

#if constant(Nettle.GOST94)

inherit Nettle.GOST94;

string name() { return "gost94"; }

#else
constant this_program_does_not_exist=1;
#endif
