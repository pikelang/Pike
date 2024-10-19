#pike __REAL_VERSION__
#pragma strict_types

//! Base class for AE (Authenticated Encryption) algorithms.
//!
//! AE algorithms behave like a combination of a @[Cipher] and
//! a HMAC.
//!
//! Note that no actual AE algorithm is implemented
//! in the base class. They are implemented in classes
//! that inherit this class.
//!
//! @seealso
//!   @[AEAD]

inherit __builtin.Nettle.Cipher;

//! Returns the size of a hash digest.
int(0..) digest_size();

//! This is the context for a single incrementally updated AE cipher.
//!
//! Most of the functions here are only prototypes, and need to
//! be overrided via inherit.
class State
{
  inherit Cipher::State;

  //! Generates a digest, and resets the hashing contents.
  //!
  //! @param length
  //!   If the length argument is provided, the digest is truncated
  //!   to the given length.
  //!
  //! @returns
  //!   The digest.
  string(8bit) digest(int|void length);

  //! Returns the size of a hash digest.
  int(0..) digest_size()
  {
    return global::digest_size();
  }

  protected void create()
  {
    /* Needed to block the default implementation in __Hash.State. */
  }
}

//! Calling `() will return a @[State] object.
protected State `()() {
  return State();
}
