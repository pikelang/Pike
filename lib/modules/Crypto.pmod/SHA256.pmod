#pike __REAL_VERSION__

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.SHA256_Info;
inherit .Hash;

.HashState `()() { return Nettle.SHA256_State(); }
