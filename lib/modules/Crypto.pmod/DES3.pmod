#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.DES3_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.DES3_State(); }
