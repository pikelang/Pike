#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.MD4_Info;
inherit .Hash;

.HashState `()() { return Nettle.MD4_State(); }
