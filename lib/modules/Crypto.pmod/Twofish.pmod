#pike __REAL_VERSION__

//! Another @[AES] finalist, this one designed by Bruce Schneier and
//! others.

#if constant(Nettle.Twofish_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.Twofish_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.Twofish_State(); }

#endif
