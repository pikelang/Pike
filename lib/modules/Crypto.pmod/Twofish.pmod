#pike __REAL_VERSION__

//! Another @[AES] finalist, this one designed by Bruce Schneier and
//! others.

#if constant(Nettle) && constant(Nettle.Twofish_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.Twofish_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.Twofish_State(); }

#else
constant this_program_does_not_exist=1;
#endif
