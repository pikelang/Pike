#pike __REAL_VERSION__

//! The IDEA(tm) block cipher is covered by patents held by ETH and a
//! Swiss company called Ascom-Tech AG. The Swiss patent number is
//! PCT/CH91/00117, the European patent number is EP 0 482 154 B1, and
//! the U.S. patent number is US005214703. IDEA(tm) is a trademark of
//! Ascom-Tech AG. There is no license fee required for noncommercial
//! use.

#if constant(Nettle.IDEA_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.IDEA_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.IDEA_State(); }

#endif
