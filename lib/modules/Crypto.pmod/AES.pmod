#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.AES_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.AES_State(); }
