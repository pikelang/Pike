#pike __REAL_VERSION__

//! MD4 is a message digest function constructed by Ronald Rivest, and
//! is described in RFC 1320. It outputs message digests of 128 bits,
//! or 16 octets.

#if constant(Nettle.MD4_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.MD4_Info;
inherit .Hash;

.HashState `()() { return Nettle.MD4_State(); }

#endif
