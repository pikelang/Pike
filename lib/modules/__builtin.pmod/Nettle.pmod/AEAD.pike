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

inherit __builtin.Nettle.Hash;

//! This is the context for a single incrementally updated AEAD cipher.
//!
//! Most of the functions here are only prototypes, and need to
//! be overrided via inherit.
class State
{
  inherit Cipher::State;
  inherit Hash::State;

  protected void create(__builtin.Nettle.Cipher|
			program(__builtin.Nettle.Cipher)|function c)
  {
    /* Needed to block the default implementation in Hash.State. */
  }
}

//! Calling `() will return a @[State] object.
State `()(__builtin.Nettle.Cipher|program(__builtin.Nettle.Cipher)|function c,
	  mixed ... args) {
  return State(c, @args);
}

