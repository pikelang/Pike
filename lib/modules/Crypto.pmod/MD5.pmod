#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.MD5_Info;
inherit .Hash;

.HashState `()() { return Nettle.MD5_State(); }
