#pike __REAL_VERSION__

//! BLOWFISH is a block cipher designed by Bruce Schneier. It uses a
//! block size of 64 bits (8 octets), and a variable key size, up to
//! 448 bits. It has some weak keys.

#if constant(Nettle.BLOWFISH_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.BLOWFISH_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.BLOWFISH_State(); }

#endif
