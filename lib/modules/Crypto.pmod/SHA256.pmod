#pike __REAL_VERSION__

//! SHA256 is another hash function specified by NIST, intended as a
//! replacement for @[SHA], generating larger digests. It outputs hash
//! values of 256 bits, or 32 octets.

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.SHA256_Info;
inherit .Hash;

.HashState `()() { return Nettle.SHA256_State(); }
