#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.Twofish_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.Twofish_State(); }

