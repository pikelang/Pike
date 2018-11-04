#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.ARCFOUR)

//! RC4 is a stream cipher, sometimes refered to as Arcfour, and while
//! very fast isn't considered secure anymore.
//!
//! @note
//! The key setup of RC4 is quite weak, so you should never use keys
//! with structure, such as ordinary passwords. If you have keys that
//! don't look like random bit strings, and you want to use RC4,
//! always hash the key before feeding it to RC4.
//!
//! The first few thousand bits have a slight bias, so it is not
//! uncommon for applications to encrypt a few kilobytes of dummy data
//! before actual encryption.

inherit Nettle.ARCFOUR : pre;

string(7bit) name() { return "RC4"; }

class State
{
  inherit pre::State;

  string(7bit) name() { return "RC4"; }
}
