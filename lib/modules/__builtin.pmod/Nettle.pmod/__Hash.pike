#pike __REAL_VERSION__
#pragma strict_types

//! Base class for hash algorithms.
//!
//! Note that no actual hash algorithm is implemented
//! in the base class. They are implemented in classes
//! that inherit this class.

//! Returns a human readable name for the algorithm.
string(7bit) name();

//! Returns the size of a hash digest.
int(0..) digest_size();

//! Returns the internal block size of the hash algorithm.
int(1..) block_size();

//! This is the context for a single incrementally updated hash.
//!
//! Most of the functions here are only prototypes, and need to
//! be overrided via inherit.
class State
{
  //! Create the new context, and optionally
  //! add some initial data to hash.
  //!
  //! The default implementation calls @[update()] with @[data] if any,
  //! so there's usually no reason to override this function, since
  //! overriding @[update()] should be sufficient.
  protected void create(string(8bit)|void data)
  {
    if (data) {
      update([string]data);
    }
  }

  //! Reset the context, and optionally
  //! add some initial data to the hash.
  this_program init(string(8bit)|void data);

  //! Add some more data to hash.
  this_program update(string(8bit) data);

  //! Generates a digest, and resets the hashing contents.
  //!
  //! @param length
  //!   If the length argument is provided, the digest is truncated
  //!   to the given length.
  //!
  //! @returns
  //!   The digest.
  string(8bit) digest(int|void length);

  //! Returns a human readable name for the algorithm.
  string(7bit) name()
  {
    return global::name();
  }

  //! Returns the size of a hash digest.
  int(0..) digest_size()
  {
    return global::digest_size();
  }

  //! Returns the internal block size of the hash algorithm.
  int(1..) block_size()
  {
    return global::block_size();
  }
}
