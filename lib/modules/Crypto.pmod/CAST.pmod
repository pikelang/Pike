#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.CAST128_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.CAST128_State(); }
