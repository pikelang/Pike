#pike 7.9

/* $Id$
 */

//! Dummy HTTPS server - Compat with Pike 7.8.

#if constant(SSL.Cipher.CipherAlgorithm)

//! @decl inherit predef::SSL.https
//! Based on the current SSL.https.

inherit SSL.https : https;

//! Pike 7.8 compatibility version of @[predef::SSL.https.conn].
class conn {
  //! Based on the current SSL.https.conn.
  inherit https::conn;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(object f)
  {
    ::create(f);
  }
}

//! Pike 7.8 compatibility version of @[predef::SSL.https.no_random].
class no_random {
  //! Based on the current SSL.https.no_random.
  inherit https::no_random;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(string|void secret)
  {
    ::create(secret);
  }
}

//! create() used to be visible.
//!
//! @seealso
//!   @[::create()]
void create()
{
  https::create();
}

#else // constant(SSL.Cipher.CipherAlgorithm)
constant this_program_does_not_exist = 1;
#endif
