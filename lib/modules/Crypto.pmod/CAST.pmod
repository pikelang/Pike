#pike __REAL_VERSION__
#pragma strict_types

//! CAST-128 is a block cipher, specified in RFC 2144. It uses a 64
//! bit (8 octets) block size, and a variable key size of up to 128
//! bits.

#if constant(Nettle) && constant(Nettle.CAST128_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.CAST128_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.CAST128_State(); }

#else
constant this_program_does_not_exist=1;
#endif
