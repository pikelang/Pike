#pike __REAL_VERSION__
#pragma strict_types

//! The Camellia 128-bit block cipher.

#if constant(Nettle) && constant(Nettle.CAMELLIA_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.CAMELLIA_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.CAMELLIA_State(); }

#else
constant this_program_does_not_exist=1;
#endif
