#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.ARCFOUR)

//! Arcfour is a stream cipher, also known under the trade marked name
//! RC4, and it is one of the fastest ciphers around. A problem is
//! that the key setup of Arcfour is quite weak, you should never use
//! keys with structure, keys that are ordinary passwords, or
//! sequences of keys like @expr{"secret:1"@}, @expr{"secret:2"@},
//! ..... If you have keys that don't look like random bit strings,
//! and you want to use Arcfour, always hash the key before feeding it
//! to Arcfour.

inherit Nettle.ARCFOUR;
