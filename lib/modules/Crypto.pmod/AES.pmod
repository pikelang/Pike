#pike __REAL_VERSION__

//! AES (American Encryption Standard) is a quite new block cipher,
//! specified by NIST as a replacement for the older DES standard. The
//! standard is the result of a competition between cipher designers.
//! The winning design, also known as RIJNDAEL, was constructed by
//! Joan Daemen and Vincent Rijnmen.
//!
//! Like all the AES candidates, the winning design uses a block size
//! of 128 bits, or 16 octets, and variable key-size, 128, 192 and 256
//! bits (16, 24 and 32 octets) being the allowed key sizes. It does
//! not have any weak keys.

#if constant(Nettle.AES_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.AES_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.AES_State(); }

#endif
