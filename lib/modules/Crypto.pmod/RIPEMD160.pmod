#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.RIPEMD160)

//! RIPEMD160 is a hash function designed by Hans Dobbertin, Antoon
//! Bosselaers, and Bart Preneel, as a strengthened version of RIPEMD
//! (which, like MD4 and MD5, fails the collision-resistance
//! requirement). It produces message digests of 160 bits, or 20
//! octets.

inherit Nettle.RIPEMD160;
