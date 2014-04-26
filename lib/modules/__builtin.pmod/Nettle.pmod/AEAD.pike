#pike __REAL_VERSION__
#pragma strict_types

//! Base class for AEAD (Authenticated Encryption with Associated Data)
//! algorithms.
//!
//! AEAD algorithms behave like a combination of a @[Cipher] and
//! a HMAC.
//!
//! Note that no actual AEAD algorithm is implemented
//! in the base class. They are implemented in classes
//! that inherit this class.

inherit __builtin.Nettle.Cipher;

inherit __builtin.Nettle.__Hash;

//! This is the context for a single incrementally updated AEAD cipher.
//!
//! Most of the functions here are only prototypes, and need to
//! be overrided via inherit.
class State
{
  inherit Cipher::State;
  inherit __Hash::State;

  protected void create()
  {
    /* Needed to block the default implementation in __Hash.State. */
  }
}

//! Calling `() will return a @[State] object.
State `()() {
  return State();
}

