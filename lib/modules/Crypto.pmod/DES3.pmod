#pike __REAL_VERSION__

//! The inadequate key size of DES has already been mentioned. One way
//! to increase the key size is to pipe together several DES boxes
//! with independent keys. It turns out that using two DES ciphers is
//! not as secure as one might think, even if the key size of the
//! combination is a respectable 112 bits.
//!
//! The standard way to increase DES's key size is to use three DES
//! boxes. The mode of operation is a little peculiar: the middle DES
//! box is wired in the reverse direction. To encrypt a block with
//! DES3, you encrypt it using the first 56 bits of the key, then
//! decrypt it using the middle 56 bits of the key, and finally
//! encrypt it again using the last 56 bits of the key. This is known
//! as "ede" triple-DES, for "encrypt-decrypt-encrypt".
//!
//! The "ede" construction provides some backward compatibility, as
//! you get plain single DES simply by feeding the same key to all
//! three boxes. That should help keeping down the gate count, and the
//! price, of hardware circuits implementing both plain DES and DES3.
//!
//! DES3 has a key size of 168 bits, but just like plain DES, useless
//! parity bits are inserted, so that keys are represented as 24
//! octets (192 bits). As a 112 bit key is large enough to make brute
//! force attacks impractical, some applications uses a "two-key"
//! variant of triple-DES. In this mode, the same key bits are used
//! for the first and the last DES box in the pipe, while the middle
//! box is keyed independently. The two-key variant is believed to be
//! secure, i.e. there are no known attacks significantly better than
//! brute force.

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.DES3_Info;
inherit .Cipher;

.CipherState `()() { return Nettle.DES3_State(); }
