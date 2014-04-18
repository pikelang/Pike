#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.CHACHA)

//! ChaCha is a stream cipher by D. J. Bernstein.

inherit Nettle.CHACHA;
