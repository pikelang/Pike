#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.ARCFOUR_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.ARCFOUR_State(); }
