#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.IDEA_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.IDEA_State(); }
