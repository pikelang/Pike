#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.Serpent_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.Serpent_State(); }
