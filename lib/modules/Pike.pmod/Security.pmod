#pike __REAL_VERSION__
#pragma strict_types

#if constant(__builtin.security)
// Pike internal security support.
//
// @note
//   This only exists if the run-time has been compiled with
//   @tt{--with-security@}.

//! @ignore
inherit __builtin.security;
//! @endignore

//! Virtual class for User objects, used in @[Creds] objects.
class User {

  //! This callback gets called when a new file is to be opened (and
  //! the @[Creds] object has @[BIT_CONDITIONAL_IO] set).
  //! @param type
  //! The type of file operation requested. Can either be @expr{"read"@}
  //! or @expr{"write"@}.
  //! @param current
  //! The current object, i.e. the Fd object the user is trying to open.
  //! @param filename
  //! The file name requested.
  //! @param flags
  //! The flag string passed to open, e.g. @expr{"cwt"@}.
  //! @param access
  //! The access flags requested for the file, e.g. @expr{0666@}.
  //! @returns
  //! The function can either return a string, which means that the user
  //! is allowed to open a file, but the returned file should be opened
  //! instead, or it can return an integer. The integers are intepreted
  //! as follows.
  //! @int
  //!   @value 0
  //!     The user was not allowed to open the file. ERRNO will be set
  //!     to EPERM and an exception is thrown.
  //!   @value 1
  //!     Do nothing, i.e. valid_open has initilized the @[current]
  //!     object with an open file. This can (natuarally) only be returned
  //!     if a @[current] object was given, which is not always the case.
  //!   @value 2
  //!     The user was allowed to open the file and the open code proceeds.
  //!   @value 3
  //!     The user was not allowed to open the file and an exception
  //!     is thrown.
  //! @endint
  int(0..3)|string valid_open(string type, object current,
			      string filename, string flags, int access) {
    return 3;
  }

  //! This callback gets called when I/O operations not performed on
  //! file objects are performed.
  int(0..3)|array valid_io(string fun, string type, mixed ... args) {
    return 3;
  }
}

#endif /* constant(__builtin.security) */
