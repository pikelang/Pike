#pike __REAL_VERSION__

//! MD2 is a message digest function constructed by Burton Kaliski,
//! and is described in RFC 1319. It outputs message digests of 128
//! bits, or 16 octets.

#if constant(Nettle.MD2_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.MD2_Info;
inherit .Hash;

.HashState `()() { return Nettle.MD2_State(); }

#endif
