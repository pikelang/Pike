#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.SHA1_Info;
inherit .Hash;

.HashState `()() { return Nettle.SHA1_State(); }
