#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.BLOWFISH_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.BLOWFISH_State(); }
