#pike __REAL_VERSION__
#pragma strict_types

//! RIPEMD160 is a hash function designed by Hans Dobbertin, Antoon
//! Bosselaers, and Bart Preneel, as a strengthened version of RIPEMD
//! (which, like MD4 and MD5, fails the collision-resistance
//! requirement). It produces message digests of 160 bits, or 20
//! octets.

#if constant(Nettle.RIPEMD160)

inherit Nettle.RIPEMD160;

#else
constant this_program_does_not_exist=1;
#endif
