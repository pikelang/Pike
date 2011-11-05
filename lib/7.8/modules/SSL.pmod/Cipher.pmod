#pike 7.9

#if constant(Crypto.Hash)

//! Encryption and MAC algorithms used in SSL - Compat with Pike 7.8.

//! @decl inherit predef::SSL.Cipher
//! Based on the current SSL.Cipher.

inherit SSL.Cipher : Cipher;

//! Pike 7.8 compatibility version of @[predef::SSL.Cipher.DES].
class DES
{
  //! Based on the current SSL.Cipher.DES.
  inherit Cipher::DES;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create()
  {
    ::create();
  }
}

//! Pike 7.8 compatibility version of @[predef::SSL.Cipher.DES3].
class DES3
{
  //! Based on the current SSL.Cipher.DES3.
  inherit Cipher::DES3;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create()
  {
    ::create();
  }
}

//! Pike 7.8 compatibility version of @[predef::SSL.Cipher.IDEA].
class IDEA
{
  //! Based on the current SSL.Cipher.IDEA.
  inherit Cipher::IDEA;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create()
  {
    ::create();
  }
}

//! Pike 7.8 compatibility version of @[predef::SSL.Cipher.AES].
class AES
{
  //! Based on the current SSL.Cipher.AES.
  inherit Cipher::AES;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create()
  {
    ::create();
  }
}

//! Pike 7.8 compatibility version of @[predef::SSL.Cipher.DHKeyExchange].
class DHKeyExchange
{
  //! Based on the current SSL.Cipher.DHKeyExchange.
  inherit Cipher::DHKeyExchange;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(DHParameters p)
  {
    ::create(p);
  }
}

#else // constant(Crypto.Hash)
constant this_program_does_not_exist = 1;
#endif
